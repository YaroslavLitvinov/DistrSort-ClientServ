/*
 * fs_inmem.h
 *
 *  Created on: 20.04.2012
 *      Author: yaroslav
 */

#ifndef FUSEXMP_H_
#define FUSEXMP_H_

#include <sys/types.h> //ssize_t

//external
struct db_records_t;



enum {EFILE_STD, EFILE_MSQ, EFILE_USER};

struct fs_in_memory_t{
	struct file_info_array_t *fs_structure;
};


struct file_info_t {
	int fd;
	char* path;
	int access_mode;
	int nlink;
	int inode;
};

enum { EFS_ARAAY_GRANULARITY=20};
struct file_info_array_t{
	struct file_info_t *array;
	int count;
	int maxcount;
};
struct file_info_t *
fsfile_by_path(struct file_info_array_t *array, const char *path);
int callback_add_dir(const char *dirpath, int len);
void process_subdirs_via_callback( int (*callback_add_dir)(const char *, int), const char *path, int len );

void
fill_fsstructure_by_records( struct file_info_array_t **fs_structure, struct db_records_t *file_records);


int run_fuse_main( struct db_records_t *file_records, int argc, char *argv[]);


#endif /* FUSEXMP_H_ */
