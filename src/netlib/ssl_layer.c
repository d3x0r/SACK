


#include <stdhdrs.h>
#ifdef BUILD_NODE_ADDON
#  include <node_version.h>
#endif

#ifdef BUILD_NODE_ADDON
#  ifdef NODE_MAJOR_VERSION
#    if NODE_MAJOR_VERSION >= 10
#      define HACK_NODE_TLS
#    endif
#  endif
#endif

#ifdef _WIN32
#  include <wincrypt.h>
#  include <prsht.h>
#  include <cryptuiapi.h>
#endif
#include <deadstart.h>
#include <procreg.h>
#include <sqlgetoption.h>


#include "netstruc.h"

//#define DEBUG_SSL_IO
// also has option to control output
#define DEBUG_SSL_IO_BUFFERS
//#define DEBUG_SSL_IO_RAW
//#define DEBUG_SSL_IO_VERBOSE

#if defined ( NO_SSL )

SACK_NETWORK_NAMESPACE

LOGICAL ssl_Send( PCLIENT pc, CPOINTER buffer, size_t length ) {
	return FALSE;
}

LOGICAL ssl_BeginServer( PCLIENT pc, CPOINTER cert, size_t certlen, CPOINTER keypair, size_t keylen, CPOINTER keypass, size_t keypasslen ) {
	return FALSE;
}

LOGICAL ssl_BeginClientSession( PCLIENT pc, CPOINTER client_keypair, size_t client_keypairlen, CPOINTER keypass, size_t keypasslen, CPOINTER rootCert, size_t rootCertLen ) {
	return FALSE;
}

LOGICAL ssl_IsClientSecure( PCLIENT pc ) {
	return FALSE;
}

CTEXTSTR ssl_GetRequestedHostName( PCLIENT pc ) {
	return NULL;
}

void ssl_CloseSession( PCLIENT pc ) {
   return;
}

SACK_NETWORK_NAMESPACE_END

#else

#ifndef OPENSSL_API_COMPAT
#  define OPENSSL_API_COMPAT 10101
#endif

#define _LIB
#  if NODE_MAJOR_VERSION >= 17
// this can't work?
#    include <openssl/configuration.h>
#  endif
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/err.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#if OPENSSL_VERSION_MAJOR >= 3
#include <openssl/core_names.h>
#endif

SACK_NETWORK_NAMESPACE

//#define RSA_KEY_SIZE (1024)
const int serverKBits = 4096;
const int kBits = 1024;// 4096;
const int kExp = RSA_F4;

struct internalCert {
	X509_REQ *req;
	X509 * x509;
	EVP_PKEY *pkey;
	STACK_OF( X509 ) *chain;
};

void loadSystemCerts(SSL_CTX* ctx,X509_STORE *store );

//static void gencp
struct internalCert * MakeRequest( void );


EVP_PKEY *genKey() {
	EVP_PKEY *keypair = EVP_PKEY_new();
	//int keylen;
	//char *pem_key;
	//BN_GENCB cb = { }
	BIGNUM          *bne = BN_new();
	int ret;
	ret = BN_set_word( bne, kExp );
#if OPENSSL_VERSION_MAJOR < 3 || OPENSSL_API_COMPAT==10101
	RSA *rsa = RSA_new();

	if( ret != 1 ) {
		BN_free( bne );
		RSA_free( rsa );
		EVP_PKEY_free( keypair );
		return NULL;
	}
	RSA_generate_key_ex( rsa, kBits, bne, NULL );
	EVP_PKEY_set1_RSA( keypair, rsa );

	RSA_free( rsa );
#else
	EVP_PKEY_CTX* ctx; 
	ctx = EVP_PKEY_CTX_new_from_name( NULL, "rsa", NULL );
	if( !ctx )
		return NULL;
	EVP_KEYMGMT* keymgmt;  keymgmt = EVP_KEYMGMT_fetch( NULL, "rsa", NULL );
	if( !keymgmt )
		return NULL;
	if( EVP_PKEY_set_type_by_keymgmt( keypair, keymgmt ) != 1 )
		return NULL;

	{
		OSSL_PARAM params[2] = {
			OSSL_PARAM_BN( OSSL_PKEY_PARAM_RSA_E, bne, (size_t)BN_num_bytes( bne ) ),
			OSSL_PARAM_END
		};

		if( EVP_PKEY_fromdata_init( ctx ) != 1 ||
			EVP_PKEY_fromdata( ctx, &keypair, EVP_PKEY_KEY_PARAMETERS, params ) != 1 )
			exit( 7 );
	}
#endif
	BN_free( bne );

	return keypair;
}

#define ALLOW_ANON_CONNECTIONS	1

#ifndef UNICODE

typedef struct {
  int verbose_mode;
  int verify_depth;
  int always_continue;
} verify_mydata_t;
#define verify_mydata_index 0

// template
verify_mydata_t verify_default = { FALSE, 100, FALSE };


struct ssl_hostContext {
	SSL_CTX* ctx;
	struct internalCert* cert;
	char* host;
};

struct ssl_session {
	PLIST hosts;
	SSL_CTX        *ctx;
	struct internalCert* cert;
	LOGICAL ignoreVerification;
	LOGICAL firstPacket;
	LOGICAL closed;
	BIO *rbio;
	BIO *wbio;
	//EVP_PKEY *privkey;
	PLIST          accepting; // sockets being accepted so we can find the proper SSL*
	TEXTSTR	       hostname;  // accepted client's hostname request (NULL if none)
	PLIST          protocols; // protocol select from TLS
	SSL*           ssl;
	uint32_t       dwOriginalFlags; // CF_CPPREAD
	cReadComplete  user_read;
	cppReadComplete  cpp_user_read;

	cNotifyCallback user_connected;
	cppNotifyCallback cpp_user_connected;

	cCloseCallback user_close;
	cppCloseCallback cpp_user_close;

	uint8_t *obuffer;
	size_t obuflen;
	uint8_t *ibuffer;
	size_t ibuflen;
	uint8_t *dbuffer;
	size_t dbuflen;
	CRITICALSECTION csReadWrite;
	verify_mydata_t verify_data;
	//CRITICALSECTION csReadWrite;
	//CRITICALSECTION csWrite;
};

static struct ssl_global
{
	struct {
		BIT_FIELD bInited : 1;
		BIT_FIELD bLogBuffers : 1;
		BIT_FIELD bLogBuffersVerbose : 1;
	} flags;
	LOGICAL trace;
	struct tls_config *tls_config;
	uint8_t cipherlen;
	uint32_t *lock_cs;// = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(HANDLE));
}ssl_global;

PRELOAD( InitSSL ) {
	ssl_global.flags.bLogBuffers = SACK_GetProfileIntEx( "SACK", "Network/SSL/Log Network Data", 0, TRUE );	
	ssl_global.flags.bLogBuffersVerbose = SACK_GetProfileIntEx( "SACK", "Network/SSL/Log Network Data Verbose", 0, TRUE );	
}

ATEXIT( CloseSSL )
{
	if( ssl_global.flags.bInited ) {
	}
}

static int logerr( const char *str, size_t len, void *userdata ) {
	lprintf( "%d: %s", *((int*)&userdata), str );
	return 0;
}

