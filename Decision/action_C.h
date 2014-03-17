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
    
typedef struct {
    unsigned int size;
    uint64_t from;
    uint64_t to;
}Header_C;
    
typedef struct {
    uint64_t ts;
    uint32_t hash;
    //also a pointer to the header in the future
} __attribute__((aligned(8)))Action_C ;

typedef struct{
    unsigned int count;
    Action_C * actionList;
}ActionList_C;
//TODO: add a new free function
    
ActionList_C* init_actionList_with_capacity(unsigned int capacity);
void free_actionList(ActionList_C* ac_list);
void merge_new_action(ActionList_C * ac_list, uint64_t ts);
    
#ifdef __cplusplus
}
#endif
#endif /* defined(__Decision__action_C__) */
