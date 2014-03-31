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
#include "AROLogInterface.h"
#include "commonDefines.h"
#include <math.h>

bool is_equal(Raw_Header_C * rh1, Raw_Header_C * rh2);
BackBundle_C * init_BB_with_capacity(unsigned int count);

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

void load_action_from_cache(ActionList_C* ac_list,uint64_t * ts, unsigned int n){
    AROLog(logINFO, 0, "", "%d %lld-%lld\n",n,ts[0],ts[n-1]);
    for (unsigned int  i = 0; i < n; i++) {
        ac_list->action_list[i].ts = ts[i];
    }
    ac_list->action_count = n;
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
            if (ac_list->header_list[i].raw_header) free_raw_header(ac_list->header_list[i].raw_header);
            if (ac_list->header_list[i].bb) free_BB(ac_list->header_list[i].bb);
        }
        free(ac_list->header_list);
    }
    
    free(ac_list);
    
}

void free_raw_header(Raw_Header_C * raw_header){
    if (!raw_header) return;
    if (raw_header->outliers) free(raw_header->outliers);
    free(raw_header);
}

void merge_new_action(ActionList_C * ac_list, uint64_t ts){
    if (!ac_list ) {
        AROLog(logERROR, 1, "", "Merge Action ERROR\n");
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
        AROLog(logDEBUG, 1, "", "Merge Action %lld\n", ts);
    }
}



