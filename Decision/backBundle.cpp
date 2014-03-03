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
}

BackBundle::BackBundle(Header &h):header(h){
    bb_empty = true;
}

void BackBundle::set(std::vector<uint64_t> & sync, unsigned int size){
    ts_list = new uint64_t [size];
    std::copy(sync.begin(), sync.begin() + size, ts_list);
    std::sort(ts_list, ts_list + size);
    
    bb_empty = false;
}

BackBundle::~BackBundle(){
    if (!bb_empty) delete [] ts_list;
}

////////////////////////
bool BackBundle::search(uint64_t ts){
    if (bb_empty) return false;
    else return  std::binary_search(ts_list, ts_list+size, ts);
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