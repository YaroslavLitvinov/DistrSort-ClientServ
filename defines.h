/*
 * defines.h
 *
 *  Created on: 08.05.2012
 *      Author: YaroslavLitvinov
 */

#ifndef DEFINES_H_
#define DEFINES_H_


#define CLIENT_LOG "log/client\0"
#define SERVER_LOG "log/server\0"
#define SOURCE_FILE_FMT "data/%dinput.data"
#define DEST_FILE_FMT "data/%doutput.data"

#define DB_PATH "client_dsort.db\0"
#define SERVER_DB_PATH "server_dsort.db"

#define MANAGER "manager\0"
#define SOURCE "source\0"
#define DEST "dest\0"

/*FUSE support, data flow between nodes is changed: big array data was divided
 * into smaller data packets, with size FUSE_IO_MAX_CHUNK_SIZE*/
#define FUSE_CLIENT
#ifdef FUSE_CLIENT
	#define FUSE_IO_MAX_CHUNK_SIZE (100*1024)
#endif

#define ARRAY_ITEMS_COUNT 1000000
#define SRC_NODES_COUNT 5

#define FIRST_SOURCE_NODEID (2)
#define LAST_SOURCE_NODEID (FIRST_SOURCE_NODEID+SRC_NODES_COUNT-1)
#define FIRST_DEST_NODEID (52)
#define LAST_DEST_NODEID (FIRST_DEST_NODEID+SRC_NODES_COUNT-1)

#define MANAGER_FD_READ_HISTOGRAM 3
#define MANAGER_FD_WRITE_D_HISTOGRAM_REQ 4
#define MANAGER_FD_READ_D_HISTOGRAM_REQ 54
#define MANAGER_FD_WRITE_RANGE_REQUEST 104
#define MANAGER_FD_READ_SORT_RESULTS 154
#define MANAGER_FD_READ_CRC 155

#define SOURCE_FD_WRITE_HISTOGRAM 3
#define SOURCE_FD_READ_D_HISTOGRAM_REQ 4
#define SOURCE_FD_WRITE_D_HISTOGRAM_REQ 5
#define SOURCE_FD_READ_SEQUENCES_REQ 6
#define SOURCE_FD_WRITE_SORTED_RANGES_REQ 7
#define SOURCE_FD_READ_SORTED_RANGES_REQ 57
#define SOURCE_FD_WRITE_CRC 107

#define DEST_FD_READ_SORTED_RANGES_REP 3
#define DEST_FD_WRITE_SORTED_RANGES_REP 4
#define DEST_FD_WRITE_SORT_RESULT 5

#endif /* DEFINES_H_ */
