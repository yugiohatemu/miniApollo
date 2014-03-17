//
//  backBundle.cpp
//  Decision
//
//  Created by Yue Huang on 2014-02-28.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "backBundle.h"
#include <algorithm>
//#include "log.h"

BackBundle::BackBundle(std::vector<uint64_t> sync, unsigned int size){
//    current = target;
    bb_synced = true;
    ts_list.reserve(size);
    for (unsigned int i =0; i < size; i++) ts_list.push_back(sync[i]);
}

BackBundle::BackBundle(Header &h):target(h),current(){
    bb_synced = false;
}

BackBundle::BackBundle(const BackBundle & b){
    target = b.target;
    current = b.current;
    bb_synced = b.bb_synced;
    for (unsigned int i =0; i < b.ts_list.size(); i++) ts_list.push_back(b.ts_list[i]);
    
}

BackBundle::~BackBundle(){

}

////////////////////////

unsigned int BackBundle::size(){
    return (int)ts_list.size();
}

bool BackBundle::is_ts_in_bb(uint64_t ts){
    if (ts_list.empty()) return false;
    else return  std::binary_search(ts_list.begin(), ts_list.end(), ts);
}

void BackBundle::add_ts(uint64_t ts){
    ts_list.push_back(ts);
    std::sort(ts_list.begin(), ts_list.end());
    update_sync();
}

bool BackBundle::operator<(const BackBundle & bb) const{
    return ts_list[0]< bb.ts_list[0];
}

bool BackBundle::is_bb_synced(){
    if (ts_list.empty()) return false;
    return bb_synced;
}

void BackBundle::update_sync(){
    //recreate header based on current content
    if (!ts_list.empty()) current = Header(ts_list);
    bb_synced = (target == current);
}
