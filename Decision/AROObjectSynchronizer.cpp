//
//  AROObjectSynchronizer.cpp
//  ApolloLinuxClient
//
//  Created by Adam Kinsman on 2013-08-20.
//  Copyright (c) 2013 Arroware. All rights reserved.
//

#include <stdio.h>

#include "AROUtil_C.h"
#include "AROObjectCommonDefines_C.h"

#include "AROLog.h"
#include "AROObjectSynchronizer.h"

AROObjectSynchronizer::AROObjectSynchronizer(uint64_t *syncIDhead, uint32_t *syncHashhead, int stride, AROSyncResponder *theDelegate) :	
	delegate(theDelegate)
{
	syncPhase = tAROObjectSynchronizerPhase_idle;
    syncState = AROObjectSynchronizerInf_init();
	syncIDhead = NULL; syncHashhead = NULL;
	this->setSyncIDArrays(syncIDhead,syncHashhead,stride);
    localSyncRes = syncState->localRes;
    
    lastProcessTime = 0ll;
	setNetworkPeriod(20.0);
	numspinBuf = numtsreqBuf = numspreqBuf = 0;
	headspinBuf = headtsreqBuf = headspreqBuf = 0;

#if AROObjectSynchronizer_debugVerbosity >= 1
//	setDebugInfo(0, "Sync- ");
	setDebugInfo(0, " ");
//	debugLevel = 0; sprintf(debugLabel, debugLabel);
#endif
}

AROObjectSynchronizer::~AROObjectSynchronizer() { 
#if AROObjectSynchronizer_debugVerbosity >= 1
	if (debugLevel >= 1) AROLog::Log()->Print(logDEBUG, 1, debugLabel, "HEREA\n");
#endif
	AROObjectSynchronizerInf_free(syncState);
#if AROObjectSynchronizer_debugVerbosity >= 1
	if (debugLevel >= 1) AROLog::Log()->Print(logDEBUG, 1, debugLabel, "HEREB\n");
#endif
}

void AROObjectSynchronizer::setDebugInfo(int debugLevel_, const char *debugLabel_) {
	AROObjectSynchronizerInf_setDebugInfo(syncState, debugLevel_, debugLabel_);
#if AROObjectSynchronizer_debugVerbosity >= 1
	if (debugLevel_ >= 0) debugLevel = debugLevel_;
	if (debugLabel_ != NULL) snprintf(debugLabel, 30, "Sync-%s", debugLabel_);
#endif
}

void AROObjectSynchronizer::setNetworkPeriod(double value) { networkPeriod = (uint64_t)(value * 1e6); }

void AROObjectSynchronizer::setSyncIDArrays(uint64_t *syncIDhead, uint32_t *syncHashhead, int stride) {
	if (syncIDhead != NULL) { syncIDptr = syncIDhead; syncIDstride = stride; } 
	if (syncHashhead != NULL) syncHashptr = syncHashhead;	
}

void AROObjectSynchronizer::getGlobalSyncPoint(SyncPoint *syncPoint, int numts) {	
	if (syncPoint == NULL) return;
    syncPoint->res = syncState->localRes;
    if ((numts > 0) && (syncIDptr != NULL)) {
		syncPoint->id1 = 1; syncPoint->id2 = numts;
	    syncPoint->ts1 = *(syncIDptr + (syncPoint->id1-1) * syncIDstride);
	    syncPoint->ts2 = *(syncIDptr + (syncPoint->id2-1) * syncIDstride);
		syncPoint->hash = 0x0; 
	} else { syncPoint->id1 = syncPoint->id2 = 0; syncPoint->ts1 = syncPoint->ts2 = 0ll; syncPoint->hash = 0x0; }
}

void AROObjectSynchronizer::processSyncState(uint64_t *syncIDhead, uint32_t *syncHashhead, int stride, int numts) {
#if AROObjectSynchronizer_debugVerbosity >= 1
    if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"Sync called over %p,%p (%p,%p) %d p %d sp %d spbuf %d\n", 
		syncIDhead, syncIDptr, syncHashhead, syncHashptr, numts, syncPhase, syncState->numSyncPoints, numspinBuf);
