//
//  dataPool.h
//  Decision
//
//  Created by Yue Huang on 2014-02-28.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__dataPool__
#define __Decision__dataPool__

#include <vector>
#include <boost/thread/mutex.hpp>
#include <inttypes.h>

//Acts as a Priority peer so that it is always on line to keep all data
//Technically it is an instance

class PriorityPeer{
    std::vector<uint64_t> ts_list;
//    std::vector<SyncHeader *> sh_list;
    boost::mutex mutex;
public:
    PriorityPeer();
    ~PriorityPeer();
    void add_ts(uint64_t ts);
//    void add_sh(SyncHeader * sh);
    
};

#endif /* defined(__Decision__dataPool__) */
