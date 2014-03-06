//
//  bbSynchronizer.h
//  Decision
//
//  Created by Yue Huang on 2014-03-03.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__bbSynchronizer__
#define __Decision__bbSynchronizer__

#include "backBundle.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <inttypes.h>
#include <vector>

class Peer;
class Synchronizer;

class BB_Synchronizer{
    struct Message{
        BackBundle::Header h;
        unsigned int pid;
        Message(BackBundle::Header h, unsigned int pid):h(h),pid(pid){}
    };
    
    unsigned int pid;
    boost::system::error_code error;
    boost::asio::io_service &io_service;
    boost::asio::io_service::strand &strand;
    std::vector<Peer *> &peer_list;
    boost::asio::deadline_timer t_broadcast;
    boost::mutex mutex;
    
    std::vector<BackBundle *> bb_list;

    void broadcast(boost::system::error_code error);
    void sync(Message m);
//    void sync_bb(unsigned int p);
public:
    BB_Synchronizer(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list);
    ~BB_Synchronizer();
    
    Synchronizer * synchronizer;
    void add_BB(BackBundle * BB);
    void add_empty_BB_with_header(BackBundle::Header &h);
    bool is_BB_empty();
    bool is_BB_synced();
    void stop();
    void start();
    void sync_bb(boost::system::error_code error);
    bool is_ts_in_BB(uint64_t ts);
    std::vector<uint64_t> get_ts_list(unsigned int i);
    BackBundle* get_BB_with_header(BackBundle::Header h);
};

#endif /* defined(__Decision__bbSynchronizer__) */
