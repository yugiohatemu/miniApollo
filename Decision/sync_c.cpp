//
//  sync_c.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-04.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "sync_c.h"
#include <algorithm>

void sync_c(std::vector<uint64_t>& source, std::vector<uint64_t> &target){

    std::vector<uint64_t> diff;
    for (unsigned int i = 0; i < target.size(); i++) {
        bool found = std::binary_search(source.begin(), source.end(), target[i]);
        if (!found) diff.push_back(target[i]);
        
    }
    
    //since we are modifying synchronizer
    if (!diff.empty()) {
        for (int i = 0; i < diff.size(); i++) source.push_back(diff[i]);
        std::sort(source.begin(), source.end());
    }
}