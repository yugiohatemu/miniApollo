//
//  AROLogInterface.h
//  ApolloLinuxClient
//
//  Created by Phil Kinsman on 2014-03-23.
//  Copyright (c) 2014 Arroware. All rights reserved.
//

#ifndef __ApolloLinuxClient__AROLogInterface__
#define __ApolloLinuxClient__AROLogInterface__

enum LogLevel {
    logERROR = 0,
    logWARNING,
    logINFO,
    logDEBUG,
    logDEBUG1,
    logDEBUG2,
    logDEBUG3,
    logDEBUG4
};

#ifdef __cplusplus

#include <stdarg.h>
#include <iostream>
#define AROLog(A, B, C, D, ...) AROLogInterface::Log().Print(A, B, C, D, ##__VA_ARGS__)

class AROLogInterface {
private:
    std::string current_log_date;
    std::string current_log_name;
    std::string log_suffix;
    std::string path;
    FILE *fp;
    enum LogLevel current_level;
    
    AROLogInterface(){};
    ~AROLogInterface(){};
    AROLogInterface(AROLogInterface const&){};
    
public:
    static AROLogInterface& Log();
    void OpenLog(std::string path, std::string log_name);
    void CloseLog();
    void Print(int level, int timed, const char *tag, const char *format, ...);
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

extern "C" {
    void AROLog_Print(int level, int timed, const char *tag, const char *format, ...);
}

#else

#define AROLog(...) AROLog_Print(__VA_ARGS__)
void AROLog_Print(int level, int timed, const char *tag, const char *format, ...);

#endif

#endif
