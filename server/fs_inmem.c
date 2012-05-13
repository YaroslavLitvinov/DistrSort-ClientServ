/*
 * fs_inmem.c
 *
 *  Created on: 20.04.2012
 *      Author: yaroslav
 */

#define FUSE_USE_VERSION 26

#include "logfile.h"
#include "fs_inmem.h"
#include "sqluse_srv.h"
#include "zmq_netw.h"
#include <zmq.h>

#define _FILE_OFFSET_BITS 64
#include <fuse.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>



#define ACS_ALLREAD  S_IRUSR|S_IRGRP|S_IROTH
#define ACS_ALLWRITE S_IWUSR|S_IWGRP|S_IWOTH

static struct fs_in_memory_t *__fs;
static struct zeromq_pool *__zmq_pool;
struct db_records_t *__db_records;

int callback_add_dir(const char *dirpath, int len){
	assert(__fs->fs_structure);
	if ( !len ) return -1;
	struct file_info_array_t *fs_struct = __fs->fs_structure;
	int found = 0;
	if ( fsfile_by_path(fs_struct, dirpath) )
		found=1;
	/*if dir path not found then add it*/
	if ( !found ){
		/*realloc array if became full*/
		if ( fs_struct->maxcount <= fs_struct->count ){
			/*alloc array*/
			if ( !fs_struct->array ){
				fs_struct->maxcount=EFS_ARAAY_GRANULARITY;
				fs_struct->array = malloc( fs_struct->maxcount*sizeof(struct file_info_t) );
			}
			/*realloc array*/
			else{
				fs_struct->maxcount+=EFS_ARAAY_GRANULARITY;
				fs_struct->array = realloc( fs_struct->array, fs_struct->maxcount*sizeof(struct file_info_t) );
			}
		}
		char *copypath = malloc((len+1)*sizeof(char));
		struct file_info_t *finfo = &fs_struct->array[fs_struct->count++];
		memcpy( copypath, dirpath, len );
		copypath[len] = '\0';
		finfo->path = copypath;
		finfo->access_mode = S_IFDIR|ACS_ALLREAD|ACS_ALLWRITE;
		return 0;
	}
	else
		return -1;
}

/*Search rightmost '/' and return substring*/
void process_subdirs_via_callback( int (*callback_add_dir)(const char *, int), const char *path, int len ){
	/*locate rightmost '/' inside path*/
	const char *dir = NULL;
	for(int i=len-1; i >=0; i--)
		if(path[i] == '/'){
			dir = &path[i];
			break;
		}
	if ( dir ){
		int subpathlen = dir-path;
		/*if root '/' should be processed */
		if ( dir[0]=='/' && len > 1 && !subpathlen ){
			subpathlen = 1;
		}
		process_subdirs_via_callback( callback_add_dir, path, subpathlen );
		(*callback_add_dir)(path, subpathlen);
	}
	return;
}

void
fill_fsstructure_by_records( struct file_info_array_t **fs_structure, struct db_records_t *db_records){
	assert(db_records);
	*fs_structure = malloc(sizeof(struct file_info_array_t));
	(*fs_structure)->maxcount = (*fs_structure)->count = 0;
	(*fs_structure)->array = NULL;
	/*add db_records to fs_tructure*/
	for ( int i=0; i < db_records->count; i++ ){
		struct db_record_t* dbrecord = &db_records->array[i];
		//process directories include all sub dirs
		process_subdirs_via_callback(callback_add_dir, dbrecord->fpath, strlen(dbrecord->fpath));
		/*check path, do not allow duplicates filenames*/
		int found = 0;
		if ( fsfile_by_path(*fs_structure, dbrecord->fpath) )
			found=1;
		/*add unique filename*/
		if ( !found ){
			/*prepare array to add file
			 * realloc array if became full*/
			if ( (*fs_structure)->maxcount <= (*fs_structure)->count ){
				(*fs_structure)->maxcount+=10;
				(*fs_structure)->array = realloc( (*fs_structure)->array, (*fs_structure)->maxcount*sizeof(struct file_info_t) );
			}
			int pathlen = strlen(dbrecord->fpath);
			struct file_info_t *fsfile = &(*fs_structure)->array[(*fs_structure)->count++];
			/*set path*/
			fsfile->path = malloc((pathlen+1)*sizeof(char));
			memcpy( fsfile->path, dbrecord->fpath, pathlen );
			fsfile->path[pathlen] = '\0';
			/*set fd file decriptor*/
			fsfile->fd = dbrecord->fd;
			/*set nlink=1 for file*/
			fsfile->nlink = 1;
			/*set access mode*/
			if ( 'r' == dbrecord->fmode )
				fsfile->access_mode = S_IFREG|ACS_ALLREAD;
			else
				fsfile->access_mode = S_IFREG|ACS_ALLWRITE;
		}
	}
	return;
}


