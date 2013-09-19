/***********************************************************
	Copyright 1998 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else /* HAVE_STRINGS_H */
# include <string.h>
#endif /* HAVE_STRINGS_H */

#include <ctype.h>
#include <sys/types.h>

#ifdef WIN32
#define STRICT

#include <tchar.h>
#include <string.h>
#include <winsock2.h>
#include <memory.h>
#include <windows.h>

#else /* WIN32 */
#include <sys/time.h>
#include <sys/param.h>
#include <netinet/in.h>
#endif /* WIN32 */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

#include <snmp/libsnmp.h>
#if 0
#include "asn1.h"
#include "snmp_pdu.h"
#include "snmp_vars.h"
#include "snmp_session.h"
#include "snmp_impl.h"
#include "snmp_api.h"
#include "snmp_extra.h"
#include "parse.h"
#include "mib.h"
#include "options.h"
#endif
SNMP_NAMESPACE

static int
sprint_by_type(//buf, var, enums, quiet)
    char *buf,
    struct variable_list *var,
    struct enum_list	    *enums,
    int quiet);
//static int sprint_by_type();

static char *
uptimeString(//timeticks, buf)
  unsigned int timeticks,
    char *buf)
{
    int	seconds, minutes, hours, days;

    timeticks /= 100;
    days = timeticks / (60 * 60 * 24);
    timeticks %= (60 * 60 * 24);

    hours = timeticks / (60 * 60);
    timeticks %= (60 * 60);

    minutes = timeticks / 60;
    seconds = timeticks % 60;

    if (days == 0){
	sprintf(buf, WIDE("%d:%02d:%02d"), hours, minutes, seconds);
    } else if (days == 1) {
	sprintf(buf, WIDE("%d day, %d:%02d:%02d"), days, hours, minutes, seconds);
    } else {
	sprintf(buf, WIDE("%d days, %d:%02d:%02d"), days, hours, minutes, seconds);
    }
    return buf;
}

