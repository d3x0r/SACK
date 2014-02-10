#include "genx.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#if defined( WIN32 ) || defined( _WIN32 )
#define DEVNULL "NUL"
#define RANDOM rand
#define SRANDOM srand
#else
#define DEVNULL "/dev/null"
#define RANDOM random
#define SRANDOM srandom
#endif

static int errorcount = 0;

static void ouch2(genxWriter w, char * message)
{
  fprintf(stderr, WIDE("*** stress test error '%s': %s\n"), message,
	  genxLastErrorMessage(w));
  errorcount++;
}
	  

static void ouch(genxWriter  w, char * message, genxStatus code, genxStatus wanted)
{
  if (code == -1)
    fprintf(stderr, WIDE("*** %s\n"), message);
  else if (wanted == -1)
    fprintf(stderr,
	    "*** %s: got %s\n", message, genxGetErrorMessage(w, code));

  else
    fprintf(stderr,
	    "*** %s: got %s wanted %s\n", message,
	    genxGetErrorMessage(w, code),
	    genxGetErrorMessage(w, wanted));
  errorcount++;
}

typedef struct
{
  unsigned char buf[BUFSIZ];
  utf8 nowAt;
} iobuf_rec;

genxStatus sends(void * userData, constUtf8 s)
{
  iobuf_rec * io = (iobuf_rec *) userData;
  while (*s)
    *io->nowAt++ = *s++;
  *io->nowAt = 0;
  return GENX_SUCCESS;
}
genxStatus brokenSends(void * userData, constUtf8 s)
{
  return GENX_IO_ERROR;
}

genxStatus sendb(void * userData, constUtf8 start, constUtf8 end)
{
  iobuf_rec * io = (iobuf_rec *) userData;
  while (start < end)
    *io->nowAt++ = *start++;
  *io->nowAt = 0;
  return GENX_SUCCESS;
}
genxStatus sflush(void * userData)
{
  return GENX_SUCCESS;
}

iobuf_rec iobuf;
genxSender sender = { &sends, &sendb, &sflush };

static void checkUTF8()
{
  genxStatus ret;
  genxWriter w;


  // see http://www.tbray.org/ongoing/When/200x/2003/04/26/UTF
  // this is a legal UTF-8 4-char string
  unsigned char t1[] =
  {
    0x26,
    0xd0, 0x96,
    0xe4, 0xb8, 0xad,
    0xF0, 0x90, 0x8D, 0x86,
    0
  };
  int charsInT1[] = { 0x26, 0x416, 0x4e2d, 0x10346, 0 };

  unsigned char t2[] =
  {
    '<', 'a', '>', 1, 0
  };
  static unsigned char t3[] = { 0xc0, 0xaf, 0 };
  static unsigned char t4[] = { 0xc2, 0x7f, 0 };
  static unsigned char t5[] = { 0x80, 0x80, 0 };
  static unsigned char t6[] = { 0x80, 0x9f, 0 };
  static unsigned char t7[] = { 0xe1, 0x7f, 0 };
  static unsigned char t8[] = { 0xe1, 0x80, 0 };
  static unsigned char t9[] = { 0xe1, 0x80, 0x7f, 0 };
  static unsigned char * tBadUtf8[] =
  {
    t3, t4, t5, t6, t7, t8, t9, NULL
  };
  int i;
  utf8 s;

  tBadUtf8[0] = t3;
  tBadUtf8[1] = t4;
  tBadUtf8[2] = t5;
  tBadUtf8[3] = t6;
  tBadUtf8[4] = t7;
  tBadUtf8[5] = t8;
  tBadUtf8[6] = t9;
  tBadUtf8[7] = NULL;
  fprintf(stderr, WIDE("Testing genxCheckText\n"));
  w = genxNew(NULL, NULL, NULL);
  if (!w)
  {
    perror("genxNew");
    exit(1);
  }

  if ((ret = genxCheckText(w, t1)) != GENX_SUCCESS)
    ouch(w, WIDE("Error on string t1"), ret, GENX_SUCCESS);
  if ((ret = genxCheckText(w, t2)) != GENX_NON_XML_CHARACTER)
    ouch(w, WIDE("Error on string t2"), ret, GENX_NON_XML_CHARACTER);

  for (i = 0; tBadUtf8[i]; i++)
    if ((ret = genxCheckText(w, tBadUtf8[i])) != GENX_BAD_UTF8)
    {
      char msg[1024];
      sprintf(msg, WIDE("Error on BadUTF8 #%d"), i);
      ouch(w, msg, ret, GENX_BAD_UTF8);
    }

  s = t1;
  for (i = 0; i < 4; i++)
  {
    int c = genxNextUnicodeChar((constUtf8 *) &s);
    char msg[1024];

    if (c == charsInT1[i])
      continue;

    sprintf(msg, WIDE("t1[%d] got %d wanted %d"), i, c, charsInT1[i]);
    ouch(w, msg, -1, -1);
  }
}

static void checkScrub()
{
  genxWriter w;

  // see http://www.tbray.org/ongoing/When/200x/2003/04/26/UTF
  // this is a legal UTF-8 4-char string
  unsigned char t1[] =
  {
    0x26,
    0xd0, 0x96,
    0xe4, 0xb8, 0xad,
    0xF0, 0x90, 0x8D, 0x86,
    0
  };
  unsigned char t2[] =
  {
    '<', 'a', '>', 1, 0
  };
  static unsigned char t3[] = { 0xc0, 0xaf, 0 };
  static unsigned char t4[] = { 0xc2, 0x7f, 0 };
  static unsigned char t5[] = { 0x80, 0x80, 0 };
  static unsigned char t6[] = { 0x80, 0x9f, 0 };
  static unsigned char t7[] = { 0xe1, 0x7f, 0 };
  static unsigned char t8[] = { 0xe1, 0x80, 0 };
  static unsigned char t9[] = { 0xe1, 0x80, 0x7f, 0 };
  static unsigned char t10[] = { 0x80, 'a', 0 };
  static unsigned char t11[] = { 0x05, 'a', 0 };

  
  static unsigned char * tBadUtf8[] =
  {
    t3, t4, t5, t6, t7, t8, t9, t10, t11, NULL
  };
  unsigned char out[1024];
  int i;

  fprintf(stderr, WIDE("Testing genxScrubText\n"));
  w = genxNew(NULL, NULL, NULL);
  if (!w)
  {
    perror("genxNew");
    exit(1);
  }

  if (genxScrubText(w, t1, out) != GENX_SUCCESS ||
      strcmp(t1, out) != 0)
    ouch(w, WIDE("Error on string t1"), -1, -1);
  if (genxScrubText(w, t2, out) == 0 ||
      strlen(out) >= strlen(t2))
    ouch(w, WIDE("Error on string t2"), -1, -1);

  for (i = 0; tBadUtf8[i]; i++)
    if (genxScrubText(w, tBadUtf8[i], out) == 0 ||
	strlen(out) >= strlen(tBadUtf8[i]) ||
	genxCheckText(w, out) != GENX_SUCCESS)
    {
      char msg[1024];
      sprintf(msg, WIDE("Error on BadUTF8 #%d"), i);
      ouch(w, msg, -1, -1);
    }
}

