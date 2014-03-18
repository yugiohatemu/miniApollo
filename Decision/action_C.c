//
//  action_C.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-11.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "action_C.h"
#include <stdlib.h>
//#include <algorithm>
#include "AROUtil_C.h"
#include "AROLog_C.h"

const int DEFAULT_HEADER_CAPACITY = 10;
const int DEFAULT_SYNCHORNIZER_CAPATICY = 50;

bool is_equal(Raw_Header_C * rh1, Raw_Header_C * rh2);
bool is_less_than(Raw_Header_C * rh1, Raw_Header_C * rh2);
void free_backBundle(BackBundle_C * bb);

#pragma mark - Action List
ActionList_C* init_default_actionList(){
    ActionList_C * ac_list = (ActionList_C*) malloc(sizeof(ActionList_C));
    
    ac_list->action_count = 0;
    ac_list->action_list = (Action_C *) malloc(sizeof(Action_C) * DEFAULT_SYNCHORNIZER_CAPATICY);
  
    ac_list->header_count = 0;
    ac_list->header_list = (Header_C *) malloc(sizeof(Header_C) * DEFAULT_HEADER_CAPACITY);
    
    for (unsigned int i = 0; i <DEFAULT_HEADER_CAPACITY; i++) {
        ac_list->header_list[i].raw_header = NULL;
        ac_list->header_list[i].bb = NULL;
    }
    
    return ac_list;
}

void free_actionList(ActionList_C* ac_list){
    if (!ac_list) return;
    //Free action_list
    if (ac_list->action_list) {
        for (unsigned int i= 0; i < ac_list->action_count; i++) {
               AROLog_Print(logINFO, 1, "", "%d - %lld\n",i, ac_list->action_list[i].ts);
        }
        free(ac_list->action_list);
    }
    
    
    //Free header_list
    if (ac_list->header_list) {
        for (unsigned int i = 0; i < ac_list->header_count; i++) {
            if (ac_list->header_list[i].raw_header) free(ac_list->header_list[i].raw_header);
            if (ac_list->header_list[i].bb) free_backBundle(ac_list->header_list[i].bb);
        }
        free(ac_list->header_list);
    }
    
    free(ac_list);
    
}


void merge_new_action(ActionList_C * ac_list, uint64_t ts){
    if (!ac_list ) {
        AROLog_Print(logERROR, 1, "", "Merge Action ERROR");
        return ;
    }
    
    int lo = 0; int hi =  ac_list->action_count - 1;
    binaryReduceRangeWithKey(lo,hi,ts,ac_list->action_list[lo].ts,ac_list->action_list[hi].ts,ac_list->action_list[pivot].ts);
    
    if (lo > hi) {
        
        if (lo < ac_list->action_count) {
            for (int i = ac_list->action_count; i > lo; i--) {

                ac_list->action_list[i].ts = ac_list->action_list[i-1].ts;
                ac_list->action_list[i].hash = ac_list->action_list[i-1].hash;
            }
        }
        ac_list->action_list[lo].ts = ts;
        ac_list->action_list[lo].hash = 0;
        
        ac_list->action_count++;
    }
}

//Cheat it now
void merge_new_header(ActionList_C * ac_list, Raw_Header_C *raw_header){
    //is less or is greater
    if (!ac_list || !raw_header) {
        AROLog_Print(logERROR, 1, "", "Merge Header ERROR");
        return ;
    }
    
    int lo = 0; int hi =  ac_list->header_count - 1;
    uint64_t ts = raw_header->from;
    binaryReduceRangeWithKey(lo,hi,ts,ac_list->header_list[lo].ts,ac_list->header_list[hi].ts,ac_list->header_list[pivot].ts);
    
    if (lo > hi) {
        
        if (lo < ac_list->header_count) {
            for (int i = ac_list->header_count; i > lo; i--) {
                ac_list->header_list[i] = ac_list->header_list[i-1];
                //??does this work?
                ac_list->header_list[i].ts = ac_list->header_list[i-1].ts;
                ac_list->header_list[i].hash = ac_list->header_list[i-1].hash;
                ac_list->header_list[i].raw_header = ac_list->header_list[i-1].raw_header;
                ac_list->header_list[i].bb = ac_list->header_list[i-1].bb;
                ac_list->header_list[i].synced = ac_list->header_list[i-1].synced;
                
            }
        }
        ac_list->header_list[lo].ts = ts;
        ac_list->header_list[lo].hash = 0;
        ac_list->header_list[lo].raw_header = raw_header;
        ac_list->header_list[lo].bb = NULL;
        ac_list->header_list[lo].synced = false;
        
        ac_list->header_count++;
    }
    
}

