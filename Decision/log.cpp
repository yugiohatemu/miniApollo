//
//  log.cpp
//  Decision
//
//  Created by Yue Huang on 2014-03-03.
//  Copyright (c) 2014 Yue Huang. All rights reserved.
//

#include "log.h"

Log& Log::log(){
    static Log m_instance;
    return m_instance;
}

void Log::Print(const char* format, ...){
    boost::mutex::scoped_lock lock(mutex);
    
    va_list argList;
    va_start(argList, format);
    vprintf(format, argList);
    va_end(argList);
}

