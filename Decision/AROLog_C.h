
//
//  AROLog.h
//  ApolloPriorityPeer
//
//  Created by Henry Ko on 2013-05-23.
//  Copyright (c) 2013 Arroware. All rights reserved.
//

#ifndef __ApolloPriorityPeer__AROLog_C__
#define __ApolloPriorityPeer__AROLog_C__

enum LogLevel {
    logERROR = 0,
    logWARNING,
    logINFO,
    logDEBUG,
    logDEBUG1,
    logDEBUG2,
    logDEBUG3,
    logDEBUG4
};

#ifdef __cplusplus
extern "C" {
#endif 

void AROLog_Print(int level, int timed, const char *tag, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* defined(__ApolloPriorityPeer__AROLog_C__) */
