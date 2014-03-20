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

const unsigned int BB_SIZE = 5;
const int T_TRY_BB_TIMEOUT = 7;
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
#ifdef SYN_CHEAT
    AROLog::Log().Print(logINFO, 1, tag.c_str(),"CHEAT synchronizer for base case.\n");
#endif
    
}

Application::~Application(){
    AROLog_Print(logINFO, 1, tag.c_str(), "Header %d\n", ac_list->header_count);
    for (unsigned int i = 0; i < ac_list->header_count; i++) {
        Raw_Header_C * raw_header =ac_list->header_list[i].raw_header;
        AROLog_Print(logINFO,1,tag.c_str(), "Raw-Header %d %lld %lld -- %c\n",raw_header->size, raw_header->from, raw_header->to, ac_list->header_list[i].synced ? 'Y':'N');
        BackBundle_C * bb = ac_list->header_list[i].bb;
        if (bb) {
            for (unsigned int j = 0; j < bb->action_count; j++) {
                AROLog_Print(logINFO, 1, tag.c_str(), "BB %d - %lld\n",j,bb->action_list[j].ts);
            }
        }
    }
    AROLog_Print(logINFO, 1, tag.c_str(), "App Region\n");
    for (unsigned int i = 0; i < ac_list->action_count; i++) {
        AROLog_Print(logINFO, 1, tag.c_str(), "App %d - %lld\n", i, ac_list->action_list[i].ts);
    }
    
    if (header_syncrhonizer) delete header_syncrhonizer;
    if (synchronizer)  delete synchronizer;
    if (bb_synchronizer) delete bb_synchronizer;
    
    free_actionList(ac_list);
}

void Application::pause(){
    t_header_sync.cancel();
    app_pause();
    bb_pause();
    GLOBAL_SYNCED = false;
    BB_ing = false;
}

void Application::resume(){
    GLOBAL_SYNCED = false;
    BB_ing = false;
    
    header_syncrhonizer->setNetworkPeriod(0.5);
    header_syncrhonizer->setSyncIDArrays(&ac_list->header_list[0].ts, &ac_list->header_list[0].hash, sizeof(Header_C)/sizeof(uint64_t));
    strand.dispatch(boost::bind(&Application::header_periodicSync,this,error));
}

void Application::broadcastToPeers(Packet packet){
    boost::this_thread::sleep(boost::posix_time::seconds(1));
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
            strand.dispatch(boost::bind(&Application::header_mergeHeader,this, p.content.raw_header));
            break;
#ifdef SYN_CHEAT
        case HEADER_COUNT:
            strand.dispatch(boost::bind(&Application::header_processCount, this, p.content.header_count));
            break;
#endif
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
        case BB_MERGE_ACTION:
            strand.dispatch(boost::bind(&Application::bb_mergeAction,this, p.content.ts));
            break;
        default:
            break;
    }
}

