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
    t_broadcast(io_service, boost::posix_time::seconds(0)),
    t_clean_up(io_service, boost::posix_time::seconds(0))
{
//strand_.dispatch(boost::bind(&boost::asio::deadline_timer::cancel,&sync_timer));
    
    ac_list = (Action_C*) malloc(sizeof(Action_C) * 100);
    synchronizer = new AROObjectSynchronizer(&ac_list[0].ts,NULL,1,this);
    
}


ActionList::~ActionList(){

    AROLog::Log().Print(logINFO, 1, "SYNC", "Peer # %d summary\nBackBundle Region\n",pid);
    free(ac_list);
    delete synchronizer;
}

void ActionList::stop(){
    t_clean_up.cancel();
    t_broadcast.cancel();

    BB_started = false;
}

void ActionList::start(){
    t_broadcast.expires_from_now(boost::posix_time::seconds(5));
    t_broadcast.async_wait(strand.wrap(boost::bind(&ActionList::broadcast,this, boost::asio::placeholders::error)));
    //Change this to periosdic sync?
    
    
    t_clean_up.expires_from_now(boost::posix_time::seconds(5));
    t_clean_up.async_wait(strand.wrap(boost::bind(&ActionList::search_good_peer,this, boost::asio::placeholders::error)));
    
    BB_started = false;
    
}

void ActionList::broadcast(boost::system::error_code error){
    if(error == boost::asio::error::operation_aborted ) return;
    
    //Check Header first
    
    //Then check for sync content?
    
    t_broadcast.expires_from_now(boost::posix_time::seconds(7));
    t_broadcast.async_wait(strand.wrap(boost::bind(&ActionList::broadcast,this,boost::asio::placeholders::error)));
    
}

void ActionList::search_good_peer(boost::system::error_code e){
    if(e == boost::asio::error::operation_aborted ) return;
    
    boost::mutex::scoped_lock lock(mutex);
    
    //TODO: if synced, start finding dood peer
    
    t_clean_up.expires_from_now(boost::posix_time::seconds(7));
    t_clean_up.async_wait(strand.wrap(boost::bind(&ActionList::search_good_peer,this, boost::asio::placeholders::error)));
    
}

void ActionList::good_peer_first(){
    //create our BB bundle
    boost::mutex::scoped_lock lock(mutex);
    //TODO: Create new C strcutre to adapt BackBundle
}


//////////////////////
void ActionList::add_new_action(){
    count++;
    ac_list[count].ts = AOc_localTimestamp();
}

void ActionList::sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender){
    //set network periord
    //Retrieve content from syncpoint and broadcast to all peers
}

void ActionList::notificationOfSyncAchieved(double networkPeriod, void *sender){
    //similar
}

void ActionList::periodicSync_(const boost::system::error_code &error){
    if ( error == boost::asio::error::operation_aborted) return ;
    
    //Ask for global picture
    synchronizer->processSyncState(&ac_list[0].ts, NULL, 1, count);
    
    //sync_timer.expires_from_now(boost::posix_time::milliseconds(syncPeriod*1000));
    //sync_timer.async_wait(strand_.wrap(boost::bind(&ARONetworkFeed::periodicSync_, boost::asio::placeholders::error)));

}

void ActionList::processSyncPoint_(SyncPoint msgSyncPoint){
    //Ask for between certain range
    if ((msgSyncPoint.id1 == 0) && (msgSyncPoint.id2 == 0)) {
        //Not user the global picture
        if ((msgSyncPoint.ts1 == 0ll) && (msgSyncPoint.ts2 == 0ll)) { //0,0,0,0 global picture
            //                uint64_t * token = new uint64_t[2];
            //                token[0] = ts_list[0]; token[1] = ts_list[count-1];
            //TODO: Create matching token
            //TODO:
            //Broadcast the token to peers
            //Each peer should clean up after they receive that?
        } else if (count > 0) { //current region not empty
            //AROLog::Log()->Print(logINFO,1,"NF","Responding to request for %lld-%lld (%d-%d)\n",msgSyncPoint.ts1,msgSyncPoint.ts2,lo,hi);
            int lo = 0, hi = count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,ac_list[lo].ts, ac_list[hi].ts, ac_list[pivot].ts);
            
            //TODO: check range, make sure lo and high is valid
            //                uint64_t * token = new uint64_t [hi - lo + 1];
            //                std::copy(ts_list+lo, ts_list+hi, token);
            //TODO: create matching token structure
            //TODO:
            //BroadCast Token to peers or one of them
        }
        
    }else {
        //id1, id2, ts1, ts2
        synchronizer->notifyOfSyncPoint(&msgSyncPoint); //add sync point in buffer
    }
}


