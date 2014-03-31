//
//  packet.h
//  Decision
//
//  Created by Yue Huang on 2014-03-28.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__packet__
#define __Decision__packet__

#include "commonDefines.h"
#include "action_C.h"
#include "AROObjectSynchronizer.h"

union BLOCK_PAYLOAD{
#ifdef SYN_CHEAT
    unsigned int header_count;
#endif
    uint64_t ts;
    SyncPoint sync_point;
    Raw_Header_C *raw_header;
    float weight;
    
    BLOCK_PAYLOAD(){}
    ~BLOCK_PAYLOAD(){}
};

struct Packet{
    enum FLAG{
        BULLY_WEIGHT,
        HEADER_PROCESS_SP, HEADER_MERGE_HEADER,
#ifdef SYN_CHEAT
        HEADER_COUNT,
#endif
        APP_PROCESS_SP,APP_MERGE_ACTION,
        BB_PROCESS_SP,BB_MERGE_ACTION,
    };
    
    enum FLAG flag;
    BLOCK_PAYLOAD * content;
    Packet(FLAG flag);
    Packet(Packet &p);
    ~Packet();
};


#endif /* defined(__Decision__packet__) */
