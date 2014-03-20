//
//  peer.h
//  Decision
//
//  Created by Yue Huang on 2014-02-27.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__peer__
#define __Decision__peer__

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
    enum PEER_TYPE{
        NEW_PEER,
        BAD_PEER,
        NORMAL_PEER,
        GOOD_PEER,
        PRIORITY_PEER,
    };
    
    
    Peer(unsigned int pid,std::vector<Peer *> &peer_list);
    ~Peer();
    void set_peer_type(PEER_TYPE peer_type);
    
    Application * application;
    boost::asio::io_service io_service;
    boost::system::error_code error;
    
    template <typename TFunc>
    void enqueue(TFunc func){
        io_service.post(strand.wrap(boost::bind( &Peer::execute<TFunc>, this, func )));
    }
    void start_bully(const boost::system::error_code &e);
    void stop_bully();
    void get_online();
private:
    template <typename TFunc>
    void execute(TFunc func){
        if(online) func();
    }
    
    std::string tag;
    std::vector<Peer *> &peer_list;
    State state;
    unsigned int pid;
    float availability;
    PEER_TYPE peer_type;
    
    boost::asio::io_service::work work;
	boost::asio::io_service::strand strand;
    boost::thread_group thread_group;
    
    boost::asio::deadline_timer t_new_feed;
    boost::asio::deadline_timer t_on_off_line;
    
    boost::asio::deadline_timer t_bully_other;
    boost::asio::deadline_timer t_being_bully;
    
    boost::mutex mutex;
    
    void cancel_all();
    
    void new_feed(const boost::system::error_code &e);
    void read_feed();
    
    
    void get_offline();
    bool online;
    
    void get_bullyed(Message m);
    void finish_bully(const boost::system::error_code &e);
    
};

#endif /* defined(__Decision__peer__) */