//Cheat it now
void merge_new_header(ActionList_C * ac_list, Raw_Header_C *raw_header){
    //is less or is greater
    if (!ac_list || !raw_header) {
        AROLog(logERROR, 1, "", "Merge Header ERROR\n");
        return ;
    }
    
    int lo = 0; int hi =  ac_list->header_count - 1;
    uint64_t ts = raw_header->confidence_low;
    
    binaryReduceRangeWithKey(lo,hi,ts,ac_list->header_list[lo].ts,ac_list->header_list[hi].ts,ac_list->header_list[pivot].ts);
    if (lo > hi) {
        
        if (lo < ac_list->header_count) {
            for (int i = ac_list->header_count; i > lo; i--) {
                ac_list->header_list[i] = ac_list->header_list[i-1];
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
        ac_list->header_list[lo].bb = init_BB_with_capacity(BB_SIZE); //should
        ac_list->header_list[lo].synced = false;
        
        ac_list->header_count++;
        
//        AROLog(logINFO, 1, "", "Header get merged with capacity %d\n",raw_header->confidence_count + raw_header->outsider_count);
//        print_raw_header(raw_header);
    }
    
}

void merge_new_header_with_BB(ActionList_C * ac_list, Raw_Header_C *raw_header, BackBundle_C * bb){
    if (!ac_list || !raw_header || !bb) {
        AROLog(logERROR, 1, "", "Merge BB with Header ERROR\n");
        return ;
    }
    int lo = 0; int hi =  ac_list->header_count - 1;
  
    uint64_t ts = raw_header->confidence_low;
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
    if (ac_list->header_list[ac_list->header_count-1].synced == true) {
        return true;
    }else
        return false;
   
}

void merge_action_into_header(Header_C * header, uint64_t ts){
    if (header->synced == true) return ;
    
    BackBundle_C * bb = header->bb;
    if (!bb){ AROLog(logERROR, 1, "", "Merge action into unexisted BB"); return;}
    
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


#warning still need to fix that
void update_sync_state(ActionList_C * ac_list){
    if (ac_list->header_count == 0) return ;

    Header_C *header = &(ac_list->header_list[ac_list->header_count-1]);
    if (!header->synced) {
        Raw_Header_C * current = init_raw_header_with_BB(header->bb);
//        print_raw_header(current);
        if (is_equal(current, header->raw_header))  header->synced = true;
        
    }
}


#pragma mark - BackBundle
int cmpfunc (const void * a, const void * b){
    Action_C * action_a = (Action_C *) a;
    Action_C * action_b = (Action_C *) b;
    
    if (action_a->ts >  action_b->ts) return  1;
    if (action_a->ts <  action_b->ts) return  -1;
    return  0;
}

BackBundle_C * init_BB_with_sampled_randomization(ActionList_C * ac_list){
    BackBundle_C * bb = (BackBundle_C *) malloc(sizeof(BackBundle_C));
    bb->action_count = BB_SIZE;
    bb->action_list = (Action_C *) malloc(sizeof(Action_C) * bb->action_count);

    //copy first
    for (unsigned int i = 0; i < bb->action_count; i++) {
        bb->action_list[i].ts = ac_list->action_list[i].ts;
        bb->action_list[i].hash = ac_list->action_list[i].hash;
    }
   
    //swaping some of them
    unsigned int random_count = bb->action_count * RADOMIZATION;
    unsigned int left_over = ac_list->action_count - bb->action_count;
    AROLog(logINFO, 1, "", "Sampled %d  - %d in range \n",random_count, left_over);

    for (unsigned int i = 0; i < random_count; i++) {
        unsigned int r1 = random() % bb->action_count;
        unsigned int r2 = (random() % left_over) + BB_SIZE;
//        AROLog(logINFO, 1, "", "%d %lld - %d %lld\n", r1, ac_list->action_list[r2].ts,r2,bb->action_list[r1].ts);
        bb->action_list[r1].ts = ac_list->action_list[r2].ts;
    }
    
    qsort(bb->action_list, bb->action_count, sizeof(Action_C), cmpfunc);
    for (unsigned int i=1; i < bb->action_count; i++) {
        if (bb->action_list[i-1].ts > bb->action_list[i].ts) AROLog(logERROR, 1, "", "Not sorted in ascending order\n");
    }
    return bb;
}

BackBundle_C * init_BB_with_capacity(unsigned int count){
    BackBundle_C * bb = (BackBundle_C *) malloc(sizeof(BackBundle_C));
    
    bb->action_count = 0;
    bb->action_list = (Action_C *) malloc(sizeof(Action_C) * count);
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

    ac_list->action_count = prev;
    AROLog(logINFO, 1, "", "Action %d vs BB %d\n", ac_list->action_count,bb->action_count);
}

#pragma mark - Raw Header
void print_raw_header(Raw_Header_C * raw_header){
    if (!raw_header) {
        AROLog(logERROR, 1, "", "Raw Header Empty\n");
        return;
    }
  
    AROLog(logINFO, 1, "", "Conf range %d with %d%%, %lld - %lld\n",raw_header->confidence_count,raw_header->percent,raw_header->confidence_low, raw_header->confidence_high);
    AROLog(logINFO, 1, "", "Outlier size %d\n",raw_header->outlier_count);
    
    for (unsigned int i = 0 ; i < raw_header->outlier_count; i++) {
        AROLog(logINFO, 1, "", "%d - %lld\n", i, raw_header->outliers[i]);
    }
}


uint64_t get_quartie(BackBundle_C * bb, float percent){
    uint16_t m = (bb->action_count+1) * percent;
    return (bb->action_list[m].ts + bb->action_list[m+1].ts)/2;
}

void set_fence(Raw_Header_C * raw_header,  BackBundle_C  * bb, float percent){
    
    uint64_t low_q = get_quartie(bb, percent);
    uint64_t high_q = get_quartie(bb, 1-percent);
    
    uint64_t inner_range = ceil((high_q - low_q) * 1.5);
    uint64_t inner_low = low_q - inner_range;
    uint64_t inner_high = high_q + inner_range;

    uint16_t inner_low_ind = 0;
    uint16_t inner_high_ind = bb->action_count-1;
    
    for (; bb->action_list[inner_low_ind].ts < inner_low && inner_low_ind < bb->action_count; inner_low_ind++) {}
    for (; bb->action_list[inner_high_ind].ts > inner_high && inner_high_ind >= inner_low_ind ; inner_high_ind--) {}

    raw_header->confidence_low = bb->action_list[inner_low_ind].ts;
    raw_header->confidence_high = bb->action_list[inner_high_ind].ts;
    raw_header->outlier_count = bb->action_count- (inner_high_ind - inner_low_ind) - 1;
    raw_header->confidence_count = bb->action_count - raw_header->outlier_count;
    
}

Raw_Header_C *init_raw_header_with_BB(BackBundle_C * bb){
    if (!bb) {
        AROLog(logERROR, 1, "", "Create Header with empty or not larger enough BB\n");
        return NULL;
    }

    Raw_Header_C * header = (Raw_Header_C *) malloc(sizeof(Raw_Header_C));
    //TODO: just a easy check for now
    float percentage = 0.35f;
    set_fence(header, bb, percentage);
    header->percent = (uint8_t)(percentage * 100);
    //Actually picking out outliers
    header->outliers = (uint64_t *) malloc(sizeof(uint64_t) * header->outlier_count);

    for (unsigned int i = 0, j = 0; i < bb->action_count; i++) {
        if (bb->action_list[i].ts < header->confidence_low || bb->action_list[i].ts > header->confidence_high ) {
            header->outliers[j] = bb->action_list[i].ts;
            j++;
        }
    }
    
    return header;
}

Raw_Header_C *copy_raw_header(Raw_Header_C * raw_header){
    if (!raw_header) return NULL;
    Raw_Header_C * copy = (Raw_Header_C *) malloc(sizeof(Raw_Header_C));
    copy->percent = raw_header->percent;
    copy->confidence_count = raw_header->confidence_count;
    copy->confidence_low = raw_header->confidence_low;
    copy->confidence_high = raw_header->confidence_high;
    copy->outlier_count = raw_header->outlier_count;
    
    copy->outliers = (uint64_t *) malloc(sizeof(uint64_t) * raw_header->outlier_count);
    for (unsigned int i = 0; i < copy->outlier_count; i++) copy->outliers[i] = raw_header->outliers[i];
    
    return copy;
}

bool is_equal(Raw_Header_C * rh1, Raw_Header_C * rh2){
    if (!rh1 || !rh2) return false;
    
    if (rh1->percent != rh2->percent||rh1->confidence_count != rh2->confidence_count|| rh1->outlier_count != rh2->outlier_count
        || rh1->confidence_low != rh2->confidence_low || rh1->confidence_high != rh2->confidence_high)return false;
    
    for (unsigned int i = 0; i < rh1->outlier_count; i++){
        if (rh1->outliers[i] != rh2->outliers[i]) return false;
    }
    
    return true;
}

bool is_action_likely_in_bb(Raw_Header_C * raw_header, uint64_t ts){
    for (unsigned int i = 0; i < raw_header->outlier_count; i++){
        if (ts == raw_header->outliers[i]) return true;
    }
    return false;
}

#pragma mark - Header

void sync_header_with_self(ActionList_C * ac_list){
    Header_C header = ac_list->header_list[ac_list->header_count-1];
    Raw_Header_C * raw_header = header.raw_header;
    
    for (unsigned int i = 0; i  < ac_list->action_count; i++) {
        if (is_action_likely_in_bb(raw_header, ac_list->action_list[i].ts)) {
            merge_action_into_header(&header,  ac_list->action_list[i].ts);
        }
    }
    remove_duplicate_actions(ac_list, get_latest_BB(ac_list));
    update_sync_state(ac_list);
}
Header_C * get_latest_header(ActionList_C * ac_list){
    if (ac_list->header_count == 0) return NULL;
    else return &ac_list->header_list[ac_list->header_count-1];
    
}