const char* name_from_path_get_path_len(const char *fullpath, int *pathlen){
	//locate most right dir by symbol '/'
	char *c = strrchr(fullpath, '/');
	if ( c && *++c != '\0' ){
		*pathlen = (c - fullpath) / sizeof(char);
		//For paths that bigger than '/' it should not include trailing '/' symbol, for example '/in'
		if ( *pathlen > 1 )
			--*pathlen;
		return (const char*)( c );
	}
	else return 0;
}

/*@return 0 -allowed, rest are error codes*/
int check_access_mode( int allowed_chmod, int fuse_wanted_chmod ){
	//if it's a file
	int want_read = 0;
	int want_write = 0;
	int want_read_write = 0;
	int open_mode = fuse_wanted_chmod & 0b11;
	switch( open_mode ){
	case O_RDONLY:
		want_read = 1;
		break;
	case O_WRONLY:
		want_write = 1;
		break;
	case O_RDWR:
		want_read_write = 1;
		break;
	}


	int user_chmod = allowed_chmod >> 6;
	user_chmod = user_chmod << 6; //reset 6 lower bits
	switch( user_chmod ){
	case S_IFREG | S_IRUSR | S_IWUSR: /*allowed read & write*/
		return 0;
	case S_IFREG | S_IRUSR: /*allowed read*/
		if ( want_read && !want_write && !want_read_write ) return 0;
		else return -EACCES;
	case S_IFREG | S_IWUSR: /*allowed write*/
		if ( !want_read && want_write && !want_read_write ) return 0;
		else return -EACCES;
	default:
		break;
	}
	return -EACCES;
}

struct file_info_t *
fsfile_by_path(struct file_info_array_t *array, const char *path){
	assert(array);
	struct file_info_t *finfo = NULL;
	for( int i=0; i < array->count; i++ ){
		finfo = &array->array[i];
		if ( ! strcmp(path, finfo->path) ) return finfo;
	}
	return NULL;
}


void stat_printf(const char *path, struct stat* stat){
	WRITE_FMT_LOG(LOG_MISC, "path=%s stat{ st_mode=0x%x, st_ino=%d }", path, stat->st_mode, (int)stat->st_ino );
}



static int zvm_getattr(const char *path, struct stat *stbuf)
{
	WRITE_FMT_LOG(LOG_MISC, "getattr_path=%s", path);
	memset(stbuf, 0, sizeof(struct stat));

	struct file_info_t *finfo = fsfile_by_path(__fs->fs_structure, path);
	if ( finfo ){
		stbuf->st_ino = finfo->fd;
		stbuf->st_mode = finfo->access_mode;
		//stbuf->st_size =
		if ( finfo->access_mode & S_IFREG ){
			//if a file, it should contains at least single hardlink
			stbuf->st_nlink = 1;
		}
		else if ( finfo->access_mode & S_IFDIR ){
			//if a dir
			if ( !strcmp("/\0", path) )
				stbuf->st_nlink = 4;
			else
				stbuf->st_nlink = 2;
		}

		stat_printf( path, stbuf );
		return 0;
	}
	return -ENOENT;
}


static int zvm_access(const char *path, int mask)
{
	(void)path;
	(void)mask;
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s, mask=%d\n", __func__, path, mask );
	return 0;
}

static int zvm_readlink(const char *path, char *buf, size_t size)
{
	(void)path;
	(void)size;
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s, size=%d\n", __func__, path, (int)size );
	return 0;
}

