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
#include "log.h"

PriorityPeer::PriorityPeer(){
    ts_list.reserve(50);
}

PriorityPeer::~PriorityPeer(){
    Log::log().Print("Total syncs item %d\n", ts_list.size());
    for (unsigned int i = 0; i < bb_list.size(); i++) {
        delete bb_list[i];
    }
}
void PriorityPeer::add_ts(uint64_t ts){
//    boost::mutex::scoped_lock scoped_lock(mutex);
    ts_list.push_back(ts);
}

//ask for BB copy
void PriorityPeer::add_bb(BackBundle * bb){
    boost::mutex::scoped_lock scoped_lock(mutex);
    //make a copy of BB
    bb_list.push_back(new BackBundle(*bb));
}