static void sprint_hexstring(//buf, cp, len)
    char *buf,
    u_char  *cp,
    int	    len)
{

    for(; len >= 16; len -= 16){
	sprintf(buf, WIDE("%02X %02X %02X %02X %02X %02X %02X %02X "), cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
	buf += strlen(buf);
	cp += 8;
	sprintf(buf, WIDE("%02X %02X %02X %02X %02X %02X %02X %02X\n"), cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
	buf += strlen(buf);
	cp += 8;
    }
    for(; len > 0; len--){
	sprintf(buf, WIDE("%02X "), *cp++);
	buf += strlen(buf);
    }
    *buf = '\0';
}

static void sprint_asciistring(//buf, cp, len)
    char *buf,
    u_char  *cp,
    int	    len)
{
    int	x;

    for(x = 0; x < len; x++){
	if (isprint(*cp)){
	    *buf++ = *cp++;
	} else {
	    *buf++ = '.';
	    cp++;
	}
	if ((x % 48) == 47)
	    *buf++ = '\n';
    }
    *buf = '\0';
}

static void
sprint_octet_string(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
struct enum_list *foo,
    int quiet)
{
    int hex, x;
    u_char *cp;

    if (var->type != ASN_OCTET_STR){
	sprintf(buf, WIDE("Wrong Type (should be OCTET STRING): "));
	buf += strlen(buf);
	sprint_by_type(buf, var, (struct enum_list *)NULL, quiet);
	return;
    }
    hex = 0;
    for(cp = var->val.string, x = 0; x < (var->val_len-1); x++, cp++) {
      if (!(isprint(*cp) || isspace(*cp))) {
	hex = 1;
	break;
      }
    }
    /* Now only check the last char if it's not a NULL.
     * Some things include the trailing NULL in the supplied length,
     * while others don't.
     */
    /*  If len>0 we have to check the last char */
    if ((!hex) && (var->val_len) && (*cp != '\0') &&
	(!(isprint(*cp) || isspace(*cp)))) {
      hex = 1;
    }

#if 0
    if (var->val_len <= 4)
	hex = 1;    /* not likely to be ascii */
#endif
    if (hex){
      if (!quiet) {
	sprintf(buf, WIDE("OCTET STRING-   (hex):\t"));
	buf += strlen(buf);
      }
	sprint_hexstring(buf, var->val.string, var->val_len);
    } else {
      if (!quiet) {
	sprintf(buf, WIDE("OCTET STRING- (ascii):\t"));
	buf += strlen(buf);
      }
	sprint_asciistring(buf, var->val.string, var->val_len);
    }
}

static void
sprint_opaque(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
struct enum_list *foo,
    int quiet)
{

    if (var->type != SMI_OPAQUE){
	sprintf(buf, WIDE("Wrong Type (should be Opaque): "));
	buf += strlen(buf);
	sprint_by_type(buf, var, (struct enum_list *)NULL, quiet);
	return;
    }
    if (!quiet) {
      sprintf(buf, WIDE("OPAQUE -   (hex):\t"));
      buf += strlen(buf);
    }
    sprint_hexstring(buf, var->val.string, var->val_len);
}

static void
sprint_object_identifier(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
struct enum_list *foo,
    int quiet)
{
    if (var->type != SMI_OBJID){
	sprintf(buf, WIDE("Wrong Type (should be OBJECT IDENTIFIER): "));
	buf += strlen(buf);
	sprint_by_type(buf, var, (struct enum_list *)NULL, quiet);
	return;
    }
    if (!quiet) {
      sprintf(buf, WIDE("OBJECT IDENTIFIER:\t"));
      buf += strlen(buf);
    }
    sprint_objid(buf, (oid *)(var->val.objid), var->val_len / sizeof(oid));
}

static void
sprint_timeticks(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
struct enum_list *foo,
    int quiet)
{
    char timebuf[32];

    if (var->type != SMI_TIMETICKS){
	sprintf(buf, WIDE("Wrong Type (should be Timeticks): "));
	buf += strlen(buf);
	sprint_by_type(buf, var, (struct enum_list *)NULL, quiet);
	return;
    }
    if (!quiet) {
      sprintf(buf, WIDE("Timeticks: "));
      buf += strlen(buf);
    }
    sprintf(buf, WIDE("(%u) %s"),
	    *(var->val.integer),
	    uptimeString(*(var->val.integer), timebuf));
}

static void
sprint_integer(//buf, var, enums, quiet)
    char *buf,
    struct variable_list *var,
    struct enum_list	    *enums,
    int quiet)
{
    char    *enum_string = NULL;

    if (var->type != SMI_INTEGER){
	sprintf(buf, WIDE("Wrong Type (should be INTEGER): "));
	buf += strlen(buf);
	sprint_by_type(buf, var, (struct enum_list *)NULL, quiet);
	return;
    }
    for (; enums; enums = enums->next)
	if (enums->value == *var->val.integer){
	    enum_string = enums->label;
	    break;
	}

    if (!quiet) {
      sprintf(buf, WIDE("INTEGER: "));
      buf += strlen(buf);
    }

    if (enum_string == NULL)
	sprintf(buf, WIDE("%u"), *var->val.integer);
    else
	sprintf(buf, WIDE("%s(%u)"), enum_string, *var->val.integer);
}

static void
sprint_gauge(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
struct enum_list *foo,
int quiet)
{
    if (var->type != SMI_GAUGE32){
	sprintf(buf, WIDE("Wrong Type (should be Gauge): "));
	buf += strlen(buf);
	sprint_by_type(buf, var, (struct enum_list *)NULL, quiet);
	return;
    }
    if (!quiet) {
      sprintf(buf, WIDE("Gauge: "));
      buf += strlen(buf);
    }
    sprintf(buf, WIDE("%u"), *var->val.integer);
}

static void
sprint_counter(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
struct enum_list *foo,
    int quiet)
{
    if (var->type != SMI_COUNTER32){
	sprintf(buf, WIDE("Wrong Type (should be Counter): "));
	buf += strlen(buf);
	sprint_by_type(buf, var, (struct enum_list *)NULL, quiet);
	return;
    }
    if (!quiet) {
      sprintf(buf, WIDE("Counter: "));
      buf += strlen(buf);
    }
    sprintf(buf, WIDE("%u"), *var->val.integer);
}

static void
sprint_networkaddress(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
void *foo,
    int quiet)
{
    int x, len;
    u_char *cp;

    if (!quiet) {
      sprintf(buf, WIDE("Network Address:\t"));
      buf += strlen(buf);
    }
    cp = var->val.string;
    len = var->val_len;
    for(x = 0; x < len; x++){
	sprintf(buf, WIDE("%02X"), *cp++);
	buf += strlen(buf);
	if (x < (len - 1))
	    *buf++ = ':';
    }
}

static void
sprint_ipaddress(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
struct enum_list *foo,
int quiet)
{
    u_char *ip;

    if (var->type != SMI_IPADDRESS){
	sprintf(buf, WIDE("Wrong Type (should be Ipaddress): "));
	buf += strlen(buf);
	sprint_by_type(buf, var, (struct enum_list *)NULL, quiet);
	return;
    }
    ip = var->val.string;
    if (!quiet) {
      sprintf(buf, WIDE("IPAddress:\t"));
      buf += strlen(buf);
    }
    sprintf(buf, WIDE("%d.%d.%d.%d"),ip[0], ip[1], ip[2], ip[3]);
}
//cpg26dec2006 src\snmplib\mib.c(388): Warning! W202: Symbol 'sprint_unsigned_short' has been defined, but not referenced
static void
sprint_unsigned_short(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
void *foo,
    int quiet)
{
    if (var->type != SMI_INTEGER){
	sprintf(buf, WIDE("Wrong Type (should be INTEGER): "));
	buf += strlen(buf);
	sprint_by_type(buf, var, (struct enum_list *)NULL, quiet);
	return;
    }
    if (!quiet) {
      sprintf(buf, WIDE("INTEGER (0..65535): "));
      buf += strlen(buf);
    }
    sprintf(buf, WIDE("%u"), *var->val.integer);
}

static void
sprint_null(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
struct enum_list *foo,
int quiet)
{
    if (var->type != SMI_NULLOBJ){
	sprintf(buf, WIDE("Wrong Type (should be NULL): "));
	buf += strlen(buf);
	sprint_by_type(buf, var, (struct enum_list *)NULL, quiet);
	return;
    }
    sprintf(buf, WIDE("NULL"));
}

static void
sprint_unknowntype(//buf, var, foo, quiet)
    char *buf,
    struct variable_list *var,
void *foo,
int quiet)
{
/*    sprintf(buf, WIDE("Variable has bad type")); */
    sprint_by_type(buf, var, (struct enum_list*)foo, quiet);
}

static void
sprint_badtype(//buf)
    char *buf)
{
    sprintf(buf, WIDE("Variable has bad type"));
}

#define SPRINT_BY_TYPE_TYPE (int (*)( char *buf, struct variable_list *var, struct enum_list *enums, int quiet))

static int
sprint_by_type(//buf, var, enums, quiet)
    char *buf,
    struct variable_list *var,
    struct enum_list	    *enums,
    int quiet)
{
    switch (var->type){
	case SMI_INTEGER:
	    sprint_integer(buf, var, enums, quiet);
	    break;
	case SMI_STRING:
	    sprint_octet_string(buf, var, enums, quiet);
	    break;
	case SMI_OPAQUE:
	    sprint_opaque(buf, var, enums, quiet);
	    break;
	case SMI_OBJID:
	    sprint_object_identifier(buf, var, enums, quiet);
	    break;
	case SMI_TIMETICKS:
	    sprint_timeticks(buf, var, enums, quiet);
	    break;
	case SMI_GAUGE32:
	    sprint_gauge(buf, var, enums, quiet);
	    break;
	case SMI_COUNTER32:
	    sprint_counter(buf, var, enums, quiet);
	    break;
	case SMI_IPADDRESS:
	    sprint_ipaddress(buf, var, enums, quiet);
	    break;
	case SMI_NULLOBJ:
	    sprint_null(buf, var, enums, quiet);
	    break;
	default:
	    sprint_badtype(buf);
	    break;
    }
    return strlen( buf );
}

static struct snmp_mib_tree *get_symbol(    oid	    *objid,
    int	    objidlen,
    struct snmp_mib_tree    *subtree,
    char    *buf
);

static oid RFC1066_MIB[] = { 1, 3, 6, 1, 2, 1 };
static unsigned char RFC1066_MIB_text[] = ".iso.org.dod.internet.mgmt.mib-2";
static struct snmp_mib_tree *Mib = NULL;

static void
set_functions(//subtree)
    struct snmp_mib_tree *subtree)
{
    for(; subtree; subtree = subtree->next_peer){
	switch(subtree->type){
	    case TYPE_OBJID:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_object_identifier;
		break;
	    case TYPE_OCTETSTR:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_octet_string;
		break;
	    case TYPE_INTEGER:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_integer;
		break;
	    case TYPE_NETADDR:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_networkaddress;
		break;
	    case TYPE_IPADDR:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_ipaddress;
		break;
	    case TYPE_COUNTER:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_counter;
		break;
	    case TYPE_GAUGE:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_gauge;
		break;
	    case TYPE_TIMETICKS:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_timeticks;
		break;
	    case TYPE_OPAQUE:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_opaque;
		break;
	    case TYPE_NULL:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_null;
		break;
	    case TYPE_OTHER:
	    default:
		subtree->printer = SPRINT_BY_TYPE_TYPE sprint_unknowntype;
		break;
	}
	set_functions(subtree->child_list);
    }
}

int load_mib(char *path, int MibType)
{
	if (Mib != NULL) return(0);

  if (MibType == OLD_CMU_SNMP_MIB)
    Mib = read_mib(path);
  else if (MibType == SNMPV2_MIB)
    Mib = read_mib_v2(path);

  if (Mib == NULL)
    return(0);

  set_functions(Mib);
  return(1);
}


int init_mib(void)
{
    char *file;//, *getenv();

		if (Mib != NULL) return (0);

    /* First, try the new parser if the variable exists.
     */

    file = getenv("MIBFILE_v2");
    if (file != NULL)
	Mib = read_mib_v2(file);

    /* Then our overrides
     */
    if (Mib == NULL)
      if ((file = getenv("MIBFILE")) != NULL)
	Mib = read_mib(file);

    /* Then the default mibfiles
     */
    if (Mib == NULL)
	Mib = read_mib_v2("mib-v2.txt");
    if (Mib == NULL)
	Mib = read_mib_v2("/etc/mib-v2.txt");

#ifndef WIN32
#define MIBDIR "."
    if (Mib == NULL) {
      char path[MAXPATHLEN];
      sprintf(path, WIDE("%s/mib-v2.txt"), MIBDIR);
      Mib = read_mib_v2(path);
    }
#endif /* WIN32 */

    /* And finally the old faithful files.
     */
    if (Mib == NULL)
	Mib = read_mib("mib.txt");
    if (Mib == NULL)
	Mib = read_mib("/etc/mib.txt");

#ifndef WIN32
    if (Mib == NULL) {
      char path[MAXPATHLEN];
      sprintf(path, WIDE("%s/mib.txt"), MIBDIR);
      Mib = read_mib(path);
    }
#endif /* WIN32 */

#ifdef WIN32

    if (Mib == NULL) {
      /* Fetch the name from the registry */
      long ret;
      HKEY   hKey;
      DWORD Type;

#define MAX_VALUE_NAME              128
#define KEY "SOFTWARE\\Carnegie Mellon\\Network Group\\SNMP Library"

      TCHAR ValueName[MAX_VALUE_NAME];
      DWORD dwcValueName = MAX_VALUE_NAME;

      ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			 _TEXT(KEY),
			 0,
			 KEY_READ,
			 &hKey);

      if (ret == ERROR_SUCCESS) {
        /* Found this registry entry. */
        ret = RegQueryValueEx(hKey,
          _TEXT("MIB Location"),
          NULL,
          &Type,
          (LPBYTE)&ValueName,
			      &dwcValueName);

        if (ret == ERROR_SUCCESS) {

#ifdef UNICODE

          _tprintf(_TEXT("Found '%s'\n"), ValueName);

          {
            char lpszBuf[MAX_VALUE_NAME];

            if (WideCharToMultiByte(CP_ACP, 0, ValueName, WC_SEPCHARS, // -1
              lpszBuf, dwcValueName, NULL, NULL)) {
              Mib = read_mib_v2(lpszBuf);
            }
          }

#else /* UNICODE */
           Mib = read_mib_v2(ValueName);

#endif /* UNICODE */

        } else {
          /* Unable to read key */
          LPVOID lpMsgBuf;
          FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS, NULL, ret,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (TCHAR *)&lpMsgBuf, 0, NULL);
#ifdef STDERR_OUTPUT
          fprintf(stderr, WIDE("Reg Read Error: %s\n"), (LPTSTR)lpMsgBuf);
#endif
        }
        /* And close the registry */
        RegCloseKey(hKey);

      } else {

        /* Unable to open key */
  	LPVOID lpMsgBuf;
	  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		      FORMAT_MESSAGE_IGNORE_INSERTS, NULL, ret,
		      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (TCHAR *)&lpMsgBuf, 0, NULL);
#ifdef STDERR_OUTPUT
    fprintf(stderr, WIDE("Reg Open Error: %s\n"), (LPTSTR)lpMsgBuf);
#endif
      }
    }