static void ssl_handlePendingControlwrites( PCLIENT pc ) {
	// the read generated write data, output that data
	size_t pending = BIO_ctrl_pending( pc->ssl_session->wbio );
	if( !pc->ssl_session ) {
		lprintf( "SSL SESSION SELF DESTRUCTED!" );
	}
	if( pending > 0 ) {
		int read;
#ifdef DEBUG_SSL_IO_VERBOSE
		lprintf( "pending to send is %zd into %zd %p " , pending, pc->ssl_session->obuflen, pc->ssl_session->obuffer );
#endif
		if( pending > pc->ssl_session->obuflen ) {
			if( pc->ssl_session->obuffer )
				Deallocate( uint8_t *, pc->ssl_session->obuffer );
			pc->ssl_session->obuffer = NewArray( uint8_t, pc->ssl_session->obuflen = pending * 2 );
			//lprintf( "making obuffer bigger %d %d", pending, pending * 2 );
		}
		read  = BIO_read( pc->ssl_session->wbio, pc->ssl_session->obuffer, (int)pending );
		if( read < 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
			lprintf( "failed to read pending control data...SSL will fail without it." );
		} else {
#ifdef DEBUG_SSL_IO
			lprintf( "Send pending control %p %d", pc->ssl_session->obuffer, read );
#endif
			SendTCP( pc, pc->ssl_session->obuffer, read );
		}
	}
}

LOGICAL ssl_IsClosed( PCLIENT pc ) {
	if( pc->ssl_session ) {
		return pc->ssl_session->closed;
	}
	return TRUE;
}

void ssl_CloseSession( PCLIENT pc )
{
	if( pc->ssl_session )
	{
		INDEX idx;
		PTEXT seg;
		SSL_shutdown( pc->ssl_session->ssl );
		ssl_handlePendingControlwrites( pc );

		LIST_FORALL( pc->ssl_session->protocols, idx, PTEXT, seg ) {
			LineRelease( seg );
		}
		DeleteList( &pc->ssl_session->protocols );
		if( pc->ssl_session->hostname ) {
			Release( pc->ssl_session->hostname );
			pc->ssl_session->hostname = NULL;
		}
		pc->ssl_session->closed = TRUE;
		//Release( pc->ssl_session );
		//pc->ssl_session = NULL;
	}
	if( !( pc->dwFlags & ( CF_CLOSED|CF_CLOSING ) ) )
		RemoveClient( pc );
}

static int handshake( PCLIENT pc ) {
	struct ssl_session *ses = pc->ssl_session;
	if (!SSL_is_init_finished(ses->ssl)) {
		int r;
#ifdef DEBUG_SSL_IO_VERBOSE
		lprintf( "doing handshake...." );
#endif
		/* NOT INITIALISED */
#ifdef DEBUG_SSL_IO
		lprintf(" Handshake is not finished? %p", pc );
#endif
		r = SSL_do_handshake(ses->ssl);
#ifdef DEBUG_SSL_IO_VERBOSE
		lprintf( "handle data posted to SSL? %d", r );
#endif
		if( r == 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
			r = SSL_get_error( ses->ssl, r );
			ERR_print_errors_cb( logerr, (void*)__LINE__ );

#ifdef DEBUG_SSL_IO
			lprintf( "SSL_Read failed... %d", r );
#endif
			return -1;
		}
		//if( r >= 0 )
		{
			size_t pending;
			while( ( pending = BIO_ctrl_pending( ses->wbio) ) > 0 ) {
				if( r == -1 && pending < 8 ) break;
				if (pending > 0) {
					int read;
					if( pending > ses->obuflen ) {
						if( ses->obuffer )
							Deallocate( uint8_t *, ses->obuffer );
						ses->obuffer = NewArray( uint8_t, ses->obuflen = pending*2 );
						//lprintf( "making obuffer bigger %d %d", pending, pending * 2 );
					}
					read = BIO_read(ses->wbio, ses->obuffer, (int)pending);
#ifdef DEBUG_SSL_IO_VERBOSE
  					lprintf( "send %d %d for handshake", pending, read );
#endif
  					if( read > 0 ) {
#ifdef DEBUG_SSL_IO
  						lprintf( "handshake send %d", read );
#endif
  						SendTCP( pc, ses->obuffer, read );
  					}
  				}
  			}
  		}
		if (r < 0) {

			r = SSL_get_error(ses->ssl, r);
			if( SSL_ERROR_SSL == r ) {
#ifdef DEBUG_SSL_IO
				lprintf( "SSL_Read failed... %d", r );
#endif
				if( !pc->errorCallback )
					ERR_print_errors_cb( logerr, (void*)__LINE__ );
				return -1;
			}
			if (SSL_ERROR_WANT_READ == r)
			{
			}
			else {
				ERR_print_errors_cb( logerr, (void*)__LINE__ );
				return -1;
			}
			return 0;
		}
		return 2;
	}
	else {
		/* SSL IS INITIALISED */
#ifdef DEBUG_SSL_IO
		lprintf(" Handshake is and has been finished?");
#endif
		return 1;
	}
}


