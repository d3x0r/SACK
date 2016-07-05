/**
 *	@file    x509.c
 *	@version a90e925 (tag: 3-8-3-open)
 *
 *	X.509 Parser.
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

#include "../cryptoApi.h"

#ifdef USE_X509

/******************************************************************************/

#ifdef POSIX
#include <time.h>
#endif

/******************************************************************************/

#define MAX_CERTS_PER_FILE		16

#ifdef USE_CERT_PARSE
/*
	Certificate extensions
*/
#define IMPLICIT_ISSUER_ID		1
#define IMPLICIT_SUBJECT_ID		2
#define EXPLICIT_EXTENSION		3

/*
	Distinguished Name attributes
*/
#define ATTRIB_COUNTRY_NAME		6
#define ATTRIB_LOCALITY			7
#define ATTRIB_ORGANIZATION		10
#define ATTRIB_ORG_UNIT			11
#define ATTRIB_DN_QUALIFIER		46
#define ATTRIB_STATE_PROVINCE	8
#define ATTRIB_COMMON_NAME		3

/** Enumerate X.509 milestones for issuedBefore() api */
typedef enum {
	RFC_6818,	/* January 2013 X.509 Updates Below */
	RFC_5280,	/* May 2008 X.509 Obsoletes Below */
	RFC_3280, 	/* April 2002 X.509 Obsoletes Below */
	RFC_2459,	/* January 1999 X.509 First RFC */
	X509_V3,	/* 1996 X.509v3 Pre-RFC */
	X509_V2,	/* 1993 X.509v2 Pre-RFC */
	X509_V1,	/* 1988 X.509v1 Pre-RFC */
} rfc_e;

#define USE_OID_TRACE

#ifdef USE_OID_TRACE
#define OID_LIST(A, B) { { A, B }, #B, oid_##B }
#else
#define OID_LIST(A, B) { { A, B }, oid_##B }
#endif
static const struct {
	uint16_t		oid[MAX_OID_LEN];
#ifdef USE_OID_TRACE
	char		name[32];
#endif
	int			id;
} oid_list[] = {
	/* X.509 certificate extensions */
	OID_LIST(id_ce, id_ce_authorityKeyIdentifier),
	OID_LIST(id_ce, id_ce_subjectKeyIdentifier),
	OID_LIST(id_ce, id_ce_keyUsage),
	OID_LIST(id_ce, id_ce_certificatePolicies),
	OID_LIST(id_ce, id_ce_policyMappings),
	OID_LIST(id_ce, id_ce_subjectAltName),
	OID_LIST(id_ce, id_ce_issuerAltName),
	OID_LIST(id_ce, id_ce_subjectDirectoryAttributes),
	OID_LIST(id_ce, id_ce_basicConstraints),
	OID_LIST(id_ce, id_ce_nameConstraints),
	OID_LIST(id_ce, id_ce_policyConstraints),
	OID_LIST(id_ce, id_ce_extKeyUsage),
	OID_LIST(id_ce, id_ce_cRLDistributionPoints),
	OID_LIST(id_ce, id_ce_inhibitAnyPolicy),
	OID_LIST(id_ce, id_ce_freshestCRL),
	OID_LIST(id_pe, id_pe_authorityInfoAccess),
	OID_LIST(id_pe, id_pe_subjectInfoAccess),
	/* Extended Key Usage */
	OID_LIST(id_ce_eku, id_ce_eku_anyExtendedKeyUsage),
	OID_LIST(id_kp, id_kp_serverAuth),
	OID_LIST(id_kp, id_kp_clientAuth),
	OID_LIST(id_kp, id_kp_codeSigning),
	OID_LIST(id_kp, id_kp_emailProtection),
	OID_LIST(id_kp, id_kp_timeStamping),
	OID_LIST(id_kp, id_kp_OCSPSigning),
	
	/* List terminator */
	OID_LIST(0, 0),
};

/*
	Hybrid ASN.1/X.509 cert parsing helpers
*/
static int32_t getExplicitVersion(const unsigned char **pp, uint16_t len,
				int32_t expVal, int32_t *val);
static int32_t getTimeValidity(psPool_t *pool, const unsigned char **pp,
				uint16_t len,
				int32_t *notBeforeTimeType, int32_t *notAfterTimeType,
				char **notBefore, char **notAfter);
static int32_t getImplicitBitString(psPool_t *pool, const unsigned char **pp,
				uint16_t len, int32_t impVal, unsigned char **bitString,
				uint16_t *bitLen);
static int32_t validateDateRange(psX509Cert_t *cert);
static int32_t issuedBefore(rfc_e rfc, const psX509Cert_t *cert);

#ifdef USE_RSA
static int32_t x509ConfirmSignature(const unsigned char *sigHash,
				const unsigned char *sigOut, uint16_t sigLen);
#endif

#ifdef USE_CRL
static void x509FreeRevoked(x509revoked_t **revoked);
#endif

#endif /* USE_CERT_PARSE */

/******************************************************************************/
#ifdef MATRIX_USE_FILE_SYSTEM
/******************************************************************************/

static int32_t pemCertFileBufToX509(psPool_t *pool, const unsigned char *fileBuf,
				uint16_t fileBufLen, psList_t **x509certList);

/******************************************************************************/
/*
	Open a PEM X.509 certificate file and parse it

	Memory info:
		Caller must free outcert with psX509FreeCert on function success
		Caller does not have to free outcert on function failure
*/
int32 psX509ParseCertFile(psPool_t *pool, char *fileName,
						psX509Cert_t **outcert, int32 flags)
{
	int32			fileBufLen, err;
	unsigned char	*fileBuf;
	psList_t		*fileList, *currentFile, *x509list, *frontX509;
	psX509Cert_t	*currentCert, *firstCert, *prevCert;

	*outcert = NULL;
/*
	First test to see if there are multiple files being passed in.
	Looking for a semi-colon delimiter
*/
	if ((err = psParseList(pool, fileName, ';', &fileList)) < 0) {
		return err;
	}
	currentFile = fileList;
	firstCert = prevCert = NULL;

	/* Recurse each individual file */
	while (currentFile) {
		if ((err = psGetFileBuf(pool, (char*)currentFile->item, &fileBuf,
				&fileBufLen)) < PS_SUCCESS) {
			psFreeList(fileList, pool);
			if (firstCert) psX509FreeCert(firstCert);
			return err;
		}

		if ((err = pemCertFileBufToX509(pool, fileBuf, fileBufLen, &x509list))
				< PS_SUCCESS) {
			psFreeList(fileList, pool);
			psFree(fileBuf, pool);
			if (firstCert) psX509FreeCert(firstCert);
			return err;
		}
		psFree(fileBuf, pool);

		frontX509 = x509list;
/*
		Recurse each individual cert buffer from within the file
*/
		while (x509list != NULL) {
			if ((err = psX509ParseCert(pool, x509list->item, x509list->len,
					&currentCert, flags)) < PS_SUCCESS) {
				psX509FreeCert(currentCert);
				psFreeList(fileList, pool);
				psFreeList(frontX509, pool);
				if (firstCert) psX509FreeCert(firstCert);
				return err;
			}

			x509list = x509list->next;
			if (firstCert == NULL) {
				firstCert = currentCert;
			} else {
				prevCert->next = currentCert;
			}
			prevCert = currentCert;
			currentCert = currentCert->next;
		}
		currentFile = currentFile->next;
		psFreeList(frontX509, pool);
	}
	psFreeList(fileList, pool);

	*outcert = firstCert;

	return PS_SUCCESS;
}

/******************************************************************************/
/*
*/
static int32_t pemCertFileBufToX509(psPool_t *pool, const unsigned char *fileBuf,
				uint16_t fileBufLen, psList_t **x509certList)
{
	psList_t		*front, *prev, *current;
	unsigned char	*start, *end, *endTmp;
	const unsigned char *chFileBuf;
	unsigned char	l;

	*x509certList = NULL;
	prev = NULL;
	if (fileBufLen < 0 || fileBuf == NULL) {
		psTraceCrypto("Bad parameters to pemCertFileBufToX509\n");
		return PS_ARG_FAIL;
	}
	front = current = psMalloc(pool, sizeof(psList_t));
	if (current == NULL) {
		psError("Memory allocation error first pemCertFileBufToX509\n");
		return PS_MEM_FAIL;
	}
	l = strlen("CERTIFICATE-----");
	memset(current, 0x0, sizeof(psList_t));
	chFileBuf = fileBuf;
	while (fileBufLen > 0) {
		if (
((start = (unsigned char *)strstr((char *)chFileBuf, "-----BEGIN")) != NULL) &&
((start = (unsigned char *)strstr((char *)chFileBuf, "CERTIFICATE-----")) != NULL) &&
((end = (unsigned char *)strstr((char *)start, "-----END")) != NULL) &&
((endTmp = (unsigned char *)strstr((char *)end,"CERTIFICATE-----")) != NULL)
		) {
			start += l;
			if (current == NULL) {
				current = psMalloc(pool, sizeof(psList_t));
				if (current == NULL) {
					psFreeList(front, pool);
					psError("Memory allocation error: pemCertFileBufToX509\n");
					return PS_MEM_FAIL;
				}
				memset(current, 0x0, sizeof(psList_t));
				prev->next = current;
			}
			current->len = (uint16_t)(end - start);
			end = endTmp + l;
			while (*end == '\x0d' || *end == '\x0a' || *end == '\x09'
				   || *end == ' ') {
				end++;
			}
		} else {
			psFreeList(front, pool);
			psTraceCrypto("File buffer does not look to be X.509 PEM format\n");
			return PS_PARSE_FAIL;
		}
		current->item = psMalloc(pool, current->len);
		if (current->item == NULL) {
			psFreeList(front, pool);
			psError("Memory allocation error: pemCertFileBufToX509\n");
			return PS_MEM_FAIL;
		}
		memset(current->item, '\0', current->len);

		fileBufLen -= (uint16_t)(end - fileBuf);
		fileBuf = end;

		if (psBase64decode(start, current->len, current->item, &current->len) != 0) {
			psFreeList(front, pool);
			psTraceCrypto("Unable to base64 decode certificate\n");
			return PS_PARSE_FAIL;
		}
		prev = current;
		current = current->next;
		chFileBuf = fileBuf;
	}
	*x509certList = front;
	return PS_SUCCESS;
}
#endif /* MATRIX_USE_FILE_SYSTEM */
/******************************************************************************/


#ifdef USE_PKCS1_PSS
/*
	RSASSA-PSS-params ::= SEQUENCE {
		hashAlgorithm      [0] HashAlgorithm    DEFAULT sha1,
		maskGenAlgorithm   [1] MaskGenAlgorithm DEFAULT mgf1SHA1,
		saltLength         [2] INTEGER          DEFAULT 20,
		trailerField       [3] TrailerField     DEFAULT trailerFieldBC
	}
	Note, each of these is sequential, but optional.
*/
static int32 getRsaPssParams(const unsigned char **pp, int32 size,
				psX509Cert_t *cert,	int32 secondPass)
{
	const unsigned char	*p, *end;
	int32			oi, second, asnint;
	uint16_t		plen;

	p = *pp;
	/* SEQUENCE has already been pulled off into size */
	end = p + size;

	/* The signature algorithm appears twice in an X.509 cert and must be
		identical.  If secondPass is set we check for that */
	if ((uint32)(end - p) < 1) {
		return PS_PARSE_FAIL;
	}
	if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 0)) {
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &plen) < 0 ||
				(end - p) < plen) {
			psTraceCrypto("Error parsing rsapss hash alg\n");
			return PS_PARSE_FAIL;
		}
		/* hashAlgorithm is OID */
		if (getAsnAlgorithmIdentifier(&p, (uint32)(end - p), &oi, &plen) < 0) {
			psTraceCrypto("Error parsing rsapss hash alg 2\n");
			return PS_PARSE_FAIL;
		}
		if (secondPass) {
			if (oi != cert->pssHash) {
				psTraceCrypto("rsapss hash alg doesn't repeat\n");
				return PS_PARSE_FAIL;
			}
			/* Convert to PKCS1_ ID for pssDecode on second pass */
			if (oi == OID_SHA1_ALG) {
				second = PKCS1_SHA1_ID;
			} else if (oi == OID_SHA256_ALG) {
				second = PKCS1_SHA256_ID;
			} else if (oi == OID_MD5_ALG) {
				second = PKCS1_MD5_ID;
#ifdef USE_SHA384
			} else if (oi == OID_SHA384_ALG) {
				second = PKCS1_SHA384_ID;
#endif
#ifdef USE_SHA512
			} else if (oi == OID_SHA512_ALG) {
				second = PKCS1_SHA512_ID;
#endif
			} else {
				psTraceCrypto("Unsupported rsapss hash alg\n");
				return PS_UNSUPPORTED_FAIL;
			}
			cert->pssHash = second;
		} else {
			/* first time, save the OID for compare */
			cert->pssHash = oi;
		}
	}
	if ((uint32)(end - p) < 1) {
		return PS_PARSE_FAIL;
	}
	if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 1)) {
		/* maskGenAlgorthm is OID */
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &plen) < 0 ||
				(end - p) < plen) {
			psTraceCrypto("Error parsing mask gen alg\n");
			return PS_PARSE_FAIL;
		}
		if (getAsnAlgorithmIdentifier(&p, (uint32)(end - p), &oi, &plen) < 0) {
			psTraceCrypto("Error parsing mask gen alg 2\n");
			return PS_PARSE_FAIL;
		}
		if (secondPass) {
			if (oi != cert->maskGen) {
				psTraceCrypto("rsapss mask gen alg doesn't repeat\n");
				return PS_PARSE_FAIL;
			}
		}
		cert->maskGen = oi;
		if (cert->maskGen != OID_ID_MGF1) {
			psTraceCrypto("Unsupported RSASSA-PSS maskGenAlgorithm\n");
			return PS_UNSUPPORTED_FAIL;
		}
		/*  MaskGenAlgorithm ::= AlgorithmIdentifier {
				{PKCS1MGFAlgorithms}
			}
			PKCS1MGFAlgorithms    ALGORITHM-IDENTIFIER ::= {
				{ OID id-mgf1 PARAMETERS HashAlgorithm },
				...  -- Allows for future expansion --
			}

			The default mask generation function is MGF1 with SHA-1:

			mgf1SHA1    MaskGenAlgorithm ::= {
				algorithm   id-mgf1,
				parameters  HashAlgorithm : sha1
			}
		*/
		if (getAsnAlgorithmIdentifier(&p, (uint32)(end - p), &oi, &plen) < 0) {
			psTraceCrypto("Error parsing mask hash alg\n");
			return PS_PARSE_FAIL;
		}
		if (secondPass) {
			if (oi != cert->maskHash) {
				psTraceCrypto("rsapss mask hash alg doesn't repeat\n");
				return PS_PARSE_FAIL;
			}
		}
		cert->maskHash = oi;
	}
	if ((uint32)(end - p) < 1) {
		return PS_PARSE_FAIL;
	}
	if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 2)) {
		/* saltLen */
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &plen) < 0 ||
				(end - p) < plen) {
			psTraceCrypto("Error parsing salt len\n");
			return PS_PARSE_FAIL;
		}
		if (getAsnInteger(&p, (uint32)(end - p), &asnint) < 0) {
			psTraceCrypto("Error parsing salt len 2\n");
			return PS_PARSE_FAIL;
		}
		if (secondPass) {
			if (asnint != cert->saltLen) {
				psTraceCrypto("Error: salt len doesn't repeat\n");
				return PS_PARSE_FAIL;
			}
		}
		cert->saltLen = plen;
	}
	if ((uint32)(end - p) < 1) {
		return PS_PARSE_FAIL;
	}
	if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 3)) {
		/* It shall be 1 for this version of the document, which represents
			the trailer field with hexadecimal value 0xBC */
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &plen) < 0 ||
				(end - p) < plen) {
			psTraceCrypto("Error parsing rsapss trailer\n");
			return PS_PARSE_FAIL;
		}
		if (getAsnInteger(&p, (uint32)(end - p), &asnint) < 0 ||
				plen != 0xBC) {
			psTraceCrypto("Error parsing rsapss trailer 2\n");
			return PS_PARSE_FAIL;
		}
	}

	if (p != end) {
		return PS_PARSE_FAIL;
	}
	*pp = (unsigned char*)p;
	return PS_SUCCESS;
}
#endif

