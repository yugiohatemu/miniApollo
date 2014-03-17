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

#include "priorityPeer.h"
//need to keep a link to the backbundle synchronizer
#include "action_C.h"
#include "AROObjectSynchronizer.h"
#include "AROProtocols.h"
#include <string.h>
class Peer;

class Application:public AROSyncResponder{

    enum FLAG{
        APP_PROCESS_SP,
        APP_MERGE_ACTION,
        BB_PROCESS_SP,
        BB_MERGE_HEADER,
        BB_MERGE_ACTION,
    };
    
    union BLOCK_PAYLOAD{
        uint64_t ts;
        SyncPoint sync_point;
        Raw_Header_C raw_header;
    };
    
    struct Packet{
        BLOCK_PAYLOAD content;
        enum FLAG flag;
    };
    
    unsigned int pid;
    
    boost::system::error_code error;
    boost::asio::io_service &io_service;
    boost::asio::io_service::strand &strand;
    
    boost::asio::deadline_timer t_app_sync;
    boost::asio::deadline_timer t_bb_sync;
    boost::asio::deadline_timer t_good_peer;
    boost::asio::deadline_timer t_clean_up;
    
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

    void pause();
    void resume();
    void add_new_action();
    
    void processMessage(Packet p);
    
    void app_periodicSync(const boost::system::error_code &error);
    void bb_periodicSync(const boost::system::error_code &error);

    void app_processSyncPoint(SyncPoint msgSyncPoint);
    void bb_processSyncPoint(SyncPoint msgSyncPoint);

    void app_mergeAction(uint64_t ts); //or Application??

    void sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender);
    void notificationOfSyncAchieved(double networkPeriod, int code, void *sender);
  
    void search_good_peer(boost::system::error_code error);
    void good_peer_first();
    
    void bb_mergeHeader(Raw_Header_C * raw_header);
    void bb_mergeAction(uint64_t ts);
    
    void clean_up();
};
#endif /* defined(__Decision__synchronizer__) */