static void ssl_ReadComplete( PCLIENT pc, POINTER buffer, size_t length )
{
#if defined( DEBUG_SSL_IO ) || defined( DEBUG_SSL_IO_RAW )
	lprintf( "SSL Read complete %p %p %zd", pc, buffer, length );
#  if defined( DEBUG_SSL_IO_RAW )
	LogBinary( (const uint8_t*)buffer, length );
#  endif
#endif
	if( pc->ssl_session )
	{
		if( buffer )
		{
			int len;
			int hs_rc;
			//lprintf( "Read from network: %d", length );
			//LogBinary( (const uint8_t*)buffer, length );

			EnterCriticalSec( &pc->ssl_session->csReadWrite );

			len = BIO_write( pc->ssl_session->rbio, buffer, (int)length );
#ifdef DEBUG_SSL_IO_VERBOSE
			lprintf( "Wrote %zd", len );
#endif
			if( len < (int)length ) {
				lprintf( "Internal buffer error; wrote less to buffer than specified?" );
				LeaveCriticalSec( &pc->ssl_session->csReadWrite );
				ssl_CloseSession( pc );
				return;
			}

			if( !( hs_rc = handshake( pc ) ) ) {
				if( !pc->ssl_session ) {
					LeaveCriticalSec( &pc->ssl_session->csReadWrite ); //-V522
					return;
				}
#ifdef DEBUG_SSL_IO_VERBOSE
				// normal condition...
				lprintf( "Receive handshake not complete iBuffer" );
#endif
				LeaveCriticalSec( &pc->ssl_session->csReadWrite );
				ReadTCP( pc, pc->ssl_session->ibuffer, pc->ssl_session->ibuflen );
				return;
			}
			if( !pc->ssl_session ) {
				LeaveCriticalSec( &pc->ssl_session->csReadWrite );
				return;
			}
			// == 1 if is already done, and not newly done
			if( hs_rc == 2 ) {
				// newly completed handshake.
				{
					if( !pc->ssl_session->ignoreVerification && SSL_get_peer_certificate( pc->ssl_session->ssl ) ) {
						int r;
						if( ( r = SSL_get_verify_result( pc->ssl_session->ssl ) ) != X509_V_OK ) {
							if( pc->errorCallback )
								pc->errorCallback( pc->psvErrorCallback, pc, SACK_NETWORK_ERROR_SSL_CERTCHAIN_FAIL );
							lprintf( "Certificate verification failed. %d", r );
							LeaveCriticalSec( &pc->ssl_session->csReadWrite );
							ssl_CloseSession( pc );
							return;
							//ERR_print_errors_cb( logerr, (void*)__LINE__ );
						}
					}

					LeaveCriticalSec( &pc->ssl_session->csReadWrite );

					//lprintf( "Initial read dispatch.." );
					if( pc->ssl_session->dwOriginalFlags & CF_CPPREAD )
						pc->ssl_session->cpp_user_read( pc->psvRead, NULL, 0 );
					else
						pc->ssl_session->user_read( pc, NULL, 0 );

					EnterCriticalSec( &pc->ssl_session->csReadWrite );
				}
				len = 0;
			}
			if( hs_rc >= 1 )
			{
			read_more:
				// if read isn't done before pending, pending doesn't get set
				// but this read doens't return a useful length.
				len = SSL_read( pc->ssl_session->ssl, NULL, 0 ); //-V575
				//lprintf( "return of 0 read: %d", len );
				//if( len < 0 )
				//	lprintf( "error of 0 read is %d", SSL_get_error( pc->ssl_session->ssl, len ) );
				len = SSL_pending( pc->ssl_session->ssl );
				//lprintf( "do read.. pending %d", len );
				if( len ) {
					if( len > (int)pc->ssl_session->dbuflen ) {
						//lprintf( "Needed to expand buffer..." );
						Release( pc->ssl_session->dbuffer );
						pc->ssl_session->dbuflen = ( len + 4095 ) & 0xFFFF000;
						pc->ssl_session->dbuffer = NewArray( uint8_t, pc->ssl_session->dbuflen );
					}
					len = SSL_read( pc->ssl_session->ssl, pc->ssl_session->dbuffer, (int)pc->ssl_session->dbuflen );
#ifdef DEBUG_SSL_IO_VERBOSE
					lprintf( "normal read - just get the data from the other buffer : %d", len );
#endif
					if( len < 0 ) {
						int error = SSL_get_error( pc->ssl_session->ssl, len );
						if( error == SSL_ERROR_WANT_READ ) {
							lprintf( "want more data?" );
						} else
							lprintf( "SSL_Read failed. %d", error );
						if( pc->errorCallback )
							pc->errorCallback( pc->psvErrorCallback, pc, SACK_NETWORK_ERROR_SSL_FAIL );
					}
				}
			}
			else if( hs_rc == -1 ) {
				if( pc->errorCallback )
					pc->errorCallback( pc->psvErrorCallback, pc, pc->ssl_session->firstPacket
						? SACK_NETWORK_ERROR_SSL_HANDSHAKE
						: SACK_NETWORK_ERROR_SSL_HANDSHAKE_2
						, buffer, length );
				// disableSSL call can happen during error callback
				// in which case this will eventually fall back to non-SSL reading
				// and the buffer will get passed back in (or a new buffer if someone really crafty uses it...)
				if( pc->ssl_session ) {
					LeaveCriticalSec( &pc->ssl_session->csReadWrite );
					ssl_CloseSession( pc );
				}
				return;
			}
			else
				len = 0;

			ssl_handlePendingControlwrites( pc );
			if( !pc->ssl_session ) {
				LeaveCriticalSec( &pc->ssl_session->csReadWrite );
				return;
			}

			LeaveCriticalSec( &pc->ssl_session->csReadWrite );

			// do was have any decrypted data to give to the application?
			if( len > 0 ) {
#ifdef DEBUG_SSL_IO_BUFFERS
				if( ssl_global.flags.bLogBuffers ) {
					lprintf( "READ BUFFER:" );
					LogBinary( pc->ssl_session->dbuffer, (( ssl_global.flags.bLogBuffers ) || ( 256 > len ))? len : 256 );
				}
#endif
				if( pc->ssl_session->dwOriginalFlags & CF_CPPREAD )
					pc->ssl_session->cpp_user_read( pc->psvRead, pc->ssl_session->dbuffer, len );
				else
					pc->ssl_session->user_read( pc, pc->ssl_session->dbuffer, len );
				if( pc->ssl_session ) { // might have closed during read.
					EnterCriticalSec( &pc->ssl_session->csReadWrite );
					goto read_more;
				}
			}
			else if( len == 0 ) {
#ifdef DEBUG_SSL_IO_VERBOSE
				lprintf( "incomplete read" );
#endif
			}

		}
		else {
			EnterCriticalSec( &pc->ssl_session->csReadWrite );

			pc->ssl_session->ibuffer = NewArray( uint8_t, pc->ssl_session->ibuflen = (4327 + 39) );
			pc->ssl_session->dbuffer = NewArray( uint8_t, pc->ssl_session->dbuflen = 4096 );
			{
				int r;
				if( ( r = SSL_do_handshake( pc->ssl_session->ssl ) ) < 0 ) {
					//char buf[256];
					//r = SSL_get_error( ses->ssl, r );
					ERR_print_errors_cb( logerr, (void*)__LINE__ );
					//lprintf( "err: %s", ERR_error_string( r, buf ) );
				}
				{
					// the read generated write data, output that data
					size_t pending = BIO_ctrl_pending( pc->ssl_session->wbio );
#ifdef DEBUG_SSL_IO
					lprintf( "Pending Control To Send: %d", pending );
#endif
					if( pending > 0 ) {
						int read;
						if( pending > pc->ssl_session->obuflen ) {
							if( pc->ssl_session->obuffer )
								Deallocate( uint8_t *, pc->ssl_session->obuffer );
							pc->ssl_session->obuffer = NewArray( uint8_t, pc->ssl_session->obuflen = pending * 2 );
							//lprintf( "making obuffer bigger %d %d", pending, pending * 2 );
						}
						read = BIO_read( pc->ssl_session->wbio, pc->ssl_session->obuffer, (int)pc->ssl_session->obuflen );
						SendTCP( pc, pc->ssl_session->obuffer, read );
					}
				}
			}
			LeaveCriticalSec( &pc->ssl_session->csReadWrite );
		}
		//lprintf( "Read more data..." );
		if( pc->ssl_session ) {
			if( !buffer )
				pc->ssl_session->firstPacket = 1;
			else
				pc->ssl_session->firstPacket = 0;
			if( pc->ssl_session ) {
				ReadTCP( pc, pc->ssl_session->ibuffer, pc->ssl_session->ibuflen );
			}
		}
	}
}

LOGICAL ssl_Send( PCLIENT pc, CPOINTER buffer, size_t length )
{
	int len;
	int32_t len_out;
	size_t offset = 0;
	size_t pending_out = length;
	struct ssl_session *ses = pc->ssl_session;
	if( !ses )
		return FALSE;
#if defined( DEBUG_SSL_IO ) || defined( DEBUG_SSL_IO_VERBOSE)
	lprintf( "SSL SEND....%d ", length );
#endif
#ifdef DEBUG_SSL_IO_BUFFERS
	if( ssl_global.flags.bLogBuffers ) {
		lprintf( "SSL SEND....%d ", length );
		LogBinary( (((uint8_t*)buffer) + offset), (( ssl_global.flags.bLogBuffers ) || ( 256 > length )) ? length : 256 );
	}
#endif
	while( length ) {
		if( pending_out > 4327 )
			pending_out = 4327;
#ifdef DEBUG_SSL_IO_VERBOSE
		lprintf( "Sending %d of %d at %d", pending_out, length, offset );
#endif
		EnterCriticalSec( &pc->ssl_session->csReadWrite );
		len = SSL_write( pc->ssl_session->ssl, (((uint8_t*)buffer) + offset), (int)pending_out );
		if (len < 0) {
			ERR_print_errors_cb(logerr, (void*)__LINE__);
			if( pc->ssl_session )
				LeaveCriticalSec( &pc->ssl_session->csReadWrite );
			return FALSE;
		}
		offset += len;
		length -= (size_t)len;
		pending_out = length;

		// signed/unsigned comparison here.
		// the signed value is known to be greater than 0 and less than max unsigned int
		// so it is in a valid range to check, and is NOT a warning or error condition EVER.
		len = BIO_pending( pc->ssl_session->wbio );
		//lprintf( "resulting send is %d", len );
		if( SUS_GT( len, int, ses->obuflen, size_t ) )
		{
			Release( ses->obuffer );
#ifdef DEBUG_SSL_IO
			lprintf( "making obuffer bigger %d %d", len, len * 2 );
#endif
			ses->obuffer = NewArray( uint8_t, len * 2 );
			ses->obuflen = len * 2;
		}
		len_out = BIO_read( pc->ssl_session->wbio, ses->obuffer, (int)ses->obuflen );
		if( pc->ssl_session )
			LeaveCriticalSec( &pc->ssl_session->csReadWrite );
#ifdef DEBUG_SSL_IO_VERBOSE
		lprintf( "ssl_Send  %d", len_out );
#endif
		SendTCP( pc, ses->obuffer, len_out );
	}
	return TRUE;

}


