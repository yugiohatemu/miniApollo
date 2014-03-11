//
//  AROObjectSynchronizer_C.h
//  apollo-ios-final
//
//  Created by Adam Kinsman on 13-08-14.
//  Copyright (c) 2013 Arroware Industries Inc. All rights reserved.
//

#ifndef apollo_ios_final_AROObjectSynchronizer_C_h
#define apollo_ios_final_AROObjectSynchronizer_C_h

#ifdef __cplusplus
extern "C" {
#endif

#define AROObjectSynchronizer_C_debugVerbosity  0

//#define OLD_SYNCHRONIZATION_LATTICE
#ifdef OLD_SYNCHRONIZATION_LATTICE
#define moduloSplitRange(mid,id1,id2,res,num) \
	moduloSplitRange_fromIDX0(mid,id1,id2,res)
#else
#define moduloSplitRange(mid,id1,id2,res,num) \
	moduloSplitRange_fromIDX0(mid,(num)-(id2),(num)-(id1),res); \
	mid = (num)-mid;                          \
	if (!((mid>(id1)) && (mid<(id2))))        \
	    mid = ((id1)+(id2))/2;
#endif

#define moduloSplitRange_fromIDX0(mid,id1,id2,res) \
    mid = ((id2)-(id1))/(res);                  \
    if (mid == 1) {                             \
        mid = (id1)+(res)-1;                    \
    } else {                                    \
        mid = ((id2)+(id1))/2;                  \
        if ((mid%(res))>((res)/2)) mid+=(res);  \
        mid -= mid%(res);                       \
    }                                           \
    if (!((mid>(id1)) && (mid<(id2))))          \
        mid = ((id1)+(id2))/2;

typedef enum AROObjectSynchronizerPhaseType_e {
    tAROObjectSynchronizerPhase_idle,
    tAROObjectSynchronizerPhase_partition,
    tAROObjectSynchronizerPhase_aggregate
} AROObjectSynchronizerPhaseType;

typedef struct SyncPoint_s { //int staleCount;
	int res, id1, id2; uint64_t ts1, ts2; uint32_t hash;
} SyncPoint;

typedef struct AROObjectSynchronizerInf_s {
	int numSyncPoints, allocSyncPoints;
    SyncPoint *syncPoints;
    int localRes;

#if AROObjectSynchronizer_C_debugVerbosity >= 1
	int debugLevel; char *debugLabel;
#endif
} AROObjectSynchronizerInf;

AROObjectSynchronizerInf *AROObjectSynchronizerInf_init(void);
void AROObjectSynchronizerInf_free(AROObjectSynchronizerInf *op);

void AROObjectSynchronizerInf_setDebugInfo(AROObjectSynchronizerInf *op, int debugLevel_, const char *debugLabel_);

int AROObjectSynchronizerInf_processSyncPoint(
	AROObjectSynchronizerInf *syncState, SyncPoint *proc, int numts, uint64_t *ts, int stride, 
	uint64_t *tsreq, int *numreq_ret, SyncPoint *newsyncpoints, int *num_newsyncpoints_ret
);
void AROObjectSynchronizerInf_resolveLocalAlignment(
	AROObjectSynchronizerInf *syncState, int numts, uint64_t *ts, int stride, 
	uint64_t *tsreq, int *numreq_ret, SyncPoint *newsyncpoints, int *num_newsyncpoints_ret
);
int AROObjectSynchronizerInf_compactSyncPoints(SyncPoint *spreqBuf, int numspreqBuf, int max_numspreqBuf, SyncPoint *spreq, int numspreq);
int AROObjectSynchronizerInf_checkAllSyncPoints(AROObjectSynchronizerInf *syncState, int numts, uint64_t *ts, int stride);

#ifdef __cplusplus
}
#endif

#endif