void goodAttrVals(genxWriter w,
		  genxNamespace ns1, genxNamespace ns2,
		  genxElement ela, genxElement elb, genxElement elc,
		  genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  int status;

  // see http://www.tbray.org/ongoing/When/200x/2003/04/26/UTF
  // this is a legal UTF-8 4-char string
  unsigned char t1[] =
  {
    0x26,
    0xd0, 0x96,
    0xe4, 0xb8, 0xad,
    0xF0, 0x90, 0x8D, 0x86,
    0
  };
  unsigned char t2[] =
  {
    ' ', '<', ' ', '>', ' ', 0xd, ' ', '"', ' ', 0
  };

  fprintf(stderr, WIDE("Testing good Attribute values\n"));

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxStartElement(ela)) != GENX_SUCCESS)
    ouch(w, WIDE("startEl 1"), status, GENX_SUCCESS);
  if ((status = genxAddAttribute(a1, t1)) != GENX_SUCCESS)
    ouch(w, WIDE("add a1/t1"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("end el 1"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("end doc 1"), status, GENX_SUCCESS);

  iobuf.nowAt = iobuf.buf;
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);
  if ((status = genxStartElement(ela)) != GENX_SUCCESS)
    ouch(w, WIDE("startEl 3"), status, GENX_SUCCESS);
  if ((status = genxAddAttribute(a1, t2)) != GENX_SUCCESS)
    ouch(w, WIDE("add a1/t2"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("end el 2"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("end doc 2"), status, GENX_SUCCESS);

  if (strcmp(iobuf.buf, WIDE("<a a1=\") &lt; > &#xD; &quot; \"></a>"))
  {
    char msg[1024];
    sprintf(msg, WIDE("strcmp failed, got [%s]"), iobuf.buf);
    ouch(w, msg, -1, -1);
  }

}

void checkDeclareNS()
{
  genxStatus status;
  genxNamespace ns;
  genxWriter w;

  fprintf(stderr, WIDE("Testing genxDeclareNamespace\n"));
  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  ns = genxDeclareNamespace(w, WIDE("http://www.textuality.com/ns/"), NULL, &status);
  if (status != GENX_SUCCESS || ns == NULL)
    ouch(w, WIDE("Declare namespace no prefix"), status, GENX_SUCCESS);
    
  ns = genxDeclareNamespace(w, WIDE("http://www.textuality.com/ns/"), WIDE("foo"),
			    &status);
  if (status != GENX_SUCCESS || ns == NULL)
    ouch(w, WIDE("Declare dupe namespace"), status, GENX_SUCCESS);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew 2");
    exit(1);
  }

  ns = genxDeclareNamespace(w, NULL, NULL, &status);
  if (status != GENX_BAD_NAMESPACE_NAME || ns != NULL)
    ouch(w, WIDE("Declare NULL ns uri"), status, GENX_BAD_NAMESPACE_NAME);
    
  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew 3");
    exit(1);
  }

  ns = genxDeclareNamespace(w, WIDE(""), NULL, &status);
  if (status != GENX_BAD_NAMESPACE_NAME || ns != NULL)
    ouch(w, WIDE("Declare EMPTY ns uri"), status, GENX_BAD_NAMESPACE_NAME);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew 4");
    exit(1);
  }

  ns = genxDeclareNamespace(w, WIDE(""), NULL, &status);
  if (status != GENX_BAD_NAMESPACE_NAME || ns != NULL)
    ouch(w, WIDE("Declare EMPTY ns uri"), status, GENX_BAD_NAMESPACE_NAME);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew 5");
    exit(1);
  }

  ns = genxDeclareNamespace(w, WIDE("http://tbray.org/"), WIDE("foo"), &status);
  if (status != GENX_SUCCESS || ns == NULL)
    ouch(w, WIDE("Can't declare tbray.org=>foo"), status, GENX_SUCCESS);
  ns = genxDeclareNamespace(w, WIDE("http://tbray.org/"), WIDE("foo"), &status);
  if (status != GENX_SUCCESS || ns == NULL)
    ouch(w, WIDE("Can't declare tbray.org=>foo twice"), status, GENX_SUCCESS);
  ns = genxDeclareNamespace(w, WIDE("http://textuality.com/"), WIDE("foo"), &status);
  if (status != GENX_DUPLICATE_PREFIX || ns != NULL)
    ouch(w, WIDE("Accepted dupe prefix"), status, GENX_DUPLICATE_PREFIX);
  
  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew 6");
    exit(1);
  }

  ns = genxDeclareNamespace(w, WIDE("http://tbray.org/"), NULL, &status);
  if (status != GENX_SUCCESS || ns == NULL)
    ouch(w, WIDE("Can't declare tbray.org=>NULL"), status, GENX_SUCCESS);
  ns = genxDeclareNamespace(w, WIDE("http://foo.org/"), WIDE("g1"), &status);
  if (status != GENX_DUPLICATE_PREFIX || ns != NULL)
    ouch(w, WIDE("Accepted g-1 collision"), status, GENX_DUPLICATE_PREFIX);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew 6");
    exit(1);
  }
  ns = genxDeclareNamespace(w, WIDE("http://tbray.org/"), WIDE(""), &status);
  if (status != GENX_SUCCESS || ns == NULL)
    ouch(w, WIDE("Disallowed empty prefix"), status, GENX_SUCCESS);

  ns = genxDeclareNamespace(w, WIDE("http://tbray2.org/xyz"), WIDE(""), &status);
  if (status != GENX_DUPLICATE_PREFIX || ns != NULL)
    ouch(w, WIDE("Accepted dupe URI with empty prefix"), status,
	 GENX_DUPLICATE_PREFIX);
}

