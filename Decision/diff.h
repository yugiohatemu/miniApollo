//
//  diff.h
//  Decision
//
//  Created by Yue Huang on 2014-03-26.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__diff__
#define __Decision__diff__

#include <inttypes.h>
#include <vector>
#include "action_C.h"

class Diff{
    Diff(){};
    ~Diff(){};
    Diff(Diff const&){};
    std::vector<uint64_t> synced_bb;
    std::vector<uint64_t> unsynced_bb;
public:
    static Diff& get();
    void set_synced_bb(BackBundle_C * bb);
    void set_unsynced_actions(BackBundle_C * bb);
    void out_put_diff();
    bool check_hash(int bb_lo, int bb_hi, int a_lo, Action_C * action_list);
    //get a whole list at once
};

#endif /* defined(__Decision__diff__) */
