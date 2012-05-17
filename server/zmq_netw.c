/*
 * zmq_netw.c
 *
 *  Created on: 03.05.2012
 *      Author: yaroslav
 */


#include "zmq_netw.h"
#include "sqluse_srv.h"
#include "logfile.h"
#include <zmq.h>

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


void init_zeromq_pool(struct zeromq_pool * zpool){
	assert(zpool);
	zpool->context = NULL;
	zpool->count_max=ESOCKF_ARRAY_GRANULARITY;
	zpool->sockf_array = malloc(zpool->count_max * sizeof(struct sock_file_t));
	for (int i=0; i < zpool->count_max; i++)
		zpool->sockf_array[i].unused = 1;
}

struct sock_file_t* sockf_by_fd(struct zeromq_pool* zpool, int fd){
	for (int i=0; i < zpool->count_max; i++)
		if ( !zpool->sockf_array[i].unused && zpool->sockf_array[i].fs_fd == fd )
			return &zpool->sockf_array[i];
	return 0;
}

size_t sockf_array_items_count(const struct zeromq_pool* zpool){
	size_t count=0;
	for (int i=0; i < zpool->count_max; i++)
		if ( !zpool->sockf_array[i].unused )
			count++;
	return count;
}


void add_sockf_copy_to_array(struct zeromq_pool* zpool, struct sock_file_t* sockf){
	assert(zpool);
	assert(sockf);
	if( sockf_by_fd(zpool, sockf->fs_fd) ) return;

	struct sock_file_t* sockf_add = NULL;
	do{
		/*search unused array cell*/
		for (int i=0; i < zpool->count_max; i++)
			if ( zpool->sockf_array[i].unused ){
				sockf_add = &zpool->sockf_array[i];
				break;
			}
		/*extend array if not found unused cell*/
		if ( !sockf_add ){
			zpool->count_max+=ESOCKF_ARRAY_GRANULARITY;
			zpool->sockf_array = realloc(zpool->sockf_array, zpool->count_max * sizeof(struct sock_file_t));
		}
	}while(!sockf_add);
	*sockf_add = *sockf;
	sockf_add->unused = 0;
}


void remove_sockf_from_array_by_fd(struct zeromq_pool* zpool, int fd){
	assert(zpool);
	for (int i=0; i < zpool->count_max; i++)
		if ( zpool->sockf_array[i].fs_fd == fd){
			zpool->sockf_array[i].unused = 1;
			break;
		}
}

