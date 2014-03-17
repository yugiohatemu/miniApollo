//
//  peer.h
//  Decision
//
//  Created by Yue Huang on 2014-02-27.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__peer__
#define __Decision__peer__

#include "priorityPeer.h"
#include "application.h"
#include <string.h>

class Peer{
    struct Message{
        int p;
        float weight;
        Message(int p,float weight):p(p),weight(weight){};
    };
    enum State{
        NOTHING,
        BULLY_OTHER,
        BEING_BULLIED,
    };
public:
    unsigned int pid;
    float availability;
    Peer(unsigned int pid,std::vector<Peer *> &peer_list, PriorityPeer * priority_peer);
    ~Peer();
    
    void get_bullyed(Message m);

    Application * application;
//    BB_Synchronizer * bb_synchronizer;
    boost::asio::io_service io_service;
    boost::system::error_code error;
    
    template <typename TFunc>
    void enqueue(TFunc func){
        io_service.post(strand.wrap(boost::bind( &Peer::execute<TFunc>, this, func )));
    }
    void start_bully(const boost::system::error_code &e);
    void cancel_bully(const boost::system::error_code &e);
private:
    template <typename TFunc>
    void execute(TFunc func){
        if(online) func();
    }
    
    std::string tag;
    
    std::vector<Peer *> &peer_list;
    PriorityPeer * priority_peer;
    State state;
    
    boost::asio::io_service::work work;
	boost::asio::io_service::strand strand;
    boost::thread_group thread_group;
    
    boost::asio::deadline_timer t_new_feed;
    boost::asio::deadline_timer t_on_off_line;
    
    boost::asio::deadline_timer t_bully_other;
    boost::asio::deadline_timer t_being_bully;
    
    
    boost::mutex sync_lock;
    
    void cancel_all();
    
    void new_feed(const boost::system::error_code &e);
    void read_feed();
    
    void get_online();
    void get_offline();
    bool online;
    
    
    void finish_bully(const boost::system::error_code &e);
    void stop_bully(const boost::system::error_code &e);
};

#endif /* defined(__Decision__peer__) */
