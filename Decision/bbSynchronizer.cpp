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
    if (!is_empty()) return bb_list.back()->is_bb_empty();
    else return false;
}

bool BB_Synchronizer::is_empty(){
    return bb_list.empty();
}
//there is still a problem here
BackBundle::Header BB_Synchronizer::get_latest_header(){
    return bb_list.back()->get_header();
}

bool BB_Synchronizer::is_header_in_bb(BackBundle::Header h){
    if (is_empty()) return false;
    
    for (unsigned int i = 0; i < bb_list.size(); i++) {
        if (bb_list[i]->get_header() == h) return true;
    }
    return false;
}

void BB_Synchronizer::add_bb(BackBundle * bb){
    bb_list.push_back(bb);
}

void BB_Synchronizer::add_empty_bb_with_header(BackBundle::Header &h){
    bb_list.push_back(new BackBundle(h));
}

bool BB_Synchronizer::is_ts_in_BB(uint64_t ts){
    for (unsigned int i = 0; i < bb_list.size(); i++) {
        if (bb_list[i]->search(ts)) return true;
    }
    return false;
}