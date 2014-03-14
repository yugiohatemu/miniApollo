//
//  dataPool.cpp
//  Decision
//
//  Created by Yue Huang on 2014-02-28.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "priorityPeer.h"
#include <stdlib.h>
#include <iostream>
#include "AROLog.h"

PriorityPeer::PriorityPeer(){
    ts_list.reserve(50);
}

PriorityPeer::~PriorityPeer(){
//    AROLog::Log().Print(logINFO, 1, "PP", "Total syncs item %d\n", ts_list.size());
//    for (unsigned int i = 0; i < sh_list.size(); i++) {
//        delete sh_list[i];
//    }
}
void PriorityPeer::add_ts(uint64_t ts){
    ts_list.push_back(ts);
}

//ask for BB copy
//void PriorityPeer::add_sh(SyncHeader * sh){
//    sh_list.push_back(new SyncHeader(*sh));
//}

