//
//  backBundle.cpp
//  Decision
//
//  Created by Yue Huang on 2014-02-28.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "backBundle.h"
#include <algorithm>
#include "log.h"

BackBundle::BackBundle(std::vector<uint64_t> & sync, unsigned int size): target(sync, size){
    current = target;
    bb_synced = true;
    
    //we do all of them here because I want this process to be atomic
    ts_list.reserve(size);
    for (unsigned int i =0; i < size; i++) ts_list.push_back(sync[i]);
//    sync.erase(sync.begin(),sync.end() + size);
}

BackBundle::BackBundle(const BackBundle & b){
    target = b.target;
    current = b.current;
    bb_synced = b.bb_synced;
    for (unsigned int i =0; i < b.ts_list.size(); i++) ts_list.push_back(b.ts_list[i]);
    
}

BackBundle::BackBundle(Header &h):target(h){
    bb_synced = false;
}

BackBundle::~BackBundle(){
    if (ts_list.empty()) {
        Log::log().Print("BB empty\n");
    }else
        Log::log().Print("BB size: %d %lld - %lld, synced: %c\n", ts_list.size(), ts_list.front(),ts_list.back(), bb_synced ? 'Y':'N');
}

////////////////////////

std::vector<uint64_t>& BackBundle::get_list(){
    return ts_list;
}

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
    if (target == current) bb_synced = true;
    return bb_synced;
}

void BackBundle::update_sync(){
    //recreate header based on current content
    if (!ts_list.empty()) target = Header(ts_list);
    bb_synced = (target == current);
}
