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

const unsigned int BB_SIZE = 13;
const unsigned int LIST_SIZE = 50;

Application::Application(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list,PriorityPeer * priority_peer):
    io_service(io_service),
    strand(strand),
    peer_list(peer_list),
    pid(pid),
    priority_peer(priority_peer),
    t_sync(io_service, boost::posix_time::seconds(0)),
    t_clean_up(io_service, boost::posix_time::seconds(0))
{
    
    ac_list = init_actionList_with_capacity(LIST_SIZE);
    
    std::stringstream ss; ss<<"AC# "<<pid; tag = ss.str();
    ss.clear();
    ss<<"SYNC# "<<pid; std::string sync_tag = ss.str();
    
    synchronizer = new AROObjectSynchronizer(NULL,NULL,  0 ,this); //
    synchronizer->setDebugInfo(0, sync_tag.c_str());
    synchronizer->setNetworkPeriod(0.5);
    
    bb_synchronizer = NULL;
}

Application::~Application(){
    //    AROLog::Log().Print(logINFO, 1, "SYNC", "Peer # %d summary\nBackBundle Region\n",pid);
    delete synchronizer;
    if (bb_synchronizer) delete bb_synchronizer;
    
    free_actionList(ac_list);
}

void Application::pause(){
    t_clean_up.cancel();
    t_sync.cancel();
    
    BB_started = false;
}

void Application::resume(){
    t_sync.expires_from_now(boost::posix_time::seconds(1));
    t_sync.async_wait(strand.wrap(boost::bind(&Application::periodicSync,this, boost::asio::placeholders::error)));
    
    synchronizer->setNetworkPeriod(0.5);
    BB_started = false;
}
//Change this to periosdic sync?


//    t_clean_up.expires_from_now(boost::posix_time::seconds(5));
//    t_clean_up.async_wait(strand.wrap(boost::bind(&Application::search_good_peer,this, boost::asio::placeholders::error)));

void Application::processAppDirective(SyncPoint p,  bool flag){
    boost::this_thread::sleep(boost::posix_time::seconds(1));

    if (flag) {
        strand.dispatch(boost::bind(&Application::processSyncPoint,this,p));
    }else{
        strand.dispatch(boost::bind(&Application::mergeAction,this,p));
    }
}

void Application::mergeAction(SyncPoint p){
    //Find the insertion
    boost::mutex::scoped_lock lock(mutex);
    merge_new_action(ac_list, p.ts1);
}


void Application::periodicSync(const boost::system::error_code &error){
    if ( error == boost::asio::error::operation_aborted) return ;
    
    AROLog::Log().Print(logDEBUG, 1, tag.c_str(), "Periodic sync\n");
    synchronizer->processSyncState(&ac_list->actionList[0].ts, &ac_list->actionList[0].hash, sizeof(Action_C) / sizeof(uint64_t), ac_list->count);
    
    t_sync.expires_from_now(boost::posix_time::seconds(1));
    t_sync.async_wait(strand.wrap(boost::bind(&Application::periodicSync,this, boost::asio::placeholders::error)));
}


//////////////////////
void Application::add_new_action(){
    
    if(ac_list->count <=3){
        SyncPoint s ; s.ts1 = AOc_localTimestamp(); s.hash = 0;
        AROLog::Log().Print(logINFO, 1, tag.c_str(),"New action %lld created\n", s.ts1);
        strand.dispatch(boost::bind(&Application::mergeAction, this, s));
    }
//    count++
}

//below are the only ways that synchronizer talks with sender
//Called by the synchronizer
void Application::sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender){
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
//    AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d sendRequestForSyncPoint from  %d %d %lld %lld\n",pid, syncPoint->id1, syncPoint->id2, syncPoint->ts1, syncPoint->ts2);
       for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i!=pid) {
            peer_list[i]->enqueue(boost::protect(boost::bind(&Application::processAppDirective,peer_list[i]->application,*syncPoint,true)));
        }
    }
}

