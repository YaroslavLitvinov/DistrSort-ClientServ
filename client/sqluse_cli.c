/*
 * sqluse_cli.c
 *
 *  Created on: 02.05.2012
 *      Author: yaroslav
 */

#include "sqluse_cli.h"
#include "logfile.h"

#include <sqlite3.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



struct file_record_t* match_file_record_by_fd(struct file_records_t *records, int fd){
	assert(records);
	for ( int i=0; i < records->count; i++ ){
		if ( records->array[i].fd == fd )
			return &records->array[i];
	}
	return NULL;
}

int get_filerecords_callback(void *file_records, int argc, char **argv, char **azColName){
	if ( file_records ){
		struct file_record_t *frecord = NULL;
		struct file_records_t *records = (struct file_records_t *) file_records;
		if ( records->count < records->maxcount )
			frecord = &records->array[records->count++];
		else
			return 0;
		for(int i=0; i<argc; i++){
			switch(i){
			case ECOL_NODENAME:
				frecord->nodename = malloc( strlen(argv[i])+1 );
				strcpy(frecord->nodename, argv[i]);
				break;
			case ECOL_FTYPE:
				strncpy(frecord->ftype, argv[i], 4);
				break;
			case ECOL_FMODE:
				strncpy(&(frecord->fmode), argv[i], 1);
				break;
			case ECOL_FPATH:
				frecord->fpath = malloc( strlen(records->vfs_path) + strlen(argv[i])+1 );
				strcpy(frecord->fpath, records->vfs_path );
				strcat(frecord->fpath, argv[i]);
				break;
			case ECOL_FD:
				frecord->fd = atoi(argv[i]);
				break;
			}
		}
		printf("nodename:%s, ftype=%s, fmode=%c, fpath=%s, fd=%d\n",frecord->nodename, frecord->ftype, frecord->fmode,
				frecord->fpath, frecord->fd);
	}
	return 0;
}

int get_all_files_from_dbtable(const char *path, const char *nodename, struct file_records_t *frecords){
	sqlite3 *db = NULL;
	char *zErrMsg = 0;
	int rc = sqlite3_open( path, &db);
	if( rc ){
		WRITE_FMT_LOG(LOG_ERR, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}

	frecords->array = malloc( sizeof(struct file_record_t)*30 );
	frecords->maxcount = 30;
	frecords->count = 0;
	char sqlstr[100];
	sprintf(sqlstr, "select * from files where nodename='%s';", nodename);
	rc = sqlite3_exec(db, sqlstr, get_filerecords_callback, frecords, &zErrMsg);
	assert( SQLITE_OK==rc );
	sqlite3_free(zErrMsg);
	sqlite3_close(db);
	return 0;
}

