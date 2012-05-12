/*
 * zmq_netw.h
 *
 *  Created on: 03.05.2012
 *      Author: yaroslav
 */

#ifndef ZMQ_NETW_H_
#define ZMQ_NETW_H_

#include "sqluse_srv.h"
#include <stddef.h>
#include <sys/types.h>


#define max(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

#define min(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })


enum { EMETHOD_UNEXPECTED=-1, EMETHOD_BIND=0, EMETHOD_CONNECT=1 };


struct tempbuf_t{
	int pos;
	char *buf;
	size_t size;
};

struct sock_file_t{
	void *netw_socket;
	int sock_type;
	int fs_fd;
	struct tempbuf_t *tempbuf;
	int unused; /*used by zeromq_pool*/
};

enum { ESOCKF_ARRAY_GRANULARITY=10 };
struct zeromq_pool{
	void *context;
	struct sock_file_t* sockf_array;
	int count_max;
};
void init_zeromq_pool(struct zeromq_pool * zpool);
void add_sockf_copy_to_array(struct zeromq_pool* zpool, struct sock_file_t* sockf);
void remove_sockf_from_array_by_fd(struct zeromq_pool* zpool, int fd);
size_t sockf_array_items_count(const struct zeromq_pool* zpool);

/*find*/
struct sock_file_t* sockf_by_fd(struct zeromq_pool* zpool, int fd);
/*open*/
struct sock_file_t* open_sockf(struct zeromq_pool* zpool, struct db_records_t *db_records, int fd);
/*close*/
void close_sockf(struct zeromq_pool* zpool, struct sock_file_t *sockf);
/*stream write*/
ssize_t write_sockf(struct sock_file_t *sockf, const char *buf, size_t count);
/*stream write*/
ssize_t read_sockf(struct sock_file_t *sockf, char *buf, size_t count);
void zeromq_term(struct zeromq_pool* zpool);


#endif /* ZMQ_NETW_H_ */
