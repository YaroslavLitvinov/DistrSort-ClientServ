/*
 * sqluse_srv.c
 *
 *  Created on: 02.05.2012
 *      Author: YaroslavLitvinov
 */

#include "sqluse_srv.h"
#include "zmq_netw.h"
#include "fs_inmem.h"
#include "logfile.h"
#include "errcodes.h"

#include <zmq.h>
#include <sqlite3.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


struct db_record_t* match_db_record_by_fd(struct db_records_t *records, int fd){
	if ( !records ) return NULL;
	if ( !records->array ) return NULL;
	for ( int i=0; i < records->count; i++ ){
		if ( records->array[i].fd == fd )
			return &records->array[i];
	}
	return NULL;
}

/* In use case it's should not be directly called, but only as callback for sqlite API;
 *@param argc columns count
 *@param argv columns values */
int get_dbrecords_callback(void *file_records, int argc, char **argv, char **azColName){
	if ( file_records ){
		struct db_record_t *frecord = NULL;
		struct db_records_t *records = (struct db_records_t *) file_records;
		//if need to expand array
		if ( records->count >= records->maxcount ){
			records->maxcount += DB_RECORDS_GRANULARITY;
			records->array = realloc(records->array, sizeof(struct db_record_t)*records->maxcount);
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
				if ( !strcmp(col_value, FILE_TYPE_STD) ) frecord->ftype = EFILE_STD;
				else if ( !strcmp(col_value, FILE_TYPE_MSQ) ) frecord->ftype = EFILE_MSQ;
				else { frecord->ftype = EFILE_USER; }
				break;
			case ECOL_SOCK:
				if ( !strcmp(col_value, PUSH) ) frecord->sock = ZMQ_PUSH;
				else if ( !strcmp(col_value, PULL) ) frecord->sock = ZMQ_PULL;
				else if ( !strcmp(col_value, REQ) ) frecord->sock = ZMQ_REQ;
				else if ( !strcmp(col_value, REP) ) frecord->sock = ZMQ_REP;
				else frecord->sock = -1;
				break;
			case ECOL_METHOD:
				if ( !strcmp(col_value, METHOD_BIND) )
					frecord->method = EMETHOD_BIND;
				else if ( !strcmp(col_value, METHOD_CONNECT) )
					frecord->method = EMETHOD_CONNECT;
				else frecord->method = EMETHOD_UNEXPECTED;
				break;
			case ECOL_ENDPOINT:
				if ( strstr(col_value, "%d\0") > 0 ){
					//needs to be postprocessed
					frecord->endpoint = malloc( len_value+20 );
					memset(frecord->endpoint, '\0', len_value+20);
					sprintf(frecord->endpoint, col_value, records->cid);
				}
				else{
					frecord->endpoint = malloc( len_value+1 );
					strcpy(frecord->endpoint, col_value);
					frecord->endpoint[len_value] = '\0';
				break;
			}
			case ECOL_FMODE:
				strncpy(&(frecord->fmode), col_value, 1);
				break;
			case ECOL_FPATH:
				frecord->fpath = malloc( len_value+1 );
				strcpy(frecord->fpath, col_value );
				frecord->fpath[len_value] = '\0';
				break;
			case ECOL_FD:
				frecord->fd = atoi( col_value );
				break;
			}
		}
		WRITE_FMT_LOG(LOG_DEBUG, "nodename:%s, ftype=%d, sock=%d, method=%d, endpoint=%s, fmode=%c, fpath=%s, fd=%d\n",
				frecord->nodename, frecord->ftype, frecord->sock, frecord->method,
				frecord->endpoint, frecord->fmode, frecord->fpath, frecord->fd);
	}
	return 0;
}

/*Issue db request.
 * @param path DB filename
 * @param nodename which records are needed
 * @param db_records data structure to get results, should be only valid pointer*/
int get_all_records_from_dbtable(const char *path, const char *nodename, struct db_records_t *db_records){
	if ( !path || !nodename || !db_records) return ERR_BAD_ARG;
	sqlite3 *db = NULL;
	char *zErrMsg = 0;
	int rc = sqlite3_open( path, &db);
	if( rc ){
		fprintf(stderr, "Can't open database file=%s, errtext %s, errcode=%d\n",
				path, sqlite3_errmsg(db), rc);
		sqlite3_close(db);
		return ERR_ERROR;
	}
	db_records->maxcount = DB_RECORDS_GRANULARITY;
	db_records->array = malloc( sizeof(struct db_record_t)*db_records->maxcount );
	db_records->count = 0;
	char sqlstr[100];
	sprintf(sqlstr, "select * from channels where nodename='%s';", nodename);
	int sqlite_db_request_err = sqlite3_exec(db, sqlstr, get_dbrecords_callback, db_records, &zErrMsg);
	if ( SQLITE_OK != sqlite_db_request_err ){
		WRITE_FMT_LOG(LOG_ERR, "Sql statement : %s, exec error text=%s, errcode=%d\n",
				sqlstr, sqlite3_errmsg(db), sqlite_db_request_err);
		return ERR_ERROR;
	}
	sqlite3_free(zErrMsg);
	sqlite3_close(db);
	return ERR_OK;
}



