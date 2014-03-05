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
//need to keep a link to the backbundle synchronizer
class Peer;
class BB_Synchronizer;

class Synchronizer{

    unsigned int pid;
    
    boost::system::error_code error;
    boost::asio::io_service &io_service;
    boost::asio::io_service::strand &strand;
    
    boost::asio::deadline_timer t_broadcast;
    boost::asio::deadline_timer t_clean_up;
    
    std::vector<uint64_t> ts_list;
    std::vector<Peer *> &peer_list;
    
    boost::mutex mutex;
    void sync(std::vector<uint64_t> &other_list);
    void broadcast(boost::system::error_code );
    
    bool BB_started;
public:
    Synchronizer(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list);
    ~Synchronizer();
   
    BB_Synchronizer * bb_synchronizer;
    
    void stop();
    void start();
    unsigned int size();
    void add_ts(uint64_t ts);
    void set_ts_list(std::vector<uint64_t> &list);
    std::vector<uint64_t> & get_ts_list();
    
    void clean_up(boost::system::error_code error);
    void first_clean_up();
    void clean_up_done();
    
};

#endif /* defined(__Decision__synchronizer__) */
