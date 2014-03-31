//
//  packet.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-28.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "packet.h"

Packet::Packet(FLAG flag):flag(flag){
    content = new BLOCK_PAYLOAD();
}

Packet::Packet(Packet &p):flag(p.flag){
    content = new BLOCK_PAYLOAD();
    switch (flag){
        case HEADER_PROCESS_SP: case APP_PROCESS_SP: case BB_PROCESS_SP:
            content->sync_point = p.content->sync_point;
            break;
        case HEADER_MERGE_HEADER:
            content->raw_header = copy_raw_header(p.content->raw_header);
            break;
        case APP_MERGE_ACTION:case BB_MERGE_ACTION:
            content->ts = p.content->ts;
            break;
#ifdef SYN_CHEAT
        case HEADER_COUNT:
            content->header_count = p.content->header_count;
            break;
#endif
        default:
            break;
    }
}
Packet::~Packet(){
    if(flag == HEADER_MERGE_HEADER) free_raw_header(content->raw_header);
    delete content;
}
