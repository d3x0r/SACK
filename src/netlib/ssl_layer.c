#include <stdhdrs.h>
#include <deadstart.h>
#include <procreg.h>
#include <sqlgetoption.h>

#define _LIB
#include "matrixssl/matrixsslApi.h"

#define ALLOW_ANON_CONNECTIONS	1

#include "netstruc.h"
#ifndef UNICODE

SACK_NETWORK_NAMESPACE

struct ssl_session {
	sslKeys_t		*keys;
	//sslSessionId_t	*sid;
	ssl_t           *ssl;
	_32 dwReadFlags; // CF_CPPREAD
	cReadComplete user_read;
	cppReadComplete  cpp_user_read;
};

static struct ssl_global
{
	struct {
		BIT_FIELD bInited : 1;
	} flags;
	LOGICAL trace;
	sslSessionId_t	*sid; // reusable key data
	uint8 cipherlen;
}ssl_global;


uint16_t ciphers[] = {
 TLS_RSA_WITH_AES_128_CBC_SHA		
, TLS_DHE_RSA_WITH_AES_128_CBC_SHA		
//,	TLS_DH_anon_WITH_AES_128_CBC_SHA		
, TLS_RSA_WITH_AES_256_CBC_SHA			
, TLS_DHE_RSA_WITH_AES_256_CBC_SHA		
//,	TLS_DH_anon_WITH_AES_256_CBC_SHA		
, TLS_RSA_WITH_AES_128_CBC_SHA256			
, TLS_RSA_WITH_AES_256_CBC_SHA256			
//, TLS_DHE_RSA_WITH_AES_128_CBC_SHA256		  
//, TLS_DHE_RSA_WITH_AES_256_CBC_SHA256		
//, TLS_RSA_WITH_SEED_CBC_SHA				
//, TLS_PSK_WITH_AES_128_CBC_SHA			
//, TLS_PSK_WITH_AES_128_CBC_SHA256			
//, TLS_PSK_WITH_AES_256_CBC_SHA384			
//, TLS_PSK_WITH_AES_256_CBC_SHA			
//, TLS_DHE_PSK_WITH_AES_128_CBC_SHA		
//, TLS_DHE_PSK_WITH_AES_256_CBC_SHA		
, TLS_RSA_WITH_AES_128_GCM_SHA256			
, TLS_RSA_WITH_AES_256_GCM_SHA384			

//, TLS_EMPTY_RENEGOTIATION_INFO_SCSV		
//, TLS_FALLBACK_SCSV						

, TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA		//8
, TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA		
, TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA	
, TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA	
//, TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA		
, TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA		
, TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA		
, TLS_ECDH_RSA_WITH_AES_128_CBC_SHA		
, TLS_ECDH_RSA_WITH_AES_256_CBC_SHA		
, TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256 
, TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384 
, TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256  
, TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384  
, TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256	
, TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384	
, TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256	
, TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384	
, TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256 
, TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384 
, TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256	
, TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384	
, TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256	
, TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384	
, TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256	
, TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384	};


static const char *default_certs[] = {
	"Equifax_Secure_Certificate_Authority.der",
"Equifax_Secure_eBusiness_CA-1.der",
"Equifax_Secure_Global_eBusiness_CA-1.der",
"GeoTrust_CA_for_Adobe.der",
"GeoTrust_Global_CA.cer",
"GeoTrust_Global_CA.der",
"GeoTrust_Global_CA2.der",
"GeoTrust_Mobile_Device_Root_-_Privileged.der",
"GeoTrust_Mobile_Device_Root_-_Unprivileged.der",
"Geotrust_PCA_G3_Root.der",
"GeoTrust_Primary_CA.der",
"GeoTrust_Primary_CA_G2_ECC.der",
"GeoTrust_True_Credentials_CA_2.der",
"GeoTrust_Universal_CA.der",
"GeoTrust_Universal_CA2.der",
"thawte_Personal_Freemail_CA.der",
"thawte_Premium_Server_CA.der",
"thawte_Primary_Root_CA-G2_ECC.der",
"thawte_Primary_Root_CA-G3_SHA256.der",
"thawte_Primary_Root_CA.der",
"thawte_Server_CA.der" };


