//
//  peer.cpp
//  Decision
//
//  Created by Yue Huang on 2014-02-27.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "peer.h"
#include <boost/date_time.hpp>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <algorithm>
#include "AROLogInterface.h"
#include <sstream>
#include "commonDefines.h"
#include "packet.h"

Peer::Peer(unsigned int pid, std::vector<Peer *> &peer_list):pid(pid), peer_list(peer_list),
    work(io_service), strand(io_service),
    t_new_feed(io_service, boost::posix_time::seconds(0)),
    t_on_off_line(io_service, boost::posix_time::seconds(0)),
    t_bully_other(io_service, boost::posix_time::seconds(0)),
    t_being_bully(io_service, boost::posix_time::seconds(0)),
    t_queue_delay(io_service, boost::posix_time::seconds(0))
{
        //Init timers for that
    std::stringstream ss; ss<<"Peer# "<<pid;
    tag = ss.str();
    
    application = new Application(io_service,strand,pid, peer_list);
    for (unsigned int i = 0; i < 3; i++) {
        thread_group.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
    }
}

Peer::~Peer(){
    cancel_all();
    io_service.stop();
    thread_group.interrupt_all();
    delete application;
    AROLog(logINFO, 1, tag.c_str(), "-------------------\n");
}

void Peer::load_cache(uint64_t * ts, unsigned int n){ //need at least a uint64* t & n
    boost::mutex::scoped_lock lock(mutex);
    load_action_from_cache(application->get_actionList(), ts, n);
}

void Peer::test(){
    strand.dispatch(boost::bind(&Peer::get_online, this));
    if(pid == 0){
        strand.dispatch(boost::bind(&Application::pack_full_bb, application));
    }
}

void Peer::get_offline(){
    cancel_all();
    
    AROLog(logINFO, 1, tag.c_str(), "Get offline\n");
    
    t_on_off_line.expires_from_now(boost::posix_time::seconds((int) 10 *(1-availability) + rand() % 10));
    t_on_off_line.async_wait(strand.wrap(boost::bind(&Peer::get_online,this)));
}

void Peer::get_online(){
    online = true;
    state = NOTHING;
    application->resume();
    process_packet_with_delay(error);
    AROLog(logINFO, 1, tag.c_str(), "Get online\n");
    
    if(peer_type != PRIORITY_PEER){
//        t_new_feed.expires_from_now(boost::posix_time::seconds(rand() % 6 + 3));
//        t_new_feed.async_wait(strand.wrap(boost::bind(&Peer::new_feed,this,boost::asio::placeholders::error)));
        //        t_on_off_line.expires_from_now(boost::posix_time::seconds((int) 50 * availability + rand() % 5));
        //        t_on_off_line.async_wait(boost::bind(&Peer::get_offline,this));
    }
}
void Peer::cancel_all(){
    online = false;
    application->pause();
    stop_bully();
    t_new_feed.cancel();
    state = NOTHING;
}

void Peer::set_peer_type(PEER_TYPE type){
    peer_type = type;
    switch (peer_type) {
        case NEW_PEER:
        case BAD_PEER:
        case NORMAL_PEER:
        case GOOD_PEER:
            availability =  (float)(rand() % 10 + 1) / 10;
            if(availability < 0.8) availability = 0.8;
            break;
        case PRIORITY_PEER:
            availability = 1;
            break;
        default:
            break;
    }
    AROLog(logINFO, 1, tag.c_str(), "Init with availability %f\n",availability);
    
}

void Peer::broadcastToPeers(Packet *packet){
    
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        Packet * p = new Packet(*packet);
        if (i!=pid && peer_list[i]) peer_list[i]->enqueue(boost::protect(boost::bind(&Peer::receivePacket,peer_list[i],p)));
    }
    delete packet;
}

void Peer::receivePacket(Packet * p){
    packet_queue.push(p);
}

void Peer::process_packet_with_delay(boost::system::error_code error){
    if (!online || error == boost::asio::error::operation_aborted) return ;
    
    if (!packet_queue.empty()) {
        Packet * p = packet_queue.front();
        packet_queue.pop();
        if (p->flag == Packet::BULLY_WEIGHT) {
            strand.post(boost::bind(&Peer::get_bullyed,this, p->content->weight));
            delete p;
        }else{
            strand.post(boost::bind(&Application::processPacket,application,p));
        }
        
    }
    
    t_queue_delay.expires_from_now(boost::posix_time::millisec(rand()% 100 + 100));
    t_queue_delay.async_wait(strand.wrap(boost::bind(&Peer::process_packet_with_delay,this, boost::asio::placeholders::error)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - Base peer function
//timeed function series


void Peer::new_feed(const boost::system::error_code &e){
    if (!online || e == boost::asio::error::operation_aborted) return ;
   
    application->add_new_action();
    
    t_new_feed.expires_from_now(boost::posix_time::seconds(rand() % 6 + 5));
    t_new_feed.async_wait(strand.wrap(boost::bind(&Peer::new_feed,this, boost::asio::placeholders::error)));
    
}

void Peer::read_feed(){
    if (!online) return ;
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - Bully algorithm
void Peer::start_bully(const boost::system::error_code &e){
    if (e == boost::asio::error::operation_aborted || !online) return ;
    
    stop_bully();
    state = BULLY_OTHER;
    AROLog(logINFO, 1,tag.c_str(), "Start bully\n");
//    AROLog(logINFO, 1,tag.c_str(), "Start bully\n");
    Packet * p = new Packet(Packet::BULLY_WEIGHT); p->content->weight = availability;
    enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers, this, p)));
   
    
    t_bully_other.expires_from_now(boost::posix_time::seconds(BULLY_TIME_OUT));
    t_bully_other.async_wait(strand.wrap(boost::bind(&Peer::finish_bully,this, boost::asio::placeholders::error)));
}


void Peer::get_bullyed(float p_weight){
    if (!online) return ;
    
    boost::this_thread::sleep(boost::posix_time::seconds(1));
  
    //Special case: application is not synced, then we have to ask other to stop all bullying process
    if (!application->is_header_fully_synced()) {
        strand.dispatch(boost::bind(&Peer::stop_bully,this));
        for (unsigned int i = 0; i < peer_list.size(); i++) {
            if (pid != i)  peer_list[i]->enqueue(boost::protect(boost::bind(&Peer::stop_bully, peer_list[i])));
        }
    }else if(p_weight < availability || (p_weight == availability && pid < p_weight)){
        state = BULLY_OTHER;
        t_being_bully.cancel(); //might not cancel that
        strand.dispatch(boost::bind(&Peer::start_bully,this,error));
        
    }else{
        state = BEING_BULLIED;
        
        t_bully_other.cancel();
        t_being_bully.expires_from_now(boost::posix_time::seconds(BULLY_TIME_OUT));
        t_being_bully.async_wait(strand.wrap(boost::bind(&Peer::start_bully,this,boost::asio::placeholders::error)));
    }
}



void Peer::finish_bully(const boost::system::error_code &e){
    if (e == boost::asio::error::operation_aborted || !online) return ;
    
//    AROLog(logINFO, 1, tag.c_str(),"finish bully\n");
    strand.dispatch(boost::bind(&Application::pack_full_bb, application));
    strand.dispatch(boost::bind(&Peer::stop_bully,this));
    
    //ask everyone to stop
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (pid != i) peer_list[i]->enqueue(boost::protect(boost::bind(&Peer::stop_bully, peer_list[i])));
    }
}

void Peer::stop_bully(){    
//    AROLog(logINFO, 1, tag.c_str(),"Stop bully\n");
    t_bully_other.cancel();
    t_being_bully.cancel();

}
