/**
 *	@file    x509.h
 *	@version a90e925 (tag: 3-8-3-open)
 *
 *	X.509 header.
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

#ifndef _h_PS_X509
#define _h_PS_X509

#ifdef USE_X509

/******************************************************************************/

/* ClientCertificateType */
enum {
	RSA_SIGN = 1,
	DSS_SIGN,
	RSA_FIXED_DH,
	DSS_FIXED_DH,
	ECDSA_SIGN = 64,
	RSA_FIXED_ECDH,
	ECDSA_FIXED_ECDH
};

/* Parsing flags */
#define	CERT_STORE_UNPARSED_BUFFER	0x1
#define	CERT_STORE_DN_BUFFER		0x2

#ifdef USE_CERT_PARSE

/* Per specification, any critical extension in an X.509 cert should cause
	the connection to fail. SECURITY - Uncomment at your own risk */
/* #define ALLOW_UNKNOWN_CRITICAL_EXTENSIONS */

/*
	DN attributes are used outside the X509 area for cert requests,
	which have been included in the RSA portions of the code
*/
typedef struct {
	char	*country;
	char	*state;
	char	*locality;
	char	*organization;
	char	*orgUnit;
	char	*commonName;
	char	hash[MAX_HASH_SIZE];
	char	*dnenc; /* CERT_STORE_DN_BUFFER */
	uint16_t dnencLen;
	short	countryType;
	uint16_t countryLen;
	short	stateType;
	uint16_t stateLen;
	short	localityType;
	uint16_t localityLen;
	short	organizationType;
	uint16_t organizationLen;
	short	orgUnitType;
	uint16_t orgUnitLen;
	short	commonNameType;
	uint16_t commonNameLen;
} x509DNattributes_t;

typedef struct {
	int32	cA;
	int32	pathLenConstraint;
} x509extBasicConstraints_t;

typedef struct psGeneralNameEntry {
	psPool_t						*pool;
	enum {
		GN_OTHER = 0,	// OtherName
		GN_EMAIL,		// IA5String
		GN_DNS,			// IA5String
		GN_X400,		// ORAddress
		GN_DIR,			// Name
		GN_EDI,			// EDIPartyName
		GN_URI,			// IA5String
		GN_IP,			// OCTET STRING
		GN_REGID		// OBJECT IDENTIFIER
	}								id;
	unsigned char					name[16];
	unsigned char					oid[32]; /* SubjectAltName OtherName */
	unsigned char					*data;
	uint16_t						oidLen;
	uint16_t						dataLen;
	struct psGeneralNameEntry		*next;
} x509GeneralName_t;

typedef struct {
	unsigned char	*id;
	uint16_t		len;
} x509extSubjectKeyId_t;

typedef struct {
	unsigned char		*keyId;
	unsigned char		*serialNum;
	x509DNattributes_t	attribs;
	uint16_t			keyLen;
	uint16_t			serialNumLen;
} x509extAuthKeyId_t;

#ifdef USE_FULL_CERT_PARSE
typedef struct {
	x509GeneralName_t	*permitted;
	x509GeneralName_t	*excluded;
} x509nameConstraints_t;
#endif /* USE_FULL_CERT_PARSE */
 
/******************************************************************************/
/*
	OID parsing and lookup.
*/
#define MAX_OID_LEN		16	/**< Maximum number of segments in OID */

