//
//  action_C.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-11.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "action_C.h"
#include <stdlib.h>
#include "AROUtil_C.h"
#include "AROLog_C.h"

const int DEFAULT_HEADER_CAPACITY = 10;
const int DEFAULT_SYNCHORNIZER_CAPATICY = 50;

bool is_equal(Raw_Header_C * rh1, Raw_Header_C * rh2);
//bool is_less_than(Raw_Header_C * rh1, Raw_Header_C * rh2);
void free_BB(BackBundle_C * bb);
BackBundle_C * init_empty_BB();
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
        free(ac_list->action_list);
    }
    
    
    //Free header_list
    if (ac_list->header_list) {
        for (unsigned int i = 0; i < ac_list->header_count; i++) {
            if (ac_list->header_list[i].raw_header) free(ac_list->header_list[i].raw_header);
            if (ac_list->header_list[i].bb) free_BB(ac_list->header_list[i].bb);
        }
        free(ac_list->header_list);
    }
    
    free(ac_list);
    
}


void merge_new_action(ActionList_C * ac_list, uint64_t ts){
    if (!ac_list ) {
        AROLog_Print(logERROR, 1, "", "Merge Action ERROR\n");
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
        AROLog_Print(logDEBUG, 1, "", "Merge Action %lld\n", ts);
    }
}

BackBundle_C * init_BB_with_capacity(unsigned int count){
    BackBundle_C * bb = (BackBundle_C *) malloc(sizeof(BackBundle_C));
    
    bb->action_count = 0;
    bb->action_list = (Action_C *) malloc(sizeof(Action_C) * count);
    return bb;
}

//Cheat it now
void merge_new_header(ActionList_C * ac_list, Raw_Header_C *raw_header){
    //is less or is greater
    if (!ac_list || !raw_header) {
        AROLog_Print(logERROR, 1, "", "Merge Header ERROR\n");
        return ;
    }
    
    int lo = 0; int hi =  ac_list->header_count - 1;
    uint64_t ts = raw_header->outsider_[0];
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
        ac_list->header_list[lo].bb = init_BB_with_capacity(raw_header->confidence_count + raw_header->outsider_count);
        ac_list->header_list[lo].synced = false;
        
        ac_list->header_count++;
        
        AROLog_Print(logINFO, 1, "", "Header get merged\n");
    }
    
}

void merge_new_header_with_BB(ActionList_C * ac_list, Raw_Header_C *raw_header, BackBundle_C * bb){
    if (!ac_list || !raw_header || !bb) {
        AROLog_Print(logERROR, 1, "", "Merge BB with Header ERROR\n");
        return ;
    }
    int lo = 0; int hi =  ac_list->header_count - 1;
    uint64_t ts = raw_header->outsider_[0];
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
        ac_list->header_list[lo].synced = true;
        
        ac_list->header_count++;
    }
    
    remove_duplicate_actions(ac_list,bb);
}

bool is_header_section_synced(ActionList_C * ac_list){
    if (ac_list->header_count == 0) return true;
    return ac_list->header_list[ac_list->header_count-1].synced;
}