#endif

	setSyncIDArrays(syncIDhead, syncHashhead, stride);
	if (syncIDptr == NULL) return;

	int i, j;
    uint64_t tsreq[1000]; int numtsreq = 0;
    SyncPoint spreq[1000]; int numspreq = 0;	

	if ((numts <= 0) && (syncState->numSyncPoints <= 0) && (numspinBuf == 0)) { 
		lastProcessTime = 0ll; syncPhase = tAROObjectSynchronizerPhase_aggregate;
	} else if (syncState->numSyncPoints == 0) {
		this->getGlobalSyncPoint(syncState->syncPoints, numts);

 		if (numts > 0) {
			syncState->numSyncPoints = 1;
			syncState->syncPoints[1].res = syncState->syncPoints[0].res; 
			syncState->syncPoints[1].id1 = syncState->syncPoints[0].id2; 
			syncState->syncPoints[1].ts1 = syncState->syncPoints[0].ts2; 
			syncState->syncPoints[1].id2 = syncState->syncPoints[0].id2 = 0; 
			syncState->syncPoints[1].ts2 = syncState->syncPoints[0].ts2 = 0ll; 
			syncState->syncPoints[1].hash = syncState->syncPoints[0].hash = 0x0; 
		} else if (numts == 0) {
			syncState->numSyncPoints = 0; syncState->syncPoints[0].hash = 0x0; 
			syncState->syncPoints[0].id2 = 0; syncState->syncPoints[0].ts2 = 0ll; 
		} else syncState->numSyncPoints = -1;

        syncPhase = tAROObjectSynchronizerPhase_partition;	
//        syncPhase = tAROObjectSynchronizerPhase_idle;
	}

	if (syncPhase == tAROObjectSynchronizerPhase_idle) {
        // initiate the sync process
        this->getGlobalSyncPoint(spreq, numts); numspreq = 1;
#if AROObjectSynchronizer_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"Sync initiated with broadcast : %d %d %d %lld %lld %08x\n", spreq->res, spreq->id1, spreq->id2, spreq->ts1, spreq->ts2, spreq->hash);
#endif
        syncPhase = tAROObjectSynchronizerPhase_partition;
    } else if (syncPhase == tAROObjectSynchronizerPhase_partition) {
#if AROObjectSynchronizer_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"Processing batch of syncpoints - pre compact - %d to %d (%d)\n", headspinBuf, headspinBuf + numspinBuf - 1, (headspinBuf + numspinBuf - 1) % AROObjectSynchronizer_reqBufferSize);
#endif

		// compact the batch of incoming syncpoints to be processed
		for (i = headspinBuf; i < headspinBuf + numspinBuf; i++) {
			SyncPoint *basePoint; basePoint = spinBuf + (i % AROObjectSynchronizer_reqBufferSize);
			for (j = i+1; j < headspinBuf + numspinBuf; j++) {
				SyncPoint *chkPoint; chkPoint = spinBuf + (j % AROObjectSynchronizer_reqBufferSize);
				if (
					(basePoint->ts1 == chkPoint->ts1) && (basePoint->ts2 == chkPoint->ts2) && 
					(basePoint->res == chkPoint->res) && (basePoint->id1 == chkPoint->id1) && (basePoint->id2 == chkPoint->id2)
				) { int k; // duplicated sp, bump down
#if AROObjectSynchronizer_debugVerbosity >= 1
					if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"Drop from batch: (%d) %d %d %d %lld %lld %08x == (%d) %d %d %d %lld %lld %08x\n",
						i, basePoint->res, basePoint->id1, basePoint->id2, basePoint->ts1, basePoint->ts2, basePoint->hash, j, chkPoint->res, chkPoint->id1, chkPoint->id2, chkPoint->ts1, chkPoint->ts2, chkPoint->hash);
#endif
					for (k = j+1; k < headspinBuf + numspinBuf; k++) {
						*(spinBuf + ((k-1) % AROObjectSynchronizer_reqBufferSize)) = 
							*(spinBuf + (k % AROObjectSynchronizer_reqBufferSize));
					}
					numspinBuf--; j--;
				}
			}
		}

		int start = headspinBuf; int end = headspinBuf + numspinBuf - 1, flag = 1;
#if AROObjectSynchronizer_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"Processing batch of syncpoints - post compact - %d to %d (%d)\n", start, end, end % AROObjectSynchronizer_reqBufferSize);
#endif
		
