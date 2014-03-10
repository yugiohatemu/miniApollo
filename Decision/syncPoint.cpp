//
//  syncPoint.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-07.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "syncPoint.h"
#include <sys/time.h>


SyncPoint::SyncPoint():SyncEntry(){
    struct timeval tp;
    gettimeofday(&tp, NULL);
    ts = tp.tv_sec * 1e6 + tp.tv_usec;
}

SyncPoint::SyncPoint(uint64_t ts):SyncEntry(ts){
    
}

SyncPoint::SyncPoint(const SyncPoint & sp):SyncEntry(){
    ts = sp.ts;
}


SyncPoint::~SyncPoint(){
    
}