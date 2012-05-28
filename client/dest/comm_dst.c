/*
 * comm_src.c
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */


#include "comm_dst.h"
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



/*reading data
 * 1x struct packet_data_t (packet type, size of array)
 *writing data
 * 1x char, reply
 *reading data
 * packet_data_t::size x BigArrayItem (sorted part of array)
 *writing data
 * 1x char, reply
 * */
void
repreq_read_sorted_ranges( const char *readf, const char *writef, int nodeid,
		BigArrayPtr dst_array, int dst_array_len, int ranges_count ){
	int fdr = open(readf, O_RDONLY);
	int fdw = open(writef, O_WRONLY);
	int bytes;

	char reply='-';

#ifdef FUSE_CLIENT
	/*FUSE server does not support data size > 128KB by sending side
	 * therefore data divided into smaller blocks FUSE_IO_MAX_CHUNK_SIZE*/
	int recv_bytes_count = 0;
	int waiting_array_bytes_count = 0;
	int recv_ranges=0;
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] recv array -->", nodeid);

	struct packet_data_t header;
	do{
		bytes=read( fdr, (char*)&header, sizeof(header) );
		WRITE_FMT_LOG(LOG_DEBUG, "r EPACKET_RANGE bytes=%d", bytes);

		switch( header.type ){
		case EPACKET_RANGE:
			waiting_array_bytes_count += header.size;
			recv_ranges++;
			break;
		case EPACKET_RANGE_PART:
			/*two reads one by one: first reading header, it's read from socket only 24bytes,
			 * second read is reading from buffer where unread data by 1st read was saved*/
			bytes = read( fdr, (char*)&dst_array[recv_bytes_count/sizeof(BigArrayItem)], header.size );
			recv_bytes_count += bytes;
			WRITE_FMT_LOG(LOG_DEBUG, "r dst_array bytes=%d", bytes);
			assert(bytes>0);
			break;
		default:
			WRITE_FMT_LOG(LOG_DEBUG, "Unknown header=%d", header.type);
			break;
		}
		bytes=write(fdw, &reply, 1);
		WRITE_FMT_LOG(LOG_DEBUG, "w reply bytes=%d", bytes);
		/*waiting in loop while not all waiting bytes received and for all ranges*/
	}while( recv_bytes_count < waiting_array_bytes_count || recv_ranges < ranges_count );
#else
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] Read ranges from %s\n", nodeid, readf);
	for (int i=0; i < ranges_count; i++)
	{
		int recv_bytes_count = 0;
		WRITE_FMT_LOG(LOG_DEBUG, "[%d] recv array -->", nodeid);
		struct packet_data_t t;

		bytes=read( fdr, (char*)&t, sizeof(t) );
		WRITE_FMT_LOG(LOG_DEBUG, "r packet_data_t bytes=%d", bytes);
		WRITE_FMT_LOG(LOG_ERR, "asert t.type=%d %d(EPACKET_RANGE)", t.type, EPACKET_RANGE);
		assert(t.type == EPACKET_RANGE);
		bytes=write(fdw, &reply, 1);
		WRITE_FMT_LOG(LOG_DEBUG, "w reply bytes=%d", bytes);
		WRITE_FMT_LOG(LOG_DEBUG, "[%d]Send reply to %s", nodeid, writef);
		read( fdr, (char*)&dst_array[recv_bytes_count/sizeof(BigArrayItem)], t.size );
		recv_bytes_count += t.size;
		WRITE_FMT_LOG(LOG_DEBUG, "[%d]--size=%d OK\n", nodeid, (int)t.size );
		write(fdw, &reply, 1);
		WRITE_FMT_LOG(LOG_DEBUG, "[%d]Send reply to %s", nodeid, writef);
	}
#endif

	close(fdr);
	close(fdw);
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] channel_receive_sorted_ranges OK\n", nodeid );
}

/*writing data:
 * 1x struct sort_result*/
void
write_sort_result( const char *writef, int nodeid, BigArrayPtr sorted_array, int len ){
	if ( !len ) return;
	int fdw = open(writef, O_WRONLY);

	uint32_t sorted_crc = array_crc( sorted_array, ARRAY_ITEMS_COUNT );
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] send_sort_result: min=%d, max=%d, crc=%u", nodeid,
			sorted_array[0], sorted_array[len-1], sorted_crc );
	struct sort_result result;
	result.nodeid = nodeid;
	result.min = sorted_array[0];
	result.max = sorted_array[len-1];
	result.crc = sorted_crc;
	write(fdw, &result, sizeof(result));
	close( fdw );
}


