//
//  synchronizer.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-04.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "actionList.h"
#include "peer.h"
#include <boost/date_time.hpp>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <algorithm>

#include "AROLog.h"
#include "AROUtil_C.h"
const unsigned int BB_SIZE = 13;

ActionList::ActionList(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list,PriorityPeer * priority_peer):
io_service(io_service),
strand(strand),
peer_list(peer_list),
pid(pid),
priority_peer(priority_peer),
t_sync(io_service, boost::posix_time::seconds(0)),
t_clean_up(io_service, boost::posix_time::seconds(0))
{
    //strand_.dispatch(boost::bind(&boost::asio::deadline_timer::cancel,&sync_timer));
    
    ac_list = (Action_C*) malloc(sizeof(Action_C) * 100);
    synchronizer = new AROObjectSynchronizer(NULL,NULL,0,this);
    synchronizer->setDebugInfo(0, "AC-Sync");
    synchronizer->setNetworkPeriod(5);
    
    count = 0; //???
    
}

ActionList::~ActionList(){
    //    AROLog::Log().Print(logINFO, 1, "SYNC", "Peer # %d summary\nBackBundle Region\n",pid);
    delete synchronizer;
    for (unsigned int i= 0; i < count; i++) {
        AROLog::Log().Print(logINFO, 1, "SYNC", "Action # %d - %lld\n",i, ac_list[i].ts);
    }
    
    free(ac_list);
}

void ActionList::pause(){
    t_clean_up.cancel();
    t_sync.cancel();
    
    BB_started = false;
}

void ActionList::resume(){
    t_sync.expires_from_now(boost::posix_time::seconds(3));
    t_sync.async_wait(strand.wrap(boost::bind(&ActionList::periodicSync,this, boost::asio::placeholders::error)));
    synchronizer->setNetworkPeriod(3);
    BB_started = false;
}
//Change this to periosdic sync?


//    t_clean_up.expires_from_now(boost::posix_time::seconds(5));
//    t_clean_up.async_wait(strand.wrap(boost::bind(&ActionList::search_good_peer,this, boost::asio::placeholders::error)));

//void ARONetworkFeed::processAppDirective(AOc_BlockPayload *blkpyld, boost::shared_ptr<AROConnection> src)
//called process sync point
//since we are not using the block pay load, this is an actually syncPoint
void ActionList::broadcast(SyncPoint p){ //Message?
    //    AROLog::Log().Print(logINFO, 1, "SYNC", "Peer # %d broadcast %d %d %lld %lld\n",pid, p.id1, p.id2, p.ts1, p.ts2);
    //    for (unsigned int i = 0; i < peer_list.size(); i++) {
    //        if (i!=pid) {
    //            peer_list[i]->enqueue(boost::protect(boost::bind(&ActionList::processSyncPoint, peer_list[i]->action_list,p, pid)));
    //        }
    //    }
    
}


//Called by processMessage
//need a flag??? to differentiate from merging and that?
void ActionList::processAppDirective(SyncPoint p,  bool flag){
    if (flag) {
        //        peer_list[pid]->enqueue(boost::protect(boost::bind(&ActionList::processSyncPoint, this, p)));
        boost::this_thread::sleep(boost::posix_time::milliseconds(500));
        processSyncPoint(p);
    }else{
        //        peer_list[pid]->enqueue(boost::protect(boost::bind(&ActionList::mergeAction, this, p)));
        mergeAction(p);
    }
}

void ActionList::mergeAction(SyncPoint p){
    //Find the insertion
    boost::mutex::scoped_lock lock(mutex);
    //    AROLog::Log().Print(logINFO, 1, "SYNC","Peer # %d merge action %lld\n",pid, p.ts1);
    //
    int lo = 0; int hi = count - 1;
    binaryReduceRangeWithKey(lo,hi,p.ts1,ac_list[lo].ts,ac_list[hi].ts,ac_list[pivot].ts);
    
    if (lo > hi) {
        AROLog::Log().Print(logINFO, 1, "SYNC", "Peer # %d merge low %d, high %d with total %d\n", pid, lo,hi, count);
        if (lo < count) {
            //TODO: check if range is right?
            //move everything one step after
            for (int i = lo; i < count; i++) {
                ac_list[i+1].ts = ac_list[i].ts;
            }
        }
        ac_list[lo].ts = p.ts1;
        ac_list[lo].hash = 0;
        count++;
    }
}


void ActionList::periodicSync(const boost::system::error_code &error){
    if ( error == boost::asio::error::operation_aborted) return ;
    
    AROLog::Log().Print(logINFO, 1, "SYNC", "Peer # %d periodic sync\n",pid);
    synchronizer->processSyncState(&ac_list[0].ts, &ac_list[0].hash, 1, count);
    
    t_sync.expires_from_now(boost::posix_time::seconds(3));
    t_sync.async_wait(strand.wrap(boost::bind(&ActionList::periodicSync,this, boost::asio::placeholders::error)));
}


