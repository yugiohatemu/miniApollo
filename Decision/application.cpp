//
//  synchronizer.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-04.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "application.h"
#include "peer.h"
#include <boost/date_time.hpp>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <sstream>

#include "AROLog.h"
#include "AROUtil_C.h"

const unsigned int BB_SIZE = 10;
const int t_try_bb_TIMEOUT = 7;
const int T_APP_SYNC_TIMEOUT = 1;
const int T_BB_SYNC_TIMEOUT = 5;
const int T_HEADER_SYNC_TIMEOUT = 3;

Application::Application(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list,PriorityPeer * priority_peer):
    io_service(io_service),
    strand(strand),
    peer_list(peer_list),
    pid(pid),
    priority_peer(priority_peer),
    t_app_sync(io_service, boost::posix_time::seconds(0)),
    t_bb_sync(io_service, boost::posix_time::seconds(0)),
    t_try_bb(io_service, boost::posix_time::seconds(0)),
    t_header_sync(io_service, boost::posix_time::seconds(0))
{
    
    ac_list = init_default_actionList();
    //Setting up tags and syncrhonizer
    std::stringstream ss; ss<<"AC# "<<pid; tag = ss.str();
    
    ss.clear(); ss<<"H-SYNC# "<<pid; std::string sync_h_tag = ss.str();
    header_syncrhonizer = new AROObjectSynchronizer(NULL,NULL,0,this);
    header_syncrhonizer->setDebugInfo(0, sync_h_tag.c_str());
    
    ss.clear(); ss<<"SYNC# "<<pid; std::string sync_tag = ss.str();
    synchronizer = new AROObjectSynchronizer(NULL,NULL,  0 ,this); //
    synchronizer->setDebugInfo(0, sync_tag.c_str());
    
    ss.clear(); ss<<"BB-SYNC# "<<pid; std::string sync_bb_tag = ss.str();
    bb_synchronizer = new AROObjectSynchronizer(NULL,NULL, 0, this);
    bb_synchronizer->setDebugInfo(0, sync_bb_tag.c_str());
}

Application::~Application(){
    if (header_syncrhonizer) delete header_syncrhonizer;
    if (synchronizer)  delete synchronizer;
    if (bb_synchronizer) delete bb_synchronizer;
    
    free_actionList(ac_list);
}

void Application::pause(){
    app_pause();
    bb_pause();
    
    t_header_sync.cancel();
    GLOBAL_SYNCED = false;
    BB_ing = false;
}

void Application::resume(){
    GLOBAL_SYNCED = false;
    BB_ing = false;
    
    header_syncrhonizer->setNetworkPeriod(0.5);
    strand.dispatch(boost::bind(&Application::header_periodicSync, this,error));
}

void Application::broadcastToPeers(Packet packet){
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i!=pid) peer_list[i]->enqueue(boost::protect(boost::bind(&Application::processMessage,peer_list[i]->application,packet)));
    }
}

void Application::processMessage(Packet p){
    
    switch(p.flag){
        case HEADER_PROCESS_SP:
            strand.dispatch(boost::bind(&Application::header_processSyncPoint,this,p.content.sync_point));
            break;
        case HEADER_MERGE_HEADER:
            strand.dispatch(boost::bind(&Application::header_mergeHeader,this, &p.content.raw_header));
            break;
        case APP_PROCESS_SP:
            if (GLOBAL_SYNCED)
            strand.dispatch(boost::bind(&Application::app_processSyncPoint,this,p.content.sync_point));
            break;
        case APP_MERGE_ACTION:
            if (GLOBAL_SYNCED)
            strand.dispatch(boost::bind(&Application::app_mergeAction,this,p.content.ts));
            break;
        case BB_PROCESS_SP:
            strand.dispatch(boost::bind(&Application::bb_processSyncPoint,this,p.content.sync_point));
            break;
        case BB_MERGE_ACTION: //BB->SYNC
            strand.dispatch(boost::bind(&Application::bb_mergeAction,this, p.content.ts));
            break;
        default:
            break;
    }
}