void Application::notificationOfSyncAchieved(double networkPeriod, int code, void *sender){
    AROLog::Log().Print(logINFO,1,tag.c_str(),"Sync achieved at %llf\n", networkPeriod);
    
    if (networkPeriod < 60) networkPeriod *= 1.5;
    if (sender == synchronizer) synchronizer->setNetworkPeriod(networkPeriod);
}


void Application::processSyncPoint(SyncPoint msgSyncPoint){ //TODO: add a peer id here
    //Ask for between certain range
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    if ((msgSyncPoint.id1 == 0) && (msgSyncPoint.id2 == 0)) {
        
        if ((msgSyncPoint.ts1 == 0ll) && (msgSyncPoint.ts2 == 0ll)) { //0,0,0,0 global picture
            
            if (ac_list->count > 0) {
                SyncPoint global_pic;
                global_pic.id1 = 1; global_pic.id2 = ac_list->count;
                global_pic.ts1 = ac_list->actionList[0].ts;global_pic.ts2 = ac_list->actionList[ac_list->count-1].ts;
                global_pic.hash = 0; global_pic.res = ac_list->count;
                
                AROLog::Log().Print(logINFO,1,tag.c_str(),"Responding to global pic with %d-%d %lld-%lld\n",global_pic.id1,global_pic.id2,global_pic.ts1,global_pic.ts2);
                for (unsigned int i = 0; i < peer_list.size(); i++) {
                    if (i!=pid) {
                        peer_list[i]->enqueue(boost::protect(boost::bind(&Application::processAppDirective,peer_list[i]->application,global_pic,true)));
                    }
                }
            }
            
        } else if (ac_list->count > 0) { //current region not empty
            
            int lo = 0, hi = ac_list->count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,ac_list->actionList[lo].ts, ac_list->actionList[hi].ts, ac_list->actionList[pivot].ts);
            for (; lo <= hi; lo++) {
                SyncPoint one_point;
                one_point.id1 = lo; one_point.ts1 = ac_list->actionList[lo].ts;
                one_point.id2 = 0; one_point.ts2 = 0ll;
                
                for (unsigned int i = 0; i < peer_list.size(); i++) {
                    if (i!=pid) {
                        peer_list[i]->enqueue(boost::protect(boost::bind(&Application::processAppDirective, peer_list[i]->application,one_point,false)));
                    }
                }
//                AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d responding to request for %d %lld\n",pid, lo, one_point.ts1);
            }
        }
    }else {
        //id1, id2, ts1, ts2
        AROLog::Log().Print(logINFO,1,tag.c_str(),"Notify of sync point %d-%d %lld-%lld\n",msgSyncPoint.id1,msgSyncPoint.id2,msgSyncPoint.ts1,msgSyncPoint.ts2);
        synchronizer->notifyOfSyncPoint(&msgSyncPoint);
   }
}

void Application::search_good_peer(boost::system::error_code e){
    if(e == boost::asio::error::operation_aborted ) return;
    
    if (ac_list->count > BB_SIZE || !BB_started ) {
        
    }
    
    //TODO: if synced, start finding dood peer
    //TODO: also update current BB_Synchronizer state.
    
    t_clean_up.expires_from_now(boost::posix_time::seconds(7));
    t_clean_up.async_wait(strand.wrap(boost::bind(&Application::search_good_peer,this, boost::asio::placeholders::error)));
    
}

void Application::good_peer_first(){
//    boost::mutex::scoped_lock lock(mutex);
    //TODO: create header now
    Header_C * header = (Header_C*)malloc(sizeof(Header_C));
    //TODO: one we create that BackBundle, we start the synchronizer
    
    //1st, they need to talk with itself first, ie. it is similar as treat it its own synchronizer as another peer
    
    //2nd, talke with other bb_synchronizer...similar to ?
    
    //Once synced up,
}