/******************************************************************************/
/*
	Parse an X509 v3 ASN.1 certificate stream
	http://tools.ietf.org/html/rfc3280

	flags
		CERT_STORE_UNPARSED_BUFFER
		CERT_STORE_DN_BUFFER

	Memory info:
		Caller must always free outcert with psX509FreeCert.  Even on failure
*/
int32 psX509ParseCert(psPool_t *pool, const unsigned char *pp, uint32 size,
						psX509Cert_t **outcert, int32 flags)
{
	psX509Cert_t		*cert;
	const unsigned char	*p, *end, *far_end, *certStart;
	uint16_t			len;
	uint32_t			oneCertLen;
	int32				parsing, rc;
	unsigned char		sha1KeyHash[SHA1_HASH_SIZE];
#ifdef USE_CERT_PARSE
	psDigestContext_t	hashCtx;
	const unsigned char	*certEnd;
	uint16_t			certLen, plen;
#endif

/*
	Allocate the cert structure right away.  User MUST always call
	psX509FreeCert regardless of whether this function succeeds.
	memset is important because the test for NULL is what is used
	to determine what to free
*/
	*outcert = cert = psMalloc(pool, sizeof(psX509Cert_t));
	if (cert == NULL) {
		psError("Memory allocation failure in psX509ParseCert\n");
		return PS_MEM_FAIL;
	}
	memset(cert, 0x0, sizeof(psX509Cert_t));
	cert->pool = pool;

	p = pp;
	far_end = p + size;
/*
	Certificate  ::=  SEQUENCE  {
		tbsCertificate		TBSCertificate,
		signatureAlgorithm	AlgorithmIdentifier,
		signatureValue		BIT STRING }
*/
	parsing = 1;
	while (parsing) {
		certStart = p;
		if ((rc = getAsnSequence32(&p, (uint32_t)(far_end - p), &oneCertLen, 0))
				< 0){
			psTraceCrypto("Initial cert parse error\n");
			return rc;
		}
		/* The whole list of certs could be > 64K bytes, but we still
			restrict individual certs to 64KB */
		if (oneCertLen > 0xFFFF) {
			psAssert(oneCertLen <= 0xFFFF);
			return PS_FAILURE;
		}
		end = p + oneCertLen;
/*
		 If the user has specified to keep the ASN.1 buffer in the X.509
		 structure, now is the time to account for it
*/
		if (flags & CERT_STORE_UNPARSED_BUFFER) {
			cert->binLen = oneCertLen + (int32)(p - certStart);
			cert->unparsedBin = psMalloc(pool, cert->binLen);
			if (cert->unparsedBin == NULL) {
				psError("Memory allocation error in psX509ParseCert\n");
				return PS_MEM_FAIL;
			}
			memcpy(cert->unparsedBin, certStart, cert->binLen);
		}

#ifdef ENABLE_CA_CERT_HASH
		/* We use the cert_sha1_hash type for the Trusted CA Indication so
			run a SHA1 has over the entire Certificate DER encoding. */
		psSha1Init(&hashCtx.sha1);
		psSha1Update(&hashCtx.sha1, certStart,
			oneCertLen + (int32)(p - certStart));
		psSha1Final(&hashCtx.sha1, cert->sha1CertHash);
#endif

#ifdef USE_CERT_PARSE
		certStart = p;
/*
		TBSCertificate  ::=  SEQUENCE  {
		version			[0]		EXPLICIT Version DEFAULT v1,
		serialNumber			CertificateSerialNumber,
		signature				AlgorithmIdentifier,
		issuer					Name,
		validity				Validity,
		subject					Name,
		subjectPublicKeyInfo	SubjectPublicKeyInfo,
		issuerUniqueID	[1]		IMPLICIT UniqueIdentifier OPTIONAL,
							-- If present, version shall be v2 or v3
		subjectUniqueID	[2]	IMPLICIT UniqueIdentifier OPTIONAL,
							-- If present, version shall be v2 or v3
		extensions		[3]	EXPLICIT Extensions OPTIONAL
							-- If present, version shall be v3	}
*/
		if ((rc = getAsnSequence(&p, (uint32)(end - p), &len)) < 0) {
			psTraceCrypto("ASN sequence parse error\n");
			return rc;
		}
		certEnd = p + len;
		certLen = certEnd - certStart;

/*
		Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
*/
		if ((rc = getExplicitVersion(&p, (uint32)(end - p), 0, &cert->version))
				< 0) {
			psTraceCrypto("ASN version parse error\n");
			return rc;
		}
		if (cert->version != 2) {
			psTraceIntCrypto("ERROR: non-v3 certificate version %d insecure\n",
				cert->version);
			return PS_PARSE_FAIL;
		}
/*
		CertificateSerialNumber  ::=  INTEGER
		There is a special return code for a missing serial number that
		will get written to the parse warning flag
*/
		if ((rc = getSerialNum(pool, &p, (uint32)(end - p), &cert->serialNumber,
				&cert->serialNumberLen)) < 0) {
			psTraceCrypto("ASN serial number parse error\n");
			return rc;
		}
/*
		AlgorithmIdentifier  ::=  SEQUENCE  {
		algorithm				OBJECT IDENTIFIER,
		parameters				ANY DEFINED BY algorithm OPTIONAL }
*/
		if ((rc = getAsnAlgorithmIdentifier(&p, (uint32)(end - p),
				&cert->certAlgorithm, &plen)) < 0) {
			psTraceCrypto("Couldn't parse algorithm identifier for certAlgorithm\n");
			return rc;
		}
		if (plen != 0) {
#ifdef USE_PKCS1_PSS
			if (cert->certAlgorithm == OID_RSASSA_PSS) {
				/* RSASSA-PSS-params ::= SEQUENCE {
					hashAlgorithm      [0] HashAlgorithm    DEFAULT sha1,
					maskGenAlgorithm   [1] MaskGenAlgorithm DEFAULT mgf1SHA1,
					saltLength         [2] INTEGER          DEFAULT 20,
					trailerField       [3] TrailerField     DEFAULT trailerFieldBC
					}
				*/
				if ((rc = getAsnSequence(&p, (uint32)(end - p), &len)) < 0) {
					psTraceCrypto("ASN sequence parse error\n");
					return rc;
				}
				/* Always set the defaults before parsing */
				cert->pssHash = PKCS1_SHA1_ID;
				cert->saltLen = SHA1_HASH_SIZE;
				/* Something other than defaults to parse here? */
				if (len > 0) {
					if ((rc = getRsaPssParams(&p, len, cert, 0)) < 0) {
						return rc;
					}
				}
			} else {
				psTraceCrypto("Unsupported X.509 certAlgorithm\n");
				return PS_UNSUPPORTED_FAIL;
			}
#else
			psTraceCrypto("Unsupported X.509 certAlgorithm\n");
			return PS_UNSUPPORTED_FAIL;
#endif
		}
/*
		Name ::= CHOICE {
		RDNSequence }

		RDNSequence ::= SEQUENCE OF RelativeDistinguishedName

		RelativeDistinguishedName ::= SET OF AttributeTypeAndValue

		AttributeTypeAndValue ::= SEQUENCE {
		type	AttributeType,
		value	AttributeValue }

		AttributeType ::= OBJECT IDENTIFIER

		AttributeValue ::= ANY DEFINED BY AttributeType
*/
		if ((rc = psX509GetDNAttributes(pool, &p, (uint32)(end - p),
				&cert->issuer, flags)) < 0) {
			psTraceCrypto("Couldn't parse issuer DN attributes\n");
			return rc;
		}
/*
		Validity ::= SEQUENCE {
		notBefore	Time,
		notAfter	Time	}
*/
		if ((rc = getTimeValidity(pool, &p, (uint32)(end - p),
				&cert->notBeforeTimeType, &cert->notAfterTimeType,
				&cert->notBefore, &cert->notAfter)) < 0) {
			psTraceCrypto("Couldn't parse validity\n");
			return rc;
		}

		/* SECURITY - platforms without a date function will always succeed */
		if ((rc = validateDateRange(cert)) < 0) {
			psTraceCrypto("Validity date check failed\n");
			return rc;
		}
/*
		Subject DN
*/
		if ((rc = psX509GetDNAttributes(pool, &p, (uint32)(end - p),
				&cert->subject,	flags)) < 0) {
			psTraceCrypto("Couldn't parse subject DN attributes\n");
			return rc;
		}
/*
		SubjectPublicKeyInfo  ::=  SEQUENCE  {
		algorithm			AlgorithmIdentifier,
		subjectPublicKey	BIT STRING	}
*/
		if ((rc = getAsnSequence(&p, (uint32)(end - p), &len)) < 0) {
			psTraceCrypto("Couldn't get ASN sequence for pubKeyAlgorithm\n");
			return rc;
		}
		if ((rc = getAsnAlgorithmIdentifier(&p, (uint32)(end - p),
				&cert->pubKeyAlgorithm, &plen)) < 0) {
			psTraceCrypto("Couldn't parse algorithm id for pubKeyAlgorithm\n");
			return rc;
		}

		/* Populate with correct type based on pubKeyAlgorithm OID */
		switch (cert->pubKeyAlgorithm) {
#ifdef USE_ECC
		case OID_ECDSA_KEY_ALG:
			if (plen == 0 || plen > (int32)(end - p)) {
				psTraceCrypto("Bad params on EC OID\n");
				return PS_PARSE_FAIL;
			}
			psInitPubKey(pool, &cert->publicKey, PS_ECC);
			if (getEcPubKey(pool, &p, (uint16_t)(end - p),
					&cert->publicKey.key.ecc, sha1KeyHash) < 0) {
				return PS_PARSE_FAIL;
			}
			/* keysize will be the size of the public ecc key (2 * privateLen) */
			cert->publicKey.keysize = psEccSize(&cert->publicKey.key.ecc);
			if (cert->publicKey.keysize < (MIN_ECC_BITS / 8)) {
				psTraceIntCrypto("ECC key size < %d\n", MIN_ECC_BITS);
				psClearPubKey(&cert->publicKey);
				return PS_PARSE_FAIL;
			}
			break;
#endif
#ifdef USE_RSA
		case OID_RSA_KEY_ALG:
			psAssert(plen == 0); /* No parameters on RSA pub key OID */
			psInitPubKey(pool, &cert->publicKey, PS_RSA);
			if ((rc = psRsaParseAsnPubKey(pool, &p, (uint16_t)(end - p),
					&cert->publicKey.key.rsa, sha1KeyHash)) < 0) {
				psTraceCrypto("Couldn't get RSA pub key from cert\n");
				return rc;
			}
			cert->publicKey.keysize = psRsaSize(&cert->publicKey.key.rsa);

			if (cert->publicKey.keysize < (MIN_RSA_BITS / 8)) {
				psTraceIntCrypto("RSA key size < %d\n", MIN_RSA_BITS);
				psClearPubKey(&cert->publicKey);
				return PS_PARSE_FAIL;
			}

			break;
#endif
		default:
			psTraceIntCrypto("Unsupported public key algorithm in cert parse: %d\n",
				cert->pubKeyAlgorithm);
			return PS_UNSUPPORTED_FAIL;
		}
		
#ifdef USE_OCSP
		/* A sha1 hash of the public key is useful for OCSP */
		memcpy(cert->sha1KeyHash, sha1KeyHash, SHA1_HASH_SIZE);
#endif

		/* As the next three values are optional, we can do a specific test here */
		if (*p != (ASN_SEQUENCE | ASN_CONSTRUCTED)) {
			if (getImplicitBitString(pool, &p, (uint32)(end - p),
						IMPLICIT_ISSUER_ID, &cert->uniqueIssuerId,
						&cert->uniqueIssuerIdLen) < 0 ||
					getImplicitBitString(pool, &p, (uint32)(end - p),
						IMPLICIT_SUBJECT_ID, &cert->uniqueSubjectId,
						&cert->uniqueSubjectIdLen) < 0 ||
					getExplicitExtensions(pool, &p, (uint32)(end - p),
						EXPLICIT_EXTENSION, &cert->extensions, 0) < 0) {
				psTraceCrypto("There was an error parsing a certificate\n");
				psTraceCrypto("extension.  This is likely caused by an\n");
				psTraceCrypto("extension format that is not currently\n");
				psTraceCrypto("recognized.  Please email Inside support\n");
				psTraceCrypto("to add support for the extension.\n\n");
				return PS_PARSE_FAIL;
			}
		}

		/* This is the end of the cert.  Do a check here to be certain */
		if (certEnd != p) {
			psTraceCrypto("Error.  Expecting end of cert\n");
			return PS_LIMIT_FAIL;
		}

		/* Reject any cert without a distinguishedName or subjectAltName */
		if (cert->subject.commonName == NULL &&
				cert->subject.country == NULL &&
				cert->subject.state == NULL &&
				cert->subject.locality == NULL &&
				cert->subject.organization == NULL &&
				cert->subject.orgUnit == NULL &&
				cert->extensions.san == NULL) {
			psTraceCrypto("Error. Cert has no name information\n");
			return PS_PARSE_FAIL;
		}
		/* Certificate signature info */
		if ((rc = getAsnAlgorithmIdentifier(&p, (uint32)(end - p),
				&cert->sigAlgorithm, &plen)) < 0) {
			psTraceCrypto("Couldn't get algorithm identifier for sigAlgorithm\n");
			return rc;
		}
		if (plen != 0) {
#ifdef USE_PKCS1_PSS
			if (cert->sigAlgorithm == OID_RSASSA_PSS) {
				/* RSASSA-PSS-params ::= SEQUENCE {
					hashAlgorithm      [0] HashAlgorithm    DEFAULT sha1,
					maskGenAlgorithm   [1] MaskGenAlgorithm DEFAULT mgf1SHA1,
					saltLength         [2] INTEGER          DEFAULT 20,
					trailerField       [3] TrailerField     DEFAULT trailerFieldBC
					}
				*/
				if ((rc = getAsnSequence(&p, (uint32)(end - p), &len)) < 0) {
					psTraceCrypto("ASN sequence parse error\n");
					return rc;
				}
				/* Something other than defaults to parse here? */
				if (len > 0) {
					if ((rc = getRsaPssParams(&p, len, cert, 1)) < 0) {
						return rc;
					}
				}
			} else {
				psTraceCrypto("Unsupported X.509 sigAlgorithm\n");
				return PS_UNSUPPORTED_FAIL;
			}
#else
			psTraceCrypto("Unsupported X.509 sigAlgorithm\n");
			return PS_UNSUPPORTED_FAIL;
#endif
		}
/*
		Signature algorithm must match that specified in TBS cert
*/
		if (cert->certAlgorithm != cert->sigAlgorithm) {
			psTraceCrypto("Parse error: mismatched signature type\n");
			return PS_CERT_AUTH_FAIL;
		}

/*
		Compute the hash of the cert here for CA validation
*/
		switch (cert->certAlgorithm) {
#ifdef ENABLE_MD5_SIGNED_CERTS
#ifdef USE_MD2
		case OID_MD2_RSA_SIG:
			psMd2Init(&hashCtx.md2);
			psMd2Update(&hashCtx.md2, certStart, certLen);
			psMd2Final(&hashCtx.md2, cert->sigHash);
			break;
#endif
		case OID_MD5_RSA_SIG:
			psMd5Init(&hashCtx.md5);
			psMd5Update(&hashCtx.md5, certStart, certLen);
			psMd5Final(&hashCtx.md5, cert->sigHash);
			break;
#endif
#ifdef ENABLE_SHA1_SIGNED_CERTS
		case OID_SHA1_RSA_SIG:
#ifdef USE_ECC
		case OID_SHA1_ECDSA_SIG:
#endif
			psSha1Init(&hashCtx.sha1);
			psSha1Update(&hashCtx.sha1, certStart, certLen);
			psSha1Final(&hashCtx.sha1, cert->sigHash);
			break;
#endif
#ifdef USE_SHA256
		case OID_SHA256_RSA_SIG:
#ifdef USE_ECC
		case OID_SHA256_ECDSA_SIG:
#endif
			psSha256Init(&hashCtx.sha256);
			psSha256Update(&hashCtx.sha256, certStart, certLen);
			psSha256Final(&hashCtx.sha256, cert->sigHash);
			break;
#endif
#ifdef USE_SHA384
		case OID_SHA384_RSA_SIG:
#ifdef USE_ECC
		case OID_SHA384_ECDSA_SIG:
#endif
			psSha384Init(&hashCtx.sha384);
			psSha384Update(&hashCtx.sha384, certStart, certLen);
			psSha384Final(&hashCtx.sha384, cert->sigHash);
			break;
#endif
#ifdef USE_SHA512
		case OID_SHA512_RSA_SIG:
#ifdef USE_ECC
		case OID_SHA512_ECDSA_SIG:
#endif
			psSha512Init(&hashCtx.sha512);
			psSha512Update(&hashCtx.sha512, certStart, certLen);
			psSha512Final(&hashCtx.sha512, cert->sigHash);
			break;
#endif
#ifdef USE_PKCS1_PSS
		case OID_RSASSA_PSS:
			switch (cert->pssHash) {
#ifdef ENABLE_MD5_SIGNED_CERTS
			case PKCS1_MD5_ID:
				psMd5Init(&hashCtx.md5);
				psMd5Update(&hashCtx.md5, certStart, certLen);
				psMd5Final(&hashCtx.md5, cert->sigHash);
				break;
#endif
#ifdef ENABLE_SHA1_SIGNED_CERTS
			case PKCS1_SHA1_ID:
				psSha1Init(&hashCtx.sha1);
				psSha1Update(&hashCtx.sha1, certStart, certLen);
				psSha1Final(&hashCtx.sha1, cert->sigHash);
				break;
#endif
#ifdef USE_SHA256
			case PKCS1_SHA256_ID:
				psSha256Init(&hashCtx.sha256);
				psSha256Update(&hashCtx.sha256, certStart, certLen);
				psSha256Final(&hashCtx.sha256, cert->sigHash);
				break;
#endif
#ifdef USE_SHA384
			case PKCS1_SHA384_ID:
				psSha384Init(&hashCtx.sha384);
				psSha384Update(&hashCtx.sha384, certStart, certLen);
				psSha384Final(&hashCtx.sha384, cert->sigHash);
				break;
#endif
#ifdef USE_SHA512
			case PKCS1_SHA512_ID:
				psSha512Init(&hashCtx.sha512);
				psSha512Update(&hashCtx.sha512, certStart, certLen);
				psSha512Final(&hashCtx.sha512, cert->sigHash);
				break;
#endif
			default:
				return PS_UNSUPPORTED_FAIL;

			} /* switch pssHash */
			break;
#endif /* USE_PKCS1_PSS */

		default:
			psTraceCrypto("Unsupported cert algorithm\n");
			return PS_UNSUPPORTED_FAIL;

		} /* switch certAlgorithm */

		/* 6 empty bytes is plenty enough to know if sigHash didn't calculate */
		if (memcmp(cert->sigHash, "\0\0\0\0\0\0", 6) == 0) {
			psTraceIntCrypto("No library signature alg support for cert: %d\n",
				cert->certAlgorithm);
			return PS_UNSUPPORTED_FAIL;
		}

		if ((rc = psX509GetSignature(pool, &p, (uint32)(end - p),
				&cert->signature, &cert->signatureLen)) < 0) {
			psTraceCrypto("Couldn't parse signature\n");
			return rc;
		}

#else /* !USE_CERT_PARSE */
		p = certStart + len + (int32)(p - certStart);
#endif /* USE_CERT_PARSE */
/*
		The ability to parse additional chained certs is a PKI product
		feature addition.  Chaining in MatrixSSL is handled internally.
*/
		if ((p != far_end) && (p < (far_end + 1))) {
			if (*p == 0x0 && *(p + 1) == 0x0) {
				parsing = 0; /* An indefinite length stream was passed in */
				/* caller will have to deal with skipping these becuase they
					would have read off the TL of this ASN.1 stream */
			} else {
				cert->next = psMalloc(pool, sizeof(psX509Cert_t));
				if (cert->next == NULL) {
					psError("Memory allocation error in psX509ParseCert\n");
					return PS_MEM_FAIL;
				}
				cert = cert->next;
				memset(cert, 0x0, sizeof(psX509Cert_t));
				cert->pool = pool;
			}
		} else {
			parsing = 0;
		}
	}

	return (int32)(p - pp);
}

#ifdef USE_CERT_PARSE
void x509FreeExtensions(x509v3extensions_t *extensions)
{

	x509GeneralName_t		*active, *inc;

	if (extensions->san) {
		active = extensions->san;
		while (active != NULL) {
			inc = active->next;
			psFree(active->data, extensions->pool);
			psFree(active, extensions->pool);
			active = inc;
		}
	}

#ifdef USE_CRL
	if (extensions->crlDist) {
		active = extensions->crlDist;
		while (active != NULL) {
			inc = active->next;
			psFree(active->data, extensions->pool);
			psFree(active, extensions->pool);
			active = inc;
		}
	}
#endif /* CRL */

#ifdef USE_FULL_CERT_PARSE
	if (extensions->nameConstraints.excluded) {
		active = extensions->nameConstraints.excluded;
		while (active != NULL) {
			inc = active->next;
			psFree(active->data, extensions->pool);
			psFree(active, extensions->pool);
			active = inc;
		}
	}
	if (extensions->nameConstraints.permitted) {
		active = extensions->nameConstraints.permitted;
		while (active != NULL) {
			inc = active->next;
			psFree(active->data, extensions->pool);
			psFree(active, extensions->pool);
			active = inc;
		}
	}
#endif /* USE_FULL_CERT_PARSE */
	if (extensions->sk.id)		psFree(extensions->sk.id, extensions->pool);
	if (extensions->ak.keyId)	psFree(extensions->ak.keyId, extensions->pool);
	if (extensions->ak.serialNum) psFree(extensions->ak.serialNum,
		extensions->pool);
	if (extensions->ak.attribs.commonName)
		psFree(extensions->ak.attribs.commonName, extensions->pool);
	if (extensions->ak.attribs.country) psFree(extensions->ak.attribs.country,
		extensions->pool);
	if (extensions->ak.attribs.state) psFree(extensions->ak.attribs.state,
		extensions->pool);
	if (extensions->ak.attribs.locality)
		psFree(extensions->ak.attribs.locality, extensions->pool);
	if (extensions->ak.attribs.organization)
		psFree(extensions->ak.attribs.organization, extensions->pool);
	if (extensions->ak.attribs.orgUnit) psFree(extensions->ak.attribs.orgUnit,
		extensions->pool);
	if (extensions->ak.attribs.dnenc) psFree(extensions->ak.attribs.dnenc,
		extensions->pool);
}
#endif /* USE_CERT_PARSE */

/******************************************************************************/
/*
	User must call after all calls to psX509ParseCert
	(we violate the coding standard a bit here for clarity)
*/
void psX509FreeCert(psX509Cert_t *cert)
{
	psX509Cert_t			*curr, *next;
	psPool_t				*pool;

	curr = cert;
	while (curr) {
		pool = curr->pool;
		if (curr->unparsedBin)			psFree(curr->unparsedBin, pool);
#ifdef USE_CERT_PARSE
		psX509FreeDNStruct(&curr->issuer, pool);
		psX509FreeDNStruct(&curr->subject, pool);
		if (curr->serialNumber)			psFree(curr->serialNumber, pool);
		if (curr->notBefore)			psFree(curr->notBefore, pool);
		if (curr->notAfter)				psFree(curr->notAfter, pool);
		if (curr->signature)			psFree(curr->signature, pool);
		if (curr->uniqueIssuerId)		psFree(curr->uniqueIssuerId, pool);
		if (curr->uniqueSubjectId)		psFree(curr->uniqueSubjectId, pool);


		if (curr->publicKey.type != PS_NONE) {
			switch (curr->pubKeyAlgorithm) {
#ifdef USE_RSA
			case OID_RSA_KEY_ALG:
				psRsaClearKey(&curr->publicKey.key.rsa);
				break;
#endif

#ifdef USE_ECC
			case OID_ECDSA_KEY_ALG:
				psEccClearKey(&curr->publicKey.key.ecc);
				break;
#endif
			default:
				psAssert(0);
				break;
			}
			curr->publicKey.type = PS_NONE;
		}

		x509FreeExtensions(&curr->extensions);
#ifdef USE_CRL
		x509FreeRevoked(&curr->revoked);
#endif
#endif /* USE_CERT_PARSE */
		next = curr->next;
		psFree(curr, pool);
		curr = next;
	}
}