/*
	X.509 Certificate Extension OIDs
	@see https://tools.ietf.org/html/rfc5280#section-4.2

	id-ce   OBJECT IDENTIFIER ::=  { joint-iso-ccitt(2) ds(5) 29 }

	id-ce-authorityKeyIdentifier	OBJECT IDENTIFIER ::=  { id-ce 35 }
	id-ce-subjectKeyIdentifier		OBJECT IDENTIFIER ::=  { id-ce 14 }
	id-ce-keyUsage					OBJECT IDENTIFIER ::=  { id-ce 15 }
	id-ce-certificatePolicies		OBJECT IDENTIFIER ::=  { id-ce 32 }
	id-ce-policyMappings			OBJECT IDENTIFIER ::=  { id-ce 33 }
	id-ce-subjectAltName			OBJECT IDENTIFIER ::=  { id-ce 17 }
	id-ce-issuerAltName				OBJECT IDENTIFIER ::=  { id-ce 18 }
	id-ce-subjectDirectoryAttributes OBJECT IDENTIFIER ::= { id-ce  9 }
	id-ce-basicConstraints			OBJECT IDENTIFIER ::=  { id-ce 19 }
	id-ce-nameConstraints			OBJECT IDENTIFIER ::=  { id-ce 30 }
	id-ce-policyConstraints			OBJECT IDENTIFIER ::=  { id-ce 36 }
	id-ce-extKeyUsage				OBJECT IDENTIFIER ::=  { id-ce 37 }
	id-ce-cRLDistributionPoints		OBJECT IDENTIFIER ::=  { id-ce 31 }
	id-ce-inhibitAnyPolicy			OBJECT IDENTIFIER ::=  { id-ce 54 }
	id-ce-freshestCRL				OBJECT IDENTIFIER ::=  { id-ce 46 }
*/
#define id_ce  2,5,29
enum {
	id_ce_authorityKeyIdentifier = 35,
	id_ce_subjectKeyIdentifier = 14,
	id_ce_keyUsage = 15,
	id_ce_certificatePolicies = 32,
	id_ce_policyMappings = 33,
	id_ce_subjectAltName = 17,
	id_ce_issuerAltName = 18,
	id_ce_subjectDirectoryAttributes = 9,
	id_ce_basicConstraints = 19,
	id_ce_nameConstraints = 30,
	id_ce_policyConstraints = 36,
	id_ce_extKeyUsage = 37,
	id_ce_cRLDistributionPoints = 31,
	id_ce_inhibitAnyPolicy = 54,
	id_ce_freshestCRL = 46,
};

/*	id-pkix	OBJECT IDENTIFIER  ::= {
		iso(1) identified-organization(3) dod(6) internet(1)
		security(5) mechanisms(5) pkix(7) }
*/
#define id_pxix	1,3,6,1,5,5,7

/*
The following key usage purposes are defined:

   anyExtendedKeyUsage OBJECT IDENTIFIER ::= { id-ce-extKeyUsage 0 }

   id-kp OBJECT IDENTIFIER ::= { id-pkix 3 }

   id_kp_serverAuth             OBJECT IDENTIFIER ::= { id-kp 1 }
   -- TLS WWW server authentication
   -- Key usage bits that may be consistent: digitalSignature,
   -- keyEncipherment or keyAgreement

   id_kp_clientAuth             OBJECT IDENTIFIER ::= { id-kp 2 }
   -- TLS WWW client authentication
   -- Key usage bits that may be consistent: digitalSignature
   -- and/or keyAgreement

   id_kp_codeSigning             OBJECT IDENTIFIER ::= { id-kp 3 }
   -- Signing of downloadable executable code
   -- Key usage bits that may be consistent: digitalSignature

   id_kp_emailProtection         OBJECT IDENTIFIER ::= { id-kp 4 }
   -- Email protection
   -- Key usage bits that may be consistent: digitalSignature,
   -- nonRepudiation, and/or (keyEncipherment or keyAgreement)

   id_kp_timeStamping            OBJECT IDENTIFIER ::= { id-kp 8 }
   -- Binding the hash of an object to a time
   -- Key usage bits that may be consistent: digitalSignature
   -- and/or nonRepudiation

   id_kp_OCSPSigning            OBJECT IDENTIFIER ::= { id-kp 9 }
   -- Signing OCSP responses
   -- Key usage bits that may be consistent: digitalSignature
   -- and/or nonRepudiation
*/
#define id_ce_eku  id_ce,id_ce_extKeyUsage
#define id_kp  id_pxix,3
enum {
	id_ce_eku_anyExtendedKeyUsage = 0,
	id_kp_serverAuth = 1,
	id_kp_clientAuth = 2,
	id_kp_codeSigning = 3,
	id_kp_emailProtection = 4,
	id_kp_timeStamping = 8,
	id_kp_OCSPSigning = 9,
};

/*
	id-pe	OBJECT IDENTIFIER  ::=  { id-pkix 1 }

	id-pe-authorityInfoAccess		OBJECT IDENTIFIER ::= { id-pe  1 }
	id-pe-subjectInfoAccess			OBJECT IDENTIFIER ::= { id-pe 11 }
*/
#define id_pe id_pxix,1
enum {
	id_pe_authorityInfoAccess = 1,
	id_pe_subjectInfoAccess = 11,
};

