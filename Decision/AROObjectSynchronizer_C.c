//
//  AROObjectSynchronizer_C.c
//  apollo-ios-final
//
//  Created by Adam Kinsman on 13-08-14.
//  Copyright (c) 2013 Arroware Industries Inc. All rights reserved.
//

#include <stdio.h>

#include "AROLog_C.h"
#include "AROUtil_C.h"
#include "AROObjectCommonDefines_C.h"
#include "AROObjectSynchronizer_C.h"

#define AROObjectSynchronizer_allocBlockSize 100

//#define AROObjectSynchronizer_C_debugVerbosity  0

int AROObjectSynchronizerInf_findIDForTimestamp(uint64_t searchts, int num_syncpoints, SyncPoint *syncpoints, int numts, uint64_t *ts, int stride) {
	if ((searchts < *(ts + 0*stride)) && (searchts < syncpoints[0].ts1)) return 0;
	if ((searchts > *(ts+(numts-1)*stride)) && (searchts > syncpoints[num_syncpoints].ts1))
		return (numts+1 > syncpoints[num_syncpoints].id1) ? numts+1 : syncpoints[num_syncpoints].id1;

#if AROObjectSynchronizer_C_debugVerbosity >= 1
//	int debugLevel = syncState->debugLevel; 
//	const char *debugLabel = (syncState->debugLabel == NULL) ? "Sync_C" : syncState->debugLabel;
	int debugLevel = 1; const char *debugLabel = "Sync_C";
#endif
#if AROObjectSynchronizer_C_debugVerbosity >= 2
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"Finding id for ts: %lld (%d syncpts)\n", (long long int)searchts, num_syncpoints);
#endif

    // id(ts) return the largest id we can definitively assign for this timestamp, i.e. the best lower bound
    int lo = 0, hi = numts-1; binaryReduceRangeWithKey(lo,hi,searchts,*(ts+lo*stride),*(ts+hi*stride),*(ts+pivot*stride));
#if AROObjectSynchronizer_C_debugVerbosity >= 2
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\tchk1: %d %d\n", lo, hi);
#endif
    int retval = (hi > lo) ? hi+1 : lo+1; // the value from our local list
#if AROObjectSynchronizer_C_debugVerbosity >= 2
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\tchk2: %d %d\n", lo, hi);
#endif
	int splo = 0, sphi = num_syncpoints;
	binaryReduceRangeWithKey(splo,sphi,searchts,
		syncpoints[splo].ts1,syncpoints[sphi].ts1,syncpoints[pivot].ts1);
#if AROObjectSynchronizer_C_debugVerbosity >= 2
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\tchk3: %d %d | %d %d - %d\n", lo, hi, splo, sphi, retval);
#endif
	if (sphi > splo) splo = sphi;
#if AROObjectSynchronizer_C_debugVerbosity >= 2
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\tchk4: %d %d | %d %d - %d\n", lo, hi, splo, sphi, retval);
#endif
	if (syncpoints[splo].ts1 == searchts) {
#if AROObjectSynchronizer_C_debugVerbosity >= 2
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\tchk5a: %d %d | %d %d - %d\n", lo, hi, splo, sphi, retval);
#endif
		if (syncpoints[splo].id1 > retval) retval = syncpoints[splo].id1;
#if AROObjectSynchronizer_C_debugVerbosity >= 2
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\tchk6a: %d %d | %d %d - %d\n", lo, hi, splo, sphi, retval);
#endif
	} else if (splo > 0) {
#if AROObjectSynchronizer_C_debugVerbosity >= 2
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\tchk5b: %d %d | %d %d - %d\n", lo, hi, splo, sphi, retval);
#endif
		if (syncpoints[splo-1].id1+1 > retval) retval = syncpoints[splo-1].id1+1;
#if AROObjectSynchronizer_C_debugVerbosity >= 2
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\tchk6b: %d %d | %d %d - %d\n", lo, hi, splo, sphi, retval);
#endif
	} else retval = 1;
#if AROObjectSynchronizer_C_debugVerbosity >= 2
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\tchk7: %d %d | %d %d - %d\n", lo, hi, splo, sphi, retval);
#endif
    return retval;
}

uint64_t AROObjectSynchronizerInf_findTimestampForID(int searchid, int num_syncpoints, SyncPoint *syncpoints, int numts, uint64_t *ts, int stride) {
	if ((searchid < 1) && (searchid < syncpoints[0].id1)) return 0ll;
	uint64_t lastts = *(ts+(numts-1)*stride);
	uint64_t maxts = (lastts > syncpoints[num_syncpoints].ts1) ?
		lastts : syncpoints[num_syncpoints].ts1;
	if ((searchid > numts) && (searchid > syncpoints[num_syncpoints].id1)) return maxts;
    
	// ts(id) return the lowest timestamp we can definitively assign for this id i.e. the best upper bound
	uint64_t retval = (searchid <= numts) ? *(ts+(searchid-1)*stride) : maxts;
	int splo = 0, sphi = num_syncpoints;
	binaryReduceRangeWithKey(splo,sphi,searchid,syncpoints[splo].id1,syncpoints[sphi].id1,syncpoints[pivot].id1);
	if (syncpoints[splo].ts1 < retval) retval = syncpoints[splo].ts1;

    return retval;
}

AROObjectSynchronizerInf *AROObjectSynchronizerInf_init(void) {
    AROObjectSynchronizerInf *retval;
    retval = (AROObjectSynchronizerInf *)AOc_malloc(sizeof(AROObjectSynchronizerInf));
    retval->numSyncPoints = -1;
    retval->allocSyncPoints = AROObjectSynchronizer_allocBlockSize;
    retval->syncPoints = (SyncPoint *)AOc_malloc((retval->allocSyncPoints+1) * sizeof(SyncPoint));
    retval->localRes = 3;
	
#if AROObjectSynchronizer_C_debugVerbosity >= 1
	retval->debugLevel = 0; retval->debugLabel = AOc_strdup("Sync_C");
#endif
    return retval;
}

void AROObjectSynchronizerInf_free(AROObjectSynchronizerInf *op) {
    if (op == NULL) return; 
#if AROObjectSynchronizer_C_debugVerbosity >= 1
	if (op->debugLabel != NULL) { AOc_free(op->debugLabel); op->debugLabel = NULL; }
#endif
    AROLog_Print(logDEBUG4,0,"Sync_C","FREEA - %d\n", ugetmalloccounter());
    if (op->syncPoints != NULL)
        AOc_free(op->syncPoints);
    AROLog_Print(logDEBUG4,0,"Sync_C","FREEB - %d\n", ugetmalloccounter());
    AOc_free(op);
    AROLog_Print(logDEBUG4,0,"Sync_C","FREEC - %d\n", ugetmalloccounter());
}