#endif /* WIN32 */

    if (Mib == NULL) {
#ifdef STDERR_OUTPUT
      fprintf(stderr, WIDE("Couldn't find mib file\n"));
#endif
      return(0);
    }
    set_functions(Mib);
    return(1);
}


static struct snmp_mib_tree *
find_rfc1066_mib(//root)
    struct snmp_mib_tree *root)
{
    oid *op = RFC1066_MIB;
    struct snmp_mib_tree *tp;
    int len;

    for(len = sizeof(RFC1066_MIB)/sizeof(oid); len; len--, op++){
	for(tp = root; tp; tp = tp->next_peer){
	    if (tp->subid == *op){
		root = tp->child_list;
		break;
	    }
	}
	if (tp == NULL)
	    return NULL;
    }
    return root;
}

static int
lc_cmp(//s1, s2)
    char *s1, char *s2 )
{
    char c1, c2;

    while(*s1 && *s2){
	if (isupper(*s1))
	    c1 = tolower(*s1);
	else
	    c1 = *s1;
	if (isupper(*s2))
	    c2 = tolower(*s2);
	else
	    c2 = *s2;
	if (c1 != c2)
	    return ((c1 - c2) > 0 ? 1 : -1);
	s1++;
	s2++;
    }

    if (*s1)
	return -1;
    if (*s2)
	return 1;
    return 0;
}