#if AROObjectSynchronizer_debugVerbosity >= 1
		if (debugLevel >= 2) {
	        AROLog_Print(logINFO,0,debugLabel,"------------------ sync points (cpp) at start ------------------\n");
	        for (i = 0; i <= syncState->numSyncPoints; i++)
	            AROLog_Print(logINFO,0,debugLabel,"\t%d : %d %d %d %lld %lld\n", i, 
					(syncState->syncPoints + i)->res, (syncState->syncPoints + i)->id1, (syncState->syncPoints + i)->id2, 
					(long long int)((syncState->syncPoints + i)->ts1), (long long int)((syncState->syncPoints + i)->ts2));
	        AROLog_Print(logINFO,0,debugLabel,"--------------------------------------------------------\n");		
		}
#endif

		while (start <= end) {			
			SyncPoint *procPoint; procPoint = spinBuf + (start % AROObjectSynchronizer_reqBufferSize);
#if AROObjectSynchronizer_debugVerbosity >= 1
			if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"going to process syncpoint %d %d %d %lld %lld %08x over %d items\n", procPoint->res, procPoint->id1, procPoint->id2, procPoint->ts1, procPoint->ts2, procPoint->hash, numts);
#endif
			flag &= AROObjectSynchronizerInf_processSyncPoint(syncState, procPoint, numts, syncIDptr, syncIDstride, tsreq, &numtsreq, spreq, &numspreq);
			start++; headspinBuf = (headspinBuf == AROObjectSynchronizer_reqBufferSize-1) ? 0 : headspinBuf+1; numspinBuf--;
#if AROObjectSynchronizer_debugVerbosity >= 1
			if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"processed syncpoint (%d to %d) %d %d %d %lld %lld %08x over %d items to %d requests and %d syncpoints\n", 
				start, end, procPoint->res, procPoint->id1, procPoint->id2, procPoint->ts1, procPoint->ts2, procPoint->hash, numts, numtsreq, numspreq);
#endif
			// compact updates to the requests for sync IDs
		    for (i = 0; i < numtsreq; i++) {
		        for (j = 0; j < numtsreqBuf; j++) {
		            if (tsreqBuf[j] == tsreq[i]) break; }
		        if (j == numtsreqBuf) {
		            tsreqBuf[numtsreqBuf] = tsreq[i]; numtsreqBuf++; }
		    }

			// compact updates to the requests for sync points
			for (i = 0; i < numspreq; i++) spreq[i].res = localSyncRes;
			numspreqBuf = AROObjectSynchronizerInf_compactSyncPoints(spreqBuf, numspreqBuf, AROObjectSynchronizer_reqBufferSize, spreq, numspreq);
		}
#if AROObjectSynchronizer_debugVerbosity >= 1
		if (debugLevel >= 1) AROLog_Print(logINFO,0,debugLabel,"overall sync'd flag: %d\n", flag);
#endif

		if (flag == 1) { 
			AROObjectSynchronizerInf_resolveLocalAlignment(
				syncState, numts, syncIDptr, syncIDstride, tsreq, &numtsreq, spreq, &numspreq);
#if AROObjectSynchronizer_debugVerbosity >= 1
			if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"%d new requests from local alignment\n", numtsreq + numspreq);
#endif
			// compact updates to the requests for sync IDs
		    for (i = 0; i < numtsreq; i++) {
				for (j = 0; j < numtsreqBuf; j++) {
					if (tsreqBuf[j] == tsreq[i]) break; }
				if (j == numtsreqBuf) {
					tsreqBuf[numtsreqBuf] = tsreq[i]; numtsreqBuf++; }
		    }

			// compact updates to the requests for sync points
			for (i = 0; i < numspreq; i++) spreq[i].res = localSyncRes;
			numspreqBuf = AROObjectSynchronizerInf_compactSyncPoints(spreqBuf, numspreqBuf, AROObjectSynchronizer_reqBufferSize, spreq, numspreq);
		}

#if AROObjectSynchronizer_debugVerbosity >= 1
		if (debugLevel >= 2) {
			AROLog_Print(logINFO,0,debugLabel,"------------------ sync points (cpp) at end ------------------\n");
			for (i = 0; i <= syncState->numSyncPoints; i++)
				AROLog_Print(logINFO,0,debugLabel,"\t%d : %d %d %d %lld %lld\n", i, 
					(syncState->syncPoints + i)->res, (syncState->syncPoints + i)->id1, (syncState->syncPoints + i)->id2, 
					(long long int)((syncState->syncPoints + i)->ts1), (long long int)((syncState->syncPoints + i)->ts2));
			AROLog_Print(logINFO,0,debugLabel,"--------------------------------------------------------\n");
		}
