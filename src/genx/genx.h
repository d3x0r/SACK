
/*
 * genx - C-callable library for generating XML documents
 */

/*
 * Copyright (c) 2004 by Tim Bray and Sun Microsystems.  For copying
 *  permission, see http://www.tbray.org/ongoing/genx/COPYING
 */
#ifndef GENX_STUFF_DEFINED
#define GENX_STUFF_DEFINED

#include <stdio.h>

#include <sack_types.h> // CPROC

#ifdef GENX_SOURCE
#define GENX_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define GENX_PROC(type,name) IMPORT_METHOD type CPROC name
#endif



#ifdef __cplusplus
#define GENX_NAMESPACE namespace sack {  namespace xml { namespace gen {
#define GENX_NAMESPACE_END }}}
#else
#define GENX_NAMESPACE
#define GENX_NAMESPACE_END
#endif

GENX_NAMESPACE


/*
 * Note on error handling: genx routines mostly return
 *  GENX_SUCCESS (guaranteed to be zero) in normal circumstances, one of
 *  these other GENX_ values on a memory allocation or I/O failure or if the
 *  call would result in non-well-formed output.
 * You can associate an error message with one of these codes explicitly
 *  or with the most recent error using genxGetErrorMessage() and
 *  genxLastErrorMessage(); see below.
 */
typedef enum
{
  GENX_SUCCESS = 0,
  GENX_BAD_UTF8,
  GENX_NON_XML_CHARACTER,
  GENX_BAD_NAME,
  GENX_ALLOC_FAILED,
  GENX_BAD_NAMESPACE_NAME,
  GENX_INTERNAL_ERROR,
  GENX_DUPLICATE_PREFIX,
  GENX_SEQUENCE_ERROR,
  GENX_NO_START_TAG,
  GENX_IO_ERROR,
  GENX_MISSING_VALUE,
  GENX_MALFORMED_COMMENT,
  GENX_XML_PI_TARGET,
  GENX_MALFORMED_PI,
  GENX_DUPLICATE_ATTRIBUTE,
  GENX_ATTRIBUTE_IN_DEFAULT_NAMESPACE,
  GENX_DUPLICATE_NAMESPACE,
  GENX_BAD_DEFAULT_DECLARATION
} genxStatus;

/* character types */
#define GENX_XML_CHAR 1
#define GENX_LETTER 2
#define GENX_NAMECHAR 4

/* a UTF-8 string */
#if defined( __cplusplus )
#define UTFTYPE char
#else
#define UTFTYPE unsigned char
#endif
typedef UTFTYPE * utf8;
typedef const UTFTYPE * constUtf8;

/*
 * genx's own types
 */
typedef struct genxWriter_rec * genxWriter;
typedef struct genxNamespace_rec * genxNamespace;
typedef struct genxElement_rec * genxElement;
typedef struct genxAttribute_rec * genxAttribute;

/*
 * Constructors, set/get
 */

/*
 * Create a new writer.  For generating multiple XML documents, it's most
 *  efficient to re-use the same genx object.  However, you can only write
 *  one document at a time with a writer.
 * Returns NULL if it fails, which can only be due to an allocation failure.
 */
GENX_PROC(genxWriter, genxNew)(void * (*alloc)(void * userData, int bytes),
		   void (* dealloc)(void * userData, void * data),
		   void * userData);

/*
 * Dispose of a writer, freeing all associated memory
 */
GENX_PROC(void, genxDispose)(genxWriter w);

/*
 * Set/get
 */

/*
 * The userdata pointer will be passed to memory-allocation
 *  and I/O callbacks. If not set, genx will pass NULL
 */
GENX_PROC(void, genxSetUserData)(genxWriter w, void * userData);
GENX_PROC(void *, genxGetUserData)(genxWriter w);

/*
 * User-provided memory allocator, if desired.  For example, if you were
 *  in an Apache module, you could arrange for genx to use ap_palloc by
 *  making the pool accessible via the userData call.
 * The "dealloc" is to be used to free memory allocated with "alloc".  If
 *  alloc is provided but dealloc is NULL, genx will not attempt to free
 *  the memory; this would be appropriate in an Apache context.
 * If "alloc" is not provided, genx routines use malloc() to allocate memory
 */
GENX_PROC(void, genxSetAlloc)(genxWriter w,
		  void * (* alloc)(void * userData, int bytes));
GENX_PROC(void, genxSetDealloc)(genxWriter w,
		    void (* dealloc)(void * userData, void * data));
void * (* genxGetAlloc(genxWriter w))(void * userData, int bytes);
void (* genxGetDealloc(genxWriter w))(void * userData, void * data);

/*
 * Get the prefix associated with a namespace
 */
GENX_PROC(utf8, genxGetNamespacePrefix)(genxNamespace ns);

/*
 * Declaration functions
 */

/*
 * Declare a namespace.  The provided prefix is the default but can be
 *  overridden by genxAddNamespace.  If no default prefiix is provided,
 *  genx will generate one of the form g-%d.  
 * On error, returns NULL and signals via statusp
 */
GENX_PROC(genxNamespace, genxDeclareNamespace )(genxWriter w,
				   constUtf8 uri, constUtf8 prefix,
				   genxStatus * statusP);

