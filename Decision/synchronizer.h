//
//  synchronizer.h
//  Decision
//
//  Created by Yue Huang on 2014-03-04.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__synchronizer__
#define __Decision__synchronizer__

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <inttypes.h>
#include <vector>

class Peer;

class Synchronizer{
    unsigned int pid;
    
    boost::system::error_code error;
    boost::asio::io_service &io_service;
    boost::asio::io_service::strand &strand;
    
    boost::asio::deadline_timer t_broadcast;
    
    std::vector<uint64_t> ts_list;
    std::vector<Peer *> &peer_list;
    
    boost::mutex mutex;
    void sync(std::vector<uint64_t> &other_list);
    void broadcast(boost::system::error_code );
public:
    Synchronizer(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list);
    ~Synchronizer();
   
    void stop();
    void start();
    unsigned int size();
    void add_ts(uint64_t ts);
   
};

#endif /* defined(__Decision__synchronizer__) */
