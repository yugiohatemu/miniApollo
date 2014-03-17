//
//  AROLog.h
//  ApolloPriorityPeer
//
//  Created by Henry Ko on 2013-05-23.
//  Copyright (c) 2013 Arroware. All rights reserved.
//

#ifndef __ApolloPriorityPeer__AROLog__
#define __ApolloPriorityPeer__AROLog__

#include "AROLog_C.h"
#include <stdarg.h>
#include <iostream>

class AROLog {
private:
    std::string current_log_date;
    std::string current_log_name;
    std::string log_suffix;
    std::string path;
    FILE *fp;
    enum LogLevel current_level;

    AROLog(){};
    ~AROLog(){};
    AROLog(AROLog const&){};
    
public:
    static AROLog& Log();

    void OpenLog(std::string path, std::string log_name);
    void CloseLog();
    void Print(int level, int timed, const char *tag, const char* format, ...);
    void vPrint(int level, int timed, const char *tag, const char *format, va_list args);
    void SetLogLevel(int level);
    int GetLogLevel();
    std::string GetDate();
    std::string GetTime();
    
private:
    void CheckLogDate();
    const std::string ToString(int level);
    std::string GetLogFilePrefix();
};

#endif /* defined(__ApolloPriorityPeer__AROLog__) */