void Application::add_new_action(){
    if(ac_list->action_count <=15){
        uint64_t ts = AOc_localTimestamp();
        AROLog::Log().Print(logINFO, 1, tag.c_str(),"New action %lld created\n", ts);
        strand.dispatch(boost::bind(&Application::app_mergeAction, this, ts));
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - AROSyncResponder interface
void Application::sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender){
    if (sender == header_syncrhonizer) {
        Packet packet; packet.flag = HEADER_PROCESS_SP; packet.content.sync_point = *syncPoint;
        broadcastToPeers(packet);
    }else if (sender == synchronizer) {
        Packet packet; packet.flag = APP_PROCESS_SP ; packet.content.sync_point = *syncPoint;
        broadcastToPeers(packet);
    }else if(sender == bb_synchronizer){
        Packet packet; packet.flag = BB_PROCESS_SP; packet.content.sync_point = *syncPoint;
        broadcastToPeers(packet);
    }
}

void Application::notificationOfSyncAchieved(double networkPeriod, int code, void *sender){
    AROLog::Log().Print(logINFO,1,tag.c_str(),"Sync achieved at %llf\n", networkPeriod);
    
    if (networkPeriod < 60) networkPeriod *= 1.5;
    if (sender == header_syncrhonizer) {
        AROLog::Log().Print(logINFO, 1, tag.c_str(), "Header section is synced\n");
        //Set a longer period
        header_syncrhonizer->setNetworkPeriod(networkPeriod); //set a longer network period
        GLOBAL_SYNCED = true;
        
        if (is_header_section_synced(ac_list)) {
            app_resume();
        }else{
            bb_resume();
        }
        
        t_header_sync.expires_from_now(boost::posix_time::seconds(T_HEADER_SYNC_TIMEOUT));
        t_header_sync.async_wait(strand.wrap(boost::bind(&Application::header_periodicSync,this, boost::asio::placeholders::error)));
//
    }else if (sender == synchronizer) {
        if (GLOBAL_SYNCED) {
            synchronizer->setNetworkPeriod(networkPeriod);
            t_app_sync.expires_from_now(boost::posix_time::seconds(T_APP_SYNC_TIMEOUT));
            t_app_sync.async_wait(strand.wrap(boost::bind(&Application::app_periodicSync,this, boost::asio::placeholders::error)));
        }else{
            app_pause();
        }
    }else if(sender == bb_synchronizer) {
        
        if (is_header_section_synced(ac_list)) {
            app_resume();
        }
        t_bb_sync.expires_from_now(boost::posix_time::seconds(T_BB_SYNC_TIMEOUT));
        t_bb_sync.async_wait(strand.wrap(boost::bind(&Application::bb_periodicSync,this, boost::asio::placeholders::error)));
        bb_synchronizer->setNetworkPeriod(networkPeriod);
        
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - Header Sync functionlity

void Application::header_periodicSync(const boost::system::error_code &error){
    //TODO: add a manul sync ahieved here because synchronizer might not working or overwrite synchornizer
    if ( error == boost::asio::error::operation_aborted) return ;
    
    AROLog::Log().Print(logDEBUG, 1, tag.c_str(), "H-Periodic sync\n");
    header_syncrhonizer->processSyncState(&ac_list->header_list[0].ts, &ac_list->header_list[0].hash, sizeof(Header_C) / sizeof(uint64_t), ac_list->header_count);
    
    t_header_sync.expires_from_now(boost::posix_time::seconds(T_HEADER_SYNC_TIMEOUT));
    t_header_sync.async_wait(strand.wrap(boost::bind(&Application::header_periodicSync,this, boost::asio::placeholders::error)));
}


void Application::header_processSyncPoint(SyncPoint msgSyncPoint){
    if ((msgSyncPoint.id1 == 0) && (msgSyncPoint.id2 == 0)) {
        
        if ((msgSyncPoint.ts1 == 0ll) && (msgSyncPoint.ts2 == 0ll)) { //0,0,0,0 global picture
            
            if (ac_list->header_count > 0) {
                SyncPoint global_pic;
                global_pic.id1 = 1; global_pic.id2 = ac_list->header_count;
                global_pic.ts1 = ac_list->header_list[0].ts;global_pic.ts2 = ac_list->header_list[ac_list->header_count-1].ts;
                global_pic.hash = 0; global_pic.res = ac_list->header_count;
                
                AROLog::Log().Print(logINFO,1,tag.c_str(),"Responding to global pic with %d-%d %lld-%lld\n",global_pic.id1,global_pic.id2,global_pic.ts1,global_pic.ts2);
                
                Packet packet; packet.flag = HEADER_PROCESS_SP; packet.content.sync_point = global_pic;
                broadcastToPeers(packet);
            }
            
        } else if (ac_list->header_count > 0) { //current region not empty
            
            int lo = 0, hi = ac_list->header_count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,ac_list->header_list[lo].ts, ac_list->header_list[hi].ts, ac_list->header_list[pivot].ts);
            
            for (; lo <= hi; lo++) {
    
                Packet packet; packet.flag = HEADER_MERGE_HEADER;
                packet.content.raw_header = *ac_list->header_list[lo].raw_header;
                
                broadcastToPeers(packet);
            }
        }
    }else {
        //id1, id2, ts1, ts2
        AROLog::Log().Print(logINFO,1,tag.c_str(),"Notify of sync point %d-%d %lld-%lld\n",msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        
        header_syncrhonizer->notifyOfSyncPoint(&msgSyncPoint);
    }

}

void Application::header_mergeHeader(Raw_Header_C *raw_header){
    boost::mutex::scoped_lock lock(mutex);
    merge_new_header(ac_list, raw_header);
    
    BB_ing = false;
    GLOBAL_SYNCED = false;
    sync_header_with_self(ac_list);
    
    bb_reset();
    bb_resume();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - App Sync functionality

//Only be called by the sync Header maybe
void Application::app_resume(){
    
    synchronizer->setSyncIDArrays(&ac_list->action_list[0].ts, &ac_list->action_list[0].hash, sizeof(Action_C) / sizeof(uint64_t));
    synchronizer->setNetworkPeriod(0.5);
    
    t_try_bb.expires_from_now(boost::posix_time::seconds(t_try_bb_TIMEOUT));
    t_try_bb.async_wait(strand.wrap(boost::bind(&Application::try_bb,this, boost::asio::placeholders::error)));
    
    strand.dispatch(boost::bind(&Application::app_periodicSync,this,error));
    
}
void Application::app_reset(){
    synchronizer->reset();
}

void Application::app_pause(){
    t_app_sync.cancel();
    t_try_bb.cancel();
    
    synchronizer->setNetworkPeriod(0ll);
}

void Application::app_mergeAction(uint64_t ts){ //the content of action
    //Find the insertion
    boost::mutex::scoped_lock lock(mutex);
    merge_new_action(ac_list, ts);
}

void Application::app_periodicSync(const boost::system::error_code &error){
    if ( error == boost::asio::error::operation_aborted) return ;
    
    AROLog::Log().Print(logDEBUG, 1, tag.c_str(), "Periodic sync\n");
    synchronizer->processSyncState(&ac_list->action_list[0].ts, &ac_list->action_list[0].hash, sizeof(Action_C) / sizeof(uint64_t), ac_list->action_count);
    
    t_app_sync.expires_from_now(boost::posix_time::seconds(T_APP_SYNC_TIMEOUT));
    t_app_sync.async_wait(strand.wrap(boost::bind(&Application::app_periodicSync,this, boost::asio::placeholders::error)));
}

void Application::app_processSyncPoint(SyncPoint msgSyncPoint){
    //Ask for between certain range
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    if ((msgSyncPoint.id1 == 0) && (msgSyncPoint.id2 == 0)) {
        
        if ((msgSyncPoint.ts1 == 0ll) && (msgSyncPoint.ts2 == 0ll)) { //0,0,0,0 global picture
            
            if (ac_list->action_count > 0) {
                SyncPoint global_pic;
                global_pic.id1 = 1; global_pic.id2 = ac_list->action_count;
                global_pic.ts1 = ac_list->action_list[0].ts;global_pic.ts2 = ac_list->action_list[ac_list->action_count-1].ts;
                global_pic.hash = 0; global_pic.res = ac_list->action_count;
                
                AROLog::Log().Print(logINFO,1,tag.c_str(),"Responding to global pic with %d-%d %lld-%lld\n",global_pic.id1,global_pic.id2,global_pic.ts1,global_pic.ts2);
                
                Packet packet; packet.flag = APP_PROCESS_SP; packet.content.sync_point = global_pic;
                broadcastToPeers(packet);
            }
            
        } else if (ac_list->action_count > 0) { //current region not empty
            
            int lo = 0, hi = ac_list->action_count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,ac_list->action_list[lo].ts, ac_list->action_list[hi].ts, ac_list->action_list[pivot].ts);
            for (; lo <= hi; lo++) {
                
                Packet packet; packet.flag = APP_MERGE_ACTION; packet.content.ts = ac_list->action_list[lo].ts;
                broadcastToPeers(packet);
            }
        }
    }else {
        //id1, id2, ts1, ts2
        AROLog::Log().Print(logINFO,1,tag.c_str(),"Notify of sync point %d-%d %lld-%lld\n",msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        synchronizer->notifyOfSyncPoint(&msgSyncPoint);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - between Header and action

void Application::try_bb(boost::system::error_code e){ //try bb
    if(e == boost::asio::error::operation_aborted ) return;
   
    if (GLOBAL_SYNCED && ac_list->action_count > BB_SIZE && GLOBAL_SYNCED && !BB_ing) {
        BB_ing = true;
        strand.dispatch(boost::bind(&Peer::start_bully, peer_list[pid], error));
    }else{
        t_try_bb.expires_from_now(boost::posix_time::seconds(t_try_bb_TIMEOUT));
        t_try_bb.async_wait(strand.wrap(boost::bind(&Application::try_bb,this, boost::asio::placeholders::error)));
    }
}

void Application::pack_full_bb(){ //pack complete bb
    
    app_pause();
    
    BackBundle_C * bb = init_BB_with_actions(ac_list->action_list, BB_SIZE);
    Raw_Header_C * raw_header = init_raw_header_with_BB(bb);
    
    {
        boost::mutex::scoped_lock lock(mutex);
        merge_new_header_with_BB(ac_list, raw_header, bb); //also cleans itself btw
        GLOBAL_SYNCED = true;
        
        Packet packet; packet.flag = HEADER_MERGE_HEADER; packet.content.raw_header = *raw_header;
        broadcastToPeers(packet);
        
        BB_ing = false;
    }
    
    app_reset();
    bb_reset();
    bb_resume();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - BB functionality

void Application::bb_resume(){
    BackBundle_C * bb =  get_latest_BB(ac_list);
    if (!bb) return;
    
    bb_synchronizer->setSyncIDArrays(&bb->action_list[0].ts , &bb->action_list[0].hash, sizeof(Action_C) / sizeof(uint64_t));
    bb_synchronizer->setNetworkPeriod(0.5);
    
    t_bb_sync.expires_from_now(boost::posix_time::seconds(T_BB_SYNC_TIMEOUT));
    t_bb_sync.async_wait(strand.wrap(boost::bind(&Application::bb_periodicSync,this, boost::asio::placeholders::error)));
    
}

void Application::bb_pause(){
    t_bb_sync.cancel();
}

void Application::bb_reset(){
    bb_synchronizer->reset();
}

#warning - See comment
//technically it should only be the latest one, we will have at most one unsynced bundle...
//But in future, it will be used by other application to help it to sync up, but should be treated differently
//Because this one is only used for transition between active->header, we have have at most unsynced bb_synchronizer
void Application::bb_periodicSync(const boost::system::error_code &e){
    if(bb_synchronizer->isEmpty()) return;
    
    AROLog::Log().Print(logDEBUG, 1, tag.c_str(), "BB-Periodic sync\n");
    
    BackBundle_C * bb = get_latest_BB(ac_list);
    if (!bb) return;
    
    bb_synchronizer->processSyncState(&bb->action_list[0].ts, &bb->action_list[0].hash, sizeof(Action_C) / sizeof(uint64_t), bb->action_count);
    
    t_bb_sync.expires_from_now(boost::posix_time::seconds(T_BB_SYNC_TIMEOUT));
    t_bb_sync.async_wait(strand.wrap(boost::bind(&Application::bb_periodicSync,this, boost::asio::placeholders::error)));
}

void Application::bb_mergeAction(uint64_t ts){
    if (ac_list->header_count == 0) return ;
    
    Header_C h = ac_list->header_list[ac_list->header_count-1];
    if(h.synced) return;

    BackBundle_C * bb = get_latest_BB(ac_list);
    merge_action_into_BB(bb, ts);

    update_sync_state(&h);
    
    if (h.synced) {
        boost::mutex::scoped_lock lock(mutex);
        remove_duplicate_actions(ac_list, bb);
    }
}


void Application::bb_processSyncPoint(SyncPoint msgSyncPoint){
    BackBundle_C * bb  = get_latest_BB(ac_list);
    if (!bb) return ;
    
    if ((msgSyncPoint.id1 == 0) && (msgSyncPoint.id2 == 0)) {
        
        if ((msgSyncPoint.ts1 == 0ll) && (msgSyncPoint.ts2 == 0ll)) { //0,0,0,0 global picture
            //BB_GLOBAL
            if (bb->action_count > 0) {
                SyncPoint global_pic;
                global_pic.id1 = 1; global_pic.id2 = bb->action_count;
                global_pic.ts1 = bb->action_list[0].ts;global_pic.ts2 = bb->action_list[bb->action_count-1].ts;
                global_pic.hash = 0; global_pic.res = bb->action_count;
                
                AROLog::Log().Print(logINFO,1,tag.c_str(),"Responding to global pic with %d-%d %lld-%lld\n",global_pic.id1,global_pic.id2,global_pic.ts1,global_pic.ts2);
                Packet packet; packet.flag = BB_PROCESS_SP; packet.content.sync_point = global_pic;
                broadcastToPeers(packet);
            }
            
        } else if (bb->action_count > 0) { //current region not empty
            //BB_MERGE_ACTION
            int lo = 0, hi = bb->action_count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,bb->action_list[lo].ts, bb->action_list[hi].ts, bb->action_list[pivot].ts);
            for (; lo <= hi; lo++) {
            
                Packet packet; packet.flag = BB_MERGE_ACTION; packet.content.ts = bb->action_list[lo].ts;
                broadcastToPeers(packet);
            }
        }
    }else {
        //id1, id2, ts1, ts2
        AROLog::Log().Print(logINFO,1,tag.c_str(),"Notify of sync point in BB %d-%d %lld-%lld\n",msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        bb_synchronizer->notifyOfSyncPoint(&msgSyncPoint);
    }
    
}

