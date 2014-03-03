//
//  peer.h
//  Decision
//
//  Created by Yue Huang on 2014-02-27.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__peer__
#define __Decision__peer__

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <inttypes.h>
#include <vector>

#include "backBundle.h"
#include "dataPool.h"
#include "bbSynchronizer.h"

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
        DELAY
    };
public:
    unsigned int pid;
    float availability;
    unsigned int bb_count;
    Peer(unsigned int pid,std::vector<Peer *> &peer_list, DataPool * data_pool);
    ~Peer();
    
//    void init(std::vector<Peer *> &peer_list);
    void sync(std::vector<uint64_t> &synchronizer);
    
    void get_bullyed(Message m);
    void pack_bb(const boost::system::error_code &e);
    
    boost::asio::io_service io_service;
    boost::system::error_code error;
private:
    std::vector<uint64_t> synchronizer;
    BB_Synchronizer * bb_synchronizer;
    //BB bundles
    //disk
    //harddisk
    std::vector<Peer *> &peer_list;
    DataPool * data_pool;
    State state;
    
    unsigned int BB_SIZE = 15;
    
    
    boost::asio::io_service::work work;
	boost::asio::io_service::strand strand;
    boost::thread_group thread_group;
    
    boost::asio::deadline_timer t_broadcast;
    boost::asio::deadline_timer t_new_feed;
    boost::asio::deadline_timer t_on_off_line;
    boost::asio::deadline_timer t_miscellaneous;
    
    boost::asio::deadline_timer t_bully_other;
    boost::asio::deadline_timer t_being_bully;
    
    boost::asio::deadline_timer t_try_bb;
    
    boost::mutex sync_lock;
    
    void cancel_all();
    
    void broadcast();
    void new_feed();
    void read_feed();
    
    void get_online();
    void get_offline();
    bool online;
    
    void start_bully(const boost::system::error_code &e);
    void finish_bully(const boost::system::error_code &e);
    
    void get_header(BackBundle::Header header); //copy Header from BB, or just the actual BB now
    void broadcast_header();
    
    void try_bb(const boost::system::error_code &e);
};

#endif /* defined(__Decision__peer__) */