#ifdef USE_CERT_PARSE
/******************************************************************************/
/*
	Currently just returning the raw BIT STRING and size in bytes
*/
#define MIN_HASH_SIZE	16
int32_t psX509GetSignature(psPool_t *pool, const unsigned char **pp, uint16_t len,
					unsigned char **sig, uint16_t *sigLen)
{
	const unsigned char		*p = *pp, *end;
	uint16_t				llen;

	end = p + len;
	if (len < 1 || (*(p++) != ASN_BIT_STRING) ||
		getAsnLength(&p, len - 1, &llen) < 0 ||
		(uint32)(end - p) < llen ||
		llen < (1 + MIN_HASH_SIZE)) {

		psTraceCrypto("Initial parse error in getSignature\n");
		return PS_PARSE_FAIL;
	}
	/* We assume this ignore_bits byte is always 0.  */
	psAssert(*p == 0);
	p++;
	/* Length was including the ignore_bits byte, subtract it */
	*sigLen = llen - 1;
	*sig = psMalloc(pool, *sigLen);
	if (*sig == NULL) {
		psError("Memory allocation error in getSignature\n");
		return PS_MEM_FAIL;
	}
	memcpy(*sig, p, *sigLen);
	*pp = p + *sigLen;
	return PS_SUCCESS;
}

/******************************************************************************/
/*
	Validate the expected name against a subset of the GeneralName rules
	for DNS, Email and IP types.
	We assume the expected name is not maliciously entered. If it is, it may
	match an invalid GeneralName in a remote cert chain.
	Returns 0 on valid format, PS_FAILURE on invalid format of GeneralName
*/
int32_t psX509ValidateGeneralName(const char *n)
{
	const char	*c;
	int			atfound;	/* Ampersand found */
	int			notip;		/* Not an ip address */

	if (n == NULL) return 0;

	/* Must be at least one character */
	if (*n == '\0') return PS_FAILURE;

	atfound = notip = 0;
	for (c = n; *c != '\0'; c++ ) {

		/* Negative tests first in the loop */
		/* Can't have any combination of . and - and @ together */
		if (c != n) {
			if (*c == '.' && *(c-1) == '.') return PS_FAILURE;
			if (*c == '.' && *(c-1) == '-') return PS_FAILURE;
			if (*c == '.' && *(c-1) == '@') return PS_FAILURE;
			if (*c == '-' && *(c-1) == '.') return PS_FAILURE;
			if (*c == '-' && *(c-1) == '-') return PS_FAILURE;
			if (*c == '-' && *(c-1) == '@') return PS_FAILURE;
			if (*c == '@' && *(c-1) == '.') return PS_FAILURE;
			if (*c == '@' && *(c-1) == '-') return PS_FAILURE;
			if (*c == '@' && *(c-1) == '@') return PS_FAILURE;
		}

		/* Note whether we have hit a non numeric name */
		if (*c != '.' && (*c < '0' || *c > '9')) notip++;

		/* Now positive tests */
		/* Cannot start or end with . or -, but can contain them */
		if (c != n && *(c + 1) != '\0' && (*c == '.' || *c == '-')) continue;
		/* Can contain at most one @ , and not at the start or end */
		if (*c == '@') {
			atfound++;
			if (c != n && *(c + 1) != '\0' && atfound == 1) {
				continue;
			}
		}
		/* Numbers allowed generally */
		if (*c >= '0' && *c <= '9') continue;
		/* Upper and lowercase characters allowed */
		if (*c >= 'A' && *c <= 'Z') continue;
		if (*c >= 'a' && *c <= 'z') continue;

		/* Everything else is a failure */
		return PS_FAILURE;
	}
	/* RFC 1034 states if it's not an IP, it can't start with a number,
		However, RFC 1123 updates this and does allow a number as the
		first character of a DNS name.
		See the X.509 RFC: http://tools.ietf.org/html/rfc5280#section-4.2.1.6 */
	if (atfound && (*n >= '0' && *n <= '9')) return PS_FAILURE;

	/* We could at this point store whether it is a DNS, Email or IP */

	return 0;
}

/******************************************************************************/
/*
	Parses a sequence of GeneralName types
	TODO: the actual types should be parsed.  Just copying data blob

	GeneralName ::= CHOICE {
		otherName						[0]		OtherName,
		rfc822Name						[1]		IA5String,
		dNSName							[2]		IA5String,
		x400Address						[3]		ORAddress,
		directoryName					[4]		Name,
		ediPartyName					[5]		EDIPartyName,
		uniformResourceIdentifier		[6]		IA5String,
		iPAddress						[7]		OCTET STRING,
		registeredID					[8]		OBJECT IDENTIFIER }
*/
static int32_t parseGeneralNames(psPool_t *pool, const unsigned char **buf,
				uint16_t len, const unsigned char *extEnd,
				x509GeneralName_t **name, int16_t limit)
{
	uint16_t				otherNameLen;
	const unsigned char		*p, *c, *save, *end;
	x509GeneralName_t		*activeName, *firstName, *prevName;

	if (*name == NULL) {
		firstName = NULL;
	} else {
		firstName = *name;
	}
	p = *buf;
	end = p + len;
	
	while (len > 0) {
		if (firstName == NULL) {
			activeName = firstName = psMalloc(pool,	sizeof(x509GeneralName_t));
			if (activeName == NULL) {
				return PS_MEM_FAIL;
			}
			memset(firstName, 0x0, sizeof(x509GeneralName_t));
			firstName->pool = pool;
			*name = firstName;
		} else {
/*
			Find the end
*/
			prevName = firstName;
			activeName = firstName->next;
			while (activeName != NULL) {
				prevName = activeName;
				activeName = activeName->next;
			}
			prevName->next = psMalloc(pool,	sizeof(x509GeneralName_t));
			if (prevName->next == NULL) {
				/* TODO: free the list */
				return PS_MEM_FAIL;
			}
			activeName = prevName->next;
			memset(activeName, 0x0, sizeof(x509GeneralName_t));
			activeName->pool = pool;
		}
		activeName->id = *p & 0xF;
		p++; len--;
		switch (activeName->id) {
			case GN_OTHER:
				strncpy((char *)activeName->name, "other",
					sizeof(activeName->name) - 1);
				/*  OtherName ::= SEQUENCE {
					type-id    OBJECT IDENTIFIER,
					value      [0] EXPLICIT ANY DEFINED BY type-id }
				*/
				save = p;
				if (getAsnLength(&p, (uint32)(extEnd - p), &otherNameLen) < 0 ||
						otherNameLen < 1 ||
						(uint32)(extEnd - p) < otherNameLen) {
					psTraceCrypto("ASN parse error SAN otherName\n");
					return PS_PARSE_FAIL;
				}
				if (*(p++) != ASN_OID ||
					getAsnLength(&p, (int32)(extEnd - p), &activeName->oidLen) < 0 ||
					(uint32)(extEnd - p) < activeName->oidLen) {

					psTraceCrypto("ASN parse error SAN otherName oid\n");
					return -1;
				}
				/* Note activeName->oidLen could be zero here */
				memcpy(activeName->oid, p, activeName->oidLen);
				p += activeName->oidLen;
				/* value looks like
					0xA0, <len>, <TYPE>, <dataLen>, <data>
					We're supporting only string-type TYPE so just skipping	it
				*/
				if ((uint32)(extEnd - p) < 1 || *p != 0xA0) {
					psTraceCrypto("ASN parse error SAN otherName\n");
					return PS_PARSE_FAIL;
				}
				p++; /* Jump over A0 */
				if (getAsnLength(&p, (uint32)(extEnd - p), &otherNameLen) < 0 ||
						otherNameLen < 1 ||
						(uint32)(extEnd - p) < otherNameLen) {
					psTraceCrypto("ASN parse error SAN otherName value\n");
					return PS_PARSE_FAIL;
				}
				if ((uint32)(extEnd - p) < 1) {
					psTraceCrypto("ASN parse error SAN otherName len\n");
					return PS_PARSE_FAIL;
				}
				/* TODO - validate *p == STRING type? */
				p++; /* Jump over TYPE */
				len -= (p - save);
				break;
			case GN_EMAIL:
				strncpy((char *)activeName->name, "email",
					sizeof(activeName->name) - 1);
				break;
			case GN_DNS:
				strncpy((char *)activeName->name, "DNS",
					sizeof(activeName->name) - 1);
				break;
			case GN_X400:
				strncpy((char *)activeName->name, "x400Address",
					sizeof(activeName->name) - 1);
				break;
			case GN_DIR:
				strncpy((char *)activeName->name, "directoryName",
					sizeof(activeName->name) - 1);
				break;
			case GN_EDI:
				strncpy((char *)activeName->name, "ediPartyName",
					sizeof(activeName->name) - 1);
				break;
			case GN_URI:
				strncpy((char *)activeName->name, "URI",
					sizeof(activeName->name) - 1);
				break;
			case GN_IP:
				strncpy((char *)activeName->name, "iPAddress",
					sizeof(activeName->name) - 1);
				break;
			case GN_REGID:
				strncpy((char *)activeName->name, "registeredID",
					sizeof(activeName->name) - 1);
				break;
			default:
				strncpy((char *)activeName->name, "unknown",
					sizeof(activeName->name) - 1);
				break;
		}

		save = p;
		if (getAsnLength(&p, (uint32)(extEnd - p), &activeName->dataLen) < 0 ||
				activeName->dataLen < 1 ||
				(uint32)(extEnd - p) < activeName->dataLen) {
			psTraceCrypto("ASN len error in parseGeneralNames\n");
			return PS_PARSE_FAIL;
		}
		len -= (p - save);

		/*	Currently we validate that the IA5String fields are printable
			At a minimum, this prevents attacks with null terminators or
			invisible characters in the certificate.
			Additional validation of name format is done indirectly
			via byte comparison to the expected name in ValidateGeneralName
			or directly by the user in the certificate callback */
		switch (activeName->id) {
			case GN_EMAIL:
			case GN_DNS:
			case GN_URI:
				save = p + activeName->dataLen;
				for (c = p; c < save; c++) {
					if (*c < ' ' || *c > '~') {
						psTraceCrypto("ASN invalid GeneralName character\n");
						return PS_PARSE_FAIL;
					}
				}
				break;
			case GN_IP:
				if (activeName->dataLen < 4) {
					psTraceCrypto("Unknown GN_IP format\n");
					return PS_PARSE_FAIL;
				}
				break;
			default:
				break;
		}

		activeName->data = psMalloc(pool, activeName->dataLen + 1);
		if (activeName->data == NULL) {
			psError("Memory allocation error: activeName->data\n");
			return PS_MEM_FAIL;
		}
		/* This guarantees data is null terminated, even for non IA5Strings */
		memset(activeName->data, 0x0, activeName->dataLen + 1);
		memcpy(activeName->data, p, activeName->dataLen);

		p = p + activeName->dataLen;
		len -= activeName->dataLen;
		
		if (limit > 0) {
			if (--limit == 0) {
				*buf = end;
				return PS_SUCCESS;
			}
		}
	}
	*buf = p;
	return PS_SUCCESS;
}

/******************************************************************************/
/**
	Parse ASN.1 DER encoded OBJECT bytes into an OID array.
	@param[in] der Pointer to start of OBJECT encoding.
	@param[in] derlen Number of bytes pointed to by 'der'.
	@param[out] oid Caller allocated array to receive OID, of
	at least MAX_OID_LEN elements.
	@return Number of OID elements written to 'oid', 0 on error.
*/
static uint8_t psParseOid(const unsigned char *der, uint16_t derlen,
				uint32_t oid[MAX_OID_LEN])
{
	const unsigned char *end;
	uint8_t				n, sanity;

	if (derlen < 1) {
		return 0;
	}
	end = der + derlen;
	/* First two OID elements are single octet, base 40 for some reason */
	oid[0] = *der / 40;
	oid[1] = *der % 40;
	der++;
	/* Zero the remainder of OID and leave n == 2 */
	for (n = MAX_OID_LEN - 1; n > 2; n--) {
		oid[n] = 0;
	}
	while (der < end && n < MAX_OID_LEN) {
		/* If the high bit is 0, it's short form variable length quantity */
		if (!(*der & 0x80)) {
			oid[n++] = *der++;
		} else {
			sanity = 0;
			/* Long form. High bit means another (lower) 7 bits following */
			do {
				oid[n] |= (*der & 0x7F);
				/* A clear high bit ends the byte sequence */
				if (!(*der & 0x80)) {
					break;
				}
				/* Allow a maximum of 4 x 7 bit shifts (28 bits) */
				if (++sanity > 4) {
					return 0;
				}
				/* Make room for the next 7 bits */
				oid[n] <<= 7;
				der++;
			} while (der < end);
			der++;
			n++;
		}
	}
	if (n < MAX_OID_LEN) {
		return n;
	}
	return 0;
}

/**
	Look up an OID in our known oid table.
	@param[in] oid Array of OID segments to look up in table.
	@param[in] oidlen Number of segments in 'oid'
	@return A valid OID enum on success, 0 on failure.
*/
static oid_e psFindOid(const uint32_t oid[MAX_OID_LEN], uint8_t oidlen)
{
	int		i, j;

	psAssert(oidlen <= MAX_OID_LEN);
	for (j = 0; oid_list[j].id != 0; j++) {
		for (i = 0; i < oidlen; i++) {
			if ((uint16_t)(oid[i] & 0xFFFF) != oid_list[j].oid[i]) {
				break;
			}
			if ((i + 1) == oidlen) {
				return oid_list[j].id;
			}
		}
	}
	return 0;
}

#ifdef USE_OID_TRACE
/**
	Print an OID in dot notation, with it's symbolic name, if found.
	@param[in] oid Array of OID segments print.
	@param[in] oidlen Number of segments in 'oid'
	@return void
*/
static void psTraceOid(uint32_t oid[MAX_OID_LEN], uint8_t oidlen)
{
	int		i, j, found;

	for (i = 0; i < oidlen; i++) {
		if ((i + 1) < oidlen) {
			psTraceIntCrypto("%u.", oid[i]);
		} else {
			psTraceIntCrypto("%u", oid[i]);
		}
	}
	found = 0;
	for (j = 0; oid_list[j].oid[0] != 0 && !found; j++) {
		for (i = 0; i < oidlen; i++) {
			if ((uint8_t)(oid[i] & 0xFF) != oid_list[j].oid[i]) {
				break;
			}
			if ((i + 1) == oidlen) {
				psTraceStrCrypto(" (%s)", oid_list[j].name);
				found++;
			}
		}
	}
	psTraceCrypto("\n");
}
#else
#define psTraceOid(A, B)	psTraceCrypto("\n");
#endif

/******************************************************************************/
/*
	X509v3 extensions
*/