void AROObjectSynchronizerInf_setDebugInfo(AROObjectSynchronizerInf *op, int debugLevel_, const char *debugLabel_) {
	AROLog_Print(logDEBUG4,0,"Sync_C","Updating Sync_C debug info: %d %s\n", 
		debugLevel_, (debugLabel_ == NULL) ? "-" : debugLabel_);
	if (op == NULL) return;
#if AROObjectSynchronizer_C_debugVerbosity >= 1
	if (debugLevel_ >= 0) op->debugLevel = debugLevel_;
	if (debugLabel_ != NULL) {
		if (op->debugLabel != NULL) AOc_free(op->debugLabel);
		char tmpLabel[31]; snprintf(tmpLabel, 30, "Sync_C-%s", debugLabel_);
		op->debugLabel = AOc_strdup(tmpLabel);
	}
#endif	
}

void AROObjectSynchronizerInf_sweepToAnchor(AROObjectSynchronizerInf *syncState, 
	uint64_t *ts, int numts, int stride, uint64_t *tsreq, int *numreq
) {
	int i, lastloc = -1, lastbnd, lastoff, newloc, newbnd, newoff; 
	SyncPoint *sypts = syncState->syncPoints;

#if AROObjectSynchronizer_C_debugVerbosity >= 1
	int debugLevel = syncState->debugLevel; 
	const char *debugLabel = (syncState->debugLabel == NULL) ? "Sync_C" : syncState->debugLabel;
#endif

	// scan through to drop corrupted syncpoints
	for (i = 0; i <= syncState->numSyncPoints; i++) { if (sypts[i].ts1 == 0ll) { int j; 
		for (j = i; j < syncState->numSyncPoints; j++) sypts[j] = sypts[j+1];
		syncState->numSyncPoints--; i--;
	} }

	for (i = 0; i <= syncState->numSyncPoints; i++) { 

#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].res, sypts[i].hash, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
int j; for (j = 0; j <= syncState->numSyncPoints; j++) 
	if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t%d : %08x %d %d %lld %lld\n", j, sypts[j].hash, sypts[j].id1, sypts[j].id2, (long long int)sypts[j].ts1, (long long int)sypts[j].ts2);
#endif

#if AROObjectSynchronizer_C_debugVerbosity >= 1
		SyncPoint tmppt = sypts[i];
#endif		
		if (i > 0) if (sypts[i].id1 <= sypts[i-1].id1)
			sypts[i].id1 = sypts[i-1].id1+1;

#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t--chksypts1 i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].res, sypts[i].hash, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
#endif

		int lo = 0, hi = numts-1; 
		binaryReduceRangeWithKey(lo,hi,sypts[i].ts1, 
			*(ts + lo*stride),*(ts + hi*stride),*(ts + pivot*stride));
#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"chk: %d %d - %lld\n", lo, hi, (long long int)sypts[i].ts1);
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t--chksypts2 i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].res, sypts[i].hash, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
#endif
		if (hi >= lo) { // we have this ts, and its best lower bound is hi
			if (hi > lo) if (sypts[i].ts2 != sypts[i].ts1) { 
				sypts[i].ts2 = sypts[i].ts1; sypts[i].id2 = 1; }

			if (sypts[i].ts2 == sypts[i].ts1) { // multiple entries in our list at this ts
#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t--chksypts2a i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].res, sypts[i].hash, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
#endif
				if (hi+1 > sypts[i].id1) sypts[i].id1 = hi+1;
#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t--chksypts2b i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].res, sypts[i].hash, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
#endif
				if ((hi - lo + 1) > sypts[i].id2) { sypts[i].id2 = hi - lo + 1; 
					tsreq[*numreq] = sypts[i].ts1; (*numreq)++; }
#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t--chksypts2c i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].res, sypts[i].hash, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
#endif
				if (i > 0) if ((sypts[i].id1 - sypts[i].id2) <= sypts[i-1].id1)
					sypts[i].id1 = sypts[i-1].id1+sypts[i].id2;
#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t--chksypts2d i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].res, sypts[i].hash, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
#endif
					
#if AROObjectSynchronizer_C_debugVerbosity >= 1
				if ((sypts[i].id1 != tmppt.id1) || (sypts[i].id2 != tmppt.id2)) 
					if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\tupdating pt_hi %d : (%d %lld) (%d %lld) : %d %d %lld %lld --> %d %d %lld %lld\n", i, 
						lo, (long long int)*(ts + lo*stride), hi, (long long int)*(ts + hi*stride), 
						tmppt.id1, tmppt.id2, (long long int)tmppt.ts1, (long long int)tmppt.ts2, 
						sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2
					);
#endif
			} else { // a single entry in our list at this ts
				sypts[i].ts2 = 0ll; sypts[i].id2 = 0;
				if (hi+1 > sypts[i].id1) sypts[i].id1 = hi+1;
#if AROObjectSynchronizer_C_debugVerbosity >= 1
				if ((sypts[i].id1 != tmppt.id1) || (sypts[i].id2 != tmppt.id2)) 
					if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\tsingle entry at %d : (%d %lld) (%d %lld) : %d %d %lld %lld --> %d %d %lld %lld\n", i, 
						lo, (long long int)*(ts + lo*stride), hi, (long long int)*(ts + hi*stride), 
						tmppt.id1, tmppt.id2, (long long int)tmppt.ts1, (long long int)tmppt.ts2, 
						sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2
					);
#endif
			}
			newloc = i; newbnd = sypts[i].id1-1; newoff = 0;
		} else { // we do not have this ts, the next one before it is the best lower bound
			// request it
			tsreq[*numreq] = sypts[i].ts1; (*numreq)++;

#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t--chksypts3 i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].res, sypts[i].hash, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
#endif

			// update this position
			if (lo+1 > sypts[i].id1) sypts[i].id1 = lo+1;
			if (i > 0) if ((sypts[i].id1 - sypts[i].id2) < sypts[i-1].id1)
				sypts[i].id1 = sypts[i-1].id1;

#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t--chksypts4 i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].res, sypts[i].hash, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
#endif

#if AROObjectSynchronizer_C_debugVerbosity >= 1
			if ((sypts[i].id1 != tmppt.id1) || (sypts[i].id2 != tmppt.id2)) 
				if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\tupdating pt_lo %d : (%d %lld) : %d %d %lld %lld --> %d %d %lld %lld\n", i, 
					lo, (long long int)*(ts + lo*stride), 
					tmppt.id1, tmppt.id2, (long long int)tmppt.ts1, (long long int)tmppt.ts2, 
					sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2
				);
#endif
			newloc = i; newbnd = sypts[i].id1-1; newoff = 0;//newoff = 1;
		}
		if (lastloc >= 0) {
#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t--chksypts5 i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].hash, sypts[i].res, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
			if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\tbr-3 the previous one we had in our list was %d at %d\n", lastbnd, lastloc);
			if ((newbnd + newoff - lastbnd) > (sypts[i].id1 - sypts[lastloc].id1))
				if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\tupdating pt_span %d : bnd %d loc %d : (%d %lld) (%d %lld) : %d %d %lld %lld --> %d %d %lld %lld\n", 
					i, lastbnd, lastloc, 
					lo, (long long int)*(ts + lo*stride), hi, (long long int)*(ts + hi*stride), 
					tmppt.id1, tmppt.id2, (long long int)tmppt.ts1, (long long int)tmppt.ts2, 
					sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2
				);	
