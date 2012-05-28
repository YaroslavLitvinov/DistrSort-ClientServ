/*
 * logfile.h
 *
 *  Created on: 10.05.2012
 *      Author: YaroslavLitvinov
 */

#ifndef LOGFILE_H_
#define LOGFILE_H_

#include <stdio.h>

#define LOG_ERR 0
#define LOG_UI 1
#define LOG_DETAILED_UI 2
#define LOG_DEBUG 3
#define LOG_LEVEL LOG_DEBUG

//forward declarations
int log_open_stream(const char *log_file, const char* prefix, int id);
const char *log_prefix();
FILE *log_file();

#ifdef LOG_ENABLE
	#define OPEN_LOG(path, prefix, id) log_open_stream(path, prefix, id)
	#define WRITE_FMT_LOG(level, fmt, args...) \
	if (level<=LOG_LEVEL){\
		fprintf(log_file(), "%s %s() ", log_prefix(), __FUNCTION__ );\
		fprintf(log_file(), fmt, args); \
		fprintf(log_file(), "\n"); \
		fflush(log_file());\
	}
#define WRITE_LOG(level, str) \
if (level<=LOG_LEVEL){\
	fprintf(log_file(), "%s %s() ", log_prefix(), __FUNCTION__ );\
	fprintf(log_file(), "%s\n", str); \
	fflush(log_file());\
}
#else
	#define OPEN_LOG(path, prefix, id)
	#define WRITE_FMT_LOG(format, ...)
	#define WRITE_LOG(format, str)
#endif




#endif /* LOGFILE_H_ */
