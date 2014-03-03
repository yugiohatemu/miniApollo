//
//  peer.cpp
//  Decision
//
//  Created by Yue Huang on 2014-02-27.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "peer.h"
#include <iostream>
#include <boost/date_time.hpp>
#include <boost/bind.hpp>
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include "log.h"

Peer::Peer(unsigned int pid,std::vector<Peer *> &peer_list,DataPool * data_pool):
    pid(pid), peer_list(peer_list),data_pool(data_pool),
    work(io_service), strand(io_service),
    t_broadcast(io_service, boost::posix_time::seconds(0)),
    t_new_feed(io_service, boost::posix_time::seconds(0)),
    t_on_off_line(io_service, boost::posix_time::seconds(15)),
    t_bully_other(io_service, boost::posix_time::seconds(0)),
    t_being_bully(io_service, boost::posix_time::seconds(0)),
    t_miscellaneous(io_service, boost::posix_time::seconds(0)),
    t_try_bb(io_service, boost::posix_time::seconds(0))
{
        //Init timers for that
    availability =  (float)(rand() % 10 + 1) / 10;
    bb_count = 0;
    bb_synchronizer = new BB_Synchronizer();
    
    for (unsigned int i = 0; i < 3; i++) {
        thread_group.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
    }
    
    Log::log().Print("Peer # %d init with availability %f\n",pid,availability);
    
    srand(time(NULL));
    
    io_service.post(boost::bind(&Peer::get_online, this));
}

Peer::~Peer(){
    io_service.stop();
    //cancel all the tasks
    cancel_all();
    thread_group.join_all();
    
    delete bb_synchronizer;
    Log::log().Print("Peer # %d deleted with packed bb # %d\n",pid, bb_count);
   
}

void Peer::cancel_all(){
    online = false;
    
    t_bully_other.cancel();
    t_being_bully.cancel();
    
    t_broadcast.cancel();
    t_new_feed.cancel();
    
    t_miscellaneous.cancel();
    t_try_bb.cancel();
    
    state = NOTHING;
}

void Peer::get_offline(){
    cancel_all();
    
    Log::log().Print("Peer # %d get offline\n",pid);
    
    t_on_off_line.expires_from_now(boost::posix_time::seconds((int) 20 *(1-availability) + rand() % 10));
    t_on_off_line.async_wait(strand.wrap(boost::bind(&Peer::get_online,this)));
}

void Peer::get_online(){
    online = true;
    state = NOTHING;
    
    t_new_feed.expires_from_now(boost::posix_time::seconds(rand() % 6 + 2));
    t_new_feed.async_wait(strand.wrap(boost::bind(&Peer::new_feed,this)));
    
    t_broadcast.expires_from_now(boost::posix_time::seconds(5));
    t_broadcast.async_wait(strand.wrap(boost::bind(&Peer::broadcast,this)));
    
    t_try_bb.expires_from_now(boost::posix_time::seconds(15));
    
    io_service.post(strand.wrap(boost::bind(&Peer::finish_bully,this, error)));
    
    //    io_service.post(boost::bind(&Peer::new_feed, this));
    //    io_service.post(boost::bind(strand.wrap(&Peer::broadcast, this)));
    
    Log::log().Print("Peer # %d get online\n",pid);
    
    t_on_off_line.expires_from_now(boost::posix_time::seconds((int) 20 * availability + rand() % 5));
    t_on_off_line.async_wait(boost::bind(&Peer::get_offline,this));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - Base peer function
//timeed function series

void Peer::sync(std::vector<uint64_t> &sync_list){
    if (!online) return;
    //we have other list to sync with our current list
    //when we sync, we need to beaware of exitence of BB
    
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    
    boost::mutex::scoped_lock lock(sync_lock);
    
    std::vector<uint64_t> extra;
    for (unsigned int i = 0; i < sync_list.size(); i++) {
        
        bool found = std::binary_search(synchronizer.begin(), synchronizer.end(), sync_list[i]);
        // != synchronizer.end();
        if (!found) { //try back bundle
            if (bb_synchronizer->is_ts_in_BB(sync_list[i])) found = true;
        }
        if (!found) extra.push_back(sync_list[i]);
        
    }
    
    //since we are modifying synchronizer
    if (!extra.empty()) {
        for (int i = 0; i < extra.size(); i++) synchronizer.push_back(extra[i]);
        std::sort(synchronizer.begin(), synchronizer.end());
    }
    //easy way is to find the sync that is not in our current list
    
}

void Peer::broadcast(){
    if (!online) return;
    
    
    if (synchronizer.size() != 0) {
        Log::log().Print("Peer # %d broadcasting sync 1: %lld - %d: %lld\n",pid, synchronizer[0], synchronizer.size(), synchronizer.back());
    }else{
        Log::log().Print("Peer # %d broadcasting sync 0\n", pid);
    }
    
    
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i != pid) {
            peer_list[i]->io_service.post(boost::bind(&Peer::sync, peer_list[i], synchronizer));
        }
    }
    t_broadcast.expires_from_now(boost::posix_time::seconds(5));
    t_broadcast.async_wait(strand.wrap(boost::bind(&Peer::broadcast,this)));
}

void Peer::new_feed(){
    if (!online) return ;
    
    struct timeval tp;
    gettimeofday(&tp, NULL);
    uint64_t current_time = tp.tv_sec * 1e6 + tp.tv_usec;
    
    sync_lock.lock();
    synchronizer.push_back(current_time);
    data_pool->add_ts(current_time);
    sync_lock.unlock();
    
    t_new_feed.expires_from_now(boost::posix_time::seconds(rand() % 6 + 2));
    t_new_feed.async_wait(strand.wrap(boost::bind(&Peer::new_feed,this)));
    
}