static int
parse_subtree(//subtree, input, output, out_len)
    struct snmp_mib_tree *subtree,
    char *input,
    oid	*output,
    int	*out_len)   /* number of subid's */
{
    char buf[128], *to = buf;
    u_int subid = 0;
    struct snmp_mib_tree *tp;

    /*
     * No empty strings.  Can happen if there is a trailing '.' or two '.'s
     * in a row, i.e. "..".
     */
    if ((*input == '\0') ||
	(*input == '.'))
	return (0);

    if (isdigit(*input)) {
	/*
	 * Read the number, then try to find it in the subtree.
	 */
	while (isdigit(*input)) {
	    subid *= 10;
	    subid += *input++ - '0';
	}
	for (tp = subtree; tp; tp = tp->next_peer) {
	    if (tp->subid == subid)
		goto found;
	}
	tp = NULL;
    }
    else {
	/*
	 * Read the name into a buffer.
	 */
	while ((*input != '\0') &&
	       (*input != '.')) {
	    *to++ = *input++;
	}
	*to = '\0';

	/*
	 * Find the name in the subtree;
	 */
	for (tp = subtree; tp; tp = tp->next_peer) {
	    if (lc_cmp(tp->label, buf) == 0) {
		subid = tp->subid;
		goto found;
	    }
	}

	/*
	 * If we didn't find the entry, punt...
	 */
	if (tp == NULL) {
#ifdef STDERR_OUTPUT
	    fprintf(stderr, WIDE("sub-identifier not found: %s\n"), buf);
#endif
	    return (0);
	}
    }

 found:
    //cpg26dec2006 src\snmplib\mib.c(808): Warning! W124: Comparison result always 0
	 if(subid > (u_int)MAX_SUBID){
		 {
#ifdef STDERR_OUTPUT
			 fprintf(stderr, WIDE("sub-identifier too large: %s\n"), buf);
#endif
			 return (0);
		 }
	 }

    if ((*out_len)-- <= 0){
#ifdef STDERR_OUTPUT
	fprintf(stderr, WIDE("object identifier too long\n"));
#endif
	return (0);
    }
    *output++ = subid;

    if (*input != '.')
	return (1);
    if ((*out_len =
	 parse_subtree(tp ? tp->child_list : NULL, ++input, output, out_len)) == 0)
	return (0);
    return (++*out_len);
}

