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

#include "AROLogInterface.h"
#include "AROUtil_C.h"
#include "diff.h"
#include "packet.h"

#include <algorithm>
Application::Application(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list):
io_service(io_service),
strand(strand),
peer_list(peer_list),
pid(pid),
t_app_sync(io_service, boost::posix_time::seconds(0)),
t_bb_sync(io_service, boost::posix_time::seconds(0)),
t_try_bb(io_service, boost::posix_time::seconds(0)),
t_header_sync(io_service, boost::posix_time::seconds(0))
{
    ac_list = init_default_actionList();
    //Setting up tags and syncrhonizer
    std::stringstream ss; ss<<"AC#"<<pid; tag = ss.str();
    
    ss.clear(); ss<<"H-SYNC#"<<pid; std::string sync_h_tag = ss.str();
    header_syncrhonizer = new AROObjectSynchronizer(NULL,NULL,0,this);
    header_syncrhonizer->setDebugInfo(0, sync_h_tag.c_str());
    
    ss.clear(); ss<<"SYNC#"<<pid; std::string sync_tag = ss.str();
    synchronizer = new AROObjectSynchronizer(NULL,NULL,  0 ,this); //
    synchronizer->setDebugInfo(0, sync_tag.c_str());
    
    ss.clear(); ss<<"BB-SYNC#"<<pid; std::string sync_bb_tag = ss.str();
    bb_synchronizer = new AROObjectSynchronizer(NULL,NULL, 0, this);
    bb_synchronizer->setDebugInfo(0, sync_bb_tag.c_str());
#ifdef SYN_CHEAT
    AROLog(logINFO, 1, tag.c_str(),"CHEAT synchronizer for base case.\n");
#endif
    
}

Application::~Application(){
    AROLog(logINFO, 1, tag.c_str(), "Header %d\n", ac_list->header_count);
    Header_C * header = get_latest_header(ac_list);
    if (header) {
        BackBundle_C * bb = header->bb;
        if (bb) {
            AROLog(logINFO, 1,tag.c_str() ,"BB Synced : %c with size %d\n",header->synced? 'Y':'N', bb->action_count);
            if (!header->synced) {
                Diff::get().set_unsynced_actions(bb);
            }
            for (unsigned int j = 0; j < bb->action_count; j++) {
                AROLog(logINFO, 1, tag.c_str(), "BB %d - %lld\n",j,bb->action_list[j].ts);
            }
        }
        
    }
    
    
    if (header_syncrhonizer) delete header_syncrhonizer;
    if (bb_synchronizer) delete bb_synchronizer;
    if (synchronizer)  delete synchronizer;
    
    free_actionList(ac_list);
}

void Application::pause(){
    t_header_sync.cancel();
    t_app_sync.cancel();
    t_bb_sync.cancel();
    t_try_bb.cancel();
}

void Application::resume(){
    header_syncrhonizer->setNetworkPeriod(0.5);
    strand.post(boost::bind(&Application::header_periodicSync,this,error));
    bb_resume();
}


ActionList_C* Application::get_actionList(){
    return ac_list;
}

void Application::processPacket(Packet *p){
    
    BLOCK_PAYLOAD * content = p->content;
    switch(p->flag){
        case Packet::HEADER_PROCESS_SP:
            strand.post(boost::bind(&Application::header_processSyncPoint,this,content->sync_point));
            break;
        case Packet::HEADER_MERGE_HEADER:
            strand.post(boost::bind(&Application::header_mergeHeader,this, copy_raw_header(content->raw_header)));
            break;
#ifdef SYN_CHEAT
        case Packet::HEADER_COUNT:
            strand.post(boost::bind(&Application::header_processCount, this, content->header_count));
            break;
#endif
        case Packet::APP_PROCESS_SP:
            if(is_header_fully_synced())
                strand.post(boost::bind(&Application::app_processSyncPoint,this,content->sync_point));
            break;
        case Packet::APP_MERGE_ACTION:
            if(is_header_fully_synced())
                strand.post(boost::bind(&Application::app_mergeAction,this,content->ts));
            break;
        case Packet::BB_PROCESS_SP:
            strand.post(boost::bind(&Application::bb_processSyncPoint,this,content->sync_point));
            break;
        case Packet::BB_MERGE_ACTION:
            strand.post(boost::bind(&Application::bb_mergeAction,this, content->ts));
            break;
        default:
            break;
    }
    delete p;
}