void checkDeclareEl()
{
  genxWriter w;
  genxElement el, el2;
  genxStatus status;
  genxNamespace ns1, ns2;

  fprintf(stderr, WIDE("Testing genxDeclareElement\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  el = genxDeclareElement(w, NULL, WIDE("a"), &status);
  if (status != GENX_SUCCESS || el == NULL)
    ouch(w, WIDE("Ordinary declare el"), status, GENX_SUCCESS);

  el = genxDeclareElement(w, NULL, WIDE("???"), &status);
  if (status != GENX_BAD_NAME || el != NULL)
    ouch(w, WIDE("Should catch bad name"), status, GENX_BAD_NAME);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew 2");
    exit(1);
  }

  el = genxDeclareElement(w, NULL, WIDE("a"), &status);
  if (status != GENX_SUCCESS || el == NULL)
    ouch(w, WIDE("Ordinary declare el (2)"), status, GENX_SUCCESS);

  el = genxDeclareElement(w, NULL, WIDE("a"), &status);
  if (status != GENX_SUCCESS || el == NULL)
    ouch(w, WIDE("Dupe declare el"), status, GENX_SUCCESS);

  ns1 = genxDeclareNamespace(w, WIDE("http://tbray.org/"), NULL, &status);
  ns2 = genxDeclareNamespace(w, WIDE("http://foo.org/"), WIDE("foo"), &status);

  el = genxDeclareElement(w, ns1, WIDE("x"), &status);
  if (status != GENX_SUCCESS || el == NULL)
    ouch(w, WIDE("Basic with ns 1"), status, GENX_SUCCESS);
  el2 = genxDeclareElement(w, ns2, WIDE("y"), &status);
  if (status != GENX_SUCCESS || el == NULL)
    ouch(w, WIDE("Basic with ns 2"), status, GENX_SUCCESS);
  el2 = genxDeclareElement(w, ns1, WIDE("x"), &status);
  if (status != GENX_SUCCESS || el == NULL)
    ouch(w, WIDE("Dupe with ns"), status, GENX_SUCCESS);
  if (el != el2)
    ouch(w, WIDE("Dupe made new el object?!?"), -1, -1);
}

void checkDeclareAttr()
{
  genxWriter w;
  genxAttribute a, a2;
  genxStatus status;
  genxNamespace ns1, ns2;

  fprintf(stderr, WIDE("Testing genxDeclareAttribute\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  a = genxDeclareAttribute(w, NULL, WIDE("a"), &status);
  if (status != GENX_SUCCESS || a == NULL)
    ouch(w, WIDE("Ordinary declare attr"), status, GENX_SUCCESS);

  a = genxDeclareAttribute(w, NULL, WIDE("^^^"), &status);
  if (status != GENX_BAD_NAME || a != NULL)
    ouch(w, WIDE("Should catch bad name"), status, GENX_BAD_NAME);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew 2");
    exit(1);
  }

  a = genxDeclareAttribute(w, NULL, WIDE("a"), &status);
  if (status != GENX_SUCCESS || a == NULL)
    ouch(w, WIDE("Ordinary declare attr (2)"), status, GENX_SUCCESS);

  a = genxDeclareAttribute(w, NULL, WIDE("a"), &status);
  if (status != GENX_SUCCESS || a == NULL)
    ouch(w, WIDE("Dupe declare attr"), status, GENX_SUCCESS);

  ns1 = genxDeclareNamespace(w, WIDE("http://tbray.org/"), NULL, &status);
  ns2 = genxDeclareNamespace(w, WIDE("http://foo.org/"), WIDE("foo"), &status);

  a = genxDeclareAttribute(w, ns1, WIDE("x"), &status);
  if (status != GENX_SUCCESS || a == NULL)
    ouch(w, WIDE("Basic with ns 1"), status, GENX_SUCCESS);
  a2 = genxDeclareAttribute(w, ns2, WIDE("y"), &status);
  if (status != GENX_SUCCESS || a == NULL)
    ouch(w, WIDE("Basic with ns 2"), status, GENX_SUCCESS);
  a2 = genxDeclareAttribute(w, ns1, WIDE("x"), &status);
  if (status != GENX_SUCCESS || a == NULL)
    ouch(w, WIDE("Dupe with ns"), status, GENX_SUCCESS);
  if (a != a2)
    ouch(w, WIDE("Dupe made new attr object?!?"), -1, -1);
}

void checkSeq1(genxWriter w,
	       genxNamespace ns1, genxNamespace ns2,
	       genxElement ela, genxElement elb, genxElement elc,
	       genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  FILE * f = fopen(DEVNULL, WIDE("w"));
  genxStatus status;

  /* 2 start-docs */
  if ((status = genxStartDocFile(w, f)) != GENX_SUCCESS)
    ouch(w, WIDE("seq1 startDoc"), status, GENX_SUCCESS);
  if ((status = genxStartDocFile(w, f)) != GENX_SEQUENCE_ERROR)
    ouch(w, WIDE("seq1 double startDoc"), status, GENX_SEQUENCE_ERROR);
  fclose(f);
}

void checkSeq2(genxWriter w,
	       genxNamespace ns1, genxNamespace ns2,
	       genxElement ela, genxElement elb, genxElement elc,
	       genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  FILE * f = fopen(DEVNULL, WIDE("w"));
  genxStatus status;

  /* missing startDoc */
  if ((status = genxStartElement(ela)) != GENX_SEQUENCE_ERROR)
    ouch(w, WIDE("seq1 startel no startdoc"), status, GENX_SEQUENCE_ERROR);
  fclose(f);
}

void checkSeq3(genxWriter w,
	       genxNamespace ns1, genxNamespace ns2,
	       genxElement ela, genxElement elb, genxElement elc,
	       genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  FILE * f = fopen(DEVNULL, WIDE("w"));
  genxStatus status;

  /* endel without start */
  if ((status = genxStartDocFile(w, f)) != GENX_SUCCESS)
    ouch(w, WIDE("seq1 startDoc"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SEQUENCE_ERROR)
    ouch(w, WIDE("seq1 bogus endel"), status, GENX_SEQUENCE_ERROR);
  fclose(f);
}

void checkSeq4(genxWriter w,
	       genxNamespace ns1, genxNamespace ns2,
	       genxElement ela, genxElement elb, genxElement elc,
	       genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  FILE * f = fopen(DEVNULL, WIDE("w"));
  genxStatus status;

  /* enddoc with stuff on stack */
  if ((status = genxStartDocFile(w, f)) != GENX_SUCCESS)
    ouch(w, WIDE("seq4 startDoc"), status, GENX_SUCCESS);
  if ((status = genxStartElement(ela)) != GENX_SUCCESS)
    ouch(w, WIDE("seq4 startEl"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SEQUENCE_ERROR)
    ouch(w, WIDE("premature end"), status, GENX_SEQUENCE_ERROR);
  fclose(f);
}

void checkSeq5(genxWriter w,
	       genxNamespace ns1, genxNamespace ns2,
	       genxElement ela, genxElement elb, genxElement elc,
	       genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  FILE * f = fopen(DEVNULL, WIDE("w"));
  genxStatus status;

  /* premature end-doc */
  if ((status = genxStartDocFile(w, f)) != GENX_SUCCESS)
    ouch(w, WIDE("seq5 startDoc"), status, GENX_SUCCESS);
  if ((status = genxStartElement(ela)) != GENX_SUCCESS)
    ouch(w, WIDE("seq5 startEl a"), status, GENX_SUCCESS);
  if ((status = genxStartElement(elb)) != GENX_SUCCESS)
    ouch(w, WIDE("seq5 startEl b"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("seq5 end el"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SEQUENCE_ERROR)
    ouch(w, WIDE("premature end"), status, GENX_SEQUENCE_ERROR);
  fclose(f);
}

void checkSeq6(genxWriter w,
	       genxNamespace ns1, genxNamespace ns2,
	       genxElement ela, genxElement elb, genxElement elc,
	       genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  FILE * f = fopen(DEVNULL, WIDE("w"));
  genxStatus status;

  /* addAttr not after startEl */
  if ((status = genxStartDocFile(w, f)) != GENX_SUCCESS)
    ouch(w, WIDE("seq5 startDoc"), status, GENX_SUCCESS);
  if ((status = genxAddAttribute(a1, WIDE("x"))) != GENX_SEQUENCE_ERROR)
    ouch(w, WIDE("bogus addattr"), status, GENX_SEQUENCE_ERROR);
  fclose(f);
}

void checkSeq7(genxWriter w,
	       genxNamespace ns1, genxNamespace ns2,
	       genxElement ela, genxElement elb, genxElement elc,
	       genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  FILE * f = fopen(DEVNULL, WIDE("w"));
  genxStatus status;

  /* addText w/o startEl */
  if ((status = genxStartDocFile(w, f)) != GENX_SUCCESS)
    ouch(w, WIDE("seq5 startDoc"), status, GENX_SUCCESS);
  if ((status = genxAddText(w, WIDE("x"))) != GENX_SEQUENCE_ERROR)
    ouch(w, WIDE("bogus addattr"), status, GENX_SEQUENCE_ERROR);
  fclose(f);
}

void checkSeq8(genxWriter w,
	       genxNamespace ns1, genxNamespace ns2,
	       genxElement ela, genxElement elb, genxElement elc,
	       genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  FILE * f = fopen(DEVNULL, WIDE("w"));
  genxStatus status;

  /* start comment without startdoc */
  if ((status = genxComment(w, WIDE("foo"))) != GENX_SEQUENCE_ERROR)
    ouch(w, WIDE("comment pre startdoc"), status, GENX_SEQUENCE_ERROR);
  fclose(f);
}

void checkNSDecls(genxWriter w,
	       genxNamespace ns1, genxNamespace ns2,
	       genxElement ela, genxElement elb, genxElement elc,
	       genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  genxStatus status;
  char * wanted = "<a><g1:b xmlns:g1=\"http://example.com/1\"></g1:b><a xmlns:g1=\"http://example.com/1\" g1:a3=\"x\"></a></a>";

  fprintf(stderr, WIDE("Testing namespace declarations\n"));
  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxStartElement(ela)) != GENX_SUCCESS)
    ouch(w, WIDE("StartEl ela"), status, GENX_SUCCESS);
  if ((status = genxStartElement(elb)) != GENX_SUCCESS)
    ouch(w, WIDE("StartEl elb"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("EndElb"), status, GENX_SUCCESS);
  if ((status = genxStartElement(ela)) != GENX_SUCCESS)
    ouch(w, WIDE("StartEl child ela"), status, GENX_SUCCESS);
  if ((status = genxAddAttribute(a3, WIDE("x"))) != GENX_SUCCESS)
    ouch(w, WIDE("AddAttr a3"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("EndEla child"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("EndEla root"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("enddoc"), status, GENX_SUCCESS);

  if (strcmp(iobuf.buf, wanted))
  {
    char msg[1024];
    sprintf(msg, WIDE("strcmp failed, got \n[%s], wanted \n[%s]\n"), iobuf.buf,
	    wanted);
    ouch(w, msg, -1, -1);
  }

  /*
   * see if it blows up, anyhwo
   */
  genxDispose(w);
}

void checkDupeAttr(genxWriter w,
		   genxNamespace ns1, genxNamespace ns2,
		   genxElement ela, genxElement elb, genxElement elc,
		   genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  genxStatus status;

  fprintf(stderr, WIDE("Testing duplicate attributes\n"));

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxStartElement(ela)) != GENX_SUCCESS)
    ouch(w, WIDE("AddEl ela"), status, GENX_SUCCESS);
  if ((status = genxAddAttribute(a1, WIDE("1"))) != GENX_SUCCESS)
    ouch(w, WIDE("AddAttr a1"), status, GENX_SUCCESS);
  if ((status = genxAddAttribute(a1, WIDE("1"))) != GENX_DUPLICATE_ATTRIBUTE)
    ouch(w, WIDE("Dupe attr"), status, GENX_DUPLICATE_ATTRIBUTE);
}

void checkAttrOrder(genxWriter w,
	       genxNamespace ns1, genxNamespace ns2,
	       genxElement ela, genxElement elb, genxElement elc,
	       genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  genxStatus status;
  utf8 wanted = "<a xmlns:a-ns2=\"http://example.com/2\" xmlns:g1=\"http://example.com/1\" a1=\"1\" g1:a3=\"3\" a-ns2:a2=\"2\"></a>";

  fprintf(stderr, WIDE("Testing attribute sorting\n"));
  
  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxStartElement(ela)) != GENX_SUCCESS)
    ouch(w, WIDE("AddEl ela"), status, GENX_SUCCESS);
  if ((status = genxAddAttribute(a1, WIDE("1"))) != GENX_SUCCESS)
    ouch(w, WIDE("AddAttr a1"), status, GENX_SUCCESS);
  if ((status = genxAddAttribute(a2, WIDE("2"))) != GENX_SUCCESS)
    ouch(w, WIDE("AddAttr a2"), status, GENX_SUCCESS);
  if ((status = genxAddAttribute(a3, WIDE("3"))) != GENX_SUCCESS)
    ouch(w, WIDE("AddAttr a3"), status, GENX_SUCCESS);

  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("EndEl"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("EndDoc"), status, GENX_SUCCESS);


  if (strcmp(iobuf.buf, wanted))
  {
    char msg[1024];
    sprintf(msg, WIDE("strcmp failed, got \n[%s], wanted\n[%s]"), iobuf.buf, wanted);
    ouch(w, msg, -1, -1);
  }
}
  

    
void checkWriting(void (*t)(genxWriter w,
			      genxNamespace, genxNamespace,
			      genxElement, genxElement, genxElement,
			      genxAttribute, genxAttribute, genxAttribute))
{
  genxWriter w;
  genxNamespace ns1, ns2;
  genxElement   ela, elb, elc;
  genxAttribute a1, a2, a3;
  genxStatus status;

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }
  ns1 = genxDeclareNamespace(w, WIDE("http://example.com/1"), NULL, &status);
  if (ns1 == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare ns1"), status, GENX_SUCCESS);
  ns2 = genxDeclareNamespace(w, WIDE("http://example.com/2"), WIDE("a-ns2"), &status);
  if (ns2 == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare ns2"), status, GENX_SUCCESS);

  ela = genxDeclareElement(w, NULL, WIDE("a"), &status);
  if (ela == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare ela"), status, GENX_SUCCESS);
  elb = genxDeclareElement(w, ns1, WIDE("b"), &status);
  if (elb == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare elb"), status, GENX_SUCCESS);
  elc = genxDeclareElement(w, ns2, WIDE("c"), &status);
  if (elc == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare elc"), status, GENX_SUCCESS);

  a1 = genxDeclareAttribute(w, NULL, WIDE("a1"), &status);
  if (a1 == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare a1"), status, GENX_SUCCESS);
  a2 = genxDeclareAttribute(w, ns2, WIDE("a2"), &status);
  if (a2 == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare a1"), status, GENX_SUCCESS);
  a3 = genxDeclareAttribute(w, ns1, WIDE("a3"), &status);
  if (a3 == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare a3"), status, GENX_SUCCESS);

  (*t)(w, ns1, ns2, ela, elb, elc, a1, a2, a3);
}

utf8 badVal;
genxStatus baWanted;

void checkBA(genxWriter w,
	     genxNamespace ns1, genxNamespace ns2,
	     genxElement ela, genxElement elb, genxElement elc,
	     genxAttribute a1, genxAttribute a2, genxAttribute a3)
{
  FILE * f = fopen(DEVNULL, WIDE("w"));
  genxStatus status;

  if ((status = genxStartDocFile(w, f)) != GENX_SUCCESS)
    ouch(w, WIDE("ba startDoc"), status, GENX_SUCCESS);
  if ((status = genxStartElement(ela)) != GENX_SUCCESS)
    ouch(w, WIDE("ba startEl"), status, GENX_SUCCESS);
  if ((status = genxAddAttribute(a1, badVal)) != baWanted)
    ouch(w, WIDE("ba addAttr"), status, baWanted);
  fclose(f);
}

void checkBadAttrVals()
{
  static unsigned char t2[] =
  {
    '<', 'a', '>', 1, 0
  };
  static unsigned char t3[] = { 0xc0, 0xaf, 0 };
  static unsigned char t4[] = { 0xc2, 0x7f, 0 };
  static unsigned char t5[] = { 0x80, 0x80, 0 };
  static unsigned char t6[] = { 0x80, 0x9f, 0 };
  static unsigned char t7[] = { 0xe1, 0x7f, 0 };
  static unsigned char t8[] = { 0xe1, 0x80, 0 };
  static unsigned char t9[] = { 0xe1, 0x80, 0x7f, 0 };
  
  static unsigned char * tBadUtf8[] =
  {
    t3, t4, t5, t6, t7, t8, t9, NULL
  };
  int i;

  fprintf(stderr, WIDE("Testing bad attribute values\n"));
  badVal = t2;
  baWanted = GENX_NON_XML_CHARACTER;
  checkWriting(&checkBA);

  baWanted = GENX_BAD_UTF8;
  for (i = 0; tBadUtf8[i]; i++)
  {
    badVal = tBadUtf8[i];
    checkWriting(&checkBA);
  }
}

void checkHello()
{
  genxElement greeting;
  genxStatus status;
  genxWriter w;

  fprintf(stderr, WIDE("Testing Hello world\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);
  if ((status = genxAddText(w, WIDE("Hello world!"))) != GENX_SUCCESS)
    ouch(w, WIDE("addText"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endElement"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endDoc"), status, GENX_SUCCESS);

  if (strcmp(iobuf.buf, WIDE("<greeting>Hello world!</greeting>")))
  {
    char msg[1024];
    sprintf(msg, WIDE("strcmp failed, got [%s]"), iobuf.buf);
    ouch(w, msg, -1, -1);
  }
}

void * bustedAlloc(void * userData, int bytes)
{
  return NULL;
}

void checkAllocator()
{
  genxElement greeting;
  genxStatus status;
  genxWriter w;

  fprintf(stderr, WIDE("Testing allocator\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  genxSetAlloc(w, &bustedAlloc);

  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting != NULL || status != GENX_ALLOC_FAILED)
    ouch(w, WIDE("expectd alloc_failed"), status, GENX_ALLOC_FAILED);
}

void checkHelloNS()
{
  genxElement greeting;
  genxStatus status;
  genxWriter w;
  genxNamespace ns;

  fprintf(stderr, WIDE("Testing Namespaced Hello world\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  ns = genxDeclareNamespace(w, WIDE("http://example.org/x"), WIDE("eg"), &status);
  if (ns == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("DeclareNS"), status, GENX_SUCCESS);
  greeting = genxDeclareElement(w, ns, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);
  if ((status = genxAddText(w, WIDE("Hello world!"))) != GENX_SUCCESS)
    ouch(w, WIDE("addText"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endElement"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endDoc"), status, GENX_SUCCESS);

  if (strcmp(iobuf.buf, WIDE("<eg:greeting xmlns:eg=\")http://example.org/x\">Hello world!</eg:greeting>"))
  {
    char msg[1024];
    sprintf(msg, WIDE("strcmp failed, got [%s]"), iobuf.buf);
    ouch(w, msg, -1, -1);
  }
}

void checkIOError()
{
  genxElement greeting;
  genxStatus status;
  genxWriter w;
  genxSender sender = { &brokenSends, &sendb, &sflush };

  fprintf(stderr, WIDE("Testing IO error\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("Start Element"), status, GENX_SUCCESS);
  if ((status = genxAddText(w, WIDE("x"))) != GENX_IO_ERROR)
    ouch(w, WIDE("AddText"), status, GENX_IO_ERROR);
}

void checkAddText()
{
  int i;
  genxElement greeting;
  genxStatus status;
  genxWriter w;
  unsigned char input[] =
  {
    '&',
    0xd0, 0x96,
    0xe4, 0xb8, 0xad,
    0xF0, 0x90, 0x8D, 0x86,
    0
  };

  unsigned char expected [] =
  {
    '&', 'a', 'm', 'p', ';',
    0xd0, 0x96,
    0xe4, 0xb8, 0xad,
    0xF0, 0x90, 0x8D, 0x86,
    0
  };
  unsigned char t2[] =
  {
    ' ', '<', ' ', '>', ' ', 0xd, ' ', '"', ' ', 0
  };

  fprintf(stderr, WIDE("Testing AddText\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);

  if ((status = genxAddText(w, input)) != GENX_SUCCESS)
    ouch(w, WIDE("addText"), status, GENX_SUCCESS);

  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endEl"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endDoc"), status, GENX_SUCCESS);

  for (i = 1; iobuf.buf[i] != '<'; i++)
    ;
  iobuf.buf[i] = 0;
  for (i = 0; iobuf.buf[i] != '>'; i++)
    ;
  i++;
  if (strcmp(iobuf.buf + i, expected))
    ouch(w, WIDE("incorrect UTF8"), -1, -1);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }
  genxSetUserData(w, &iobuf);
  iobuf.nowAt = iobuf.buf;
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);

  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);

  if ((status = genxAddText(w, t2)) != GENX_SUCCESS)
    ouch(w, WIDE("addText"), status, GENX_SUCCESS);

  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endEl"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endDoc"), status, GENX_SUCCESS);

  for (i = 1; iobuf.buf[i] != '<'; i++)
    ;
  iobuf.buf[i] = 0;
  for (i = 0; iobuf.buf[i] != '>'; i++)
    ;
  i++;
  if (strcmp(iobuf.buf + i, WIDE(" &lt; &gt; &#xD; \") "))
  {
    char msg[1024];
    sprintf(msg, WIDE("strcmp failed, got [%s]"), iobuf.buf + i);
    ouch(w, msg, -1, -1);
  }

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }
  genxSetUserData(w, &iobuf);
  iobuf.nowAt = iobuf.buf;
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);

  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);

  if ((status = genxAddBoundedText(w, input, input + 10)) != GENX_SUCCESS)
    ouch(w, WIDE("addText"), status, GENX_SUCCESS);

  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endEl"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endDoc"), status, GENX_SUCCESS);

  for (i = 1; iobuf.buf[i] != '<'; i++)
    ;
  iobuf.buf[i] = 0;
  for (i = 0; iobuf.buf[i] != '>'; i++)
    ;
  i++;
  if (strcmp(iobuf.buf + i, expected))
    ouch(w, WIDE("incorrect UTF8"), -1, -1);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }
  genxSetUserData(w, &iobuf);
  iobuf.nowAt = iobuf.buf;
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);

  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);

  if ((status = genxAddCountedText(w, input, 10)) != GENX_SUCCESS)
    ouch(w, WIDE("addText"), status, GENX_SUCCESS);

  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endEl"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endDoc"), status, GENX_SUCCESS);

  for (i = 1; iobuf.buf[i] != '<'; i++)
    ;
  iobuf.buf[i] = 0;
  for (i = 0; iobuf.buf[i] != '>'; i++)
    ;
  i++;
  if (strcmp(iobuf.buf + i, expected))
    ouch(w, WIDE("incorrect UTF8"), -1, -1);


}

void checkAddChar()
{
  int i;
  genxElement greeting;
  genxStatus status;
  genxWriter w;
  unsigned char expected [] =
  {
    '&', 'a', 'm', 'p', ';',
    0xd0, 0x96,
    0xe4, 0xb8, 0xad,
    0xF0, 0x90, 0x8D, 0x86,
    0
  };
  int input[] = { 0x26, 0x416, 0x4e2d, 0x10346, 0 };

  fprintf(stderr, WIDE("Testing AddChar\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);

  for (i = 0; input[i]; i++)
    if ((status = genxAddCharacter(w, input[i])) != GENX_SUCCESS)
      ouch(w, WIDE("addchar"), status, GENX_SUCCESS);

  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endEl"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endDoc"), status, GENX_SUCCESS);

  /* find the string in the output */
  for (i = 1; iobuf.buf[i] != '<'; i++)
    ;
  iobuf.buf[i] = 0;
  for (i = 0; iobuf.buf[i] != '>'; i++)
    ;
  i++;
  if (strcmp(iobuf.buf + i, expected))
    ouch(w, WIDE("incorrect UTF8"), -1, -1);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }
  genxSetUserData(w, &iobuf);
  iobuf.nowAt = iobuf.buf;
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);
  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);
  if ((status = genxAddCharacter(w, -5)) != GENX_NON_XML_CHARACTER)
    ouch(w, WIDE("Add -5"), status, GENX_NON_XML_CHARACTER);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }
  genxSetUserData(w, &iobuf);
  iobuf.nowAt = iobuf.buf;
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);
  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);
  if ((status = genxAddCharacter(w, 1)) != GENX_NON_XML_CHARACTER)
    ouch(w, WIDE("Add -5"), status, GENX_NON_XML_CHARACTER);
}

