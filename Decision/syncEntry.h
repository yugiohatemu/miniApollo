//
//  syncEntry.h
//  Decision
//
//  Created by Yue Huang on 2014-03-07.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__syncEntry__
#define __Decision__syncEntry__

#include <inttypes.h>

class SyncEntry{
public:
    uint64_t ts;
    SyncEntry(){ ts = 0;};
    virtual ~SyncEntry(){}
    bool operator< (const SyncEntry & se) const{
        return ts < se.ts;
    }
};

#endif /* defined(__Decision__syncEntry__) */
