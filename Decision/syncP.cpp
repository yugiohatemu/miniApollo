//
//  SyncP.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-07.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "syncP.h"
#include <sys/time.h>


SyncP::SyncP():SyncEntry(){
    struct timeval tp;
    gettimeofday(&tp, NULL);
    ts = tp.tv_sec * 1e6 + tp.tv_usec;
}

SyncP::SyncP(uint64_t ts):SyncEntry(ts){
    
}

SyncP::SyncP(const SyncP & sp):SyncEntry(){
    ts = sp.ts;
}

SyncP::~SyncP(){
    
}