#endif
			if ((newbnd + newoff - lastbnd) > (sypts[i].id1 - sypts[lastloc].id1))
				sypts[i].id1 = sypts[lastloc].id1 + newbnd - lastbnd + newoff;
#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\t--chksypts6 i: %d - %d %08x %d %d %lld %lld\n", i, sypts[i].res, sypts[i].hash, sypts[i].id1, sypts[i].id2, (long long int)sypts[i].ts1, (long long int)sypts[i].ts2);
#endif
		}
#if AROObjectSynchronizer_C_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\tupdate last bnd/loc/off %d / %d / %d --> %d / %d / %d\n", 
			lastbnd, lastloc, lastoff, newbnd, newloc, newoff);
#endif
		lastloc = newloc; lastbnd = newbnd; lastoff = newoff;
	}
}

int AROObjectSynchronizerInf_insertSyncPoint(AROObjectSynchronizerInf *syncState, SyncPoint *inspoint) {
#if AROObjectSynchronizer_C_debugVerbosity >= 1
	int debugLevel = syncState->debugLevel; 
	const char *debugLabel = (syncState->debugLabel == NULL) ? "Sync_C" : syncState->debugLabel;
#endif
#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"Inserting - %d %08x %d %d %lld %lld\n", inspoint->res, inspoint->hash, inspoint->id1, inspoint->id2, (long long int)inspoint->ts1, (long long int)inspoint->ts2);
#endif
	int splo = 0, sphi = syncState->numSyncPoints;
	if (syncState->numSyncPoints < 0) { splo = 0;
		// trap here for the case when the local sync picture is empty
		syncState->syncPoints[splo].id1 = inspoint->id1; syncState->syncPoints[splo].id2 = 0;
		syncState->syncPoints[splo].ts1 = inspoint->ts1; syncState->syncPoints[splo].ts2 = 0ll;
		syncState->syncPoints[splo].res = inspoint->res; syncState->syncPoints[splo].hash = 0x0; 
		syncState->numSyncPoints++;
	} else {
		binaryReduceRangeWithKey(splo,sphi,inspoint->ts1,
			syncState->syncPoints[splo].ts1,syncState->syncPoints[sphi].ts1,syncState->syncPoints[pivot].ts1);
#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\tsplo = %d sphi = %d\n", splo, sphi);
#endif
		if (sphi >= splo) { int i; 		
			if (inspoint->id1 > syncState->syncPoints[splo].id1) 
				syncState->syncPoints[splo].id1 = inspoint->id1;
			for (i = splo; i < sphi; i++) 
				if (syncState->syncPoints[i+1].id1 > syncState->syncPoints[splo].id1) 
					syncState->syncPoints[splo].id1 = syncState->syncPoints[i+1].id1;
			if (sphi > splo) {
				for (i = splo+1; i <= syncState->numSyncPoints-sphi+splo; i++) 
					syncState->syncPoints[i] = syncState->syncPoints[i+1];
				syncState->numSyncPoints -= sphi - splo;
			}
		} else {
#if AROObjectSynchronizer_C_debugVerbosity >= 1
//if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\tDoes not exist in syncpoints\n");
#endif
//			if (splo < syncState->numSyncPoints) { 
//				if ((syncState->syncPoints[splo+1].id1 - syncState->syncPoints[splo].id1) < syncState->syncPoints[splo].res) 
//					splo = -1; 
//			}
			if (splo >= 0) { int j;
				if (syncState->numSyncPoints >= syncState->allocSyncPoints) { 
					// re-alloc the space for syncpoints
					syncState->allocSyncPoints += AROObjectSynchronizer_allocBlockSize;
					SyncPoint *tmpsp = (SyncPoint *)AOc_malloc((syncState->allocSyncPoints+1) * sizeof(SyncPoint));
					for (j = 0; j < splo; j++) tmpsp[j] = syncState->syncPoints[j];
					for (j = syncState->numSyncPoints+1; j > splo; j--) 
						tmpsp[j] = syncState->syncPoints[j-1];
					tmpsp[splo] = tmpsp[splo+1] = syncState->syncPoints[splo];
					AOc_free(syncState->syncPoints); syncState->syncPoints = tmpsp;
				} else for (j = syncState->numSyncPoints+1; j > splo; j--) 
					syncState->syncPoints[j] = syncState->syncPoints[j-1];
				syncState->syncPoints[splo].id1 = inspoint->id1; syncState->syncPoints[splo].id2 = 0;
				syncState->syncPoints[splo].ts1 = inspoint->ts1; syncState->syncPoints[splo].ts2 = 0ll;
				syncState->syncPoints[splo].res = inspoint->res; syncState->syncPoints[splo].hash = 0x0; 
				syncState->numSyncPoints++;			
			}
		}		
	}
	return splo;
}

int AROObjectSynchronizerInf_processSyncPoint(AROObjectSynchronizerInf *syncState, SyncPoint *proc, int numts, uint64_t *ts, int stride, uint64_t *tsreq, int *numreq_ret, SyncPoint *newsyncpoints, int *num_newsyncpoints_ret) {
    if ((syncState == NULL) || (proc == NULL) || (ts == NULL)) return -1;
    if ((tsreq == NULL) || (numreq_ret == NULL)) return -1;
    if ((newsyncpoints == NULL) || (num_newsyncpoints_ret == NULL)) return -1;
	if (syncState->syncPoints == NULL) return -1;

#if AROObjectSynchronizer_C_debugVerbosity >= 1
	int debugLevel = syncState->debugLevel; 
	const char *debugLabel = (syncState->debugLabel == NULL) ? "Sync_C" : syncState->debugLabel;
#endif

#if AROObjectSynchronizer_C_debugVerbosity >= 1
	if (numts <= 0) AROLog_Print(logDEBUG4,0,debugLabel,"We have no entries - nothing to process in AROObjectSynchronizerInf_processSyncPoint\n");
#endif

	if (syncState->numSyncPoints < 0) {
		if (numts > 0) {
			SyncPoint tmppoint; tmppoint.res = syncState->localRes;
			tmppoint.id2 = 0; tmppoint.ts2 = 0ll; 
			tmppoint.id1 = 1; tmppoint.ts1 = *(ts + (tmppoint.id1-1)*stride); 
			AROObjectSynchronizerInf_insertSyncPoint(syncState, &tmppoint);
			if (numts > 1) {
				tmppoint.id1 = numts; tmppoint.ts1 = *(ts + (tmppoint.id1-1)*stride); 
				AROObjectSynchronizerInf_insertSyncPoint(syncState, &tmppoint);
			}
		} else if (proc->id1 != 1) return -1;
	}/*
	if (syncState->numSyncPoints < 0) {
>>>>>>> b1a020c39ce601c5ed08afb1b0aa99a8bc55f0ec
		SyncPoint tmppoint; tmppoint.res = syncState->localRes;
		tmppoint.id2 = 0; tmppoint.ts2 = 0ll; 
		tmppoint.id1 = 1; tmppoint.ts1 = *(ts + (tmppoint.id1-1)*stride); 		
		AROObjectSynchronizerInf_insertSyncPoint(syncState, &tmppoint);
		if (numts > 1) {
			tmppoint.id1 = numts; tmppoint.ts1 = *(ts + (tmppoint.id1-1)*stride); 		
			AROObjectSynchronizerInf_insertSyncPoint(syncState, &tmppoint);
		}
	}*/

    //if (syncState->numSyncPoints < 0) return -1;

    int i, j, numreq = 0, num_newsyncpoints = 0;
//    SyncPoint *syncpoints; syncpoints = syncState->syncPoints;

#if AROObjectSynchronizer_C_debugVerbosity >= 1
    if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"Processing sync point\n\t%d %08x %d %d %lld %lld\n\tfront: %d %lld back: %d %lld\n\n",
		proc->res, proc->hash, proc->id1, proc->id2, (long long int)proc->ts1, (long long int)proc->ts2, 
		AROObjectSynchronizerInf_findIDForTimestamp(proc->ts1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride),
		(long long int)AROObjectSynchronizerInf_findTimestampForID(proc->id1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride),
		AROObjectSynchronizerInf_findIDForTimestamp(proc->ts2, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride),
		(long long int)AROObjectSynchronizerInf_findTimestampForID(proc->id2, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride)
    );