void merge_action_into_header(Header_C * header, uint64_t ts){
    BackBundle_C * bb = header->bb;
    if (!bb) AROLog_Print(logERROR, 1, "", "Merge action into unexisted BB");
    
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


bool is_action_in_active_region(ActionList_C * ac_list, uint64_t ts){
    int lo = 0; int hi =  ac_list->action_count - 1;
    binaryReduceRangeWithKey(lo,hi,ts,ac_list->action_list[lo].ts,ac_list->action_list[hi].ts,ac_list->action_list[pivot].ts);
    return (lo<=hi);
}

#pragma mark - BackBundle

BackBundle_C * init_empty_BB(){
    BackBundle_C * bb = (BackBundle_C *) malloc(sizeof(BackBundle_C));
    
    bb->action_count = 0;
    bb->action_list = (Action_C *) malloc(sizeof(Action_C) * 13);
    
    return bb;
}

BackBundle_C * init_BB_with_actions(Action_C * action_list, unsigned int count){
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

void free_BB(BackBundle_C * bb){
    if (!bb) return;
    if (bb->action_list) free(bb->action_list);
    free(bb);
}

BackBundle_C * get_latest_BB(ActionList_C * ac_list){
    if (ac_list->header_count == 0) return NULL;
    else return ac_list->header_list[ac_list->header_count-1].bb;
}

void remove_duplicate_actions(ActionList_C * ac_list,BackBundle_C * bb){
    for (unsigned int i = 0; i < bb->action_count; i++) {
        int lo = 0; int hi =  ac_list->action_count - 1;
        binaryReduceRangeWithKey(lo,hi,bb->action_list[i].ts,ac_list->action_list[lo].ts,ac_list->action_list[hi].ts,ac_list->action_list[pivot].ts);
        if (lo <= hi){ ac_list->action_list[lo].ts = 0;  ac_list->action_list[lo].hash = 0;}
    }
    
    //Shift actions to fill in space
    int current = 0; int prev = 0;
    while (current < ac_list->action_count) {
        if (ac_list->action_list[current].ts != 0) {
            
            ac_list->action_list[prev].ts = ac_list->action_list[current].ts;
            ac_list->action_list[prev].hash = ac_list->action_list[current].hash;
            
            prev++;
        }
        current++;
    }
    //update real count
    AROLog_Print(logDEBUG, 1, "", "Action count %d vs bb count %d\n", ac_list->action_count, bb->action_count);
    ac_list->action_count -= bb->action_count;
    AROLog_Print(logDEBUG, 1, "", "Action is reduced to %d\n", ac_list->action_count);
}

#pragma mark - Raw Header
Raw_Header_C *init_raw_header_with_BB(BackBundle_C * bb){
    if (!bb || bb->action_count <= 3) {
        AROLog_Print(logERROR, 1, "", "Create Header with empty BB\n");
        return NULL;
    }

#warning we manually take the front and back away and cast them as outsiders
    Raw_Header_C * header = (Raw_Header_C *) malloc(sizeof(Raw_Header_C));
    header->confidence_count = bb->action_count - 2;
    header->confidence_ = (uint64_t *) malloc(sizeof(uint64_t) * header->confidence_count);
    for (unsigned int i = 0; i < header->confidence_count; i++) header->confidence_[i] = bb->action_list[i+1].ts;
    
    header->outsider_count = 2;
    header->outsider_ = (uint64_t *) malloc(sizeof(uint64_t) * header->outsider_count);
    header->outsider_[0] = bb->action_list[0].ts;
    header->outsider_[1] = bb->action_list[bb->action_count - 1].ts;
    
    return header;
}

Raw_Header_C *copy_raw_header(Raw_Header_C * raw_header){
    if (!raw_header) return NULL;
    Raw_Header_C * copy = (Raw_Header_C *) malloc(sizeof(Raw_Header_C));
    copy->confidence_count = raw_header->confidence_count - 2;
    copy->confidence_ = (uint64_t *) malloc(sizeof(uint64_t) * copy->confidence_count);
    for (unsigned int i = 0; i < copy->confidence_count; i++) {
        copy->confidence_[i] = raw_header->confidence_[i];
    }
    
    copy->outsider_count = raw_header->confidence_count;
    copy->outsider_ = (uint64_t *) malloc(sizeof(uint64_t) * copy->outsider_count);
    for (unsigned int i = 0; i < copy->outsider_count; i++) {
        copy->outsider_[i] = raw_header->outsider_[i];
    }
    return copy;
}

bool is_equal(Raw_Header_C * rh1, Raw_Header_C * rh2){
    if (!rh1 || !rh2) return false;
    //    AROLog_Print(logINFO, 1, "", "%lld- %lld %d vs %lld-%lld %d\n", rh1->from,rh1->to, rh1->size,rh2->from,rh2->to, rh2->size);
    if (rh1->confidence_count != rh2->confidence_count || rh1->outsider_count != rh2->outsider_count) return false;
    for (unsigned int i = 0; i < rh1->confidence_count; i++) {
        if (rh1->confidence_[i]!= rh2->confidence_[i]) return false;
    }
    for (unsigned int i = 0; i < rh1->outsider_count; i++) {
        if (rh1->outsider_[i]!= rh2->outsider_[i]) return false;
    }
    return true;
}

bool is_action_likely_in_bb(Raw_Header_C * raw_header, uint64_t ts){
    for (unsigned int i =0 ; i < raw_header->confidence_count; i++) {
        if (ts == raw_header->confidence_[i]) return true;
    }
    
    return false;
}

#pragma mark - Header

void update_sync_state(Header_C * header){
    Raw_Header_C * current_raw_header = init_raw_header_with_BB(header->bb);
    header->synced =  is_equal(header->raw_header, current_raw_header);
    AROLog_Print(logDEBUG, 1, "", "--%c--\n", header->synced ? 'Y':'N');
    if (current_raw_header) free(current_raw_header);
}

void sync_header_with_self(ActionList_C * ac_list){
    Header_C header = ac_list->header_list[ac_list->header_count-1];
    Raw_Header_C * raw_header = header.raw_header;
    
    for (unsigned int i = 0; i  < ac_list->action_count; i++) {
        if (is_action_likely_in_bb(raw_header, ac_list->action_list[i].ts)) {
            merge_action_into_header(&header,  ac_list->action_list[i].ts);
        }
    }
    remove_duplicate_actions(ac_list, header.bb);
    update_sync_state(&header);
    
}


///////////////////////////
#pragma mark - Unused function?
bool is_raw_header_in_actionList(ActionList_C * ac_list,  Raw_Header_C * raw_header){
    for ( int i = ac_list->header_count - 1; i >= 0; i--) {
        //        if (is_equal(raw_header, ac_list->header_list[i].raw_header)) return true;
    }
    return false;
}



bool is_action_in_BB(BackBundle_C * bb, uint64_t ts){
    for (unsigned int i = 0; i < bb->action_count; i++) {
        if (bb->action_list[i].ts == ts){
            return true;
        }
    }
    return false;
}


