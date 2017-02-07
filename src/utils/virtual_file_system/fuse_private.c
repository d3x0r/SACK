#include <stdhdrs.h>

#include <sack_vfs.h>
#include "vfs_internal.h"

#define BUILD_TEST

#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#ifndef __ANDROID__
#include <fuse/fuse_lowlevel.h>
#else
#include <fuse_lowlevel.h>
#endif

static struct fuse_private_local
{
	struct fuse_chan* mount;
	struct fuse_session *session;
	CTEXTSTR private_mount_point;
	pid_t myself;
	struct directory_entry zero_entkey;
	uint8_t zerokey[BLOCK_SIZE];
	uid_t uid;
	gid_t gid;
#ifdef BUILD_TEST
	struct volume *volume;
	PTHREAD main;
#endif
} fpl;

static const char *hello_str = "Hello World!\n";
static const char *hello_name = "hello";

static int doStat(fuse_ino_t ino, struct stat *attr, double *timeout)
{
	lprintf( "request attr %ld", ino );
	memset( attr, 0, sizeof( *attr ) );
	attr->st_dev = 1;
	attr->st_nlink = 1;
	attr->st_ino = ino;
	attr->st_uid = fpl.uid;
	attr->st_gid = fpl.gid;
	attr->st_size = 1234;
	attr->st_blocks = 4321;
	attr->st_blksize = 1;
	if( fpl.volume->read_only )
		if( ino == 1 ) {
			attr->st_mode = S_IFDIR | 0500;
	(*timeout) = 10000.0;
		}
		else {
			attr->st_mode = S_IFREG | 0400;
			(*timeout) = 1.0;
		}
	else
		if( ino == 1 ) {
			attr->st_mode = S_IFDIR | 0700;
         (*timeout) = 10000.0;
		}
		else {
			attr->st_mode = S_IFREG | 0600;
         (*timeout) = 1.0;
		}
   return 0;
}

static int hello_stat(fuse_ino_t ino, struct stat *stbuf)
{
        stbuf->st_ino = ino;
        switch (ino) {
        case 1:
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
                break;
        case 2:
                stbuf->st_mode = S_IFREG | 0444;
                stbuf->st_nlink = 1;
                stbuf->st_size = strlen(hello_str);
                break;
        default:
                return -1;
        }
        return 0;
}
static void hello_ll_getattr(fuse_req_t req, fuse_ino_t ino,
                             struct fuse_file_info *fi)
{
	struct stat stbuf;
	double timeout;

        memset(&stbuf, 0, sizeof(stbuf));
        if (doStat(ino, &stbuf, &timeout) == -1)
                fuse_reply_err(req, ENOENT);
        else
                fuse_reply_attr(req, &stbuf, timeout);
}
static void hello_ll_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
   const struct fuse_ctx *ctx = fuse_req_ctx(req);
	struct fuse_entry_param e;
	lprintf( "lookup from %d  %d %ld %s", ctx->pid, fpl.myself, parent, name );


	if (parent != 1 || strcmp(name, hello_name) != 0)
		fuse_reply_err(req, ENOENT);
	else {
		double timeout;
		memset(&e, 0, sizeof(e));
		e.ino = 2;
		doStat(e.ino, &e.attr, &timeout );
		e.attr_timeout = timeout;
		e.entry_timeout = timeout;
		fuse_reply_entry(req, &e);
	}
}
struct dirbuf {
        char *p;
        size_t size;
};
static void dirbuf_add(fuse_req_t req, struct dirbuf *b, const char *name,
                       fuse_ino_t ino)
{
        struct stat stbuf;
        size_t oldsize = b->size;
        b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
        b->p = (char *) realloc(b->p, b->size);
        memset(&stbuf, 0, sizeof(stbuf));
        stbuf.st_ino = ino;
        fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
                          b->size);
}

//#define min(x, y) ((x) < (y) ? (x) : (y))
static int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
                             off_t off, size_t maxsize)
{
	if (off < bufsize)
	{
		return fuse_reply_buf(req, buf + off,
									 min(bufsize - off, maxsize));
	}
	else
		return fuse_reply_buf(req, NULL, 0);
}

static void DumpDirectory( struct volume *vol, fuse_req_t req, struct dirbuf *b  )
{
	int n;
	int this_dir_block = 0;
	int next_dir_block;
   fuse_ino_t ino = 100;
	struct directory_entry *next_entries;
	do
	{
		next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, BLOCK_CACHE_DIRECTORY );
		for( n = 0; n < VFS_DIRECTORY_ENTRIES; n++ )
		{
			struct directory_entry *entkey = ( vol->key )?((struct directory_entry *)vol->usekey[BLOCK_CACHE_DIRECTORY])+n:&fpl.zero_entkey;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			if( !name_ofs ) return;  // end of directory
			if( !(next_entries[n].first_block ^ entkey->first_block ) ) continue;// if file is deleted; don't check it's name.
			{
				char buf[256];
				uint8_t* name = TSEEK( uint8_t*, vol, name_ofs, BLOCK_CACHE_NAMES );
				int ch;
				uint8_t c;
            if( vol->key )
					for( ch = 0; c = (name[ch] ^ vol->usekey[BLOCK_CACHE_NAMES][( name_ofs & BLOCK_MASK ) +ch]); ch++ )
						buf[ch] = c;
            else
					for( ch = 0; c = name[ch]; ch++ )
					{
                  if( c == '/' ) c = '\\';
						buf[ch] = c;
					}
				buf[ch] = c;
            dirbuf_add( req, b, buf, ino++ );
			}
		}
		next_dir_block = vfs_GetNextBlock( vol, this_dir_block, TRUE, TRUE );
		if( this_dir_block == next_dir_block )
			DebugBreak();
		if( next_dir_block == 0 )
			DebugBreak();
		this_dir_block = next_dir_block;

	}
	while( 1 );
}

