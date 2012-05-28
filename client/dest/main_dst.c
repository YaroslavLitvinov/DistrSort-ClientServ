/*
 * main_src.c
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */

#include "defines.h"
#include "sort.h"
#include "dsort.h"
#include "comm_dst.h"
#include "sqluse_cli.h"
#include "logfile.h"

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


int main(int argc, char *argv[]){
	struct file_records_t file_records;
	if ( argc > 2 ){
		char logname[50];
		sprintf(logname, "%s%s%s.log", CLIENT_LOG, DEST, argv[2]);
		OPEN_LOG(logname, DEST, atoi(argv[2]));
		file_records.vfs_path = argv[1];
		int err = get_all_files_from_dbtable(DB_PATH, DEST, &file_records);
		if ( err != 0 ) return 1;
	}
	else{
		printf("usage: 1st arg: should be path to VFS folder, 2nd: unique node integer id\n");fflush(0);
		return 1;
	}

	int nodeid = atoi(argv[2]);
	WRITE_FMT_LOG(LOG_UI, "[%d] Destination node started", nodeid);

	BigArrayPtr unsorted_array = NULL;
	BigArrayPtr sorted_array = NULL;

	unsorted_array = malloc( ARRAY_ITEMS_COUNT*sizeof(BigArrayItem) );


	struct file_record_t* ranges_r_record = match_file_record_by_fd( &file_records, DEST_FD_READ_SORTED_RANGES_REP);
	struct file_record_t* ranges_w_record = match_file_record_by_fd( &file_records, DEST_FD_WRITE_SORTED_RANGES_REP);
	assert(ranges_r_record);
	assert(ranges_w_record);
	repreq_read_sorted_ranges( ranges_r_record->fpath, ranges_w_record->fpath, nodeid,
			unsorted_array, ARRAY_ITEMS_COUNT, SRC_NODES_COUNT );

	sorted_array = alloc_merge_sort( unsorted_array, ARRAY_ITEMS_COUNT );

	/*save sorted array to output file*/
	char outputfile[100];
	memset(outputfile, '\0', 100);
	sprintf(outputfile, DEST_FILE_FMT, nodeid );
	int destfd = open(outputfile, O_WRONLY|O_CREAT);
	if ( destfd >= 0 ){
		const size_t data_size = sizeof(BigArrayItem)*ARRAY_ITEMS_COUNT;
		if ( sorted_array ){
			const ssize_t wrote = write(destfd, sorted_array, data_size);
			assert(wrote == data_size );
		}
		close(destfd);
	}

	struct file_record_t* results_w_record = match_file_record_by_fd( &file_records, DEST_FD_WRITE_SORT_RESULT);
	assert(results_w_record);
	write_sort_result( results_w_record->fpath, nodeid, sorted_array, ARRAY_ITEMS_COUNT );

	free(unsorted_array);
	WRITE_FMT_LOG(LOG_UI, "[%d] Destination node complete", nodeid);
}