#endif
    
#if AROObjectSynchronizer_C_debugVerbosity >= 2
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"Working with list of %d entries:\n", numts);
	for (i = 0; i < numts; i++) if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t%d : %lld\n", i+1, (long long int)(*(ts + i*stride)));
#endif

#if AROObjectSynchronizer_C_debugVerbosity >= 2
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"----------------- sync points at start -----------------\n");
	for (i = 0; i <= syncState->numSyncPoints; i++)
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t%d : %d %d %lld %lld\n", i, syncState->syncPoints[i].id1, syncState->syncPoints[i].id2, (long long int)syncState->syncPoints[i].ts1, (long long int)syncState->syncPoints[i].ts2);
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"--------------------------------------------------------\n");
#endif

	// merge info from the proc point directly with our global view
	SyncPoint insProc; insProc.res = proc->res; insProc.id2 = 0; insProc.ts2 = 0ll; insProc.hash = 0x0; 
	insProc.id1 = proc->id1; insProc.ts1 = proc->ts1; AROObjectSynchronizerInf_insertSyncPoint(syncState, &insProc);
	insProc.id1 = proc->id2; insProc.ts1 = proc->ts2; AROObjectSynchronizerInf_insertSyncPoint(syncState, &insProc);
	
    // sweep the sync state to update for new entries in the list
#if AROObjectSynchronizer_C_debugVerbosity >= 1
	if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"Sweeping %d syncpoints to update:\n", syncState->numSyncPoints);
#endif
	AROObjectSynchronizerInf_sweepToAnchor(syncState, ts, numts, stride, tsreq, &numreq);
	
#if AROObjectSynchronizer_C_debugVerbosity >= 2
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"----------------- sync points at mid1 -----------------\n");
	for (i = 0; i <= syncState->numSyncPoints; i++)
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t%d : %d %d %lld %lld\n", i, syncState->syncPoints[i].id1, syncState->syncPoints[i].id2, (long long int)syncState->syncPoints[i].ts1, (long long int)syncState->syncPoints[i].ts2);
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"--------------------------------------------------------\n");
#endif

#if AROObjectSynchronizer_C_debugVerbosity >= 1
	if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"Updated anchor points to sync point\n\t%d %08x %d %d %lld %lld\n\tfront: %d %lld back: %d %lld\n\n",
		proc->res, proc->hash, proc->id1, proc->id2, (long long int)proc->ts1, (long long int)proc->ts2, 
		AROObjectSynchronizerInf_findIDForTimestamp(proc->ts1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride),
		(long long int)AROObjectSynchronizerInf_findTimestampForID(proc->id1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride),
		AROObjectSynchronizerInf_findIDForTimestamp(proc->ts2, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride),
		(long long int)AROObjectSynchronizerInf_findTimestampForID(proc->id2, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride)
    );
#endif

	// global sync is aligned to local list -- process incoming syncpoint
	// begin processing the request - search for the limits
	int rqlo = 0, rqhi = numts-1; int sweeplo = -1, sweephi = -1;
	binaryReduceRangeWithKey(rqlo,rqhi,proc->ts1,*(ts+rqlo*stride),*(ts+rqhi*stride),*(ts+pivot*stride));
	if (rqhi < rqlo) { tsreq[numreq] = proc->ts1; numreq++; 
		SyncPoint tmp; tmp.res = syncState->localRes; tmp.ts2 = 0ll; tmp.id2 = 0;
		tmp.id1 = AROObjectSynchronizerInf_findIDForTimestamp(
			proc->ts1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
		tmp.ts1 = proc->ts1; if (proc->id1 > tmp.id1) tmp.id1 = proc->id1; 
		sweeplo = AROObjectSynchronizerInf_insertSyncPoint(syncState, &tmp);
	}

#if AROObjectSynchronizer_C_debugVerbosity >= 1
	if (rqhi < rqlo) { if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"rq %d : %lld\n", numreq, (long long int)tsreq[numreq-1]); }
	else { if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"found %lld in local list at %d %d (%lld %lld)!\n", (long long int)proc->ts1, 
		rqlo, rqhi, (long long int)*(ts+rqlo*stride), (long long int)*(ts+rqhi*stride)); }
