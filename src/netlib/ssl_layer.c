#include <stdhdrs.h>
#include <deadstart.h>
#include <procreg.h>
#include <sqlgetoption.h>
#include "../contrib/MatrixSSL/3.7.1/matrixssl/matrixsslApi.h"

#define ALLOW_ANON_CONNECTIONS	1

#include "netstruc.h"

SACK_NETWORK_NAMESPACE

static struct sack_MatrixSSL_local
{
	struct MatrixSSL_local_flags{
		BIT_FIELD bInited : 1;
	} flags;
	int trace;
	int port, _new, resumed;
	int ciphers;
	int version, closeServer;
	uint32 cipher[16];
} *sack_ssl_local;
#define l (*sack_ssl_local)

static int32 loadRsaKeys( sslKeys_t *keys, LOGICAL client )
{
	uint32 priv_key_len;
	uint32 pub_key_len;
	unsigned char *CAstream = NULL;
	int32 CAstreamLen;
	int32 rc;
	TEXTCHAR buf[256];
	unsigned char *priv_key;
	unsigned char *pub_key;
	FILE *file;
	size_t size;
	{
	/*
		In-memory based keys
		Build the CA list first for potential client auth usage
	*/
		SACK_GetProfileString( "SSL/Cert", "filename", "mycert.pem", buf, 256 );
		file = sack_fopen( GetFileGroup( "ssl certs", "./certs" ), buf, "rb" );
		size = sack_fsize( file );
		CAstreamLen = size;
		if (CAstreamLen > 0) {
			CAstream = (unsigned char*)psMalloc(NULL, CAstreamLen);
		} else {
			CAstream = NULL;
		}

		CAstreamLen = 0;
		CAstreamLen += sack_fread( CAstream, 1, size, file );
		sack_fclose( file );
	}

	{
	/*
		In-memory based keys
		Build the CA list first for potential client auth usage
	*/
		SACK_GetProfileString( "SSL/Public Key", "filename", "myprivkey.pem", buf, 256 );
		file = sack_fopen( GetFileGroup( "ssl certs", "./certs" ), buf, "rb" );
		pub_key_len = sack_fsize( file );
		pub_key = NewArray( _8, pub_key_len );

		sack_fread( pub_key, 1, pub_key_len, file );
		sack_fclose( file );
	}
	{
	/*
		In-memory based keys
		Build the CA list first for potential client auth usage
	*/
		SACK_GetProfileString( "SSL/Private Key", "filename", "mykey.pem", buf, 256 );
		file = sack_fopen( GetFileGroup( "ssl certs", "./certs" ), buf, "rb" );
		priv_key_len = sack_fsize( file );
		priv_key = NewArray( _8, priv_key_len );

		sack_fread( priv_key, 1, priv_key_len, file );
		sack_fclose( file );
	}

	rc = matrixSslLoadRsaKeysMem(keys, pub_key, pub_key_len,
			priv_key, priv_key_len, CAstream, CAstreamLen);

	if (rc < 0) {
		lprintf("No certificate material loaded.  Exiting");
		if (CAstream) {
			psFree(CAstream, NULL);
		}
		matrixSslDeleteKeys(keys);
		matrixSslClose();
	}

	return rc;
}


PRELOAD( InitMatrixSSL )
{
   SimpleRegisterAndCreateGlobal( sack_ssl_local );
	if( matrixSslOpenWithConfig( MATRIXSSL_CONFIG /* "YN" */ ) != MATRIXSSL_SUCCESS )
		return;
   //matrixSslNewKeys(
	l.flags.bInited = 1;
}

ATEXIT( CloseMatrixSSL )
{
   if( sack_ssl_local && l.flags.bInited )
		matrixSslClose();
}

struct ssl_session {
	sslKeys_t		*keys;
	sslSessionId_t	*sid;
	ssl_t           *ssl;
};


PSSL_SESSION ssl_InitSession( LOGICAL client )
{
	PSSL_SESSION ses = New( struct ssl_session );
	if (matrixSslNewKeys(&ses->keys, NULL) < 0) {
		lprintf("MatrixSSL library key init failure.  Exiting");
		Release( ses );
		return NULL;
	}

	matrixSslNewSessionId(&ses->sid, NULL);
	loadRsaKeys( ses->keys, client );
	// PreSharedKey
	//matrixSslLoadPsk( ses->keys, key, sizeof key, id, sizeof( id ) );
	return ses;
}

void CloseSession( PSSL_SESSION ses )
{

	matrixSslDeleteSessionId(ses->sid);

	matrixSslDeleteKeys(ses->keys);
	Release( ses );

}

/******************************************************************************/
/*
	Example callback to show possiblie outcomes of certificate validation.
	If this callback is not registered in matrixSslNewClientSession
	the connection will be accepted or closed based on the alert value.
 */
