//
//  syncHeader.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-07.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "syncHeader.h"
#include "log.h"
#include <algorithm>
SyncHeader::SyncHeader(std::vector<SyncEntry *>& se_list, unsigned int sync_region, unsigned int size):SyncEntry(){
    for (unsigned int i = sync_region; i < sync_region + size;i++) {
        ts_list.push_back(se_list[i]->ts);
        se_list[i]->ts = 0;
    }
    header = BackBundle::Header(ts_list); //TODO: check correctness
    synced = true;
}

SyncHeader::SyncHeader(BackBundle::Header header):header(header){
    synced = false;
}

SyncHeader::~SyncHeader(){
    Log::log().Print("H: %d %lld - %lld VS %d, %lld - %lld\n", header.size, header.from, header.to, ts_list.size(), ts_list.front(), ts_list.back());
}

std::vector<uint64_t> SyncHeader::get_ts_list(){
    return ts_list;
}

BackBundle::Header SyncHeader::get_header(){
    return header;
}

bool SyncHeader::is_synced(){
    return synced;
}

void SyncHeader::sync_with_self(std::vector<SyncEntry *>& se_list, unsigned int sync_region){
    if (synced) return ;
    
    for(unsigned int i = sync_region; i < se_list.size(); i++){
        if (header.is_ts_in_header(se_list[i]->ts)) { 
            ts_list.push_back(se_list[i]->ts);
            se_list[i]->ts = 0;
        }
    }
    
    if(!ts_list.empty()){
        std::sort(ts_list.begin(), ts_list.end());
        BackBundle::Header current = BackBundle::Header(ts_list);
        synced = (current == header);
    }
}

void SyncHeader::sync_with_other_header(SyncHeader * other_sh){
    std::vector<uint64_t> other_ts_list = other_sh->get_ts_list();
    
    for (unsigned int i = 0; i < other_ts_list.size(); i++) {
        if (!is_ts_in_bb(other_ts_list[i])) {
            ts_list.push_back(other_ts_list[i]);
        }
    }
    std::sort(ts_list.begin(), ts_list.end());
    BackBundle::Header current = BackBundle::Header(ts_list);
    synced = (current == header);
}




bool SyncHeader::is_ts_in_bb(uint64_t ts){
    return std::binary_search(ts_list.begin(), ts_list.end(), ts);
}


