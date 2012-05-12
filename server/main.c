/*
 * main.c
 *
 *  Created on: 16.04.2012
 *      Author: YaroslavLitvinov
 */

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include "defines.h"
#include "logfile.h"
#include "sqluse_srv.h"
#include "fs_inmem.h"
#include "zmq_netw.h"
#include <zmq.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
	if ( argc > 6 ){
		struct db_records_t *db_records = malloc(sizeof(struct db_records_t));
		db_records->cid = atoi( argv[argc-1] );
		char logname[50];
		sprintf(logname, "%s%s%d.log", SERVER_LOG, argv[argc-2], db_records->cid);
		OPEN_LOG(logname, argv[argc-2], db_records->cid);
		int err = get_all_records_from_dbtable(SERVER_DB_PATH, argv[argc-2], db_records);
		WRITE_FMT_LOG(LOG_ERR, "get_all_records_from_dbtable err=%d", err);
		if ( err != 0 ) return 1;
		else{
			WRITE_FMT_LOG(LOG_UI, "started server %s, nodeid=%d", argv[argc-2], db_records->cid );
			return run_fuse_main(db_records, argc-2, argv);
		}
	}
	else{
		/* FUSE options:
		 * -odirect_io option to get read, write call same as user called, but not by 4096bytes by FUSE layer
		 * -s no multithread support
		 * -d /path/yourpathtofs setup mount dir*/
		printf("usage: FUSE first args'-odirect_io -s -d /path/yourpathtofs', \
and two last args should be nodename key for DB: source, dest, manager, etc..; and a integer node id \n");fflush(0);
		return 1;
	}
	return 0; //should never reached
}


