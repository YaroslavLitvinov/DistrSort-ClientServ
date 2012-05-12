/*
 * communic.c
 *
 *  Created on: 29.04.2012
 *      Author: yaroslav
 */


#include "node_io.h"
#include "distr_common.h"
#include "defines.h"
#include "logfile.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


/*READ 1x struct packet_data_t
 *READ array_len x  HistogramArrayItem, array*/
void
recv_histograms( const char *readf, struct Histogram *histograms, int wait_histograms_count ){
	int fdr = open(readf, O_RDONLY);
	WRITE_FMT_LOG(LOG_NET, "Reading histograms %d count", wait_histograms_count);
	int bytes;
	for( int i=0; i < wait_histograms_count; i++ ){
		WRITE_FMT_LOG(LOG_NET, "Reading histogram #%d", i);
		struct packet_data_t t; t.type = EPACKET_UNKNOWN;
		bytes=read(fdr, (char*) &t, sizeof(t) );
		WRITE_FMT_LOG(LOG_DEBUG, "r packet_data_t bytes=%d", bytes);
		WRITE_FMT_LOG(LOG_NET, "readed packet: type=%d, size=%d, src_nodeid=%d", t.type, (int)t.size, t.src_nodeid );
		struct Histogram *histogram = &histograms[i];
		if ( EPACKET_HISTOGRAM == t.type ){
			histogram->array = malloc( t.size );
			bytes=read(fdr, (char*) histogram->array, t.size);
			WRITE_FMT_LOG(LOG_DEBUG, "r histogram->array bytes=%d", bytes);
			histogram->array_len = t.size / sizeof(HistogramArrayItem);
			histogram->src_nodeid = t.src_nodeid;
		}
		else{
			WRITE_FMT_LOG(LOG_NET, "recv_histograms::wrong packet type %d size %d", t.type, (int)t.size);
			exit(-1);
		}
	}
	WRITE_LOG(LOG_NET, "Hitograms ok");
	close(fdr);
}


/*i/o 2 synchronous files
 *WRITE  1x struct request_data_t
 *READ 1x char, reply
 *WRITE  1x char is_complete flag, if 0
 *READ 1x int, nodeid
 *WRITE  1x char, reply
 *READ 1x size_t, array_len
 *WRITE  1x char, reply
 *READ array_size, array HistogramArrayItem
 * @param complete Flag 0 say to client in request that would be requested again, 1-last request send
 *return Histogram Caller is responsive to free memory after using result*/
struct Histogram*
reqrep_detailed_histograms_alloc( const char *writef, const char *readf, int nodeid,
		const struct request_data_t* request_data, int complete ){
	int fdw = open(writef, O_WRONLY);
	int fdr = open(readf, O_RDONLY);
	char reply;
	WRITE_FMT_LOG(LOG_NET, "[%d] complete=%d, Write detailed histogram requests to %s",
			nodeid, complete, writef );
	int bytes;
	//send detailed histogram request
	bytes=write(fdw, request_data, sizeof(struct request_data_t));
	WRITE_FMT_LOG(LOG_DEBUG, "w packet_data_t bytes=%d", bytes);
	bytes=read(fdr, &reply, 1);
	WRITE_FMT_LOG(LOG_DEBUG, "r reply bytes=%d", bytes);
	bytes=write(fdw, &complete, sizeof(complete));
	WRITE_FMT_LOG(LOG_DEBUG, "w is_complete bytes=%d", bytes);
	WRITE_LOG(LOG_NET, "detailed histograms receiving");
	//recv reply
	struct Histogram *item = malloc(sizeof(struct Histogram));
	bytes=read(fdr, (char*) &item->src_nodeid, sizeof(item->src_nodeid));
	WRITE_FMT_LOG(LOG_DEBUG, "r item->src_nodeid bytes=%d", bytes);
	bytes=write(fdw, &reply, 1);
	WRITE_FMT_LOG(LOG_DEBUG, "w reply bytes=%d", bytes);
	bytes=read(fdr, (char*) &item->array_len, sizeof(item->array_len));
	WRITE_FMT_LOG(LOG_DEBUG, "r item-?array_len bytes=%d", bytes);
	bytes=write(fdw, &reply, 1);
	WRITE_FMT_LOG(LOG_DEBUG, "w reply bytes=%d", bytes);
	size_t array_size = sizeof(HistogramArrayItem)*item->array_len;
	item->array = malloc(array_size);
	bytes=read(fdr, (char*) item->array, array_size);
	WRITE_FMT_LOG(LOG_DEBUG, "r item->array bytes=%d", bytes);

	WRITE_FMT_LOG(LOG_NET, "detailed histograms received from%d: expected len:%d",
			item->src_nodeid, (int)(sizeof(HistogramArrayItem)*item->array_len) );

	close(fdw);
	close(fdr);
	return item;
}

/*WRITE 1x struct packet_data_t
 *WRITE 1x struct request_data_t*/
void
write_range_request( const char *writef, struct request_data_t** range, int len, int i ){
	int fdw = open(writef, O_WRONLY);
	struct packet_data_t t;
	t.type = EPACKET_SEQUENCE_REQUEST;
	t.src_nodeid = (int)range[i][0].dst_nodeid;
	t.size = len;
	int bytes;
	WRITE_FMT_LOG(LOG_NET, "t.ize=%d, t.type=%d(EPACKET_SEQUENCE_REQUEST)", (int)t.size, t.type);
	bytes=write(fdw, &t, sizeof(t));
	WRITE_FMT_LOG(LOG_DEBUG, "w packet_data_t bytes=%d", bytes);
	for ( int j=0; j < len; j++ ){
		/*request data copy*/
		struct request_data_t data = range[j][i];
		data.dst_nodeid = j+FIRST_DEST_NODEID;
		bytes=write(fdw, &data, sizeof(struct request_data_t));
		WRITE_FMT_LOG(LOG_DEBUG, "w request_data_t bytes=%d", bytes);
		WRITE_FMT_LOG(LOG_NET, "sendseq %d %d %d %d\n",
				data.dst_nodeid, data.src_nodeid, data.first_item_index, data.last_item_index );
	}
	close(fdw);
}

/*
 *READ waiting_results x struct sort_result*/
struct sort_result*
read_sort_result( const char* readf, int waiting_results ){
	if ( !waiting_results ) return NULL;
	int fdr = open(readf, O_RDONLY);

	struct sort_result *results = malloc( SRC_NODES_COUNT*sizeof(struct sort_result) );
	for ( int i=0; i < waiting_results; i++ ){
		read( fdr, (char*) &results[i], sizeof(struct sort_result) );
	}

	close(fdr);
	return results;
}

#ifdef LOG_ENABLE
void
print_request_data_array( struct request_data_t* const range, int len ){
	for ( int j=0; j < len; j++ )
	{
		WRITE_FMT_LOG(LOG_NET, "SEQUENCE N:%d, dst_pid=%d, src_pid=%d, findex %d, lindex %d \n",
				j, range[j].dst_nodeid, range[j].src_nodeid,
				range[j].first_item_index, range[j].last_item_index );
	}
}
#endif
