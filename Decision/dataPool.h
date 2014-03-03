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

class DataPool{
    std::vector<uint64_t> ts_list;
    boost::mutex mutex;
  
public:
    DataPool();
    ~DataPool();
    uint64_t get_random_ts();
    void add_ts(uint64_t ts);
};

#endif /* defined(__Decision__dataPool__) */
