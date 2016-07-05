/**
 *	@file    opensslApi.h
 *	@version a90e925 (tag: 3-8-3-open)
 *
 *	An OpenSSL interface to MatrixSSL.
 */
/*
 *	Copyright (c) 2013-2016 INSIDE Secure Corporation
 *	Copyright (c) PeerSec Networks, 2002-2011
 *	All Rights Reserved
 *
 *	The latest version of this code is available at http://www.matrixssl.org
 *
 *	This software is open source; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This General Public License does NOT permit incorporating this software
 *	into proprietary programs.  If you are unable to comply with the GPL, a
 *	commercial license for this software may be purchased from INSIDE at
 *	http://www.insidesecure.com/
 *
 *	This program is distributed in WITHOUT ANY WARRANTY; without even the
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *	See the GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	http://www.gnu.org/copyleft/gpl.html
 */

/******************************************************************************/

#ifndef _h_SSLAPI
#define _h_SSLAPI

#ifdef __cplusplus
extern "C" {
#endif

#include "opensslSocket.h"

#ifdef USE_MATRIX_OPENSSL_LAYER

#define ENABLE_ENTRY_EXIT_TRACE
#define ENABLE_UNIMPLEMENTED_TRACE

#ifdef ENABLE_ENTRY_EXIT_TRACE
#define eeTrace _psTrace
#else
#define eeTrace
#endif

#ifdef ENABLE_UNIMPLEMENTED_TRACE
#define uTrace _psTrace
#else
#define uTrace
#endif


typedef psX509Cert_t		X509_STORE_CTX;
typedef psX509Cert_t		X509_STORE;
typedef psX509Cert_t		X509;
typedef x509DNattributes_t	X509_NAME;
typedef psPubKey_t			EVP_PKEY;
typedef void				X509_LOOKUP;
typedef x509v3extensions_t	X509_EXTENSION;

/* Lookup CRLs */
#define X509_V_FLAG_CRL_CHECK           0x4
/* Lookup CRLs for whole chain */
#define X509_V_FLAG_CRL_CHECK_ALL       0x8

typedef struct {
	sslCertCb_t	method;
} X509_LOOKUP_METHOD;

typedef struct {
	psX509Cert_t	*cert_info;
} X509_CINF; /* cert info */

#define	X509_FILETYPE_PEM	1
#define X509_V_OK	0

typedef struct {
	sslKeys_t	*keys;
	int (*verify_callback)(int, X509_STORE_CTX *);
	ssl_t		*ssl;
} SSL_CTX;

typedef struct {
	SSL_CTX			*ctx;
	sslSessionId_t	*resume;
//	sslKeys_t		*keys;
	int32			fd;
	int32           outBufOffset;
	int32			appRecLen;
	int32			resumedAppDataLen;
} SSL;

typedef sslCipherSpec_t	SSL_CIPHER;

/***/

#define STACK_OF(x) x


/*** BIO ***/

typedef struct {
	sslBuf_t	*buf;
	int			type;
} BIO_METHOD;

typedef struct {
	BIO_METHOD	*method;
} BIO;

#define BIO_S_MEM_TYPE 1

typedef struct {
	int				length;
	unsigned char	*data;
} BUF_MEM;

typedef SSL				SSL_SESSION;
typedef void			SSL_METHOD;
#define SSL_METHOD_QUAL

/* use either SSL_VERIFY_NONE or SSL_VERIFY_PEER, the last 2 options
 * are 'ored' with SSL_VERIFY_PEER if they are desired */
#define SSL_VERIFY_NONE         0x00
#define SSL_VERIFY_PEER         0x01
#define SSL_VERIFY_FAIL_IF_NO_PEER_CERT 0x02
#define SSL_VERIFY_CLIENT_ONCE      0x04

#define SSL_FILETYPE_PEM	1
#define SSL_FILETYPE_ASN1	2
#define SSL_FILETYPE_ENGINE	3
#define SSL_FILETYPE_PKCS12	4

/* CONF_MFLAGS_IGNORE_MISSING_FILE if set will make CONF_load_modules_file()
	ignore missing configuration files. Normally a missing configuration file
	return an error. */
#define CONF_MFLAGS_IGNORE_MISSING_FILE 1

/* The options XN_FLAG_SEP_COMMA_PLUS, XN_FLAG_SEP_CPLUS_SPC,
	XN_FLAG_SEP_SPLUS_SPC and XN_FLAG_SEP_MULTILINE determine the field
	separators to use. Two distinct separators are used between distinct
	RelativeDistinguishedName components and separate values in the same RDN
	for a multi-valued RDN. Multi-valued RDNs are currently very rare so the
	second separator will hardly ever be used */
#define XN_FLAG_SEP_COMMA_PLUS	1
#define XN_FLAG_SEP_CPLUS_SPC	2
#define XN_FLAG_SEP_SPLUS_SPC	3
#define XN_FLAG_SEP_MULTILINE	4


#define SSL_ERROR_NONE				0
#define SSL_ERROR_SSL				1
#define SSL_ERROR_WANT_READ			2
#define SSL_ERROR_WANT_WRITE        3
#define SSL_ERROR_WANT_X509_LOOKUP  4
#define SSL_ERROR_SYSCALL			5
#define SSL_ERROR_ZERO_RETURN       6
#define SSL_ERROR_WANT_CONNECT      7
#define SSL_ERROR_WANT_ACCEPT       8

#define SSL2_VERSION			0x0002
#define SSL3_VERSION			0x0300
#define TLS1_VERSION            0x0301
#define TLS1_1_VERSION          0x0302
#define TLS1_2_VERSION          0x0303

#define NID_subject_alt_name        85
#define NID_commonName				13

#define SSL_OP_ALL                  0x80000BFFL
#define SSL_OP_NO_SSLv2                 0x01000000L
#define SSL_OP_NO_SSLv3                 0x02000000L
#define SSL_OP_NO_TLSv1                 0x04000000L
#define SSL_OP_NO_TLSv1_2               0x08000000L
#define SSL_OP_NO_TLSv1_1               0x10000000L


typedef struct asn1_string_st
{
	int length;
	int type;
	unsigned char *data;
} ASN1_STRING;

typedef unsigned char* ASN1_OBJECT;
typedef ASN1_STRING	ASN1_UTCTIME;
typedef ASN1_STRING ASN1_INTEGER;
typedef ASN1_STRING ASN1_TIME;
#define V_ASN1_UTF8STRING ASN_UTF8STRING

typedef struct GENERAL_NAME_st {

#define GEN_OTHERNAME   0
#define GEN_EMAIL   1
#define GEN_DNS     2
#define GEN_X400    3
#define GEN_DIRNAME 4
#define GEN_EDIPARTY    5
#define GEN_URI     6
#define GEN_IPADD   7
#define GEN_RID     8

int type;
union {
    ASN1_STRING *ptr;
    ASN1_STRING *otherName; /* otherName */
    ASN1_STRING *rfc822Name;
    ASN1_STRING *dNSName;
    ASN1_STRING *x400Address;
    ASN1_STRING *directoryName;
    ASN1_STRING *ediPartyName;
    ASN1_STRING *uniformResourceIdentifier;
    ASN1_STRING *iPAddress;
    ASN1_STRING *registeredID;

    /* Old names */
    ASN1_STRING *ip; /* iPAddress */
    ASN1_STRING *dirn;        /* dirn */
    ASN1_STRING *ia5;/* rfc822Name, dNSName, uniformResourceIdentifier */
    ASN1_STRING *rid; /* registeredID */
    ASN1_STRING *other; /* x400Address */
} d;
} GENERAL_NAME;




 /*	OpenSSL API */
int		SSL_library_init(void);
#define OpenSSL_add_ssl_algorithms()    SSL_library_init()
#define SSLeay_add_ssl_algorithms()     SSL_library_init()
#define OpenSSL_add_all_algorithms()    SSL_library_init()
void	SSL_load_error_strings(void);
void	ERR_free_strings(void);
void	ERR_remove_state(unsigned long pid);
void	ERR_clear_error(void);
unsigned long ERR_peek_error(void);
int		CONF_modules_load_file(const char *filename, const char *appname,
			unsigned long flags);
void	EVP_cleanup(void);

long	SSL_CTX_set_options(SSL_CTX *ctx, long options);
int		SSL_CTX_set_cipher_list(SSL_CTX *ctx, const char *str);
int		SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile,
                                   const char *CApath);
