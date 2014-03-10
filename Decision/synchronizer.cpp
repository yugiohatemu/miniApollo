//
//  synchronizer.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-04.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "synchronizer.h"
#include "peer.h"
#include "log.h"
#include <boost/date_time.hpp>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <algorithm>

#include "syncPoint.h"
#include "syncHeader.h"

const unsigned int BB_SIZE = 13;

Synchronizer::Synchronizer(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list,PriorityPeer * priority_peer):
    io_service(io_service),
    strand(strand),
    peer_list(peer_list),
    pid(pid),
    priority_peer(priority_peer),
    t_broadcast(io_service, boost::posix_time::seconds(0)),
    t_clean_up(io_service, boost::posix_time::seconds(0))
{

}


Synchronizer::~Synchronizer(){

    Log::log().Print("Peer # %d summary\nBackBundle Region\n",pid);
    
    for(unsigned int i = 0; i < sync_region;i++){
        delete se_list[i];
    }
    Log::log().Print("Active Sync Region\n");
    for(unsigned int i =  sync_region; i < se_list.size(); i++){
        Log::log().Print("%d - %lld\n",i, se_list[i]->ts);
        delete se_list[i];
    }
}

void Synchronizer::stop(){
    t_clean_up.cancel();
    t_broadcast.cancel();
    
    BB_started = false;
}

void Synchronizer::start(){
    t_broadcast.expires_from_now(boost::posix_time::seconds(5));
    t_broadcast.async_wait(strand.wrap(boost::bind(&Synchronizer::broadcast,this, boost::asio::placeholders::error)));
    
    t_clean_up.expires_from_now(boost::posix_time::seconds(5));
    t_clean_up.async_wait(strand.wrap(boost::bind(&Synchronizer::search_good_peer,this, boost::asio::placeholders::error)));
    
    BB_started = false;
    
}

void Synchronizer::broadcast(boost::system::error_code error){
    if(error == boost::asio::error::operation_aborted ) return;
    
    if (se_list.empty()) {
        Log::log().Print("Peer # %d broadcasting sync 0\n", pid);
    }else{
        Log::log().Print("Peer # %d broadcasting sync 1: %lld - %d: %lld\n",pid, se_list[sync_region]->ts, se_list.size(), se_list.back()->ts);
    }
    
    //Header first
    int header_region = sync_region - 1;
    if (header_region > 0) {
        SyncHeader * sh = dynamic_cast<SyncHeader *>(se_list[header_region]);
        if (sh) {
            //do what
            BackBundle::Header header = sh->get_header();
            for (unsigned int i =0 ; i < peer_list.size(); i++) {
                if (i != pid) peer_list[i]->enqueue(boost::protect(boost::bind(&Synchronizer::sync_header, peer_list[i]->synchronizer, header)));
            }
        }
    }
    //Active region 2nd
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i != pid) peer_list[i]->enqueue(boost::protect(boost::bind(&Synchronizer::sync, peer_list[i]->synchronizer, pid)));
    }
    
    
    t_broadcast.expires_from_now(boost::posix_time::seconds(7));
    t_broadcast.async_wait(strand.wrap(boost::bind(&Synchronizer::broadcast,this,boost::asio::placeholders::error)));
    
}

void Synchronizer::search_good_peer(boost::system::error_code e){
    if(e == boost::asio::error::operation_aborted ) return;
    
    boost::mutex::scoped_lock lock(mutex);
    
    if (se_list.size() -  sync_region > BB_SIZE && !BB_started) {
        int header_region = sync_region - 1;
        BB_started = true;
        if (header_region >= 0) {
            SyncHeader * sh =  dynamic_cast<SyncHeader *>(se_list[header_region]);
            if (sh && sh->is_synced()) peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::start_bully, peer_list[pid], error)));
        }else{
            //No BB has created or existed for now
            peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::start_bully, peer_list[pid], error)));
        }
        
    }
    
    t_clean_up.expires_from_now(boost::posix_time::seconds(7));
    t_clean_up.async_wait(strand.wrap(boost::bind(&Synchronizer::search_good_peer,this, boost::asio::placeholders::error)));
    
}

