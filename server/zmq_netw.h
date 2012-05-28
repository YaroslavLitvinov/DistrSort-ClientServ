/*
 * zmq_netw.h
 *
 *  Created on: 03.05.2012
 *      Author: YaroslavLitvinov
 * Server side code.
 * This communication engine is implementation of channel communications;
 * It's code can be used unchanged for interprocess and tcp communications because networking configuration
 * is completely described by db_records data;
 * It's used by nodes as layer between file I/O and networking where write_sockf, read_sockf
 * main input output routines. Every communication socket (sock_file_t) is related to own fd - file descriptor;
 * It uses Zero MQ messaging library, and supported next communication patterns:
 * PUSH-PULL It uses generic single direction sockets, PUSH writes PULL reads data;
 * For every PUSH/PULL zeromq socket is associated single sock_file_t;
 * REQ-REP use dual direction zeromq sockets; REQ writes, reads data; REP reads/writes data;
 * For every REQ/REP socket is exist two struct sock_file_t one for reading another for writing;
 * Pair of sock_file_t file sockets related to single REQ/REP zmq socket, and should be used
 * sequently: one read, one write. After reading, data should be wrote into file socket and vice versa;
 * Either zmq will raise errors;
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

/*ZeroMQ related sockets open methods; Generally BIND allow to receive data, CONNECT to send;
 *Working pair of communicate sockets: one uses BIND, another CONNECT*/
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

/*find sock in array*/
struct sock_file_t* sockf_by_fd(struct zeromq_pool* zpool, int fd);
/*add sock to array*/
int add_sockf_copy_to_array(struct zeromq_pool* zpool, struct sock_file_t* sockf);
/*remove sock from array*/
int remove_sockf_from_array_by_fd(struct zeromq_pool* zpool, int fd);

/*init zmq, init array of sockets*/
int init_zeromq_pool(struct zeromq_pool * zpool);
/*free zmq resources, and sockets array*/
int zeromq_term(struct zeromq_pool* zpool);

/*search already opened dual direction socket associated with given fd;
 *Dual direction socket is linked with two communication channels; One for reading another for writing;
 *Both of these are using the same zmq socket internally, that can be closed only if both communication
 *channels are tryed to close by close_sockf;
 *@return NULL if no socket is associated with fd, else return associated socket;*/
struct sock_file_t* get_dual_sockf(struct zeromq_pool* zpool, struct db_records_t *db_records, int fd);

/*open file socket is associated with fd; It's should be called close_sockf for every opened file socket;
 * @param zpool should be valid and preconfigured by init_zeromq_pool
 * @param db_records should contains complete db
 * @return opened file socket object, owned by zpool; return NULL if no fd found in db_records*/
struct sock_file_t* open_sockf(struct zeromq_pool* zpool, struct db_records_t *db_records, int fd);

/*close file socket, opened by open_sockf; it's removing it from zpool and free resources;
 * @return err*/
int close_sockf(struct zeromq_pool* zpool, struct sock_file_t *sockf);

/*stream write file socket*/
ssize_t write_sockf(struct sock_file_t *sockf, const char *buf, size_t count);
/*stream read file socket*/
ssize_t read_sockf(struct sock_file_t *sockf, char *buf, size_t count);

/*open all sockets connections; It can be used instead explicitly opening of file sockets;
 *Use close_all_comm_files to close file sockets;  */
int open_all_comm_files(struct zeromq_pool* zpool, struct db_records_t *db_records);
/*Close all connections; used in pair with open_all_comm_files*/
int close_all_comm_files(struct zeromq_pool* zpool);

#endif /* ZMQ_NETW_H_ */