#define OID_ENUM(A) oid_##A
typedef enum {
	OID_ENUM(0) = 0,
	/* X.509 certificate extensions */
	OID_ENUM(id_ce_authorityKeyIdentifier),
	OID_ENUM(id_ce_subjectKeyIdentifier),
	OID_ENUM(id_ce_keyUsage),
	OID_ENUM(id_ce_certificatePolicies),
	OID_ENUM(id_ce_policyMappings),
	OID_ENUM(id_ce_subjectAltName),
	OID_ENUM(id_ce_issuerAltName),
	OID_ENUM(id_ce_subjectDirectoryAttributes),
	OID_ENUM(id_ce_basicConstraints),
	OID_ENUM(id_ce_nameConstraints),
	OID_ENUM(id_ce_policyConstraints),
	OID_ENUM(id_ce_extKeyUsage),
	OID_ENUM(id_ce_cRLDistributionPoints),
	OID_ENUM(id_ce_inhibitAnyPolicy),
	OID_ENUM(id_ce_freshestCRL),
	OID_ENUM(id_pe_authorityInfoAccess),
	OID_ENUM(id_pe_subjectInfoAccess),
	/* Extended Key Usage */
	OID_ENUM(id_ce_eku_anyExtendedKeyUsage),
	OID_ENUM(id_kp_serverAuth),
	OID_ENUM(id_kp_clientAuth),
	OID_ENUM(id_kp_codeSigning),
	OID_ENUM(id_kp_emailProtection),
	OID_ENUM(id_kp_timeStamping),
	OID_ENUM(id_kp_OCSPSigning),
} oid_e;

/* Make the flag value, given the enum above */
#define EXT_CRIT_FLAG(A) (unsigned int)(1 << (A))

/* Flags for known keyUsage (first byte) */
#define KEY_USAGE_DIGITAL_SIGNATURE		0x0080
#define KEY_USAGE_NON_REPUDIATION		0x0040
#define KEY_USAGE_KEY_ENCIPHERMENT		0x0020
#define KEY_USAGE_DATA_ENCIPHERMENT		0x0010
#define KEY_USAGE_KEY_AGREEMENT			0x0008
#define KEY_USAGE_KEY_CERT_SIGN			0x0004
#define KEY_USAGE_CRL_SIGN				0x0002
#define KEY_USAGE_ENCIPHER_ONLY			0x0001
/* Flags for known keyUsage (second, optional byte) */
#define KEY_USAGE_DECIPHER_ONLY			0x8000

/* Flags for known extendedKeyUsage */
#define EXT_KEY_USAGE_ANY				(1 << 0)
#define EXT_KEY_USAGE_TLS_SERVER_AUTH	(1 << 1)
#define EXT_KEY_USAGE_TLS_CLIENT_AUTH	(1 << 2)
#define EXT_KEY_USAGE_CODE_SIGNING		(1 << 3)
#define EXT_KEY_USAGE_EMAIL_PROTECTION	(1 << 4)
#define EXT_KEY_USAGE_TIME_STAMPING		(1 << 8)
#define EXT_KEY_USAGE_OCSP_SIGNING		(1 << 9)

/******************************************************************************/

/* Holds the known extensions we support */
typedef struct {
	psPool_t					*pool;
	x509extBasicConstraints_t	bc;
	x509GeneralName_t			*san;
	uint32						critFlags;		/* EXT_CRIT_FLAG(EXT_KEY_USE) */
	uint32						keyUsageFlags;	/* KEY_USAGE_ */
	uint32						ekuFlags;		/* EXT_KEY_USAGE_ */
	x509extSubjectKeyId_t		sk;
	x509extAuthKeyId_t			ak;
#ifdef USE_FULL_CERT_PARSE
	x509nameConstraints_t		nameConstraints;
#endif /* USE_FULL_CERT_PARSE */
#ifdef USE_CRL
	x509GeneralName_t			*crlDist;
#endif
} x509v3extensions_t;

#endif /* USE_CERT_PARSE */

#ifdef USE_CRL
typedef struct x509revoked {
	psPool_t			*pool;
	unsigned char		*serial;
	uint16_t			serialLen;
	struct x509revoked	*next;
} x509revoked_t;
#endif