void Synchronizer::good_peer_first(){
    //create our BB bundle
    boost::mutex::scoped_lock lock(mutex);
    if (se_list.size() - sync_region < BB_SIZE) {
        BB_started = false;
        return ;
    }
    
    SyncHeader * sh = new SyncHeader(se_list, sync_region, BB_SIZE); // how do I fill in now?
    se_list.insert(se_list.begin() + sync_region,sh);
    sync_region++;
    
    remove_empty_se();
    BB_started = false;
    
    Log::log().Print("Peer # %d syncrhonizer create BB %d\n",pid, BB_SIZE);
    
    BackBundle::Header header = sh->get_header();
    //Give others Header first
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i != pid)
            peer_list[i]->enqueue(boost::protect(boost::bind(&Synchronizer::add_new_header, peer_list[i]->synchronizer, header)));
    }
}


///////////////////////////////////////////////////////////////
#pragma mark - sync functionaliry


void Synchronizer::sync(unsigned int peer_id){
    boost::mutex::scoped_lock lock(mutex);
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    
    if (BB_started) {
        int header_region = sync_region -1;
        if(header_region < 0) return;
        SyncHeader * sh = dynamic_cast<SyncHeader *>(se_list[header_region]);
        if(!sh) return;
        //get SyncHeader based on Header?
        SyncHeader * peer_sh =  peer_list[peer_id]->synchronizer->get_sync_header_with(sh->get_header());
        if(!peer_sh) return;
        sh->sync_with_other_header(peer_sh);
        
        if(sh->is_synced()) BB_started = false;
        
    }else{
        std::vector<SyncEntry *> peer_sp_list = peer_list[peer_id]->synchronizer->get_se_list();
        unsigned int peer_sync_region = peer_list[peer_id]->synchronizer->get_sync_region();
        
        for (unsigned int i = peer_sync_region; i < peer_sp_list.size(); i++) {
            uint64_t ts = peer_sp_list[i]->ts;
            if (!has_ts(ts)) {
                se_list.insert(se_list.begin() + find_insert_pos(ts), new SyncPoint(ts));
            }
        }
    }
}

void Synchronizer::sync_header(BackBundle::Header header ){
    bool found = false;
    for(int i = sync_region - 1; i >= 0; i--){
        SyncHeader * sh = dynamic_cast<SyncHeader *>(se_list[i]);
        if(sh && sh->get_header() == header) {
            found = true;
            break;
        }
    }
    
    if (!found ) {
        se_list.insert(se_list.begin() + sync_region, new SyncHeader(header));
        sync_region++;
        remove_empty_se();
        BB_started = true;
        Log::log().Print("Peer # %d get sync header\n",pid);
        peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::cancel_bully, peer_list[pid], error)));
    }
}

void Synchronizer::add_new_se(){
    boost::mutex::scoped_lock lock(mutex);
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    se_list.push_back(new SyncPoint());
    priority_peer->add_ts(se_list.back()->ts);
}


void Synchronizer::add_new_header(BackBundle::Header header){ //broad cast or get???
    boost::mutex::scoped_lock lock(mutex);
    
    SyncHeader * sh = new SyncHeader(header);
    sh->sync_with_self(se_list, sync_region);
    se_list.insert(se_list.begin() + sync_region,sh);
    sync_region++;
    remove_empty_se();
    
    
    if (sh->is_synced()) BB_started = false;
    
    Log::log().Print("Peer # %d add new header and sync self\n",pid);
}

////////////////////////////////////////////////
#pragma mark - helper
SyncHeader * Synchronizer::get_sync_header_with(BackBundle::Header header){
    for(int i = sync_region - 1; i >= 0; i--){
        SyncHeader * sh = dynamic_cast<SyncHeader *>(se_list[i]);
        if(sh && sh->get_header() == header) return sh;
    }
    return NULL;
}

std::vector<SyncEntry *>& Synchronizer::get_se_list(){
    return se_list;
}

unsigned int Synchronizer::get_sync_region(){
    return sync_region;
}

bool Synchronizer::has_ts(uint64_t ts){
    if (ts == 0) return true;
    
    for (unsigned int i = sync_region; i < se_list.size(); i++) {
        if (se_list[i]->ts == ts) return true;
    }
    return false;
}

unsigned int Synchronizer::find_insert_pos(uint64_t ts){
    for (unsigned int i = sync_region; i < se_list.size(); i++) {
        if (ts < se_list[i]->ts) return i;
    }
    return (unsigned int)se_list.size();
}

void Synchronizer::remove_empty_se(){
    std::vector<SyncEntry *>::iterator it= se_list.begin() + sync_region;
    while (it != se_list.end()) {
        if ((*it)->ts == 0) it = se_list.erase(it);
        else it++;
    }
}