int		SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *file, int type);
int		SSL_CTX_use_certificate_chain_file(SSL_CTX *ctx, const char *file);
int		SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *file, int type);
int		SSL_CTX_check_private_key(const SSL_CTX *ctx);
X509	*SSL_get_certificate(SSL *ssl);
EVP_PKEY *X509_get_pubkey(X509 *cert);
EVP_PKEY *SSL_get_privatekey(SSL *s);
long	SSL_get_verify_result(const SSL *ssl);
int		SSL_get_error(const SSL *cp, int ret);
SSL_CIPHER *SSL_get_cipher(const SSL *ssl);
X509	*SSL_get_peer_cert_chain(const SSL *s);
X509	*SSL_get_peer_certificate(const SSL *s);

EVP_PKEY *EVP_PKEY_new(void);
void	EVP_PKEY_free(EVP_PKEY *key);
int		EVP_PKEY_copy_parameters(EVP_PKEY *to, const EVP_PKEY *from);

void	SSL_CTX_set_default_passwd_cb(SSL_CTX *ctx,
			int (*pem_password_cb)(char*, int, int, void*));
int SSL_CTX_load_rsa_key_material(SSL_CTX *ctx, const char *cert,
			const char *privkey, const char *CAfile); /* not openssl */
void SSL_CTX_set_verify(SSL_CTX *ctx, int mode,
			int (*verify_callback)(int, X509_STORE_CTX *));