static void ll_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
   fi->fh = (uintptr_t)New( struct dirbuf );
	memset((void*)fi->fh, 0, sizeof(struct dirbuf));
   fuse_reply_open( req, fi );
}



static void hello_ll_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                             off_t off, struct fuse_file_info *fi)
{
	struct dirbuf *b = (struct dirbuf*)fi->fh;
	if( b->size == 0 )
	{
		if (ino != 1)
			fuse_reply_err(req, ENOTDIR);
		else {
			  //memset(&b, 0, sizeof(b));
			dirbuf_add(req, b, ".", 1);
			dirbuf_add(req, b, "..", 1);
			DumpDirectory( fpl.volume, req, b );
			  //dirbuf_add(req, &b, hello_name, 2);
			reply_buf_limited(req, b->p, b->size, off, size);
			//free(b.p);
		}
	}
	else
	{
		reply_buf_limited(req, b->p, b->size, off, size);
	}
}
static void ll_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	Release( (void*)fi->fh );
   fuse_reply_none( req );
}


static void hello_ll_open(fuse_req_t req, fuse_ino_t ino,
                          struct fuse_file_info *fi)
{
	if (ino != 2)
		fuse_reply_err(req, EISDIR);
	else if ((fi->flags & 3) != O_RDONLY)
		fuse_reply_err(req, EACCES);
	else
		fuse_reply_open(req, fi);
}
static void hello_ll_read(fuse_req_t req, fuse_ino_t ino, size_t size,
                          off_t off, struct fuse_file_info *fi)
{
        (void) fi;
        //assert(ino == 2);
        reply_buf_limited(req, hello_str, strlen(hello_str), off, size);
}


static void ll_write(fuse_req_t req, fuse_ino_t ino, size_t size,
                          off_t off, struct fuse_file_info *fi)
{
        (void) fi;
        //assert(ino == 2);
        reply_buf_limited(req, hello_str, strlen(hello_str), off, size);
}

static void ll_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	struct stat attr;
	double timeout;
	lprintf( "request attr %ld", ino );
   doStat( ino, &attr, &timeout );
	fuse_reply_attr( req, &attr, timeout );
}


static struct fuse_lowlevel_ops ll_oper = {
        .lookup         = hello_ll_lookup,
        .getattr        = ll_getattr,
        .readdir        = hello_ll_readdir,
        .open           = hello_ll_open,
		  .read           = hello_ll_read,
        .write          = ll_write,
        .opendir        = ll_opendir,
		  .releasedir     = ll_releasedir,
};

static void fpvfs_close( void )
{
	if( fpl.session ) {
		fuse_session_destroy( fpl.session );
      fpl.session = NULL;
	}
	if( fpl.mount ) {
		fuse_unmount( fpl.private_mount_point, fpl.mount );
		fpl.mount = NULL;
	}
}

static uintptr_t CPROC sessionLoop( PTHREAD thread )
{
   lprintf( "thread as %d %d", getpid(), getpgrp() );
	if( fuse_set_signal_handlers( fpl.session ) != -1 ) {
      int error;
		fuse_session_add_chan( fpl.session, fpl.mount );
		error = fuse_session_loop( fpl.session );
      lprintf( "Volume has been unmounted" );
		fuse_remove_signal_handlers( fpl.session );
		fuse_session_remove_chan( fpl.mount );
		fpvfs_close();
		if( fpl.main )
         WakeThread( fpl.main );
	}
   return 0;
}

LOGICAL fpvfs_init( CTEXTSTR path )
{
	struct fuse_args args = { argc: 0, argv: NULL, allocated:0 };
   fuse_opt_add_arg( &args, "-d" );
   //fuse_opt_add_arg( &args, "-o" );
   //fuse_opt_add_arg( &args, "idmap=user" );
	fpl.mount = fuse_mount( path, &args );
	fpl.myself = getpid();
	fpl.uid = getuid();
   fpl.gid = getgid();
	if( fpl.mount ) {
      fpl.private_mount_point = StrDup( path );
		fpl.session = fuse_lowlevel_new( &args, &ll_oper, sizeof( ll_oper ), NULL );
		if( fpl.session ) {
         ThreadTo( sessionLoop, 0 );
		}
	}

}

#ifdef BUILD_TEST
int main(int argc, char *argv[])
{
	fpl.main = MakeThread();
   lprintf( "Started as %d %d", getpid(), getpgrp() );
	fpvfs_init( "./test" );
   fpl.volume = sack_vfs_load_volume( "test.vfs" );
	while( fpl.mount )
	{
      WakeableSleep( 10000 );
	}
   fpvfs_close();
	return 0;
}
#endif


