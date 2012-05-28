/*
 * manager.c
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */

#include "sqluse_cli.h"
#include <zmq.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DB_PATH "client_dsort.db"

#define TEST1_FD_PUSH 10
#define TEST1_FD_PULL 11
#define TEST2_FD_REQ_SEND 12
#define TEST2_FD_REQ_RECV 13
#define TEST2_FD_REP_RECV 14
#define TEST2_FD_REP_SEND 15


static int send_primitive(const char *testname, struct file_records_t *file_records, int fdtest, const char *str){
	struct file_record_t* test_record = match_file_record_by_fd( file_records, fdtest);
	assert(test_record);
	int fd = open(test_record->fpath, O_WRONLY);
	printf("%s: writing data %s, %d bytes...\n", testname, str, (int)strlen(str));fflush(0);
	int bytes = write(fd, str, strlen(str));
	printf("%s: wrote %d bytes\n", testname, bytes);fflush(0);
	close(fd);
	return bytes;
}

static char* recv_primitive(const char *testname, struct file_records_t *file_records, int fdtest, int count){
	struct file_record_t* test_record = match_file_record_by_fd( file_records, fdtest);
	assert(test_record);
	int fd = open(test_record->fpath, O_RDONLY);
	char *test_data = malloc( count+1 );
	memset(test_data, '\0', count+1);
	printf("%s: reading %d bytes...\n", testname, count );fflush(0);
	int bytes = read( fd, test_data, count );
	//int bytes = read(fd, test_data, count);
	printf("%s: readed data:%s, %d bytes\n", testname, test_data, bytes);fflush(0);
	close(fd);
	return test_data;
}

int main(int argc, char *argv[]){
	struct file_records_t file_records;
	if ( argc > 2 ){
		file_records.vfs_path = argv[1];
		int err = get_all_files_from_dbtable(DB_PATH, argv[2], &file_records);
		if ( err != 0 ) return 1;
	}
	else{
		printf("usage: arg1 should be path to VFS folder, \
arg2 testname{testPUSH,testPULL,testREQ,testREP,...}\n");fflush(0);
		return 1;
	}

	const char *testname = argv[2];
	const char *teststr = "Hello world!\0";
	int testlen = strlen(teststr);
	if ( !strcmp(testname, "test1PUSH") ){
		int send_bytes = send_primitive(testname, &file_records, TEST1_FD_PUSH, teststr);
		(void)send_bytes;
	}
	else if ( !strcmp(testname, "test1PULL") ){
		char *recv_str = recv_primitive(testname, &file_records, TEST1_FD_PULL, testlen);
		(void)recv_str;
	}
	else if ( !strcmp(testname, "test2REQ") ){
		struct file_record_t* w_record = match_file_record_by_fd( &file_records, TEST2_FD_REQ_SEND);
		assert(w_record);
		int wfd = open(w_record->fpath, O_WRONLY);
		printf("%s: writing data %s, %d bytes...\n", testname, teststr, testlen);fflush(0);
		int wbytes = write(wfd, teststr, testlen);
		printf("%s: wrote %d bytes\n", testname, wbytes);fflush(0);

		struct file_record_t* test_record = match_file_record_by_fd( &file_records, TEST2_FD_REQ_RECV);
		assert(test_record);
		int rfd = open(test_record->fpath, O_RDONLY);
		/*testcase: divide recv for 2 parts, to test receive single recv into 2 parts */

		//recv part1
		int len1 = 4;
		char *test_data1 = malloc( len1+1 );
		memset(test_data1, '\0', len1+1);
		printf("%s: reading %d bytes...\n", testname, len1 );fflush(0);
		int rbytes1 = read( rfd, test_data1, len1 );
		printf("%s: readed data:%s, %d bytes\n", testname, test_data1, rbytes1);fflush(0);
		//recv part2
		int len2 = 4;
		char *test_data2 = malloc( len2+1 );
		memset(test_data2, '\0', len2+1);
		printf("%s: reading %d bytes...\n", testname, len2 );fflush(0);
		int rbytes2 = read( rfd, test_data2, len2 );
		printf("%s: readed data:%s, %d bytes\n", testname, test_data2, rbytes2);fflush(0);
		//recv part3
		int len3 = testlen - len1 - len2;
		char *test_data3 = malloc( len3+1 );
		memset(test_data3, '\0', len3+1);
		printf("%s: reading %d bytes...\n", testname, len3 );fflush(0);
		int rbytes3 = read( rfd, test_data3, len3 );
		printf("%s: readed data:%s, %d bytes\n", testname, test_data3, rbytes3);fflush(0);
		close(rfd);
		close(wfd);
	}
	else if ( !strcmp(testname, "test2REP") ){
		struct file_record_t* test_record = match_file_record_by_fd( &file_records, TEST2_FD_REP_RECV);
		assert(test_record);
		int rfd = open(test_record->fpath, O_RDONLY);
		char *test_data = malloc( testlen+1 );
		memset(test_data, '\0', testlen+1);
		printf("%s: reading %d bytes...\n", testname, testlen );fflush(0);
		int rbytes = read( rfd, test_data, testlen );
		//int bytes = read(fd, test_data, count);
		printf("%s: readed data:%s, %d bytes\n", testname, test_data, rbytes);fflush(0);

		struct file_record_t* w_record = match_file_record_by_fd( &file_records, TEST2_FD_REP_SEND);
		assert(w_record);
		int wfd = open(w_record->fpath, O_WRONLY);
		printf("%s: writing data %s, %d bytes...\n", testname, teststr, testlen);fflush(0);
		int wbytes = write(wfd, teststr, testlen);
		printf("%s: wrote %d bytes\n", testname, wbytes);fflush(0);

		close(rfd);
		close(wfd);

	}
	else if ( !strcmp(testname, "test-PUSH-PULL") ){
		const char *testname = "selftest\0";
		void *context = zmq_init(1);
		void *writer = zmq_socket(context, ZMQ_PUSH);
		assert( !zmq_connect(writer, "ipc:///tmp/self1") );

		printf("%s: writing data %s, %d bytes...\n", testname, teststr, (int)testlen);fflush(0);
		zmq_msg_t msg;
		zmq_msg_init_size (&msg, testlen);
		memcpy (zmq_msg_data (&msg), teststr, testlen);
		zmq_send (writer, &msg, 0);
		zmq_msg_close (&msg);
		printf("%s: wrote %d bytes\n", testname, testlen);fflush(0);

		void *reader = zmq_socket(context, ZMQ_PULL);
		assert( !zmq_bind(reader, "ipc:///tmp/self1") );

		printf("%s: recv data...\n", testname );fflush(0);
		zmq_msg_t msg2;
		zmq_msg_init(&msg2);
		zmq_recv (reader, &msg2, 0);
		const size_t msg_size = zmq_msg_size(&msg2);
		char *buf = malloc(msg_size+1);
		buf[msg_size] = '\0';
		memcpy( buf, zmq_msg_data (&msg2), msg_size);
		zmq_msg_close (&msg2);
		printf("%s: received data:%s, %d bytes\n", testname, buf, (int)msg_size);fflush(0);
		zmq_close(reader);
		zmq_close(writer);
		zmq_term(context);
	}
	else{
		printf("Unknown test case:%s\n", testname);fflush(0);
	}

	return 0;
}
