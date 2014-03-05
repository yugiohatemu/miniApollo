//
//  dataPool.cpp
//  Decision
//
//  Created by Yue Huang on 2014-02-28.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "dataPool.h"
#include <stdlib.h>
#include <iostream>
#include "log.h"
DataPool::DataPool(){
    ts_list.reserve(50);
}

DataPool::~DataPool(){
    Log::log().Print("Total feed # %d\n", ts_list.size());
//    for (unsigned int i = 0; i < ts_list.size(); i++) {
//         Log::log().Print("%d - %lld\n",i, ts_list[i]);
//    }
}

uint64_t DataPool::get_random_ts(){
    return ts_list[rand() % (ts_list.size())];
}

void DataPool::add_ts(uint64_t ts){
    boost::mutex::scoped_lock scoped_lock(mutex);
    ts_list.push_back(ts);
}