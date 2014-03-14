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
    
    ac_list = (Action_C*) malloc(sizeof(Action_C) * 100);
    synchronizer = new AROObjectSynchronizer(NULL,NULL,  0 ,this); //
    synchronizer->setDebugInfo(0, "AC-Sync");
    synchronizer->setNetworkPeriod(0.5);
    
    count = 0;
    
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
    t_sync.expires_from_now(boost::posix_time::seconds(1));
    t_sync.async_wait(strand.wrap(boost::bind(&ActionList::periodicSync,this, boost::asio::placeholders::error)));
    
    synchronizer->setNetworkPeriod(0.5);
    BB_started = false;
}
//Change this to periosdic sync?


//    t_clean_up.expires_from_now(boost::posix_time::seconds(5));
//    t_clean_up.async_wait(strand.wrap(boost::bind(&ActionList::search_good_peer,this, boost::asio::placeholders::error)));

void ActionList::processAppDirective(SyncPoint p,  bool flag){
    boost::this_thread::sleep(boost::posix_time::seconds(1));

    if (flag) {
        strand.dispatch(boost::bind(&ActionList::processSyncPoint,this,p));
    }else{
        strand.dispatch(boost::bind(&ActionList::mergeAction,this,p));
    }
}

void ActionList::mergeAction(SyncPoint p){
    //Find the insertion
    boost::mutex::scoped_lock lock(mutex);
    int lo = 0; int hi = count - 1;
    binaryReduceRangeWithKey(lo,hi,p.ts1,ac_list[lo].ts,ac_list[hi].ts,ac_list[pivot].ts);
    
    if (lo > hi) {
        AROLog::Log().Print(logINFO, 1, "SYNC", "Peer # %d merge %lld low %d, high %d with total %d\n", pid,p.ts1, lo,hi, count);
        if (lo < count) {
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
    
//    AROLog::Log().Print(logINFO, 1, "SYNC", "Peer # %d periodic sync\n",pid);
    synchronizer->processSyncState(&ac_list[0].ts, &ac_list[0].hash, sizeof(Action_C) / sizeof(uint64_t), count);
    
    t_sync.expires_from_now(boost::posix_time::seconds(1));
    t_sync.async_wait(strand.wrap(boost::bind(&ActionList::periodicSync,this, boost::asio::placeholders::error)));
}


//////////////////////
void ActionList::add_new_action(){
    
    if(count <=3){
        
    
        SyncPoint s ; s.ts1 = AOc_localTimestamp(); s.hash = 0;
        AROLog::Log().Print(logINFO, 1, "SYNC","Peer # %d new action %lld created\n",pid, s.ts1);
        strand.dispatch(boost::bind(&ActionList::mergeAction, this, s));
        }
//    count++
}

//below are the only ways that synchronizer talks with sender
//Called by the synchronizer
void ActionList::sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender){
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
//    AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d sendRequestForSyncPoint from  %d %d %lld %lld\n",pid, syncPoint->id1, syncPoint->id2, syncPoint->ts1, syncPoint->ts2);
       for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i!=pid) {
            peer_list[i]->enqueue(boost::protect(boost::bind(&ActionList::processAppDirective,peer_list[i]->action_list,*syncPoint,true)));
        }
    }
}

void ActionList::notificationOfSyncAchieved(double networkPeriod, int code, void *sender){
    AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d sync achieved at %llf\n",pid, networkPeriod);
    
    if (networkPeriod < 60) networkPeriod *= 1.5;
    if (sender == synchronizer) synchronizer->setNetworkPeriod(networkPeriod);
}


void ActionList::processSyncPoint(SyncPoint msgSyncPoint){ //TODO: add a peer id here
    //Ask for between certain range
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    if ((msgSyncPoint.id1 == 0) && (msgSyncPoint.id2 == 0)) {
        
        if ((msgSyncPoint.ts1 == 0ll) && (msgSyncPoint.ts2 == 0ll)) { //0,0,0,0 global picture
            
            if (count > 0) {
                SyncPoint global_pic;
                global_pic.id1 = 1; global_pic.id2 = count;
                global_pic.ts1 = ac_list[0].ts;global_pic.ts2 = ac_list[count-1].ts;
                global_pic.hash = 0; global_pic.res = count;
                
                AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d responding to global pic with %d-%d %lld-%lld\n",pid,global_pic.id1,global_pic.id2,global_pic.ts1,global_pic.ts2);
                for (unsigned int i = 0; i < peer_list.size(); i++) {
                    if (i!=pid) {
                        peer_list[i]->enqueue(boost::protect(boost::bind(&ActionList::processAppDirective,peer_list[i]->action_list,global_pic,true)));
                    }
                }
            }
            
        } else if (count > 0) { //current region not empty
            
            int lo = 0, hi = count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,ac_list[lo].ts, ac_list[hi].ts, ac_list[pivot].ts);
            for (; lo <= hi; lo++) {
                SyncPoint one_point;
                one_point.id1 = lo; one_point.ts1 = ac_list[lo].ts;
                one_point.id2 = 0; one_point.ts2 = 0ll;
                
                for (unsigned int i = 0; i < peer_list.size(); i++) {
                    if (i!=pid) {
                        peer_list[i]->enqueue(boost::protect(boost::bind(&ActionList::processAppDirective, peer_list[i]->action_list,one_point,false)));
                    }
                }
//                AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d responding to request for %d %lld\n",pid, lo, one_point.ts1);
            }
        }
    }else {
        //id1, id2, ts1, ts2
        AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d notify of sync point %d-%d %lld-%lld\n",pid, msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        synchronizer->notifyOfSyncPoint(&msgSyncPoint);
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

