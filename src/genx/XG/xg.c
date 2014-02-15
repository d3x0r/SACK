/* $Id: xg.c,v 1.1 2005/06/02 19:12:50 jim Exp $ */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <expat.h>
#include <genxml/genx.h>
#include "xg.h"

static int srcpi=1;
static char *xml;
static XML_Parser expat=NULL;
static genxWriter genx=NULL;
static int lastline,lastcol;
static char srcposbuf[22]; 
static char *prevxml;
static int ok;
static utf8 nsbuf;
static int nsbufInd = 0;
static int nsbufSize = 1024;
static int *savedNs;
static int savedNsInd = 0;
static int savedNsSize = 20;
static int inDTD = 0;

static void verror_handler(int erno,va_list ap) {
  int line=XML_GetCurrentLineNumber(expat),
    col=XML_GetCurrentColumnNumber(expat);
  ok=0;
  if(line!=lastline||col!=lastcol) {
    fprintf(stderr,"error (%s,%i,%i): ",xml,lastline=line,lastcol=col);
    xg_perror(erno,ap);
  }
}

static void error_handler(int erno,...) {
  va_list ap; va_start(ap,erno); verror_handler(erno,ap); va_end(ap);
}

static int popNs(utf8 *uriP,utf8 *pfxP) {
  int i;
  if(savedNsInd==0) return 0;
  if((i=savedNs[--savedNsInd])==-1) *pfxP=NULL;
  else *pfxP=nsbuf+i;
  if((i=savedNs[--savedNsInd])==-1) *uriP=NULL;
  else *uriP=nsbuf+i;
  return 1;
}

static void pushI(int i) {
  if (savedNsInd==savedNsSize) {
    int *newbuf;
    if(!(newbuf=(int *)realloc(savedNs,savedNsSize=savedNsSize*2)))
      error_handler(ER_MALLOC);
    savedNs=newbuf;
  }
  savedNs[savedNsInd++]=i;
}

static int storeT(utf8 t) {
  int i=0;
  int at=nsbufInd;
  do {
    if(nsbufInd==nsbufSize) {
      utf8 newbuf;
      if (!(newbuf=(utf8)realloc(nsbuf,nsbufSize=nsbufSize*2)))
	error_handler(ER_MALLOC);
      nsbuf=newbuf;
    }
    nsbuf[nsbufInd++]=*t;
  } while(*t++);
  return at;
}
  
static void pushNs(utf8 uri,utf8 pfx) {
  if(!uri) pushI(-1);
  else pushI(storeT(uri));
  if(!pfx) pushI(-1);
  else pushI(storeT(pfx));
}

static void insrc() {
  if(srcpi) {
    if(prevxml!=xml) {
      !genxPI(genx,PIFILE,xml)
        || (error_handler(ER_GENX,"genxPI",genxLastErrorMessage(genx)),1);
      prevxml=xml;
    }
    sprintf(srcposbuf,"%u %u",
      XML_GetCurrentLineNumber(expat),
      XML_GetCurrentColumnNumber(expat));
    !genxPI(genx,PIPOS,srcposbuf)
      || (error_handler(ER_GENX,"genxPI",genxLastErrorMessage(genx)),1);
  }
}

static void start_dtd(void *userData, const char *dtn, const char *sys,
    const char *pub, int his) {
  inDTD = 1;
}
static void end_dtd(void *userData) {
  inDTD = 0;
}

static void start_element(void *userData,
    const char *name,const char **attrs) {
  char *sep=strrchr(name,':'),*xmlns;
  utf8 uri,pfx;
  insrc();
  if(sep) {*sep='\0'; xmlns=(char*)name; name=sep+1;} else xmlns=NULL;
  !genxStartElementLiteral(genx,xmlns,(char*)name)
    || (error_handler(ER_GENX,"genxStartElementLiteral",
	genxLastErrorMessage(genx)),1);

  while(popNs(&uri, &pfx)) {
    if(!uri) {
      if(genxUnsetDefaultNamespace(genx))
	error_handler(ER_GENX,"genxUnsetDefaultNamespace");
    }
    else {
      genxStatus status;
      genxNamespace ns;

      ns=genxDeclareNamespace(genx,uri,(pfx)?pfx:(utf8)"",&status);

      /* someone else might have grabbed this default */
      if (status==GENX_DUPLICATE_PREFIX)
	ns=genxDeclareNamespace(genx,uri,NULL,&status);
      if (!ns)
	error_handler(ER_GENX,"genxDeclareNamespace",
		      genxLastErrorMessage(genx));
      if (genxAddNamespace(ns,pfx?(utf8)pfx:(utf8)""))
	error_handler(ER_GENX,"genxAddNamespace", genxLastErrorMessage(genx));
    }
  }
  nsbufInd=savedNsInd=0;

  if(sep) *sep=':';
  while(*attrs) {
    sep=strrchr(*attrs,':');
    if(sep) {*sep='\0'; xmlns=(char*)*attrs; *attrs=sep+1;} 
    else xmlns=NULL;
    !genxAddAttributeLiteral(genx,xmlns,(char*)*attrs,(char*)*(attrs+1))
      || (error_handler(ER_GENX,"genxAddAttributeLiteral",
	  genxLastErrorMessage(genx)),1);
    if(sep) *sep=':';
    attrs+=2;
  }
}

