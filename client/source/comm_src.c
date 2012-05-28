/*
 * comm_src.c
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */


#include "comm_src.h"
#include "dsort.h"
#include "logfile.h"
#include "defines.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>


void
init_request_data_array( struct request_data_t *req_data, int len ){
	for ( int j=0; j < len; j++ ){
		req_data[j].src_nodeid = 0;
		req_data[j].dst_nodeid = 0;
		req_data[j].first_item_index = 0;
		req_data[j].last_item_index = 0;
	}
}

/*writing data
 *WRITE 1x uint32_t, crc*/
void
write_crc(const char *writef, uint32_t crc ){
	int fdw = open(writef, O_WRONLY);
	int bytes = 0;
	bytes = write(fdw, &crc, sizeof(crc) );
	WRITE_FMT_LOG(LOG_DEBUG, "w crc bytes=%d", bytes);
	close(fdw);
}

/*writing data
 * 1x struct packet_data_t[EPACKET_HISTOGRAM]
 * array_len x HistogramArrayItem*/
void
write_histogram( const char *writef, const struct Histogram *histogram ){
	size_t array_size = sizeof(HistogramArrayItem)*(histogram->array_len);
	struct packet_data_t t;
	t.type = EPACKET_HISTOGRAM;
	t.src_nodeid = histogram->src_nodeid;
	t.size = array_size;
	WRITE_FMT_LOG(LOG_DEBUG, "size=%d, type=%d, src_nodeid=%d", (int)t.size, t.type, t.src_nodeid);

	int fdw = open(writef, O_WRONLY);
	int bytes = 0;
	bytes = write(fdw, &t, sizeof(t) );
	WRITE_FMT_LOG(LOG_DEBUG, "w packet_data_t bytes=%d", bytes);
	bytes = write(fdw, histogram->array, array_size );
	WRITE_FMT_LOG(LOG_DEBUG, "w array bytes=%d", bytes);
	close(fdw);
}

/*i/o 2 files
 *READ  1x struct request_data_t
 *WRITE 1x char, reply
 *READ  1x char is_complete flag, if 0
 *WRITE 1x int, nodeid
 *READ  1x char, reply
 *WRITE 1x size_t, array_len
 *READ  1x char, reply
 *WRITE array_len x HistogramArrayItem, histograms array
 * @return 1-should receive again, 0-complete request - it should not be listen again*/
int
read_requests_write_detailed_histograms( const char* readf, const char* writef, int nodeid,
		const BigArrayPtr source_array, int array_len){
	int fdr = open(readf, O_RDONLY);
	int fdw = open(writef, O_WRONLY);

	int is_complete = 0;
	char reply;
	do {
		WRITE_FMT_LOG(LOG_DEBUG, "Reading from file %s detailed histograms request", readf );fflush(0);
		/*receive data needed to create histogram using step=1,
		actually requested histogram should contains array items range*/
		struct request_data_t received_histogram_request;
		int bytes;
		bytes = read(fdr, (char*) &received_histogram_request, sizeof(received_histogram_request) );
		WRITE_FMT_LOG(LOG_DEBUG, "r request_data_t bytes=%d", bytes);
		bytes = write(fdw, &reply, 1);
		WRITE_FMT_LOG(LOG_DEBUG, "w reply bytes=%d", bytes);
		bytes = read(fdr, (char*) &is_complete, sizeof(is_complete) );
		WRITE_FMT_LOG(LOG_DEBUG, "r is_complete bytes=%d", bytes);

		int histogram_len = 0;
		//set to our offset, check it
		int offset = min(received_histogram_request.first_item_index, array_len-1 );
		int requested_length = received_histogram_request.last_item_index - received_histogram_request.first_item_index;
		requested_length = min( requested_length, array_len - offset );

		HistogramArrayPtr histogram = alloc_histogram_array_get_len( source_array, offset, requested_length, 1, &histogram_len );

		size_t sending_array_len = histogram_len;
		/*Response to request, entire reply contains requested detailed histogram*/
		bytes=write(fdw, &nodeid, sizeof(int) );
		WRITE_FMT_LOG(LOG_DEBUG, "w node_id bytes=%d", bytes);
		bytes=read(fdr, &reply, 1);
		WRITE_FMT_LOG(LOG_DEBUG, "r reply bytes=%d", bytes);
		bytes=write(fdw, &sending_array_len, sizeof(size_t) );
		WRITE_FMT_LOG(LOG_DEBUG, "w sending_array_len bytes=%d", bytes);
		bytes=read(fdr, &reply, 1);
		WRITE_FMT_LOG(LOG_DEBUG, "r reply bytes=%d", bytes);
		bytes=write(fdw, histogram, histogram_len*sizeof(HistogramArrayItem) );
		WRITE_FMT_LOG(LOG_DEBUG, "r array histogram_len bytes=%d", bytes);
		free( histogram );
		WRITE_FMT_LOG(LOG_DETAILED_UI, "histograms wrote into file %s", writef );
	}while(!is_complete);

	close(fdr);
	close(fdw);
	return is_complete;
}



/*READ 1x struct packet_data_t {type EPACKET_SEQUENCE_REQUEST}
 *READ 1x struct request_data_t
 * @param dst_pid destination process should receive ranges*/