static int32 loadRsaKeys( sslKeys_t *keys, LOGICAL client )
{
	uint32 priv_key_len = 0;
	uint32 cert_key_len = 0;
	unsigned char *CAstream = NULL;
	int32 CAstreamLen = 0;
	int32 rc;
	TEXTCHAR buf[256];
	TEXTCHAR privkey_buf[256];
	TEXTCHAR cert_buf[256];
	unsigned char *priv_key = NULL;
	unsigned char *cert_key = NULL;
	FILE *file;
	size_t size;
	int fileGroup = GetFileGroup( "ssl certs", "certs" );
	{

	/*
		In-memory based keys
		Build the CA list first for potential client auth usage
	*/
		int n;
		CAstreamLen = 0;
		for( n = 0; n < ( sizeof( default_certs ) / sizeof( default_certs[0] ) ); n++ )
		{
			file = sack_fopen( fileGroup, default_certs[n], "rb" );
			if( file )
			{
				size = sack_fsize( file );
				CAstreamLen += size;
				sack_fclose( file );
			}
		}
		{
			/*
			 In-memory based keys
			 Build the CA list first for potential client auth usage
			 */
			priv_key_len = 0;
			SACK_GetProfileString( "SSL/Private Key", "filename", "myprivkey.pem", privkey_buf, 256 );
			file = sack_fopen( fileGroup, privkey_buf, "rb" );
			if( file )
			{
				priv_key_len = sack_fsize( file );
				priv_key = NewArray( _8, priv_key_len );

				sack_fread( priv_key, 1, priv_key_len, file );
				sack_fclose( file );
			}
			else
				priv_key = NULL;
		}

		SACK_GetProfileString( "SSL/Cert Authority Extra", "filename", "mycert.pem", cert_buf, 256 );
		file = sack_fopen( fileGroup, cert_buf, "rb" );
		if( file )
		{
			CAstreamLen += sack_fsize( file );
			sack_fclose( file );
		}

		if( CAstreamLen )
			CAstream = NewArray( _8, CAstreamLen);
		CAstreamLen = 0;
		for( n = 0; n < ( sizeof( default_certs ) / sizeof( default_certs[0] ) ); n++ )
		{
			file = sack_fopen( fileGroup, default_certs[n], "rb" );
			if( file )
			{
				size = sack_fsize( file );
				CAstreamLen += sack_fread( CAstream + CAstreamLen, 1, size, file );
				//rc = matrixSslLoadRsaKeysMem(keys, NULL, 0, NULL, 0, CAstream+ CAstreamLen, size );
				//lprintf( "cert success : %d %s", rc, default_certs[n] );

				//CAstream[CAstreamLen++] = '\n';
				sack_fclose( file );
			}
		}

		file = sack_fopen( fileGroup, cert_buf, "rb" );
		if( file )
		{
			size = sack_fsize( file );
			CAstreamLen += sack_fread( CAstream + CAstreamLen, 1, size, file );
			sack_fclose( file );
		}
	}

	if( 0 )
	{
	/*
		In-memory based keys
		Build the CA list first for potential client auth usage
	*/
		cert_key_len = 0;
		SACK_GetProfileString( "SSL/Certificate", "filename", "mycert.pem", cert_buf, 256 );
		file = sack_fopen( fileGroup, cert_buf, "rb" );
		if( file )
		{
			cert_key_len = sack_fsize( file );
			cert_key = NewArray( _8, cert_key_len );

			sack_fread( cert_key, 1, cert_key_len, file );
			sack_fclose( file );
		}
		else
			cert_key = NULL;
	}
	if( 0 )
	{
	/*
		In-memory based keys
		Build the CA list first for potential client auth usage
	*/
		priv_key_len = 0;
		SACK_GetProfileString( "SSL/Private Key", "filename", "myprivkey.pem", buf, 256 );
		file = sack_fopen( fileGroup, buf, "rb" );
		if( file )
		{
			priv_key_len = sack_fsize( file );
			priv_key = NewArray( _8, priv_key_len );

			sack_fread( priv_key, 1, priv_key_len, file );
			sack_fclose( file );
		}
		else
			priv_key = NULL;
	}
	if( cert_key_len || priv_key_len || CAstreamLen )
	{
		rc = matrixSslLoadRsaKeysMem(keys
			, client?NULL:cert_key, client?0:cert_key_len
			, client?NULL:priv_key, client?0:priv_key_len
			, CAstream, CAstreamLen );

		if (rc < 0) {
			lprintf("No certificate material loaded.  Exiting");
			//if (CAstream) {
			//	Deallocate(P_8, CAstream);
			//}
			matrixSslDeleteKeys(keys);
			matrixSslClose();
		}
		if( cert_key_len )
			Release( cert_key );
		if( priv_key_len )
			Release( priv_key );
		if( CAstream )
			Release( CAstream );
		return rc;
	}
	return PS_SUCCESS;
}


