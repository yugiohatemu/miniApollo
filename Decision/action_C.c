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

ActionList_C* init_actionList_with_capacity(unsigned int capacity){
    ActionList_C * ac_list = (ActionList_C*) malloc(sizeof(ActionList_C));
    ac_list->actionList = (Action_C *) malloc(sizeof(Action_C) * capacity);
    ac_list->count = 0;
    return ac_list;
}

void free_actionList(ActionList_C* ac_list){
    if (!ac_list) return;
    if (ac_list->actionList) {
        for (unsigned int i= 0; i < ac_list->count; i++) {
               AROLog_Print(logINFO, 1, "", "%d - %lld\n",i, ac_list->actionList[i].ts);
        }
        free(ac_list->actionList);
    }
    free(ac_list);
}

void merge_new_action(ActionList_C * ac_list, uint64_t ts){
    int lo = 0; int hi =  ac_list->count - 1;
    binaryReduceRangeWithKey(lo,hi,ts,ac_list->actionList[lo].ts,ac_list->actionList[hi].ts,ac_list->actionList[pivot].ts);
    
    if (lo > hi) {
        
        if (lo < ac_list->count) {
            for (int i = ac_list->count; i > lo; i--) {
                ac_list->actionList[i].ts = ac_list->actionList[i-1].ts;
            }
        }
        ac_list->actionList[lo].ts = ts;
        ac_list->actionList[lo].hash = 0;
        
        ac_list->count++;
    }
}




