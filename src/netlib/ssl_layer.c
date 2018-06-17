


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

#if NO_SSL

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

SACK_NETWORK_NAMESPACE_END

#else

#define _LIB
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/err.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>

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

void loadSystemCerts(X509_STORE *store );

//static void gencp
struct internalCert * MakeRequest( void );


EVP_PKEY *genKey() {
	EVP_PKEY *keypair = EVP_PKEY_new();
	//int keylen;
	//char *pem_key;
	//BN_GENCB cb = { }
	BIGNUM          *bne = BN_new();
	RSA *rsa = RSA_new();
	int ret;
	ret = BN_set_word( bne, kExp );
	if( ret != 1 ) {
		BN_free( bne );
		RSA_free( rsa );
		EVP_PKEY_free( keypair );
		return NULL;
	}
	RSA_generate_key_ex( rsa, kBits, bne, NULL );
	EVP_PKEY_set1_RSA( keypair, rsa );

	BN_free( bne );
	RSA_free( rsa );

	return keypair;
}

#define ALLOW_ANON_CONNECTIONS	1

#ifndef UNICODE


struct ssl_session {
	SSL_CTX        *ctx;
	LOGICAL ignoreVerification;
	BIO *rbio;
	BIO *wbio;
	//EVP_PKEY *privkey;
	struct internalCert *cert;

	SSL            *ssl;
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
};

static struct ssl_global
{
	struct {
		BIT_FIELD bInited : 1;
	} flags;
	LOGICAL trace;
	struct tls_config *tls_config;
	uint8_t cipherlen;
}ssl_global;


ATEXIT( CloseSSL )
{
	if( ssl_global.flags.bInited ) {
	}
}


void CloseSession( PCLIENT pc )
{
	if( pc->ssl_session )
	{
		Release( pc->ssl_session );
		pc->ssl_session = NULL;
	}
	if( !( pc->dwFlags & ( CF_CLOSED|CF_CLOSING ) ) )
		RemoveClient( pc );
}

static int logerr( const char *str, size_t len, void *userdata ) {
	lprintf( "%d: %s", *((int*)&userdata), str );
	return 0;
}

