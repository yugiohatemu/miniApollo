//
//  AROObjectSynchronizer.h
//  ApolloLinuxClient
//
//  Created by Adam Kinsman on 2013-08-20.
//  Copyright (c) 2013 Arroware. All rights reserved.
//

#ifndef __ApolloLinuxClient__AROObjectSynchronizer__
#define __ApolloLinuxClient__AROObjectSynchronizer__

#include <iostream>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "AROProtocols.h"
#include "AROObjectSynchronizer_C.h"

#define AROObjectSynchronizer_debugVerbosity 2
#ifdef ACT_AS_PRIORITY_PEER
#define AROObjectSynchronizer_reqBufferSize 65536
#else
#define AROObjectSynchronizer_reqBufferSize 512
#endif

class AROObjectSynchronizer {

private:
	uint64_t *syncIDptr; uint32_t *syncHashptr;
	int syncIDstride, localSyncRes;
    AROObjectSynchronizerInf *syncState;
	AROObjectSynchronizerPhaseType syncPhase;
#if AROObjectSynchronizer_debugVerbosity >= 1
	int debugLevel; char debugLabel[31];
#endif

	uint64_t lastProcessTime, networkPeriod;
	//uint64_t tsinBuf[AROObjectSynchronizer_reqBufferSize]; int headtsinBuf, numtsinBuf;
	SyncPoint spinBuf[AROObjectSynchronizer_reqBufferSize]; int headspinBuf, numspinBuf;
	uint64_t tsreqBuf[AROObjectSynchronizer_reqBufferSize]; int headtsreqBuf, numtsreqBuf;
	SyncPoint spreqBuf[AROObjectSynchronizer_reqBufferSize]; int headspreqBuf, numspreqBuf;

public:
	AROObjectSynchronizer(uint64_t *syncIDhead, uint32_t *syncHashhead, int stride, AROSyncResponder *theDelegate);
	
	~AROObjectSynchronizer();
	AROSyncResponder *delegate;

	void setDebugInfo(int debugLevel_, const char *debugLabel_);
	void setNetworkPeriod(double value);

	void setSyncIDArrays(uint64_t *syncIDhead, uint32_t *syncHashhead, int stride);
	void getGlobalSyncPoint(SyncPoint *syncPoint, int numts);
	void processSyncState(uint64_t *syncIDhead, uint32_t *syncHashhead, int stride, int numts);

	void notifyOfSyncPoint(SyncPoint *syncPoint);
	//void notifyOfUpdatedSyncID(uint64_t syncID);
	bool checkMalformedSyncPoint(SyncPoint *syncPoint);
};

#endif /* defined(__ApolloLinuxClient__AROObjectSynchronizer__) */
