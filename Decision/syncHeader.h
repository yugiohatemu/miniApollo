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
    //TODO:contains real header
    BackBundle::Header goal;
public:
    std::vector<uint64_t> ts_list;
    SyncHeader();
    ~SyncHeader();
    bool is_synced();
    
//    std::vector<uint64_t> get_list();
};


#endif /* defined(__Decision__syncHeader__) */