void Application::add_new_action(){
//    if(ac_list->action_count <=10){
        uint64_t ts = AOc_localTimestamp();
        AROLog::Log().Print(logINFO, 1, tag.c_str(),"New action %lld created\n", ts);
        strand.dispatch(boost::bind(&Application::app_mergeAction, this, ts));
//    }
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
   
    if (networkPeriod < 60) networkPeriod *= 1.5;
    if (sender == header_syncrhonizer) {
//        AROLog::Log().Print(logINFO, 1, tag.c_str(), "Header Sync achieved at %llf\n", networkPeriod);
        //Set a longer period
        header_syncrhonizer->setNetworkPeriod(networkPeriod); //set a longer network period
        if(!GLOBAL_SYNCED && is_header_section_synced(ac_list)){
            app_resume();
            GLOBAL_SYNCED = true;
            t_header_sync.expires_from_now(boost::posix_time::seconds(T_HEADER_SYNC_TIMEOUT * 2));
        }else if(!is_header_section_synced(ac_list)){
            if (bb_synchronizer->isEmpty()) {//TODO: should be start it if we havn't start it yet
                bb_resume();
            }
            GLOBAL_SYNCED = false;
            t_header_sync.expires_from_now(boost::posix_time::seconds(T_HEADER_SYNC_TIMEOUT));
        }
        
        
        t_header_sync.async_wait(strand.wrap(boost::bind(&Application::header_periodicSync,this, boost::asio::placeholders::error)));
//
    }else if (sender == synchronizer) {
        
        if (GLOBAL_SYNCED) {
            AROLog::Log().Print(logINFO,1,tag.c_str(),"App Sync achieved at %llf\n", networkPeriod);
            synchronizer->setNetworkPeriod(networkPeriod);
            t_app_sync.expires_from_now(boost::posix_time::seconds(T_APP_SYNC_TIMEOUT));
            t_app_sync.async_wait(strand.wrap(boost::bind(&Application::app_periodicSync,this, boost::asio::placeholders::error)));
        }else{
            app_pause();
        }
    }else if(sender == bb_synchronizer) {
        AROLog::Log().Print(logINFO,1, tag.c_str(),"BB Sync achieved at %llf\n", networkPeriod);
       
        t_bb_sync.expires_from_now(boost::posix_time::seconds(T_BB_SYNC_TIMEOUT));
        t_bb_sync.async_wait(strand.wrap(boost::bind(&Application::bb_periodicSync,this, boost::asio::placeholders::error)));
        bb_synchronizer->setNetworkPeriod(networkPeriod);
        
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - Header Sync functionlity

void Application::header_periodicSync(const boost::system::error_code &error){
    if ( error == boost::asio::error::operation_aborted) return ;
    
    AROLog::Log().Print(logDEBUG, 1, tag.c_str(), "H-Periodic sync\n");
    
    header_syncrhonizer->processSyncState(&ac_list->header_list[0].ts, &ac_list->header_list[0].hash, sizeof(Header_C) / sizeof(uint64_t), ac_list->header_count);
    
    t_header_sync.expires_from_now(boost::posix_time::seconds(T_HEADER_SYNC_TIMEOUT));
    t_header_sync.async_wait(strand.wrap(boost::bind(&Application::header_periodicSync,this, boost::asio::placeholders::error)));
}

#ifdef SYN_CHEAT
void Application::header_processCount(unsigned int header_count){
    if(header_count == ac_list->header_count){
//        GLOBAL_SYNCED = true;
        notificationOfSyncAchieved(header_syncrhonizer->getNetworkPeriod(), 0, header_syncrhonizer);
    }else{
        AROLog::Log().Print(logINFO, 1, "", "--------------it Happens, so what?\n");
        GLOBAL_SYNCED = false;
    }
}
#endif

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
#ifdef SYN_CHEAT
            Packet p; p.flag = HEADER_COUNT;p.content.header_count = ac_list->header_count;
            broadcastToPeers(p);
            AROLog::Log().Print(logDEBUG,1,tag.c_str(),"Responding with Header Count %d\n", ac_list->header_count);
#endif
//            else{
//
//                SyncPoint global_pic;
//                global_pic.id1 = global_pic.id2 = global_pic.ts1 = global_pic.ts2 = global_pic.hash = global_pic.res = 0;
//                AROLog::Log().Print(logINFO,1,tag.c_str(),"Responding to empty global pic\n");
//                
//                Packet packet; packet.flag = HEADER_PROCESS_SP; packet.content.sync_point = global_pic;
//                broadcastToPeers(packet);
//            }
            
        } else if (ac_list->header_count > 0) { //current region not empty
            
            int lo = 0, hi = ac_list->header_count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,ac_list->header_list[lo].ts, ac_list->header_list[hi].ts, ac_list->header_list[pivot].ts);
            
            for (; lo <= hi; lo++) {
    
                Packet packet; packet.flag = HEADER_MERGE_HEADER;
                packet.content.raw_header = copy_raw_header(ac_list->header_list[lo].raw_header);
                
                broadcastToPeers(packet);
            }
        }
    }else {
        //id1, id2, ts1, ts2
        AROLog::Log().Print(logDEBUG,1,tag.c_str(),"Notify of sync point %d-%d %lld-%lld\n",msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        
        header_syncrhonizer->notifyOfSyncPoint(&msgSyncPoint);
    }

}