static void win32_locking_callback(int mode, int type, const char *file, int line)
{
	if (mode & CRYPTO_LOCK) {
		while( LockedExchange( ssl_global.lock_cs+type, 1 ) )
			Relinquish();
	} else {
		ssl_global.lock_cs[type] = 0;
	}
}

unsigned long pthreads_thread_id(void)
{
	return (unsigned long)GetThisThreadID();
}

LOGICAL ssl_InitLibrary( void ){
	if( !ssl_global.flags.bInited )
	{
		SSL_library_init();

		ssl_global.lock_cs = NewArray( uint32_t, CRYPTO_num_locks() );
		memset( ssl_global.lock_cs, 0, sizeof( uint32_t ) * CRYPTO_num_locks() );
		CRYPTO_set_locking_callback(win32_locking_callback);
		CRYPTO_set_id_callback(pthreads_thread_id);
		//tls_init();
		//ssl_global.tls_config = tls_config_new();

		SSL_load_error_strings();
//		ERR_load_BIO_strings();
		OpenSSL_add_all_algorithms();
		ssl_global.flags.bInited = 1;
	}
	return TRUE;
}

static void ssl_InitSession( struct ssl_session *ses ) {
	ses->rbio = BIO_new( BIO_s_mem() );
	ses->wbio = BIO_new( BIO_s_mem() );

	SSL_set_bio( ses->ssl, ses->rbio, ses->wbio );
	InitializeCriticalSec( &ses->csReadWrite );
	//InitializeCriticalSec( &ses->csWrite );
}

static void ssl_CloseCallback( PCLIENT pc ) {
	struct ssl_session *ses = pc->ssl_session;
	if (!ses) {
		lprintf("already closed?");
		return;
	}
	pc->ssl_session = NULL;

	if( ses->dwOriginalFlags & CF_CPPCLOSE ){
		if( ses->cpp_user_close ) ses->cpp_user_close( pc->psvClose );
	}else{
		if( ses->user_close ) ses->user_close( pc );
	}

	DeleteCriticalSec( &ses->csReadWrite );
	//DeleteCriticalSec( &ses->csWrite );
	Release( ses->dbuffer );
	Release( ses->ibuffer );
	Release( ses->obuffer );

	if( ses->cert ) {
		EVP_PKEY_free( ses->cert->pkey );
		X509_free( ses->cert->x509 );
		Release( ses->cert );
	}
	SSL_free( ses->ssl );
	SSL_CTX_free( ses->ctx );
	// these are closed... with the ssl connection.
	//BIO_free( ses->rbio );
	//BIO_free( ses->wbio );

	Release( ses );
}

#ifdef DEBUG_SSL_IO_VERBOSE
static void infoCallback( const SSL *ssl, int where, int ret ){
	if( !ret )
		lprintf( "ERROR : SSL Info Event %p %s %x %x", ssl, SSL_state_string(ssl), where, ret );
	else if( ret & SSL_CB_ALERT ) {
		lprintf( "ALERT : SSL Alert Event %p %s %s %s", ssl, SSL_state_string(ssl), SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret) );
	}
	else
		lprintf( "INFO : SSL Info Event %p %s %x %x %s %s", ssl, SSL_state_string(ssl)
			, where, ret, SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret) );
}
#endif

static void ssl_ClientConnected( PCLIENT pcServer, PCLIENT pcNew ) {
	struct ssl_session *ses;
	ses = New( struct ssl_session );
	MemSet( ses, 0, sizeof( struct ssl_session ) );

	ses->ssl = SSL_new( pcServer->ssl_session->ctx );
	{
		static uint32_t tick;
		tick++;
		SSL_set_session_id_context( ses->ssl, (const unsigned char*)&tick, 4 );//sizeof( ses->ctx ) );
	}
	ssl_InitSession( ses );

	SSL_set_accept_state( ses->ssl );
	AddLink( &pcServer->ssl_session->accepting, pcNew );
	pcNew->ssl_session = ses;
#ifdef DEBUG_SSL_IO_VERBOSE
	SSL_set_info_callback( pcNew->ssl_session->ssl, infoCallback );
#endif

	if( pcServer->ssl_session->dwOriginalFlags & CF_CPPCONNECT )
		pcServer->ssl_session->cpp_user_connected( pcServer->psvConnect, pcNew );
	else
		pcServer->ssl_session->user_connected( pcServer, pcNew);


	ses->user_read = pcNew->read.ReadComplete;
	ses->cpp_user_read = pcNew->read.CPPReadComplete;

	ses->user_close = pcNew->close.CloseCallback;
	ses->cpp_user_close = pcNew->close.CPPCloseCallback;

	pcNew->read.ReadComplete = ssl_ReadComplete;
	pcNew->dwFlags &= ~CF_CPPREAD;
	pcNew->close.CloseCallback = ssl_CloseCallback;
	pcNew->dwFlags &= ~CF_CPPCLOSE;


	ses->dwOriginalFlags = pcServer->ssl_session->dwOriginalFlags;

}

#if !defined( LIBRESSL_VERSION_NUMBER )

