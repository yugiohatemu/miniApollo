//
//  log.h
//  Decision
//
//  Created by Yue Huang on 2014-03-03.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#ifndef __Decision__log__
#define __Decision__log__

#include <iostream>
#include <boost/thread/mutex.hpp>
#include <stdarg.h>

class Log{
    Log(){};
    ~Log(){};
    Log(const Log & L){};
    boost::mutex mutex;
    
public:
    static Log& log();
    void Print(const char* format, ...);
};

#endif /* defined(__Decision__log__) */