int32_t getExplicitExtensions(psPool_t *pool, const unsigned char **pp,
								 uint16_t inlen, int32_t expVal,
								 x509v3extensions_t *extensions, uint8_t known)
{
	const unsigned char	*p = *pp, *end;
	const unsigned char	*extEnd, *extStart, *save;
	unsigned char		critical;
	uint16_t			len, fullExtLen;
	uint32_t			oid[MAX_OID_LEN];
	uint8_t				oidlen;
	oid_e				noid;
#ifdef USE_FULL_CERT_PARSE
	uint16_t		subExtLen;
	const unsigned char	*subSave;
	int32_t				nc = 0;
#endif

	end = p + inlen;
	if (inlen < 1) {
		return PS_ARG_FAIL;
	}
	extensions->pool = pool;
	if (known) {
		goto KNOWN_EXT;
	}
/*
	Not treating this as an error because it is optional.
*/
	if (*p != (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | expVal)) {
		return 0;
	}
	p++;
	if (getAsnLength(&p, (uint32)(end - p), &len) < 0 ||
			(uint32)(end - p) < len) {
		psTraceCrypto("Initial getAsnLength failure in extension parse\n");
		return PS_PARSE_FAIL;
	}
KNOWN_EXT:
/*
	Extensions  ::=  SEQUENCE SIZE (1..MAX) OF Extension

	Extension  ::=  SEQUENCE {
		extnID		OBJECT IDENTIFIER,
		extnValue	OCTET STRING	}
*/
	if (getAsnSequence(&p, (uint32)(end - p), &len) < 0 ||
			(uint32)(end - p) < len) {
		psTraceCrypto("Initial getAsnSequence failure in extension parse\n");
		return PS_PARSE_FAIL;
	}
	extEnd = p + len;
	while ((p != extEnd) && *p == (ASN_SEQUENCE | ASN_CONSTRUCTED)) {
		if (getAsnSequence(&p, (uint32)(extEnd - p), &fullExtLen) < 0) {
			psTraceCrypto("getAsnSequence failure in extension parse\n");
			return PS_PARSE_FAIL;
		}
		extStart = p;
/*
		Conforming CAs MUST support key identifiers, basic constraints,
		key usage, and certificate policies extensions
*/
		if (extEnd - p < 1 || *p++ != ASN_OID) {
			psTraceCrypto("Malformed extension header\n");
			return PS_PARSE_FAIL;
		}
		if (getAsnLength(&p, (uint32)(extEnd - p), &len) < 0 ||
				(uint32)(extEnd - p) < len) {
			psTraceCrypto("Malformed extension length\n");
			return PS_PARSE_FAIL;
		}
		if ((oidlen = psParseOid(p, len, oid)) < 1) {
			psTraceCrypto("Malformed extension OID\n");
			return PS_PARSE_FAIL;
		}
		noid = psFindOid(oid, oidlen);
		p += len;
/*
		Possible boolean value here for 'critical' id.  It's a failure if a
		critical extension is found that is not supported
*/
		critical = 0;
		if (extEnd - p < 1) {
			psTraceCrypto("Malformed extension length\n");
			return PS_PARSE_FAIL;
		}
		if (*p == ASN_BOOLEAN) {
			p++;
			if (extEnd - p < 2) {
				psTraceCrypto("Error parsing critical id len for cert extension\n");
				return PS_PARSE_FAIL;
			}
			if (*p != 1) {
				psTraceCrypto("Error parsing critical id for cert extension\n");
				return PS_PARSE_FAIL;
			}
			p++;
			if (*p > 0) {
				/* Officially DER TRUE must be 0xFF, openssl is more lax */
				if (*p != 0xFF) {
					psTraceCrypto("Warning: DER BOOLEAN TRUE should be 0xFF\n");
				}
				critical = 1;
			}
			p++;
		}
		if (extEnd - p < 1 || (*p++ != ASN_OCTET_STRING) ||
				getAsnLength(&p, (uint32)(extEnd - p), &len) < 0 ||
				(uint32)(extEnd - p) < len) {
			psTraceCrypto("Expecting OCTET STRING in ext parse\n");
			return PS_PARSE_FAIL;
		}

		/* Set bits 1..9 to indicate criticality of known extensions */
		if (critical) {
			extensions->critFlags |= EXT_CRIT_FLAG(noid);
		}

		switch (noid) {
/*
			 BasicConstraints ::= SEQUENCE {
				cA						BOOLEAN DEFAULT FALSE,
				pathLenConstraint		INTEGER (0..MAX) OPTIONAL }
*/
			case OID_ENUM(id_ce_basicConstraints):
				if (getAsnSequence(&p, (uint32)(extEnd - p), &len) < 0) {
					psTraceCrypto("Error parsing BasicConstraints extension\n");
					return PS_PARSE_FAIL;
				}
/*
				"This goes against PKIX guidelines but some CAs do it and some
				software requires this to avoid interpreting an end user
				certificate as a CA."
					- OpenSSL certificate configuration doc

				basicConstraints=CA:FALSE
*/
				if (len == 0) {
					break;
				}
/*
				Have seen some certs that don't include a cA bool.
*/
				if (*p == ASN_BOOLEAN) {
					if (extEnd - p < 3) {
						psTraceCrypto("Error parsing BC extension\n");
						return PS_PARSE_FAIL;
					}
					p++;
					if (*p++ != 1) {
						psTraceCrypto("Error parse BasicConstraints CA bool\n");
						return PS_PARSE_FAIL;
					}
					/* Officially DER TRUE must be 0xFF, openssl is more lax */
					if (*p > 0 && *p != 0xFF) {
						psTraceCrypto("Warning: cA TRUE should be 0xFF\n");
					}
					extensions->bc.cA = *p++;
				} else {
					extensions->bc.cA = 0;
				}
/*
				Now need to check if there is a path constraint. Only makes
				sense if cA is true.  If it's missing, there is no limit to
				the cert path
*/
				if (*p == ASN_INTEGER) {
					if (getAsnInteger(&p, (uint32)(extEnd - p),
							&(extensions->bc.pathLenConstraint)) < 0) {
						psTraceCrypto("Error parsing BasicConstraints pathLen\n");
						return PS_PARSE_FAIL;
					}
				} else {
					extensions->bc.pathLenConstraint = -1;
				}
				break;

			case OID_ENUM(id_ce_subjectAltName):
				if (getAsnSequence(&p, (uint32)(extEnd - p), &len) < 0) {
					psTraceCrypto("Error parsing altSubjectName extension\n");
					return PS_PARSE_FAIL;
				}
				/* NOTE: The final limit parameter was introduced for this
					case because a well known search engine site sends back
					about 7 KB worth of subject alt names and that has created
					memory problems for a couple users.  Set the -1 here to
					something reasonable (5) if you've found yourself here 
					for this memory reason */
				if (parseGeneralNames(pool, &p, len, extEnd, &extensions->san,
						-1) < 0) {
					psTraceCrypto("Error parsing altSubjectName names\n");
					return PS_PARSE_FAIL;
				}

				break;

			case OID_ENUM(id_ce_keyUsage):
/*
				KeyUsage ::= BIT STRING {
					digitalSignature		(0),
					nonRepudiation			(1),
					keyEncipherment			(2),
					dataEncipherment		(3),
					keyAgreement			(4),
					keyCertSign				(5),
					cRLSign					(6),
					encipherOnly			(7),
					decipherOnly			(8) }
*/
				if (*p++ != ASN_BIT_STRING) {
					psTraceCrypto("Error parsing keyUsage extension\n");
					return PS_PARSE_FAIL;
				}
				if (getAsnLength(&p, (int32)(extEnd - p), &len) < 0 || 
						(uint32)(extEnd - p) < len) {
					psTraceCrypto("Malformed keyUsage extension\n");
					return PS_PARSE_FAIL;
				}
				if (len < 2) {
					psTraceCrypto("Malformed keyUsage extension\n");
					return PS_PARSE_FAIL;
				}
/*
				If the lenth is <= 3, then there might be a
				KEY_USAGE_DECIPHER_ONLY (or maybe just some empty bytes).
*/
				if (len >= 3) {
					if (p[2] == (KEY_USAGE_DECIPHER_ONLY >> 8) && p[0] == 7) {
						extensions->keyUsageFlags |= KEY_USAGE_DECIPHER_ONLY;
					}
				}
				extensions->keyUsageFlags |= p[1];
				p = p + len;
				break;

			case OID_ENUM(id_ce_extKeyUsage):
				if (getAsnSequence(&p, (int32)(extEnd - p), &fullExtLen) < 0) {
					psTraceCrypto("Error parsing authKeyId extension\n");
					return PS_PARSE_FAIL;
				}
				save = p;
				while (fullExtLen > 0) {
					if (*p++ != ASN_OID) {
						psTraceCrypto("Malformed extension header\n");
						return PS_PARSE_FAIL;
					}
					if (getAsnLength(&p, fullExtLen, &len) < 0 ||
							fullExtLen < len) {
						psTraceCrypto("Malformed extension length\n");
						return PS_PARSE_FAIL;
					}
					if ((oidlen = psParseOid(p, len, oid)) < 1) {
						psTraceCrypto("Malformed extension OID\n");
						return PS_PARSE_FAIL;
					}
					noid = psFindOid(oid, oidlen);
					p += len;
					if (fullExtLen < (uint32)(p - save)) {
						psTraceCrypto("Inner OID parse fail EXTND_KEY_USAGE\n");
						return PS_PARSE_FAIL;
					}
					fullExtLen -= (p - save);
					save = p;
					switch (noid) {
					case OID_ENUM(id_kp_serverAuth):
						extensions->ekuFlags |= EXT_KEY_USAGE_TLS_SERVER_AUTH;
						break;
					case OID_ENUM(id_kp_clientAuth):
						extensions->ekuFlags |= EXT_KEY_USAGE_TLS_CLIENT_AUTH;
						break;
					case OID_ENUM(id_kp_codeSigning):
						extensions->ekuFlags |= EXT_KEY_USAGE_CODE_SIGNING;
						break;
					case OID_ENUM(id_kp_emailProtection):
						extensions->ekuFlags |= EXT_KEY_USAGE_EMAIL_PROTECTION;
						break;
					case OID_ENUM(id_kp_timeStamping):
						extensions->ekuFlags |= EXT_KEY_USAGE_TIME_STAMPING;
						break;
					case OID_ENUM(id_kp_OCSPSigning):
						extensions->ekuFlags |= EXT_KEY_USAGE_OCSP_SIGNING;
						break;
					case OID_ENUM(id_ce_eku_anyExtendedKeyUsage):
						extensions->ekuFlags |= EXT_KEY_USAGE_ANY;
						break;
					default:
						psTraceCrypto("WARNING: Unknown EXT_KEY_USAGE:");
						psTraceOid(oid, oidlen);
						break;
					} /* end switch */
				}
				break;

#ifdef USE_FULL_CERT_PARSE

			case OID_ENUM(id_ce_nameConstraints):
				if (critical) {
					/* We're going to fail if critical since no real
						pattern matching is happening yet */
					psTraceCrypto("ERROR: critical nameConstraints unsupported\n");
					return PS_PARSE_FAIL;
				}
				if (getAsnSequence(&p, (int32)(extEnd - p), &fullExtLen) < 0) {
					psTraceCrypto("Error parsing authKeyId extension\n");
					return PS_PARSE_FAIL;
				}
				while (fullExtLen > 0) {
					save = p;

					if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 0)) {
						/* permittedSubtrees */
						p++;
						nc = 0;
					}
					if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 1)) {
						/* excludedSubtrees */
						p++;
						nc = 1;
					}
					subExtLen = 0;
					if (getAsnLength(&p, (uint32)(extEnd - p), &subExtLen) < 0 ||
							subExtLen < 1 || (uint32)(extEnd - p) < subExtLen) {
						psTraceCrypto("ASN get len error in nameConstraint\n");
						return PS_PARSE_FAIL;
					}
					if (fullExtLen < (subExtLen + (p - save))) {
						psTraceCrypto("fullExtLen parse fail nameConstraint\n");
						return PS_PARSE_FAIL;
					}
					fullExtLen -= subExtLen + (p - save);
					while (subExtLen > 0) {
						subSave = p;
						if (getAsnSequence(&p, (int32)(extEnd - p), &len) < 0) {
							psTraceCrypto("Error parsing nameConst ext\n");
							return PS_PARSE_FAIL;
						}
						if (subExtLen < (len + (p - subSave))) {
							psTraceCrypto("subExtLen fail nameConstraint\n");
							return PS_PARSE_FAIL;
						}
						subExtLen -= len + (p - subSave);
						if (nc == 0) {
							if (parseGeneralNames(pool, &p, len, extEnd,
								&extensions->nameConstraints.permitted, -1) <0){
							 psTraceCrypto("Error parsing nameConstraint\n");
							 return PS_PARSE_FAIL;
							}
						} else {
							if (parseGeneralNames(pool, &p, len, extEnd,
								&extensions->nameConstraints.excluded, -1) < 0){
							 psTraceCrypto("Error parsing nameConstraint\n");
							 return PS_PARSE_FAIL;
							}
						}
					}
				}
				break;

#ifdef USE_CRL
			case OID_ENUM(id_ce_cRLDistributionPoints):

				if (getAsnSequence(&p, (int32)(extEnd - p), &fullExtLen) < 0) {
					psTraceCrypto("Error parsing authKeyId extension\n");
					return PS_PARSE_FAIL;
				}

				while (fullExtLen > 0) {
					save = p;
					if (getAsnSequence(&p, (uint32)(extEnd - p), &len) < 0) {
						psTraceCrypto("getAsnSequence fail in crldist parse\n");
						return PS_PARSE_FAIL;
					}
					if (fullExtLen < (len + (p - save))) {
						psTraceCrypto("fullExtLen parse fail crldist\n");
						return PS_PARSE_FAIL;
					}
					fullExtLen -= len + (p - save);
					/* All memebers are optional */
					if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 0)) {
						/* DistributionPointName */
						p++;
						if (getAsnLength(&p, (uint32)(extEnd - p), &len) < 0 ||
								len < 1 || (uint32)(extEnd - p) < len) {
							psTraceCrypto("ASN get len error in CRL extension\n");
							return PS_PARSE_FAIL;
						}

						if ((*p & 0xF) == 0) { /* fullName (GeneralNames) */
							p++;
							if (getAsnLength(&p, (uint32)(extEnd - p), &len) < 0
									|| len < 1 || (uint32)(extEnd - p) < len) {
								psTraceCrypto("ASN get len error in CRL extension\n");
								return PS_PARSE_FAIL;
							}
							if (parseGeneralNames(pool, &p, len, extEnd,
									&extensions->crlDist, -1) > 0) {
								psTraceCrypto("dist gen name parse fail\n");
								return PS_PARSE_FAIL;
							}
						} else if ((*p & 0xF) == 1) { /* RelativeDistName */
							p++;
							/* RelativeDistName not parsed */
							if (getAsnLength(&p, (uint32)(extEnd - p), &len) < 0
									|| len < 1 || (uint32)(extEnd - p) < len) {
								psTraceCrypto("ASN get len error in CRL extension\n");
								return PS_PARSE_FAIL;
							}
							p += len;
						} else {
							psTraceCrypto("DistributionPointName parse fail\n");
							return PS_PARSE_FAIL;
						}
					}
					if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 1)) {
						p++;
						/* ReasonFlags not parsed */
						if (getAsnLength(&p, (uint32)(extEnd - p), &len) < 0 ||
								len < 1 || (uint32)(extEnd - p) < len) {
							psTraceCrypto("ASN get len error in CRL extension\n");
							return PS_PARSE_FAIL;
						}
						p += len;
					}
					if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 2)) {
						p++;
						/* General Names not parsed */
						if (getAsnLength(&p, (uint32)(extEnd - p), &len) < 0 ||
								len < 1 || (uint32)(extEnd - p) < len) {
							psTraceCrypto("ASN get len error in CRL extension\n");
							return PS_PARSE_FAIL;
						}
						p += len;
					}
				}
				break;
#endif /* USE_CRL */
#endif /* FULL_CERT_PARSE */

			case OID_ENUM(id_ce_authorityKeyIdentifier):
/*
				AuthorityKeyIdentifier ::= SEQUENCE {
				keyIdentifier			[0] KeyIdentifier			OPTIONAL,
				authorityCertIssuer		[1] GeneralNames			OPTIONAL,
				authorityCertSerialNumber [2] CertificateSerialNumber OPTIONAL }

				KeyIdentifier ::= OCTET STRING
*/
				if (getAsnSequence(&p, (int32)(extEnd - p), &len) < 0) {
					psTraceCrypto("Error parsing authKeyId extension\n");
					return PS_PARSE_FAIL;
				}
				/* Have seen a cert that has a zero length ext here. Let it pass. */
				if (len == 0) {
					break;
				}
				/* All members are optional */
				if (*p == (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 0)) {
					p++;
					if (getAsnLength(&p, (int32)(extEnd - p),
							&extensions->ak.keyLen) < 0 ||
							(uint32)(extEnd - p) < extensions->ak.keyLen) {
						psTraceCrypto("Error keyLen in authKeyId extension\n");
						return PS_PARSE_FAIL;
					}
					extensions->ak.keyId =psMalloc(pool, extensions->ak.keyLen);
					if (extensions->ak.keyId == NULL) {
						psError("Mem allocation err: extensions->ak.keyId\n");
						return PS_MEM_FAIL;
					}
					memcpy(extensions->ak.keyId, p, extensions->ak.keyLen);
					p = p + extensions->ak.keyLen;
				}
				if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 1)) {
					p++;
					if (getAsnLength(&p, (int32)(extEnd - p), &len) < 0 ||
							len < 1 || (uint32)(extEnd - p) < len) {
						psTraceCrypto("ASN get len error in authKeyId extension\n");
						return PS_PARSE_FAIL;
					}
					if ((*p ^ ASN_CONTEXT_SPECIFIC ^ ASN_CONSTRUCTED) != 4) {
						/* We are just dealing with DN formats here */
						psTraceIntCrypto("Error auth key-id name type: %d\n",
							*p ^ ASN_CONTEXT_SPECIFIC ^ ASN_CONSTRUCTED);
						return PS_PARSE_FAIL;
					}
					p++;
					if (getAsnLength(&p, (int32)(extEnd - p), &len) < 0 ||
							(uint32)(extEnd - p) < len) {
						psTraceCrypto("ASN get len error2 in authKeyId extension\n");
						return PS_PARSE_FAIL;
					}
					if (psX509GetDNAttributes(pool, &p, (int32)(extEnd - p),
							&(extensions->ak.attribs), 0) < 0) {
						psTraceCrypto("Error parsing ak.attribs\n");
						return PS_PARSE_FAIL;
					}
				}
				if ((*p == (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 2)) ||
						(*p == ASN_INTEGER)){
/*
					Treat as a serial number (not a native INTEGER)
*/
					if (getSerialNum(pool, &p, (int32)(extEnd - p),
							&(extensions->ak.serialNum), &len) < 0) {
						psTraceCrypto("Error parsing ak.serialNum\n");
						return PS_PARSE_FAIL;
					}
					extensions->ak.serialNumLen = len;
				}
				break;

			case OID_ENUM(id_ce_subjectKeyIdentifier):
/*
				The value of the subject key identifier MUST be the value
				placed in the key identifier field of the Auth Key Identifier
				extension of certificates issued by the subject of
				this certificate.
*/
				if (*p++ != ASN_OCTET_STRING || getAsnLength(&p,
						(int32)(extEnd - p), &(extensions->sk.len)) < 0 ||
						(uint32)(extEnd - p) < extensions->sk.len) {
					psTraceCrypto("Error parsing subjectKeyId extension\n");
					return PS_PARSE_FAIL;
				}
				extensions->sk.id = psMalloc(pool, extensions->sk.len);
				if (extensions->sk.id == NULL) {
					psError("Memory allocation error extensions->sk.id\n");
					return PS_MEM_FAIL;
				}
				memcpy(extensions->sk.id, p, extensions->sk.len);
				p = p + extensions->sk.len;
				break;

			/* These extensions are known but not handled */
			case OID_ENUM(id_ce_certificatePolicies):
			case OID_ENUM(id_ce_policyMappings):
			case OID_ENUM(id_ce_issuerAltName):
			case OID_ENUM(id_ce_subjectDirectoryAttributes):
			case OID_ENUM(id_ce_policyConstraints):
			case OID_ENUM(id_ce_inhibitAnyPolicy):
			case OID_ENUM(id_ce_freshestCRL):
			case OID_ENUM(id_pe_subjectInfoAccess):
			default:
				/* Unsupported or skipping because USE_FULL_CERT_PARSE undefd */
				if (critical) {
					psTraceCrypto("Unsupported critical ext encountered: ");
					psTraceOid(oid, oidlen);
#ifndef ALLOW_UNKNOWN_CRITICAL_EXTENSIONS
					_psTrace("An unsupported critical extension was "
						"encountered.  X.509 specifications say "
						"connections must be terminated in this case. "
						"Define ALLOW_UNKNOWN_CRITICAL_EXTENSIONS to "
						"bypass this rule if testing and email Inside "
						"support to inquire about this extension.\n\n");
					return PS_PARSE_FAIL;
#else
#ifdef WIN32
#pragma message("IGNORING UNKNOWN CRITICAL EXTENSIONS IS A SECURITY RISK")
#else
#warning "IGNORING UNKNOWN CRITICAL EXTENSIONS IS A SECURITY RISK"
#endif
#endif
				}
				p++;
/*
				Skip over based on the length reported from the ASN_SEQUENCE
				surrounding the entire extension.  It is not a guarantee that
				the value of the extension itself will contain it's own length.
*/
				p = p + (fullExtLen - (p - extStart));
				break;
		}
	}
	*pp = p;
	return 0;
}

/******************************************************************************/
/*
	Although a certificate serial number is encoded as an integer type, that
	doesn't prevent it from being abused as containing a variable length
	binary value.  Get it here.
*/
int32_t getSerialNum(psPool_t *pool, const unsigned char **pp, uint16_t len,
						unsigned char **sn, uint16_t *snLen)
{
	const unsigned char		*p = *pp;
	uint16_t			vlen;

	if ((*p != (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 2)) &&
			(*p != ASN_INTEGER)) {
		psTraceCrypto("X.509 getSerialNum failed on first bytes\n");
		return PS_PARSE_FAIL;
	}
	p++;

	if (len < 1 || getAsnLength(&p, len - 1, &vlen) < 0 || (len - 1) < vlen) {
		psTraceCrypto("ASN getSerialNum failed\n");
		return PS_PARSE_FAIL;
	}
	*snLen = vlen;

	if (vlen > 0) {
		*sn = psMalloc(pool, vlen);
		if (*sn == NULL) {
			psError("Memory allocation failure in getSerialNum\n");
			return PS_MEM_FAIL;
		}
		memcpy(*sn, p, vlen);
		p += vlen;
	}
	*pp = p;
	return PS_SUCCESS;
}

/******************************************************************************/
/**
	Explicit value encoding has an additional tag layer.
 */
static int32_t getExplicitVersion(const unsigned char **pp, uint16_t len,
				int32_t expVal, int32_t *val)
{
	const unsigned char		*p = *pp;
	uint16_t				exLen;

	if (len < 1) {
		psTraceCrypto("Invalid length to getExplicitVersion\n");
		return PS_PARSE_FAIL;
	}
/*
	This is an optional value, so don't error if not present.  The default
	value is version 1
*/
	if (*p != (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | expVal)) {
		*val = 0;
		return PS_SUCCESS;
	}
	p++;
	if (getAsnLength(&p, len - 1, &exLen) < 0 || (len - 1) < exLen) {
		psTraceCrypto("getAsnLength failure in getExplicitVersion\n");
		return PS_PARSE_FAIL;
	}
	if (getAsnInteger(&p, exLen, val) < 0) {
		psTraceCrypto("getAsnInteger failure in getExplicitVersion\n");
		return PS_PARSE_FAIL;
	}
	*pp = p;
	return PS_SUCCESS;
}

/******************************************************************************/
/**
	Verify a string has nearly valid date range format and length.
 */
static unsigned char asciidate(const unsigned char *c, unsigned int utctime)
{
	if (utctime != ASN_UTCTIME) {	/* 4 character year */
		if (*c < '1' && *c > '2') return 0; c++; /* Year 1900 - 2999 */
		if (*c < '0' && *c > '9') return 0; c++;
	}
	if (*c < '0' && *c > '9') return 0; c++;
	if (*c < '0' && *c > '9') return 0; c++;
	if (*c < '0' && *c > '1') return 0; c++; /* Month 00 - 19 */
	if (*c < '0' && *c > '9') return 0; c++;
	if (*c < '0' && *c > '3') return 0; c++; /* Day 00 - 39 */
	if (*c < '0' && *c > '9') return 0;
	return 1;
}

