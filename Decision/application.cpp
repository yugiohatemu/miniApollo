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
const int T_GOOD_PEER_TIMEOUT = 7;


Application::Application(boost::asio::io_service &io_service, boost::asio::io_service::strand &strand,unsigned int pid, std::vector<Peer *> &peer_list,PriorityPeer * priority_peer):
    io_service(io_service),
    strand(strand),
    peer_list(peer_list),
    pid(pid),
    priority_peer(priority_peer),
    t_sync(io_service, boost::posix_time::seconds(0)),
    t_good_peer(io_service, boost::posix_time::seconds(0))
{
    
    ac_list = init_default_actionList();
    
    std::stringstream ss; ss<<"AC# "<<pid; tag = ss.str(); ss.clear();
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
    t_good_peer.cancel();
    t_sync.cancel();
    
    BB_started = false;
}

void Application::resume(){
    
    synchronizer->setNetworkPeriod(0.5);
    
    BB_started = false;
    
    t_sync.expires_from_now(boost::posix_time::seconds(1));
    t_sync.async_wait(strand.wrap(boost::bind(&Application::periodicSync,this, boost::asio::placeholders::error)));
    
    t_good_peer.expires_from_now(boost::posix_time::seconds(T_GOOD_PEER_TIMEOUT));
    t_good_peer.async_wait(strand.wrap(boost::bind(&Application::search_good_peer,this, boost::asio::placeholders::error)));
    
}

void Application::processAppDirective(Packet p){
    switch(p.flag){
        case PROCESS_SYNC_POINT:
            strand.dispatch(boost::bind(&Application::processSyncPoint,this,p.content.sync_point));
            break;
        case MERGE_ACTION:
            strand.dispatch(boost::bind(&Application::mergeAction,this,p.content.ts));
            break;
        case NEW_HEADER:
            //TODO: add matching things to that
            //But fi
            break;
        default:
            break;
    }
}


void Application::mergeAction(uint64_t ts){ //the content of action
    //Find the insertion
    boost::mutex::scoped_lock lock(mutex);
    merge_new_action(ac_list, ts);
}


void Application::periodicSync(const boost::system::error_code &error){
    if ( error == boost::asio::error::operation_aborted) return ;
    
    AROLog::Log().Print(logDEBUG, 1, tag.c_str(), "Periodic sync\n");
    synchronizer->processSyncState(&ac_list->action_list[0].ts, &ac_list->action_list[0].hash, sizeof(Action_C) / sizeof(uint64_t), ac_list->action_count);
    
    t_sync.expires_from_now(boost::posix_time::seconds(1));
    t_sync.async_wait(strand.wrap(boost::bind(&Application::periodicSync,this, boost::asio::placeholders::error)));
}


//////////////////////
void Application::add_new_action(){
    if(ac_list->action_count <=3){
        uint64_t ts = AOc_localTimestamp();
        AROLog::Log().Print(logINFO, 1, tag.c_str(),"New action %lld created\n", ts);
        strand.dispatch(boost::bind(&Application::mergeAction, this, ts));
    }
}

void Application::sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender){
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
//    AROLog::Log().Print(logINFO,1,"SYNC","Peer # %d sendRequestForSyncPoint from  %d %d %lld %lld\n",pid, syncPoint->id1, syncPoint->id2, syncPoint->ts1, syncPoint->ts2);
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i!=pid) {
            Packet packet; packet.flag = PROCESS_SYNC_POINT; packet.content.sync_point = *syncPoint;
            peer_list[i]->enqueue(boost::protect(boost::bind(&Application::processAppDirective,peer_list[i]->application,packet)));
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
            
            if (ac_list->action_count > 0) {
                SyncPoint global_pic;
                global_pic.id1 = 1; global_pic.id2 = ac_list->action_count;
                global_pic.ts1 = ac_list->action_list[0].ts;global_pic.ts2 = ac_list->action_list[ac_list->action_count-1].ts;
                global_pic.hash = 0; global_pic.res = ac_list->action_count;
                
                AROLog::Log().Print(logINFO,1,tag.c_str(),"Responding to global pic with %d-%d %lld-%lld\n",global_pic.id1,global_pic.id2,global_pic.ts1,global_pic.ts2);
                for (unsigned int i = 0; i < peer_list.size(); i++) {
                    if (i!=pid) {
                        Packet packet; packet.flag = PROCESS_SYNC_POINT; packet.content.sync_point = global_pic;
                        peer_list[i]->enqueue(boost::protect(boost::bind(&Application::processAppDirective,peer_list[i]->application,packet)));
                    }
                }
            }
            
        } else if (ac_list->action_count > 0) { //current region not empty
            
            int lo = 0, hi = ac_list->action_count - 1;
            binaryReduceRangeWithRange(lo, hi, msgSyncPoint.ts1, msgSyncPoint.ts2,ac_list->action_list[lo].ts, ac_list->action_list[hi].ts, ac_list->action_list[pivot].ts);
            for (; lo <= hi; lo++) {
                uint64_t ts = ac_list->action_list[lo].ts;
                
                for (unsigned int i = 0; i < peer_list.size(); i++) {
                    if (i!=pid) {
                        Packet packet; packet.flag = MERGE_ACTION; packet.content.ts = ts;
                        peer_list[i]->enqueue(boost::protect(boost::bind(&Application::processAppDirective, peer_list[i]->application,packet)));
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

#warning Double Check

void Application::search_good_peer(boost::system::error_code e){
    if(e == boost::asio::error::operation_aborted ) return;
    
    //TODO: do not create another BB until we have synced up the last BB, ie. ask for other peers that we are synced in header
    if (ac_list->action_count > BB_SIZE || !BB_started ) {
        BB_started = true;
        if (ac_list->header_count == 0) {
            peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::start_bully, peer_list[pid], error)));
        }else if (ac_list->header_list[ac_list->header_count-1].synced) { //TODO: is it
            peer_list[pid]->enqueue(boost::protect(boost::bind(&Peer::start_bully, peer_list[pid], error)));
        }
        t_good_peer.cancel();
        return ;
    }else{
        t_good_peer.expires_from_now(boost::posix_time::seconds(T_GOOD_PEER_TIMEOUT));
        t_good_peer.async_wait(strand.wrap(boost::bind(&Application::search_good_peer,this, boost::asio::placeholders::error)));
    }
}

void Application::good_peer_first(){
    Header_C * header = init_header_with_actionList(ac_list->action_list, BB_SIZE);
    add_header_to_actionList(ac_list, header);
    //TODO: create message flage
    for (unsigned int i = 0; i < peer_list.size(); i++) {
        if (i!=pid) {
            Packet packet; packet.flag = NEW_HEADER; packet.content.raw_header = *(header->raw_header);
            peer_list[i]->enqueue(boost::protect(boost::bind(&Application::processAppDirective, peer_list[i]->application,packet)));
        }
    }
    
}

