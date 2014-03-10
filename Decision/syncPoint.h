//
//  syncPoint.h
//  Decision
//
//  Created by Yue Huang on 2014-03-07.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__syncPoint__
#define __Decision__syncPoint__

#include "syncEntry.h"

class SyncPoint:public SyncEntry{
    
public:
    SyncPoint();
    SyncPoint(uint64_t ts);
    SyncPoint(const SyncPoint & sp);
    bool operator< (const SyncPoint & sp){ return ts< sp.ts; }
    ~SyncPoint();
};

#endif /* defined(__Decision__syncPoint__) */
