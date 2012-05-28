/*
 * sqluse_srv.h
 *
 *  Created on: 02.05.2012
 *      Author: YaroslavLitvinov
 * This is channels configuration reading engine used by server side;
 * All DB data represented by strings, and can be copied into data structure unchanged;
 * But endpoint column string data can be used as format string for sprintf if %d format parameter is found,
 * and db_records_t::cid as format value, contains properly value setted up before request;
 * Some columns should converted into internal representation as enum or integer;
 * The main selective parameter is nodename, all records contains same nodename are related to same node.
 */

#ifndef SQLUSE_CLI_H_
#define SQLUSE_CLI_H_

struct db_records_t{
	struct db_record_t *array;
	int count;
	int maxcount;
	int cid; /*it's contains unique client id and used to post processing db data*/
};

#define FILE_TYPE_STD "std\0"
#define FILE_TYPE_MSQ "msq\0"


#define METHOD_BIND "bind\0"
#define METHOD_CONNECT "connect\0"

#define PUSH "PUSH\0"
#define PULL "PULL\0"
#define REQ  "REQ\0"
#define REP  "REP\0"

enum { ECOL_NODENAME=0, ECOL_FTYPE, ECOL_SOCK, ECOL_METHOD, ECOL_ENDPOINT, ECOL_FMODE, ECOL_FPATH, ECOL_FD,
	ECOL_COLUMNS_COUNT};



struct db_record_t{
	char *nodename;
	int ftype;
	int sock;
	int method;
	char *endpoint;
	char fmode;
	char *fpath;
	int fd;
};

enum {DB_RECORDS_GRANULARITY=20};
/*search record by file descriptor*/
struct db_record_t* match_db_record_by_fd(struct db_records_t *records, int fd);

/* In use case it's should not be directly called, but only as callback for sqlite API;*/
int get_dbrecords_callback(void *file_records, int argc, char **argv, char **azColName);

/*Issue db request.
 * @param path DB filename
 * @param nodename which records are needed
 * @param db_records data structure to get results, should be only valid pointer*/
int get_all_records_from_dbtable(const char *path, const char *nodename, struct db_records_t *dbrecords);

#endif /* SQLUSE_CLI_H_ */