void Application::add_new_action(){
    uint64_t ts = AOc_localTimestamp();
    AROLog(logINFO, 1, tag.c_str(),"New action %lld created\n", ts);
    strand.post(boost::bind(&Application::app_mergeAction, this, ts));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - AROSyncResponder interface
void Application::sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender){
    if (sender == header_syncrhonizer) {
        Packet * p = new Packet(Packet::HEADER_PROCESS_SP); p->content->sync_point = *syncPoint;
        peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers,peer_list[pid], p)));
    }else if (sender == synchronizer) {
        Packet * p = new Packet(Packet::APP_PROCESS_SP); p->content->sync_point = *syncPoint;
        AROLog(logDEBUG, 1, tag.c_str(), "App Request %d - %d, %lld - %lld\n",syncPoint->id1, syncPoint->id2, syncPoint->ts1, syncPoint->ts2);
        peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers,peer_list[pid], p )));
    }else if(sender == bb_synchronizer){
        Packet * p = new Packet(Packet::BB_PROCESS_SP); p->content->sync_point = *syncPoint;
        Header_C * header = get_latest_header(ac_list);
        
        if(header && !header->synced){
            BackBundle_C  * bb = header->bb;
            //Check if it is already in BB, avoid overload
            bool already_in = false;
            if(syncPoint->id1 == 0 && syncPoint->id2 == 0){ //&& syncPoint->ts1 != syncPoint->ts2
                already_in = true;
            }else{
                int b_lo = 0, b_hi = bb->action_count - 1;
                binaryReduceRangeWithRange(b_lo, b_hi, syncPoint->ts1, syncPoint->ts2,bb->action_list[b_lo].ts, bb->action_list[b_hi].ts, bb->action_list[pivot].ts);
//                AROLog(logINFO, 1, "BB SELF", "lo %d- hi %d vs %d %lld - - %d %lld\n",b_lo,b_hi, syncPoint->id1,  syncPoint->ts1, syncPoint->id2, syncPoint->ts2);
                if (b_hi - b_lo == syncPoint->id2 - syncPoint->id1) {
                    if (Diff::get().check_hash(syncPoint->id1-1, syncPoint->id2-1, b_lo, bb->action_list)) {
                        already_in = true;
                    }
                }
            }
            if (!already_in) {
                int a_lo = 0, a_hi = ac_list->action_count - 1;
                binaryReduceRangeWithRange(a_lo, a_hi, syncPoint->ts1, syncPoint->ts2,ac_list->action_list[a_lo].ts, ac_list->action_list[a_hi].ts, ac_list->action_list[pivot].ts);
                AROLog(logINFO, 1, "SYNC SELF", "lo %d - hi %d vs id1 %d %lld - %d %lld\n", a_lo, a_hi, syncPoint->id1, syncPoint->ts1, syncPoint->id2,syncPoint->ts2);
                
                //TODO: Need to add a cheat for checking hash
                if (a_hi - a_lo == syncPoint->id2 - syncPoint->id1) {
                    if (Diff::get().check_hash(syncPoint->id1-1, syncPoint->id2-1, a_lo, ac_list->action_list)) {
                        AROLog(logINFO, 1, "HASH", "MATCH\n");
                        boost::mutex::scoped_lock lock(mutex);
                        for (; a_lo <= a_hi; a_lo++){
                            merge_action_into_header(header, ac_list->action_list[a_lo].ts);
                            AROLog(logINFO, 1, "Add", "- %lld\n",ac_list->action_list[a_lo].ts);
                        }
                        update_sync_state(ac_list); //should be on header?
                        
                        AROLog(logINFO, 1, "SYNC SELF", "we have %d actions and synced: %c\n", bb->action_count, header->synced ? 'Y':'N');
                        if(header->synced){
                            t_bb_sync.cancel();
                            remove_duplicate_actions(ac_list, bb);
                            synchronizer->reset();
                            AROLog(logINFO, 1, "SYNC SELF", "BB is fully Synced\n");
                        }
                    }else{
                        AROLog(logINFO, 1, "HASH", "Not MATCH\n");
                    }
                }
            }
            
        }
      
        peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers,peer_list[pid], p)));;// broadcastToPeers(p);
    }
}

