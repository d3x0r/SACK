/* $Id: lib.c,v 1.1 2005/06/02 19:12:50 jim Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "xg.h"

#define err(msg) vfprintf(stderr,msg"\n",ap);
void xg_perror(int erno,va_list ap) {
  switch(erno) {
  case ER_IO: err("%s"); break;
  case ER_XML: err("%s"); break;
  case ER_NOXENT: err("cannot open external entity '%s': %s"); break;
  case ER_NOSID: err("external entity without systemId"); break;
  case ER_GENX: err("genx error (%s): %s"); break;
  case ER_MALLOC: err("allocation failed"); break;
  default: assert(0);
  }
}

char *xg_abspath(char *r,char *b) {
  if(*r!='/') {
    char *c=b,*sep=(char*)0;
    for(;;) {if(!(*c)) break; if(*c++=='/') sep=c;}
    if(sep) {
      char *p,*q;
      p=r; while(*p++); q=p+(sep-b);
      do *(--q)=*(--p); while(p!=r);
      p=r; while(b!=sep) *p++=*b++;
    }
  }
  return r;
}