static int handshake( PCLIENT pc ) {
	struct ssl_session *ses = pc->ssl_session;
	if (!SSL_is_init_finished(ses->ssl)) {
		int r;
#ifdef DEBUG_SSL_IO
		lprintf( "doing handshake...." );
#endif
		/* NOT INITIALISED */

		r = SSL_do_handshake(ses->ssl);
#ifdef DEBUG_SSL_IO
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
		if (r < 0) {

			r = SSL_get_error(ses->ssl, r);
			if( SSL_ERROR_SSL == r ) {
#ifdef DEBUG_SSL_IO
				lprintf( "SSL_Read failed... %d", r );
#endif
				ERR_print_errors_cb( logerr, (void*)__LINE__ );
				return -1;
			}
			if (SSL_ERROR_WANT_READ == r) 
			{
				size_t pending;
				while( ( pending = BIO_ctrl_pending( ses->wbio) ) > 0 ) {
					if (pending > 0) {
						int read;
						if( pending > ses->obuflen ) {
							if( ses->obuffer )
								Deallocate( uint8_t *, ses->obuffer );
							ses->obuffer = NewArray( uint8_t, ses->obuflen = pending*2 );
							//lprintf( "making obuffer bigger %d %d", pending, pending * 2 );
						}
						read = BIO_read(ses->wbio, ses->obuffer, (int)pending);
#ifdef DEBUG_SSL_IO
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
			else {
				ERR_print_errors_cb( logerr, (void*)__LINE__ );
			}
			return 0;
		}
		return 2;
	}
	else {
		/* SSL IS INITIALISED */
		return 1;
	}
}


static void ssl_ReadComplete( PCLIENT pc, POINTER buffer, size_t length )
{
#ifdef DEBUG_SSL_IO
	lprintf( "SSL Read complete %p %zd", buffer, length );
#endif
	if( pc->ssl_session )
	{
		EnterCriticalSec( &pc->ssl_session->csReadWrite );
		if( buffer )
		{
			int len;
			int hs_rc;
			//lprintf( "Read from network: %d", length );
			//LogBinary( (const uint8_t*)buffer, length );

			len = BIO_write( pc->ssl_session->rbio, buffer, (int)length );
#ifdef DEBUG_SSL_IO
			lprintf( "Wrote %zd", len );
#endif
			if( len < (int)length ) {
				lprintf( "Protocol failure?" );
				LeaveCriticalSec( &pc->ssl_session->csReadWrite );
				Release( pc->ssl_session );
				RemoveClient( pc );
				return;
			}

			if( !( hs_rc = handshake( pc ) ) ) {
				if( !pc->ssl_session ) return;
#ifdef DEBUG_SSL_IO
				// normal condition...
				lprintf( "Receive handshake not complete iBuffer" );
#endif
				LeaveCriticalSec( &pc->ssl_session->csReadWrite );
				ReadTCP( pc, pc->ssl_session->ibuffer, pc->ssl_session->ibuflen );
				return;
			}
			if( !pc->ssl_session ) return;
			// == 1 if is already done, and not newly done
			if( hs_rc == 2 ) {
				// newly completed handshake.
				if( !(pc->dwFlags & CF_READPENDING) ) {
					if( !pc->ssl_session->ignoreVerification && SSL_get_peer_certificate( pc->ssl_session->ssl ) ) {
						int r;
						if( ( r = SSL_get_verify_result( pc->ssl_session->ssl ) ) != X509_V_OK ) {
							lprintf( "Certificate verification failed. %d", r );
							LeaveCriticalSec( &pc->ssl_session->csReadWrite );
							RemoveClientEx( pc, 0, 1 );
							return;
							//ERR_print_errors_cb( logerr, (void*)__LINE__ );
						}
					}
					//lprintf( "Initial read dispatch.." );
					if( pc->ssl_session->dwOriginalFlags & CF_CPPREAD )
						pc->ssl_session->cpp_user_read( pc->psvRead, NULL, 0 );
					else
						pc->ssl_session->user_read( pc, NULL, 0 );
				}
				len = 0;
			}
			else if( hs_rc == 1 )
			{
			read_more:
				len = SSL_read( pc->ssl_session->ssl, NULL, 0 );
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
#ifdef DEBUG_SSL_IO
					lprintf( "normal read - just get the data from the other buffer : %d", len );
#endif
					if( len < 0 ) {
						int error = SSL_get_error( pc->ssl_session->ssl, len );
						if( error == SSL_ERROR_WANT_READ ) {
							lprintf( "want more data?" );
						} else
							lprintf( "SSL_Read failed. %d", error );
						//ERR_print_errors_cb( logerr, (void*)__LINE__ );
						//RemoveClient( pc );
						//return;
					}
				}
			}
			else if( hs_rc == -1 ) {
				LeaveCriticalSec( &pc->ssl_session->csReadWrite );
  				RemoveClient( pc );
  				return;
			}
			else
				len = 0;

			{
				// the read generated write data, output that data
				size_t pending = BIO_ctrl_pending( pc->ssl_session->wbio );
				if( !pc->ssl_session ) {
					lprintf( "SSL SESSION SELF DESTRUCTED!" );
				}
				if( pending > 0 ) {
					int read; 
#ifdef DEBUG_SSL_IO
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
						if( !pc->ssl_session ) return;
					}
				}
			}
			// do was have any decrypted data to give to the application?
			if( len > 0 ) {
#ifdef DEBUG_SSL_IO
				lprintf( "READ BUFFER:" );
				LogBinary( pc->ssl_session->dbuffer, len );
#endif
				if( pc->ssl_session->dwOriginalFlags & CF_CPPREAD )
					pc->ssl_session->cpp_user_read( pc->psvRead, pc->ssl_session->dbuffer, len );
				else
					pc->ssl_session->user_read( pc, pc->ssl_session->dbuffer, len );
				if( pc->ssl_session ) // might have closed during read.
					goto read_more;
			}
			else if( len == 0 ) {
#ifdef DEBUG_SSL_IO
				lprintf( "incomplete read" );
#endif
			}
			//else {
			//	lprintf( "SSL_Read failed." );
			//	ERR_print_errors_cb( logerr, (void*)__LINE__ );
			//	RemoveClient( pc );
			//}

		}
		else {
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
		}
		//lprintf( "Read more data..." );
		if( pc->ssl_session ) {
			LeaveCriticalSec( &pc->ssl_session->csReadWrite );
			ReadTCP( pc, pc->ssl_session->ibuffer, pc->ssl_session->ibuflen );
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
	while( length ) {
#ifdef DEBUG_SSL_IO
		lprintf( "SSL SEND...." );
		LogBinary( (uint8_t*)buffer, length );
#endif
		if( pending_out > 4327 )
			pending_out = 4327;
#ifdef DEBUG_SSL_IO
		lprintf( "Sending %d of %d at %d", pending_out, length, offset );
#endif
      EnterCriticalSec( &pc->ssl_session->csReadWrite );
		len = SSL_write( pc->ssl_session->ssl, (((uint8_t*)buffer) + offset), (int)pending_out );
		if (len < 0) {
			ERR_print_errors_cb(logerr, (void*)__LINE__);
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
#ifdef DEBUG_SSL_IO
		lprintf( "ssl_Send  %d", len_out );
#endif
		SendTCP( pc, ses->obuffer, len_out );
		if( pc->ssl_session )
			LeaveCriticalSec( &pc->ssl_session->csReadWrite );
	}
	return TRUE;

}




static LOGICAL ssl_InitLibrary( void ){
	if( !ssl_global.flags.bInited )
	{
		SSL_library_init();
		//tls_init();
		//ssl_global.tls_config = tls_config_new();

		SSL_load_error_strings();
		ERR_load_BIO_strings();
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
}

static void ssl_CloseCallback( PCLIENT pc ) {
	struct ssl_session *ses = pc->ssl_session;
	if (!ses) {
		lprintf("already closed?");
		return;
	}
	pc->ssl_session = NULL;
   //lprintf( "Socket got close event... notify application..." );
	if( ses->dwOriginalFlags & CF_CPPCLOSE )
		ses->cpp_user_close( pc->psvClose );
	else
		ses->user_close( pc );

   DeleteCriticalSec( &ses->csReadWrite );
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

	pcNew->ssl_session = ses;

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

LOGICAL ssl_BeginServer( PCLIENT pc, CPOINTER cert, size_t certlen, CPOINTER keypair, size_t keylen, CPOINTER keypass, size_t keypasslen ) {
	struct ssl_session * ses;
	ses = New( struct ssl_session );
	MemSet( ses, 0, sizeof( struct ssl_session ) );
	ssl_InitLibrary();
	pc->flags.bSecure = 1;

	if( !cert ) {
		ses->cert = MakeRequest();
	} else {
		struct internalCert *certStruc = New( struct internalCert );
		BIO *keybuf = BIO_new( BIO_s_mem() );
		X509 *x509, *result;
		certStruc->chain = NULL;
		certStruc->chain = sk_X509_new_null();
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
		ses->cert = certStruc;
	}

	if( !keypair ) {
		if( !ses->cert->pkey )
			ses->cert->pkey = genKey();
	} else {
		BIO *keybuf = BIO_new( BIO_s_mem() );
		struct info_params params;
		EVP_PKEY *result;
		ses->cert->pkey = EVP_PKEY_new();
		BIO_write( keybuf, keypair, (int)keylen );
		params.password = (char*)keypass;
		params.passlen = (int)keypasslen;
		result = PEM_read_bio_PrivateKey( keybuf, &ses->cert->pkey, pem_password, &params );
		if( !result ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		BIO_free( keybuf );
	}

#ifdef HACK_NODE_TLS
	ses->ctx = SSL_CTX_new( TLS_server_method()/*TLSv1_2_server_method()*/ );
#else
	ses->ctx = SSL_CTX_new( TLSv1_2_server_method() );
#endif
	{
		int r;
		SSL_CTX_set_cipher_list( ses->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH" );

		/*
		r = SSL_CTX_set0_chain( ses->ctx, ses->cert->chain );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		*/
		r = SSL_CTX_use_certificate( ses->ctx, sk_X509_value( ses->cert->chain, 0 ) );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		r = SSL_CTX_use_PrivateKey( ses->ctx, ses->cert->pkey );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		{
			int n;
			for( n = 1; n < sk_X509_num( ses->cert->chain ); n++ ) {
				r = SSL_CTX_add_extra_chain_cert( ses->ctx, sk_X509_value( ses->cert->chain, n ) );
				if( r <= 0 ) {
					ERR_print_errors_cb( logerr, (void*)__LINE__ );
				}
			}
		}
		r = SSL_CTX_check_private_key( ses->ctx );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		r = SSL_CTX_set_session_id_context( ses->ctx, (const unsigned char*)"12345678", 8 );//sizeof( ses->ctx ) );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		//r = SSL_CTX_get_session_cache_mode( ses->ctx );
		//r = SSL_CTX_set_session_cache_mode( ses->ctx, SSL_SESS_CACHE_SERVER );
		r = SSL_CTX_set_session_cache_mode( ses->ctx, SSL_SESS_CACHE_OFF );
		if( r <= 0 )
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
	}

	ses->dwOriginalFlags = pc->dwFlags;

	ses->user_connected = pc->connect.ClientConnected;
	ses->cpp_user_connected = pc->connect.CPPClientConnected;
	pc->connect.ClientConnected = ssl_ClientConnected;
	pc->dwFlags &= ~CF_CPPCONNECT;

	pc->ssl_session = ses;
	// at this point pretty much have to assume
	// that it will be OK.
	return TRUE;

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

LOGICAL ssl_BeginClientSession( PCLIENT pc, CPOINTER client_keypair, size_t client_keypairlen, CPOINTER keypass, size_t keypasslen, CPOINTER rootCert, size_t rootCertLen )
{
	struct ssl_session * ses;
	ssl_InitLibrary();

	ses = New( struct ssl_session );
	MemSet( ses, 0, sizeof( struct ssl_session ) );
	{
#ifdef NODE_MAJOR_VERSION
#  if NODE_MAJOR_VERSION >= 10
		ses->ctx = SSL_CTX_new( TLS_client_method() /*TLSv1_2_client_method()*/ );
#  else
		ses->ctx = SSL_CTX_new( TLSv1_2_client_method() );
#  endif
#else
		ses->ctx = SSL_CTX_new( TLSv1_2_client_method() );
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
		loadSystemCerts( store );
	}
	ssl_InitSession( ses );
	//SSL_set_default_read_buffer_len( ses->ssl, 16384 );
	SSL_set_connect_state( ses->ssl );

	ses->dwOriginalFlags = pc->dwFlags;
	ses->user_read = pc->read.ReadComplete;
	ses->cpp_user_read = pc->read.CPPReadComplete;
	pc->read.ReadComplete = ssl_ReadComplete;
	pc->dwFlags &= ~CF_CPPREAD;


	pc->ssl_session = ses;

	return TRUE;
}


LOGICAL ssl_IsClientSecure(PCLIENT pc) {
	return pc->ssl_session != NULL;
}

void ssl_SetIgnoreVerification( PCLIENT pc ) {
	if( pc->ssl_session )
		pc->ssl_session->ignoreVerification = TRUE;
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
				SACK_WriteProfileInt( "TLS", "CA Length", ca_len );
				SACK_WriteProfileString( "TLS", "CA Cert", (TEXTCHAR*)ca );
				Release( ca );
			}

		}
	}
	return cert;
} 

#ifdef __LINUX__
void loadSystemCerts( X509_STORE *store )
{
   return;
}
#endif


#ifdef _WIN32

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)

void loadSystemCerts( X509_STORE *store )
{
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