static int handleServerName( SSL* ssl, int* al, void* param ) {
	PCLIENT pcListener = (PCLIENT)param;
	PCLIENT pcAccept;
	INDEX idx;
	LIST_FORALL( pcListener->ssl_session->accepting, idx, PCLIENT, pcAccept ) {
		if( pcAccept->ssl_session && (pcAccept->ssl_session->ssl == ssl) )
			break;
	}
	if( !pcAccept ) {
		lprintf( "FATAL, NO SUCH ACCEPTING SOCKET" );
		return 0;
	}
	if( SSL_client_hello_isv2(ssl) ) {
		lprintf( "Unsupported version? V2" );
		// wrong version?
		al[0] = 0;
		return 0;
	}
	PLIST* ctxList = &pcListener->ssl_session->hosts;


	int* type;
	size_t typelen;
	size_t n;
	if( SSL_client_hello_get1_extensions_present( ssl, &type, &typelen ) ) {
		for( n = 0; n < typelen; n++ ) {
			unsigned char const* buf;
			size_t buflen;
			switch( type[n] ) {
			case TLSEXT_TYPE_max_fragment_length:
				SSL_client_hello_get0_ext( ssl, type[n], &buf, &buflen );
				{
					// there's some sort of length to this... and should limit sending? receiving?				
				}					
				break;
			case TLSEXT_TYPE_ec_point_formats:
			case TLSEXT_TYPE_supported_groups:
			case TLSEXT_TYPE_session_ticket: // empty value?
			case TLSEXT_TYPE_encrypt_then_mac:  // ?? empty value
			case TLSEXT_TYPE_extended_master_secret:  //  ?? empty value
			case TLSEXT_TYPE_signature_algorithms:
			case TLSEXT_TYPE_supported_versions:

			case TLSEXT_TYPE_psk_kex_modes:
				//case TLSEXT_TYPE_psk_key_exchange_modes:
			case TLSEXT_TYPE_key_share:
			case TLSEXT_TYPE_padding:  // 21 /* ExtensionType value from RFC 7685. */
			case TLSEXT_TYPE_psk:
			//case TLSEXT_TYPE_pre_shared_key: // LibreSSL symbol
				// ignore.
				break;
			case TLSEXT_TYPE_application_layer_protocol_negotiation: // 00 0C   02 68 32    08   68 74 74 70 2F 31 2E 31       ...h2.http/1.1
				if( SSL_client_hello_get0_ext( ssl, type[n], &buf, &buflen ) ) {
					int len = (int)((buf[0] << 8) | buf[1]);
					int ofs = 0;
					while( ofs < len ) {
						int plen = buf[2 + ofs];
						char const* p = (char const*)buf + 3 + ofs;
						AddLink( &pcAccept->ssl_session->protocols, SegCreateFromCharLen( p, plen ) );
						ofs += plen + 1;
					}
				}
				break;
			case TLSEXT_TYPE_status_request: // 5 HTTPS
			case TLSEXT_TYPE_renegotiate: // ff01 HTTP Header does this.  00
			case TLSEXT_TYPE_signed_certificate_timestamp: // 18 empty value.
				break;
			case TLSEXT_TYPE_server_name:
				if( SSL_client_hello_get0_ext( ssl, type[n], &buf, &buflen ) ) {
					int len = (int)((buf[0] << 8) | buf[1]);
					if( len == (buflen - 2) ) {
						int thistype = (buf[2] == TLSEXT_NAMETYPE_host_name);
						if( thistype ) {
							int strlen = (buf[3] << 8) | buf[4];
							if( strlen == buflen - 5 ) {
								INDEX idx;
								struct ssl_hostContext* hostctx;
								unsigned char const* host = buf + 5;
								pcAccept->ssl_session->hostname = DupCStrLen( (CTEXTSTR)host, strlen );
								//lprintf( "Have hostchange: %.*s", strlen, host );
								LIST_FORALL( ctxList[0], idx, struct ssl_hostContext*, hostctx ) {
									char* checkName;
									char* nextName;
									for( checkName = hostctx->host; checkName ? (nextName = StrChr( checkName, '~' )), 1 : 0; checkName = nextName ) {
										int namelen = (int)(nextName ? (nextName - checkName) : strlen);
										if( nextName ) nextName++;
										if( namelen != strlen ) {
											//lprintf( "%.*s is not %.*s", namelen, checkName, strlen, host );
											continue;
										}
										//lprintf( "Check:%.*s", )
										if( StrCaseCmpEx( checkName, (CTEXTSTR)host, strlen ) == 0 ) {
											SSL_set_SSL_CTX( ssl, hostctx->ctx );
											//lprintf( "SET CTX, and RETRY?" );
											return SSL_CLIENT_HELLO_SUCCESS;
										}
									}
								}
							}
						}
					}
				}
				break;
			default:
				if( SSL_client_hello_get0_ext( ssl, type[n], &buf, &buflen ) ) {
					lprintf( "extension : %04x(%d)", type[n], type[n] );
					LogBinary( buf, buflen );
				}
			}
		}
		OPENSSL_free( type );
	}
	/*SSL_set_tlsext_host_name(SSL, nultermName );*/
	//lprintf( "dod NOT SET CTX, continue with default." );
	return SSL_CLIENT_HELLO_SUCCESS;
	return 1; // success.
	return 0; // fail this .  set al[0]
	return -1; // pause, resume later, handshake result SSL_ERROR_WANT_CLIENT_HELLO_CB
}
#else
static int handleServerName( SSL* ssl, int* al, void* param ) {
	PCLIENT pcListener = (PCLIENT)param;
	PLIST list = pcListener->ssl_session->accepting;
	PCLIENT pcAccept;
	INDEX idx;
	LIST_FORALL( list, idx, PCLIENT, pcAccept ) {
		if( pcAccept->ssl_session && (pcAccept->ssl_session->ssl == ssl) )
			break;
	}
	if( !pcAccept ) {
		lprintf( "FATAL, NO SUCH ACCEPTING SOCKET" );
		return 0;
	}
	SetLink( &pcListener->ssl_session->accepting, idx, NULL );
	PLIST* ctxList = &pcListener->ssl_session->hosts;
	// if no hosts to check.
	if( !ctxList[0] ) return SSL_TLSEXT_ERR_OK;
	int t;
	if( TLSEXT_NAMETYPE_host_name != (t =SSL_get_servername_type( ssl ) ) ) {
		//lprintf( "Handshake sent bad hostname type... %d", t );
		//return 0;
	}
	const char* host = NULL;
	size_t strlen = 0;
	if( t ) {
		host = SSL_get_servername( ssl, t );
		strlen = StrLen( host );
		//lprintf( "ServerName;%s", host );
		//lprintf( "Have hostchange: %.*s", strlen, host );
		pcAccept->ssl_session->hostname = DupCStrLen( host, strlen );
	} else {
		// allow connection, but without a hostanme set, the application may reject later?
		// default to what? the client IP as a hostname?
	}
	struct ssl_hostContext* hostctx;
	struct ssl_hostContext* defaultHostctx;
	LIST_FORALL( ctxList[0], idx, struct ssl_hostContext*, hostctx ) {
		char const* checkName;
		char const* nextName;
		if( !hostctx->host ) {
			defaultHostctx = hostctx;
		}
		for( checkName = hostctx->host; checkName ? (nextName = StrChr( checkName, '~' )), 1 : 0; checkName = nextName ) {
			size_t namelen = nextName ? (nextName - checkName) : strlen;
			if( nextName ) nextName++;
			if( namelen != strlen ) {
				lprintf( "%.*s is not %.*s", (int)namelen, checkName, (int)strlen, host );
				continue;
			}
			//lprintf( "Check:%.*s", )
			if( !host || ( StrCaseCmpEx( checkName, (CTEXTSTR)host, strlen ) == 0 ) ) {
				SSL_set_SSL_CTX( ssl, hostctx->ctx );
				return SSL_TLSEXT_ERR_OK;
			}
		}
	}
	if( defaultHostctx ) {
		SSL_set_SSL_CTX( ssl, defaultHostctx->ctx );
		return SSL_TLSEXT_ERR_OK;
	}

	//SSL_TLSEXT_ERR_NOACK
	//SSL_TLSEXT_ERR_ALERT_FATAL
	//SSL_TLSEXT_ERR_ALERT_WARNING
	return SSL_TLSEXT_ERR_NOACK;
}
#endif

struct info_params {
	char *password;
	int passlen;
};


static int pem_password( char *buf, int size, int rwflag, void *u )
{
	// rwflag = 0  = used for decryption
	// rwflag = 1  = used for encryption
	struct info_params *params = (struct info_params*)u;
	int len;
	memcpy( buf, params->password, len = (size<params->passlen ? size : params->passlen) );
	return len;
}