void Application::notificationOfSyncAchieved(double networkPeriod, int code, void *sender){
    if (code == 1) {
        if (networkPeriod < 60) networkPeriod *= 1.5;
        
        if (sender == header_syncrhonizer) {
//                 if (is_header_fully_synced()) {
//                       AROLog(logDEBUG, 1, tag.c_str(), "Header Sync achieved at %llf\n", networkPeriod);
            //            app_resume();
            //        }else{
            //            t_app_sync.cancel();
            //        }
            header_syncrhonizer->setNetworkPeriod(networkPeriod);
            //        bb_resume();
        }else if (sender == synchronizer) {
            AROLog(logDEBUG,1,tag.c_str(),"App Sync achieved at %llf\n", networkPeriod);
            synchronizer->setNetworkPeriod(networkPeriod);
        }else if(sender == bb_synchronizer) {
            AROLog(logINFO,1, tag.c_str(),"BB Sync achieved at %llf\n", networkPeriod);
            bb_synchronizer->setNetworkPeriod(2.0);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - Header Sync functionlity

void Application::header_periodicSync(const boost::system::error_code &error){
    if ( error == boost::asio::error::operation_aborted) return ;
    
    AROLog(logDEBUG, 1, tag.c_str(), "H-Periodic sync\n");
    
    header_syncrhonizer->processSyncState(&ac_list->header_list[0].ts, &ac_list->header_list[0].hash, sizeof(Header_C) / sizeof(uint64_t), ac_list->header_count);
    
    t_header_sync.expires_from_now(boost::posix_time::seconds(T_HEADER_SYNC_TIMEOUT));
    t_header_sync.async_wait(strand.wrap(boost::bind(&Application::header_periodicSync,this, boost::asio::placeholders::error)));
}

#ifdef SYN_CHEAT
void Application::header_processCount(unsigned int header_count){
    strand.post(boost::bind(&Application::notificationOfSyncAchieved,this,header_syncrhonizer->getNetworkPeriod(), 0, header_syncrhonizer));
    
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
                
                AROLog(logINFO,1,tag.c_str(),"Responding to global pic with %d-%d %lld-%lld\n",global_pic.id1,global_pic.id2,global_pic.ts1,global_pic.ts2);
                
                Packet * p = new Packet(Packet::HEADER_PROCESS_SP); p->content->sync_point = global_pic;
                peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers,peer_list[pid], p )));
            }
            
#ifdef SYN_CHEAT
            Packet * p = new Packet(Packet::HEADER_COUNT); p->content->header_count = ac_list->header_count;
            peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers,peer_list[pid],p )));
#endif
            
        } else if (ac_list->header_count > 0){
            
            int lo = 0, hi = ac_list->header_count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,ac_list->header_list[lo].ts, ac_list->header_list[hi].ts, ac_list->header_list[pivot].ts);
            
            for (; lo <= hi; lo++) {
                
                Packet * p = new Packet(Packet::HEADER_MERGE_HEADER);
                p->content->raw_header = copy_raw_header(ac_list->header_list[lo].raw_header);
                peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers, peer_list[pid],p )));
            }
        }
    }else {
        //id1, id2, ts1, ts2
        AROLog(logDEBUG,1,tag.c_str(),"Notify of sync point %d-%d %lld-%lld\n",msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        
        header_syncrhonizer->notifyOfSyncPoint(&msgSyncPoint);
    }
    
}

void Application::header_mergeHeader(Raw_Header_C *raw_header){
    pause();
    AROLog(logINFO, 1, tag.c_str(), "Sync Self with header\n");
    {
        boost::mutex::scoped_lock lock(mutex);
        merge_new_header(ac_list, raw_header);
        sync_header_with_self(ac_list);
        
        
        synchronizer->reset();
        bb_synchronizer->reset(); //more like transition synchronizer
    }
    resume();
}