/******************************************************************************/
/**
	Tests if the certificate was issued before the given date.
	Because there is no actual issuance date in the certificate, we use the
	'notBefore' date (the initial date the certificate is valid) as the
	effective issuance date.
	@security This api is used to be more lenient on certificates that are still
	valid, but were created before certain more strict certificate rules
	were specified.

	@param[in] rfc The RFC to check against.
	@param[in] cert The cert to check the issuing date on.
	@return 1 if yes, 0 if no, -1 on parse error.
*/
static int32 issuedBefore(rfc_e rfc, const psX509Cert_t *cert)
{
	unsigned char	*c;
	unsigned int	y;
	unsigned short	m;

	/* Validate the 'not before' date */
	if ((c = (unsigned char *)cert->notBefore) == NULL) {
		return PS_FAILURE;
	}
	/* UTCTIME, defined in 1982, has just a 2 digit year */
	/* year as unsigned int handles over/underflows */
	if (cert->notBeforeTimeType == ASN_UTCTIME) {
		if (!asciidate(c, ASN_UTCTIME))  {
			return PS_FAILURE;
		}
		y =  2000 + 10 * (c[0] - '0') + (c[1] - '0'); c += 2;
		/* Years from '96 through '99 are in the 1900's */
		if (y >= 2096) {
			y -= 100;
		}
	} else {
		if (!asciidate(c, 0))  {
			return PS_FAILURE;
		}
		y = 1000 * (c[0] - '0') + 100 * (c[1] - '0') +
			10 * (c[2] - '0') + (c[3] - '0'); c += 4;
	}
	/* month as unsigned short handles over/underflows */
	m = 10 * (c[0] - '0') + (c[1] - '0');
	/* Must have been issued at least when X509v3 was added */
	if (y < 1996 || m < 1 || m > 12) {
		return -1;
	}
	switch (rfc) {
	case RFC_6818:
		if (y < 2013) { /* No month check needed for Jan */
			return 1;
		}
		return 0;
	case RFC_5280:
		if (y < 2008 || (y == 2008 && m < 5)) {
			return 1;
		}
		return 0;
	case RFC_3280:
		if (y < 2002 || (y == 2002 && m < 4)) {
			return 1;
		}
		return 0;
	case RFC_2459:
		if (y < 1999) { /* No month check needed for Jan */
			return 1;
		}
		return 0;
	default:
		return -1;
	}
	return -1;
}

/******************************************************************************/
/**
	Validate the dates in the cert to machine date.
	SECURITY - always succeeds on systems without date support
	Returns
		0 on success
		PS_CERT_AUTH_FAIL_DATE if date is out of range
		PS_FAILURE on parse error
*/
static int32 validateDateRange(psX509Cert_t *cert)
{
#ifdef POSIX
	struct tm		t;
	time_t			rawtime;
	unsigned char	*c;
	unsigned int	y;
	unsigned short	m, d;

	time(&rawtime);
	localtime_r(&rawtime, &t);
	/* Localtime does months from 0-11 and (year-1900)! Normalize it. */
	t.tm_mon++;
	t.tm_year += 1900;

	/* Validate the 'not before' date */
	if ((c = (unsigned char *)cert->notBefore) == NULL) {
		return PS_FAILURE;
	}
	/* UTCTIME, defined in 1982, has just a 2 digit year */
	/* year as unsigned int handles over/underflows */
	if (cert->notBeforeTimeType == ASN_UTCTIME) {
		if (!asciidate(c, ASN_UTCTIME))  {
			return PS_FAILURE;
		}
		y =  2000 + 10 * (c[0] - '0') + (c[1] - '0'); c += 2;
		/* Years from '96 through '99 are in the 1900's */
		if (y >= 2096) {
			y -= 100;
		}
	} else {
		if (!asciidate(c, 0))  {
			return PS_FAILURE;
		}
		y = 1000 * (c[0] - '0') + 100 * (c[1] - '0') +
			10 * (c[2] - '0') + (c[3] - '0'); c += 4;
	}
	/* month,day as unsigned short handles over/underflows */
	m = 10 * (c[0] - '0') + (c[1] - '0'); c += 2;
	d = 10 * (c[0] - '0') + (c[1] - '0');
	/* Must have been issued at least when X509v3 was added */
	if (y < 1996 || m < 1 || m > 12 || d < 1 || d > 31) {
		return PS_FAILURE;
	}
	if (t.tm_year < (int)y) {
		cert->authFailFlags |= PS_CERT_AUTH_FAIL_DATE_FLAG;
	} else if (t.tm_year == (int)y) {
		if (t.tm_mon < m) {
			cert->authFailFlags |= PS_CERT_AUTH_FAIL_DATE_FLAG;
		} else if (t.tm_mon == m && t.tm_mday < d) {
			cert->authFailFlags |= PS_CERT_AUTH_FAIL_DATE_FLAG;
		}
	}

	/* Validate the 'not after' date */
	if ((c = (unsigned char *)cert->notAfter) == NULL) {
		return PS_FAILURE;
	}
	/* UTCTIME, defined in 1982, has just a 2 digit year */
	/* year as unsigned int handles over/underflows */
	if (cert->notAfterTimeType == ASN_UTCTIME) {
		if (!asciidate(c, ASN_UTCTIME))  {
			return PS_FAILURE;
		}
		y =  2000 + 10 * (c[0] - '0') + (c[1] - '0'); c += 2;
		/* Years from '96 through '99 are in the 1900's */
		if (y >= 2096) {
			y -= 100;
		}
	} else {
		if (!asciidate(c, 0))  {
			return PS_FAILURE;
		}
		y = 1000 * (c[0] - '0') + 100 * (c[1] - '0') +
			10 * (c[2] - '0') + (c[3] - '0'); c += 4;
	}
	/* month,day as unsigned short handles over/underflows */
	m = 10 * (c[0] - '0') + (c[1] - '0'); c += 2;
	d = 10 * (c[0] - '0') + (c[1] - '0');
	/* Must have been issued at least when X509v3 was added */
	if (y < 1996 || m < 1 || m > 12 || d < 1 || d > 31) {
		return PS_FAILURE;
	}
	if (t.tm_year > (int)y) {
		cert->authFailFlags |= PS_CERT_AUTH_FAIL_DATE_FLAG;
	} else if (t.tm_year == (int)y) {
		if (t.tm_mon > m) {
			cert->authFailFlags |= PS_CERT_AUTH_FAIL_DATE_FLAG;
		} else if (t.tm_mon == m && t.tm_mday > d) {
			cert->authFailFlags |= PS_CERT_AUTH_FAIL_DATE_FLAG;
		}
	}
	return 0;
#else
/* Warn if we are skipping the date validation checks. */
#ifdef WIN32
#pragma message("CERTIFICATE DATE VALIDITY NOT SUPPORTED ON THIS PLATFORM.")
#else
#warning "CERTIFICATE DATE VALIDITY NOT SUPPORTED ON THIS PLATFORM."
#endif
	cert->authFailFlags |= PS_CERT_AUTH_FAIL_DATE_FLAG;
	return 0;
#endif /* POSIX */
}

/******************************************************************************/
/*
	Implementation specific date parser.  Does not actually verify the date
*/
static int32_t getTimeValidity(psPool_t *pool, const unsigned char **pp,
				uint16_t len, int32_t *notBeforeTimeType,
				int32_t *notAfterTimeType,
				char **notBefore, char **notAfter)
{
	const unsigned char	*p = *pp, *end;
	uint16_t			seqLen, timeLen;

	end = p + len;
	if (len < 1 || *(p++) != (ASN_SEQUENCE | ASN_CONSTRUCTED) ||
			getAsnLength(&p, len - 1, &seqLen) < 0 ||
				(uint32)(end - p) < seqLen) {
		psTraceCrypto("getTimeValidity failed on inital parse\n");
		return PS_PARSE_FAIL;
	}
/*
	Have notBefore and notAfter times in UTCTime or GeneralizedTime formats
*/
	if ((end - p) < 1 || ((*p != ASN_UTCTIME) && (*p != ASN_GENERALIZEDTIME))) {
		psTraceCrypto("Malformed validity\n");
		return PS_PARSE_FAIL;
	}
	*notBeforeTimeType = *p;
	p++;
/*
	Allocate them as null terminated strings
*/
	if (getAsnLength(&p, seqLen, &timeLen) < 0 || (uint32)(end - p) < timeLen) {
		psTraceCrypto("Malformed validity 2\n");
		return PS_PARSE_FAIL;
	}
	*notBefore = psMalloc(pool, timeLen + 1);
	if (*notBefore == NULL) {
		psError("Memory allocation error in getTimeValidity for notBefore\n");
		return PS_MEM_FAIL;
	}
	memcpy(*notBefore, p, timeLen);
	(*notBefore)[timeLen] = '\0';
	p = p + timeLen;
	if ((end - p) < 1 || ((*p != ASN_UTCTIME) && (*p != ASN_GENERALIZEDTIME))) {
		psTraceCrypto("Malformed validity 3\n");
		return PS_PARSE_FAIL;
	}
	*notAfterTimeType = *p;
	p++;
	if (getAsnLength(&p, seqLen - timeLen, &timeLen) < 0 ||
			(uint32)(end - p) < timeLen) {
		psTraceCrypto("Malformed validity 4\n");
		return PS_PARSE_FAIL;
	}
	*notAfter = psMalloc(pool, timeLen + 1);
	if (*notAfter == NULL) {
		psError("Memory allocation error in getTimeValidity for notAfter\n");
		return PS_MEM_FAIL;
	}
	memcpy(*notAfter, p, timeLen);
	(*notAfter)[timeLen] = '\0';
	p = p + timeLen;

	*pp = p;
	return PS_SUCCESS;
}

/******************************************************************************/
/*
	Could be optional.  If the tag doesn't contain the value from the left
	of the IMPLICIT keyword we don't have a match and we don't incr the pointer.
*/
static int32_t getImplicitBitString(psPool_t *pool, const unsigned char **pp,
				uint16_t len, int32_t impVal, unsigned char **bitString,
				uint16_t *bitLen)
{
	const unsigned char	*p = *pp;
	int32_t				ignore_bits;

	if (len < 1) {
		psTraceCrypto("Initial parse error in getImplicitBitString\n");
		return PS_PARSE_FAIL;
	}
/*
	We don't treat this case as an error, because of the optional nature.
*/
	if (*p != (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | impVal)) {
		return PS_SUCCESS;
	}

	p++;
	if (getAsnLength(&p, len, bitLen) < 0) {
		psTraceCrypto("Malformed implicitBitString\n");
		return PS_PARSE_FAIL;
	}
	ignore_bits = *p++;
	(*bitLen)--;
	psAssert(ignore_bits == 0);

	*bitString = psMalloc(pool, *bitLen);
	if (*bitString == NULL) {
		psError("Memory allocation error in getImplicitBitString\n");
		return PS_MEM_FAIL;
	}
	memcpy(*bitString, p, *bitLen);
	*pp = p + *bitLen;
	return PS_SUCCESS;
}


/******************************************************************************/
/*
	Implementations of this specification MUST be prepared to receive
	the following standard attribute types in issuer names:
	country, organization, organizational-unit, distinguished name qualifier,
	state or province name, and common name
*/
int32_t psX509GetDNAttributes(psPool_t *pool, const unsigned char **pp,
				uint16_t len, x509DNattributes_t *attribs, uint32_t flags)
{
	const unsigned char		*p = *pp;
	const unsigned char		*dnEnd, *dnStart, *moreInSetPtr;
	int32					id, stringType, checkHiddenNull, moreInSet;
	uint16_t				llen, setlen, arcLen;
	char					*stringOut;
#ifdef USE_SHA1
	psSha1_t	hash;
#elif defined(USE_SHA256)
	psSha256_t	hash;
#else
//TODO can we avoid hash altogether? We do not free/finalize the hash ctx on error return below.
#error USE_SHA1 or USE_SHA256 must be defined
#endif

	dnStart = p;
	if (getAsnSequence(&p, len, &llen) < 0) {
		return PS_PARSE_FAIL;
	}
	dnEnd = p + llen;

/*
	The possibility of a CERTIFICATE_REQUEST message.  Set aside full DN
*/
	if (flags & CERT_STORE_DN_BUFFER) {
		attribs->dnencLen = (uint32)(dnEnd - dnStart);
		attribs->dnenc = psMalloc(pool, attribs->dnencLen);
		if (attribs->dnenc == NULL) {
			psError("Memory allocation error in getDNAttributes\n");
			return PS_MEM_FAIL;
		}
		memcpy(attribs->dnenc, dnStart, attribs->dnencLen);
	}
	moreInSet = 0;
	while (p < dnEnd) {
		if (getAsnSet(&p, (uint32)(dnEnd - p), &setlen) < 0) {
			psTraceCrypto("Malformed DN attributes\n");
			return PS_PARSE_FAIL;
		}
		/* 99.99% of certs have one attribute per SET but did come across
			one that nested a couple at this level so let's watch out for
			that with the "moreInSet" logic */
MORE_IN_SET:
		moreInSetPtr = p;
		if (getAsnSequence(&p, (uint32)(dnEnd - p), &llen) < 0) {
			psTraceCrypto("Malformed DN attributes 2\n");
			return PS_PARSE_FAIL;
		}
		if (moreInSet > 0) {
			moreInSet -= llen + (int32)(p - moreInSetPtr);
		} else {
			if (setlen != llen + (int32)(p - moreInSetPtr)) {
				moreInSet = setlen - (int32)(p - moreInSetPtr) - llen;
			}
		}
		if (dnEnd <= p || (*(p++) != ASN_OID) ||
				getAsnLength(&p, (uint32)(dnEnd - p), &arcLen) < 0 ||
				(uint32)(dnEnd - p) < arcLen) {
			psTraceCrypto("Malformed DN attributes 3\n");
			return PS_PARSE_FAIL;
		}
/*
		id-at   OBJECT IDENTIFIER       ::=     {joint-iso-ccitt(2) ds(5) 4}
		id-at-commonName		OBJECT IDENTIFIER		::=		{id-at 3}
		id-at-countryName		OBJECT IDENTIFIER		::=		{id-at 6}
		id-at-localityName		OBJECT IDENTIFIER		::=		{id-at 7}
		id-at-stateOrProvinceName		OBJECT IDENTIFIER	::=	{id-at 8}
		id-at-organizationName			OBJECT IDENTIFIER	::=	{id-at 10}
		id-at-organizationalUnitName	OBJECT IDENTIFIER	::=	{id-at 11}
*/
		*pp = p;
/*
		Currently we are skipping OIDs not of type {joint-iso-ccitt(2) ds(5) 4}
		However, we could be dealing with an OID we MUST support per RFC.
		domainComponent is one such example.
*/
		if (dnEnd - p < 2) {
			psTraceCrypto("Malformed DN attributes 4\n");
			return PS_LIMIT_FAIL;
		}
		/* check id-at */
		if ((*p++ != 85) || (*p++ != 4) ) {
			/* OIDs we are not parsing */
			p = *pp;
/*
			Move past the OID and string type, get data size, and skip it.
			NOTE: Have had problems parsing older certs in this area.
*/
			if ((uint32)(dnEnd - p) < arcLen + 1) {
				psTraceCrypto("Malformed DN attributes 5\n");
				return PS_LIMIT_FAIL;
			}
			p += arcLen + 1;
			if (getAsnLength(&p, (uint32)(dnEnd - p), &llen) < 0 ||
					(uint32)(dnEnd - p) < llen) {
				psTraceCrypto("Malformed DN attributes 6\n");
				return PS_PARSE_FAIL;
			}
			p = p + llen;
			continue;
		}
		/* Next are the id of the attribute type and the ASN string type */
		if (arcLen != 3 || dnEnd - p < 2) {
			psTraceCrypto("Malformed DN attributes 7\n");
			return PS_LIMIT_FAIL;
		}
		id = (int32)*p++;
		/* Done with OID parsing */
		stringType = (int32)*p++;

		if (getAsnLength(&p, (uint32)(dnEnd - p), &llen) < 0 ||
				(uint32)(dnEnd - p) < llen) {
			psTraceCrypto("Malformed DN attributes 8\n");
			return PS_LIMIT_FAIL;
		}
/*
		For the known 8-bit character string types, we flag that we want
		to test for a hidden null in the middle of the string to address the
		issue of www.goodguy.com\0badguy.com.  For BMPSTRING, the user will
		have to validate against the xLen member for such abuses.
*/
		checkHiddenNull = PS_FALSE;
		switch (stringType) {
			case ASN_PRINTABLESTRING:
			case ASN_UTF8STRING:
			case ASN_IA5STRING:
				checkHiddenNull = PS_TRUE;
			case ASN_T61STRING:
			case ASN_BMPSTRING:
			case ASN_BIT_STRING:
				stringOut = psMalloc(pool, llen + 2);
				if (stringOut == NULL) {
					psError("Memory allocation error in getDNAttributes\n");
					return PS_MEM_FAIL;
				}
				memcpy(stringOut, p, llen);
/*
				Terminate with 2 null chars to support standard string
				manipulations with any potential unicode types.
*/
				stringOut[llen] = '\0';
				stringOut[llen + 1] = '\0';

				if (checkHiddenNull) {
					if ((uint32)strlen(stringOut) != llen) {
						psFree(stringOut, pool);
						psTraceCrypto("Malformed DN attributes 9\n");
						return PS_PARSE_FAIL;
					}
				}

				p = p + llen;
				llen += 2; /* Add the two null bytes for length assignments */
				break;
			default:
				psTraceIntCrypto("Unsupported DN attrib type %d\n", stringType);
				return PS_UNSUPPORTED_FAIL;
		}

		switch (id) {
			case ATTRIB_COUNTRY_NAME:
				if (attribs->country) {
					psFree(attribs->country, pool);
				}
				attribs->country = stringOut;
				attribs->countryType = (short)stringType;
				attribs->countryLen = (short)llen;
				break;
			case ATTRIB_STATE_PROVINCE:
				if (attribs->state) {
					psFree(attribs->state, pool);
				}
				attribs->state = stringOut;
				attribs->stateType = (short)stringType;
				attribs->stateLen = (short)llen;
				break;
			case ATTRIB_LOCALITY:
				if (attribs->locality) {
					psFree(attribs->locality, pool);
				}
				attribs->locality = stringOut;
				attribs->localityType = (short)stringType;
				attribs->localityLen = (short)llen;
				break;
			case ATTRIB_ORGANIZATION:
				if (attribs->organization) {
					psFree(attribs->organization, pool);
				}
				attribs->organization = stringOut;
				attribs->organizationType = (short)stringType;
				attribs->organizationLen = (short)llen;
				break;
			case ATTRIB_ORG_UNIT:
				if (attribs->orgUnit) {
					psFree(attribs->orgUnit, pool);
				}
				attribs->orgUnit = stringOut;
				attribs->orgUnitType = (short)stringType;
				attribs->orgUnitLen = (short)llen;
				break;
			case ATTRIB_COMMON_NAME:
				if (attribs->commonName) {
					psFree(attribs->commonName, pool);
				}
				attribs->commonName = stringOut;
				attribs->commonNameType = (short)stringType;
				attribs->commonNameLen = (short)llen;
				break;
			default:
				/* Not a MUST support, so just ignore unknown */
				psFree(stringOut, pool);
				stringOut = NULL;
				break;
		}
		if (moreInSet) {
			goto MORE_IN_SET;
		}
	}
	/* Hash is used to quickly compare DNs */
#ifdef USE_SHA1
	psSha1Init(&hash);
	psSha1Update(&hash, dnStart, (dnEnd - dnStart));
	psSha1Final(&hash, (unsigned char*)attribs->hash);
#else
	psSha256Init(&hash);
	psSha256Update(&hash, dnStart, (dnEnd - dnStart));
	psSha256Final(&hash, (unsigned char*)attribs->hash);
#endif
	*pp = p;
	return PS_SUCCESS;
}

/******************************************************************************/
/*
	Free helper
*/
void psX509FreeDNStruct(x509DNattributes_t *dn, psPool_t *allocPool)
{
	if (dn->country)		psFree(dn->country, allocPool);
	if (dn->state)			psFree(dn->state, allocPool);
	if (dn->locality)		psFree(dn->locality, allocPool);
	if (dn->organization)	psFree(dn->organization, allocPool);
	if (dn->orgUnit)		psFree(dn->orgUnit, allocPool);
	if (dn->commonName)		psFree(dn->commonName, allocPool);
	if (dn->dnenc)			psFree(dn->dnenc, allocPool);
}