#endif

	rqlo = 0; rqhi = numts-1; 
	binaryReduceRangeWithKey(rqlo,rqhi,proc->ts2,*(ts+rqlo*stride),*(ts+rqhi*stride),*(ts+pivot*stride));
	if (rqhi < rqlo) { tsreq[numreq] = proc->ts2; numreq++; 
		SyncPoint tmp; tmp.res = syncState->localRes; tmp.ts2 = 0ll; tmp.id2 = 0;
		tmp.id1 = AROObjectSynchronizerInf_findIDForTimestamp(
			proc->ts2, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
		tmp.ts1 = proc->ts2; if (proc->id2 > tmp.id1) tmp.id1 = proc->id2; 
		sweephi = AROObjectSynchronizerInf_insertSyncPoint(syncState, &tmp); 
	}

	if ((sweeplo >= 0) || (sweephi >= 0)) {
#if AROObjectSynchronizer_C_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"Sweepflags: %d %d - resweeping %d syncpoints to account for information from proc\n", sweeplo, sweephi, syncState->numSyncPoints);
#endif
		AROObjectSynchronizerInf_sweepToAnchor(syncState, ts, numts, stride, tsreq, &numreq);		
		if (syncState->numSyncPoints > 0) {
			if (sweeplo >= 0) {
				newsyncpoints[num_newsyncpoints].res = syncState->localRes;
				newsyncpoints[num_newsyncpoints].hash = 0x0;
				newsyncpoints[num_newsyncpoints].id1 = syncState->syncPoints[sweeplo].id1;
				newsyncpoints[num_newsyncpoints].id2 = syncState->syncPoints[sweeplo+1].id1;
				newsyncpoints[num_newsyncpoints].ts1 = syncState->syncPoints[sweeplo].ts1;
				newsyncpoints[num_newsyncpoints].ts2 = syncState->syncPoints[sweeplo+1].ts1;
				num_newsyncpoints++;
			}
			if (sweephi >= 0) {
				if (sweephi >= syncState->numSyncPoints) sweephi = syncState->numSyncPoints;
				newsyncpoints[num_newsyncpoints].res = syncState->localRes;
				newsyncpoints[num_newsyncpoints].hash = 0x0;
				newsyncpoints[num_newsyncpoints].id1 = syncState->syncPoints[sweephi-1].id1;
				newsyncpoints[num_newsyncpoints].id2 = syncState->syncPoints[sweephi].id1;
				newsyncpoints[num_newsyncpoints].ts1 = syncState->syncPoints[sweephi-1].ts1;
				newsyncpoints[num_newsyncpoints].ts2 = syncState->syncPoints[sweephi].ts1;
				num_newsyncpoints++;
			}			
		} else {
			newsyncpoints[num_newsyncpoints].res = syncState->localRes;
			newsyncpoints[num_newsyncpoints].hash = 0x0;
			newsyncpoints[num_newsyncpoints].id1 = syncState->syncPoints[0].id1;
			newsyncpoints[num_newsyncpoints].id2 = syncState->syncPoints[0].id1 + syncState->syncPoints[0].id2;
			newsyncpoints[num_newsyncpoints].ts1 = 
				newsyncpoints[num_newsyncpoints].ts2 = syncState->syncPoints[0].ts1;
			num_newsyncpoints++;
		}
	}		
		
#if AROObjectSynchronizer_C_debugVerbosity >= 1
	if (rqhi < rqlo) { if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"rq %d : %lld\n", numreq, (long long int)tsreq[numreq-1]); }
	else { if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"found %lld in local list at %d %d (%lld %lld)!\n", (long long int)proc->ts2, 
		rqlo, rqhi, (long long int)*(ts+rqlo*stride), (long long int)*(ts+rqhi*stride)); }
