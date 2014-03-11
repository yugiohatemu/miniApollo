//
//  AROProtocols.h
//  apollo-ios-core-test
//
//  Created by Phil Kinsman on 2013-05-24.
//  Copyright (c) 2013 Arroware Industries Inc. All rights reserved.
//

#ifndef apollo_ios_core_test_AROProtocols_h
#define apollo_ios_core_test_AROProtocols_h

#include <inttypes.h>
#include <vector>
//#include "AROObjectParsing_C.h"
//#include "AROObjectSynchronizer_C.h"
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

class AROConnection;
class AROid;
struct AOc_BlockPayload_s;
struct SyncPoint_s;
class AROBundleLoader;

class AROBundleLoaderLocalDelegate {
public:
	virtual bool dataReadyOnLoader(boost::shared_ptr<AROBundleLoader>) = 0;
    virtual void error(boost::shared_ptr<AROBundleLoader>) = 0;
};

class ARODataDelegate {
public:
	virtual void processMessage(void *buf, int length, boost::shared_ptr<AROConnection>src) = 0;
//    virtual void formatReqDescriptorToBuf(void *packet, int maxlen) = 0;
};

class AROLocalResponder {
public:
//    virtual struct AOc_BlockPayload_s *getBlockPayload() = 0;
//    virtual void replaceBlockPayloadWith(struct AOc_BlockPayload_s *blockPayload) = 0;
//    virtual void mergeBlockPayload(struct AOc_BlockPayload_s *blkpyld, struct AOc_BlockPayload_s *matchKey) = 0;
//    virtual struct AOc_BlockPayload_s *matchBlockPayload(struct AOc_BlockPayload_s *matchKey) = 0;
};

class AROSyncResponder {
public:
	virtual void sendRequestForSyncPoint(struct SyncPoint_s *syncPoint, void *sender) = 0;
	virtual void notificationOfSyncAchieved(double networkPeriod, void *sender) = 0;
};

#endif