/******************************************************************************/
/*
	Fundamental routine to test whether the supplied issuerCert issued
	the supplied subjectCert.  There are currently two tests that are
	performed here:
		1. A strict SHA1 hash comparison of the Distinguished Name details
		2. A test of the public key cryptographic cert signature

	subjectCert may be a chain.  Cert chains must always be passed with
	the child-most as the first in the list (the 'next' structure member
	points to the parent).  The authentication of the entire chain
	will be tested before the issuerCert is used to authenticate the
	parent-most certificate

	issuerCert will always be a treated as a single certificate even if it
	is a chain

	If there is no issuerCert the parent-most subejct cert will always
	be tested as a self-signed CA certificate.

	So there are three uses:
	1. Test a cert was issued by another (single subjectCert, single issuerCert)
	1. Test a self signed cert (single cert to subjectCert, no issuerCert)
	2. Test a CA terminated chain (cert chain to subjectCert, no issuerCert)

	This function exits with a failure code on the first authentication
	that doesn't succeed.  The 'authStatus' members may be examined for more
	information of where the authentication failed.

	The 'authStatus' member of the issuerCert will be set to PS_FALSE
	since it will not be authenticated.

	The 'authStatus' members of the subjectCert structures will always
	be reset to PS_FALSE when this routine is called and set to PS_TRUE
	when authenticated.  Any error during the authentication will set the
	current subject cert 'authStatus' member to PS_CERT_AUTH_FAIL and the
	function will return with an error code.

	Return codes:
		PS_SUCCESS			- yes

		PS_CERT_AUTH_FAIL	- nope. these certs are not a match
		PS_UNSUPPORTED_FAIL	- unrecognized cert format
		PS_ARG_FAIL			- local, psRsaDecryptPub
		PS_LIMIT_FAIL		- psRsaDecryptPub
		PS_FAILURE			- internal psRsaDecryptPub failure

	There is nothing for the caller to free at the completion of this
	routine.
*/
int32 psX509AuthenticateCert(psPool_t *pool, psX509Cert_t *subjectCert,
						psX509Cert_t *issuerCert,  psX509Cert_t	**foundIssuer,
						void *hwCtx, void *poolUserPtr)
{
	psX509Cert_t	*ic, *sc;
	int32			sigType, rc;
	uint32			sigLen;
	void			*rsaData;
#ifdef USE_ECC
	int32			sigStat;
#endif /* USE_ECC */
#ifdef USE_RSA
	unsigned char	sigOut[10 + MAX_HASH_SIZE + 9];	/* Max size */
	unsigned char	*tempSig = NULL;
#endif /* USE_RSA */
	psPool_t	*pkiPool = NULL;
#ifdef USE_CRL
	x509revoked_t	*curr, *next;
#endif
#ifdef USE_PKCS1_PSS
	uint16_t		pssLen;
#endif

	rc = 0;
	sigLen = 0;
	if (subjectCert == NULL) {
		psTraceCrypto("No subject cert given to psX509AuthenticateCert\n");
		return PS_ARG_FAIL;
	}

/*
	Determine what we've been passed
*/
	if (issuerCert == NULL) {
		/* reset auth flags in subjectCert chain and find first sc and ic */
		sc = subjectCert;
		while (sc) {
			sc->authStatus = PS_FALSE;
			sc = sc->next;
		}
		/* Now see if this is a chain or just a single cert */
		sc = subjectCert;
		if (sc->next == NULL) {
			ic = sc; /* A single subject cert for self-signed test */
		} else {
			ic = sc->next;
		}
	} else {
		issuerCert->authStatus = PS_FALSE;
		ic = issuerCert; /* Easy case of single subject and single issuer */
		sc = subjectCert;
	}

/*
	Error on first problem seen and set the subject status to FAIL
*/
	while (ic) {
/*
		Certificate authority constraint only available in version 3 certs.
		Only parsing version 3 certs by default though.
*/
		if ((ic->version > 1) && (ic->extensions.bc.cA <= 0)) {
			if (sc != ic) {
				psTraceCrypto("Issuer does not have basicConstraint CA permissions\n");
				sc->authStatus = PS_CERT_AUTH_FAIL_BC;
				return PS_CERT_AUTH_FAIL_BC;
			}
		}

/*
		Use sha1 hash of issuer fields computed at parse time to compare
*/
		if (memcmp(sc->issuer.hash, ic->subject.hash, SHA1_HASH_SIZE) != 0) {
			if (sc == ic) {
				psTraceCrypto("Info: not a self-signed certificate\n");
			} else {
				//psTraceCrypto("Issuer DN attributes do not match subject\n");
			}
			sc->authStatus = PS_CERT_AUTH_FAIL_DN;
			return PS_CERT_AUTH_FAIL_DN;
		}

#ifdef USE_CRL
		/* Does this issuer have a list of revoked serial numbers that needs
			to be checked? */
		if (ic->revoked) {
			curr = ic->revoked;
			while (curr != NULL) {
				next = curr->next;
				if (curr->serialLen == sc->serialNumberLen) {
					if (memcmp(curr->serial, sc->serialNumber, curr->serialLen)
							== 0) {
						sc->authStatus = PS_CERT_AUTH_FAIL_REVOKED;
						return -1;
					}
				}
				curr = next;
			}

		}
#endif

/*
		Signature confirmation
		The sigLen is the ASN.1 size in bytes for encoding the hash.
		The magic 10 is comprised of the SEQUENCE and ALGORITHM ID overhead.
		The magic 9, 8, or 5 is the OID length of the corresponding algorithm.
*/
		sigType = PS_UNSUPPORTED_FAIL;
		switch (sc->sigAlgorithm) {
#ifdef USE_RSA
#ifdef ENABLE_MD5_SIGNED_CERTS
#ifdef USE_MD2
		case OID_MD2_RSA_SIG:
#endif
		case OID_MD5_RSA_SIG:
			sigType = RSA_TYPE_SIG;
			sigLen = 10 + MD5_HASH_SIZE + 8;
			break;
#endif
#ifdef ENABLE_SHA1_SIGNED_CERTS
		case OID_SHA1_RSA_SIG:
			sigLen = 10 + SHA1_HASH_SIZE + 5;
			sigType = RSA_TYPE_SIG;
			break;
#endif
#ifdef USE_SHA256
		case OID_SHA256_RSA_SIG:
			sigLen = 10 + SHA256_HASH_SIZE + 9;
			sigType = RSA_TYPE_SIG;
			break;
#endif
#ifdef USE_SHA384
		case OID_SHA384_RSA_SIG:
			sigLen = 10 + SHA384_HASH_SIZE + 9;
			sigType = RSA_TYPE_SIG;
			break;
#endif
#ifdef USE_SHA512
		case OID_SHA512_RSA_SIG:
			sigLen = 10 + SHA512_HASH_SIZE + 9;
			sigType = RSA_TYPE_SIG;
			break;
#endif
#endif /* USE_RSA */
#ifdef USE_ECC
#ifdef ENABLE_SHA1_SIGNED_CERTS
		case OID_SHA1_ECDSA_SIG:
			sigLen = SHA1_HASH_SIZE;
			sigType = ECDSA_TYPE_SIG;
			break;
#endif
#ifdef USE_SHA256
		case OID_SHA256_ECDSA_SIG:
			sigLen = SHA256_HASH_SIZE;
			sigType = ECDSA_TYPE_SIG;
			break;
#endif
#ifdef USE_SHA384
		case OID_SHA384_ECDSA_SIG:
			sigLen = SHA384_HASH_SIZE;
			sigType = ECDSA_TYPE_SIG;
			break;
#endif
#ifdef USE_SHA512
		case OID_SHA512_ECDSA_SIG:
			sigLen = SHA512_HASH_SIZE;
			sigType = ECDSA_TYPE_SIG;
			break;
#endif
#endif /* USE_ECC */

#ifdef USE_PKCS1_PSS
		case OID_RSASSA_PSS:
			switch (sc->pssHash) {
#ifdef ENABLE_MD5_SIGNED_CERTS
			case PKCS1_MD5_ID:
				sigLen = MD5_HASH_SIZE;
				break;
#endif
#ifdef ENABLE_SHA1_SIGNED_CERTS
			case PKCS1_SHA1_ID:
				sigLen = SHA1_HASH_SIZE;
				break;
#endif
#ifdef USE_SHA256
			case PKCS1_SHA256_ID:
				sigLen = SHA256_HASH_SIZE;
				break;
#endif
#ifdef USE_SHA384
			case PKCS1_SHA384_ID:
				sigLen = SHA384_HASH_SIZE;
				break;
#endif
#ifdef USE_SHA512
			case PKCS1_SHA512_ID:
				sigLen = SHA512_HASH_SIZE;
				break;
#endif
			default:
				return PS_UNSUPPORTED_FAIL;
			}
			sigType = RSAPSS_TYPE_SIG;
			break;
#endif
		default:
			sigType = PS_UNSUPPORTED_FAIL;
			break;
		}

		if (sigType == PS_UNSUPPORTED_FAIL) {
			sc->authStatus = PS_CERT_AUTH_FAIL_SIG;
			psTraceIntCrypto("Unsupported certificate signature algorithm %d\n",
				subjectCert->sigAlgorithm);
			return sigType;
		}

#ifdef USE_RSA
		if (sigType == RSA_TYPE_SIG || sigType == RSAPSS_TYPE_SIG) {
		}
		/* Now do the signature validation */
		if (sigType == RSA_TYPE_SIG) {
			psAssert(sigLen <= sizeof(sigOut));
/*
			psRsaDecryptPub destroys the 'in' parameter so let it be a tmp
*/
			tempSig = psMalloc(pool, sc->signatureLen);
			if (tempSig == NULL) {
				psError("Memory allocation error: psX509AuthenticateCert\n");
				return PS_MEM_FAIL;
			}
			memcpy(tempSig, sc->signature, sc->signatureLen);

			rsaData = NULL;

			if ((rc = psRsaDecryptPub(pkiPool, &ic->publicKey.key.rsa,
					tempSig, sc->signatureLen, sigOut, sigLen, rsaData)) < 0) {


				psTraceCrypto("Unable to RSA decrypt certificate signature\n");
				sc->authStatus = PS_CERT_AUTH_FAIL_SIG;
				psFree(tempSig, pool);
				return rc;
			}
			psFree(tempSig, pool);
			rc = x509ConfirmSignature(sc->sigHash, sigOut, sigLen);
		}
#if defined(USE_PKCS1_PSS) && !defined(USE_PKCS1_PSS_VERIFY_ONLY)
		if (sigType == RSAPSS_TYPE_SIG) {
			tempSig = psMalloc(pool, sc->signatureLen);
			if (tempSig == NULL) {
				psError("Memory allocation error: psX509AuthenticateCert\n");
				return PS_MEM_FAIL;
			}
			pssLen = sc->signatureLen;
			if ((rc = psRsaCrypt(pkiPool, &ic->publicKey.key.rsa,
					sc->signature, sc->signatureLen, tempSig, &pssLen,
					PS_PUBKEY, rsaData)) < 0) {
				psFree(tempSig, pool);
				return rc;
			}

			if (pkcs1PssDecode(pkiPool, sc->sigHash, sigLen, tempSig,
					pssLen, sc->saltLen, sc->pssHash, ic->publicKey.keysize * 8,
					&rc) < 0) {
				psFree(tempSig, pool);
				return PS_FAILURE;
			}
			psFree(tempSig, pool);

			if (rc == 0) {
				/* This is an indication the hash did NOT match */
				rc = -1; /* The test below is looking for < 0 */
			}
		}
#endif /* defined(USE_PKCS1_PSS) && !defined(USE_PKCS1_PSS_VERIFY_ONLY)	*/
#endif /* USE_RSA */

#ifdef USE_ECC
		if (sigType == ECDSA_TYPE_SIG) {
			rsaData = NULL;
			if ((rc = psEccDsaVerify(pkiPool,
					&ic->publicKey.key.ecc,
					sc->sigHash, sigLen,
					sc->signature, sc->signatureLen,
					&sigStat, rsaData)) != 0) {
				psTraceCrypto("Error validating ECDSA certificate signature\n");
				sc->authStatus = PS_CERT_AUTH_FAIL_SIG;
				return rc;
			}
			if (sigStat == -1) {
				/* No errors, but signature didn't pass */
				psTraceCrypto("ECDSA certificate signature failed\n");
				rc = -1;
			}
		}
#endif /* USE_ECC */


/*
		Test what happen in the signature test?
*/
		if (rc < PS_SUCCESS) {
			sc->authStatus = PS_CERT_AUTH_FAIL_SIG;
			return rc;
		}


		/* X.509 extension tests.  Problems below here will be collected
			in flags and given to the user */

		/* If date was out of range in parse, flag it here */
		if (sc->authFailFlags & PS_CERT_AUTH_FAIL_DATE_FLAG) {
			sc->authStatus = PS_CERT_AUTH_FAIL_EXTENSION;
		}

		/* Verify subject key and auth key if either is non-zero */
		if (sc->extensions.ak.keyLen > 0 || ic->extensions.sk.len > 0) {
			if (ic->extensions.sk.len != sc->extensions.ak.keyLen) {
				/* The one exception to this test would be if this is a 
					self-signed CA being authenticated with the exact same
					self-signed CA and that certificate does not popluate
					the Authority Key Identifier extension */
				if ((sc->signatureLen == ic->signatureLen) &&
						(memcmp(sc->signature, ic->signature, ic->signatureLen)
						== 0)) {
					if (sc->extensions.ak.keyLen != 0) {
						psTraceCrypto("Subject/Issuer key id mismatch\n");
						sc->authStatus = PS_CERT_AUTH_FAIL_AUTHKEY;
					}
				} else {
					psTraceCrypto("Subject/Issuer key id mismatch\n");
					sc->authStatus = PS_CERT_AUTH_FAIL_AUTHKEY;
				}
			} else {
				if (memcmp(ic->extensions.sk.id, sc->extensions.ak.keyId,
						ic->extensions.sk.len) != 0) {
					psTraceCrypto("Subject/Issuer key id data mismatch\n");
					sc->authStatus = PS_CERT_AUTH_FAIL_AUTHKEY;
				}
			}
		}

		/* Ensure keyCertSign of KeyUsage. The second byte of the BIT STRING
			will always contain the relevant information. */
		if ( ! (ic->extensions.keyUsageFlags & KEY_USAGE_KEY_CERT_SIGN)) {
			/* @security If keyUsageFlags is zero, it may not exist at all
				in the cert. This is allowed if the cert was issued before
				the RFC was updated to require this field for CA certificates.
				RFC3280 and above specify this as a MUST for CACerts. */
			if (ic->extensions.keyUsageFlags == 0) {
				rc = issuedBefore(RFC_3280, ic);
			} else {
				rc = 0;	/* Awkward code to force the compare below */
			}
			/* Iff rc == 1 we won't error */
			if (!rc) {
				psTraceCrypto("Issuer does not allow keyCertSign in keyUsage\n");
				sc->authFailFlags |= PS_CERT_AUTH_FAIL_KEY_USAGE_FLAG;
				sc->authStatus = PS_CERT_AUTH_FAIL_EXTENSION;
			} else if (rc < 0) {
				psTraceCrypto("Issue date check failed\n");
				return PS_PARSE_FAIL;
			}
		}
/*
		Fall through to here only if passed all non-failure checks.
*/
		if (sc->authStatus == PS_FALSE) { /* Hasn't been touched */
			sc->authStatus = PS_CERT_AUTH_PASS;
		}
/*
		Loop control for finding next ic and sc.
*/
		if (ic == sc) {
			*foundIssuer = ic;
			ic = NULL; /* Single self-signed test completed */
		} else if (ic == issuerCert) {
			*foundIssuer = ic;
			ic = NULL; /* If issuerCert was used, that is always final test */
		} else {
			sc = ic;
			ic = sc->next;
			if (ic == NULL) { /* Reached end of chain */
				*foundIssuer = ic;
				ic = sc; /* Self-signed test on final subectCert chain */
			}
		}

	}
	return PS_SUCCESS;
}

#ifdef USE_RSA
/******************************************************************************/
/*
	Do the signature validation for a subject certificate against a
	known CA certificate
*/
static int32_t x509ConfirmSignature(const unsigned char *sigHash,
				const unsigned char *sigOut, uint16_t sigLen)
{
	const unsigned char	*end;
	const unsigned char	*p = sigOut;
	unsigned char	hash[MAX_HASH_SIZE];
	int32_t			oi;
	uint16_t		len, plen;

	end = p + sigLen;
/*
	DigestInfo ::= SEQUENCE {
		digestAlgorithm DigestAlgorithmIdentifier,
		digest Digest }

	DigestAlgorithmIdentifier ::= AlgorithmIdentifier

	Digest ::= OCTET STRING
*/
	if (getAsnSequence(&p, (uint32)(end - p), &len) < 0) {
		psTraceCrypto("Initial parse error in x509ConfirmSignature\n");
		return PS_PARSE_FAIL;
	}

	/* Could be MD5 or SHA1 */
	if (getAsnAlgorithmIdentifier(&p, (uint32)(end - p), &oi, &plen) < 0) {
		psTraceCrypto("Algorithm ID parse error in x509ConfirmSignature\n");
		return PS_PARSE_FAIL;
	}
	psAssert(plen == 0);
	if ((*p++ != ASN_OCTET_STRING) ||
			getAsnLength(&p, (uint32)(end - p), &len) < 0 ||
				(uint32)(end - p) <  len) {
		psTraceCrypto("getAsnLength parse error in x509ConfirmSignature\n");
		return PS_PARSE_FAIL;
	}
	memcpy(hash, p, len);
	switch (oi) {
#ifdef ENABLE_MD5_SIGNED_CERTS
#ifdef USE_MD2
	case OID_MD2_ALG:
#endif
	case OID_MD5_ALG:
		if (len != MD5_HASH_SIZE) {
			psTraceCrypto("MD5_HASH_SIZE error in x509ConfirmSignature\n");
			return PS_LIMIT_FAIL;
		}
		break;
#endif
#ifdef ENABLE_SHA1_SIGNED_CERTS
	case OID_SHA1_ALG:
		if (len != SHA1_HASH_SIZE) {
			psTraceCrypto("SHA1_HASH_SIZE error in x509ConfirmSignature\n");
			return PS_LIMIT_FAIL;
		}
		break;
#endif
#ifdef USE_SHA256
	case OID_SHA256_ALG:
		if (len != SHA256_HASH_SIZE) {
			psTraceCrypto("SHA256_HASH_SIZE error in x509ConfirmSignature\n");
			return PS_LIMIT_FAIL;
		}
		break;
#endif
#ifdef USE_SHA384
	case OID_SHA384_ALG:
		if (len != SHA384_HASH_SIZE) {
			psTraceCrypto("SHA384_HASH_SIZE error in x509ConfirmSignature\n");
			return PS_LIMIT_FAIL;
		}
		break;
#endif
#ifdef USE_SHA512
	case OID_SHA512_ALG:
		if (len != SHA512_HASH_SIZE) {
			psTraceCrypto("SHA512_HASH_SIZE error in x509ConfirmSignature\n");
			return PS_LIMIT_FAIL;
		}
		break;
#endif
	default:
		psTraceCrypto("Unsupported alg ID error in x509ConfirmSignature\n");
		return PS_UNSUPPORTED_FAIL;
	}
	/* hash should match sigHash */
	if (memcmp(hash, sigHash, len) != 0) {
		psTraceCrypto("Signature failure in x509ConfirmSignature\n");
		return PS_SIGNATURE_MISMATCH;
	}
	return PS_SUCCESS;
}
#endif /* USE_RSA */

/******************************************************************************/
#ifdef USE_CRL
static void x509FreeRevoked(x509revoked_t **revoked)
{
	x509revoked_t		*next, *curr = *revoked;

	while (curr) {
		next = curr->next;
		psFree(curr->serial, curr->pool);
		psFree(curr, curr->pool);
		curr = next;
	}
	*revoked = NULL;
}

