/*
 * Tests for db configuration read
 *  Author: YaroslavLitvinov
 *  Date: 7.03.2012
 */

#include <limits.h>
#include <stdlib.h>

#include "gtest/gtest.h"
#include "errcodes.h"
#include "defines.h"
extern "C" {
#include "logfile.h"
#include "sqluse_cli.h"
}

#define TEST_VFS_PATH "/tmp"
#define TEST_FTYPE_MSQ "msq\0"
#define TEST_NODENAME "test\0"
#define TEST_ENDPOINT "ipc://test\0"
#define TEST_FPATH "/in/file\0"
#define TEST_FD "3"
#define TEST_MODE_W "w\0"


// Test harness for routines in sqluse_srv.c
class SqlUseCliTests : public ::testing::Test {
public:
	SqlUseCliTests(){}
};


static int
records_comparator( const void *m1, const void *m2 )
{
	const struct file_record_t *t1= (const struct file_record_t* )(m1);
	const struct file_record_t *t2= (const struct file_record_t* )(m2);

	if ( t1->fd < t2->fd )
		return -1;
	else if ( t1->fd > t2->fd )
		return 1;
	else return 0;
	return 0;
}



TEST_F(SqlUseCliTests, TestReadDbItems) {
	struct file_records_t file_records;
	memset(&file_records, '\0', sizeof(struct file_records_t));
	file_records.vfs_path = (char*)TEST_VFS_PATH;
	/*load from DB all records related to manager node*/
	WRITE_LOG(LOG_DEBUG, "testing source node records");
	EXPECT_EQ( ERR_OK, get_all_files_from_dbtable(DB_PATH, "source", &file_records) );
	/*sort db records to verify that file descriptors are unique*/
	qsort( file_records.array, file_records.count, sizeof(struct file_record_t), records_comparator );
	/*test sorted records*/
	for (int i=0; i < file_records.count-1; i++){
		EXPECT_LT( file_records.array[i].fd, file_records.array[i+1].fd );
	}
	EXPECT_NE(0, file_records.count);
	/*free array, reset mem*/
	free(file_records.array);
	memset(&file_records, '\0', sizeof(struct file_records_t));
	file_records.vfs_path = (char*)TEST_VFS_PATH;

	/*load from DB all records related to manager node*/
	WRITE_LOG(LOG_DEBUG, "testing manager node records");
	EXPECT_EQ( ERR_OK, get_all_files_from_dbtable(DB_PATH, "manager", &file_records) );
	/*sort db records to verify that file descriptors are unique*/
	qsort( file_records.array, file_records.count, sizeof(struct file_record_t), records_comparator );
	/*test sorted records*/
	for (int i=0; i < file_records.count-1; i++){
		EXPECT_LT( file_records.array[i].fd, file_records.array[i+1].fd );
	}
	EXPECT_NE(0, file_records.count);
	/*free array, reset mem*/
	free(file_records.array);
	memset(&file_records, '\0', sizeof(struct file_records_t));
	file_records.vfs_path = (char*)TEST_VFS_PATH;

	/*load from DB all records related to destination node*/
	WRITE_LOG(LOG_DEBUG, "testing dest node records");
	EXPECT_EQ( ERR_OK, get_all_files_from_dbtable(DB_PATH, "dest", &file_records) );
	/*sort db records to verify that file descriptors are unique*/
	qsort( file_records.array, file_records.count, sizeof(struct file_record_t), records_comparator );
	/*test sorted records*/
	for (int i=0; i < file_records.count-1; i++){
		EXPECT_LT( file_records.array[i].fd, file_records.array[i+1].fd );
	}
	EXPECT_NE(0, file_records.count);

	EXPECT_EQ( (struct file_record_t*)NULL, match_file_record_by_fd(&file_records, -1) );
	WRITE_LOG(LOG_DEBUG, "TestReadDbItems OK");
}



TEST_F(SqlUseCliTests, TestDbItemsParser) {
	struct file_records_t file_records;
	file_records.maxcount = FILE_RECORDS_GRANULARITY;
	file_records.array = (struct file_record_t*) malloc( sizeof(struct file_record_t)*file_records.maxcount );
	file_records.count = 0;
	char **row_values_array = (char**)malloc(sizeof(char*)*ECOL_COUNT);
	row_values_array[ECOL_NODENAME] = (char*)TEST_NODENAME;
	row_values_array[ECOL_FTYPE] = (char*)TEST_FTYPE_MSQ;
	row_values_array[ECOL_FMODE] = (char*)TEST_MODE_W;
	row_values_array[ECOL_FPATH] = (char*)TEST_FPATH;
	row_values_array[ECOL_FD] = (char*)TEST_FD;
	get_filerecords_callback( &file_records, ECOL_COUNT, row_values_array, NULL);
	ASSERT_TRUE( 1==file_records.count );
}


int main(int argc, char **argv) {
	OPEN_LOG( "log/sqluse_cli_test.log", "", 0);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
