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

#include "backBundle.h"
#include "priorityPeer.h"
//need to keep a link to the backbundle synchronizer
#include "action_C.h"
#include "AROObjectSynchronizer.h"
#include "AROProtocols.h"
class Peer;

class ActionList:public AROSyncResponder{

    unsigned int pid;
    
    boost::system::error_code error;
    boost::asio::io_service &io_service;
    boost::asio::io_service::strand &strand;
    
    boost::asio::deadline_timer t_broadcast;
    boost::asio::deadline_timer t_clean_up;
    
    Action_C * ac_list;
    std::vector<Peer *> &peer_list; //connection_pool in fact
    
    boost::mutex mutex;

    void broadcast(boost::system::error_code );
    
    bool BB_started;
    unsigned int sync_region = 0;
    PriorityPeer * priority_peer;
public:
    ActionList(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list, PriorityPeer * priority_peer);
    ~ActionList();
   
    
    AROObjectSynchronizer * synchronizer;
    unsigned int count;
    void stop();
    void start();
    void add_new_action();

    void sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender);
    void notificationOfSyncAchieved(double networkPeriod, void *sender);
    void periodicSync_(const boost::system::error_code &error);
    void processSyncPoint_(SyncPoint msgSyncPoint);
    
    void search_good_peer(boost::system::error_code error);
    void good_peer_first();
    

};
#endif /* defined(__Decision__synchronizer__) */