/*
	Parse a CRL and confirm was issued by supplied CA.

	Only interested in the revoked serial numbers which are stored in the
	CA structure if all checks out.  Used during cert validation as part of
	the default tests

	poolUserPtr is for the TMP_PKI pool
*/
int32 psX509ParseCrl(psPool_t *pool, psX509Cert_t *CA, int append,
						unsigned char *crlBin, int32 crlBinLen,
						void *poolUserPtr)
{
	unsigned char		*end, *start, *revStart, *sigStart, *sigEnd,*p = crlBin;
	int32				oi, plen, sigLen, version, rc;
	unsigned char		sigHash[SHA512_HASH_SIZE], sigOut[SHA512_HASH_SIZE];
	x509revoked_t		*curr, *next;
	x509DNattributes_t	issuer;
	x509v3extensions_t	ext;
	psDigestContext_t	hashCtx;
	psPool_t			*pkiPool = MATRIX_NO_POOL;
	uint16_t			glen, ilen, timelen;

	end = p + crlBinLen;
	/*
		CertificateList  ::=  SEQUENCE  {
			tbsCertList          TBSCertList,
			signatureAlgorithm   AlgorithmIdentifier,
			signatureValue       BIT STRING  }

		TBSCertList  ::=  SEQUENCE  {
			version                 Version OPTIONAL,
									 -- if present, shall be v2
			signature               AlgorithmIdentifier,
			issuer                  Name,
			thisUpdate              Time,
			nextUpdate              Time OPTIONAL,
			revokedCertificates     SEQUENCE OF SEQUENCE  {
			 userCertificate         CertificateSerialNumber,
			 revocationDate          Time,
			 crlEntryExtensions      Extensions OPTIONAL
										   -- if present, shall be v2
								  }  OPTIONAL,
			crlExtensions           [0]  EXPLICIT Extensions OPTIONAL
										   -- if present, shall be v2
		}
	*/
	if (getAsnSequence(&p, (uint32)(end - p), &glen) < 0) {
		psTraceCrypto("Initial parse error in psX509ParseCrl\n");
		return PS_PARSE_FAIL;
	}

	sigStart = p;
	if (getAsnSequence(&p, (uint32)(end - p), &glen) < 0) {
		psTraceCrypto("Initial parse error in psX509ParseCrl\n");
		return PS_PARSE_FAIL;
	}
	if (*p == ASN_INTEGER) {
		version = 0;
		if (getAsnInteger(&p, (uint32)(end - p), &version) < 0 || version != 1){
			psTraceIntCrypto("Version parse error in psX509ParseCrl %d\n",
				version);
			return PS_PARSE_FAIL;
		}
	}
	/* signature */
	if (getAsnAlgorithmIdentifier(&p, (int32)(end - p), &oi, &plen) < 0) {
		psTraceCrypto("Couldn't parse crl sig algorithm identifier\n");
		return PS_PARSE_FAIL;
	}

	/*
		Name            ::=   CHOICE { -- only one possibility for now --
								 rdnSequence  RDNSequence }

		RDNSequence     ::=   SEQUENCE OF RelativeDistinguishedName

		DistinguishedName       ::=   RDNSequence

		RelativeDistinguishedName  ::=
					SET SIZE (1 .. MAX) OF AttributeTypeAndValue
	*/
	memset(&issuer, 0x0, sizeof(x509DNattributes_t));
	if ((rc = psX509GetDNAttributes(pool, &p, (uint32)(end - p),
			&issuer, 0)) < 0) {
		psTraceCrypto("Couldn't parse crl issuer DN attributes\n");
		return rc;
	}
	/* Ensure crlSign flag of KeyUsage for the given CA. */
	if ( ! (CA->extensions.keyUsageFlags & KEY_USAGE_CRL_SIGN)) {
		psTraceCrypto("Issuer does not allow crlSign in keyUsage\n");
		CA->authFailFlags |= PS_CERT_AUTH_FAIL_KEY_USAGE_FLAG;
		CA->authStatus = PS_CERT_AUTH_FAIL_EXTENSION;
		psX509FreeDNStruct(&issuer, pool);
		return PS_CERT_AUTH_FAIL_EXTENSION;
	}
	if (memcmp(issuer.hash, CA->subject.hash, SHA1_HASH_SIZE) != 0) {
		psTraceCrypto("CRL NOT ISSUED BY THIS CA\n");
		psX509FreeDNStruct(&issuer, pool);
		return PS_CERT_AUTH_FAIL_DN;
	}
	psX509FreeDNStruct(&issuer, pool);

	/* thisUpdate TIME */
	if ((end - p) < 1 || ((*p != ASN_UTCTIME) && (*p != ASN_GENERALIZEDTIME))) {
		psTraceCrypto("Malformed thisUpdate CRL\n");
		return PS_PARSE_FAIL;
	}
	p++;
	if (getAsnLength(&p, (uint32)(end - p), &timelen) < 0 ||
			(uint32)(end - p) < timelen) {
		psTraceCrypto("Malformed thisUpdate CRL\n");
		return PS_PARSE_FAIL;
	}
	p += timelen;	/* Skip it */
	/* nextUpdateTIME - Optional */
	if ((end - p) < 1 || ((*p == ASN_UTCTIME) || (*p == ASN_GENERALIZEDTIME))) {
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &timelen) < 0 ||
				(uint32)(end - p) < timelen) {
			psTraceCrypto("Malformed nextUpdateTIME CRL\n");
			return PS_PARSE_FAIL;
		}
		p += timelen;	/* Skip it */
	}
	/*
		revokedCertificates     SEQUENCE OF SEQUENCE  {
			 userCertificate         CertificateSerialNumber,
			 revocationDate          Time,
			 crlEntryExtensions      Extensions OPTIONAL
										   -- if present, shall be v2
								  }  OPTIONAL,
	*/
	if (getAsnSequence(&p, (uint32)(end - p), &glen) < 0) {
		psTraceCrypto("Initial revokedCertificates error in psX509ParseCrl\n");
		return PS_PARSE_FAIL;
	}

	if (CA->revoked) {
		/* Append or refresh */
		if (append == 0) {
			/* refresh */
			x509FreeRevoked(&CA->revoked);
			CA->revoked = curr = psMalloc(pool, sizeof(x509revoked_t));
			if (curr == NULL) {
				return PS_MEM_FAIL;
			}
		} else {
			/* append.  not looking for duplicates */
			curr = psMalloc(pool, sizeof(x509revoked_t));
			if (curr == NULL) {
				return PS_MEM_FAIL;
			}
			curr->pool = pool;
			next = CA->revoked;
			while (next->next != NULL) {
				next = next->next;
			}
			next->next = curr;
		}
	} else {
		CA->revoked = curr = psMalloc(pool, sizeof(x509revoked_t));
		if (curr == NULL) {
			return PS_MEM_FAIL;
		}
	}
	memset(curr, 0x0, sizeof(x509revoked_t));
	curr->pool = pool;


	while (glen > 0) {
		revStart = p;
		if (getAsnSequence(&p, (uint32)(end - p), &ilen) < 0) {
			psTraceCrypto("Deep revokedCertificates error in psX509ParseCrl\n");
			return PS_PARSE_FAIL;
		}
		start = p;
		if ((rc = getSerialNum(pool, &p, (uint32)(end - p), &curr->serial,
				&curr->serialLen)) < 0) {
			psTraceCrypto("ASN serial number parse error\n");
			return rc;
		}
		/* skipping time and extensions */
		p += ilen - (uint32)(p - start);
		if (glen < (uint32)(p - revStart)) {
			psTraceCrypto("Deeper revokedCertificates err in psX509ParseCrl\n");
			return PS_PARSE_FAIL;
		}
		glen -= (uint32)(p - revStart);

		// psTraceBytes("revoked", curr->serial, curr->serialLen);
		if (glen > 0) {
			if ((next = psMalloc(pool, sizeof(x509revoked_t))) == NULL) {
				x509FreeRevoked(&CA->revoked);
				return PS_MEM_FAIL;
			}
			memset(next, 0x0, sizeof(x509revoked_t));
			next->pool = pool;
			curr->next = next;
			curr = next;
		}
	}
	memset(&ext, 0x0, sizeof(x509v3extensions_t));
	if (getExplicitExtensions(pool, &p, (uint32)(end - p), 0, &ext, 0) < 0) {
		psTraceCrypto("Extension parse error in psX509ParseCrl\n");
		x509FreeRevoked(&CA->revoked);
		return PS_PARSE_FAIL;
	}
	x509FreeExtensions(&ext);
	sigEnd = p;

	if (getAsnAlgorithmIdentifier(&p, (int32)(end - p), &oi, &plen) < 0) {
		x509FreeRevoked(&CA->revoked);
		psTraceCrypto("Couldn't parse crl sig algorithm identifier\n");
		return PS_PARSE_FAIL;
	}

	if ((rc = psX509GetSignature(pool, &p, (uint32)(end - p), &revStart, &ilen))
			< 0) {
		x509FreeRevoked(&CA->revoked);
		psTraceCrypto("Couldn't parse signature\n");
		return rc;
	}

	switch (oi) {
#ifdef ENABLE_MD5_SIGNED_CERTS
	case OID_MD5_RSA_SIG:
		sigLen = MD5_HASH_SIZE;
		psMd5Init(&hashCtx);
		psMd5Update(&hashCtx, sigStart, (uint32)(sigEnd - sigStart));
		psMd5Final(&hashCtx, sigHash);
		break;
#endif
#ifdef ENABLE_SHA1_SIGNED_CERTS
	case OID_SHA1_RSA_SIG:
		sigLen = SHA1_HASH_SIZE;
		psSha1Init(&hashCtx);
		psSha1Update(&hashCtx, sigStart, (uint32)(sigEnd - sigStart));
		psSha1Final(&hashCtx, sigHash);
		break;
#endif
#ifdef USE_SHA256
	case OID_SHA256_RSA_SIG:
		sigLen = SHA256_HASH_SIZE;
		psSha256Init(&hashCtx);
		psSha256Update(&hashCtx, sigStart, (uint32)(sigEnd - sigStart));
		psSha256Final(&hashCtx, sigHash);
		break;
#endif
	default:
		psTraceCrypto("Need more signatuare alg support for CRL\n");
		x509FreeRevoked(&CA->revoked);
		return PS_UNSUPPORTED_FAIL;
	}



	if ((rc = pubRsaDecryptSignedElement(pkiPool, &CA->publicKey.key.rsa,
			revStart, ilen, sigOut, sigLen, NULL)) < 0) {
		x509FreeRevoked(&CA->revoked);
		psTraceCrypto("Unable to RSA decrypt CRL signature\n");
		return rc;
	}


	if (memcmp(sigHash, sigOut, sigLen) != 0) {
		x509FreeRevoked(&CA->revoked);
		psTraceCrypto("Unable to verify CRL signature\n");
		return PS_CERT_AUTH_FAIL_SIG;
	}

	return PS_SUCCESS;
}
#endif /* USE_CRL */
#endif /* USE_CERT_PARSE */


#ifdef USE_OCSP
static int32_t parseSingleResponse(uint32_t len, const unsigned char **cp,
						const unsigned char *end, mOCSPSingleResponse_t *res)
{
	const unsigned char	*p;
	uint16_t			glen, plen;
	int32_t				oi;
	
	p = *cp;
	
	/*	SingleResponse ::= SEQUENCE {
			certID                  CertID,
			certStatus              CertStatus,
			thisUpdate              GeneralizedTime,
			nextUpdate          [0] EXPLICIT GeneralizedTime OPTIONAL,
			singleExtensions    [1] EXPLICIT Extensions OPTIONAL }
	*/
	if (getAsnSequence(&p, (int32)(end - p), &glen) < 0) {
		psTraceCrypto("Initial parseSingleResponse parse failure\n");
		return PS_PARSE_FAIL;
	}
	/* CertID ::= SEQUENCE {
		hashAlgorithm            AlgorithmIdentifier
                                 {DIGEST-ALGORITHM, {...}},
		issuerNameHash     OCTET STRING, -- Hash of issuer's DN
		issuerKeyHash      OCTET STRING, -- Hash of issuer's public key
		serialNumber       CertificateSerialNumber }
	*/
	if (getAsnSequence(&p, (int32)(end - p), &glen) < 0) {
		psTraceCrypto("Initial parseSingleResponse parse failure\n");
		return PS_PARSE_FAIL;
	}
	if (getAsnAlgorithmIdentifier(&p, (int32)(end - p), &oi, &plen) < 0){
		return PS_FAILURE;
	}
	psAssert(plen == 0);
	res->certIdHashAlg = oi;
	
	if ((*p++ != ASN_OCTET_STRING) ||
			getAsnLength(&p, (int32)(end - p), &glen) < 0 ||
			(uint32)(end - p) < glen) {
		return PS_PARSE_FAIL;
	}
	res->certIdNameHash = p;
	p += glen;
	
	if ((*p++ != ASN_OCTET_STRING) ||
			getAsnLength(&p, (int32)(end - p), &glen) < 0 ||
			(uint32)(end - p) < glen) {
		return PS_PARSE_FAIL;
	}
	res->certIdKeyHash = p;
	p += glen;
	
	/* serialNumber       CertificateSerialNumber  
	
		CertificateSerialNumber  ::=  INTEGER
	*/
	if ((*p != (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 2)) &&
			(*p != ASN_INTEGER)) {
		psTraceCrypto("X.509 getSerialNum failed on first bytes\n");
		return PS_PARSE_FAIL;
	}
	p++;

	if (getAsnLength(&p, (int32)(end - p), &glen) < 0 ||
			(uint32)(end - p) < glen) {
		psTraceCrypto("ASN getSerialNum failed\n");
		return PS_PARSE_FAIL;
	}
	res->certIdSerialLen = glen;
	res->certIdSerial = p;
	p += glen;
	
	/* CertStatus ::= CHOICE {
			good                [0]     IMPLICIT NULL,
			revoked             [1]     IMPLICIT RevokedInfo,
			unknown             [2]     IMPLICIT UnknownInfo }
	*/
	if (*p == (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 0)) {
		res->certStatus = 0;
		p += 2;
	} else if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 1)) {
		res->certStatus = 1;
		psTraceCrypto("OCSP CertStatus is revoked.  Skipping details\n");
		/* RevokedInfo ::= SEQUENCE {
				revocationTime              GeneralizedTime,
				revocationReason    [0]     EXPLICIT CRLReason OPTIONAL }
		*/
		if (getAsnSequence(&p, (int32)(end - p), &glen) < 0) {
			psTraceCrypto("Initial parseSingleResponse parse failure\n");
			return PS_PARSE_FAIL;
		}
		/* skip it */
		p += glen;
	} else if (*p == (ASN_CONTEXT_SPECIFIC | ASN_PRIMITIVE | 2)) {
		res->certStatus = 2;
		p += 2; /* TOOD: Untested parse.  Might be CONSTRUCTED encoding */
		/* UnknownInfo ::= NULL */
	} else {
		psTraceCrypto("OCSP CertStatus parse fail\n");
		return PS_PARSE_FAIL;
	}
	
	/* thisUpdate GeneralizedTime, */
	if ((end - p) < 1 || (*p != ASN_GENERALIZEDTIME)) {
		psTraceCrypto("Malformed thisUpdate OCSP\n");
		return PS_PARSE_FAIL;
	}
	p++;
	if (getAsnLength(&p, (uint32)(end - p), &glen) < 0 ||
			(uint32)(end - p) < glen) {
		return PS_PARSE_FAIL;
	}
	res->thisUpdateLen = glen;
	res->thisUpdate = p;
	p += glen;

	/* nextUpdate          [0] EXPLICIT GeneralizedTime OPTIONAL, */
	
	if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 0)) {
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &glen) < 0 ||
				(uint32)(end - p) < glen) {
			return PS_PARSE_FAIL;
		}
		p += glen; /* SKIPPING  */
	}
	
	/* singleExtensions    [1] EXPLICIT Extensions OPTIONAL */
	if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 1)) {
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &glen) < 0 ||
				(uint32)(end - p) < glen) {
			return PS_PARSE_FAIL;
		}
		/* TODO */
		p += glen; /* SKIPPING  */
	}
	
	*cp = (unsigned char*)p;
	return PS_SUCCESS;
}