int read_objid(//input, output, out_len)
    char *input,
    oid *output,
    int	*out_len)   /* number of subid's in "output" */
{
    struct snmp_mib_tree *root = Mib;
    oid *op = output;
    int i;

    if (*input == '.')
	input++;
    else {
	root = find_rfc1066_mib(root);
	for (i = 0; i < sizeof (RFC1066_MIB)/sizeof(oid); i++) {
	    if ((*out_len)-- > 0)
		*output++ = RFC1066_MIB[i];
	    else {
#ifdef STDERR_OUTPUT
		fprintf(stderr, WIDE("object identifier too long\n"));
#endif
		return (0);
	    }
	}
    }

    if (root == NULL) {
      //extern int mib_TxtToOid(char *Buf, oid **OidP, int *LenP);

      oid *tmp;
      if (!mib_TxtToOid(input, &tmp, out_len)) {
#ifdef STDERR_OUTPUT
	fprintf(stderr, WIDE("Mib not initialized.  Exiting.\n"));
#endif
	return(0);
      }

      memcpy((char *)output, (char *)tmp, (*out_len * sizeof(oid)));
      free((char *)tmp);
      return (1);
    }
    if ((*out_len =
	 parse_subtree(root, input, output, out_len)) == 0)
	return (0);
    *out_len += output - op;

    return (1);
}

