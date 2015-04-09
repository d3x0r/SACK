#include <stdhdrs.h>
#include <deadstart.h>
#include <procreg.h>
#include "../src/contrib/MatrixSSL/3.7.1/matrixssl/matrixsslApi.h"

#define ALLOW_ANON_CONNECTIONS	1
#define USE_HEADER_KEYS
#define ID_RSA

#include "../src/contrib/MatrixSSL/3.7.1/sampleCerts/RSA/ALL_RSA_CAS.h"
#include "../src/contrib/MatrixSSL/3.7.1/sampleCerts/EC/ALL_EC_CAS.h"
#include "../src/contrib/MatrixSSL/3.7.1/sampleCerts/ECDH_RSA/ALL_ECDH-RSA_CAS.h"
#ifdef ID_RSA
#define EXAMPLE_RSA_KEYS
#include "../src/contrib/MatrixSSL/3.7.1/sampleCerts/RSA/1024_RSA.h"
#include "../src/contrib/MatrixSSL/3.7.1/sampleCerts/RSA/1024_RSA_KEY.h"
#include "../src/contrib/MatrixSSL/3.7.1/sampleCerts/RSA/2048_RSA.h"
#include "../src/contrib/MatrixSSL/3.7.1/sampleCerts/RSA/2048_RSA_KEY.h"
#include "../src/contrib/MatrixSSL/3.7.1/sampleCerts/RSA/4096_RSA.h"
#include "../src/contrib/MatrixSSL/3.7.1/sampleCerts/RSA/4096_RSA_KEY.h"
#endif

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

#ifdef USE_HEADER_KEYS
static int32 loadRsaKeys( sslKeys_t *keys )
{
	uint32 key_len = 1024/*g_key_len*/;
	unsigned char *CAstream = NULL;
	int32 CAstreamLen;
	int32 rc;
	{
	/*
		In-memory based keys
		Build the CA list first for potential client auth usage
	*/
		CAstreamLen = 0;
#ifdef USE_RSA_CIPHER_SUITE
		CAstreamLen += sizeof(RSACAS);
#ifdef USE_ECC_CIPHER_SUITE
		CAstreamLen += sizeof(ECDHRSACAS);
#endif
#endif
#ifdef USE_ECC_CIPHER_SUITE
		CAstreamLen += sizeof(ECCAS);
#endif
		if (CAstreamLen > 0) {
			CAstream = (unsigned char*)psMalloc(NULL, CAstreamLen);
		} else {
			CAstream = NULL;
		}

		CAstreamLen = 0;
	#ifdef USE_RSA_CIPHER_SUITE
		memcpy(CAstream, RSACAS, sizeof(RSACAS));
		CAstreamLen += sizeof(RSACAS);
	#ifdef USE_ECC_CIPHER_SUITE
		memcpy(CAstream + CAstreamLen, ECDHRSACAS, sizeof(ECDHRSACAS));
		CAstreamLen += sizeof(ECDHRSACAS);
	#endif
	#endif
	#ifdef USE_ECC_CIPHER_SUITE
		memcpy(CAstream + CAstreamLen, ECCAS, sizeof(ECCAS));
		CAstreamLen += sizeof(ECCAS);
	#endif
	}

	if (key_len == 1024) {
		lprintf("Using 1024 bit RSA private key");
		rc = matrixSslLoadRsaKeysMem(keys, RSA1024, sizeof(RSA1024),
			RSA1024KEY, sizeof(RSA1024KEY), CAstream, CAstreamLen);
	} else if (key_len == 2048) {
		lprintf("Using 2048 bit RSA private key");
		rc = matrixSslLoadRsaKeysMem(keys, RSA2048, sizeof(RSA2048),
			RSA2048KEY, sizeof(RSA2048KEY), CAstream, CAstreamLen);
	} else if (key_len == 4096) {
		lprintf("Using 4096 bit RSA private key");
		rc = matrixSslLoadRsaKeysMem(keys, RSA4096, sizeof(RSA4096),
			RSA4096KEY, sizeof(RSA4096KEY), CAstream, CAstreamLen);
    } else {
		rc = -1;
		psAssert((key_len == 1024) || (key_len == 2048) || (key_len == 4096));
	}

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
#endif


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


PSSL_SESSION ssl_InitSession( void )
{
	PSSL_SESSION ses = New( struct ssl_session );
	if (matrixSslNewKeys(&ses->keys, NULL) < 0) {
		lprintf("MatrixSSL library key init failure.  Exiting");
		Release( ses );
		return NULL;
	}

	matrixSslNewSessionId(&ses->sid, NULL);
	loadRsaKeys( ses->keys );
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