static int zvm_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	struct stat st;
	struct file_info_t *finfo = NULL;
	int path_len = strlen(path);
	WRITE_FMT_LOG(LOG_MISC, " readdir=%s;", path);
	for( int i=0; i < __fs->fs_structure->count; i++ ){
		memset(&st, 0, sizeof(st));
		finfo = &__fs->fs_structure->array[i];
		int curr_path_len = 0;
		const char *name = name_from_path_get_path_len(finfo->path, &curr_path_len);
		printf(" current=%s;", finfo->path);fflush(0);
		if ( curr_path_len==path_len && !strncmp(path, finfo->path, curr_path_len) && name ){
			st.st_ino = finfo->fd;
			st.st_mode = finfo->access_mode;
			//st.st_size =
			stat_printf( name, &st );
			filler(buf, name, &st, 0);
		}
	}

	return 0;
}

static int zvm_mknod(const char *path, mode_t mode, dev_t rdev)
{
	(void)path;
	(void)mode;
	(void)rdev;
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s, mode=0x%x\n", __func__, path, mode );
	return 0;
}

static int zvm_mkdir(const char *path, mode_t mode)
{
	(void)path;
	(void)mode;
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s, mode=0x%x\n", __func__, path, mode );
	return 0;
}

static int zvm_unlink(const char *path)
{
	(void)path;
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s\n", __func__, path );
	return 0;
}

static int zvm_rmdir(const char *path)
{
	(void)path;
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s\n", __func__, path );
	return 0;
}

static int zvm_symlink(const char *from, const char *to)
{
	(void)from;
	(void)to;
	WRITE_FMT_LOG(LOG_MISC, "%s from=%s, to=%s\n", __func__, from, to );
	return 0;
}

static int zvm_rename(const char *from, const char *to)
{
	(void)from;
	(void)to;
	WRITE_FMT_LOG(LOG_MISC, "%s from=%s, to=%s\n", __func__, from, to );
	return 0;
}

static int zvm_link(const char *from, const char *to)
{
	(void)from;
	(void)to;
	WRITE_FMT_LOG(LOG_MISC, "%s from=%s, to=%s\n", __func__, from, to );
	return 0;
}

static int zvm_chmod(const char *path, mode_t mode)
{
	(void)path;
	(void)mode;
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s, mode=0x%x\n", __func__, path, mode );
	return 0;
}

static int zvm_chown(const char *path, uid_t uid, gid_t gid)
{
	(void)path;
	(void)uid;
	(void)gid;
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s\n", __func__, path );
	return 0;
}

static int zvm_truncate(const char *path, off_t size)
{
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s, size=%d\n", __func__, path, (int)size );

	assert(__fs);
	struct file_info_t *finfo = fsfile_by_path(__fs->fs_structure, path);
	if ( finfo ){
		return 0;
	}
	return -ENOENT;

	return 0;
}

static int zvm_utimens(const char *path, const struct timespec ts[2])
{
	(void)path;
	(void)ts;
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s\n", __func__, path );
	return 0;
}


static int zvm_open(const char *path, struct fuse_file_info *fi)
{
	struct file_info_t *finfo = fsfile_by_path(__fs->fs_structure, path);
	if ( finfo ){
		WRITE_FMT_LOG(LOG_MISC, "zvm_open %s", path );
		if ( sockf_by_fd( __zmq_pool, finfo->fd) ) return 0; /*already opened socket*/
		int access = check_access_mode( finfo->access_mode, fi->flags );
		/*if access is granted and found */
		if ( !access ){
			struct db_record_t* db_record = match_db_record_by_fd( __db_records, finfo->fd);
			if(db_record){
				struct sock_file_t* sockf = open_sockf( __zmq_pool, __db_records, finfo->fd);
				if ( sockf )
					return 0;
				else
					return -EINVAL;
			}
		}
		else {
			WRITE_FMT_LOG(LOG_MISC, "zvm_open ACCESS DENIED %s", path );
			return -EACCES;
		}
	}
	return -ENOENT;
}

