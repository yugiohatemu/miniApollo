//
//  bbSynchronizer.h
//  Decision
//
//  Created by Yue Huang on 2014-03-03.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__bbSynchronizer__
#define __Decision__bbSynchronizer__

#include <vector>
#include "backBundle.h"
#include <inttypes.h>
class BB_Synchronizer{
    std::vector<BackBundle *> bb_list;
public:
    BB_Synchronizer();
    ~BB_Synchronizer();
    bool is_BB_synced();
    BackBundle::Header get_latest_header();
    void add_bb(BackBundle * bb);
    void add_empty_bb_with_header(BackBundle::Header &h);
    
    bool is_empty();
    bool is_ts_in_BB(uint64_t ts);
    bool is_header_in_bb(BackBundle::Header h);
    //BackBundle::Header  get_specific_header
    //BackBundle get_back_bundle_with_header(Header h)
};

#endif /* defined(__Decision__bbSynchronizer__) */
