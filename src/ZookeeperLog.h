#ifndef ZOOKEEPER_LOG_H
#define ZOOKEEPER_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/**
*  @name Debug levels
*/
class ZookeeperLog;

typedef enum {ZOO_LOG_LEVEL_ERROR=1,ZOO_LOG_LEVEL_WARN=2,ZOO_LOG_LEVEL_INFO=3,ZOO_LOG_LEVEL_DEBUG=4} ZooLogLevel;

extern ZookeeperLog *gLogInstance;
static pthread_once_t ponce_ = PTHREAD_ONCE_INIT;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

const char* format_log_message(const char* format,...);

#define LOG_ERROR(x) if (gLogInstance) \
    gLogInstance->LogMessage(ZOO_LOG_LEVEL_ERROR,__LINE__,__func__,format_log_message x)
#define LOG_WARN(x) if (gLogInstance) \
    gLogInstance->LogMessage(ZOO_LOG_LEVEL_WARN,__LINE__,__func__,format_log_message x)
#define LOG_INFO(x) if (gLogInstance) \
    gLogInstance->LogMessage(ZOO_LOG_LEVEL_INFO,__LINE__,__func__,format_log_message x)
#define LOG_DEBUG(x) if (gLogInstance) \
    gLogInstance->LogMessage(ZOO_LOG_LEVEL_DEBUG,__LINE__,__func__,format_log_message x)

class ZookeeperLog {
public:
	static void Init(FILE* stream, ZooLogLevel logLevel);
	static void Release();
	void LogMessage(ZooLogLevel curLevel, int line, const char* funcName, const char* message);
private:
	ZookeeperLog();
	~ZookeeperLog();

	static void Create();

	void Initialize(FILE* stream, ZooLogLevel logLevel);
	ZooLogLevel m_logLevel;
	FILE* m_stream;
	static bool isInitialized;
	int ref_count;
};


#endif
