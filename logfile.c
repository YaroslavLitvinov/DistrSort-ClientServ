/*
 * logfile.c
 *
 *  Created on: 10.05.2012
 *      Author: YaroslavLitvinov
 */

#include "logfile.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>


static char __prefix[40];
static FILE *__log_file = NULL;


static char  *NaClTimeStampString(char   *buffer, size_t buffer_size) {
  struct timeval  tv;
  struct tm       bdt;  /* broken down time */

  if (-1 == gettimeofday(&tv, (struct timezone *) NULL)) {
    snprintf(buffer, buffer_size, "-NA-");
    return buffer;
  }
  (void) localtime_r(&tv.tv_sec, &bdt);
  /* suseconds_t holds at most 10**6 < 16**6 = (2**4)**6 = 2**24 < 2**32 */
  snprintf(buffer, buffer_size, "%02d:%02d:%02d.%06d",
           bdt.tm_hour, bdt.tm_min, bdt.tm_sec, (int) tv.tv_usec);
  return buffer;
}

/*@return 0 succes, -1 error*/
int log_open_stream(const char *log_file, const char* prefix, int id) {
	sprintf(__prefix+10, "[%s:%d]", prefix, id);
	int   fd;

	fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT, 0777);
	if (-1 == fd) {
		perror("log_open_stream::open\n");
	}

	__log_file = fdopen(fd, "a");
	if (NULL == __log_file) {
		perror("log_open_stream::fdopen\n");
	}
	return !__log_file?  -1: 0;
}

const char *log_prefix(){
	memset(__prefix, ' ', 10);
	NaClTimeStampString( __prefix, 10);
	return __prefix;
}

FILE *log_file(){
	return __log_file;
}