void print_objid(//objid, objidlen)
    oid	    *objid,
    int	    objidlen)	/* number of subidentifiers */
{
    char    buf[MAX_RETURN_BUFLEN];
    struct snmp_mib_tree    *subtree = Mib;

    *buf = '.';	/* this is a fully qualified name */
    get_symbol(objid, objidlen, subtree, buf + 1);
    printf("%s\n", buf);

}

void sprint_objid(//buf, objid, objidlen)
    char *buf,
    oid	    *objid,
    int	    objidlen)	/* number of subidentifiers */
{
    struct snmp_mib_tree    *subtree = Mib;

    *buf = '.';	/* this is a fully qualified name */
    get_symbol(objid, objidlen, subtree, buf + 1);
}


void print_variable_ex(//objid, objidlen, variable, subtree)
    oid     *objid,
    int	    objidlen,
    struct  variable_list *variable,
    struct snmp_mib_tree    *subtree)
{
    char    buf[MAX_RETURN_BUFLEN], *cp;

    *buf = '.';	/* this is a fully qualified name */
    subtree = get_symbol(objid, objidlen, subtree, buf + 1);
    cp = buf;
    if ((strlen(buf) >= strlen((char *)RFC1066_MIB_text)) && !memcmp(buf, (char *)RFC1066_MIB_text,
	strlen((char *)RFC1066_MIB_text))){
	    cp += sizeof(RFC1066_MIB_text);
    }
    printf("Name: %s -> ", cp);
    *buf = '\0';
    if( subtree )
    {
       if (subtree->printer)
	       (SPRINT_BY_TYPE_TYPE subtree->printer)(buf, variable, subtree->enums, 0);
       else {
   	    sprint_by_type(buf, variable, subtree->enums, 0);
       }
       printf("%s\n", buf);
    }
    else
       printf("Unknown Partial...\n" );
}

void print_variable(//objid, objidlen, variable)
    oid     *objid,
    int	    objidlen,
    struct  variable_list *variable)
{
   print_variable_ex( objid, objidlen, variable, Mib );
}

void sprint_variable(//buf, objid, objidlen, variable)
    char *buf,
    oid     *objid,
    int	    objidlen,
    struct  variable_list *variable)
{
    char    tempbuf[MAX_RETURN_BUFLEN], *cp;
    struct snmp_mib_tree    *subtree = Mib;

    *tempbuf = '.';	/* this is a fully qualified name */
    subtree = get_symbol(objid, objidlen, subtree, tempbuf + 1);
    cp = tempbuf;
    if ((strlen(buf) >= strlen((char *)RFC1066_MIB_text)) && !memcmp(buf, (char *)RFC1066_MIB_text,
	strlen((char *)RFC1066_MIB_text))){
	    cp += sizeof(RFC1066_MIB_text);
    }
    sprintf(buf, WIDE("Name: %s -> "), cp);
    buf += strlen(buf);
    if (subtree->printer)
		 (SPRINT_BY_TYPE_TYPE subtree->printer)(buf, variable, subtree->enums, 0);
    else {
	sprint_by_type(buf, variable, subtree->enums, 0);
    }
    strcat(buf, WIDE("\n"));
}

int sprint_value_ex(//buf, objid, objidlen, variable, subtree)
    char *buf,
    oid     *objid,
    int	    objidlen,
    struct  variable_list *variable,
    struct snmp_mib_tree    *subtree)
{
    char    tempbuf[MAX_RETURN_BUFLEN];