static int zvm_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	if ( !fi) return -EINVAL;
	assert(__fs);
	struct file_info_t *finfo = fsfile_by_path(__fs->fs_structure, path);
	if ( finfo ){
		if ( !check_access_mode(finfo->access_mode, fi->flags) ){
			int fd = finfo->fd;
			struct db_record_t* db_record = match_db_record_by_fd( __db_records, fd);
				if(db_record ){
					if ( db_record->ftype == EFILE_STD ){
						// STDIN
					}
					else if ( db_record->ftype == EFILE_MSQ ){
						struct sock_file_t* sockf = sockf_by_fd(__zmq_pool, fd);
						if ( sockf ){
							return read_sockf( sockf, buf, size);
						}
						else
							return -EINVAL;
					}
					else { //EFILE_USER, not supported
						return -ENOENT;
					}
				}
				else return -EINVAL;
		}
		else return -EACCES;
	}
	return -ENOENT;
}

static int zvm_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	WRITE_FMT_LOG(LOG_MISC, "path=%s, size=%d, off_t=%d", path, (int)size, (int)offset );
	if ( !fi ) return -EINVAL;
	assert(__fs);
	struct file_info_t *finfo = fsfile_by_path(__fs->fs_structure, path);
	if ( finfo ){
		if ( !check_access_mode(finfo->access_mode, fi->flags) ){
			int fd = finfo->fd;
			struct db_record_t* db_record = match_db_record_by_fd( __db_records, fd);
			if(db_record ){
				if ( db_record->ftype == EFILE_STD ){
					// STDOUT, STDERR
				}
				else if ( db_record->ftype == EFILE_MSQ ){
					struct sock_file_t* sockf = sockf_by_fd(__zmq_pool, fd);
					if ( sockf ){
						return write_sockf( sockf, buf, size);
					}
					else
						return -EINVAL;
				}
				else { //EFILE_USER Not supported
					return -ENOENT;
				}
			}
			else return -EINVAL;
		}
		else return -EACCES;
	}
	return -ENOENT;
}

static int zvm_statfs(const char *path, struct statvfs *stbuf){
	(void)path;
	(void)stbuf;
	WRITE_FMT_LOG(LOG_MISC, "%s path=%s\n", __func__, path );
	return 0;
}

static int zvm_release(const char *path, struct fuse_file_info *fi){
	struct file_info_t *finfo = fsfile_by_path(__fs->fs_structure, path);
	if ( finfo ){
		struct sock_file_t *sockf = sockf_by_fd( __zmq_pool, finfo->fd);
		 /*find opened socket*/
		if ( sockf ){
			close_sockf(__zmq_pool, sockf );
		}
		return -ENOENT;
	}
	return -ENOENT;
}

static struct fuse_operations zvm_oper = {
	.getattr	= zvm_getattr,
	.access		= zvm_access,
	.readlink	= zvm_readlink,
	.readdir	= zvm_readdir,
	.mknod		= zvm_mknod,
	.mkdir		= zvm_mkdir,
	.symlink	= zvm_symlink,
	.unlink		= zvm_unlink,
	.rmdir		= zvm_rmdir,
	.rename		= zvm_rename,
	.link		= zvm_link,
	.chmod		= zvm_chmod,
	.chown		= zvm_chown,
	.truncate	= zvm_truncate,
	.utimens	= zvm_utimens,
	.open		= zvm_open,
	.read		= zvm_read,
	.write		= zvm_write,
	.statfs		= zvm_statfs,
	.release	= zvm_release
};

int run_fuse_main(struct db_records_t *db_records, int argc, char *argv[])
{
	assert(db_records);
	__db_records = db_records;
	__fs = malloc( sizeof(struct fs_in_memory_t) );
	fill_fsstructure_by_records( &__fs->fs_structure, db_records);
	__zmq_pool = malloc(sizeof(struct zeromq_pool));
	init_zeromq_pool(__zmq_pool);
	/*create zmq context*/
	assert( (__zmq_pool->context = zmq_init(1)) );

	umask(0);
	int err = fuse_main(argc, argv, &zvm_oper, NULL);
	/*this point is reached after unmount vfs*/
	/*destroy zmq context*/
	zeromq_term(__zmq_pool);
	return err;
}

