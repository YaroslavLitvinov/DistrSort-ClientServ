/*
 * sqluse_cli.h
 *
 *  Created on: 02.05.2012
 *      Author: YaroslavLitvinov
 */

#ifndef SQLUSE_CLI_H_
#define SQLUSE_CLI_H_

#include <stdio.h>

struct file_records_t{
	struct file_record_t *array;
	int count;
	int maxcount;
	char *vfs_path;
};

enum { ECOL_NODENAME=0, ECOL_FTYPE=1, ECOL_FMODE=2, ECOL_FPATH=3, ECOL_FD=4, ECOL_COUNT};

struct file_record_t{
	char *nodename;
	char ftype[4];
	char fmode;
	char *fpath;
	int fd;
};

enum {FILE_RECORDS_GRANULARITY=20};
struct file_record_t* match_file_record_by_fd(struct file_records_t *records, int fd);
int get_filerecords_callback(void *file_records, int argc, char **argv, char **azColName);
int get_all_files_from_dbtable(const char *path, const char *nodename, struct file_records_t *frecords);

#endif /* SQLUSE_CLI_H_ */