    subtree = get_symbol(objid, objidlen, subtree, tempbuf);
    if( subtree )
       if (subtree->printer)
	      return (SPRINT_BY_TYPE_TYPE subtree->printer)(buf, variable, subtree->enums, 0), 0;
	return sprint_by_type(buf, variable, subtree->enums, 0);
}

void sprint_value(//buf, objid, objidlen, variable)
    char *buf,
    oid     *objid,
    int	    objidlen,
    struct  variable_list *variable)
{
   sprint_value_ex(buf, objid, objidlen, variable, Mib );
}

void print_value(//objid, objidlen, variable)
    oid     *objid,
    int	    objidlen,
    struct  variable_list *variable)
{
    char    tempbuf[MAX_RETURN_BUFLEN];
    struct snmp_mib_tree    *subtree = Mib;

    subtree = get_symbol(objid, objidlen, subtree, tempbuf);
    if (subtree->printer)
		 (SPRINT_BY_TYPE_TYPE subtree->printer)(tempbuf, variable, subtree->enums, 0);
    else {
	sprint_by_type(tempbuf, variable, subtree->enums, 0);
    }
    printf("%s\n", tempbuf);
}

static struct snmp_mib_tree *
get_symbol(//objid, objidlen, subtree, buf)
    oid	    *objid,
    int	    objidlen,
    struct snmp_mib_tree    *subtree,
    char    *buf)
{
    struct snmp_mib_tree    *return_tree = NULL;

    for(; subtree; subtree = subtree->next_peer){
	if (*objid == subtree->subid){
	    strcpy(buf, subtree->label);
	    goto found;
	}
    }

    /* subtree not found */
    while(objidlen--){	/* output rest of name, uninterpreted */
	sprintf(buf, WIDE("%u."), *objid++);
	while(*buf)
	    buf++;
    }
    *(buf - 1) = '\0'; /* remove trailing dot */
    return NULL;

found:
    if (objidlen > 1){
	while(*buf)
	    buf++;
	*buf++ = '.';
	*buf = '\0';
	return_tree = get_symbol(objid + 1, objidlen - 1, subtree->child_list, buf);
    }
    if (return_tree != NULL)
	return return_tree;
    else
	return subtree;
}

void print_variable_list(struct variable_list *V )
{
  print_variable(V->name, V->name_length, V);
}

void print_variable_list_ex(struct variable_list *V, struct snmp_mib_tree *mib)
{
  print_variable_ex(V->name, V->name_length, V, mib);
}


void print_variable_list_value(struct variable_list *variable)
{
  char    buf[MAX_RETURN_BUFLEN];
  struct snmp_mib_tree    *subtree = Mib;

  *buf = '.';	/* this is a fully qualified name */
  subtree = get_symbol(variable->name, variable->name_length, subtree, buf + 1);
  *buf = '\0';

  if (subtree->printer)
    (SPRINT_BY_TYPE_TYPE subtree->printer)(buf, variable, subtree->enums, 1);
  else {
    sprint_by_type(buf, variable, subtree->enums, 1);
  }
  printf("%s", buf);
}

void print_type(struct variable_list *var)
{
  switch (var->type){
  case SMI_INTEGER:
    printf("Integer");
    break;
  case SMI_STRING:
    printf("Octet String");
    break;
  case SMI_OPAQUE:
    printf("Opaque");
    break;
  case SMI_OBJID:
    printf("Object Identifier");
    break;
  case SMI_TIMETICKS:
    printf("Timeticks");
    break;
  case SMI_GAUGE32:
    printf("Gauge");
    break;
  case SMI_COUNTER32:
    printf("Counter");
    break;
  case SMI_IPADDRESS:
    printf("IP Address");
    break;
  case SMI_NULLOBJ:
    printf("NULL");
    break;
  default:
    printf("Unknown type %d\n", var->type);
    break;
  }
}

void print_oid_nums(oid *O, int len)
{
  int x;

  for (x=0;x<len;x++)
    printf(".%u", O[x]);
}

void free_mib(void)
{
  struct snmp_mib_tree *ptr, *next;

  for (ptr=Mib; ptr; ptr=next) {
    next = ptr->next_peer;
    tree_Free(ptr);
  }
}

SNMP_NAMESPACE_END
// $Log: $
