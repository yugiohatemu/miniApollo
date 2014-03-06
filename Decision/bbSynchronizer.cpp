//
//  bbSynchronizer.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-03.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "bbSynchronizer.h"
#include "peer.h"
#include "log.h"
#include "sync_c.h"
#include "synchronizer.h"
#include <boost/bind/protect.hpp>

BB_Synchronizer::BB_Synchronizer(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list):
    io_service(io_service),
    strand(strand),
    peer_list(peer_list),
    pid(pid),
    t_broadcast(io_service, boost::posix_time::seconds(0))
{
    
}

BB_Synchronizer::~BB_Synchronizer(){
    Log::log().Print("Peer # %d bb_synchronizer has bb %lld\n", pid, bb_list.size());
    for (unsigned int i = 0; i < bb_list.size(); i++){
//        Log::log().Print("Peer # bb %d with %lld items synced %c\n",i,bb_list[i]->size(),(bb_list[i]->is_bb_synced() ? 'y':'n'));
        delete bb_list[i];
    }

}

void BB_Synchronizer::stop(){
    t_broadcast.cancel();
}

void BB_Synchronizer::start(){
    t_broadcast.expires_from_now(boost::posix_time::seconds(5));
    t_broadcast.async_wait(strand.wrap(boost::bind(&BB_Synchronizer::broadcast,this, boost::asio::placeholders::error)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FOR NOW
//only send out the last header because we do not allow further BB unless the latest one is synced
void BB_Synchronizer::broadcast(boost::system::error_code error){
    if(error == boost::asio::error::operation_aborted ) return;
    
    Log::log().Print("Peer # %d broadcasting bb header %lld\n",pid, bb_list.size());
    if(!is_BB_empty()){
        for (unsigned int i = 0; i < peer_list.size(); i++) {
            if (i != pid){
                for (unsigned int j = 0; j < bb_list.size(); j++) {
                    peer_list[i]->enqueue(boost::protect(boost::bind(&BB_Synchronizer::sync, peer_list[i]->bb_synchronizer, Message(bb_list[j]->current,pid))));
                }
            }
        }
    }
   
    t_broadcast.expires_from_now(boost::posix_time::seconds(7));
    t_broadcast.async_wait(strand.wrap(boost::bind(&BB_Synchronizer::broadcast,this,boost::asio::placeholders::error)));

}
void BB_Synchronizer::sync(Message m){
    boost::mutex::scoped_lock lock(mutex);
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    
    if (bb_list.empty() || is_header_existed(m.h)) {
        bb_list.push_back(new BackBundle(m.h));
    }
    
    if (!is_BB_synced()) {
        //TODO: cancel it in case for now
        peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::cancel_bully,peer_list[pid], error)));
        peer_list[pid]->enqueue(boost::protect(boost::bind(&BB_Synchronizer::sync_bb ,this, error)));
    }
}

void BB_Synchronizer::sync_bb(boost::system::error_code error){
    if(error == boost::asio::error::operation_aborted ) return;
    
    for (unsigned int i = 0; i < bb_list.size(); i++) {
        if (!bb_list[i]->is_bb_synced()) {
            for (unsigned int j = 0; j < peer_list.size(); j++) {
                if (pid != j) {
                    BackBundle * bb = bb_list[i];
                    BackBundle * other_bb = peer_list[j]->bb_synchronizer->get_BB_with_header(bb->target);
                    if (other_bb){
                        bb->sync(other_bb);
                        synchronizer->remove_dup(bb->get_list());
                    }
                }
                if (bb_list[i]->is_bb_synced()) break;
            }
        }
    }
    
}



BackBundle * BB_Synchronizer::get_BB_with_header(BackBundle::Header h){
    for (unsigned int i = 0; i < bb_list.size(); i++) {
        if (bb_list[i]->target == h) {
            return bb_list[i];
        }
    }
    return NULL;
}

bool BB_Synchronizer::is_header_existed(BackBundle::Header h){
    for (unsigned int i = 0; i < bb_list.size(); i++) {
        if (h == bb_list[i]->target) {
            return true;
        }
    }
    return false;
}

void BB_Synchronizer::add_BB(BackBundle * bb){
    boost::mutex::scoped_lock lock(mutex);
    bb_list.push_back(bb);
}

void BB_Synchronizer::add_empty_BB_with_header(BackBundle::Header &h){
    boost::mutex::scoped_lock lock(mutex);
    bb_list.push_back(new BackBundle(h));
}

bool BB_Synchronizer::is_BB_empty(){
    return bb_list.empty();
}

bool BB_Synchronizer::is_BB_synced(){
    for (unsigned int i = 0;  i < bb_list.size(); i++) {
        if (!bb_list[i]->is_bb_synced()) {
            return false;
        }
    }
    return true;
}

//In future it will be probability instead of true or false
bool BB_Synchronizer::is_ts_in_BB(uint64_t ts){
    for (unsigned int i = 0; i < bb_list.size(); i++){
        if (bb_list[i]->is_ts_in_bb(ts)) {
            return true;
        }
    }
    return false;
}
