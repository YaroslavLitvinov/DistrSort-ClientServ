/*
 * sqluse_cli.c
 *
 *  Created on: 02.05.2012
 *      Author: YaroslavLitvinov
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
	if (!records || !records->array) return NULL;
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
		//if need to expand array
		if ( records->count >= records->maxcount ){
			records->maxcount += FILE_RECORDS_GRANULARITY;
			records->array = realloc(records->array, sizeof(struct file_record_t)*records->maxcount);
		}
		frecord = &records->array[records->count++];

		/*loop by table columns count*/
		for(int i=0; i<argc; i++){
			const char *col_value = argv[i];
			int len_value = strlen(col_value);
			switch(i){
			case ECOL_NODENAME:
				frecord->nodename = malloc( len_value+1 );
				strcpy(frecord->nodename, col_value);
				frecord->nodename[len_value] = '\0';
				break;
			case ECOL_FTYPE:
				strncpy(frecord->ftype, col_value, 3);
				frecord->ftype[3] = '\0';
				break;
			case ECOL_FMODE:
				strncpy(&(frecord->fmode), col_value, 1);
				break;
			case ECOL_FPATH:
				frecord->fpath = malloc( strlen(records->vfs_path) + len_value+1 );
				strcpy(frecord->fpath, records->vfs_path );
				strcat(frecord->fpath, argv[i]);
				strcat(frecord->fpath, "\0");
				break;
			case ECOL_FD:
				frecord->fd = atoi(col_value);
				break;
			default:
				break;
			}
		}
		WRITE_FMT_LOG(LOG_DEBUG, "nodename:%s, ftype=%s, fmode=%c, fpath=%s, fd=%d\n",
				frecord->nodename, frecord->ftype, frecord->fmode, frecord->fpath, frecord->fd);
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

	frecords->maxcount = FILE_RECORDS_GRANULARITY;
	frecords->array = malloc( sizeof(struct file_record_t)*frecords->maxcount );
	frecords->count = 0;
	char sqlstr[100];
	sprintf(sqlstr, "select * from files where nodename='%s';", nodename);
	rc = sqlite3_exec(db, sqlstr, get_filerecords_callback, frecords, &zErrMsg);
	if ( SQLITE_OK!=rc ){
		WRITE_FMT_LOG(LOG_ERR, "db request failed err=%d", rc);
		assert( SQLITE_OK==rc );
	}
	sqlite3_free(zErrMsg);
	sqlite3_close(db);
	return 0;
}