/* 
 * Declare an element
 * If something failed, returns NULL and sets the status code via statusP
 */
GENX_PROC(genxElement, genxDeclareElement )(genxWriter w,
			       genxNamespace ns, constUtf8 type,
			       genxStatus * statusP);

/*
 * Declare an attribute
 */
GENX_PROC(genxAttribute, genxDeclareAttribute )(genxWriter w,
				   genxNamespace ns,
				   constUtf8 name, genxStatus * statusP);

/*
 * Writing XML
 */

/*
 * Start a new document.
 */
GENX_PROC(genxStatus, genxStartDocFile )(genxWriter w, FILE * file);

/*
 * Caller-provided I/O package.
 * First form is for a null-terminated string.
 * for second, if you have s="abcdef" and want to send "abc", you'd call
 *  sendBounded(userData, s, s + 3)
 */
typedef struct
{
  genxStatus (* send)(void * userData, constUtf8 s); 
  genxStatus (* sendBounded)(void * userData, constUtf8 start, constUtf8 end);
  genxStatus (* flush)(void * userData);
} genxSender;

GENX_PROC(genxStatus, genxStartDocSender )(genxWriter w, genxSender * sender);

/*
 * End a document.  Calls "flush"
 */
GENX_PROC(genxStatus, genxEndDocument )(genxWriter w);

/*
 * Write a comment
 */
GENX_PROC(genxStatus, genxComment )(genxWriter w, constUtf8 text);

/*
 * Write a PI
 */
GENX_PROC(genxStatus, genxPI )(genxWriter w, constUtf8 target, constUtf8 text);

/*
 * Start an element
 */
GENX_PROC(genxStatus, genxStartElementLiteral )(genxWriter w,
				   constUtf8 xmlns, constUtf8 type);

/*
 * Start a predeclared element
 * - element must have been declared
 */
GENX_PROC(genxStatus, genxStartElement )(genxElement e);

/*
 * Write an attribute
 */
GENX_PROC(genxStatus, genxAddAttributeLiteral )(genxWriter w, constUtf8 xmlns,
				   constUtf8 name, constUtf8 value);

/*
 * Write a predeclared attribute
 */
GENX_PROC(genxStatus, genxAddAttribute )(genxAttribute a, constUtf8 value);

/*
 * add a namespace declaration
 */
GENX_PROC(genxStatus, genxAddNamespace )(genxNamespace ns, utf8 prefix);

/*
 * Clear default namespace declaration
 */
GENX_PROC(genxStatus, genxUnsetDefaultNamespace )(genxWriter w);

/*
 * Write an end tag
 */
GENX_PROC(genxStatus, genxEndElement )(genxWriter w);

/*
 * Write some text
 * You can't write any text outside the root element, except with
 *  genxComment and genxPI
 */
GENX_PROC(genxStatus, genxAddText )(genxWriter w, constUtf8 start);
GENX_PROC(genxStatus, genxAddCountedText )(genxWriter w, constUtf8 start, int byteCount);
GENX_PROC(genxStatus, genxAddBoundedText )(genxWriter w, constUtf8 start, constUtf8 end);

/*
 * Write one character.  The integer value is the Unicode character
 *  value, as usually expressed in U+XXXX notation.
 */
GENX_PROC(genxStatus, genxAddCharacter )(genxWriter w, int c);

/*
 * Utility routines
 */

/*
 * Return the Unicode character encoded by the UTF-8 pointed-to by the
 *  argument, and advance the argument past the encoding of the character.
 * Returns -1 if the UTF-8 is malformed, in which case advances the
 *  argument to point at the first byte past the point past the malformed
 *  ones.
 */
GENX_PROC(int, genxNextUnicodeChar )(constUtf8 * sp);

/*
 * Scan a buffer allegedly full of UTF-8 encoded XML characters; return
 *  one of GENX_SUCCESS, GENX_BAD_UTF8, or GENX_NON_XML_CHARACTER
 */
GENX_PROC(genxStatus, genxCheckText )(genxWriter w, constUtf8 s);

/*
 * return character status, the OR of GENX_XML_CHAR,
 *  GENX_LETTER, and GENX_NAMECHAR
 */
GENX_PROC(int, genxCharClass )(genxWriter w, int c);

/*
 * Silently wipe any non-XML characters out of a chunk of text.
 * If you call this on a string before you pass it addText or
 *  addAttribute, you will never get an error from genx unless
 *  (a) there's a bug in your software, e.g. a malformed element name, or
 *  (b) there's a memory allocation or I/O error
 * The output can never be longer than the input.
 * Returns true if any changes were made.
 */
GENX_PROC(int, genxScrubText )(genxWriter w, constUtf8 in, utf8 out);

/*
 * return error messages
 */
GENX_PROC(const char *, genxGetErrorMessage )(genxWriter w, genxStatus status);
GENX_PROC(const char *, genxLastErrorMessage )(genxWriter w);

/*
 * return version
 */
GENX_PROC(const char *, genxGetVersion )();

GENX_NAMESPACE_END
#ifdef __cplusplus
	using namespace sack::xml::gen;
#endif

#endif

