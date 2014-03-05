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
#include <algorithm>
#include "sync_c.h"
#include "bbSynchronizer.h"

Synchronizer::Synchronizer(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list):
    io_service(io_service),
    strand(strand),
    peer_list(peer_list),
    pid(pid),
    t_broadcast(io_service, boost::posix_time::seconds(0))
{
    bb_synchronizer = NULL;
}


Synchronizer::~Synchronizer(){
    bb_synchronizer = NULL;
    Log::log().Print("Peer # %d synchronizer has active items %lld\n", pid, ts_list.size());
}

void Synchronizer::stop(){
    t_broadcast.cancel();
}

void Synchronizer::start(){
    t_broadcast.expires_from_now(boost::posix_time::seconds(5));
    t_broadcast.async_wait(strand.wrap(boost::bind(&Synchronizer::broadcast,this, boost::asio::placeholders::error)));
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

void Synchronizer::broadcast(boost::system::error_code error){
    if(error == boost::asio::error::operation_aborted ) return;
    
    if (ts_list.empty()) {
        Log::log().Print("Peer # %d broadcasting sync 0\n", pid);
    }else{
        Log::log().Print("Peer # %d broadcasting sync 1: %lld - %d: %lld\n",pid, ts_list.front(), ts_list.size(), ts_list.back());
    }
    
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i != pid) peer_list[i]->io_service.post(boost::bind(&Synchronizer::sync, peer_list[i]->synchronizer, ts_list));
    }
    
    t_broadcast.expires_from_now(boost::posix_time::seconds(5));
    t_broadcast.async_wait(strand.wrap(boost::bind(&Synchronizer::broadcast,this,boost::asio::placeholders::error)));
    
}


void Synchronizer::sync(std::vector<uint64_t> &other_list){
    boost::mutex::scoped_lock lock(mutex);
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    
    if (!bb_synchronizer || bb_synchronizer->is_BB_empty()) return ;
    
    //try find that in BB first
    std::vector<uint64_t> diff;
    
    for(unsigned int i = 0; i < other_list.size();i++){
        if (!bb_synchronizer->is_ts_in_BB(other_list[i])) {
            diff.push_back(other_list[i]);
        }
    }
    
    if (!diff.empty()) sync_c(ts_list,diff);
    //TODO: we can do a more careful analysis here, even ask it to BB to sync faster
}
