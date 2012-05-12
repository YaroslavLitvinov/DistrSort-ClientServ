/*
 * comm_src.c
 *
 *  Created on: 30.04.2012
 *      Author: yaroslav
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
	WRITE_FMT_LOG(LOG_NET, "[%d] Read ranges from %s\n", nodeid, readf);
	char reply='-';
	for (int i=0; i < ranges_count; i++)
	{
		int recv_bytes_count = 0;
		WRITE_FMT_LOG(LOG_NET, "[%d] recv array -->", nodeid);
		struct packet_data_t t;
		bytes=read( fdr, (char*)&t, sizeof(t) );
		WRITE_FMT_LOG(LOG_DEBUG, "r packet_data_t bytes=%d", bytes);
		WRITE_FMT_LOG(LOG_ERR, "asert t.type=%d %d(EPACKET_RANGE)", t.type, EPACKET_RANGE);
		assert(t.type == EPACKET_RANGE);
		bytes=write(fdw, &reply, 1);
		WRITE_FMT_LOG(LOG_DEBUG, "w reply bytes=%d", bytes);
		WRITE_FMT_LOG(LOG_NET, "[%d]Send reply to %s", nodeid, writef);
#ifdef FUSE_CLIENT
		while(recv_bytes_count < t.size){
			int packsize = min(t.size-recv_bytes_count, FUSE_IO_MAX_CHUNK_SIZE);
			int readed = read( fdr, (char*)&dst_array[recv_bytes_count/sizeof(BigArrayItem)], packsize );
			WRITE_FMT_LOG(LOG_DEBUG, "r dst_array bytes=%d", readed);
			assert(readed>0);
			recv_bytes_count += readed;
			WRITE_FMT_LOG(LOG_NET, "readed %d bytes, all=%d\n", recv_bytes_count, (int)t.size );
			bytes=write(fdw, &reply, 1);
			WRITE_FMT_LOG(LOG_DEBUG, "w reply bytes=%d", bytes);
			WRITE_FMT_LOG(LOG_NET, "[%d]Send reply to %s", nodeid, writef);
		}
#else
		read( fdr, (char*)&dst_array[recv_bytes_count/sizeof(BigArrayItem)], t.size );
		recv_bytes_count += t.size;
		WRITE_FMT_LOG(LOG_NET, "[%d]--size=%d OK\n", nodeid, (int)t.size );
		write(fdw, &reply, 1);
		WRITE_FMT_LOG(LOG_NET, "[%d]Send reply to %s", nodeid, writef);
#endif
	}
	close(fdr);
	close(fdw);
	WRITE_FMT_LOG(LOG_NET, "[%d] channel_receive_sorted_ranges OK\n", nodeid );
}

/*writing data:
 * 1x struct sort_result*/
void
write_sort_result( const char *writef, int nodeid, BigArrayPtr sorted_array, int len ){
	if ( !len ) return;
	int fdw = open(writef, O_WRONLY);

	uint32_t sorted_crc = array_crc( sorted_array, ARRAY_ITEMS_COUNT );
	WRITE_FMT_LOG(LOG_NET, "[%d] send_sort_result: min=%d, max=%d, crc=%u", nodeid,
			sorted_array[0], sorted_array[len-1], sorted_crc );
	struct sort_result result;
	result.nodeid = nodeid;
	result.min = sorted_array[0];
	result.max = sorted_array[len-1];
	result.crc = sorted_crc;
	write(fdw, &result, sizeof(result));
	close( fdw );
}


