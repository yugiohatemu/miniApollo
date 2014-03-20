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
#include "action_C.h"
#include "AROObjectSynchronizer.h"
#include "AROProtocols.h"
#include <string.h>
class Peer;
#define SYN_CHEAT 1;

class Application:public AROSyncResponder{

    enum FLAG{
        HEADER_PROCESS_SP, HEADER_MERGE_HEADER,
#ifdef SYN_CHEAT
        HEADER_COUNT,
#endif
        APP_PROCESS_SP,APP_MERGE_ACTION,
        BB_PROCESS_SP,BB_MERGE_ACTION,
    };
    
    union BLOCK_PAYLOAD{
#ifdef SYN_CHEAT
        unsigned int header_count;
#endif
        uint64_t ts;
        SyncPoint sync_point;
        Raw_Header_C *raw_header;
    };
    
    struct Packet{
        BLOCK_PAYLOAD content;
        enum FLAG flag;
    };
    
    unsigned int pid;
    
    boost::system::error_code error;
    boost::asio::io_service &io_service;
    boost::asio::io_service::strand &strand;
    
    boost::asio::deadline_timer t_header_sync;
    boost::asio::deadline_timer t_app_sync;
    boost::asio::deadline_timer t_bb_sync;
    
    boost::asio::deadline_timer t_try_bb;
    
    ActionList_C * ac_list;
    PriorityPeer * priority_peer;
    std::vector<Peer *> &peer_list; //connection_pool in fact
    boost::mutex mutex;
    
//    State state;
    std::string tag;
    ////////////////////////////////////////////////////////////////////////////////
    void processMessage(Packet p);
    void broadcastToPeers(Packet p);
    
    void header_periodicSync(const boost::system::error_code &error);
    void header_processSyncPoint(SyncPoint msgSyncPoint);
    void header_mergeHeader(Raw_Header_C *raw_header);
#ifdef SYN_CHEAT
    void header_processCount(unsigned int h_count);
#endif
    
    void app_resume();
    void app_periodicSync(const boost::system::error_code &error);
    void app_processSyncPoint(SyncPoint msgSyncPoint);
    void app_mergeAction(uint64_t ts);
    
    void bb_resume();
    void bb_periodicSync(const boost::system::error_code &error);
    void bb_processSyncPoint(SyncPoint msgSyncPoint);
    void bb_mergeAction(uint64_t ts);
    
    void sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender);
    void notificationOfSyncAchieved(double networkPeriod, int code, void *sender);
    
public:
    Application(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list, PriorityPeer * priority_peer);
    ~Application();
   
    AROObjectSynchronizer * header_syncrhonizer;
    AROObjectSynchronizer * synchronizer;
    AROObjectSynchronizer * bb_synchronizer;

    void pause();
    void resume();
    void add_new_action();
    
    void try_bb(boost::system::error_code error);
    void pack_full_bb();
    
    bool is_header_fully_synced();
};
#endif /* defined(__Decision__synchronizer__) */
