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

class SyncP:public SyncEntry{
    
public:
    SyncP();
    SyncP(uint64_t ts);
    SyncP(const SyncP & sp);
    bool operator< (const SyncP & sp){ return ts< sp.ts; }
    ~SyncP();
};

#endif /* defined(__Decision__syncPoint__) */