X509_STORE *SSL_CTX_get_cert_store(const SSL_CTX *ctx);

SSL		*SSL_new(SSL_CTX *ctx);
void	SSL_free(SSL *cp);
SSL_CTX	*SSL_CTX_new(const SSL_METHOD *meth);
void	SSL_CTX_free(SSL_CTX *ctx);

void SSL_SESSION_free(SSL_SESSION *session);

void	*SSLv23_server_method(void);
void	*SSLv23_client_method(void);
void	*SSLv2_client_method(void);
void	*SSLv3_client_method(void);
int		SSL_set_fd(SSL *cp, int fd);

int		SSL_accept(SSL *cp);
int		SSL_connect(SSL *cp);
int		SSL_do_handshake(SSL *cp);

int		SSL_get_data(SSL *cp, unsigned char **ptBuf, int *ptBufLen);
int		SSL_processed_data(SSL *cp, unsigned char **ptBuf, int *ptBufLen);

int		SSL_pending(const SSL *ssl);
int		SSL_read(SSL *cp, void *userBuf, int userBufLen);
int		SSL_write(SSL *cp, const void *inbuf, int32 inlen);
void	SSL_shutdown(SSL *cp);

int32	SSL_cert_auth(ssl_t *ssl, psX509Cert_t *cert, int32 alert);

int		SSL_peek(SSL *s, void *buf, int num);
void	SSL_set_connect_state(SSL *ssl);
int		SSL_set_session(SSL *ssl, SSL_SESSION *session);
SSL_SESSION *SSL_get_session(const SSL *ssl);


int		SSL_version(SSL *ssl);

#define DEFAULT_BIO_BUF_LEN	1024
BIO			*BIO_new(BIO_METHOD *type);
int			BIO_free(BIO *a);
BIO_METHOD	*BIO_s_mem(void);
void		BIO_get_mem_ptr(BIO *b,BUF_MEM **pp);

