/*
 * sqluse_cli.h
 *
 *  Created on: 02.05.2012
 *      Author: yaroslav
 */

#ifndef SQLUSE_CLI_H_
#define SQLUSE_CLI_H_

struct db_records_t{
	struct db_record_t *array;
	int count;
	int maxcount;
	int cid; /*it's contains unique client id and used to post processing db data*/
};

enum { ECOL_NODENAME=0, ECOL_FTYPE, ECOL_SOCK, ECOL_METHOD, ECOL_ENDPOINT, ECOL_FMODE, ECOL_FPATH, ECOL_FD};

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
struct db_record_t* match_db_record_by_fd(struct db_records_t *records, int fd);
int get_dbrecords_callback(void *file_records, int argc, char **argv, char **azColName);
int get_all_records_from_dbtable(const char *path, const char *nodename, struct db_records_t *dbrecords);

#endif /* SQLUSE_CLI_H_ */