ATEXIT( CloseMatrixSSL )
{
	if( ssl_global.flags.bInited )
		matrixSslClose();
}


void CloseSession( PCLIENT pc )
{
	if( pc->ssl_session )
	{
		//matrixSslDeleteSessionId(pc->ssl_session->sid);

		matrixSslDeleteKeys(pc->ssl_session->keys);
		matrixSslDeleteSession( pc->ssl_session->ssl );

		Release( pc->ssl_session );
		pc->ssl_session = NULL;
	}
	if( !( pc->dwFlags & ( CF_CLOSED|CF_CLOSING ) ) )
		RemoveClient( pc );

}

/******************************************************************************/
/*
	Example callback to show possiblie outcomes of certificate validation.
	If this callback is not registered in matrixSslNewClientSession
	the connection will be accepted or closed based on the alert value.
 */
static int32_t certCb(ssl_t *ssl, psX509Cert_t *cert, int32_t alert)
{
#ifndef USE_ONLY_PSK_CIPHER_SUITE
	psX509Cert_t	*next;
		
	/* Did we even find a CA that issued the certificate? */
	if (alert == SSL_ALERT_UNKNOWN_CA) {
			/* Example to allow anonymous connections based on a define */
		if (ALLOW_ANON_CONNECTIONS) {
			if (ssl_global.trace) {
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
		//lprintf("WARNING: Certificate date window validation not implemented");
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

	
	if (ssl_global.trace && alert == 0) lprintf("SUCCESS: Validated cert for: %s.",
		cert->subject.commonName);
	
#endif /* !USE_ONLY_PSK_CIPHER_SUITE */
	return alert;
}


static int32_t extensionCb(ssl_t *ssl, uint16_t extType,
							uint8_t extLen, void *e)
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

static void ssl_ReadComplete( PCLIENT pc, POINTER buffer, size_t length )
{
	//lprintf( "SSL Read complete %p %d", buffer, length );
	if( buffer )
	{
		unsigned char *buf;
		uint32 len;

		int32 rc = 0;
		if ((rc = matrixSslReceivedData(pc->ssl_session->ssl, (int32)length, &buf,
													&len)) < 0) {
			lprintf( "Protocol failure?" );
			Release( pc->ssl_session );
			RemoveClient( pc );
			return;
		}
	Process:
		switch (rc) {
		case MATRIXSSL_REQUEST_SEND:
			while( ( len = matrixSslGetOutdata(pc->ssl_session->ssl, &buf) ) > 0 )
			{
				//lprintf( "Send..." );
				SendTCP( pc, buf, len );
				matrixSslSentData( pc->ssl_session->ssl, len);

			}
			break;
		case 0:
		case MATRIXSSL_REQUEST_RECV:
			break;  // always attempts to read more.

		case MATRIXSSL_APP_DATA:
		case MATRIXSSL_APP_DATA_COMPRESSED:
			if( pc->ssl_session->dwReadFlags & CF_CPPREAD )
				pc->ssl_session->cpp_user_read( pc->psvRead, buf, len );
			else
				pc->ssl_session->user_read( pc, buf, len );
			if( pc->ssl_session )
				rc = matrixSslProcessedData( pc->ssl_session->ssl, &buf, &len );
			else
				return;
			if( rc )
				goto Process;
			break;
		case MATRIXSSL_HANDSHAKE_COMPLETE:
			if( pc->ssl_session->dwReadFlags & CF_CPPREAD )
				pc->ssl_session->cpp_user_read( pc->psvRead, NULL, 0 );
			else
				pc->ssl_session->user_read( pc, NULL, 0 );
			break;
		case MATRIXSSL_RECEIVED_ALERT:
			/* The first byte of the buffer is the level */
			/* The second byte is the description */
			if (*buf == SSL_ALERT_LEVEL_FATAL) {
				lprintf("Fatal alert: %d, closing connection.\n",
							*(buf + 1));
				CloseSession( pc );
				return;
			}
			/* Closure alert is normal (and best) way to close */
			if (*(buf + 1) == SSL_ALERT_CLOSE_NOTIFY) {
				RemoveClient( pc );
				return;
			}
			if (*(buf + 1) == SSL_ALERT_UNRECOGNIZED_NAME) {
				// this is a normal close...
			}
			else
				lprintf("Warning alert: %d\n", *(buf + 1));
			rc = matrixSslProcessedData( pc->ssl_session->ssl, &buf, &len );
			goto Process;
		default:
			/* If rc <= 0 we fall here */
			lprintf( "Unhandled SSL Process Code: %d", rc );
			RemoveClient( pc );
			return;
		}
	}

	if( pc->ssl_session )
	{
		S_32 len;
		unsigned char *buf;
		if ((len = matrixSslGetReadbuf(pc->ssl_session->ssl, &buf)) <= 0) {
			lprintf( "need error handling failed to get a buffer?" );
		}
		else
		{
			ReadTCP( pc, buf, len );
		}
	}
}

LOGICAL ssl_Send( PCLIENT pc, POINTER buffer, size_t length )
{
	int32 available;
	unsigned char *buf;
	int32 len;
	if ((available = matrixSslGetWritebuf(pc->ssl_session->ssl, &buf, length)) < 0) {
		lprintf( "Failed to get SSL send buffer" );
		return FALSE;
	}
	memcpy( buf, buffer, length );
	if (matrixSslEncodeWritebuf(pc->ssl_session->ssl, (uint32)strlen((char *)buf)) < 0) {
		lprintf( "Failure to encode SSL send buffer" );
		return FALSE;
	}
	while( ( len = matrixSslGetOutdata(pc->ssl_session->ssl, &buf) ) > 0 )
	{
		SendTCP( pc, buf, len );
		matrixSslSentData( pc->ssl_session->ssl, len );
	}
	return TRUE;

}


LOGICAL ssl_BeginClientSession( PCLIENT pc )
{
	int32 result;
	tlsExtension_t	*extension;
	unsigned char	*ext;
	int32			extLen;
	sslSessOpts_t	options;
	const char *hostname = GetAddrName( pc->saClient );
	struct ssl_session * ses;

	if( !ssl_global.flags.bInited )
	{
		if( matrixSslOpenWithConfig( MATRIXSSL_CONFIG /* "YN" */ ) != MATRIXSSL_SUCCESS )
		{
			lprintf( "MatrixSSL Failed to initialize." );
			return FALSE;
		}
		ssl_global.cipherlen          = sizeof( ciphers ) / sizeof( ciphers[0] );
		/*
           "    53 TLS_RSA_WITH_AES_256_CBC_SHA\n"
           "    47 TLS_RSA_WITH_AES_128_CBC_SHA\n"
           "    10 SSL_RSA_WITH_3DES_EDE_CBC_SHA\n"
           "     5 SSL_RSA_WITH_RC4_128_SHA\n"
           "     4 SSL_RSA_WITH_RC4_128_MD5\n");
		*/


		//ssl_global.cipher[0]          = TLS_RSA_WITH_AES_128_CBC_SHA;
		//ssl_global.cipher[1]          = TLS_RSA_WITH_AES_256_CBC_SHA;
		//ssl_global.cipher[2]          = TLS_DHE_RSA_WITH_AES_256_CBC_SHA/*57*/;
		//ssl_global.cipher[3]          = TLS_RSA_WITH_AES_256_CBC_SHA256;
		//ssl_global.cipher[4]          = TLS_RSA_WITH_AES_256_CBC_SHA/*53*/;

		ssl_global.flags.bInited = 1;
	}

	ses = New( struct ssl_session );
	if (matrixSslNewKeys(&ses->keys, NULL) < 0) {
		lprintf("MatrixSSL library key init failure.  Exiting");
		Release( ses );
		return FALSE;
	}

	matrixSslNewSessionId(&ssl_global.sid, NULL);
	if( loadRsaKeys( ses->keys, TRUE ) < 0 )
		return FALSE;
	// PreSharedKey

	ses->dwReadFlags = pc->dwFlags;
	ses->user_read = pc->read.ReadComplete;
	ses->cpp_user_read = pc->read.CPPReadComplete;
	pc->read.ReadComplete = ssl_ReadComplete;
	pc->dwFlags &= ~CF_CPPREAD;

	pc->ssl_session = ses;

	memset(&options, 0x0, sizeof(sslSessOpts_t));
	options.versionFlag = SSL_FLAGS_TLS_1_2;
	//options.versionFlag = SSL_FLAGS_TLS_1_2;
	options.userPtr = ses->keys;

	matrixSslNewHelloExtension(&extension, NULL);

	matrixSslCreateSNIext(NULL, (unsigned char*)hostname, (uint32)strlen(hostname),
		&ext, &extLen);
	matrixSslLoadHelloExtension(extension, ext, extLen, EXT_SNI );
	psFree(ext, NULL);

	result = matrixSslNewClientSession( &pc->ssl_session->ssl, pc->ssl_session->keys
		, ssl_global.sid
		, ciphers, ssl_global.cipherlen
		, certCb
		, NULL /*g_ip*/	 // const char *expectedName
		, extension
		, extensionCb
		, &options
		);
	if( result != MATRIXSSL_REQUEST_SEND)
	{
		lprintf( "Failed to create new SSL client session, %d", result );
		return FALSE;
	}
	{
		S_32 len;
		unsigned char *buf;
		// begin initial communication
		while( (len = matrixSslGetOutdata(pc->ssl_session->ssl, &buf)) > 0 )
		{
			SendTCP( pc, buf, len );
			matrixSslSentData( pc->ssl_session->ssl, len );
		}
	}
	ssl_ReadComplete( pc, NULL, 0 );
	return TRUE;
}

static void CPROC keyGenEnd( PTRSZVAL psv, PTASK_INFO task ){
	((LOGICAL*)psv)[0] = TRUE;

}

void CreateKeyFile( void )
{
	char *args[] = { "openssl", "genrsa", "-out", "privkey.pem", "2048", NULL };
  // LaunchPeerProgram( "openssl", NULL, args, NULL, keyGenEnd, NULL, 0 );
}

const char *Something = "[ ca ]   \n\
default_ca      = CA_default      \n\
                                  \n\
[ CA_default ]                    \n\
serial = 1                        \n\
crl = ca-crl.pem                  \n\
database = ca-database.txt        \n\
name_opt = none                   \n\
cert_opt = none                   \n\
default_crl_days = 9999           \n\
default_md = md5                  \n\
                                  \n\
[ req ]                           \n\
default_bits           = 4096     \n\
days                   = 9999     \n\
distinguished_name     = req_distinguished_name\n\
attributes             = req_attributes        \n\
prompt                 = no                    \n\
output_password        = password              \n\
                                               \n\
[ req_distinguished_name ]                     \n\
C                      = US                    \n\
ST                     = NV                    \n\
L                      = Las Vegas             \n\
O                      = Freedom Collective    \n\
OU                     = experimental          \n\
CN                     = ca                    \n\
emailAddress           = noone@nowhere.net     \n\
                                               \n\
[ req_attributes ]                             \n\
challengePassword      = test                  \n";

void genkey2()
{
   //System( "openssl req -new -x509 -days 9999 -config ca.cnf -keyout ca-key.pem -out ca-cert.pem -config ca.cnf", NULL, 0 );
//	System( "openssl rsa -in ca-key.pem -out key.pem", NULL, 0 );

}



SACK_NETWORK_NAMESPACE_END
#endif