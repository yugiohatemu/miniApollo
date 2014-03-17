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
    //also a pointer to the header in the future
} __attribute__((aligned(8)))Action_C ;

typedef struct {
    unsigned int action_count;
    Action_C * action_list;
}BackBundle_C;
    
typedef struct{
    uint64_t from;
    uint64_t to;
    unsigned int size;
}Raw_Header_C;
    

    
typedef struct {
    Raw_Header_C * raw_header;
    BackBundle_C * bb; //we can just call this actionlist like ActionList_C * bb
    bool synced;
}Header_C;
    
typedef struct{
    unsigned int action_count;
    Action_C * action_list;
    unsigned int header_count;
    Header_C * header_list;
}ActionList_C;
    
ActionList_C* init_default_actionList();
void free_actionList(ActionList_C* ac_list);
void merge_new_action(ActionList_C * ac_list, uint64_t ts);
void add_header_to_actionList(ActionList_C * ac_list,  Header_C * header);
//void merge_new_header(ActionList_C * ac_list, Raw_Header_C * raw_header);
void add_raw_header_to_actionList(ActionList_C * ac_list,  Raw_Header_C * raw_header);
bool is_raw_header_in_actionList(ActionList_C * ac_list,  Raw_Header_C * raw_header);
bool is_header_section_synced(ActionList_C * ac_list);
//BackBundle_C * init_backBundle_with_capacity(unsigned int capacity);
void free_backBundle(BackBundle_C * bb);
BackBundle_C * get_newest_bb(ActionList_C * ac_list);
//
Header_C * init_header_with_actionList(Action_C * action, unsigned int action_count);
void sync_self_with_unsyced_header(ActionList_C * ac_list);
void update_sync_state(Header_C * header);
    
void merge_existing_action(BackBundle_C * bb, uint64_t ts);

void remove_duplicate(ActionList_C * ac_list);
    
#ifdef __cplusplus
}
#endif
#endif /* defined(__Decision__action_C__) */