void Application::header_mergeHeader(Raw_Header_C *raw_header){
//    AROLog::Log().Print(logINFO, 1, tag.c_str(), "Merge New Header %lld - %lld, %d\n", raw_header->from, raw_header->to, raw_header->size);
    if(!GLOBAL_SYNCED) {
        free(raw_header);
        strand.dispatch(boost::bind(&Peer::stop_bully, peer_list[pid]));
        return ;
    }
    
    app_pause();
    boost::mutex::scoped_lock lock(mutex);
    
    Raw_Header_C * copy = copy_raw_header(raw_header);
    merge_new_header(ac_list, copy);
    
    free(raw_header);
    
    BB_ing = false;
    GLOBAL_SYNCED = false;
    sync_header_with_self(ac_list);
    
    if (!bb_synchronizer->isEmpty()) {
        bb_reset();
    }
    bb_resume();
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - App Sync functionality

//Only be called by the sync Header maybe
void Application::app_resume(){
    AROLog_Print(logINFO, 1, tag.c_str(), "------------SYNC RESUME--------------\n");
    synchronizer->setSyncIDArrays(&ac_list->action_list[0].ts, &ac_list->action_list[0].hash, sizeof(Action_C) / sizeof(uint64_t));
    synchronizer->setNetworkPeriod(0.5);
    
    t_try_bb.expires_from_now(boost::posix_time::seconds(T_TRY_BB_TIMEOUT));
    t_try_bb.async_wait(strand.wrap(boost::bind(&Application::try_bb,this, boost::asio::placeholders::error)));
    
    strand.dispatch(boost::bind(&Application::app_periodicSync,this,error));
    
}
void Application::app_reset(){
    synchronizer->reset();
}

void Application::app_pause(){
    t_app_sync.cancel();
    t_try_bb.cancel();
        
    AROLog::Log().Print(logINFO, 1, tag.c_str(), "------------SYNC Freeze--------------\n");
}

void Application::app_mergeAction(uint64_t ts){ //the content of action
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
     if (!GLOBAL_SYNCED) return ;
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
        AROLog::Log().Print(logDEBUG,1,tag.c_str(),"Notify of sync point %d-%d %lld-%lld\n",msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        synchronizer->notifyOfSyncPoint(&msgSyncPoint);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - between Header and action

void Application::try_bb(boost::system::error_code e){ //try bb
    if(e == boost::asio::error::operation_aborted ) return;
   
    if (GLOBAL_SYNCED && ac_list->action_count > BB_SIZE && GLOBAL_SYNCED && !BB_ing) {
        BB_ing = true;
        AROLog::Log().Print(logINFO, 1, tag.c_str(), "Try BB\n");
        strand.dispatch(boost::bind(&Peer::start_bully, peer_list[pid], error));
    }else{
        t_try_bb.expires_from_now(boost::posix_time::seconds(T_TRY_BB_TIMEOUT));
        t_try_bb.async_wait(strand.wrap(boost::bind(&Application::try_bb,this, boost::asio::placeholders::error)));
    }
}

void Application::pack_full_bb(){ //pack complete bb
    if (!GLOBAL_SYNCED) {
        strand.dispatch(boost::bind(&Peer::stop_bully, peer_list[pid]));
        return ;
    }
    app_pause();
    AROLog::Log().Print(logINFO, 1, tag.c_str(), "Pack full BB\n");
    BackBundle_C * bb = init_BB_with_actions(ac_list->action_list, BB_SIZE);
    Raw_Header_C * raw_header = init_raw_header_with_BB(bb);
    
    {
        boost::mutex::scoped_lock lock(mutex);
        merge_new_header_with_BB(ac_list, raw_header, bb); //also cleans itself btw
        //Alocate a new one
        Packet packet; packet.flag = HEADER_MERGE_HEADER; packet.content.raw_header = copy_raw_header(raw_header);
        broadcastToPeers(packet);
        BB_ing = false;
        GLOBAL_SYNCED = true; //WARNING: somethin fishy about that
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
    AROLog_Print(logINFO, 1, tag.c_str(), "------BB_RESUME-------\n");
    bb_synchronizer->setSyncIDArrays(&bb->action_list[0].ts , &bb->action_list[0].hash, sizeof(Action_C) / sizeof(uint64_t));
    bb_synchronizer->setNetworkPeriod(0.5);
    
    t_bb_sync.expires_from_now(boost::posix_time::seconds(2 * T_BB_SYNC_TIMEOUT));
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
    if(e == boost::asio::error::operation_aborted ) return;
    
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
    
    Header_C header = ac_list->header_list[ac_list->header_count-1];
    if(header.synced) return;

    merge_action_into_header(&header, ts);
    AROLog::Log().Print(logDEBUG, 1, tag.c_str(), "BB-merge action %lld\n",ts);
    update_sync_state(&header);
    
    if (header.synced) {
        AROLog::Log().Print(logDEBUG, 1, tag.c_str(), "BB is synced\n");
        boost::mutex::scoped_lock lock(mutex);
        remove_duplicate_actions(ac_list, header.bb);
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
        AROLog::Log().Print(logDEBUG,1,tag.c_str(),"Notify of sync point in BB %d-%d %lld-%lld\n",msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        bb_synchronizer->notifyOfSyncPoint(&msgSyncPoint);
    }
    
}