void checkComment()
{
  genxWriter w;
  genxElement greeting;
  genxStatus status;

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  fprintf(stderr, WIDE("Testing comments\n"));
  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxComment(w, WIDE("1"))) != GENX_SUCCESS)
    ouch(w, WIDE("Comment 1"), status, GENX_SUCCESS);
  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);
  if ((status = genxComment(w, WIDE("2"))) != GENX_SUCCESS)
    ouch(w, WIDE("Comment 2"), status, GENX_SUCCESS);
  if ((status = genxAddText(w, WIDE("[1]"))) != GENX_SUCCESS)
    ouch(w, WIDE("Addtext 1"), status, GENX_SUCCESS);
  if ((status = genxComment(w, WIDE("3"))) != GENX_SUCCESS)
    ouch(w, WIDE("Comment 3"), status, GENX_SUCCESS);
  if ((status = genxAddText(w, WIDE("[2]"))) != GENX_SUCCESS)
    ouch(w, WIDE("Addtext 2"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endel"), status, GENX_SUCCESS);
  if ((status = genxComment(w, WIDE("4"))) != GENX_SUCCESS)
    ouch(w, WIDE("Comment 3"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endDoc"), status, GENX_SUCCESS);

  if (strcmp(iobuf.buf, WIDE("<!--1-->\n<greeting><!--2-->[1]<!--3-->[2]</greeting>\n<!--4-->")))
  {
    char msg[1024];
    sprintf(msg, WIDE("strcmp failed, got [%s]"), iobuf.buf);
    ouch(w, msg, -1, -1);
  }

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender 2"), status, GENX_SUCCESS);

  if ((status = genxComment(w, WIDE("-foo"))) != GENX_MALFORMED_COMMENT)
    ouch(w, WIDE("missed leading -"), status, GENX_MALFORMED_COMMENT);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender 3"), status, GENX_SUCCESS);

  if ((status = genxComment(w, WIDE("foo-"))) != GENX_MALFORMED_COMMENT)
    ouch(w, WIDE("missed trailing -"), status, GENX_MALFORMED_COMMENT);

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender 3"), status, GENX_SUCCESS);

  if ((status = genxComment(w, WIDE("foo--bar"))) != GENX_MALFORMED_COMMENT)
    ouch(w, WIDE("missed --"), status, GENX_MALFORMED_COMMENT);
}

void checkPI()
{
  genxWriter w;
  genxElement greeting;
  genxStatus status;

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  fprintf(stderr, WIDE("Testing PIs\n"));
  greeting = genxDeclareElement(w, NULL, WIDE("greeting"), &status);
  if (greeting == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare greeting"), status, GENX_SUCCESS);

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxPI(w, WIDE("pi1"), WIDE("1"))) != GENX_SUCCESS)
    ouch(w, WIDE("PI 1"), status, GENX_SUCCESS);
  if ((status = genxStartElement(greeting)) != GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);
  if ((status = genxPI(w, WIDE("pi2"), WIDE("2"))) != GENX_SUCCESS)
    ouch(w, WIDE("PI 2"), status, GENX_SUCCESS);
  if ((status = genxAddText(w, WIDE("[1]"))) != GENX_SUCCESS)
    ouch(w, WIDE("Addtext 1"), status, GENX_SUCCESS);
  if ((status = genxPI(w, WIDE("pi3"), WIDE("3"))) != GENX_SUCCESS)
    ouch(w, WIDE("PI 3"), status, GENX_SUCCESS);
  if ((status = genxAddText(w, WIDE("[2]"))) != GENX_SUCCESS)
    ouch(w, WIDE("Addtext 2"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endel"), status, GENX_SUCCESS);
  if ((status = genxPI(w, WIDE("pi4"), WIDE("4"))) != GENX_SUCCESS)
    ouch(w, WIDE("PI 4"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endDoc"), status, GENX_SUCCESS);

  /*
  if (strcmp(iobuf.buf, WIDE("<!--1-->\n<greeting><!--2-->[1]<!--3-->[2]</greeting>\n<!--4-->")))
  {
    char msg[1024];
    sprintf(msg, WIDE("strcmp failed, got [%s]"), iobuf.buf);
    ouch(w, msg, -1, -1);
  }
  */

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender 3"), status, GENX_SUCCESS);

  if ((status = genxPI(w, WIDE("pi5"), WIDE("foo?>bar"))) != GENX_MALFORMED_PI)
    ouch(w, WIDE("missed ?>"), status, GENX_MALFORMED_PI);
}

void checkHelloLiteral()
{
  genxWriter w;
  genxStatus status;
  genxNamespace ns;


  fprintf(stderr, WIDE("Testing Hello world (Literal)\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  ns = genxDeclareNamespace(w, WIDE("foo:bar"), WIDE("baz"), &status);
  if (ns == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("Declare namespace"), status, GENX_SUCCESS);

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  if ((status = genxStartElementLiteral(w, WIDE("foo:bar"), WIDE("greeting"))) !=
      GENX_SUCCESS)
    ouch(w, WIDE("StartElement"), status, GENX_SUCCESS);
  if ((status = genxAddText(w, WIDE("Hello world!"))) != GENX_SUCCESS)
    ouch(w, WIDE("addText"), status, GENX_SUCCESS);
  if ((status = genxEndElement(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endElement"), status, GENX_SUCCESS);
  if ((status = genxEndDocument(w)) != GENX_SUCCESS)
    ouch(w, WIDE("endDoc"), status, GENX_SUCCESS);

  if (strcmp(iobuf.buf, WIDE("<baz:greeting xmlns:baz=\")foo:bar\">Hello world!</baz:greeting>"))
  {
    char msg[1024];
    sprintf(msg, WIDE("strcmp failed, got [%s]"), iobuf.buf);
    ouch(w, msg, -1, -1);
  }
}

void checkStress()
{
  genxWriter w;
  unsigned char ename[100];
  unsigned char aname[100];
  unsigned char nname[100];
  int acount;
  int alength;
  char avchar;
  unsigned char aval[10000];
  genxStatus status;
  int i, j, k;
  
  fprintf(stderr, WIDE("Testing memory management\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  status = genxStartDocFile(w, stdout);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocFile"), status, GENX_SUCCESS);

  if (genxStartElementLiteral(w, NULL, WIDE("root")))
    ouch2(w, WIDE("root"));
  SRANDOM(581213);
  for (i = 1; i < 100; i++)
  {
    if ((i % 3) == 0)
      if (genxEndElement(w))
	ouch2(w, WIDE("end el"));

    /*
    if ((i % 200) == 0)
      fprintf(stderr, WIDE("   %d elements\n"), i);
    */
    sprintf(nname, WIDE("n%d"), (int) (RANDOM() % 10000));
    sprintf(ename, WIDE("e%d"), (int) (RANDOM() % 10000));
    if (genxStartElementLiteral(w, nname, ename))
      ouch2(w, WIDE("start el"));
    acount = RANDOM() % 20;
    for (j = 0; j < acount; j++)
    {
      alength = RANDOM() % 10000;
      avchar = 'A' + RANDOM() % 40;
      for (k = 0; k < alength; k++)
	aval[k] = avchar;
      aval[k] = 0;
      sprintf(nname, WIDE("n%d"), (int) (RANDOM() % 10000));
      sprintf(aname, WIDE("a%d"), (int) (RANDOM() % 10000));
      if ((status = genxAddAttributeLiteral(w, nname, aname, aval)))
      {
	if (status != GENX_DUPLICATE_ATTRIBUTE)
	  ouch2(w, WIDE("add attr"));
      }
    }
    if (genxAddText(w, WIDE("\n")))
      ouch2(w, WIDE("add text"));
  }
  while ((status = genxEndElement(w)) == GENX_SUCCESS)
    ;
  if (status != GENX_SEQUENCE_ERROR)
    ouch2(w, WIDE("unwind"));
  if (genxEndDocument(w))
    ouch2(w, WIDE("end doc"));
}

void checkNCNames()
{
  genxStatus status;
  genxElement e;
  genxAttribute a;
  genxNamespace n;
  genxWriter w;

  fprintf(stderr, WIDE("Testing colon suppression\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  if ((status = genxStartDocFile(w, stdout)))
    ouch(w, WIDE("start doc"), status, GENX_SUCCESS);

  n = genxDeclareNamespace(w, WIDE("http://somewhere/"), WIDE("foo:bar"), &status);
  if (n != NULL || status != GENX_BAD_NAME)
    ouch(w, WIDE("declare ns with colon"), status, GENX_BAD_NAME);
  e = genxDeclareElement(w, NULL, WIDE("foo:bar"), &status);
  if (e != NULL || status != GENX_BAD_NAME)
    ouch(w, WIDE("declare e with colon"), status, GENX_BAD_NAME);
  a = genxDeclareAttribute(w, NULL, WIDE("foo:bar"), &status);
  if (a != NULL || status != GENX_BAD_NAME)
    ouch(w, WIDE("declare e with colon"), status, GENX_BAD_NAME);
}

void checkDefaultNS()
{
  genxStatus status;
  genxAttribute a;
  genxNamespace nDefault, nPrefix, nBad;
  genxWriter w;
  genxElement eNaked, eDefault, ePrefix;
  utf8 wanted;

  fprintf(stderr, WIDE("Testing namespace defaulting\n"));

  if ((w = genxNew(NULL, NULL, NULL)) == NULL)
  {
    perror("genxNew");
    exit(1);
  }

  iobuf.nowAt = iobuf.buf;
  genxSetUserData(w, &iobuf);
  status = genxStartDocSender(w, &sender);
  if (status != GENX_SUCCESS)
    ouch(w, WIDE("startDocSender"), status, GENX_SUCCESS);

  nDefault = genxDeclareNamespace(w, WIDE("http://def"), WIDE(""), &status);
  if (nDefault == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare nDefault"), status, GENX_SUCCESS);
  nPrefix = genxDeclareNamespace(w, WIDE("http://pref"), WIDE("pref"), &status);
  if (nPrefix == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare nPrefix"), status, GENX_SUCCESS);

  a = genxDeclareAttribute(w, nDefault, WIDE("foo"), &status);
  if (a != NULL || status != GENX_ATTRIBUTE_IN_DEFAULT_NAMESPACE)
    ouch(w, WIDE("Allowed declaration of attr in default namespace"), status,
	 GENX_ATTRIBUTE_IN_DEFAULT_NAMESPACE);

  eNaked = genxDeclareElement(w, NULL, WIDE("e"), &status);
  if (eNaked == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare eNaked"), status, GENX_SUCCESS);
  eDefault = genxDeclareElement(w, nDefault, WIDE("eD"), &status);
  if (eDefault == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare eDefault"), status, GENX_SUCCESS);
  ePrefix = genxDeclareElement(w, nPrefix, WIDE("eP"), &status);
  if (ePrefix == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("declare ePrefix"), status, GENX_SUCCESS);

  if (genxStartElement(eDefault))
    ouch2(w, WIDE("start eDefault"));
  if (genxAddNamespace(nPrefix, NULL))
    ouch2(w, WIDE("add nPrefix"));
  if (genxStartElement(eNaked))
    ouch2(w, WIDE("start naked"));

  nBad = genxDeclareNamespace(w, WIDE("http://pref"), WIDE("foobar"), &status);
  if (nBad == NULL || status != GENX_SUCCESS)
    ouch(w, WIDE("bogus redeclare"), status, GENX_SUCCESS);

  if (genxEndElement(w))
    ouch2(w, WIDE("end naked"));
  if (genxStartElement(ePrefix))
    ouch2(w, WIDE("inner prefix"));
  if (genxUnsetDefaultNamespace(w))
    ouch2(w, WIDE("unset default"));
  if (genxStartElement(eNaked))
    ouch2(w, WIDE("inner naked"));
  if (genxEndElement(w))
    ouch2(w, WIDE("end inner naked"));
  if (genxStartElement(eDefault))
    ouch2(w, WIDE("inner default"));
    
  while ((status = genxEndElement(w)) == GENX_SUCCESS)
    ;
  if (status != GENX_SEQUENCE_ERROR)
    ouch2(w, WIDE("unwind"));
  if (genxEndDocument(w))
    ouch2(w, WIDE("end doc"));

  wanted = "<eD xmlns=\"http://def\" xmlns:pref=\"http://pref\"><e xmlns=\"\"></e><foobar:eP xmlns=\"\" xmlns:foobar=\"http://pref\"><e></e><eD xmlns=\"http://def\"></eD></foobar:eP></eD>";
  if (strcmp(iobuf.buf, wanted))
  {
    char msg[1024];
    sprintf(msg, WIDE("strcmp failed, got \n[%s] wanted \n[%s]"), iobuf.buf, wanted);
    ouch(w, msg, -1, -1);
  }
}
 
/*
 * genx test driver
 */
int main(int argc, char * argv[])
{
  checkUTF8();
  checkScrub();

  checkDeclareNS();
  checkDefaultNS();

  checkDeclareEl();
  checkDeclareAttr();

  checkNCNames();

  fprintf(stderr, WIDE("Testing Sequencing\n"));
  checkWriting(&checkSeq1);
  checkWriting(&checkSeq2);
  checkWriting(&checkSeq3);
  checkWriting(&checkSeq4);
  checkWriting(&checkSeq5);
  checkWriting(&checkSeq6);
  checkWriting(&checkSeq7);
  checkWriting(&checkSeq8);

  checkHello();
  checkHelloNS();

  checkWriting(&checkAttrOrder);
  checkWriting(&checkDupeAttr);

  checkIOError();

  checkWriting(&checkNSDecls);

  checkAllocator();

  checkWriting(&goodAttrVals);
  
  checkBadAttrVals();

  checkAddChar();
  checkAddText();

  checkComment();
  checkPI();

  checkHelloLiteral();
  checkStress();

  fprintf(stderr, WIDE("FAILED TESTS: %d\n"), errorcount);
  exit(errorcount);
}

