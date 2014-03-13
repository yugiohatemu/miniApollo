//
//  action_C.h
//  Decision
//
//  Created by Yue Huang on 2014-03-11.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__action_C__
#define __Decision__action_C__

#include <inttypes.h>
#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct Action_S {
    uint64_t ts;
    uint32_t hash;
    //also a pointer to the header in the future
} __attribute__((aligned(8))) Action_C;

#ifdef __cplusplus
}
#endif
#endif /* defined(__Decision__action_C__) */
