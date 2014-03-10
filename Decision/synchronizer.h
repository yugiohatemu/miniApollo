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
#include "syncHeader.h"
#include "backBundle.h"
#include "priorityPeer.h"
//need to keep a link to the backbundle synchronizer

class Peer;

class Synchronizer{

    unsigned int pid;
    
    boost::system::error_code error;
    boost::asio::io_service &io_service;
    boost::asio::io_service::strand &strand;
    
    boost::asio::deadline_timer t_broadcast;
    boost::asio::deadline_timer t_clean_up;
    
    std::vector<SyncEntry *> se_list;
    std::vector<Peer *> &peer_list;
    
    boost::mutex mutex;

    void broadcast(boost::system::error_code );
    
    bool BB_started;
    
    PriorityPeer * priority_peer;
public:
    Synchronizer(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list, PriorityPeer * priority_peer);
    ~Synchronizer();
   
    std::vector<SyncEntry *>& get_se_list();
    unsigned int get_sync_region();
    SyncHeader* get_sync_header_with(BackBundle::Header header);

    void stop();
    void start();
    void add_new_se();
    void add_new_header(BackBundle::Header header);
    
    void sync(unsigned int peer_id);
    void sync_header(BackBundle::Header header);
//    void sync_header(unsigned int peer_id);
//    void sync_self();
    unsigned int sync_region = 0;
    
    void search_good_peer(boost::system::error_code error);
    void good_peer_first();
    void remove_empty_se();
   
    bool has_ts(uint64_t ts);
    unsigned int find_insert_pos(uint64_t ts);
    
};

#endif /* defined(__Decision__synchronizer__) */