#endif

	int idforts1, idforts2; uint64_t tsforid1, tsforid2;
	tsforid1 = AROObjectSynchronizerInf_findTimestampForID(proc->id1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
	tsforid2 = AROObjectSynchronizerInf_findTimestampForID(proc->id2, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
	idforts1 = AROObjectSynchronizerInf_findIDForTimestamp(proc->ts1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
	idforts2 = AROObjectSynchronizerInf_findIDForTimestamp(proc->ts2, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);

	int flag = 0, syncdflag;
	if (
		(tsforid1 == proc->ts1) && (tsforid2 == proc->ts2) &&
		(idforts1 == proc->id1) && (idforts2 == proc->id2)
	) flag = 1;
	else {
		flag = 0;
		for (i = 0; i <= syncState->numSyncPoints; i++) {
			if ((proc->id1 == syncState->syncPoints[i].id1) && (proc->ts1 == syncState->syncPoints[i].ts1)) flag |= 0x1;
			if ((proc->id2 == syncState->syncPoints[i].id1) && (proc->ts2 == syncState->syncPoints[i].ts1)) flag |= 0x2;
			if (flag == 0x3) break;
		}
		if (flag == 0x3) flag = 2; else flag = 0;
	}
///////////////////////////////////////////////////		
	if (flag > 0) { syncdflag = 1;
#if AROObjectSynchronizer_C_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"This range is globally sync'd - flag %d!!\n", flag);
#endif
		flag = 0;
		if ((proc->id2 - proc->id1) > proc->res) flag = 1;
		//			else if (proc.id2 > (proc.id1 + 1)) {
		//				int ts1lo, ts1hi, ts2lo, ts2hi; ts1lo = ts2lo = 0; ts1hi = ts2hi = numts-1;
		//				binaryReduceRangeWithKey(ts1lo,ts1hi,proc.ts1,ts[ts1lo],ts[ts1hi],ts[pivot]);
		//				binaryReduceRangeWithKey(ts2lo,ts2hi,proc.ts2,ts[ts2lo],ts[ts2hi],ts[pivot]);
		//				if ((ts1hi < ts1lo) || (ts2hi < ts2lo)) flag = 2;
		//				else if ((proc.ts1 != proc.ts2) && ((ts2hi - ts2lo) != (proc.id2 - proc.id1))) flag = 3;
		//			}

		if (flag > 0) {
#if AROObjectSynchronizer_C_debugVerbosity >= 1
			if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\tabout to split -- %d %d %d %08x\n", proc->id1,proc->id2,proc->res,proc->hash); 
#endif			
//			int midid; moduloSplitRange(midid,proc->id1,proc->id2,proc->res);
			int midid; moduloSplitRange(midid,proc->id1,proc->id2,proc->res,numts);
			uint64_t midts = AROObjectSynchronizerInf_findTimestampForID(midid, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
#if AROObjectSynchronizer_C_debugVerbosity >= 1
			if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"\tsplitting (flag %d) range %d %d with res %d at %d %lld\n",
				flag, proc->id1, proc->id2, proc->res, midid, (long long int)midts);
#endif
			newsyncpoints[num_newsyncpoints] = newsyncpoints[num_newsyncpoints+1] = *proc;
			newsyncpoints[num_newsyncpoints].hash = newsyncpoints[num_newsyncpoints+1].hash = 0x0;
			newsyncpoints[num_newsyncpoints].id2 = newsyncpoints[num_newsyncpoints+1].id1 = midid;
			newsyncpoints[num_newsyncpoints].ts2 = newsyncpoints[num_newsyncpoints+1].ts1 = midts;
			num_newsyncpoints += 2;

			flag = 0;
			AROObjectSynchronizerInf_insertSyncPoint(syncState, newsyncpoints + num_newsyncpoints-1);
//			syncpoints = syncState->syncPoints;
        }		
	} else { syncdflag = 0;
#if AROObjectSynchronizer_C_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"Processing this range - front %d %lld back %d %lld\n", idforts1, (long long int)tsforid1, idforts2, (long long int)tsforid2);
#endif

#if AROObjectSynchronizer_C_debugVerbosity >= 2
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"----------------- sync points at mid2 -----------------\n");
		for (i = 0; i <= syncState->numSyncPoints; i++)
			if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t%d : %d %d %lld %lld\n", i, syncState->syncPoints[i].id1, syncState->syncPoints[i].id2, (long long int)syncState->syncPoints[i].ts1, (long long int)syncState->syncPoints[i].ts2);
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"--------------------------------------------------------\n");
#endif		
		
		uint64_t rqfts = AROObjectSynchronizerInf_findTimestampForID(idforts1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
		uint64_t rqbts = AROObjectSynchronizerInf_findTimestampForID(idforts2, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
		if ((tsforid1 < rqfts) || (rqfts == 0)) rqfts = tsforid1; if ((proc->ts1 < rqfts) || (rqfts == 0)) rqfts = proc->ts1; 
		if ((tsforid2 > rqbts) || (rqbts == 0)) rqbts = tsforid2; if ((proc->ts2 > rqbts) || (rqbts == 0)) rqbts = proc->ts2;

#if AROObjectSynchronizer_C_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"Span of interest: %lld -- %lld\n", (long long int)rqfts, (long long int)rqbts);
#endif		
		
		uint64_t tsPOI[4]; SyncPoint inspoint; 
		inspoint.res = syncState->localRes;
		inspoint.id2 = 0; inspoint.ts2 = 0ll; inspoint.hash = 0x0;
#if AROObjectSynchronizer_C_debugVerbosity >= 2
		int insloc; 
#endif
		for (i = 0; i < 4; i++) {
#if AROObjectSynchronizer_C_debugVerbosity >= 2
			if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"Processing this range - front %d %lld back %d %lld\n", idforts1, (long long int)tsforid1, idforts2, (long long int)tsforid2);
#endif
			switch (i) {
				case 0 : inspoint.id1 = idforts1; inspoint.ts1 = AROObjectSynchronizerInf_findTimestampForID(
					inspoint.id1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride); break;				
				case 1 : inspoint.id1 = idforts2; inspoint.ts1 = AROObjectSynchronizerInf_findTimestampForID(
					inspoint.id1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride); break;				
				case 2 : inspoint.ts1 = tsforid1; inspoint.id1 = AROObjectSynchronizerInf_findIDForTimestamp(
					inspoint.ts1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride); break;				
				case 3 : inspoint.ts1 = tsforid2; inspoint.id1 = AROObjectSynchronizerInf_findIDForTimestamp(
					inspoint.ts1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride); break;				
				default : break;
			} tsPOI[i] = inspoint.ts1;
#if AROObjectSynchronizer_C_debugVerbosity >= 1
			if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"i: %d inspoint: %d %lld\n", i, inspoint.id1, (long long int)inspoint.ts2);
#endif
#if AROObjectSynchronizer_C_debugVerbosity >= 2
			if (inspoint.id1 <= syncState->syncPoints[syncState->numSyncPoints].id1)
				insloc = AROObjectSynchronizerInf_insertSyncPoint(syncState, &inspoint);
//			syncpoints = syncState->syncPoints;
#else
			if (inspoint.id1 <= syncState->syncPoints[syncState->numSyncPoints].id1)
				AROObjectSynchronizerInf_insertSyncPoint(syncState, &inspoint);
#endif

#if AROObjectSynchronizer_C_debugVerbosity >= 2
			const char *insloclabel[4] = { "idforts1", "idforts2", "tsforid1", "tsforid2" };
			if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\tinsloc for %s = %d : %d %lld / %d %d %lld %lld %08x / %d %d %lld %lld %08x \n", 
				insloclabel[i], insloc, inspoint.id1, (long long int)inspoint.ts1, 
				syncState->syncPoints[insloc].id1, syncState->syncPoints[insloc].id2, (long long int)syncState->syncPoints[insloc].ts1, (long long int)syncState->syncPoints[insloc].ts2, syncState->syncPoints[insloc].hash, 
				syncState->syncPoints[insloc+1].id1, syncState->syncPoints[insloc+1].id2, (long long int)syncState->syncPoints[insloc+1].ts1, (long long int)syncState->syncPoints[insloc+1].ts2, syncState->syncPoints[insloc+1].hash
			);
#endif	
			
#if AROObjectSynchronizer_C_debugVerbosity >= 2
			int k;
			if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t----------------- sync points at tmp1 -----------------\n");
			for (k = 0; k <= syncState->numSyncPoints; k++)
				if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t\t%d : %08x %d %d %lld %lld\n", k, syncState->syncPoints[k].hash, syncState->syncPoints[k].id1, syncState->syncPoints[k].id2, (long long int)syncState->syncPoints[k].ts1, (long long int)syncState->syncPoints[k].ts2);
			if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t--------------------------------------------------------\n");
#endif		
		}

		int numtsPOI = 4; 
		for (i = 0; i < numtsPOI; i++) {
			if ((tsPOI[i] == rqfts) || (tsPOI[i] == rqbts)) {
				tsPOI[i] = tsPOI[numtsPOI-1]; numtsPOI--; i--; 
			} else { 
				for (j = i+1; j < numtsPOI; j++) {					
					if ((tsPOI[j] == tsPOI[i]) || (tsPOI[j] == rqfts) || (tsPOI[j] == rqbts)) { 
						tsPOI[j] = tsPOI[numtsPOI-1]; numtsPOI--; j--; 
					} else if (tsPOI[j] < tsPOI[i]) {
						uint64_t tmp = tsPOI[j]; tsPOI[j] = tsPOI[i]; tsPOI[i] = tmp; 
					}
				}
			}
		}
#if AROObjectSynchronizer_C_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"tsPOI: (%d) ", numtsPOI);
        for (j = 0; j < numtsPOI; j++) AROLog_Print(logDEBUG4,0,debugLabel," %lld", (long long int)tsPOI[j]); AROLog_Print(logDEBUG4,0,debugLabel,"\n");
#endif

#if AROObjectSynchronizer_C_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"Re-sweeping %d syncpoints to update before generating sync requests\n", syncState->numSyncPoints);
#endif
		AROObjectSynchronizerInf_sweepToAnchor(syncState, ts, numts, stride, tsreq, &numreq);

		if (numtsPOI > 0) {
			newsyncpoints[num_newsyncpoints].res = syncState->localRes;
			newsyncpoints[num_newsyncpoints].hash = 0x0;
			newsyncpoints[num_newsyncpoints].ts1 = rqfts;
			newsyncpoints[num_newsyncpoints].id1 = AROObjectSynchronizerInf_findIDForTimestamp(
				newsyncpoints[num_newsyncpoints].ts1, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
			for (i = 0; i < numtsPOI; i++) {
				newsyncpoints[num_newsyncpoints].ts2 = tsPOI[i];
				newsyncpoints[num_newsyncpoints].id2 = AROObjectSynchronizerInf_findIDForTimestamp(
					newsyncpoints[num_newsyncpoints].ts2, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
				num_newsyncpoints++;
				newsyncpoints[num_newsyncpoints].res = newsyncpoints[num_newsyncpoints-1].res;
				newsyncpoints[num_newsyncpoints].hash = newsyncpoints[num_newsyncpoints-1].hash;
				newsyncpoints[num_newsyncpoints].ts1 = newsyncpoints[num_newsyncpoints-1].ts2;
				newsyncpoints[num_newsyncpoints].id1 = newsyncpoints[num_newsyncpoints-1].id2;
			}
			newsyncpoints[num_newsyncpoints].ts2 = rqbts;
			newsyncpoints[num_newsyncpoints].id2 = AROObjectSynchronizerInf_findIDForTimestamp(
				newsyncpoints[num_newsyncpoints].ts2, syncState->numSyncPoints, syncState->syncPoints, numts, ts, stride);
			num_newsyncpoints++;	
		}
///////////////////////////////////////////////////	
	}
	
//#if AROObjectSynchronizer_C_debugVerbosity >= 1
//	AROLog_Print(logDEBUG4,0,"Sync_C","Final sweep to look for anchored but incorrect sub-resolution syncpoints - %d req\n", numreq);
//#endif
//	for (i = 0; i < syncState->numSyncPoints; i++) { 
//		if ((syncpoints[i+1].id1 - syncpoints[i].id1) < syncpoints[i].res) { j = 0; 
//			if (syncpoints[i].id1 > numts) break; 
//			if (*(ts + (syncpoints[i].id1-1)*stride) == syncpoints[i].ts1) j |= 0x1;
//			if (syncpoints[i+1].id1-1 <= numts) 
//				if (*(ts + (syncpoints[i+1].id1-1)*stride) == syncpoints[i].ts2) j |= 0x2;
//			AROLog_Print(logDEBUG4,0,"Sync_C","\t%d - %d %d %lld %lld - j %d\n", i, syncpoints[i].id1, syncpoints[i].id2, (long long int)(syncpoints[i].ts1), (long long int)(syncpoints[i].ts2), j);
//			if (j == 1) { 
//				int lim = (syncpoints[i+1].id1 > numts) ? numts : syncpoints[i+1].id1;
//				for (j = syncpoints[i].id1; j <= lim; j++) { 
//					AROLog_Print(logDEBUG4,0,"Sync_C","adding %lld\n", *(ts + (j-1)*stride));
//					tsreq[numreq] = *(ts + (j-1)*stride); numreq++; } 
//			}
//		} 
//	}
//#if AROObjectSynchronizer_C_debugVerbosity >= 1
//	AROLog_Print(logDEBUG4,0,"Sync_C","Finished final sweep with %d req\n", numreq);
//#endif

	// remove duplicate entries from ts requests
	for (i = 0; i < numreq; i++) { for (j = i+1; j < numreq; j++) {
		if (tsreq[j] == tsreq[i]) { tsreq[j] = tsreq[numreq-1]; numreq--; j--; } } }
	
#if AROObjectSynchronizer_C_debugVerbosity >= 2
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"----------------- sync points at end -------------------\n");
	for (i = 0; i <= syncState->numSyncPoints; i++)
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t%d : %08x %d %d %lld %lld\n", i, syncState->syncPoints[i].hash, syncState->syncPoints[i].id1, syncState->syncPoints[i].id2, (long long int)syncState->syncPoints[i].ts1, (long long int)syncState->syncPoints[i].ts2);
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"--------------------------------------------------------\n");

	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"----------------- ts reqs at end -----------------------\n");
	for (i = 0; i < numreq; i++)
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t%d : %lld\n", i, (long long int)tsreq[i]);
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"--------------------------------------------------------\n");

	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"----------------- sync reqs at end -----------------\n");	
	for (i = 0; i < num_newsyncpoints; i++)
		if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t%d : %08x %d %d %lld %lld\n", i, newsyncpoints[i].hash, newsyncpoints[i].id1, newsyncpoints[i].id2, (long long int)newsyncpoints[i].ts1, (long long int)newsyncpoints[i].ts2);
	if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"----------------------------------------------------\n");
