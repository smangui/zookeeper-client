#include "ZookeeperLog.h"
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#define TIME_NOW_BUF_SIZE 1024
#define FORMAT_LOG_BUF_SIZE 4096

ZookeeperLog *gLogInstance = NULL;

ZooLogLevel logLevel = ZOO_LOG_LEVEL_INFO;

static pthread_key_t time_now_buffer;
static pthread_key_t format_log_msg_buffer;
bool ZookeeperLog::isInitialized = false;

void freeBuffer(void* p){
    if(p) free(p);
}

char* getTSData(pthread_key_t key,int size){
	char* p = static_cast<char *>(pthread_getspecific(key));
	if(p == 0){
		int res;
		p = static_cast<char*>(calloc(1,size));
		res = pthread_setspecific(key,p);
		if(res != 0){
			fprintf(stderr,"Failed to set TSD key: %d",res);
		}
	}
	return p;
}


char* get_time_buffer(){
    return getTSData(time_now_buffer,TIME_NOW_BUF_SIZE);
}


char* get_format_log_buffer(){
    return getTSData(format_log_msg_buffer,FORMAT_LOG_BUF_SIZE);
}


const char* format_log_message(const char* format,...)
{
	va_list va;
	char* buf=get_format_log_buffer();
	if(!buf)
	    return "format_log_message: Unable to allocate memory buffer";
	
	va_start(va,format);
	vsnprintf(buf, FORMAT_LOG_BUF_SIZE-1,format,va);
	va_end(va);
	return buf;
}


static const char* time_now(char* now_str) {
	struct timeval tv;
	struct tm lt;
	time_t now = 0;
	size_t len = 0;
	
	gettimeofday(&tv,0);
	
	now = tv.tv_sec;
	localtime_r(&now, &lt);
	
	// clone the format used by log4j ISO8601DateFormat
	// specifically: "yyyy-MM-dd HH:mm:ss,SSS"
	len = strftime(now_str, TIME_NOW_BUF_SIZE,
	                      "%Y-%m-%d %H:%M:%S",
	                      &lt);
	len += snprintf(now_str + len,
	                TIME_NOW_BUF_SIZE - len,
	                ",%03d",
	                (int)(tv.tv_usec/1000));
	return now_str;
}


void
ZookeeperLog::Init(FILE* stream, ZooLogLevel logLevel) {
	if (isInitialized) {
		return;
	}
	pthread_once(&ponce_, &ZookeeperLog::Create);
	gLogInstance->Initialize(stream, logLevel);
}


void
ZookeeperLog::Release()
{
	if (gLogInstance) {
		delete gLogInstance;
	}
}


ZookeeperLog::ZookeeperLog()
	:m_logLevel(ZOO_LOG_LEVEL_INFO),
	 m_stream(NULL),
	 ref_count(1)
{
	pthread_key_create(&time_now_buffer, freeBuffer);
	pthread_key_create(&format_log_msg_buffer, freeBuffer);
}


ZookeeperLog::~ZookeeperLog()
{
}


void
ZookeeperLog::Create()
{
	gLogInstance = new ZookeeperLog();
}


void
ZookeeperLog::LogMessage(ZooLogLevel curLevel, int line, const char* funcName, const char* message)
{
	if (m_logLevel < curLevel) {
		return;
	}
	static const char* dbgLevelStr[] = {"ZOO_INVALID","ZOO_ERROR", "ZOO_WARN", "ZOO_INFO", "ZOO_DEBUG"};
	static pid_t pid = 0;

	if (pid == 0) pid = getpid();
	fprintf(m_stream, "%s:%d(0x%lx):%s@%s@%d: %s\n", time_now(get_time_buffer()), pid,
	        (unsigned long int)pthread_self(),
	        dbgLevelStr[curLevel],funcName,line,message);
	fflush(m_stream);
}


void
ZookeeperLog::Initialize(FILE* stream, ZooLogLevel logLevel)
{
	if (isInitialized) {
		return;
	}
	pthread_mutex_lock(&log_mutex);
	if (isInitialized) {
		return;
	}
	m_stream = stream ? stream: stderr;

	if (logLevel == 0) {
		m_logLevel = (ZooLogLevel)0;
		return;
	}
	if (logLevel < ZOO_LOG_LEVEL_ERROR) logLevel = ZOO_LOG_LEVEL_ERROR;
	if (logLevel > ZOO_LOG_LEVEL_DEBUG) logLevel = ZOO_LOG_LEVEL_DEBUG;
	m_logLevel = logLevel;
	isInitialized = true;
	LOG_INFO(("Log Initialize LogLevel = %d", m_logLevel));
	pthread_mutex_unlock(&log_mutex);
}