LOGICAL ssl_BeginServer_v2( PCLIENT pc, CPOINTER cert, size_t certlen
                          , CPOINTER keypair, size_t keylen
                          , CPOINTER keypass, size_t keypasslen
                          , char *hosts ) {
	struct ssl_session * ses;
	struct internalCert* certStruc;
	struct ssl_hostContext* ctx;
	ses = pc->ssl_session;
	if( !ses ) {
		ses = New( struct ssl_session );
		MemSet( ses, 0, sizeof( struct ssl_session ) );
	}
	ssl_InitLibrary();
	pc->flags.bSecure = 1;

	if( !cert ) {
		if( hosts ) {
			lprintf( "Ignoring hostname for anonymous, quickshot server certificate" );
		}
		certStruc = ses->cert = MakeRequest();
		ctx = New( struct ssl_hostContext );
		ctx->host = StrDup( hosts );
		ctx->ctx = NULL;
		ctx->cert = certStruc;

		AddLink( &ses->hosts, ctx );
		if( !ses->cert )
			ses->cert = certStruc;

	} else {
		certStruc = New( struct internalCert );
		BIO *keybuf = BIO_new( BIO_s_mem() );
		X509 *x509, *result;

		certStruc->chain = sk_X509_new_null();
		certStruc->pkey = NULL;
		certStruc->x509 = NULL;
		//PKCS12_parse( )
		BIO_write( keybuf, cert, (int)certlen );
		do {
			if( !BIO_pending( keybuf ) )
				break;
			x509 = X509_new();
			result = PEM_read_bio_X509( keybuf, &x509, NULL, NULL );
			if( result )
				sk_X509_push( certStruc->chain, x509 );
			else {
				ERR_print_errors_cb( logerr, (void*)__LINE__ );
				X509_free( x509 );
			}
		} while( result );
		BIO_free( keybuf );

		{
			ctx = New( struct ssl_hostContext );
			ctx->host = StrDup( hosts );
			ctx->ctx = NULL;
			ctx->cert = certStruc;
		}
		AddLink( &ses->hosts, ctx );
		if( !ses->cert )
			ses->cert = certStruc;
	}

	if( !keypair ) {
		if( !certStruc->pkey )
			certStruc->pkey = genKey();
	} else {
		BIO *keybuf = BIO_new( BIO_s_mem() );
		struct info_params params;
		EVP_PKEY *result;
		certStruc->pkey = EVP_PKEY_new();
		BIO_write( keybuf, keypair, (int)keylen );
		params.password = (char*)keypass;
		params.passlen = (int)keypasslen;
		result = PEM_read_bio_PrivateKey( keybuf, &certStruc->pkey, pem_password, &params );
		if( !result ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		BIO_free( keybuf );
	}

#if NODE_MAJOR_VERSION < 10
	ctx->ctx = SSL_CTX_new( TLSv1_2_server_method() );
#else
	ctx->ctx = SSL_CTX_new( TLS_server_method() );
	SSL_CTX_set_min_proto_version( ctx->ctx, TLS1_2_VERSION );
#endif
	{
		int r;
		SSL_CTX_set_cipher_list( ctx->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH" );

		/*
		r = SSL_CTX_set0_chain( ctx->ctx, ses->cert->chain );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		*/
		r = SSL_CTX_use_certificate( ctx->ctx, sk_X509_value( certStruc->chain, 0 ) );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		r = SSL_CTX_use_PrivateKey( ctx->ctx, certStruc->pkey );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		{
			int n;
			for( n = 1; n < sk_X509_num( certStruc->chain ); n++ ) {
				r = SSL_CTX_add_extra_chain_cert( ctx->ctx, sk_X509_value( certStruc->chain, n ) );
				if( r <= 0 ) {
					ERR_print_errors_cb( logerr, (void*)__LINE__ );
				}
			}
		}
		r = SSL_CTX_check_private_key( ctx->ctx );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		r = SSL_CTX_set_session_id_context( ctx->ctx, (const unsigned char*)"12345678", 8 );//sizeof( ctx->ctx ) );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		//r = SSL_CTX_get_session_cache_mode( ctx->ctx );
		//r = SSL_CTX_set_session_cache_mode( ctx->ctx, SSL_SESS_CACHE_SERVER );
		r = SSL_CTX_set_session_cache_mode( ctx->ctx, SSL_SESS_CACHE_OFF );
		if( r <= 0 )
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
	}

	// these are set per-context... but only the first context is actually used?

#if !defined( LIBRESSL_VERSION_NUMBER )
	SSL_CTX_set_client_hello_cb( ctx->ctx, handleServerName, pc );// &ses->hosts );
#else
	SSL_CTX_set_tlsext_servername_callback( ctx->ctx, handleServerName );
	SSL_CTX_set_tlsext_servername_arg( ctx->ctx, pc );//&ses->hosts );
#endif

	if( !pc->ssl_session ) {
		ses->ctx = ctx->ctx;
		ses->dwOriginalFlags = pc->dwFlags;

		ses->user_connected = pc->connect.ClientConnected;
		ses->cpp_user_connected = pc->connect.CPPClientConnected;
		pc->connect.ClientConnected = ssl_ClientConnected;
		pc->dwFlags &= ~CF_CPPCONNECT;

		pc->ssl_session = ses;
	}
	// at this point pretty much have to assume
	// that it will be OK.
	return TRUE;
}


LOGICAL ssl_BeginServer( PCLIENT pc, CPOINTER cert, size_t certlen, CPOINTER keypair, size_t keylen, CPOINTER keypass, size_t keypasslen ) {
	return ssl_BeginServer_v2( pc, cert, certlen, keypair, keylen, keypass, keypasslen, NULL );
}


LOGICAL ssl_GetPrivateKey( PCLIENT pc, POINTER *keyoutbuf, size_t *keylen ) {
	if( pc && keyoutbuf && keylen ) {
		struct ssl_session * ses = pc->ssl_session;
		BIO *keybuf = BIO_new( BIO_s_mem() );
		if( !ses->cert->pkey )
			ses->cert->pkey = genKey();
		PEM_write_bio_PrivateKey( keybuf, ses->cert->pkey, NULL, NULL, 0, NULL, NULL );
		(*keylen) = BIO_pending( keybuf );
		(*keyoutbuf) = NewArray( uint8_t, (*keylen) + 1 );
		BIO_read( keybuf, (*keyoutbuf), (int)(*keylen) );
		((char*)*keyoutbuf)[(*keylen)] = 0;
		return TRUE;
	}
	return FALSE;
}


static int verify_cb( int preverify_ok, X509_STORE_CTX *ctx) {
	char    buf[256];
	X509   *err_cert;
	int     err, depth;
	SSL    *ssl;
	verify_mydata_t *mydata;
	err_cert = X509_STORE_CTX_get_current_cert(ctx);
	err = X509_STORE_CTX_get_error(ctx);
	depth = X509_STORE_CTX_get_error_depth(ctx);
	/*
	 * Retrieve the pointer to the SSL of the connection currently treated
	 * and the application specific data stored into the SSL object.
	 */
	ssl = (SSL*)X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
	mydata = (verify_mydata_t*)SSL_get_ex_data(ssl, verify_mydata_index);
	X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);
	/*
	 * Catch a too long certificate chain. The depth limit set using
	 * SSL_CTX_set_verify_depth() is by purpose set to "limit+1" so
	 * that whenever the "depth>verify_depth" condition is met, we
	 * have violated the limit and want to log this error condition.
	 * We must do it here, because the CHAIN_TOO_LONG error would not
	 * be found explicitly; only errors introduced by cutting off the
	 * additional certificates would be logged.
	 */
	if (depth > mydata->verify_depth) {
		preverify_ok = 0;
		err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
		X509_STORE_CTX_set_error(ctx, err);
	}
	if( !mydata->always_continue )
		if (!preverify_ok ) {
			lprintf("verify error:num=%d:%s:depth=%d:%s", err,
				   X509_verify_cert_error_string(err), depth, buf);
		} else if (mydata->verbose_mode) {
			lprintf("depth=%d:%s", depth, buf);
		}
	/*
	 * At this point, err contains the last verification error. We can use
	 * it for something special
	 */
	if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)) {
		X509_NAME_oneline(X509_get_issuer_name(err_cert), buf, 256);
		lprintf("issuer= %s", buf);
	}
	if (mydata->always_continue)
		return 1;
	else
		return preverify_ok;
}