static void end_element(void *userData,const char *name) {
  insrc();
  !genxEndElement(genx)
    || (error_handler(ER_GENX,"genxEndElement",
        genxLastErrorMessage(genx)),1);
}

static void characters(void *userData,const char *s,int len) {
  !genxAddCountedText(genx,(char*)s,len)
    || (error_handler(ER_GENX,"genxAddCountedText",
        genxLastErrorMessage(genx)),1);
}

static void startNS(void *userData,const char *pfx,const char *uri) {
  pushNs((utf8)uri,(utf8)pfx);
}

static void endNS(void *userData,const char *pfx) {
}

static void processingInstruction(void *userData,
    const char *target,const char *data) {
  if (inDTD)
    return;
  !genxPI(genx,(char*)target,(char*)data)
    || (error_handler(ER_GENX,"genxPI",genxLastErrorMessage(genx)),1);
}

static void comment(void *userData,const char *data) {
  if (inDTD)
    return;
  !genxComment(genx,(char*)data)
    || (error_handler(ER_GENX,"genxComment",genxLastErrorMessage(genx)),1);
}

static int process(FILE *fp) {
  void *buf; int len;
  for(;;) {
    buf=XML_GetBuffer(expat,BUFSIZ);
    len=fread(buf,1,BUFSIZ,fp);
    if(len!=BUFSIZ && ferror(fp)) {
      error_handler(ER_IO,xml,strerror(errno));
      goto ERROR;
    }
    if(!XML_ParseBuffer(expat,len,len==0)) goto PARSE_ERROR;
    if(len==0) break;
  }
  return 1;

PARSE_ERROR:
  error_handler(ER_XML,XML_ErrorString(XML_GetErrorCode(expat)));
ERROR:
  return 0;
}

static int externalEntityRef(XML_Parser p,
    const char *context, const char *base,
    const char *systemId, const char *publicId) {
  if(systemId) {
    FILE *fp; char *entity;
    assert(base);
    entity=(char*)calloc(strlen(base)+strlen(systemId)+2,1);
    strcpy(entity,systemId); xg_abspath(entity,(char*)base);
    if((fp=fopen(entity,"r"))) {
      XML_Parser expat0=expat; char *xml0=xml; xml=entity;
      expat=XML_ExternalEntityParserCreate(expat0,context,NULL);
      ok=process(fp);
      XML_ParserFree(expat);
      xml=xml0; expat=expat0;
      fclose(fp);
    } else {
      error_handler(ER_NOXENT,entity,strerror(errno));
    }
    free(entity);
  } else {
    error_handler(ER_NOSID);
  }
  return 1;
}

static void c14nize(FILE *fp) {
  expat=XML_ParserCreateNS(NULL,':');
  genx=genxNew(NULL,NULL,NULL);
  XML_SetElementHandler(expat,&start_element,&end_element);
  XML_SetCharacterDataHandler(expat,&characters);
  XML_SetNamespaceDeclHandler(expat,&startNS,&endNS);
  XML_SetExternalEntityRefHandler(expat,&externalEntityRef);
  XML_SetProcessingInstructionHandler(expat,&processingInstruction);
  XML_SetCommentHandler(expat,&comment);
  XML_SetStartDoctypeDeclHandler(expat,&start_dtd);
  XML_SetEndDoctypeDeclHandler(expat,&end_dtd);
  XML_SetBase(expat,xml);
  prevxml=NULL;
  genxStartDocFile(genx,stdout);
  ok=process(fp);
  genxEndDocument(genx);
  XML_ParserFree(expat);
  genxDispose(genx);
}

static void usage(void) {fprintf(stderr,"usage: xg [-Ph] [input.xml]\n");}
int main(int argc,char **argv) {
  lastline=lastcol=-1; ok=1;

  if(!(nsbuf=(utf8)malloc(nsbufSize))) error_handler(ER_MALLOC);
  if(!(savedNs=(int *)malloc(savedNsSize*sizeof(int))))
    error_handler(ER_MALLOC);

  srcpi=1;
  while(*(++argv)&&**argv=='-') {
    int i=1;
    for(;;) {
      switch(*(*argv+i)) {
      case '\0': goto END_OF_OPTIONS;
      case 'P': srcpi=0; break;
      case 'h': case '?': usage(); return 1;
      default: fprintf(stderr,"unknown option '-%c'\n",*(*argv+i)); break;
      }
      ++i;
    }
    END_OF_OPTIONS:;
  }

  if(*(argv) && *(argv+1)) {usage(); return EXIT_FAILURE;}
  if(*argv) {
    FILE *fp; xml=*argv;
    if((fp=fopen(xml,"r"))) {
      c14nize(fp);
      fclose(fp);
    } else {
      fprintf(stderr,"I/O error (%s): %s\n",xml,strerror(errno));
      ok=0;
    }
  } else {
    xml="stdin";
    c14nize(stdin);
  }

  return ok?EXIT_SUCCESS:EXIT_FAILURE;
}
