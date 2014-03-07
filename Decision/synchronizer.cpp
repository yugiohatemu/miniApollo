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
//#include "sync_c.h"
#include "bbSynchronizer.h"

const int BB_SIZE = 7;

Synchronizer::Synchronizer(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list):
    io_service(io_service),
    strand(strand),
    peer_list(peer_list),
    pid(pid),
    t_broadcast(io_service, boost::posix_time::seconds(0)),
    t_clean_up(io_service, boost::posix_time::seconds(0))
{
    bb_synchronizer = NULL;
}


Synchronizer::~Synchronizer(){
    bb_synchronizer = NULL;
    Log::log().Print("Peer # %d synchronizer has active items %lld\n", pid, ts_list.size());
    for(unsigned int i =  0; i < ts_list.size();i++){
        Log::log().Print("%d - %lld\n",i, ts_list[i]);
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
    
    t_clean_up.expires_from_now(boost::posix_time::seconds(7));
    t_clean_up.async_wait(strand.wrap(boost::bind(&Synchronizer::clean_up,this, boost::asio::placeholders::error)));
    
    BB_started = false;
    
}

unsigned int Synchronizer::size(){
    return (int)ts_list.size();
}

void Synchronizer::add_ts(uint64_t ts){
    ts_list.push_back(ts);
}

void Synchronizer::set_ts_list(std::vector<uint64_t> &list){
    ts_list = list;
}

std::vector<uint64_t> & Synchronizer::get_ts_list(){
    return ts_list;
}
///////////////////
#pragma mark - timer functionality
void Synchronizer::broadcast(boost::system::error_code error){
    if(error == boost::asio::error::operation_aborted ) return;
    
    if (ts_list.empty()) {
        Log::log().Print("Peer # %d broadcasting sync 0\n", pid);
    }else{
        Log::log().Print("Peer # %d broadcasting sync 1: %lld - %d: %lld\n",pid, ts_list.front(), ts_list.size(), ts_list.back());
    }
    
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i != pid)
            peer_list[i]->enqueue(boost::protect(boost::bind(&Synchronizer::sync, peer_list[i]->synchronizer, ts_list)));
    }
    
    t_broadcast.expires_from_now(boost::posix_time::seconds(7));
    t_broadcast.async_wait(strand.wrap(boost::bind(&Synchronizer::broadcast,this,boost::asio::placeholders::error)));
    
}


void Synchronizer::sync(std::vector<uint64_t> &other_list){
    boost::mutex::scoped_lock lock(mutex);
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    
    //try find that in BB first
    std::vector<uint64_t> diff;
    
    for(unsigned int i = 0; i < other_list.size();i++){
        if (!bb_synchronizer->is_ts_in_BB(other_list[i])) diff.push_back(other_list[i]);
    }
    
    if (diff.empty()) return;
    
    //TODO: we can do a more careful analysis here, even ask it to BB to sync faster
    for (unsigned int i = 0; i < diff.size(); i++) {
        bool found = std::binary_search(ts_list.begin(), ts_list.end(), diff[i]);
        if (!found) ts_list.push_back(diff[i]);
    }
    
    std::sort(ts_list.begin(), ts_list.end());
}


void Synchronizer::clean_up(boost::system::error_code e){
    if(e == boost::asio::error::operation_aborted ) return;
    
    boost::mutex::scoped_lock lock(mutex);
    if (ts_list.size() > BB_SIZE && !BB_started) {
        if (bb_synchronizer->is_BB_empty() || bb_synchronizer->is_BB_synced()) {
            BB_started = true;
            peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::start_bully, peer_list[pid], error)));
        }else{
            //maybe syncs up more frequently
        }
    }else{
        t_clean_up.expires_from_now(boost::posix_time::seconds(7));
        t_clean_up.async_wait(strand.wrap(boost::bind(&Synchronizer::clean_up,this, boost::asio::placeholders::error)));
    }
}

void Synchronizer::first_clean_up(){
    //create our BB bundle
    boost::mutex::scoped_lock lock(mutex);
    unsigned int t = std::min((int)ts_list.size()-1, BB_SIZE);
    Log::log().Print("Peer # %d syncrhonizer create BB with size %d\n",pid, t);
    bb_synchronizer->add_BB(new BackBundle(ts_list, t));
    ts_list.erase(ts_list.begin(), ts_list.begin() + t);
}

void Synchronizer::clean_up_done(){
    BB_started = false;
  
    t_clean_up.expires_from_now(boost::posix_time::seconds(7));
    t_clean_up.async_wait(strand.wrap(boost::bind(&Synchronizer::clean_up,this, boost::asio::placeholders::error)));
}

void Synchronizer::remove_dup(std::vector<uint64_t> list){
    boost::mutex::scoped_lock lock(mutex);
    for (unsigned int i = 0; i < list.size(); i++) {
        std::vector<uint64_t>:: iterator it = std::find(ts_list.begin(), ts_list.end(), list[i]);
        if (it != ts_list.end()) {
            ts_list.erase(it);
        }
    }
}
