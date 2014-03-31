//
//  diff.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-26.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "diff.h"
#include "AROLogInterface.h"
#include <algorithm>
Diff& Diff::get(){
    static Diff m_pInstance;
    return m_pInstance;
}

void Diff::set_synced_bb(BackBundle_C * bb){
    AROLog(logINFO, 1, "Diff", "GLOBAL HASH Set HERE\n");
    for (unsigned int i = 0 ; i < bb->action_count; i++) {
        synced_bb.push_back(bb->action_list[i].ts);
    }
}
void Diff::set_unsynced_actions(BackBundle_C * bb){
    for (unsigned int i = 0 ; i < bb->action_count; i++) {
        unsynced_bb.push_back(bb->action_list[i].ts);
    }
}
void Diff::out_put_diff(){
    if (unsynced_bb.empty()) {
        AROLog(logINFO, 1, "Diff", "SYNCED\n");
        return;
    }
    
    std::sort(unsynced_bb.begin(), unsynced_bb.end());
    
    AROLog(logINFO, 1, "Diff", "%d vs %d\n", unsynced_bb.size(), synced_bb.size());
    for(unsigned int i = 0; i < synced_bb.size();i++){
        if (!std::binary_search(unsynced_bb.begin(), unsynced_bb.end(), synced_bb[i])) {
            AROLog(logINFO, 1, "Diff", "Not loaded %d - %lld\n",i, synced_bb[i]);
        }
    }
    for(unsigned int i = 0; i < unsynced_bb.size();i++){
        if (!std::binary_search(synced_bb.begin(), synced_bb.end(), unsynced_bb[i])) {
            AROLog(logERROR, 1, "Diff", "ERROR loaded %d - %lld\n",i, unsynced_bb[i]);
        }
    }
}

bool Diff::check_hash(int bb_lo, int bb_hi, int a_lo, Action_C * action_list){
//    for (int i = bb_lo,j = a_lo; i <= bb_hi;i++,j++){
//        AROLog(logERROR,1, "DIFF", "%d - %lld\n", j, action_list[j].ts);
//    }
    
    for (int i = bb_lo, j = a_lo; i <= bb_hi; i++,j++) {
        if (synced_bb[i] != action_list[j].ts) {
//            AROLog(logINFO, 1, "DIFF", "BB %d - %lld vs Active %d - %lld not match\n",i, synced_bb[i],j, action_list[j].ts);
            return false;
        }
    }
    return true;
}