//Or merge bb into header....
void merge_new_header_with_bb(ActionList_C * ac_list, Raw_Header_C *raw_header, BackBundle_C * bb){
    if (!ac_list || !raw_header || !bb) {
        AROLog_Print(logERROR, 1, "", "Merge BB with Header ERROR");
        return ;
    }
    int lo = 0; int hi =  ac_list->header_count - 1;
    uint64_t ts = raw_header->from;
    binaryReduceRangeWithKey(lo,hi,ts,ac_list->header_list[lo].ts,ac_list->header_list[hi].ts,ac_list->header_list[pivot].ts);
    
    if (lo > hi) {
        
        if (lo < ac_list->header_count) {
            for (int i = ac_list->header_count; i > lo; i--) {
                ac_list->header_list[i] = ac_list->header_list[i-1];
                //??does this work?
                ac_list->header_list[i].ts = ac_list->header_list[i-1].ts;
                ac_list->header_list[i].hash = ac_list->header_list[i-1].hash;
                ac_list->header_list[i].raw_header = ac_list->header_list[i-1].raw_header;
                ac_list->header_list[i].bb = ac_list->header_list[i-1].bb;
                ac_list->header_list[i].synced = ac_list->header_list[i-1].synced;
                
            }
        }
        ac_list->header_list[lo].ts = ts;
        ac_list->header_list[lo].hash = 0;
        ac_list->header_list[lo].raw_header = raw_header;
        ac_list->header_list[lo].bb = bb;
        ac_list->header_list[lo].synced = false;
        
        ac_list->header_count++;
    }
}

bool is_header_section_synced(ActionList_C * ac_list){
    if (ac_list->header_count == 0) return true;
    return ac_list->header_list[ac_list->header_count-1].synced;
}

#pragma mark - BackBundle

BackBundle_C * init_backBundle_with_actions(Action_C * action_list, unsigned int count){
    BackBundle_C * bb = (BackBundle_C *) malloc(sizeof(BackBundle_C));
    
    bb->action_count = count;
    bb->action_list = (Action_C *) malloc(sizeof(Action_C) * count);
    
    //Copy content over
    for (unsigned int i = 0; i < count; i++) {
        bb->action_list[i].ts = action_list[i].ts;
        bb->action_list[i].hash = action_list[i].hash;
    }
    
    return bb;
}

void free_backBundle(BackBundle_C * bb){
    if (!bb) return;
    if (bb->action_list) free(bb->action_list);
    free(bb);
}

#pragma mark - Raw Header
Raw_Header_C *init_raw_header_with_bb(BackBundle_C * bb){
    if (bb || bb->action_count == 0) {
         AROLog_Print(logERROR, 1, "", "Create Header with empty BB");
        return NULL;
    }
    
    Raw_Header_C * header = (Raw_Header_C *) malloc(sizeof(Raw_Header_C));
    header->from = bb->action_list[0].ts;
    header->to = bb->action_list[bb->action_count - 1].ts;
    header->size = bb->action_count;
    return header;
}

///////////////////////////
#pragma mark - Unused function?
bool is_raw_header_in_actionList(ActionList_C * ac_list,  Raw_Header_C * raw_header){
    for ( int i = ac_list->header_count - 1; i >= 0; i--) {
        //        if (is_equal(raw_header, ac_list->header_list[i].raw_header)) return true;
    }
    return false;
}

bool is_equal(Raw_Header_C * rh1, Raw_Header_C * rh2){
    if (!rh1 || !rh2) return false;
    
    return rh1->from == rh2->from && rh1->to == rh2->to && rh1->size == rh2->size;
}