int
read_range_request( const char *readf, struct request_data_t* sequence ){
	int len = 0;
	int fdr = open(readf, O_RDONLY);

	WRITE_FMT_LOG(LOG_DEBUG, "reading range  request from %s (EPACKET_SEQUENCE_REQUEST)", readf);
	int bytes;
	struct packet_data_t t;
	t.type = EPACKET_UNKNOWN;
	bytes=read( fdr, (char*)&t, sizeof(t) );
	WRITE_FMT_LOG(LOG_DEBUG, "r packet_data_t bytes=%d", bytes);
	WRITE_FMT_LOG(LOG_DEBUG, "readed packet: type=%d, size=%d, src_node=%d", t.type, (int)t.size, t.src_nodeid );

	if ( t.type == EPACKET_SEQUENCE_REQUEST )
	{
		struct request_data_t *data = NULL;
		for ( int j=0; j < t.size; j++ ){
			data = &sequence[j];
			bytes=read(fdr, data, sizeof(struct request_data_t));
			WRITE_FMT_LOG(LOG_DEBUG, "r packet_data_t bytes=%d", bytes);
			WRITE_FMT_LOG(LOG_DEBUG, "recv range request %d %d %d", data->src_nodeid, data->first_item_index, data->last_item_index );
		}
	}
	else{
		WRITE_LOG(LOG_ERR, "channel_recv_sequences_request::packet Unknown");
		assert(0);
	}
	close(fdr);
	return len;
}

/*i/o 2 files
 *WRITE 1x struct packet_data_t [EPACKET_RANGE]
 *READ 1x char, reply
 *WRITE array_len x BigArrayItem, array
 *READ 1x char, reply*/
void
write_sorted_ranges( const char *writef, const char *readf,
		const struct request_data_t* sequence, const BigArrayPtr src_array ){
	int fdw = open(writef, O_WRONLY);
	int fdr = open(readf, O_RDONLY);

	const int array_len = sequence->last_item_index - sequence->first_item_index + 1;
	const BigArrayPtr array = src_array+sequence->first_item_index;
	WRITE_FMT_LOG(LOG_DEBUG, "Sending array_len=%d; min=%d, max=%d", array_len, array[0], array[array_len-1]);
	int bytes;
	char unused_reply;

#ifdef FUSE_CLIENT
	/*FUSE server does not support data size > 128KB passed trying to write into file
	 * therefore divide array data into smaller blocks FUSE_IO_MAX_CHUNK_SIZE*/
	int length_of_array_part = FUSE_IO_MAX_CHUNK_SIZE / sizeof(BigArrayItem);
	/*get size of array part alligned by item size*/
	int size_of_array_part = length_of_array_part * sizeof(BigArrayItem);
	int sent_array_size = 0;

	/*write into header complete size of sending array, size field contain sending array size*/
	struct packet_data_t range_header;
	range_header.size = array_len*sizeof(BigArrayItem);
	range_header.type = EPACKET_RANGE;
	bytes=write(fdw, (char*) &range_header, sizeof(struct packet_data_t) );
	WRITE_FMT_LOG(LOG_DEBUG, "w header EPACKET_RANGE bytes=%d", bytes);
	bytes=read( fdr, &unused_reply, 1 );
	WRITE_FMT_LOG(LOG_DEBUG, "r reply bytes=%d", bytes);

	struct packet_data_t range_part_header;
	range_part_header.type = EPACKET_RANGE_PART;
	while( sent_array_size < range_header.size ){
		int sending_data_size = min(size_of_array_part, range_header.size - sent_array_size );
		range_part_header.size = sending_data_size;
		int buf_size = sizeof(struct packet_data_t) + sending_data_size;
		char *buffer = malloc(buf_size);
		/*write header into buffer*/
		memcpy(buffer, &range_part_header, sizeof(struct packet_data_t) );
		WRITE_FMT_LOG(LOG_DEBUG, "w into buffer EPACKET_RANGE_PART bytes=%d", (int)sizeof(struct packet_data_t));
		/*write array data into buffer*/
		int sent_array_items = sent_array_size / sizeof(BigArrayItem);
		memcpy(buffer+sizeof(struct packet_data_t), array+sent_array_items, sending_data_size );
		WRITE_FMT_LOG(LOG_DEBUG, "w into buffer array bytes=%d", sending_data_size);
		/*write buffer into file*/
		bytes=write(fdw, buffer, buf_size);
		WRITE_FMT_LOG(LOG_DEBUG, "w array bytes=%d", bytes);
		bytes=read( fdr, &unused_reply, 1 );
		WRITE_FMT_LOG(LOG_DEBUG, "r reply bytes=%d", bytes);
		free(buffer);
		sent_array_size += sending_data_size;
	}
#else
	/*write header*/
	struct packet_data_t t;
	t.size = array_len*sizeof(BigArrayItem);
	t.type = EPACKET_RANGE;
	WRITE_FMT_LOG(LOG_DEBUG, "writing to %s: size=%d, type=%d (EPACKET_RANGE)", writef, (int)t.size, t.type );
	WRITE_FMT_LOG(LOG_DEBUG, "sizeof(struct packet_data_t)=%d", (int)sizeof(struct packet_data_t));
	bytes=write(fdw, (char*) &t, sizeof(struct packet_data_t) );
	WRITE_FMT_LOG(LOG_DEBUG, "w packet_data_t bytes=%d", bytes);
	bytes=read( fdr, &unused_reply, 1 );
	WRITE_FMT_LOG(LOG_DEBUG, "r reply bytes=%d", bytes);
	/*write array data*/
	bytes=write(fdw, array, t.size);
	WRITE_FMT_LOG(LOG_DEBUG, "w array bytes=%d", bytes);
	WRITE_FMT_LOG(LOG_DEBUG, "Reading from %s receiver reply;", readf);
	bytes=read( fdr, &unused_reply, 1 );
	WRITE_FMT_LOG(LOG_DEBUG, "r reply bytes=%d", bytes);
#endif
	WRITE_LOG(LOG_DEBUG, "Reply from receiver OK;");
	close(fdw);
	close(fdr);
}