bool Application::is_header_fully_synced(){
    boost::mutex::scoped_lock lock(mutex);
    return is_header_section_synced(ac_list);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - App Sync functionality

//Only be called by the sync Header maybe
void Application::app_resume(){
    //current timer already expires, we call it again, else, do nothing
    if (t_app_sync.expires_from_now(boost::posix_time::seconds(T_APP_SYNC_TIMEOUT)) <= 0) {
        strand.post(boost::bind(&Application::app_periodicSync,this,error));
        //        synchronizer->setNetworkPeriod(0.5);
        //        AROLog(logINFO, 1, tag.c_str(), "------------SYNC RESUME--------------\n");
    }
    
    if (t_try_bb.expires_from_now(boost::posix_time::seconds(T_TRY_BB_TIMEOUT)) <= 0) {
        t_try_bb.expires_from_now(boost::posix_time::seconds(T_TRY_BB_TIMEOUT));
        t_try_bb.async_wait(strand.wrap(boost::bind(&Application::try_bb,this, boost::asio::placeholders::error)));
    }
}

void Application::app_mergeAction(uint64_t ts){ //the content of action
    boost::mutex::scoped_lock lock(mutex);
    merge_new_action(ac_list, ts);
}

void Application::app_periodicSync(const boost::system::error_code &error){
    if ( error == boost::asio::error::operation_aborted) return ;
    
    AROLog(logDEBUG, 1, tag.c_str(), "Periodic sync\n");
    synchronizer->processSyncState(&ac_list->action_list[0].ts, &ac_list->action_list[0].hash, sizeof(Action_C) / sizeof(uint64_t), ac_list->action_count);
    
    t_app_sync.expires_from_now(boost::posix_time::seconds(T_APP_SYNC_TIMEOUT));
    t_app_sync.async_wait(strand.wrap(boost::bind(&Application::app_periodicSync,this, boost::asio::placeholders::error)));
}

void Application::app_processSyncPoint(SyncPoint msgSyncPoint){
    
    
    if ((msgSyncPoint.id1 == 0) && (msgSyncPoint.id2 == 0)) {
        
        if ((msgSyncPoint.ts1 == 0ll) && (msgSyncPoint.ts2 == 0ll)) { //0,0,0,0 global picture
            
            if (ac_list->action_count > 0) {
                SyncPoint global_pic;
                global_pic.id1 = 1; global_pic.id2 = ac_list->action_count;
                global_pic.ts1 = ac_list->action_list[0].ts;global_pic.ts2 = ac_list->action_list[ac_list->action_count-1].ts;
                global_pic.hash = 0; global_pic.res = ac_list->action_count;
                
                AROLog(logINFO,1,tag.c_str(),"App Responding to global pic with %d-%d %lld-%lld\n",global_pic.id1,global_pic.id2,global_pic.ts1,global_pic.ts2);
                
                Packet *p = new Packet(Packet::APP_PROCESS_SP); p->content->sync_point = global_pic;
                peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers,peer_list[pid], p )));
            }
        } else if (ac_list->action_count > 0) { //0, 0, ts1, ts2 request for merge
            
            int lo = 0, hi = ac_list->action_count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,ac_list->action_list[lo].ts, ac_list->action_list[hi].ts, ac_list->action_list[pivot].ts);
            AROLog(logINFO, 1, tag.c_str(), "App Respond lo %d - hi %d syncPoint in Sync: %d-%d %lld-%lld\n", lo,hi,msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
            for (; lo <= hi; lo++) {
                Packet *p = new Packet(Packet::APP_MERGE_ACTION); p->content->ts = ac_list->action_list[lo].ts;
                peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers,peer_list[pid], p )));
            }
        }
    }else {
        //id1, id2, ts1, ts2
        AROLog(logINFO,1,tag.c_str(),"Notify of sync point %d-%d %lld-%lld\n",msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        synchronizer->notifyOfSyncPoint(&msgSyncPoint);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - between Header and action

void Application::try_bb(boost::system::error_code e){ //try bb
    if(e == boost::asio::error::operation_aborted ) return;
    
    if (is_header_fully_synced() && ac_list->action_count > BB_SIZE) {
        AROLog(logINFO, 1, tag.c_str(), "Try BB\n");
        strand.post(boost::bind(&Peer::start_bully, peer_list[pid], error));
    }else{
        t_try_bb.expires_from_now(boost::posix_time::seconds(T_TRY_BB_TIMEOUT));
        t_try_bb.async_wait(strand.wrap(boost::bind(&Application::try_bb,this, boost::asio::placeholders::error)));
    }
}

void Application::pack_full_bb(){ //pack complete bb
    //Stop first, check second
    
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        peer_list[i]->enqueue(boost::protect(boost::bind(&Peer::stop_bully, peer_list[i])));
    }
    if (!is_header_fully_synced() || BB_SIZE > ac_list->action_count) return ;
    
    pause();
    {
        boost::mutex::scoped_lock lock(mutex);
        AROLog(logINFO, 1, tag.c_str(), "Pack full BB\n");
        BackBundle_C * bb = init_BB_with_sampled_randomization(ac_list);
        Raw_Header_C * raw_header = init_raw_header_with_BB(bb);
        print_raw_header(raw_header);
        merge_new_header_with_BB(ac_list, raw_header, bb); //also cleans itself btw
        
        Packet * p = new Packet(Packet::HEADER_MERGE_HEADER); p->content->raw_header = copy_raw_header(raw_header);
        peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers,peer_list[pid],p)));
        
#warning Cheating for hashing
        Diff::get().set_synced_bb(bb);
        
        synchronizer->reset();
        bb_synchronizer->reset();
    }
    resume();
    //TODO: want to change here such that we get a request from a BB_Synchronizer and we wrap around it
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma mark - BB functionality

