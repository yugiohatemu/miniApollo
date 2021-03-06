//
//  action_C.h
//  Decision
//
//  Created by Yue Huang on 2014-03-11.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__action_C__
#define __Decision__action_C__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdbool.h>

typedef struct {
    uint64_t ts;
    uint32_t hash;
    uint64_t ref_ts;
    uint64_t next_ts;
    //also a pointer to the header in the future
} __attribute__((aligned(8)))Action_C ;

typedef struct {
    uint16_t action_count;
    Action_C * action_list;
}BackBundle_C;

    
typedef struct{
    //modified into
    uint64_t confidence_low;
    uint64_t confidence_high;
    uint64_t * outliers;
    uint16_t confidence_count;
    uint16_t outlier_count;
    uint8_t percent; //actually char is enough
    //inliers is more useful when trying to calculate hit rate
}Raw_Header_C;
    
typedef struct {
    uint64_t ts; //use this as a way to make header, just pick a #
    uint32_t hash;//so we also need that to be aligned so that we can let the sync state process
    Raw_Header_C *raw_header;
    BackBundle_C * bb; //we can just call this actionlist like ActionList_C * bb
    bool synced;
}__attribute__((aligned(8)))Header_C;
    
typedef struct{
    uint16_t action_count;
    Action_C * action_list;
    uint16_t header_count;
    Header_C * header_list;
    uint16_t * action_ref_list;
}ActionList_C;
    
//ActionList
ActionList_C* init_default_actionList();
void load_action_from_cache(ActionList_C* ac_list,uint64_t * ts, unsigned int n);
void load_action_from_cache_with_chain(ActionList_C* ac_list,uint64_t * ts_list, unsigned int n);
void free_actionList(ActionList_C* ac_list);
void merge_new_action(ActionList_C * ac_list, uint64_t ts, uint64_t ref_ts);
void merge_new_header(ActionList_C * ac_list, Raw_Header_C *raw_header);
void merge_new_header_with_BB(ActionList_C * ac_list, Raw_Header_C *raw_header, BackBundle_C * bb);
bool is_header_section_synced(ActionList_C * ac_list);
void remove_duplicate_actions(ActionList_C * ac_list,BackBundle_C * bb);
void sync_header_with_self(ActionList_C * ac_list);
void update_sync_state(ActionList_C * ac_list );
    
//BB
bool is_BB_threshold_met(ActionList_C * ac_list);
BackBundle_C * init_BB_with_threashold(ActionList_C * ac_list);
//BackBundle_C * init_BB_with_sampled_randomization(ActionList_C * ac_list);
BackBundle_C * get_latest_BB(ActionList_C * ac_list);
void free_BB(BackBundle_C * bb);
//Raw_Header
Raw_Header_C *init_raw_header_with_BB(BackBundle_C * bb);
Raw_Header_C *copy_raw_header(Raw_Header_C * raw_header);
void free_raw_header(Raw_Header_C * raw_header);
void print_raw_header(Raw_Header_C * raw_header);
//
Header_C * get_latest_header(ActionList_C * ac_list);
void merge_action_into_header(Header_C * header, uint64_t ts);
    
//Actually could be replcaed by AciontList_C
    
#ifdef __cplusplus
}
#endif
#endif /* defined(__Decision__action_C__) */