static int32 certCb(ssl_t *ssl, psX509Cert_t *cert, int32 alert)
{
#ifndef USE_ONLY_PSK_CIPHER_SUITE
	psX509Cert_t	*next;
		
	/* Did we even find a CA that issued the certificate? */
	if (alert == SSL_ALERT_UNKNOWN_CA) {
			/* Example to allow anonymous connections based on a define */
		if (ALLOW_ANON_CONNECTIONS) {
			if (l.trace) {
				lprintf("Allowing anonymous connection for: %s.",
						cert->subject.commonName);
			}
			return SSL_ALLOW_ANON_CONNECTION;
		}
		lprintf("ERROR: No matching CA found.  Terminating connection");
	}

	/*
 		If the expectedName passed to matrixSslNewClientSession does not
		match any of the server subject name or subjAltNames, we will have
		the alert below.
		For security, the expected name (typically a domain name) _must_
		match one of the certificate subject names, or the connection
		should not continue.
		The default MatrixSSL certificates use localhost and 127.0.0.1 as
		the subjects, so unless the server IP matches one of those, this
		alert will happen.
		To temporarily disable the subjet name validation, NULL can be passed
		as expectedName to matrixNewClientSession.
	*/
	if (alert == SSL_ALERT_CERTIFICATE_UNKNOWN) {
		lprintf("ERROR: %s not found in cert subject names",
			ssl->expectedName);
	}
	
	if (alert == SSL_ALERT_CERTIFICATE_EXPIRED) {
#ifdef POSIX
		lprintf("ERROR: A cert did not fall within the notBefore/notAfter window");
#else
		lprintf("WARNING: Certificate date window validation not implemented");
		alert = 0;
#endif
	}
	
	if (alert == SSL_ALERT_ILLEGAL_PARAMETER) {
		lprintf("ERROR: Found correct CA but X.509 extension details are wrong");
	}
	
	/* Key usage related problems */
	next = cert;
	while (next) {
		if (next->authStatus == PS_CERT_AUTH_FAIL_EXTENSION) {
			if (cert->authFailFlags & PS_CERT_AUTH_FAIL_KEY_USAGE_FLAG) {
				lprintf("CA keyUsage extension doesn't allow cert signing");
			}
			if (cert->authFailFlags & PS_CERT_AUTH_FAIL_EKU_FLAG) {
				lprintf("Cert extendedKeyUsage extension doesn't allow TLS");
			}
		}
		next = next->next;
	}
	
	if (alert == SSL_ALERT_BAD_CERTIFICATE) {
		/* Should never let a connection happen if this is set.  There was
			either a problem in the presented chain or in the final CA test */
		lprintf("ERROR: Problem in certificate validation.  Exiting.");	
	}

	
	if (l.trace && alert == 0) lprintf("SUCCESS: Validated cert for: %s.",
		cert->subject.commonName);
	
#endif /* !USE_ONLY_PSK_CIPHER_SUITE */
	return alert;
}


static int32 extensionCb(ssl_t *ssl, unsigned short extType,
							unsigned short extLen, void *e)
{
	unsigned char	*c;
	short			len;
	char			proto[128];
	
	c = (unsigned char*)e;
	
	if (extType == EXT_ALPN) {
		memset(proto, 0x0, 128);
		/* one byte proto len, then proto */
		len = *c; c++;
		memcpy(proto, c, len);
		lprintf("Server agreed to use %s", proto);
	}
	return PS_SUCCESS;
}

void ssl_BeginClientSession( PCLIENT pc, PSSL_SESSION ses )
{
	int32 result;
	tlsExtension_t	*extension;
	unsigned char	*ext;
	int32			extLen;
	sslSessOpts_t	options;
	const char *hostname = GetAddrName( pc->saClient );

	memset(&options, 0x0, sizeof(sslSessOpts_t));
	options.versionFlag = SSL_FLAGS_TLS_1_2;
	options.userPtr = ses->keys;

	matrixSslNewHelloExtension(&extension, NULL);

	matrixSslCreateSNIext(NULL, (unsigned char*)hostname, (uint32)strlen(hostname),
		&ext, &extLen);
	matrixSslLoadHelloExtension(extension, ext, extLen, EXT_SERVER_NAME);
	psFree(ext, NULL);

	result = matrixSslNewClientSession( &pc->ssl_session->ssl, pc->ssl_session->keys
		, pc->ssl_session->sid
		, l.cipher, l.ciphers
		, certCb
		, NULL /*g_ip*/    // const char *expectedName
		, extension
		, extensionCb
		, &options
		);


}


SACK_NETWORK_NAMESPACE_END
