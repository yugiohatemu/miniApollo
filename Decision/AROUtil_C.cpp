//
//  AROUtil_C.c
//  apollo-ios-final
//
//  Created by Phil Kinsman on 2013-08-12.
//  Copyright (c) 2013 Arroware Industries Inc. All rights reserved.
//

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <boost/atomic.hpp>

#include "AROUtil_C.h"
#include "AROLogInterface.h"
#include "AROObjectCommonDefines_C.h"

static int umalloctraceflag = 0;
static boost::atomic<int> umalloccounter(0);

uint64_t htonll(uint64_t value)
{
    int num = 1;
    if(*(char *)&num == 1)
        return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32LL) | htonl(value >> 32);
    else
        return value;
}

uint64_t ntohll(uint64_t value)
{
    int num = 1;
    if(*(char *)&num == 1)
        return ((uint64_t)ntohl(value & 0xFFFFFFFF) << 32LL) | ntohl(value >> 32);
    else
        return value;
}

/*
 void usetmalloccounter(int val) {
 AROLog(logINFO,1,"UTIL","usetmalloccounter - %d -> %d\n", umalloccounter, val);
 umalloccounter = val;
 }
 */

int ugetmalloccounter() {
    return umalloccounter.load(boost::memory_order::memory_order_relaxed);
}

void usetmalloctraceflag() {
    AROLog(logINFO,1,"UTIL","umalloctrace activated\n");
    umalloctraceflag = 1;
}

void uunsetmalloctraceflag() {
    AROLog(logINFO,1,"UTIL","umalloctrace de-activated\n");
    umalloctraceflag = 0;
}

void *umalloc(int size) {
    void *retval = malloc(size); umalloccounter++;
    if (umalloctraceflag == 1)
        AROLog(logINFO,1,"UTIL","umalloc - %d @ %p : %d\n", size, retval, ugetmalloccounter());
    return retval;
}

char *ustrdup(const char *op) {
    char *retval; retval = strndup(op, AOc_MaxTokenFieldSize); umalloccounter++;
    if (umalloctraceflag == 1)
        AROLog(logINFO,1,"UTIL","ustrdup %d @ %p : %d\n", (int)strlen(retval)+1, retval, ugetmalloccounter());
    return retval;
}

void *urealloc(void *ptr, int size) {
    void *retval = realloc(ptr, size);
    if ((ptr == NULL) && (retval != NULL)) umalloccounter++;
    
    if (umalloctraceflag == 1)
        AROLog(logINFO,1,"UTIL","urealloc - %d @ %p -> %p : %d\n", size, ptr, retval, ugetmalloccounter());
    return retval;
}

void ufree(void *ptr) {
    if (ptr) {
        umalloccounter--;
        if (umalloctraceflag == 1)
            AROLog(logINFO,1,"UTIL","ufree - %p : %d\n", ptr, ugetmalloccounter());
        free(ptr);
    }
}

uint64_t AOc_localTimestamp(void) {
    struct timeval tp; gettimeofday(&tp, NULL);
    return tp.tv_sec * 1e6 + tp.tv_usec;
}

uint32_t AOc_localTimestamp31(void) {
    return (uint32_t)(AOc_localTimestamp() & (uint32_t)0x7fffffff);
}

uint16_t AOc_localTimestamp15_ms(void) {
    return (uint16_t)((AOc_localTimestamp() / 1000) & (uint16_t)0x7fff);
}

void AOc_uint64ToString(uint64_t value, char *str) {
	sprintf(str, "%08x%08x",  (int)((value >> 32ll) & 0xFFFFFFFF), (int)(value & 0xFFFFFFFF));
    //#if AROIMConversation_C_debugVerbosity >= 1
    //	AROLog(logINFO,1,"UTIL",("genTimeString %lld -> '%s'\n", timestamp, timestring);
    //#endif
}

uint64_t AOc_uint64FromString(const char *str) {
	uint64_t retval = 0;
	sscanf(str, "%16llx", (unsigned long long *)&retval);
    
	
    //#if AROIMConversation_C_debugVerbosity >= 1
    //	AROLog(logINFO,1,"UTIL",("timestampFromString '%s' -> %lld\n", timestring, retval);
    //#endif
	return retval;
}

void AOc_uint32ToString(uint32_t value, char *str) {
    sprintf(str, "%08x", value);
}

uint32_t AOc_uint32FromString(const char *str) {
    uint32_t retval;
    sscanf(str, "%x", &retval);
    return retval;
}

uint32_t AOc_hashRawData(uint32_t seed, uint8_t *bytes, int length) {
	uint32_t retval = seed; int i; char hibyte;
	for (i = 0; i < length; i++) {
		hibyte = (retval >> 24) & 0xFF; retval <<= 8;
		uint8_t val = (bytes == NULL) ? 0x0 : bytes[i];
		retval ^= 0xbeefab1e ^ ((hibyte ^ val ^ 0xa5) & 0xFF);
	}
	return retval;
}
