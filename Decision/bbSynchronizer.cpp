//
//  bbSynchronizer.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-03.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "bbSynchronizer.h"

BB_Synchronizer::BB_Synchronizer(){
    
}

BB_Synchronizer::~BB_Synchronizer(){
    for (unsigned int i = 0; i < bb_list.size(); i++) delete bb_list[i];
}

/////////////////////////////////////////////////////////////////////////
bool BB_Synchronizer::is_BB_synced(){
    if (!is_BB_empty()){
        if (bb_list.back()->is_bb_empty()) return false;
        else return bb_list.back()->is_bb_synced();
    }
    else return false;
}

bool BB_Synchronizer::is_BB_empty(){
    return bb_list.empty();
}
//there is still a problem here
BackBundle::Header BB_Synchronizer::get_latest_header(){
    return bb_list.back()->get_header();
}

bool BB_Synchronizer::is_header_in_BB(BackBundle::Header h){
    if (is_BB_empty()) return false;
    
    for (unsigned int i = 0; i < bb_list.size(); i++) {
        if (bb_list[i]->get_header() == h) return true;
    }
    return false;
}

void BB_Synchronizer::add_BB(BackBundle * bb){
    bb_list.push_back(bb);
}

void BB_Synchronizer::add_empty_BB_with_header(BackBundle::Header &h){
    bb_list.push_back(new BackBundle(h));
}

bool BB_Synchronizer::is_ts_in_BB(uint64_t ts){
    for (unsigned int i = 0; i < bb_list.size(); i++) {
        if (bb_list[i]->search(ts)) return true;
    }
    return false;
}

//Here should be a synchronizer, should only be used when there is a header
void BB_Synchronizer::try_pack_BB(std::vector<uint64_t> &sync){ //pass in a bool&?
    
    BackBundle::Header h = get_latest_header();
    //find item before that
    unsigned int high = 0; unsigned int low = (int) sync.size()-1;
    for (; high < sync.size(); high++) if (h.to > sync[high]) break;
    for (; low > 0 ; low--) if (h.from < sync[low]) break;
    
    //have high and low now
    bb_list.back()->set(sync, low, high);
    //Currently , if the size doesn't match , we know it is not compelte
    
}


