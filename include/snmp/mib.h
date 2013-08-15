#ifndef _SNMP_MIB_H_
#define _SNMP_MIB_H_

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

#ifdef WIN32
#define DLLEXPORT EXPORT_METHOD
#else  /* WIN32 */
#define DLLEXPORT
#endif /* WIN32 */

SNMP_NAMESPACE

DLLEXPORT int  init_mib(void);
DLLEXPORT int  load_mib(char *, int);
DLLEXPORT int  read_objid(char *, oid *, int *);
DLLEXPORT void print_objid(oid *, int);
DLLEXPORT void sprint_objid(char *, oid *, int);
DLLEXPORT void print_variable(oid *, int, struct variable_list *);
DLLEXPORT void sprint_variable(char *, oid *, int, struct variable_list *);
DLLEXPORT void sprint_value(char *, oid *, int, struct variable_list *);
DLLEXPORT int sprint_value_ex(char *, oid *, int, struct variable_list *, struct snmp_mib_tree *);
DLLEXPORT void print_value(oid *, int, struct variable_list *);


DLLEXPORT void print_variable_list(struct variable_list *);
DLLEXPORT void print_variable_list_ex(struct variable_list *V, struct snmp_mib_tree *mib);
DLLEXPORT void print_variable_list_value(struct variable_list *);
DLLEXPORT void print_type(struct variable_list *);

DLLEXPORT void print_oid_nums(oid *, int);

SNMP_NAMESPACE_END

#define OLD_CMU_SNMP_MIB 1
#define SNMPV2_MIB 2

#endif /* _SNMP_MIB_H_ */
// $Log: $