static int32_t parseBasicOCSPResponse(psPool_t *pool, uint32_t len,
				const unsigned char **cp, unsigned char *end,
				mOCSPResponse_t *res)
{
	const unsigned char	*p, *seqend, *startRes, *endRes;
	mOCSPSingleResponse_t	*singleResponse;
	psSha1_t			sha;
#ifdef USE_SHA256
	psSha256_t			sha2;
#endif
#ifdef USE_SHA384
	psSha384_t			sha3;
#endif
	uint16_t			glen, plen;
	uint32_t			blen;
	int32_t				version, oid;
	
	/* id-pkix-ocsp-basic
			
		BasicOCSPResponse       ::= SEQUENCE {
			tbsResponseData      ResponseData,
			signatureAlgorithm   AlgorithmIdentifier,
			signature            BIT STRING,
			certs        [0] EXPLICIT SEQUENCE OF Certificate OPTIONAL }
	*/
	p = *cp;
	
	if (getAsnSequence(&p, (uint32)(end - p), &glen) < 0) {
		psTraceCrypto("Initial parse error in parseBasicOCSPResponse\n");
		return PS_PARSE_FAIL;
	}
	/* 
		ResponseData ::= SEQUENCE {
			version              [0] EXPLICIT Version DEFAULT v1,
			responderID              ResponderID,
			producedAt               GeneralizedTime,
			responses                SEQUENCE OF SingleResponse,
			responseExtensions   [1] EXPLICIT Extensions OPTIONAL }
	*/
	startRes = p; /* A response signature will be over ResponseData */
	if (getAsnSequence(&p, (uint32)(end - p), &glen) < 0) {
		psTraceCrypto("Early ResponseData parse error in parseOCSPResponse\n");
		return PS_PARSE_FAIL;
	}
	if (getExplicitVersion(&p, (uint32)(end - p), 0, &version) < 0) {
		psTraceCrypto("Version parse error in ResponseData\n");
		return PS_PARSE_FAIL;
	}
	if (version != 0) {
		psTraceIntCrypto("WARNING: Unknown OCSP ResponseData version %d\n",
			version);
	}
	/* 
		ResponderID ::= CHOICE {
			byName               [1] Name,
			byKey                [2] KeyHash }
	*/
	if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 1)) {
		psTraceCrypto("TODO: Unsupported byName ResponderID in ResponseData\n");
		return PS_PARSE_FAIL;
	} else if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 2)) {
		p++;
		if (getAsnLength32(&p, (uint32_t)(end - p), &blen, 0) < 0 ||
				(uint32_t)(end - p) < blen) {
			psTraceCrypto("Error parsing KeyHash in ResponseData\n");
			return PS_PARSE_FAIL;
		}
		/* KeyHash ::= OCTET STRING -- SHA-1 hash of responder's public key
                         -- (i.e., the SHA-1 hash of the value of the
                         -- BIT STRING subjectPublicKey [excluding
                         -- the tag, length, and number of unused
                         -- bits] in the responder's certificate) */
		if ((*p++ != ASN_OCTET_STRING) ||
				getAsnLength(&p, (int32)(end - p), &glen) < 0 ||
				(uint32)(end - p) < glen) {

			psTraceCrypto("Couldn't parse KeyHash in ResponseData\n");
			return PS_FAILURE;
		}
		psAssert(glen == SHA1_HASH_SIZE);
		res->responderKeyHash = p;
		p += SHA1_HASH_SIZE;
	} else {
		psTraceCrypto("ResponderID parse error in ResponseData\n");
		return PS_PARSE_FAIL;
	}
		
	/* producedAt GeneralizedTime, */
	if ((end - p) < 1 || (*p != ASN_GENERALIZEDTIME)) {
		psTraceCrypto("Malformed thisUpdate CRL\n");
		return PS_PARSE_FAIL;
	}
	p++;
	if (getAsnLength(&p, (uint32)(end - p), &glen) < 0 ||
			(uint32)(end - p) < glen) {
		psTraceCrypto("Malformed producedAt in ResponseData\n");
		return PS_PARSE_FAIL;
	}
	psAssert(glen <= 20); /* TODO: length hardcoded in structure */
	res->timeProducedLen = glen;
	res->timeProduced = p;
	p += glen;
		
	/* responses                SEQUENCE OF SingleResponse, */
	if (getAsnSequence(&p, (int32)(end - p), &glen) < 0) {
		psTraceCrypto("Initial SingleResponse parse failure\n");
		return PS_PARSE_FAIL;
	}

	seqend = p + glen;

	plen = 0; /* for MAX_OCSP_RESPONSES control */
	while (p < seqend) {
		singleResponse = &res->singleResponse[plen];
		if (parseSingleResponse(glen, &p, seqend, singleResponse) < 0) {
			return PS_PARSE_FAIL;
		}
		plen++;
		if (p < seqend) {
			/* Additional responses */
			if (plen == MAX_OCSP_RESPONSES) {
				psTraceCrypto("ERROR: Multiple OCSP SingleResponse items. ");
				psTraceCrypto("Increase MAX_OCSP_RESPONSES to support\n");
				return PS_PARSE_FAIL;
			}
		}
	}
		
	/* responseExtensions   [1] EXPLICIT Extensions OPTIONAL } */
	if (*p == (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 1)) {
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &glen) < 0 ||
				(uint32)(end - p) < glen) {
			return PS_PARSE_FAIL;
		}
		/* TODO: */
		p += glen; /* SKIPPING  */
	}
	endRes = p;
		
	/* ResponseData DONE.  On to signature:
		
		signatureAlgorithm   AlgorithmIdentifier
		signature            BIT STRING,
			
		The value for signature SHALL be computed on the hash of the DER
		encoding of ResponseData.  The responder MAY include certificates in
		the certs field of BasicOCSPResponse that help the OCSP client
		verify the responder's signature.  If no certificates are included,
		then certs SHOULD be absent. */
	if (getAsnAlgorithmIdentifier(&p, (uint32)(end - p), &oid, &plen) < 0) {
		psTraceCrypto("Initial SingleResponse parse failure\n");
		return PS_PARSE_FAIL;
	}
	if (plen > 0) {
		psTraceCrypto("Algorithm parameters on ResponseData sigAlg\n");
		p += plen;
	}
	res->sigAlg = oid;
		
	switch (oid) {
	/* OSCP requires SHA1 so no wrapper here */
	case OID_SHA1_RSA_SIG:
#ifdef USE_ECC
	case OID_SHA1_ECDSA_SIG:
#endif
		res->hashLen = SHA1_HASH_SIZE;
		psSha1Init(&sha);
		psSha1Update(&sha, startRes, (int32)(endRes - startRes));
		psSha1Final(&sha, res->hashResult);
		break;
#ifdef USE_SHA256
	case OID_SHA256_RSA_SIG:
#ifdef USE_ECC
	case OID_SHA256_ECDSA_SIG:
#endif
		res->hashLen = SHA256_HASH_SIZE;
		psSha256Init(&sha2);
		psSha256Update(&sha2, startRes, (int32)(endRes - startRes));
		psSha256Final(&sha2, res->hashResult);
		break;
#endif
#ifdef USE_SHA384
	case OID_SHA384_RSA_SIG:
#ifdef USE_ECC
	case OID_SHA384_ECDSA_SIG:
#endif
		res->hashLen = SHA384_HASH_SIZE;
		psSha384Init(&sha3);
		psSha384Update(&sha3, startRes, (int32)(endRes - startRes));
		psSha384Final(&sha3, res->hashResult);
		break;
#endif
	default:
		psTraceCrypto("No support for sigAlg in OCSP ResponseData\n");
		return PS_UNSUPPORTED_FAIL;
	}

	if (*p++ != ASN_BIT_STRING) {
		psTraceCrypto("Error parsing signature in ResponseData\n");
		return PS_PARSE_FAIL;
	}
	if (getAsnLength(&p, (int32)(end - p), &glen) < 0 ||
			(uint32)(end - p) < glen) {
		psTraceCrypto("Error parsing signature in ResponseData\n");
		return PS_PARSE_FAIL;
	}
	if (*p++ != 0) {
		psTraceCrypto("Error parsing ignore bits in ResponseData sig\n");
		return PS_PARSE_FAIL;
	}
	glen--; /* ignore bits above */
	res->sig = p;
	res->sigLen = glen;
	p += glen;

	/* certs        [0] EXPLICIT SEQUENCE OF Certificate OPTIONAL } */
	if (end != p) {
		/* The responder MAY include certificates in the certs field of
			BasicOCSPResponse that help the OCSP client verify the responder's
			signature. */
		if (*p != (ASN_CONTEXT_SPECIFIC | ASN_CONSTRUCTED | 0)) {
			psTraceCrypto("Unexpected Certificage encoding in OCSPResponse\n");
			return PS_PARSE_FAIL;
		}
		p++;
		if (getAsnLength(&p, (uint32)(end - p), &glen) < 0 ||
				(uint32)(end - p) < glen) {
			return PS_PARSE_FAIL;
		}
		/* If here, this is the cert that issued the OCSPResponse.  Will
			authenticate during validateOCSPResponse */
		if (getAsnSequence(&p, (uint32)(end - p), &glen) < 0) {
			psTraceCrypto("\n");
			return PS_PARSE_FAIL;
		}
		psAssert(glen == (end - p));
		/* reusing oid.  will handle multiple certs if needed */
		oid = psX509ParseCert(pool, p, glen, &res->OCSPResponseCert, 0);
		if (oid < 0) {
			psX509FreeCert(res->OCSPResponseCert);
			return PS_PARSE_FAIL;
		}
		p += oid;
	}
	psAssert(p == end);
	
	*cp = (unsigned char*)p;
	return PS_SUCCESS;
}

int32_t parseOCSPResponse(psPool_t *pool, int32_t len, unsigned char **cp,
				unsigned char *end,	mOCSPResponse_t *response)
{
	const unsigned char	*p;
	int32_t				status, oi;
	uint16_t			glen;
	uint32_t			blen;
	
	p = *cp;
	//psTraceBytes("OCSPResponse", p, len);
	/* 
		OCSPResponse ::= SEQUENCE {
			responseStatus          OCSPResponseStatus,
			responseBytes       [0] EXPLICIT ResponseBytes OPTIONAL }
	*/	
	if (getAsnSequence(&p, (uint32)(end - p), &glen) < 0) {
		psTraceCrypto("Initial parse error in parseOCSPResponse\n");
		return PS_PARSE_FAIL;
	}
	if (getAsnEnumerated(&p, (uint32)(end - p), &status) < 0) {
		psTraceCrypto("Enum parse error in parseOCSPResponse\n");
		return PS_PARSE_FAIL;
	}
	/*
		OCSPResponseStatus ::= ENUMERATED {
			successful          (0),  -- Response has valid confirmations
			malformedRequest    (1),  -- Illegal confirmation request
			internalError       (2),  -- Internal error in issuer
			tryLater            (3),  -- Try again later
                             -- (4) is not used
			sigRequired         (5),  -- Must sign the request
			unauthorized        (6)   -- Request unauthorized
		}
	*/
	if (status != 0) {
		/* Something other than success.  List right above here */
		psTraceCrypto("OCSPResponse contains no valid confirmations\n");
		return PS_PARSE_FAIL;
	}
	
	/* responseBytes       [0] EXPLICIT ResponseBytes OPTIONAL, */
	if (*p == (ASN_CONSTRUCTED | ASN_CONTEXT_SPECIFIC | 0)) {
		p++;
		if (getAsnLength32(&p, (uint32_t)(end - p), &blen, 0) < 0 ||
				(uint32_t)(end - p) < blen) {
			psTraceCrypto("Error parsing UserKeyingMaterial\n");
			return PS_PARSE_FAIL;
		}
	
		/* ResponseBytes ::= SEQUENCE {
			responseType            OBJECT IDENTIFIER,
			response                OCTET STRING }
		*/
		if (getAsnSequence(&p, (uint32)(end - p), &glen) < 0) {
			psTraceCrypto("ResponseBytes parse error in parseOCSPResponse\n");
			return PS_PARSE_FAIL;
		}
		if (getAsnOID(&p, (uint32)(end - p), &oi, 1, &glen) < 0) {
			psTraceCrypto("responseType parse error in parseOCSPResponse\n");
			return PS_PARSE_FAIL;
		}
		if ((*p++ != ASN_OCTET_STRING) ||
				getAsnLength32(&p, (int32)(end - p), &blen, 0) < 0 ||
				(uint32)(end - p) < blen) {

			psTraceCrypto("Couldn't parse response in parseOCSPResponse\n");
			return PS_PARSE_FAIL;
		}
		if (oi == 117) {
			/* id-pkix-ocsp-basic
			
				BasicOCSPResponse       ::= SEQUENCE {
					tbsResponseData      ResponseData,
					signatureAlgorithm   AlgorithmIdentifier,
					signature            BIT STRING,
					certs        [0] EXPLICIT SEQUENCE OF Certificate OPTIONAL }
			*/
			if (parseBasicOCSPResponse(pool, blen, &p, end, response) < 0) {
				psTraceCrypto("parseBasicOCSPResponse failure\n");
				return PS_PARSE_FAIL;
			}
		} else if (oi == 116) {
			/* id-pkix-ocsp */
			psTraceCrypto("unsupported id-pkix-ocsp in parseOCSPResponse\n");
			return PS_PARSE_FAIL;
		} else {
			psTraceCrypto("unsupported responseType in parseOCSPResponse\n");
			return PS_PARSE_FAIL;
		}
		
	}
	psAssert(end == p);
	*cp = (unsigned char*)p;
	return PS_SUCCESS;
}

/* Diff the current time against the OCSP timestamp and confirm it's not
	longer than the user is willing to trust */
static int16_t checkOCSPtimestamp(mOCSPResponse_t *response)
{
	/* GeneralizedTime values MUST be
		expressed in Greenwich Mean Time (Zulu) and MUST include seconds
		(i.e., times are YYYYMMDDHHMMSSZ), even where the number of seconds
		is zero.  GeneralizedTime values MUST NOT include fractional seconds.
	*/
#ifdef POSIX
	struct tm		mytime;
	time_t			currrawtime, myrawtime;
	double			diffsecs;
	const unsigned char	*c;
	
	/* timeProduced here is the producedAt of the response which is the
		time the signature was generated */
	c = response->timeProduced;
	
	/* Goal is to create time_t from this GeneralizedTime to perform a 
		diff against current time (also a time_t) */
	memset(&mytime, 0x0, sizeof(struct tm));
	mytime.tm_year = 1000 * (c[0] - '0') + 100 * (c[1] - '0') +
		10 * (c[2] - '0') + (c[3] - '0'); c += 4;
	mytime.tm_year -= 1900;
	mytime.tm_mon  = 10 * (c[0] - '0') + (c[1] - '0'); c += 2;
	mytime.tm_mon--; /* month is 0 based for some reason */
	mytime.tm_mday = 10 * (c[0] - '0') + (c[1] - '0'); c += 2;
	mytime.tm_hour = 10 * (c[0] - '0') + (c[1] - '0'); c += 2;
	mytime.tm_min = 10 * (c[0] - '0') + (c[1] - '0'); c += 2;
	mytime.tm_sec = 10 * (c[0] - '0') + (c[1] - '0');
	
	/* If there is no timegm on the platform, maybe there will be a mktime
		which will interpret 'mytime' as local time rather than GMT.  This
		change will cause a margin of error in the difftime below if
		the current time is not adjusted for GMT */
	//myrawtime = mktime(&mytime); /* margin of error will exist */
	myrawtime = timegm(&mytime); /* Adds missing members of mytime */
	
	/* current time */
	time(&currrawtime);

	diffsecs = difftime(currrawtime, myrawtime);
	
	if (diffsecs < 0 || diffsecs > OCSP_VALID_TIME_WINDOW) {
		return PS_LIMIT_FAIL;
	}
	return PS_SUCCESS;
	
#else
/* Warn if we are skipping the date validation checks. */
#ifdef WIN32
#pragma message("OCSP DATE VALIDITY NOT SUPPORTED ON THIS PLATFORM.")
#else
#warning "OCSP DATE VALIDITY NOT SUPPORTED ON THIS PLATFORM."
#endif
	return PS_SUCCESS;
#endif /* POSIX */
}

int32_t validateOCSPResponse(psPool_t *pool, psX509Cert_t *trustedOCSP,
			psX509Cert_t *srvCerts, mOCSPResponse_t *response)
{
	psX509Cert_t			*curr, *issuer, *subject, *ocspResIssuer;
	mOCSPSingleResponse_t	*subjectResponse;
	unsigned char			sigOut[MAX_HASH_SIZE];
	int32					sigOutLen, sigType, index;
	psPool_t				*pkiPool = NULL;
	
	/* Find the OCSP cert that signed the response.  First place to look is
		within the OCSPResponse itself */
	issuer = NULL;
	if (response->OCSPResponseCert) {
		/* If there is a cert here it is something that has to be authenticated.
			We will either leave this case with a successful auth or failure */
		curr = response->OCSPResponseCert;
		while (curr != NULL) {
			/* The outer responderKeyHash should be matching one of the certs
				that was attached to the OCSPResonse itself */
			if (memcmp(response->responderKeyHash, curr->sha1KeyHash, 20) == 0){
				/* Found it... but now we have to authenticate it against
					our known list of CAs.  issuer in the context of this
					function is the	OCSPResponse issuer but here we are looking
					for the	CA of THAT cert so it's 'subject' in this area */
				subject = curr;
				ocspResIssuer = trustedOCSP; /* preloaded sslKeys->CA */
				while (ocspResIssuer) {
					if (memcmp(ocspResIssuer->subject.hash,
							subject->issuer.hash, 20) == 0) {
						
						if (psX509AuthenticateCert(pool, subject, ocspResIssuer,
								&ocspResIssuer, NULL, NULL) == 0) {
							/* OK, we held the CA that issued the OCSPResponse
								so we'll now trust that cert that was provided
								in the OCSPResponse */
							ocspResIssuer = NULL;
							issuer = subject;
						} else {
							/* Auth failure */
							psTraceCrypto("Attached OCSP cert didn't auth\n");
							return PS_FAILURE;
						}
					} else {
						ocspResIssuer = ocspResIssuer->next;
					}
				}
				curr = NULL;
			} else {
				curr = curr->next;
			}
		}
		if (issuer == NULL) {
			psTraceCrypto("Found no CA to authenticate attached OCSP cert\n");
			return PS_FAILURE; /* no preloaded CA to auth cert in response */
		}
	}
	
	/* Issuer will be NULL if there was no certificate attached to the
		OCSP response.  Now look to the user loaded CA files */
	if (issuer == NULL) {
		curr = trustedOCSP;
		while (curr != NULL) {
			/* Currently looking for the subjectKey extension to match the
				public key hash from the response */
			if (memcmp(response->responderKeyHash, curr->sha1KeyHash, 20) == 0){
				issuer = curr;
				curr = NULL;
			} else {
				curr = curr->next;
			}
		}
	}
	
	/* It is possible a certificate embedded in the server certificate
			chain was itself the OCSP responder */
	if (issuer == NULL) {
		/* Don't look at the first cert in the chain because that is the
			one we are trying to find the OCSP responder public key for */
		curr = srvCerts->next;
		while (curr != NULL) {
			/* Currently looking for the subjectKey extension to match the
				public key hash from the response */
			if (memcmp(response->responderKeyHash, curr->sha1KeyHash, 20) == 0){
				issuer = curr;
				curr = NULL;
			} else {
				curr = curr->next;
			}
		}
	}
	
	if (issuer == NULL) {
		psTraceCrypto("Unable to locate OCSP responder CA for validation\n");
		return PS_FAILURE;
	}
	
	/* Now check to see that the response is vouching for the subject cert
		that we are interested in.  The subject will always be the first
		cert in the server CERTIFICATE chain */
	subject = srvCerts;
	
	/* Now look to match this cert within the singleResponse members.
	
		There are three components to a CertID that should be used to validate
		we are looking at the correct OCSP response for the subjecct cert.
		
		It appears the only "unique" portion of our subject cert that
		went into the signature of this response is the serial number.
		The "issuer" information of the subject cert also went into the
		signature but that isn't exactly unique.  Seems a bit odd that the
		combo of the issuer and the serial number are the only thing that tie
		this subject cert back to the response but serial numbers are the basis
		for CRL as well so it must be good enough */
	index = 0;
	while (index < MAX_OCSP_RESPONSES) {
		subjectResponse = &response->singleResponse[index];
		if ((subject->serialNumberLen == subjectResponse->certIdSerialLen) &&
				(memcmp(subject->serialNumber, subjectResponse->certIdSerial,
				subject->serialNumberLen) == 0)) {
			break; /* got it */
		}
		index++;
	}
	if (index == MAX_OCSP_RESPONSES) {
		psTraceCrypto("Unable to locate our subject cert in OCSP response\n");
		return PS_FAILURE;
	}
	
	/* If the response is reporting that this cert is bad right in the status,
		just return that immediately and stop the connection */
	if (subjectResponse->certStatus != 0) {
		psTraceCrypto("ERROR: OCSP info: server cert is revoked!\n");
		return PS_FAILURE;
	}
	
	/* Is the response within the acceptable time window */
	if (checkOCSPtimestamp(response) < 0) {
		psTraceCrypto("ERROR: OCSP response older than threshold\n");
		return PS_FAILURE;
	}
	
	/* Issuer portion of the validation - the subject cert issuer key and name
		hash should match what the subjectResponse reports
	
		POSSIBLE PROBLEMS:  Only supporting a SHA1 hash here.  The MatrixSSL
		parser will only use SHA1 for the DN and key hash. Just warning on
		this for now.  The signature validation will catch any key mismatch */
	if (subjectResponse->certIdHashAlg != OID_SHA1_ALG) {
		psTraceCrypto("WARNING: Non-SHA1 OCSP CertID. Issuer check bypassed\n");
	} else {
		if (memcmp(subjectResponse->certIdKeyHash, issuer->sha1KeyHash, 20)
				!= 0) {
			psTraceCrypto("Failed OCP issuer key hash validation\n");
			return PS_FAILURE;
		}
		/* Either subject->issuer or issuer->subject would work for testing */
		if (memcmp(subjectResponse->certIdNameHash, issuer->subject.hash, 20)
				!= 0){
			psTraceCrypto("Failed OCP issuer name hash validation\n");
			return PS_FAILURE;
		}
	}
	
	/* Finally do the sig validation */
	switch (response->sigAlg) {
#ifdef USE_SHA256
	case OID_SHA256_RSA_SIG:
		sigOutLen = SHA256_HASH_SIZE;
		sigType = PS_RSA;
		break;
	case OID_SHA256_ECDSA_SIG:
		sigOutLen = SHA256_HASH_SIZE;
		sigType = PS_ECC;
		break;
#endif
#ifdef USE_SHA384
	case OID_SHA384_RSA_SIG:
		sigOutLen = SHA384_HASH_SIZE;
		sigType = PS_RSA;
		break;
	case OID_SHA384_ECDSA_SIG:
		sigOutLen = SHA384_HASH_SIZE;
		sigType = PS_ECC;
		break;
#endif
	case OID_SHA1_RSA_SIG:
		sigOutLen = SHA1_HASH_SIZE;
		sigType = PS_RSA;
		break;
	case OID_SHA1_ECDSA_SIG:
		sigOutLen = SHA1_HASH_SIZE;
		sigType = PS_ECC;
		break;
	default:
		/* Should have been caught in parse phase */
		return PS_UNSUPPORTED_FAIL;
	}
	
	/* Finally test the signature */
	if (sigType == PS_RSA) {
		if (issuer->publicKey.type != PS_RSA) {
			return PS_FAILURE;
		}
		if (pubRsaDecryptSignedElement(pkiPool, &issuer->publicKey.key.rsa,
				(unsigned char*)response->sig, response->sigLen, sigOut,
				sigOutLen, NULL) < 0) {
			psTraceCrypto("Unable to decode signature in validateOCSPResponse\n");
			return PS_FAILURE;
		}
		if (memcmp(response->hashResult, sigOut, sigOutLen) != 0) {
			psTraceCrypto("OCSP RSA signature validation failed\n");
			return PS_FAILURE;
		}
	}
#ifdef USE_ECC
	else {
		if (issuer->publicKey.type != PS_ECC) {
			return PS_FAILURE;
		}
		/* ECDSA signature */
		index = 0;
		if (psEccDsaVerify(pkiPool, &issuer->publicKey.key.ecc,
				response->hashResult, sigOutLen, (unsigned char*)response->sig,
				response->sigLen, &index, NULL) < 0) {
			psTraceCrypto("ECC OCSP sig validation");
			return PS_FAILURE;
		}
		if (index != 1) {
			psTraceCrypto("OCSP ECDSA signature validation failed\n");
			return PS_FAILURE;
		}
	}
#endif
	

	/* Was able to successfully confirm OCSP signature for our subject */
	return PS_SUCCESS;
}
#endif /* USE_OCSP */

#endif /* USE_X509 */
/******************************************************************************/