LOGICAL ssl_BeginClientSession( PCLIENT pc, CPOINTER client_keypair, size_t client_keypairlen, CPOINTER keypass, size_t keypasslen, CPOINTER rootCert, size_t rootCertLen )
{
	struct ssl_session * ses;
	ssl_InitLibrary();

	ses = New( struct ssl_session );
	MemSet( ses, 0, sizeof( struct ssl_session ) );
	{
#if NODE_MAJOR_VERSION < 10
		ses->ctx = SSL_CTX_new( TLSv1_2_client_method() );
#else
		ses->ctx = SSL_CTX_new( TLS_client_method() );
		SSL_CTX_set_min_proto_version( ses->ctx, TLS1_2_VERSION );
#endif
		SSL_CTX_set_cipher_list( ses->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH" );
		ses->cert = New( struct internalCert );
		if( !client_keypair )
			ses->cert->pkey = genKey();
		else {
			BIO *keybuf = BIO_new( BIO_s_mem() );
			struct info_params params;
			BIO_write( keybuf, client_keypair, (int)client_keypairlen );
			params.password = (char*)keypass;
			params.passlen = (int)keypasslen;
			ses->cert->pkey = EVP_PKEY_new();
			if( !PEM_read_bio_PrivateKey( keybuf, &ses->cert->pkey, pem_password, &params ) ) {
				ERR_print_errors_cb( logerr, (void*)__LINE__ );
				BIO_free( keybuf );
				return FALSE;
			}
			BIO_free( keybuf );
		}
		SSL_CTX_use_PrivateKey( ses->ctx, ses->cert->pkey );
		//SSL_CTX_set_default_read_buffer_len( ses->ctx, 16384 );
	}
	ses->ssl = SSL_new( ses->ctx );

	if( rootCert ) {
		BIO *keybuf = BIO_new( BIO_s_mem() );
		X509 *cert;
		cert = X509_new();
		BIO_write( keybuf, rootCert, (int)rootCertLen );
		PEM_read_bio_X509( keybuf, &cert, NULL, NULL );
		BIO_free( keybuf );
		X509_STORE *store = SSL_CTX_get_cert_store( ses->ctx );
		X509_STORE_add_cert( store, cert );
	} else {
		X509_STORE *store = SSL_CTX_get_cert_store( ses->ctx );
		loadSystemCerts( ses->ctx, store );
	}
	SSL_set_verify( ses->ssl, SSL_VERIFY_PEER, verify_cb );
	SSL_set_ex_data( ses->ssl, verify_mydata_index, &ses->verify_data );
	ssl_InitSession( ses );
	{
		const char *addr = GetAddrName( pc->saClient );
		SSL_set_tlsext_host_name( ses->ssl, addr );
	}
	//SSL_set_default_read_buffer_len( ses->ssl, 16384 );
	SSL_set_connect_state( ses->ssl );

	ses->dwOriginalFlags = pc->dwFlags;
	ses->user_read = pc->read.ReadComplete;
	ses->cpp_user_read = pc->read.CPPReadComplete;
	ses->verify_data = verify_default;
	pc->read.ReadComplete = ssl_ReadComplete;
	pc->dwFlags &= ~CF_CPPREAD;


	pc->ssl_session = ses;
#ifdef DEBUG_SSL_IO_VERBOSE
	SSL_set_info_callback( pc->ssl_session->ssl, infoCallback );
#endif

	return TRUE;
}

LOGICAL ssl_IsClientSecure( PCLIENT pc ) {
	return pc->ssl_session != NULL;
}


void ssl_EndSecure(PCLIENT pc, POINTER buffer, size_t length ) {
	if( pc && pc->ssl_session ) {
		// revert native socket methods.
		pc->read.ReadComplete = pc->ssl_session->user_read;
		pc->close.CloseCallback = pc->ssl_session->user_close;
		pc->connect.ClientConnected = pc->ssl_session->user_connected;
		// restore all the native specified options for callbacks.
		(*(uint32_t*)&pc->dwFlags) |= (pc->ssl_session->dwOriginalFlags & (CF_CPPCONNECT | CF_CPPREAD | CF_CPPWRITE | CF_CPPCLOSE));
		// prevent close event...
		POINTER tmp;
		pc->ssl_session->user_close = NULL;
		pc->ssl_session->cpp_user_close = NULL;
		tmp = pc->ssl_session->ibuffer;
		pc->ssl_session->ibuffer = NULL;
		ssl_CloseCallback( pc );
		// SSL callback failed, but it may be possible to connect direct.
		// and if so; setup to return a redirect.
		if( buffer ) {
			if( pc->dwFlags & CF_CPPREAD ) {
				pc->read.CPPReadComplete( pc->psvRead, NULL, 0 );  // process read to get data already pending...
				pc->read.CPPReadComplete( pc->psvRead, buffer, length );
			}
			else {
				pc->read.ReadComplete( pc, NULL, 0 );
				pc->read.ReadComplete( pc, buffer, length );
			}
		}
		Release( tmp );
	}
}


CTEXTSTR ssl_GetRequestedHostName( PCLIENT pc ) {
	if( pc->ssl_session ) {
		return pc->ssl_session->hostname;
	}
	return NULL;
}



void ssl_SetIgnoreVerification( PCLIENT pc ) {
	if( pc->ssl_session ) {
		pc->ssl_session->ignoreVerification = TRUE;
		pc->ssl_session->verify_data.always_continue = TRUE;
	}
}

//--------------------- Make Cert Request
//#include <stdio.h>
//#include <stdlib.h>
//#include <openssl/x509.h>
//#include <openssl/x509v3.h>
//#include <openssl/err.h>
//#include <openssl/pem.h>
//#include <openssl/evp.h>

// Fatal error; abort with message, including file and line number
//
static int cb(const char *str, size_t len, void *u)  {
	lprintf( "%s", str);
	return 1;

}

static void fatal_error(const char *file, int line, const char *msg)
{
	fprintf(stderr, "**FATAL** %s:%i %s\n", file, line, msg);
	fprintf( stderr, "specific error available, but not dumped; ERR_print_errors_fp is gone" );
	ERR_print_errors_cb( cb, 0 );
	//ERR_print_errors_fp(stderr);
	exit(-1);
}

#define fatal(msg) fatal_error(__FILE__, __LINE__, msg)

// Parameter settings for this cert
//
#define ENTRIES (sizeof(entries)/sizeof(entries[0]))


// declare array of entries to assign to cert
struct entry
{
	const char *key;
	const char *value;
};

struct entry entries[] =
{
    { "countryName", "US" },
    { "stateOrProvinceName", "NV" },
    { "localityName", "Las Vegas" },
    { "organizationName", "d3x0r.org" },
    { "organizationalUnitName", "Development" },
    { "commonName", "Internal Project" },
};

// main ---
//
//
//int main(int argc, char *argv[])
struct internalCert * MakeRequest( void )
{
	 struct internalCert *cert = NewArray( struct internalCert, 1 );
	 int ca_len;
	 int key_len; // already cached key
	 char* ca;
	 char* key;

	 BIO *keybuf = BIO_new( BIO_s_mem() );
	 MemSet( cert, 0, sizeof( struct internalCert ) );
	 // Create evp obj to hold our rsakey
	 //
	 if( !(cert->pkey = EVP_PKEY_new()) )
		 fatal( "Could not create EVP object" );


#ifndef __NO_OPTIONS__
	 ca_len = SACK_GetProfileInt( "TLS", "CA Length", 0 );
#else
	 ca_len = 0;
#endif
	 if( ca_len ) {
		 ca = NewArray( char, ca_len + 2 );

#ifndef __NO_OPTIONS__
		 SACK_GetProfileString( "TLS", "CA Cert", "", ca, ca_len + 1 );
#endif
		 BIO_write( keybuf, ca, ca_len );
		 Deallocate( void *, ca );

		 cert->x509 = X509_new();
		 PEM_read_bio_X509( keybuf, &cert->x509, NULL, NULL );
	 }



#ifndef __NO_OPTIONS__
	 key_len = SACK_GetProfileInt( "TLS", "Key Length", 0 );
#else
	 key_len = 0;
#endif
	if( key_len ) {
		 key = NewArray( char, key_len + 2 );
#ifndef __NO_OPTIONS__
		 SACK_GetProfileString( "TLS", "Key", "", (TEXTCHAR*)key, key_len + 1 );
#endif
		 BIO_write( keybuf, key, key_len );
		 Deallocate( void *, key );
		 PEM_read_bio_PrivateKey( keybuf, &cert->pkey, NULL, NULL );
	} else {
#if 0
		RSA *rsa = RSA_new();
		BIGNUM *bne = BN_new();
		int ret;
		ret = BN_set_word( bne, kExp );
		if( ret != 1 ) {
			BN_free( bne );
			RSA_free( rsa );
			fatal( "Could not set Bignum?" );
			return NULL;
		}
		RSA_generate_key_ex( rsa, serverKBits, bne, NULL );
		if( !(EVP_PKEY_set1_RSA( cert->pkey, rsa )) )
			fatal( "Could not assign RSA key to EVP object" );

		BN_free( bne );
		RSA_free( rsa );
		{
			PEM_write_bio_PrivateKey( keybuf, cert->pkey,               /* our key from earlier */
				NULL, //EVP_des_ede3_cbc(), /* default cipher for encrypting the key on disk */
				NULL, //"replace_me",       /* passphrase required for decrypting the key on disk */
				10,                 /* length of the passphrase string */
				NULL,               /* callback for requesting a password */
				NULL                /* data to pass to the callback */
			);

			ca_len = BIO_pending( keybuf );
			ca = NewArray( char, ca_len + 1 );
			BIO_read( keybuf, ca, ca_len );
			ca[ca_len] = 0;
			SACK_WriteProfileInt( "TLS", "Key Length", ca_len );
			SACK_WriteProfileString( "TLS", "Key", (TEXTCHAR*)ca );
			Release( ca );
		}
#endif
	}

    // seed openssl's prng
    //
    /*
    if (RAND_load_file("/dev/random", -1))
        fatal("Could not seed prng");
    */
    // Generate the RSA key; we don't assign a callback to monitor progress
    // since generating keys is fast enough these days
    //

	if (!cert->x509)
	{
		X509 * x509;
		if (!cert->x509)
			cert->x509 = X509_new();
		x509 = cert->x509;
		ASN1_INTEGER_set( X509_get_serialNumber( x509 ), 1 );
		X509_gmtime_adj( X509_get_notBefore( x509 ), 0 );
		// (60 seconds * 60 minutes * 24 hours * 365 days) = 31536000.
		X509_gmtime_adj( X509_get_notAfter( x509 ), 31536000L );
		X509_set_pubkey( x509, cert->pkey );
		{
			X509_NAME *name;
			TEXTCHAR commonName[48];
			TEXTCHAR org[48];
			TEXTCHAR country[8];
#ifndef __NO_OPTIONS__
			SACK_GetProfileString( "TLS", "Default CA Common Name", "d3x0r.org", commonName, 48 );
			SACK_GetProfileString( "TLS", "Default CA Org", "Freedom Collective", org, 48 );
			SACK_GetProfileString( "TLS", "Default CA Country", "US", country, 8 );
#else
			strcpy( commonName, "d3x0r.org" );
			strcpy( org, "Freedom Collective" );
			strcpy( country, "US" );
#endif
			name = X509_get_subject_name( x509 );
			X509_NAME_add_entry_by_txt( name, "C", MBSTRING_ASC,
				(unsigned char *)country, -1, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "O", MBSTRING_ASC,
				(unsigned char *)org, -1, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "CN", MBSTRING_ASC,
				(unsigned char *)commonName, -1, -1, 0 );

			X509_set_issuer_name( x509, name );

			X509_sign( x509, cert->pkey, EVP_sha512() );

			cert->chain = sk_X509_new_null();
			sk_X509_push( cert->chain, x509 );

			{
				PEM_write_bio_X509( keybuf, x509 );
				ca_len = BIO_pending( keybuf );
				ca = NewArray( char, ca_len + 1 );

				BIO_read( keybuf, ca, ca_len );
				ca[ca_len] = 0;
#ifndef __NO_OPTIONS__
				SACK_WriteProfileInt( "TLS", "CA Length", ca_len );
				SACK_WriteProfileString( "TLS", "CA Cert", (TEXTCHAR*)ca );
#endif
				Release( ca );
			}

		}
	}
	return cert;
}

#ifdef __LINUX__
void loadSystemCerts( SSL_CTX* ctx,X509_STORE *store )
{
	// "/etc/ssl/certs"
	(void)store; // unused;
	SSL_CTX_set_default_verify_paths( ctx );
	{
		TEXTCHAR checkpath[256];
		strcpy( checkpath, "/etc/ssl/certs" );
#ifndef __NO_OPTIONS__
		SACK_GetProfileString( "TLS", "Root Path", "checkpath", commonName, 256 );
#endif
		
		SSL_CTX_load_verify_locations( ctx, NULL, checkpath );
	}
	return;
}
#endif


#ifdef _WIN32

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)

