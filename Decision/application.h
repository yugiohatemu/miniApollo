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
#include <string.h>
class Peer;

class Application:public AROSyncResponder{

    unsigned int pid;
    
    boost::system::error_code error;
    boost::asio::io_service &io_service;
    boost::asio::io_service::strand &strand;
    
    boost::asio::deadline_timer t_sync;
    boost::asio::deadline_timer t_clean_up;
    
//    Action_C * ac_list;
    ActionList_C * ac_list;
    std::vector<Peer *> &peer_list; //connection_pool in fact
    
    boost::mutex mutex;
    
    bool BB_started;
    unsigned int sync_region = 0;
    PriorityPeer * priority_peer;
    
    std::string tag;
public:
    Application(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list, PriorityPeer * priority_peer);
    ~Application();
   
    AROObjectSynchronizer * synchronizer;
    AROObjectSynchronizer * bb_synchronizer;
//    unsigned int count;
    void pause();
    void resume();
    void add_new_action();
    void processAppDirective(SyncPoint p, bool flag);
    void periodicSync(const boost::system::error_code &error);
    void processSyncPoint(SyncPoint msgSyncPoint);
    void mergeAction(SyncPoint p); //or Application??

    void sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender);
    void notificationOfSyncAchieved(double networkPeriod, int code, void *sender);
  
    
    void search_good_peer(boost::system::error_code error);
    void good_peer_first();
    

};
#endif /* defined(__Decision__synchronizer__) */