void Peer::read_feed(){
    if (!online) return ;
    
    uint64_t ts =  data_pool->get_random_ts();
    if(std::binary_search(synchronizer.begin(), synchronizer.end(), ts)){
        //data cache increase by 1
        //also, the data before that should also be increased by 1
        
    }else{
        //either not synced or it is BBundeled
    }
    
    
    //select a random, and enumerate through our cache
    
}

void Peer::try_bb(const boost::system::error_code &e){
    if(e == boost::asio::error::operation_aborted || !online ){
        Log::log().Print("Peer # %d cancel try_bb\n",pid);
        return ;
    }
    
    if (bb_synchronizer->is_empty() || bb_synchronizer->is_BB_synced()) {
        t_try_bb.expires_from_now(boost::posix_time::seconds(15));
        t_try_bb.async_wait(strand.wrap(boost::bind(&Peer::finish_bully,this, boost::asio::placeholders::error)));
    }else{
        
        if (synchronizer.size() > BB_SIZE && state == NOTHING ) {
            state = BULLY_OTHER;
            io_service.post(strand.wrap(boost::bind(&Peer::start_bully, this, error)));
        }else{
            //frsit, try to create a header cotent based on unfinished header
            
            //then compare it
            
            //if equal, pack_bb
        }
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - Bully algorithm
void Peer::start_bully(const boost::system::error_code &e){
    if (e == boost::asio::error::operation_aborted || !online || state == DELAY) {
        Log::log().Print("Peer # %d cancel start bully\n",pid);
        return ;
    }
    
    t_bully_other.cancel();
    t_being_bully.cancel();
    
    state = BULLY_OTHER;
    
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i != pid) {
            peer_list[i]->io_service.post(boost::bind(&Peer::get_bullyed, peer_list[i], Peer::Message(pid, availability)));
        }
    }
    
    t_bully_other.expires_from_now(boost::posix_time::seconds(3));
    t_bully_other.async_wait(strand.wrap(boost::bind(&Peer::finish_bully,this, boost::asio::placeholders::error)));
    
    Log::log().Print("Peer # %d start bully with %d\n",pid,synchronizer.size());
}


void Peer::get_bullyed(Peer::Message m){
    if (!online) return ;
    
    //simulate delay from network
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    
    if(m.weight < availability || (m.weight == availability && pid < m.p)){
        
        state = BULLY_OTHER;
        t_being_bully.cancel(); //might not cancel that
        
        Log::log().Print("Peer # %d start bully back %d\n",pid,m.p);
        
        io_service.post(strand.wrap(boost::bind(&Peer::start_bully,this,error)));
        //the start is put back?
        
    }else{
        state = BEING_BULLIED;
        Log::log().Print("Peer # %d being bullied by %d\n",pid,m.p);
        
        t_bully_other.cancel();
       
        t_being_bully.expires_from_now(boost::posix_time::seconds(3));
        t_being_bully.async_wait(strand.wrap(boost::bind(&Peer::start_bully,this,boost::asio::placeholders::error)));
        

    }
}

void Peer::finish_bully(const boost::system::error_code &e){
    if (e == boost::asio::error::operation_aborted || !online) {
        Log::log().Print("Peer # %d cancel finish bully\n",pid);
        return ;
    }
    
    //create header
    
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        peer_list[i]->io_service.post(boost::bind(&Peer::pack_bb,peer_list[i], error)); //pass in Header here
    }
    
    Log::log().Print("Peer # %d finish bully\n",pid);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - Backbunlde and Header
void Peer::pack_bb(const boost::system::error_code &e){
    if (e == boost::asio::error::operation_aborted || !online){
        Log::log().Print("Peer # %d cancel pack BB\n",pid);
        return ;
    }
    
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    
    t_being_bully.cancel();
    t_bully_other.cancel();
    t_miscellaneous.cancel();
    
    {
        boost::mutex::scoped_lock scoped_lock(sync_lock);
        
        //Tehcnically, we should use a bundle header here or something
        if (synchronizer.size() < BB_SIZE || state == DELAY) {
            Log::log().Print("Peer # %d not synced for BB pack\n",pid);

            //delayed until  being synced
            state = DELAY;
            
            t_miscellaneous.expires_from_now(boost::posix_time::seconds(5));
            t_miscellaneous.async_wait(boost::bind(&Peer::pack_bb, this, boost::asio::placeholders::error));
            
        }else{
            Log::log().Print("Peer # %d pack bb %d\n",pid,bb_count);
            bb_count++;
            //create BB
            bb_synchronizer->add_bb( new BackBundle(synchronizer, BB_SIZE));
            
            synchronizer.erase(synchronizer.begin(), synchronizer.begin() + BB_SIZE); //do a more careful removal later
            //also, cleans the cache
            
            state = NOTHING;
            
            io_service.post(boost::bind(&Peer::broadcast_header, this));
            
        }
    }
}

//broadcast header...if sent
//maybe for several time for active sync
//which is also the same amount of time that it is in harddisk
//if empty then we must add
//else not
void Peer::get_header(BackBundle::Header header){
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    
    if (bb_synchronizer->is_empty() || !bb_synchronizer->is_header_in_bb(header)) {
        bb_synchronizer->add_empty_bb_with_header(header);
    }
}

//we need a counter, we don't care about time
//we only broadcast the latest one
void Peer::broadcast_header(){
    if (bb_synchronizer->is_empty()) return ;
    //BroadCast for a certain amount of timer ?
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if(i != pid)
        peer_list[i]->io_service.post(boost::bind(&Peer::get_header,peer_list[i], BackBundle::Header( bb_synchronizer->get_latest_header())));
    }
}