#endif

#if AROObjectSynchronizer_C_debugVerbosity >= 1
	if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"End of AROObjectSynchronizerInf_processSyncPoint\n");
#endif
    *numreq_ret = numreq; *num_newsyncpoints_ret = num_newsyncpoints; return syncdflag;
}

void AROObjectSynchronizerInf_resolveLocalAlignment(
	AROObjectSynchronizerInf *syncState, int numts, uint64_t *ts, int stride, 
	uint64_t *tsreq, int *numreq_ret, SyncPoint *newsyncpoints, int *num_newsyncpoints_ret
) {
    if ((syncState == NULL) || (ts == NULL)) return;
    if ((tsreq == NULL) || (numreq_ret == NULL)) return;
    if ((newsyncpoints == NULL) || (num_newsyncpoints_ret == NULL)) return;

	int numreq = 0, num_newsyncpoints = 0;

#if AROObjectSynchronizer_C_debugVerbosity >= 1 
	int debugLevel = syncState->debugLevel; 
	const char *debugLabel = (syncState->debugLabel == NULL) ? "Sync_C" : syncState->debugLabel;
#else
	const char *debugLabel = "Sync_C";
#endif
    
	if (numts <= 0) {
		AROLog_Print(logDEBUG4,0,debugLabel,"We have no entries - nothing to process in AROObjectSynchronizerInf_resolveLocalAlignment\n");
		if (syncState->numSyncPoints > 0)
			AROLog_Print(logDEBUG4,0,debugLabel,"Generating requests for entries known about by syncPoints\n");			
		int k; for (k = 0; k <= syncState->numSyncPoints; k++) {
			tsreq[numreq] = syncState->syncPoints[k].ts1; numreq++;
		}
		*numreq_ret = numreq; *num_newsyncpoints_ret = num_newsyncpoints; return;
	}

#if AROObjectSynchronizer_C_debugVerbosity >= 1
	if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"Checking local alignment --\n");
//	AROLog_Print(logDEBUG4,0,"Sync_C","Sweeping %d syncpoints to update before checking local alignment\n", syncState->numSyncPoints);
#endif
//	AROObjectSynchronizerInf_sweepToAnchor(syncState, ts, numts, stride, tsreq, &numreq);
	
	int i, j, gid1, gid2, lid1 = -1, lid2 = -1;
	for (i = 0; i <= syncState->numSyncPoints; i++) { 
		int lo, hi;
		gid2 = syncState->syncPoints[i].id1; lo = 0; hi = numts - 1;
		binaryReduceRangeWithKey(lo,hi,syncState->syncPoints[i].ts1,*(ts + lo*stride),*(ts + hi*stride),*(ts + pivot*stride));
		if (hi < lo) { lid2 = -1; tsreq[numreq] = syncState->syncPoints[i].ts1; numreq++; } else lid2 = hi+1;
#if AROObjectSynchronizer_C_debugVerbosity >= 1
if (debugLevel >= 1) AROLog_Print(logDEBUG4,0,debugLabel,"chk i: %d %lld | %d %d | %d %d %d %d\n", i, (long long int)syncState->syncPoints[i].ts1, lo, hi, gid1, gid2, lid1, lid2);
#endif
		if ((i > 0) && (lid1 > 0) && (lid2 > 0)) {
#if AROObjectSynchronizer_C_debugVerbosity >= 2
			if (debugLevel >= 2) AROLog_Print(logDEBUG4,0,debugLabel,"\t%d : %d %d : %d %d : %lld %lld\n", i, gid1, gid2, lid1, lid2,  
				(long long int)syncState->syncPoints[i-1].ts1, (long long int)syncState->syncPoints[i].ts1
			);
#endif	
			if (((lid2 - lid1) < (gid2 - gid1)) && (i > 0)) {
				if ((gid2 - gid1) <= syncState->syncPoints[i].res) {
					newsyncpoints[num_newsyncpoints].id1 = newsyncpoints[num_newsyncpoints].id2 = 
					newsyncpoints[num_newsyncpoints].res = 0;
					newsyncpoints[num_newsyncpoints].ts1 = syncState->syncPoints[i-1].ts1;
					newsyncpoints[num_newsyncpoints].ts2 = syncState->syncPoints[i].ts1;
					num_newsyncpoints++;
				} else {
                    newsyncpoints[num_newsyncpoints].id1 = lid1;
                    newsyncpoints[num_newsyncpoints].id2 = lid2;
                    newsyncpoints[num_newsyncpoints].res = 0;
                    newsyncpoints[num_newsyncpoints].ts1 = syncState->syncPoints[i-1].ts1;
                    newsyncpoints[num_newsyncpoints].ts2 = syncState->syncPoints[i].ts1;
                    num_newsyncpoints++;
                }
			}
		}
		gid1 = gid2; lid1 = lid2;
	}
	
	// remove duplicate entries from ts requests
	for (i = 0; i < numreq; i++) { for (j = i+1; j < numreq; j++) {
		if (tsreq[j] == tsreq[i]) { tsreq[j] = tsreq[numreq-1]; numreq--; j--; } } }
	
	*numreq_ret = numreq; *num_newsyncpoints_ret = num_newsyncpoints;

