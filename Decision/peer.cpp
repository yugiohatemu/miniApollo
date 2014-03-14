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
#include <boost/bind/protect.hpp>
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include "AROLog.h"
#include <sstream>

const unsigned int BULLY_TIME_OUT = 7;

Peer::Peer(unsigned int pid,std::vector<Peer *> &peer_list,PriorityPeer * priority_peer):
    pid(pid), peer_list(peer_list),priority_peer(priority_peer),
    work(io_service), strand(io_service),
    t_new_feed(io_service, boost::posix_time::seconds(0)),
    t_on_off_line(io_service, boost::posix_time::seconds(15)),
    t_bully_other(io_service, boost::posix_time::seconds(0)),
    t_being_bully(io_service, boost::posix_time::seconds(0))
{
        //Init timers for that
    srand(time(NULL));

    availability =  (float)(rand() % 10 + 1) / 10;
    if(availability < 0.8) availability = 0.8;
    
    std::stringstream ss; ss<<"Peer# "<<pid;
    tag = ss.str();
    
    action_list = new ActionList(io_service,strand,pid, peer_list,priority_peer );
    
    for (unsigned int i = 0; i < 3; i++) {
        thread_group.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
    }
    
    AROLog::Log().Print(logINFO, 1, tag.c_str(), "Init with availability %f\n",availability);
    
    

    io_service.post(boost::bind(&Peer::get_online, this));
//    enqueue(boost::protect(boost::bind(&Peer::new_feed,this,error)));
}

Peer::~Peer(){
    io_service.stop();
    //cancel all the tasks
    cancel_all();

    thread_group.join_all();
    
    delete action_list;
    AROLog::Log().Print(logINFO, 1, tag.c_str(), "-------------------\n");
}

void Peer::cancel_all(){
    online = false;
    action_list->pause();
    
    t_bully_other.cancel();
    t_being_bully.cancel();
    
    t_new_feed.cancel();
    
    state = NOTHING;
    
}

void Peer::get_offline(){
    cancel_all();
    
    AROLog::Log().Print(logDEBUG, 1, tag.c_str(), "Get offline\n");
    
    t_on_off_line.expires_from_now(boost::posix_time::seconds((int) 30 *(1-availability) + rand() % 10));
    t_on_off_line.async_wait(strand.wrap(boost::bind(&Peer::get_online,this)));
    
}

void Peer::get_online(){
    online = true;
    state = NOTHING;
    
    action_list->resume();
    
    t_new_feed.expires_from_now(boost::posix_time::seconds(rand() % 6 + 5));
    t_new_feed.async_wait(strand.wrap(boost::bind(&Peer::new_feed,this,boost::asio::placeholders::error)));
    
    AROLog::Log().Print(logDEBUG, 1, tag.c_str(), "Get online\n");
    
    t_on_off_line.expires_from_now(boost::posix_time::seconds((int) 20 * availability + rand() % 5));
    t_on_off_line.async_wait(boost::bind(&Peer::get_offline,this));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - Base peer function
//timeed function series


void Peer::new_feed(const boost::system::error_code &e){
    if (!online || e == boost::asio::error::operation_aborted) return ;
  
    action_list->add_new_action();
    
    t_new_feed.expires_from_now(boost::posix_time::seconds(rand() % 6 + 5));
    t_new_feed.async_wait(strand.wrap(boost::bind(&Peer::new_feed,this, boost::asio::placeholders::error)));
    
}

void Peer::read_feed(){
    if (!online) return ;
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - Bully algorithm
void Peer::start_bully(const boost::system::error_code &e){
    if (e == boost::asio::error::operation_aborted || !online) {
        return ;
    }
    
    t_bully_other.cancel();
    t_being_bully.cancel();
    
    state = BULLY_OTHER;
    AROLog::Log().Print(logINFO, 1,tag.c_str(), "Start bully\n");
    
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i != pid) {
            peer_list[i]->enqueue(boost::protect(boost::bind(&Peer::get_bullyed, peer_list[i], Peer::Message(pid, availability))));
        }
    }
    
    t_bully_other.expires_from_now(boost::posix_time::seconds(BULLY_TIME_OUT));
    t_bully_other.async_wait(strand.wrap(boost::bind(&Peer::finish_bully,this, boost::asio::placeholders::error)));
}


void Peer::get_bullyed(Peer::Message m){
    if (!online) return ;
    
    boost::this_thread::sleep(boost::posix_time::seconds(1));
  
    if(m.weight < availability || (m.weight == availability && pid < m.p)){
        
        state = BULLY_OTHER;
        t_being_bully.cancel(); //might not cancel that
        enqueue(boost::protect(boost::bind(&Peer::start_bully,this,error)));
        
    }else{
        state = BEING_BULLIED;
        
        t_bully_other.cancel();
       
        t_being_bully.expires_from_now(boost::posix_time::seconds(BULLY_TIME_OUT));
        t_being_bully.async_wait(strand.wrap(boost::bind(&Peer::start_bully,this,boost::asio::placeholders::error)));
    }
}

void Peer::cancel_bully(const boost::system::error_code &e){
    if (e == boost::asio::error::operation_aborted || !online) {
        return ;
    }
    AROLog::Log().Print(logINFO, 1, tag.c_str(),"Cancel bully\n");
    t_bully_other.cancel();
    t_being_bully.cancel();
}

void Peer::finish_bully(const boost::system::error_code &e){
    if (e == boost::asio::error::operation_aborted || !online) return ;
    
    AROLog::Log().Print(logINFO, 1, tag.c_str(),"finish bully\n");
    action_list->good_peer_first();
    //broad cast victory
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        peer_list[i]->enqueue(boost::protect(boost::bind(&Peer::stop_bully, peer_list[i], error)));
    }
}

void Peer::stop_bully(const boost::system::error_code &e){
    if (e == boost::asio::error::operation_aborted || !online) return ;
    
    AROLog::Log().Print(logINFO, 1, tag.c_str(),"Stop bully\n");
    t_bully_other.cancel();
    t_being_bully.cancel();

}
