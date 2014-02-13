/* -*- c++ -*- */
#ifndef _SNMP_PARSE_H_
#define _SNMP_PARSE_H_

/***************************************************************************
 *
 *           Copyright 1998 by Carnegie Mellon University
 * 
 *                       All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of CMU not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * 
 * CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 * Author: Ryan Troll <ryan+@andrew.cmu.edu>
 * 
 * $Id: parse.h,v 1.1.1.1 2001/11/15 01:26:44 panther Exp $
 * 
 ***************************************************************************/

SNMP_NAMESPACE

/*
 * parse.h
 */

/*
 * A linked list of tag-value pairs for enumerated integers.
 */
struct enum_list {
    struct enum_list *next;
    int	value;
    char *label;
};

/*
 * A tree in the format of the tree structure of the MIB.
 */
struct snmp_mib_tree {
  struct snmp_mib_tree *child_list;	/* list of children of this node */
  struct snmp_mib_tree *next_peer;	/* Next node in list of peers */
  struct snmp_mib_tree *parent;
  char label[64];		/* This node's textual name */
  u_int subid;		        /* This node's integer subidentifier */
  int type;			/* This node's object type */
  struct enum_list *enums;	/* (optional) list of enumerated integers (otherwise NULL) */
  int (*printer)(
    char *buf,
    struct variable_list *var,
    struct enum_list	    *enums,
    int quiet);            /* Value printing function */
};

/* non-aggregate types for tree end nodes */
#define TYPE_OTHER	    0
#define TYPE_OBJID	    1
#define TYPE_OCTETSTR	    2
#define TYPE_INTEGER	    3
#define TYPE_NETADDR	    4
#define	TYPE_IPADDR	    5
#define TYPE_COUNTER	    6
#define TYPE_GAUGE	    7
#define TYPE_TIMETICKS	    8
#define TYPE_OPAQUE	            9
#define TYPE_NULL	    10

#ifdef WIN32
#define DLLEXPORT EXPORT_METHOD
#else  /* WIN32 */
#define DLLEXPORT
#endif /* WIN32 */

DLLEXPORT struct snmp_mib_tree *read_mib(char *);    
DLLEXPORT struct snmp_mib_tree *read_mib_v2(char *);     /* New parser */
DLLEXPORT void tree_Free(struct snmp_mib_tree *p);

SNMP_NAMESPACE_END

#endif /* _SNMP_PARSE_H_ */
// $Log: $