void Application::bb_resume(){
    BackBundle_C * bb =  get_latest_BB(ac_list);
    if (!bb) return;
    
    if (t_bb_sync.expires_from_now(boost::posix_time::seconds(T_BB_SYNC_TIMEOUT))<= 0) {
        AROLog(logINFO, 1, tag.c_str(), "------BB_RESUME-------\n");
        bb_synchronizer->setNetworkPeriod(3);
        strand.post(boost::bind(&Application::bb_periodicSync,this,error));
    }
}

#warning - See comment
//technically it should only be the latest one, we will have at most one unsynced bundle
//But in future, it will be used by other application to help it to sync up, but should be treated differently
//Because this one is only used for transition between active->header, we have have at most unsynced bb_synchronizer
void Application::bb_periodicSync(const boost::system::error_code &e){
    if(e == boost::asio::error::operation_aborted ) return;
    
    BackBundle_C * bb = get_latest_BB(ac_list);
    if (!bb) return;
    AROLog(logDEBUG, 1, tag.c_str(), "BB-Periodic sync\n");

 
//    strand.post(boost::bind(&AROObjectSynchronizer::processSyncState,bb_synchronizer, &bb->action_list[0].ts, &bb->action_list[0].hash, sizeof(Action_C) / sizeof(uint64_t), bb->action_count));
    bb_synchronizer->processSyncState(&bb->action_list[0].ts, &bb->action_list[0].hash, sizeof(Action_C) / sizeof(uint64_t), bb->action_count);
    
    t_bb_sync.expires_from_now(boost::posix_time::seconds(T_BB_SYNC_TIMEOUT));
    t_bb_sync.async_wait(strand.wrap(boost::bind(&Application::bb_periodicSync,this, boost::asio::placeholders::error)));
}

void Application::bb_mergeAction(uint64_t ts){
    if (ac_list->header_count == 0) return ;
    
    Header_C  * header = get_latest_header(ac_list);
    if(header->synced == true) return;
    //If so , then do something
    boost::mutex::scoped_lock lock(mutex);
    merge_action_into_header(header, ts);
    update_sync_state(ac_list);
    AROLog(logINFO, 1, "Merge", "%lld - total %d\n",ts, header->bb->action_count);
    if (header->synced) {
        pause();
        remove_duplicate_actions(ac_list, header->bb);
        synchronizer->reset();
        resume(); //with less network process?
        AROLog(logINFO, 1, tag.c_str(), "BB SYNCED here\n");
    }
}

void Application::bb_processSyncPoint(SyncPoint msgSyncPoint){
    Header_C * header = get_latest_header(ac_list);
    
    
    if (!header) return ;
    BackBundle_C * bb  = header->bb;
    if ((msgSyncPoint.id1 == 0) && (msgSyncPoint.id2 == 0)) {
        
        if ((msgSyncPoint.ts1 == 0ll) && (msgSyncPoint.ts2 == 0ll)) { //0,0,0,0 global picture
            //BB_GLOBAL
            if (bb->action_count > 0) {
                SyncPoint global_pic;
                global_pic.id1 = 1; global_pic.id2 = bb->action_count;
                global_pic.ts1 = bb->action_list[0].ts;global_pic.ts2 = bb->action_list[bb->action_count-1].ts;
                global_pic.hash = 0; global_pic.res = bb->action_count;
                
                AROLog(logINFO,1,tag.c_str(),"BB to global pic with %d-%d %lld-%lld\n",global_pic.id1,global_pic.id2,global_pic.ts1,global_pic.ts2);
                Packet *p = new Packet(Packet::BB_PROCESS_SP); p->content->sync_point = global_pic;
                peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers, peer_list[pid],p )));
            }
            
        } else { //current region not empty
            //BB_MERGE_ACTION
            int lo = 0, hi = bb->action_count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,bb->action_list[lo].ts, bb->action_list[hi].ts, bb->action_list[pivot].ts);
            //wait, we can also search for that in synchronizer to see if thats matching?
            
            AROLog(logINFO, 1, tag.c_str(), "Sendout lo %d - hi %d syncPoint to other BB: %d-%d %lld-%lld\n", lo,hi,msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
            
            for (; lo <= hi; lo++) {
                Packet *p = new Packet(Packet::BB_MERGE_ACTION); p->content->ts = bb->action_list[lo].ts;
                peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::broadcastToPeers, peer_list[pid],p )));
            }
        }
    }else {
        
        if(!header->synced){
            AROLog(logINFO,1,tag.c_str(),"Notify of sync point in BB %d-%d %lld-%lld\n",msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        }
        bb_synchronizer->notifyOfSyncPoint(&msgSyncPoint);
    }
    
}