#endif

		if ((lastProcessTime == 0ll) || (AOc_localTimestamp() > (lastProcessTime + networkPeriod))) {
#if AROObjectSynchronizer_debugVerbosity >= 1
			if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"Sending out sync directives (%d,%d) at time %lld\n", numtsreqBuf, numspreqBuf, AOc_localTimestamp());
#endif
			SyncPoint tsreqsp;
			for (i = 0; i < numtsreqBuf; i++) { tsreqsp.res = 3;
				tsreqsp.id1 = tsreqsp.id2 = 0; tsreqsp.ts1 = tsreqsp.ts2 = tsreqBuf[i];
				if (delegate) delegate->sendRequestForSyncPoint(&tsreqsp, this);
			} numtsreqBuf = 0;

			if (numspreqBuf == 0) {
				this->getGlobalSyncPoint(spreqBuf, numts); numspreqBuf = 1;
#if AROObjectSynchronizer_debugVerbosity >= 1
				if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"Creating default sync because of empty reply : %d %d %d %lld %lld %08x\n", 
					spreqBuf->res, spreqBuf->id1, spreqBuf->id2, spreqBuf->ts1, spreqBuf->ts2, spreqBuf->hash);
#endif
			}
	
			for (i = 0; i < numspreqBuf; i++) {
				if (checkMalformedSyncPoint(spreqBuf + i)) {
#ifdef ACT_AS_PRIORITY_PEER		
					AROLog::Log()->Print(logERROR,1,"SYNC-ERROR","Synchronizer about to send request for malformed syncPoint : %d %d %d %lld %lld\n", 
						(spreqBuf + i)->res, (spreqBuf + i)->id1, (spreqBuf + i)->id2, (long long int)((spreqBuf + i)->ts1), (long long int)((spreqBuf + i)->ts2));
#endif
				} else { if (delegate) delegate->sendRequestForSyncPoint(spreqBuf + i, this); }
			} numspreqBuf = 0;

			lastProcessTime = AOc_localTimestamp();
		} else {
#if AROObjectSynchronizer_debugVerbosity >= 1
			if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"Skipping sync directives (%d,%d) at time %lld - last send %lld\n", 
				numtsreqBuf, numspreqBuf, AOc_localTimestamp(), lastProcessTime);
#endif
		}
		
//		int pivotSyncPoint = AROObjectSynchronizerInf_checkAllSyncPoints(syncState);
		int pivotSyncPoint = AROObjectSynchronizerInf_checkAllSyncPoints(syncState, numts, syncIDptr, syncIDstride);
		if (pivotSyncPoint == -1) {
#if AROObjectSynchronizer_debugVerbosity >= 1
			if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"List has been successfully synchronized at time %lld!\n", AOc_localTimestamp());
#endif
			if (syncState->numSyncPoints > 0) {
				if (delegate) delegate->notificationOfSyncAchieved((double)networkPeriod / 1e6, this);
			}
			syncPhase = tAROObjectSynchronizerPhase_aggregate;
		}
#if AROObjectSynchronizer_debugVerbosity >= 1
		else {
			if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"List not yet sync'd - checkAllSyncPoints returned %d at time %lld!\n", pivotSyncPoint, AOc_localTimestamp());
		}			
#endif		
	} else if (syncPhase == tAROObjectSynchronizerPhase_aggregate) {
		this->getGlobalSyncPoint(syncState->syncPoints, numts);

		if ((lastProcessTime == 0ll) || (AOc_localTimestamp() > (lastProcessTime + networkPeriod))) {
#if AROObjectSynchronizer_debugVerbosity >= 1
	        if (debugLevel >= 1) AROLog::Log()->Print(logINFO,1,debugLabel,"Sending out global sync directive at time %lld\n", AOc_localTimestamp());
#endif
			if (delegate) delegate->sendRequestForSyncPoint(syncState->syncPoints, this);
			lastProcessTime = AOc_localTimestamp();
		}

 		if (numts > 0) {
			syncState->numSyncPoints = 1;
			syncState->syncPoints[1].res = syncState->syncPoints[0].res; 
			syncState->syncPoints[1].id1 = syncState->syncPoints[0].id2; 
			syncState->syncPoints[1].ts1 = syncState->syncPoints[0].ts2; 
			syncState->syncPoints[1].id2 = syncState->syncPoints[0].id2 = 0; 
			syncState->syncPoints[1].ts2 = syncState->syncPoints[0].ts2 = 0ll; 
			syncState->syncPoints[1].hash = syncState->syncPoints[0].hash = 0x0; 
		} else if (numts == 0) {
			syncState->numSyncPoints = 0; syncState->syncPoints[0].hash = 0x0; 
			syncState->syncPoints[0].id2 = 0; syncState->syncPoints[0].ts2 = 0ll; 
		} else syncState->numSyncPoints = -1;
		
		syncPhase = tAROObjectSynchronizerPhase_idle;
	}
}