//		if ((syncpoints[i+1].id1 - syncpoints[i].id1) < syncpoints[i].res) { j = 0; 
//			if (syncpoints[i].id1 > numts) break; 
//			if (*(ts + (syncpoints[i].id1-1)*stride) == syncpoints[i].ts1) j |= 0x1;
//			if (syncpoints[i+1].id1-1 <= numts) 
//				if (*(ts + (syncpoints[i+1].id1-1)*stride) == syncpoints[i].ts2) j |= 0x2;
//			AROLog_Print(logDEBUG4,0,"Sync_C","\t%d - %d %d %lld %lld - j %d\n", i, syncpoints[i].id1, syncpoints[i].id2, (long long int)(syncpoints[i].ts1), (long long int)(syncpoints[i].ts2), j);
//			if (j == 1) { 
//				int lim = (syncpoints[i+1].id1 > numts) ? numts : syncpoints[i+1].id1;
//				for (j = syncpoints[i].id1; j <= lim; j++) { 
//					AROLog_Print(logDEBUG4,0,"Sync_C","adding %lld\n", *(ts + (j-1)*stride));
//					tsreq[numreq] = *(ts + (j-1)*stride); numreq++; } 
//			}
//		} 
//	}
//#if AROObjectSynchronizer_C_debugVerbosity >= 1
//	AROLog_Print(logDEBUG4,0,"Sync_C","Finished final sweep with %d req\n", numreq);
//#endif
	
}

int AROObjectSynchronizerInf_compactSyncPoints(SyncPoint *spreqBuf, int numspreqBuf, int max_numspreqBuf, SyncPoint *spreq, int numspreq) {
	int i, j, flag1, flag2, tnumspreqBuf = numspreqBuf;
	for (i = 0; i < numspreq; i++) {
		if (tnumspreqBuf < max_numspreqBuf) { // we still have some room to add requests into the buffer
			flag1 = flag2 = 0;
			for (j = 0; j < tnumspreqBuf; j++) { 
				if ((spreq[i].id1 == spreqBuf[j].id1) && (spreq[i].ts1 == spreqBuf[j].ts1) && 
					(spreq[i].res >= spreqBuf[j].res)) flag1 = 1;
				if ((spreq[i].id2 == spreqBuf[j].id2) && (spreq[i].ts2 == spreqBuf[j].ts2) && 
					(spreq[i].res >= spreqBuf[j].res)) flag2 = 1;
				if ((flag1 == 1) && (flag2 == 1)) break;
			}
		    if ((flag1 != 1) || (flag2 != 1)) {
		        spreqBuf[tnumspreqBuf] = spreq[i]; tnumspreqBuf++; }			
		} else { // the buffer is full, we have to find someone to replace
			uint64_t oldestts = spreq[i].ts2; int oldesttsloc = -1;
			for (j = 0; j < tnumspreqBuf; j++) { 
				if (spreqBuf[j].ts2 < oldestts) {
					oldestts = spreqBuf[j].ts2; oldesttsloc = j;
				} else if (spreqBuf[j].ts2 == oldestts) {
					uint64_t tmpts1 = (oldesttsloc < 0) ? spreq[i].ts2 : spreqBuf[oldesttsloc].ts1;
					if (spreqBuf[j].ts1 < tmpts1) { 
						oldestts = spreqBuf[j].ts2; oldesttsloc = j; }
				}
			}
			if (oldesttsloc >= 0) spreqBuf[oldesttsloc] = spreq[i];
		}
	}
	return tnumspreqBuf;
}

int AROObjectSynchronizerInf_compactSyncPoints_v01(SyncPoint *spreqBuf, int numspreqBuf, SyncPoint *spreq, int numspreq) {
	int i, j, flag1, flag2, tnumspreqBuf = numspreqBuf;
	for (i = 0; i < numspreq; i++) {
		flag1 = flag2 = 0;
		for (j = 0; j < tnumspreqBuf; j++) { 
			if ((spreq[i].id1 == spreqBuf[j].id1) && (spreq[i].ts1 == spreqBuf[j].ts1) && 
				(spreq[i].res >= spreqBuf[j].res)) flag1 = 1;
			if ((spreq[i].id2 == spreqBuf[j].id2) && (spreq[i].ts2 == spreqBuf[j].ts2) && 
				(spreq[i].res >= spreqBuf[j].res)) flag2 = 1;
			if ((flag1 == 1) && (flag2 == 1)) break;
		}
	    if ((flag1 != 1) || (flag2 != 1)) {
	        spreqBuf[tnumspreqBuf] = spreq[i]; tnumspreqBuf++; }
	}
	return tnumspreqBuf;
} 

int AROObjectSynchronizerInf_checkAllSyncPoints(AROObjectSynchronizerInf *syncState, int numts, uint64_t *ts, int stride) {
	int i, flag = -1;
	for (i = 0; i <= syncState->numSyncPoints; i++) {
		if (((syncState->syncPoints + i)->id2 - (syncState->syncPoints + i)->id1) > (syncState->syncPoints + i)->res) flag = -2;
		if ((syncState->syncPoints + i)->id1 > numts) flag = -3;
		else if ((syncState->syncPoints + i)->id1 <= 0) {
            AROLog_Print(logERROR, 1, "Sync", "%d", (syncState->syncPoints + i)->id1);
            flag = -3;
        }
		else {
			if (*(ts + (((syncState->syncPoints + i)->id1-1) * stride)) != (syncState->syncPoints + i)->ts1) flag = -4;
		}
		if (i > 0) { if (((syncState->syncPoints + i-1)->id2 != 0) && ((syncState->syncPoints + i)->id1 != 
			((syncState->syncPoints + i-1)->id1 + (syncState->syncPoints + i-1)->id2)) ) flag = -5;
		}
		if (flag < -1) break;
	}
	
	return flag;
}
