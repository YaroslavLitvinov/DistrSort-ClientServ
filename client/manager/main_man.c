/*
 * manager.c
 *
 *  Created on: 29.04.2012
 *      Author: yaroslav
 */

#include "defines.h"
#include "sqluse_cli.h"
#include "comm_man.h"
#include "distr_common.h"
#include "histanlz.h"
#include "dsort.h"
#include "logfile.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>


int main(int argc, char *argv[]){
	struct file_records_t file_records;
	if ( argc > 2 ){
		char logname[50];
		sprintf(logname, "%s%s%s.log", CLIENT_LOG, MANAGER, argv[2]);
		OPEN_LOG(logname, MANAGER, atoi(argv[2]));
		file_records.vfs_path = argv[1];
		int err = get_all_files_from_dbtable(DB_PATH, MANAGER, &file_records);
		if ( err != 0 ) return 1;
	}
	else{
		printf("usage: 1st arg: should be path to VFS folder, 2nd: unique node integer id\n");fflush(0);
		return 1;
	}

	int nodeid = atoi(argv[2]);

	WRITE_LOG(LOG_UI, "Manager node started");

	//////////////////////////////////////////
	struct file_record_t* hist_record = match_file_record_by_fd( &file_records, MANAGER_FD_READ_HISTOGRAM);
	assert(hist_record);
	struct Histogram histograms[SRC_NODES_COUNT];
	WRITE_LOG(LOG_DETAILED_UI, "Recv histograms");
	recv_histograms( hist_record->fpath, (struct Histogram*) &histograms, SRC_NODES_COUNT );
	WRITE_LOG(LOG_DETAILED_UI, "Recv histograms OK");
	//////////////////////////////////////////

	WRITE_LOG(LOG_DETAILED_UI, "Analize histograms and request detailed histograms");
	struct request_data_t** range = alloc_range_request_analize_histograms( &file_records,
			ARRAY_ITEMS_COUNT, nodeid, histograms, SRC_NODES_COUNT );
	WRITE_LOG(LOG_DETAILED_UI, "Analize histograms and request detailed histograms OK");

#if defined(LOG_ENABLE) && LOG_LEVEL>=LOG_MISC
	for (int i=0; i < SRC_NODES_COUNT; i++ )
	{
		WRITE_FMT_LOG( LOG_DEBUG, "SOURCE PART N %d:\n", i );
		print_request_data_array( range[i], SRC_NODES_COUNT );
	}
#endif

	/// sending range requests to every source node
	for (int i=0; i < SRC_NODES_COUNT; i++ ){
		///
		int src_nodeid = range[0][i].src_nodeid;
		int src_write_fd = src_nodeid - FIRST_SOURCE_NODEID + MANAGER_FD_WRITE_RANGE_REQUEST;
		WRITE_FMT_LOG(LOG_DEBUG, "write_sorted_ranges fdw=%d", src_write_fd );

		struct file_record_t* rangereq_record = match_file_record_by_fd( &file_records, src_write_fd);
		assert(rangereq_record);
		write_range_request( rangereq_record->fpath, range, SRC_NODES_COUNT, i );
	}

	for ( int i=0; i < SRC_NODES_COUNT; i++ ){
		free(histograms[i].array);
		free( range[i] );
	}
	free(range);

	struct file_record_t* results_record = match_file_record_by_fd( &file_records, MANAGER_FD_READ_SORT_RESULTS);
	assert(results_record);
	struct sort_result *results = read_sort_result( results_record->fpath, SRC_NODES_COUNT );

	qsort( results, SRC_NODES_COUNT, sizeof(struct sort_result), sortresult_comparator );
	int sort_ok = 1;
	for ( int i=0; i < SRC_NODES_COUNT; i++ ){
		if ( i>0 ){
			if ( !(results[i].max > results[i].min && results[i-1].max < results[i].min) )
				sort_ok = 0;
		}
		WRITE_FMT_LOG(LOG_UI, "results[%d], nodeid=%d, min=%d, max=%d\n",
				i, results[i].nodeid, results[i].min, results[i].max);
		fflush(0);
	}

	WRITE_FMT_LOG( LOG_UI, "Distributed sort complete, Test %d\n", sort_ok );

	return 0;
}