void AROObjectSynchronizer::notifyOfSyncPoint(SyncPoint *syncPoint) {
	if (checkMalformedSyncPoint(syncPoint)) {
#ifdef ACT_AS_PRIORITY_PEER		
		AROLog::Log()->Print(logERROR,1,"SYNC-ERROR","Synchronizer was notified with malformed syncPoint : %d %d %d %lld %lld\n", 
			syncPoint->res, syncPoint->id1, syncPoint->id2, (long long int)(syncPoint->ts1), (long long int)(syncPoint->ts2));
#endif
		return;
	}
	
	int loc = (headspinBuf+numspinBuf) % AROObjectSynchronizer_reqBufferSize;
#if AROObjectSynchronizer_debugVerbosity >= 1
	if (debugLevel >= 1) AROLog::Log()->Print(logDEBUG1,1,debugLabel,"Got notification of sync point %d %d %d %lld %lld %08x into loc %d\n", 
		syncPoint->res, syncPoint->id1, syncPoint->id2, syncPoint->ts1, syncPoint->ts2, syncPoint->hash, loc);
#endif
	if (numspinBuf < AROObjectSynchronizer_reqBufferSize) {
		spinBuf[loc] = *syncPoint; numspinBuf++;
		if (syncPhase == tAROObjectSynchronizerPhase_partition) lastProcessTime = 0ll;		
	} 
#if AROObjectSynchronizer_debugVerbosity >= 1
	else { if (debugLevel >= 1) AROLog::Log()->Print(logDEBUG1,1,debugLabel,"Buffer overrun to %d at loc %d notifying of sync point %d %d %d %lld %lld %08x\n", 
		numspinBuf, loc, syncPoint->res, syncPoint->id1, syncPoint->id2, syncPoint->ts1, syncPoint->ts2, syncPoint->hash); }
#endif
#if AROObjectSynchronizer_debugVerbosity >= 1
	if (debugLevel >= 1) AROLog::Log()->Print(logDEBUG1,1,debugLabel,"Next scheduled batch at %lld (%lld + %lld)\n", 
		(long long int)(lastProcessTime + networkPeriod), (long long int)lastProcessTime, (long long int)networkPeriod);
#endif
}
/*
void AROObjectSynchronizer::notifyOfUpdatedSyncID(uint64_t syncID) {
	int loc = (headtsinBuf+numtsinBuf) % AROObjectSynchronizer_reqBufferSize;
#if AROObjectSynchronizer_debugVerbosity >= 1
	if (debugLevel >= 1) AROLog::Log()->Print(logDEBUG1,1,debugLabel,"Got notification for updated syncID %lld into loc %d\n", syncID, loc);
#endif
	if (numtsinBuf < AROObjectSynchronizer_reqBufferSize) { tsinBuf[loc] = syncID; numtsinBuf++; }
#if AROObjectSynchronizer_debugVerbosity >= 1
	else { if (debugLevel >= 1) AROLog::Log()->Print(logDEBUG1,1,debugLabel,"Buffer overrun to %d at loc %d notifying of sync ID %lld\n", numspinBuf, loc, syncID); }
#endif
}*/

bool AROObjectSynchronizer::checkMalformedSyncPoint(SyncPoint *syncPoint) {
	if ( // checking for malformed syncpoints - consider if malformed if:
		(syncPoint->id1 > syncPoint->id2) ||                    // invalid ID order
		(syncPoint->id1 < 0) || (syncPoint->id2 < 0) ||         // invalid lo ID
		(syncPoint->id1 > 65536) || (syncPoint->id2 > 65536) || // invalid hi ID
		( (!((syncPoint->id1 == 0) && (syncPoint->id2 == 0))) && (                        // it is not a request and it is:                              
			(syncPoint->ts1 < 1262304000000000) || (syncPoint->ts2 < 1262304000000000) || // before jan 1 2010 or
			(syncPoint->ts1 > 1767225599000000) || (syncPoint->ts2 > 1767225599000000)    // after  dec 31 2025
		))                    
	) { return true; } else { return false; }
}