//////////////////////
void ActionList::add_new_action(){
    ac_list[count].ts = AOc_localTimestamp();
    ac_list[count].hash = 0;
    count++;
    AROLog::Log().Print(logINFO, 1, "SYNC","Peer # %d new action %d %lld created\n",pid, count,ac_list[count-1].ts);
}

//below are the only ways that synchronizer talks with sender
//Called by the synchronizer
void ActionList::sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender){
    boost::mutex::scoped_lock lock(mutex);
    AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d sendRequestForSyncPoint from  %d %d %lld %lld\n",pid, syncPoint->id1, syncPoint->id2, syncPoint->ts1, syncPoint->ts2);
    
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i!=pid) {
            peer_list[i]->enqueue(boost::protect(boost::bind(&ActionList::processAppDirective,peer_list[i]->action_list,*syncPoint,true)));
        }
    }
}

void ActionList::notificationOfSyncAchieved(double networkPeriod, void *sender){
    AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d sync achieved at %llf\n",pid, networkPeriod);
    
    if (networkPeriod < 60) networkPeriod *= 1.5;
    if (sender == synchronizer) synchronizer->setNetworkPeriod(networkPeriod);
}


void ActionList::processSyncPoint(SyncPoint msgSyncPoint){ //TODO: add a peer id here
    //Ask for between certain range
    if ((msgSyncPoint.id1 == 0) && (msgSyncPoint.id2 == 0)) {
        //Not user the global picture
        if ((msgSyncPoint.ts1 == 0ll) && (msgSyncPoint.ts2 == 0ll)) { //0,0,0,0 global picture
            
            if (count > 0) {
                SyncPoint global_pic;
                global_pic.id1 = 1; global_pic.id2 = count;
                global_pic.ts1 = ac_list[0].ts;global_pic.ts2 = ac_list[count-1].ts;
                global_pic.hash = 0;
                
                for (unsigned int i = 0; i < peer_list.size(); i++) {
                    if (i!=pid) {
                        peer_list[i]->enqueue(boost::protect(boost::bind(&ActionList::processAppDirective,peer_list[i]->action_list,global_pic,true)));
                    }
                }
                AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d responding to global pic\n",pid);
            }
            
        } else if (count > 0) { //current region not empty
            
            int lo = 0, hi = count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,ac_list[lo].ts, ac_list[hi].ts, ac_list[pivot].ts);
            if (lo <= hi) {
                SyncPoint part_pic;
                part_pic.id1 = lo; part_pic.id2 = hi; //lo + 1, hight + 1
                part_pic.ts1 = ac_list[lo].ts; part_pic.ts2 = ac_list[hi].ts;
                part_pic.hash = 0;
                
                for (unsigned int i = 0; i < peer_list.size(); i++) {
                    if (i!=pid) {
                        peer_list[i]->enqueue(boost::protect(boost::bind(&ActionList::processAppDirective, peer_list[i]->action_list,part_pic,true)));
                    }
                }
                AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d responding to request for %d %lld- %d %lld\n",pid, lo, msgSyncPoint.ts1,hi, msgSyncPoint.ts2);
            }
            
            //TODO: fix here...
            
        }
        else{
            AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d ???\n",pid);
        }
        
    }else {
        //id1, id2, ts1, ts2
        AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d notify of sync point %lld-%lld\n",pid, msgSyncPoint.ts1,msgSyncPoint.ts2);
        
//        if (msgSyncPoint.ts1 == 0) {
//            SyncPoint p; p.ts1 = msgSyncPoint.ts2;
//            peer_list[pid]->enqueue(boost::protect(boost::bind(&ActionList::processAppDirective, this,p,false)));
//        }else if(msgSyncPoint.ts2 == 0 ){
//            SyncPoint p; p.ts1 = msgSyncPoint.ts1;
//        }
        peer_list[pid]->enqueue(boost::protect(boost::bind(&ActionList::processAppDirective, this,msgSyncPoint,false)));
        synchronizer->notifyOfSyncPoint(&msgSyncPoint);
        //add sync point in buffer
        //
        
    }
}

void ActionList::search_good_peer(boost::system::error_code e){
    if(e == boost::asio::error::operation_aborted ) return;
    
    boost::mutex::scoped_lock lock(mutex);
    
    //TODO: if synced, start finding dood peer
    
    t_clean_up.expires_from_now(boost::posix_time::seconds(7));
    t_clean_up.async_wait(strand.wrap(boost::bind(&ActionList::search_good_peer,this, boost::asio::placeholders::error)));
    
}

void ActionList::good_peer_first(){
    boost::mutex::scoped_lock lock(mutex);
    //TODO: Create new C strcutre to adapt BackBundle
}