struct sock_file_t* open_sockf(struct zeromq_pool* zpool, struct db_records_t *db_records, int fd){
	assert(db_records);
	struct sock_file_t *sockf = sockf_by_fd(zpool, fd );
	if ( sockf ) {
		WRITE_FMT_LOG(LOG_MISC, "Existing socket: Trying toopen twice? %d", sockf->fs_fd);
		return sockf;
	}
	else if ( !sockf ){
		/*socket is not exist, so search fd record in DB to get socket params*/
		struct db_record_t* db_record = match_db_record_by_fd( db_records, fd);
		if ( db_record ){
			/*create new sock_file_t*/
			sockf = malloc( sizeof(struct sock_file_t) );
			sockf->fs_fd = db_record->fd;
			sockf->sock_type = db_record->sock;
			sockf->tempbuf = NULL;

			/*search existing dual direction socket*/
			struct sock_file_t *dual_sockf = NULL;
			struct db_record_t* record1 = match_db_record_by_endpoint_mode( db_records, db_record->endpoint, 'w');
			struct db_record_t* record2 = match_db_record_by_endpoint_mode( db_records, db_record->endpoint, 'r');
			/*if found records with mode='r','w' with same endpoint, then use it as dual direction socket*/
			if ( record1 && record2 && record1->method == record2->method ){
				struct sock_file_t *sockf1 = sockf_by_fd(zpool, record1->fd );
				struct sock_file_t *sockf2 = sockf_by_fd(zpool, record2->fd );
				/*if found socket then use exiting zeromq socket*/
				if ( sockf1 )
					dual_sockf = sockf1;
				else if ( sockf2 )
					dual_sockf = sockf2;
			}
			/*should be only one zeromq socket for two files with the same endpoint
			 * (it's a trick for dual direction sockets)*/
			if ( dual_sockf ){
				WRITE_LOG(LOG_MISC, "zvm_open use dual socket");
				sockf->netw_socket = dual_sockf->netw_socket;
			}
			else{
				WRITE_FMT_LOG(LOG_NET, "open socket: %s, sock type %d\n", db_record->endpoint, db_record->sock);
				sockf->netw_socket = zmq_socket( zpool->context, db_record->sock );
				int err=-1;
				switch(db_record->method){
				case EMETHOD_BIND:
					WRITE_LOG(LOG_NET, "open socket: bind");
					err= zmq_bind(sockf->netw_socket, db_record->endpoint);
					if ( err != 0 ){
						WRITE_FMT_LOG(LOG_ERR, "zmq_bind err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
						assert(!err);
					}
					else{
						WRITE_LOG(LOG_ERR, "zmq_bind ok");
					}
					break;
				case EMETHOD_CONNECT:
					WRITE_LOG(LOG_NET, "open socket: connect");
					err = zmq_connect(sockf->netw_socket, db_record->endpoint);
					if ( err != 0 ){
						WRITE_FMT_LOG(LOG_NET, "zmq_connect err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
						assert(!err);
					}
					else{
						WRITE_LOG(LOG_ERR, "zmq_connect ok");
					}
					break;
				default:
					assert(0);
					break;
				}
			}
			add_sockf_copy_to_array(zpool, sockf);
		}
	}

	return sockf;
}


void close_sockf(struct zeromq_pool* zpool, struct sock_file_t *sockf){
	assert(zpool);

	int zmq_sock_is_used_twice = 0;
	for (int i=0; i < zpool->count_max; i++){
		if ( !zpool->sockf_array[i].unused && zpool->sockf_array[i].netw_socket == sockf->netw_socket )
			zmq_sock_is_used_twice++;
	}
	if ( zmq_sock_is_used_twice > 1 ){
		sockf->fs_fd = -1;
		sockf->netw_socket = NULL;
		if ( sockf->tempbuf ){
			free(sockf->tempbuf->buf), sockf->tempbuf->buf=NULL;
			free(sockf->tempbuf), sockf->tempbuf = NULL;
		}
		WRITE_LOG(LOG_NET, "dual direction zmq socket should be closed later");
	}else{
		WRITE_LOG(LOG_NET, "zmq socket closing...");
		int err = zmq_close( sockf->netw_socket );
		if ( err != 0 ){
			WRITE_FMT_LOG(LOG_NET, "zmq_close err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
			assert(!err);
		}
		else{
			WRITE_LOG(LOG_ERR, "zmq_close ok");
		}
	}
	remove_sockf_from_array_by_fd(zpool, sockf->fs_fd );
}


ssize_t  write_sockf(struct sock_file_t *sockf, const char *buf, size_t size){
	if ( sockf ){
		zmq_msg_t msg;
		zmq_msg_init_size (&msg, size);
		memcpy (zmq_msg_data (&msg), buf, size);
		WRITE_FMT_LOG(LOG_NET, "zmq_sending buf %d bytes...", (int)size );
		int err = zmq_send ( sockf->netw_socket, &msg, 0);
		if ( err != 0 ){
			WRITE_FMT_LOG(LOG_ERR, "zmq_send err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
			assert(!err);
		}
		else{
			WRITE_LOG(LOG_NET, "zmq_send ok");
		}
		zmq_msg_close (&msg);
		return size;
	}
	else return 0;
}


ssize_t read_sockf(struct sock_file_t *sockf, char *buf, size_t count){
	/*use sockf->tempbuf to save received results if user received more data than can be write into buf
	 * Data from temp buf should be readed at next call of read*/
	int bytes_read = 0;
	WRITE_FMT_LOG(LOG_NET, "count=%d", (int)count);
	if ( sockf->tempbuf && sockf->tempbuf->buf ){
		WRITE_FMT_LOG(LOG_NET, "read from tempbuf, size=%d, pos=%d", (int)sockf->tempbuf->size, sockf->tempbuf->pos  );
		/*how much of data can be copied, but not more than requested*/
		bytes_read = min( sockf->tempbuf->size-sockf->tempbuf->pos, count );
		/*copy to buf param from temp buffer starting from current pos*/
		memcpy (buf, sockf->tempbuf->buf+sockf->tempbuf->pos, bytes_read);
		/*update temp buf pos, increase for amount bytes read*/
		sockf->tempbuf->pos += bytes_read;
		/*temp buf completely readed*/
		if ( sockf->tempbuf->size <= sockf->tempbuf->pos ){
			free( sockf->tempbuf->buf ), sockf->tempbuf->buf = NULL;
			free( sockf->tempbuf ), sockf->tempbuf = NULL;
			if ( bytes_read < count ){
				/*rest of data should be readed from zmq socket*/
				count = count-bytes_read;
				buf = buf+bytes_read; /*set new offset just after wrote data*/
			}
			else
				return bytes_read;
		}
		else
			return bytes_read;
	}
	int bytes = 0;
	if ( sockf->netw_socket ){
		do{
			zmq_msg_t msg;
			zmq_msg_init (&msg);
			WRITE_FMT_LOG(LOG_NET, "zmq_recv %d bytes...", (int)count);
			int err = zmq_recv ( sockf->netw_socket, &msg, 0);
			if ( 0 != err ){
				/*read error*/
				WRITE_FMT_LOG(LOG_NET, "zmq_recv err %d, errno %d, status %s", err, zmq_errno(), zmq_strerror(zmq_errno()) );
				return 0;
			}
			/*read ok*/
			size_t msg_size = zmq_msg_size (&msg);
			WRITE_FMT_LOG(LOG_ERR, "zmq_recv %d bytes ok\n", (int)msg_size);
			bytes = min( msg_size, count );
			void *recv_data = zmq_msg_data (&msg);
			memcpy (buf, recv_data, bytes);
			//if readed data less than received data, then store rest in tempbuf
			if ( bytes < msg_size ){
				sockf->tempbuf = malloc( sizeof(struct tempbuf_t) );
				sockf->tempbuf->buf = malloc( msg_size-bytes );
				memcpy( sockf->tempbuf->buf, recv_data+bytes, msg_size-bytes );
				sockf->tempbuf->size = msg_size-bytes;
				sockf->tempbuf->pos=0;
			}
		}while(bytes_read+bytes < count);
	}
	return bytes_read+bytes;
}

void zeromq_term(struct zeromq_pool* zpool){
	assert(zpool);
	/*destroy zmq context*/
	if (zpool->context){
		WRITE_LOG(LOG_ERR, "zmq_term trying...\n");fflush(0);
		int err= zmq_term(zpool->context);
		if ( err != 0 ){
			WRITE_FMT_LOG(LOG_ERR, "zmq_term err %d, errno %d errtext %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
		}
		else{
			WRITE_LOG(LOG_ERR, "zmq_term ok\n");
		}
	}
}