typedef psDigestContext_t	MD5_CTX;
#define MD5_Init		psMd5Init
#define MD5_Update		psMd5Update
#define MD5_Final(a, b)	psMd5Final(b, a)



int PEM_write_bio_X509(BIO *bp, X509 *x);


typedef x509DNattributes_t	X509_NAME_ENTRY;
int X509_NAME_print_ex(BIO *out, X509_NAME *nm, int indent, unsigned long flags);
int X509_NAME_get_index_by_NID(X509_NAME *name,int nid,int lastpos);
X509_NAME_ENTRY *X509_NAME_get_entry(X509_NAME *name, int loc);
ASN1_STRING * X509_NAME_ENTRY_get_data(X509_NAME_ENTRY *ne);
X509 *X509_STORE_CTX_get_current_cert(X509_STORE_CTX *ctx);
X509_NAME *	X509_get_subject_name(X509 *a);
char * X509_NAME_oneline(X509_NAME *a,char *buf,int size);
X509_LOOKUP *X509_STORE_add_lookup(X509_STORE *v, X509_LOOKUP_METHOD *m);
int X509_STORE_set_flags(X509_STORE *ctx, unsigned long flags);
X509_LOOKUP_METHOD *X509_LOOKUP_file(void);
int X509_load_crl_file(X509_LOOKUP *ctx, const char *file, int type);
int sk_X509_EXTENSION_num(X509_EXTENSION *ext);
int sk_X509_num(X509 *certs);
X509_EXTENSION *sk_X509_EXTENSION_value(X509_EXTENSION *ext, int i);
ASN1_OBJECT *	X509_EXTENSION_get_object(X509_EXTENSION *ex);
int	X509_EXTENSION_get_critical(X509_EXTENSION *ex);
int X509V3_EXT_print(BIO *out, X509_EXTENSION *ext, unsigned long flag,
	int indent);
X509	*sk_X509_value(X509 *cert, int i);

const char *X509_verify_cert_error_string(long n);

void OPENSSL_load_builtin_modules(void);


/* Mongoose web server no-ops */
int		CRYPTO_num_locks(void);
void	CRYPTO_set_locking_callback(void (*cb)(int, int, const char *, int));
void	CRYPTO_set_id_callback(unsigned long (*cb)(void));
unsigned long	ERR_get_error(void);
char	*ERR_error_string(unsigned long x, char *y);


/* cURL no-ops */
int RAND_load_file(const char *filename, long max_bytes);
void RAND_add(const void *buf, int num, int entropy);
int  RAND_status(void);
const char *RAND_file_name(char *buf, size_t num);
int		RAND_bytes(unsigned char *buf, int num);




void    *   X509_get_ext_d2i(X509 *x, int nid, int *crit, int *idx);
const GENERAL_NAME *sk_GENERAL_NAME_value(GENERAL_NAME *altnames, int i);
int sk_GENERAL_NAME_num(GENERAL_NAME *names);
void GENERAL_NAMES_free(GENERAL_NAME *name);


unsigned char * ASN1_STRING_data(ASN1_STRING *x);
size_t ASN1_STRING_length(ASN1_STRING *x);
int ASN1_STRING_type(ASN1_STRING *x);
int ASN1_STRING_to_UTF8(unsigned char **out, ASN1_STRING *in);
int i2t_ASN1_OBJECT(char *buf,int buf_len,ASN1_OBJECT *a);
int ASN1_STRING_print(BIO *bp, const ASN1_STRING *v);


#define OPENSSL_malloc(x) psMalloc(NULL, x)
#define OPENSSL_free(x) psFree(x, NULL)
#ifdef __cplusplus
}
#endif

#endif /* USE_MATRIX_OPENSSL_LAYER */
#endif /* _h_SSLAPI */

/******************************************************************************/