typedef struct psCert {
	psPool_t			*pool;
#ifdef USE_CERT_PARSE
	psPubKey_t			publicKey;
	int32				version;
	unsigned char		*serialNumber;
	uint16_t			serialNumberLen;
	x509DNattributes_t	issuer;
	x509DNattributes_t	subject;
	int32				notBeforeTimeType;
	int32				notAfterTimeType;
	char				*notBefore;
	char				*notAfter;
	int32				pubKeyAlgorithm; /* public key algorithm OID */
	int32				certAlgorithm; /* signature algorithm OID */
	int32				sigAlgorithm; /* signature algorithm OID */
#ifdef USE_PKCS1_PSS
	int32				pssHash; /* RSAPSS sig hash OID */
	int32				maskGen; /* RSAPSS maskgen OID */
	int32				maskHash; /* hash OID for MGF1 */
	uint16_t			saltLen; /* RSAPSS salt len param */
#endif
	unsigned char		*signature;
	unsigned char		*uniqueIssuerId;
	unsigned char		*uniqueSubjectId;
	uint16_t			signatureLen;
	uint16_t			uniqueIssuerIdLen;
	uint16_t			uniqueSubjectIdLen;
	x509v3extensions_t	extensions;
	int32				authStatus; /* See psX509AuthenticateCert doc */
	uint32				authFailFlags; /* Flags for extension check failures */
#ifdef USE_CRL
	x509revoked_t		*revoked;
#endif
	unsigned char		sigHash[MAX_HASH_SIZE];
#endif /* USE_CERT_PARSE */
#ifdef USE_OCSP
	unsigned char		sha1KeyHash[SHA1_HASH_SIZE];
#endif
#ifdef ENABLE_CA_CERT_HASH
	/** @note this is used only by MatrixSSL for Trusted CA Indication extension */
	unsigned char		sha1CertHash[SHA1_HASH_SIZE];
#endif
	unsigned char		*unparsedBin; /* see psX509ParseCertFile */
	uint16_t			binLen;
	struct psCert		*next;
} psX509Cert_t;


#ifdef USE_CERT_PARSE
extern int32_t psX509GetSignature(psPool_t *pool, const unsigned char **pp,
				uint16_t len, unsigned char **sig, uint16_t *sigLen);
extern int32_t psX509GetDNAttributes(psPool_t *pool, const unsigned char **pp,
				uint16_t len, x509DNattributes_t *attribs, uint32_t flags);
extern void psX509FreeDNStruct(x509DNattributes_t *dn, psPool_t *allocPool);
extern int32_t getSerialNum(psPool_t *pool, const unsigned char **pp,
				uint16_t len, unsigned char **sn, uint16_t *snLen);
extern int32_t getExplicitExtensions(psPool_t *pool, const unsigned char **pp,
					uint16_t inlen, int32_t expVal, x509v3extensions_t *extensions,
					uint8_t known);
extern void x509FreeExtensions(x509v3extensions_t *extensions);
extern int32_t psX509ValidateGeneralName(const char *n);
#endif /* USE_CERT_PARSE */

#ifdef USE_OCSP
/* The OCSP structure members point directly into an OCSPResponse stream.
	They are validated immediately after the parse so if a change request
	requires these fields to persist, this will all have to change */
typedef struct {
	uint16_t			certIdHashAlg; /* hashAlgorithm in CertID */
	const unsigned char	*certIdNameHash;
	const unsigned char	*certIdKeyHash;
	const unsigned char	*certIdSerial;
	short				certIdSerialLen;
	short				certStatus;
	const unsigned char	*thisUpdate;
	short				thisUpdateLen;
} mOCSPSingleResponse_t;

#define MAX_OCSP_RESPONSES 3

typedef struct {
	const unsigned char		*responderKeyHash;
	const unsigned char		*timeProduced;
	short					timeProducedLen;
	mOCSPSingleResponse_t	singleResponse[MAX_OCSP_RESPONSES];
	uint16_t				sigAlg;
	const unsigned char		*sig;
	uint16_t				sigLen;
	unsigned char			hashResult[MAX_HASH_SIZE];
	uint16_t				hashLen;
	psX509Cert_t			*OCSPResponseCert; /* Allocated to hsPool */
} mOCSPResponse_t;

extern int32_t parseOCSPResponse(psPool_t *pool, int32_t len,
					unsigned char **cp, unsigned char *end,
					mOCSPResponse_t *response);
extern int32_t validateOCSPResponse(psPool_t *pool, psX509Cert_t *trustedOCSP,
					psX509Cert_t *srvCerts,	mOCSPResponse_t *response);
#endif

/******************************************************************************/


/******************************************************************************/

#endif /* USE_X509 */

#endif /* _h_PS_X509 */

/******************************************************************************/
