//
//  syncHeader.h
//  Decision
//
//  Created by Yue Huang on 2014-03-07.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__syncHeader__
#define __Decision__syncHeader__

#include "syncEntry.h"
#include "backBundle.h"
#include <vector>

class SyncHeader:public SyncEntry{
    std::vector<uint64_t> ts_list;
    BackBundle::Header header;
    bool synced;
public:
    //TODO: maybe add a cache map in future
    SyncHeader(std::vector<SyncEntry *>& se_list, unsigned int sync_region, unsigned int size);
    SyncHeader(BackBundle::Header header );
    ~SyncHeader();
    
    BackBundle::Header get_header();
    
    bool is_synced();
    bool is_ts_in_bb(uint64_t ts);
    
    void sync_with_self(std::vector<SyncEntry *>& se_list, unsigned int sync_region);
    void sync_with_other_header(SyncHeader * other_sh);
   
    std::vector<uint64_t> get_ts_list();
    
//    std::vector<uint64_t> get_list();
};


#endif /* defined(__Decision__syncHeader__) */