bool is_less_than(Raw_Header_C * rh1, Raw_Header_C * rh2){
    if (!rh1||!rh2) {
        return false;
    }
    return rh1->from < rh2->from;
}

BackBundle_C * get_newest_bb(ActionList_C * ac_list){
    if (ac_list->header_count == 0) return NULL;
    Header_C  header = ac_list->header_list[ac_list->header_count - 1];
    if (header.bb) return header.bb;
    
    return NULL;
}

bool is_action_in_BB(BackBundle_C * bb, uint64_t ts){
    for (unsigned int i = 0; i < bb->action_count; i++) {
        if (bb->action_list[i].ts == ts){
            return true;
        }
    }
    return false;
}



#pragma mark - Header

//Init header with cache map in future
Header_C * init_header_with_actionList(Action_C * action, unsigned int action_count){
    //We will create a sample header and
    BackBundle_C  * bb = init_backBundle_with_actions(action, action_count);
    if (bb) {
        Raw_Header_C * raw_header = init_raw_header_with_bb(bb);
        if (raw_header) {
            Header_C * header = (Header_C *) malloc(sizeof(Header_C));
            header->bb = bb;
//            header->raw_header = raw_header;
            header->synced = true;
            return header;
        }else{
            free_backBundle(bb);
        }
    }
    return NULL;
}

//TODO: double check here, not necessary...hmm
Header_C* init_header_with_raw_header(Raw_Header_C * raw_header){
    if (!raw_header) return NULL;
 
    Header_C * header = (Header_C *) malloc(sizeof(Header_C));;
    header->bb = NULL;
//    header->raw_header = raw_header;
    header->synced = false;
    return header;
}

//TODO: use full list to check?
bool is_action_in_header(Header_C * header, Action_C * action){
//    Raw_Header_C  * h = header->raw_header;
//    if (h->from<= action->ts && action->ts <= h->to) { //In future, this depends on header itself
//        return true;
//    }
    return false;
}

void update_sync_state(Header_C * header){
    if (!header->bb) {
        header->synced = false;
        return;
    }
    Raw_Header_C * current_raw_header = init_raw_header_with_bb(header->bb);
//    header->synced =  is_equal(header->raw_header, current_raw_header);
//    free(current_raw_header);
}

void sync_self_with_unsyced_header(ActionList_C * ac_list){
    Header_C header = ac_list->header_list[ac_list->header_count-1];
   
    if (header.synced) return;
    
    for (unsigned int i = 0; i  < ac_list->action_count; i++) {
        if (!is_action_in_header(&header,&ac_list->action_list[i])) {
            merge_new_action(ac_list, ac_list->action_list[i].ts);
        }
    }
    update_sync_state(&header);
    
}

void merge_existing_action(BackBundle_C * bb, uint64_t ts){
    int lo = 0; int hi =  bb->action_count - 1;
    binaryReduceRangeWithKey(lo,hi,ts,bb->action_list[lo].ts,bb->action_list[hi].ts,bb->action_list[pivot].ts);
    
    if (lo > hi) {
        
        if (lo < bb->action_count) {
            for (int i = bb->action_count; i > lo; i--) {
                bb->action_list[i].ts = bb->action_list[i-1].ts;
                bb->action_list[i].hash = bb->action_list[i-1].hash;
            }
        }
        bb->action_list[lo].ts = ts;
        bb->action_list[lo].hash = 0;
        
        bb->action_count++;
    }
}
//Or some suspicicous POINT because 0 is from the root action?
void remove_duplicate(ActionList_C * ac_list){
    BackBundle_C * bb = get_newest_bb(ac_list);
    
    for (unsigned int i = 0; i < ac_list->action_count; i++) {
        if (is_action_in_BB(bb, ac_list->action_list[i].ts)) {
            ac_list->action_list[i].ts = 0;
        }
    }
    
    //shift up the empty and reset action count
    int current = 0;
    int prev = 0;
    while (current < ac_list->action_count) {
        if (ac_list->action_list[current].ts != 0) {
            ac_list->action_list[prev].ts = ac_list->action_list[current].ts;
            prev++;
        }
        current++;
    }
    //update real count
    ac_list->action_count -= bb->action_count;
}
