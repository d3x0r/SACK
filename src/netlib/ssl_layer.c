#include <stdhdrs.h>
#include <deadstart.h>
#include <procreg.h>
#include "../src/contrib/MatrixSSL/3.7.1/matrixssl/matrixsslApi.h"
#include "netstruc.h"

SACK_NETWORK_NAMESPACE

static struct sack_MatrixSSL_local
{
	struct MatrixSSL_local_flags{
		BIT_FIELD bInited : 1;
	} flags;
} *sack_ssl_local;
#define l (*sack_ssl_local)

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
   if( l.flags.bInited )
		matrixSslClose();
}

void ssl_BeginClientSession( PCLIENT pc )
{
	int32 result;
#if 0
	result = matrixSslNewClientSession( &pc->ssl.ssl_session, const sslKeys_t *keys,
					sslSessionId_t *sid, uint32 cipherSpec[], uint16 cSpecLen,
					int32 (*certCb)(ssl_t *ssl, psX509Cert_t *cert,int32 alert),
					const char *expectedName,
					tlsExtension_t *extensions,	int32 (*extCb)(ssl_t *ssl,
					unsigned short extType, unsigned short extLen, void *e),
					sslSessOpts_t *options);
#endif
}


SACK_NETWORK_NAMESPACE_END
