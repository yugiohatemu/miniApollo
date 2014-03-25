#include "AROLogInterface.h"
#include <cstdio>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sstream>
#include <fstream>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

boost::mutex log_stream_lock;

void AROLog_Print(int level, int timed, const char *tag, const char *format, ...) {
#ifndef PRODUCTION
    va_list args;
    va_start(args, format);
    AROLogInterface::Log().vPrint(level, timed, tag, format, args);
    va_end(args);
#endif
}

void AROLogInterface::Print(int level, int timed, const char *tag, const char *format, ...) {
#ifndef PRODUCTION
    va_list args;
    va_start(args, format);
    vPrint(level,timed,tag,format,args);
    va_end(args);
#endif
}


AROLogInterface& AROLogInterface::Log(){
    static AROLogInterface m_pInstance;
    return m_pInstance;
}

void AROLogInterface::OpenLog(std::string path, std::string filename) {
#ifndef PRODUCTION
#if !defined(__ANDROID__)
    std::stringstream timed_filename;
    this->path = path;
    
    timed_filename << path << "/" << GetLogFilePrefix() << "_" << filename;
    if ((fp = fopen(timed_filename.str().c_str(), "a")) == NULL) {
        fprintf(stderr, "Error opening log file: %s\n", timed_filename.str().c_str());
    } else {
        fprintf(stdout, "%s: INFO  : Log file \"%s\" opened\n", GetTime().c_str(), timed_filename.str().c_str());
        fprintf(fp, "%s: INFO  : Log file \"%s\" opened\n", GetTime().c_str(), timed_filename.str().c_str());
        log_suffix = filename;
        current_log_date = GetDate();
        current_log_name = timed_filename.str();
    }
    //}
#endif
#endif
}

void AROLogInterface::CloseLog() {
#ifndef PRODUCTION
#if !defined(__ANDROID__)
    if (fp) {
        fprintf(stdout, "%s: Log file closed\n", GetTime().c_str());
        fprintf(fp, "%s: INFO  : Log file closed\n", GetTime().c_str());
        fclose(fp);
        fp = NULL;
        //        current_log_name = "";
        //        current_log_date = "";
    } else {
        fprintf(stdout, "Log already closed\n");
    }
#endif
#endif
}

const std::string AROLogInterface::ToString(int level) {
    static const char* const buffer[] = {"ERROR ", "WARN  ", "INFO  ", "DEBUG ", "DEBUG1", "DEBUG2", "DEBUG3", "DEBUG4"};
    return buffer[level];
}

void AROLogInterface::CheckLogDate() {
#if !defined(__ANDROID__)
    std::stringstream timed_filename;
    
    if (GetDate().compare(current_log_date)) {
        fprintf(stdout, "%s: INFO  : End of day for log file: %s\n", GetTime().c_str(), current_log_name.c_str());
        fprintf(fp, "%s: INFO  : End of day for log file: %s\n", GetTime().c_str(), current_log_name.c_str());
        fclose(fp);
        fp = NULL;
        OpenLog(path, log_suffix);
    }
#endif
}

void AROLogInterface::vPrint(int level, int timed, const char *tag, const char *format, va_list args) {
#ifndef PRODUCTION
    //va_list args;
    
    log_stream_lock.lock();
    
#if defined(__ANDROID__)
    int android_level;
    
    switch (level) {
        case logERROR: android_level = ANDROID_LOG_ERROR; break;
        case logINFO: android_level = ANDROID_LOG_INFO; break;
        case logWARNING: android_level = ANDROID_LOG_WARN; break;
        default: android_level = ANDROID_LOG_DEBUG; break;
    }
    //va_start(args, format);
    if (level <= current_level) {
        __android_log_vprint(android_level, tag, format, args);
    }
    //va_end(args);
#else
    
    if (!fp) {
        fprintf(stderr, "Please open log file before printing log messages.\n");
        //va_start(args, format);
        vfprintf(stdout, format, args);
        //va_end(args);
    } else {
        CheckLogDate();
        if (timed) {
            //                fprintf(stdout, "%d: ", ugetmalloccounter());
            if (level <= current_level) {
                fprintf(stdout, "tid: %s: ", boost::lexical_cast<std::string>(boost::this_thread::get_id()).c_str());
                fprintf(stdout, "%s: ", GetTime().c_str());
                fprintf(stdout, "%s: ", ToString(level).c_str());
                fprintf(stdout, "%s: ", tag);
            }
            fprintf(fp, "tid: %s: ", boost::lexical_cast<std::string>(boost::this_thread::get_id()).c_str());
            fprintf(fp, "%s: ", GetTime().c_str());
            fprintf(fp, "%s: ", ToString(level).c_str());
            fprintf(fp, "%s: ", tag);
        }
        //va_start(args, format);
        if (level <= current_level
            //                && (!strcmp(tag, "CM") || !strcmp(tag, "B") || !strcmp(tag, "D"))
            ) {
            va_list args2; va_copy(args2, args);
            vfprintf(stdout, format, args2);
            va_end(args2);
        }
        //va_end(args);
        //va_start(args, format);
        vfprintf(fp, format, args);
        //va_end(args);
        fflush(fp);
    }
#endif
    log_stream_lock.unlock();
#endif // !defined(PRODUCTION)
}


void AROLogInterface::SetLogLevel(int level) {
    current_level = (enum LogLevel)level;
}

int AROLogInterface::GetLogLevel() {
    return current_level;
}

std::string AROLogInterface::GetDate() {
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    char       buf[80];
    strftime(buf, sizeof(buf), "%Y%m%d", now);
    
    return buf;
}

std::string AROLogInterface::GetLogFilePrefix() {
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    char buf[80];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", now);
    
    return buf;
}

std::string AROLogInterface::GetTime() {
    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    char       buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", now);
    
    return buf;
}
