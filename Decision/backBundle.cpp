//
//  backBundle.cpp
//  Decision
//
//  Created by Yue Huang on 2014-02-28.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "backBundle.h"
#include <algorithm>

BackBundle::BackBundle(std::vector<uint64_t> & sync, unsigned int size):size(size), header(sync, size){
    set(sync, size);
    bb_synced = true;
    
}

BackBundle::BackBundle(Header &h):header(h), size(h.size),bb_empty(true){
    bb_synced = false;
}

void BackBundle::set(std::vector<uint64_t> & sync, unsigned int size){
    
//    std::copy(sync.begin(), sync.begin() + size, ts_list);
   
    bb_empty = false;
}

void BackBundle::set(std::vector<uint64_t> & sync, unsigned int low, unsigned int high){
    
//    std::copy(sync.begin()+low, sync.begin() + high, ts_list);
//    std::sort(ts_list.begin(), ts_list.end());
    bb_empty = false;
    bb_synced = (high - low + 1 == size);
}

BackBundle::~BackBundle(){
   
}

////////////////////////
bool BackBundle::search(uint64_t ts){
    if (bb_empty) return false;
    else return  std::binary_search(ts_list.begin(), ts_list.end(), ts);
}

void BackBundle::add_ts(uint64_t ts){
    ts_list.push_back(ts);
    std::sort(ts_list.begin(), ts_list.end());
}

bool BackBundle::operator<(const BackBundle & bb) const{
    return ts_list[0]< bb.ts_list[0];
}

BackBundle::Header BackBundle::get_header(){
    return header;
}

bool BackBundle::is_bb_empty(){
    return bb_empty;
}

bool BackBundle::is_bb_synced(){
    return bb_synced;
}