void loadSystemCerts( SSL_CTX* ctx,X509_STORE *store )
{
	(void)ctx;// unused;
	HCERTSTORE hStore;
	PCCERT_CONTEXT pContext = NULL;
	X509 *x509;

	hStore = CertOpenSystemStore((HCRYPTPROV_LEGACY)NULL, "ROOT");

	if( !hStore ) {
		lprintf( "FATAL, CANNOT OPEN ROOT STORE" );
		return;
	}

	while (pContext = CertEnumCertificatesInStore(hStore, pContext))
	{
		//uncomment the line below if you want to see the certificates as pop ups
		//CryptUIDlgViewContext(CERT_STORE_CERTIFICATE_CONTEXT, pContext,   NULL, NULL, 0, NULL);
		const unsigned char *encoded_cert = (const unsigned char *)pContext->pbCertEncoded;

		x509 = NULL;
		x509 = d2i_X509(NULL, &encoded_cert, pContext->cbCertEncoded);
		if (x509)
		{
			int i = X509_STORE_add_cert(store, x509);

			//if (i == 1)
			//	std::cout << "certificate added" << std::endl;
			//printf( "adde cert to store?", i );
			X509_free(x509);
		}
	}

	CertFreeCertificateContext(pContext);
	CertCloseStore(hStore, 0);
}

#endif


SACK_NETWORK_NAMESPACE_END
#endif
#endif
