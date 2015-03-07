/*#define DEBUG_RYAN 1*/
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
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <sack_types.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else /* HAVE_STRINGS_H */
# include <string.h>
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif /* HAVE_MALLOC_H */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if defined( __WATCOMC__ ) || defined( _MSC_VER )
#include <direct.h>
#endif

#include <stdarg.h>
#include <ctype.h>

#include <snmp/libsnmp.h>
#if 0
#include "asn1.h"
#include "parse.h"
#include "options.h"
#endif

SNMP_NAMESPACE
//static char rcsid[] = 
//"$Id: new-parse.c,v 1.1.1.1 2001/11/15 01:29:34 panther Exp $";

/***************************************************************************
 *
 ***************************************************************************/


/* ------------------------------------------------------------------*/

/* Lines read in the MIB so far.
 */ 
static int Line = 0;
static int VerboseMIB = 0;

/* ------------------------------------------------------------------*/

/* The tokens we care about in a MIB.  Everything else is a label.
 */

enum token_type_tag {
  tok_Continue     = -1,
  tok_EndOfFile    = 0,
  tok_Label,
  tok_Number,
  tok_LeftBracket,
  tok_RightBracket,
  tok_LeftParen,
  tok_RightParen,
  tok_Comma,

  /* Basic ASN Types */
  tok_Integer,
  tok_Octet,  /* "Octet String" */
  tok_Object, /* "Object Identifier" */
  tok_Null,

  /* Basic SMI Types, defined in RFC 1902 */
  tok_IPAddress,
  tok_Gauge32,
  tok_Counter32,
  tok_Opaque,
  tok_Counter64,
  tok_TimeTicks,

  tok_Equals,

  /* Everything above tok_Equals must be things that are within Textual
   * Convention's SYNTAX field
   */

  tok_Syntax,
  tok_Status,
  tok_Description,

  /* Defined in RFC 1902 */
  tok_ModuleIdentity,
  tok_ObjectIdentity,
  tok_ObjectType,
  tok_NotificationType,

  tok_Definitions,
  tok_Imports,
  tok_TextualConvention,
  tok_Sequence,
  tok_ModuleCompliance,
  tok_ObjectGroup,
  tok_NotificationGroup,

  tok_TrapType,

  tok_Include,  // JAB Inclusion extensions....

};
typedef enum token_type_tag MyTokenType;

struct Token {
  char       *Name;		/* token name */
  MyTokenType   Type;		/* type */
  int         HashVal;   	/* hash of name */
  struct Token *Next;		/* pointer to next in hash table */
};

static struct Token tokens[] = {
  { "{", tok_LeftBracket },
  { "}", tok_RightBracket },
  { "::=", tok_Equals },
  { "(", tok_LeftParen },
  { ")", tok_RightParen },
  { ",", tok_Comma },

  /* JAB : File Inclusion Extensions */
  { "INCLUDE", tok_Include },

  /* Basic ASN Types */
  { "INTEGER", tok_Integer },
  { "OCTET",   tok_Octet   },  /* "Octet String" */
  { "OBJECT",  tok_Object  },  /* "Object Identifier" */
  { "NULL",    tok_Null    },

  /* Basic SMI Types */
  { "IPADDRESS", tok_IPAddress },
  { "GAUGE",     tok_Gauge32   },
  { "GAUGE32",   tok_Gauge32   },
  { "COUNTER",   tok_Counter32 },
  { "COUNTER32", tok_Counter32 },
  { "OPAQUE",    tok_Opaque    },
  { "COUNTER64", tok_Counter64 },
  { "TIMETICKS", tok_TimeTicks },

  { "SYNTAX",          tok_Syntax },
  { "STATUS",          tok_Status },
  { "DESCRIPTION",     tok_Description },

  /* Defined in RFC 1902 */
  { "MODULE-IDENTITY",    tok_ModuleIdentity },
  { "OBJECT-IDENTITY",    tok_ObjectIdentity },
  { "OBJECT-TYPE",        tok_ObjectType },
  { "NOTIFICATION-TYPE",  tok_NotificationType },

  { "DEFINITIONS",     tok_Definitions },
  { "IMPORTS",         tok_Imports },
  { "TEXTUAL-CONVENTION", tok_TextualConvention },
  { "SEQUENCE",           tok_Sequence },
  { "MODULE-COMPLIANCE",  tok_ModuleCompliance },
  { "OBJECT-GROUP",       tok_ObjectGroup },
  { "NOTIFICATION-GROUP", tok_NotificationGroup },

  /* SMIv1 */
  { "TRAP-TYPE",  tok_TrapType },

  { NULL }
};

/* Hash functions to speed up token lookup */

#define	HASHSIZE	32
#define	BUCKET(x)	(x & 0x01F)

static struct Token *TokenBuckets[HASHSIZE];

static void hash_Initialize(void)
{
  struct Token *tp;
  char *cp;
  int h;
  int b;

  memset((char *)TokenBuckets, '\0', sizeof(TokenBuckets));

  /* Insert all tokens */
  for (tp = tokens; tp->Name; tp++) {

    for (h = 0, cp = tp->Name; *cp; cp++)
      h += *cp;
    tp->HashVal = h;

    /* Find the bucket index */
    b = BUCKET(h);

    /* Head insert */
    tp->Next = TokenBuckets[b];
    TokenBuckets[b] = tp;
  }
}

/* Function to read the next token.
 *
 * Returns: Token type
 */

static char LastCharRead = ' ';

static MyTokenType ReadNextToken(FILE *fp, char *TokenBuf)
{
  int ch;
  char *TokenBufPtr = TokenBuf;
  int hash = 0;
  struct Token *tp;

  /* NULL Terminate buffer to start */
  *TokenBufPtr = 0;

  /* Restart where we were */
  ch = LastCharRead;

  /* skip all white space */
  while (isspace(ch) && ch != -1) {
    ch = getc(fp);
    if (ch == '\n')
      Line++;
  }
  if (ch == -1)
    return (tok_EndOfFile);

  /* Accumulate characters until whitespace is found.  Then attempt
   * to match this token as a reserved word.  If a match is found,
   * return the type.  Else it is a label.  
   */
  
  do {
    if (ch == '\n')
      Line++;

    if (!(isspace(ch) || ch == '(' || ch == ')' || 
	ch == '{' || ch == '}' || ch == ',' ||
	ch == '"' )) {
      
      /* Still in a token.  So, let's keep going. */

      hash += ch;
      *TokenBufPtr++ = ch;
      if (ch == '\n')
	Line++;
    } else {

      /* End of a token.  Figure out what it is. */
	
      if (!isspace(ch) && *TokenBuf == 0) {
	hash += ch;
	*TokenBufPtr++ = ch;
	LastCharRead = ' ';
      } else {
	LastCharRead = ch;
      }
      *TokenBufPtr = '\0';

      /* Find this token */
      for (tp = TokenBuckets[BUCKET(hash)]; tp; tp = tp->Next) {
	if ((tp->HashVal == hash) && (!strcmp(tp->Name, TokenBuf)))
	  /* Found it!  Quit looking. */
	  break;
      }

      if (tp) {
#ifdef DEBUG_RYAN
	fprintf(stdout, WIDE("TOKEN: Found '%s'\n"), TokenBuf);
#endif /* DEBUG_RYAN */
	return (tp->Type);
      }

      /* Didn't find this string in the token table.
       */

      /* If it's a comment, skip it.
       */
      if (TokenBuf[0] == '-' && TokenBuf[1] == '-') {

	if (ch != '\n') {
	/* Skip until the end of the line if not already there */
	  while ((ch = getc(fp)) != -1)
	    if (ch == '\n'){
	      Line++;
	      break;
	    }
	}

	/* Was this the last line? */
	if (ch == -1)
	  return(tok_EndOfFile);

	/* No?  Read the next token. */
	LastCharRead = ch;
	return(ReadNextToken(fp, TokenBuf));		
      }

      /* Not a known token, or a comment.  Now let's see if it's a label
       * or a number.
       */

      for(TokenBufPtr = TokenBuf; *TokenBufPtr; TokenBufPtr++)
	if (!isdigit(*TokenBufPtr)) {
#ifdef DEBUG_RYAN
	  fprintf(stdout, WIDE("TOKEN: '%s' (LABEL)\n"), TokenBuf);
#endif /* DEBUG_RYAN */
	  return(tok_Label);
	}

#ifdef DEBUG_RYAN
      fprintf(stdout, WIDE("TOKEN: '%s' (NUMBER)\n"), TokenBuf);
#endif /* DEBUG_RYAN */
      return(tok_Number);
    }
  } while ((ch = getc(fp)) != -1);
  return(tok_EndOfFile);
}


/* ------------------------------------------------------------------*/

/* Enum lists functions */


static void enum_Free(struct enum_list *Ptr)
{
  struct enum_list *ep, *tep;

  ep = Ptr;

printf("Freeing ENUMs\n");
  while(ep) {
    tep = ep;
    ep = ep->next;
    free((char *)tep);
  }
}

static struct enum_list *enum_New(void)
{
  struct enum_list *ep;
  
  ep = (struct enum_list *)malloc(sizeof(struct enum_list));
  memset(ep, '\0', sizeof(struct enum_list));
  return(ep);
}

/* ------------------------------------------------------------------*/

/* This is one element of an object identifier with either an integer
 * subidentifier, or a textual string label, or both.  The subid is -1
 * if not present, and label is NULL if not present.  
 */
struct subid {
    int   SubID;
    char *Label;
};

/* A linked list of nodes.  
 */
struct node {
    struct node *Next;
    char   *Label;              /* This node's (unique) textual name */
    u_int   SubID;              /* This node's integer subidentifier */
    char   *Parent;             /* The parent's textual name */

	 MyTokenType      Type;              /* The type of object this represents */
    struct enum_list *enums;	/* List of enumerated values */
};

/* Store all nodes in a big hash table.
 *
 * Can you tell that the previous members of Network Development only knew
 * one data structure?  :)
 */

#define NHASHSIZE    128
#define NBUCKET(x)   (x & 0x7F)
struct node *NodeBuckets[NHASHSIZE];

static void init_node_hash(struct node *nodes)
{
  struct node *np, *nextp;
  char *cp;
  int hash;

  memset((char *)NodeBuckets, '\0', sizeof(NodeBuckets));

  for(np = nodes; np;) {
    nextp = np->Next;
    hash = 0;

    /* Build hash based on parent name */
    for(cp = np->Parent; *cp; cp++)
      hash += *cp;

    /* Head insert */
    np->Next = NodeBuckets[NBUCKET(hash)];
    NodeBuckets[NBUCKET(hash)] = np;
    np = nextp;
  }
}

struct node *node_New(void)
{
  struct node *np;

  np = (struct node *)malloc(sizeof(struct node));
  memset((char *)np, '\0', sizeof(struct node));


  return(np);
}

static void node_Free(struct node *np)
{
  if (np->enums)
    enum_Free(np->enums);
  if (np->Label) free(np->Label);
  if (np->Parent) free(np->Parent);
  free((char *)np);
}

/* ------------------------------------------------------------------*/

/* Some quick error routines.
 */
static void Error(char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
#ifdef STDERR_OUTPUT
  fprintf(stderr, WIDE("Line %3d: "), Line);
  vfprintf(stderr, fmt, args);
#endif
  va_end(args);
}

#define EAT_TOKEN(x, where, err) { \
  type = ReadNextToken(fp, token); \
  if (type != x) { Error("Error eating token %d, read %d (%s) - %s\n", \
			 x, type, token, where); return(err); } }

/* ------------------------------------------------------------------*/

/* These functions read something from the MIB, and turn it into the
 * appropriate node.  Or skip it.  Or something.
 */


/*
 * Takes a list of the form:
 * { iso org(3) dod(6) 1 }
 * and creates several nodes, one for each parent-child pair.
 *
 * SubOid is an array of length "length"
 *
 * Returns length of fetched OID, or 0.
 */
static int ParseOID(FILE *fp, struct subid *SubOid, int length)
{
  int count;
  MyTokenType type;
  char token[TOKENSIZE];

  EAT_TOKEN(tok_LeftBracket, WIDE("ParseOID"), 0);

  /* Can only read things as long as we have array space */
  type = ReadNextToken(fp, token);
  for(count = 0; count < length; count++, SubOid++) {

    SubOid->Label = NULL;
    SubOid->SubID = -1;

    /* Can be either '1' or 'iso' or 'iso(1)' */

    if (type == tok_RightBracket) {
      /* Done */
      return(count);

    } else if ((type != tok_Label) && (type != tok_Number)) {
      Error("%s is not a valid Object Identifier!", token);
      return(0);
    }

    if (type == tok_Label) {

      /* this entry has a label */
      if (SubOid->Label) { free(SubOid->Label); }
      SubOid->Label = strdup(token);

      type = ReadNextToken(fp, token);
      if (type == tok_LeftParen) {
	type = ReadNextToken(fp, token);
	if (type == tok_Number) {
	  SubOid->SubID = atoi(token);

	  type = ReadNextToken(fp, token);
	  if (type != tok_RightParen) {
	    Error("Expected a closing paren, found %s\n", token);
	    return(0);
	  }
	} else {
	  Error("Expected a number, found %s\n", token);
	  return(0);
	}
      } else {
	continue;
      }
    } else {
      /* this entry has just an integer sub-identifier */
      SubOid->SubID = atoi(token);
    }
    type = ReadNextToken(fp, token);
  }
  return(count);
}





/*
 * Parse an entry of the form:
 * label OBJECT IDENTIFIER ::= { parent 2 }
 * The "label OBJECT IDENTIFIER ::=" portion has already been parsed.
 * Returns NULL on error.
 */
static struct node *ParseObjectIdentifier(FILE *fp, char *name)
{
  int count = 0;
  int length;
  struct subid SubOid[32];
  struct node *np, *root, *oldnp = NULL;

  struct subid *IDPtr = NULL;
  struct subid *NextIDPtr = NULL;

  /* Now read the OID */
  length = ParseOID(fp, SubOid, 32);
#ifdef DEBUG_RYAN
printf("ParseOID returned length %d\n", length);
#endif
  if (length) {

    /* Create a new node */
    np = root = node_New();

    /* For each parent-child subid pair in the subid array,
     * create a node and link it into the node list.
     */
    for(count = 0, IDPtr = SubOid, NextIDPtr=(SubOid+1);
	count < (length - 2); 
	count++, IDPtr++, NextIDPtr++) {

      /* every node must have parent's name and child's name or number */
      if (IDPtr->Label && 
	  (NextIDPtr->Label || (NextIDPtr->SubID != -1))) {
	if (np->Parent) { free(np->Parent); }
	np->Parent = strdup(IDPtr->Label);
	
	if (NextIDPtr->Label) {
	if (np->Label) { free(np->Label); }
	  np->Label = strdup(NextIDPtr->Label);
	}

	if (NextIDPtr->SubID != -1)
	  np->SubID = NextIDPtr->SubID;

	/* set up next entry */
	np->Next = node_New();
	oldnp = np;
	np = np->Next;
      }
    }
    np->Next = (struct node *)NULL;

    /*
     * The above loop took care of all but the last pair.  This pair is taken
     * care of here.  The name for this node is taken from the label for this
     * entry.
     * np still points to an unused entry.
     */
#ifdef DEBUG_RYAN
    printf("ParseOID: Name is %s\n", name);
    printf("Count is %d, length is %d.\n", count, length);
#endif

    if (count == (length - 2)) {
      if (IDPtr && (IDPtr->Label)) {
#ifdef DEBUG_RYAN
      printf("IDLabel is %s\n", IDPtr->Label);
#endif
	if (np->Parent) { free(np->Parent); }
	np->Parent = strdup(IDPtr->Label);
	if (np->Label) { free(np->Label); }
	np->Label = strdup(name);

	if (NextIDPtr->SubID != -1)
	  np->SubID = NextIDPtr->SubID;
	else
	  Error("WARNING: This entry is silly: %s\n", np->Label);
      } else {
	node_Free(np);

	if (oldnp)
	  oldnp->Next = NULL;
	else
	  return(NULL);
      }

    } else {
      Error("Missing end of oid!\n");
      node_Free(np);   /* the last node allocated wasn't used */
      if (oldnp)
	oldnp->Next = NULL;
      return(NULL);
    }

    /* free the oid array */
    for(count = 0, IDPtr = SubOid; count < length; count++, IDPtr++){
      if (IDPtr->Label)
	free(IDPtr->Label);
      IDPtr->Label = NULL;
    }
    return(root);

  } else {
    Error("Invalid OBJECT-IDENTIFIER");
    return(NULL);
  }
}



static void EatDescriptionString(FILE *fp)
{
  int   ReadChar;

  ReadChar = LastCharRead;
  /* Read opening quote */
  while((ReadChar != '"') && (ReadChar != -1)) {
    ReadChar = getc(fp);
    if (ReadChar == '\n')
      Line++;
  }
  ReadChar = ' ';
  /* skip everything until closing quote */
  while((ReadChar != '"') && (ReadChar != -1)) {
    ReadChar = getc(fp);
    if (ReadChar == '\n')
      Line++;
  }
  LastCharRead = ' ';
}

/* ------------------------------------------------------------------*/

/*
 * DEFINED IN RFC 1902
 *
 * Parse an entry of the form:
 *
 * label MODULE-IDENTITY
 *    LAST-UPDATES "..."
 *    ORGANIZATION "..."
 *    CONTACT-INFO "..."
 *    DESCRIPTION " ... "
 *    (OPT)   REVISION "..."
 *    (OPT)   DESCRIPTION " ... "
 *       [ ... ]
 *    ::= { parent 1 }
 *
 * The "label MODULE-IDENTITY" portion has already been parsed.
 *
 * Returns NULL on error.
 *
 * XXXXX Keep ModuleIdentity around for later use?
 */
static struct node *ParseModuleIdentity(FILE *fp, char *name)
{
  MyTokenType type = tok_Label;
  char token[TOKENSIZE];

  /* Eat everything up to the ::=.  But, since that may be in
   * the description, it isn't that easy.
   */

  while (type != tok_Equals) {

    /* Read up to the description.
     *
     * First pass: Last-Updated, Org, etc.
     * Second pass on: Revision
     */
    while(type != tok_Description)
      type = ReadNextToken(fp, token);

    /* Now eat the description */
    EatDescriptionString(fp);

    /* And check the next token */
    type = ReadNextToken(fp, token);
  }

  /* ASSERT:  Just read ::= OUTSIDE of the description string.
   */

  return(ParseObjectIdentifier(fp, name));
}

/* ------------------------------------------------------------------*/

/*
 * DEFINED IN RFC 1902
 *
 * Parse an entry of the form:
 *
 * label OBJECT-IDENTITY
 *    STATUS label
 *    DESCRIPTION " ... "
 *    (OPT) REFERENCE " ... "
 *    ::= { parent 1 }
 *
 * The "label OBJECT-IDENTITY" portion has already been parsed.
 *
 * Returns NULL on error.
 */
static struct node *ParseObjectIdentity(FILE *fp, char *name)
{
  MyTokenType type = tok_Label;
  char token[TOKENSIZE];

  /* Read up to the description.
   */
  while(type != tok_Description)
    type = ReadNextToken(fp, token);

  /* Now eat the description */
  EatDescriptionString(fp);

  /* And check the next token */
  type = ReadNextToken(fp, token);

  if (type != tok_Equals) {
    /* Need to read the optional REFERENCE text. */
    EatDescriptionString(fp);
    type = ReadNextToken(fp, token);
  }

  /* ASSERT:  Just read ::= OUTSIDE of the description / reference strings.
   */

  return(ParseObjectIdentifier(fp, name));
}

/* ------------------------------------------------------------------*/

/*
 * DEFINED IN RFC 1902
 *
 * Parse an entry of the form:
 *
 * label OBJECT-TYPE
 *    SYNTAX [...]
 *    (OPT) UNITS " ... "
 *    MAX-ACCESS label
 *    STATUS label
 *    DESCRIPTION " ... "
 *    (OPT) REFERENCE " ... "
 *    (OPT) INDEX { .. }
 *    (OPT) DEFVAL { ... }
 *    ::= { parent 1 }
 *
 * The "label OBJECT-TYPE" portion has already been parsed.
 *
 * Returns NULL on error.
 */
static struct node *ParseObjectType(FILE *fp, char *name)
{
  MyTokenType type, nexttype;
  char token[TOKENSIZE];
  char nexttoken[TOKENSIZE];
  char syntax[TOKENSIZE];
  struct node *np;
  struct enum_list *ep;
  int count, length;
  struct subid SubOid[32];

  /* Grab the syntax
   */
  EAT_TOKEN(tok_Syntax, WIDE("ObjectType Syntax"), NULL);

  /* Create a new node */
  np = node_New();

  /* Find the type of this syntax */
  type = ReadNextToken(fp, token);
  nexttype = ReadNextToken(fp, nexttoken);

  np->Type = type;

  switch(type) {
    /* TABLE: Sequence Of Blah */
  case tok_Sequence:
    strcpy(syntax, token);
    if ((nexttype == tok_Label) || (!strcmp(token, WIDE("OF")))) {
      strcat(syntax, WIDE(" "));
      strcat(syntax, nexttoken);
      nexttype = ReadNextToken(fp, nexttoken); /* Blah */
      strcat(syntax, WIDE(" "));
      strcat(syntax, nexttoken);
      nexttype = ReadNextToken(fp, nexttoken); /* Read ahead */
    }
    break;

  case tok_Integer:
    strcpy(syntax, token);

    if (nexttype == tok_LeftBracket) {

      /* if there is an enumeration list, parse it */
      while((type = ReadNextToken(fp, token)) != tok_EndOfFile) {
	if (type == tok_RightBracket)
	  break;

	if (type == tok_Label) {
	  /* this is an enumerated label */
	  if (np->enums == NULL){
	    ep = np->enums = enum_New();
	  } else {
	    ep->next = enum_New();
	    ep = ep->next;
	  }

	  /* Copy the label */
	  if (ep->label) { free(ep->label); }
	  ep->label = strdup(token);

	  /* Now read the numeric value */
	  type = ReadNextToken(fp, token);
	  if (type != tok_LeftParen) {
	    Error("Object-Type Enum List: Expected Left Paren\n");
	    node_Free(np);
	    return(NULL);
	  }

	  type = ReadNextToken(fp, token);
	  if (type != tok_Number) {
	    Error("Object-Type Enum List: Expected Integer\n");
	    node_Free(np);
	    return(NULL);
	  }
	  ep->value = atoi(token);

	  type = ReadNextToken(fp, token);
	  if (type != tok_RightParen) {
	    Error("Object-Type Enum List: Expected Right Paren\n");
	    node_Free(np);
	    return(NULL);
	  }
	}
      } /* End of enum list */

      if (type == tok_EndOfFile) {
	Error("Expected right bracket\n");
	node_Free(np);
	return(NULL);
      }
      nexttype = ReadNextToken(fp, nexttoken); /* Read ahead */

    } else if (nexttype == tok_LeftParen) {

      /* ignore the "constrained integer" for now */
      nexttype = ReadNextToken(fp, nexttoken);
      nexttype = ReadNextToken(fp, nexttoken);
      nexttype = ReadNextToken(fp, nexttoken);
    }
    break;

  case tok_Octet:
    /* Assume "Octet String" */
    strcpy(syntax, WIDE("Octet String"));
    nexttype = ReadNextToken(fp, nexttoken);
    break;

  case tok_Object:
    /* Assume "Object Identifier" */
    strcpy(syntax, WIDE("Object Identifier"));
    nexttype = ReadNextToken(fp, nexttoken);
    break;

  case tok_Null:
  case tok_IPAddress:
  case tok_Gauge32:
  case tok_Counter32:
  case tok_Opaque:
  case tok_Counter64:
  case tok_TimeTicks:
    /*  case tok_NetAddress:*/
  case tok_Label:
    strcpy(syntax, token);
    break;

  default:
    Error("Unknown syntax token: %s\n", token);
    node_Free(np);
    return(NULL);
  }

  /* Now read up to the description, and eat it
   */
  type = nexttype;
  while(type != tok_Description)
    type = ReadNextToken(fp, token);

  /* Now eat the description */
  EatDescriptionString(fp);

  /* And up to the ::= */
  while(type != tok_Equals)
    type = ReadNextToken(fp, token);

  /* ASSERT:  Now ready to get the oid */
  length = ParseOID(fp, SubOid, 32);
  
  if (length > 1 && length <= 32) {

    /* just take the last pair in the oid list */
    if (SubOid[length - 2].Label) {
      if (np->Parent) { free(np->Parent); }
      np->Parent = strdup(SubOid[length - 2].Label);
    }
    if (np->Label) { free(np->Label); }
    np->Label = strdup(name);

    if (SubOid[length - 1].SubID != -1)
      np->SubID = SubOid[length - 1].SubID;
    else
      Error("Warning: This entry is pretty silly -- %s", np->Label);
  } else {
    Error("No end to oid!\n");
    node_Free(np);
    np = NULL;
  }

  /* free oid array */
  for(count = 0; count < length; count++){
    if (SubOid[count].Label)
      free(SubOid[count].Label);
    SubOid[count].Label = NULL;
  }

  return(np);
}

/* ------------------------------------------------------------------*/

/*
 * DEFINED IN RFC 1902
 *
 * Parse an entry of the form:
 *
 * label NOTIFICATION-TYPE
 *    (OPT) OBJECTS { ... }
 *    STATUS label
 *    DESCRIPTION " ... "
 *    (OPT) REFERENCE " ... "
 *    ::= { parent 1 }
 *
 * The "label NOTIFICATION-TYPE" portion has already been parsed.
 *
 * Returns NULL on error.
 */
static struct node *ParseNotificationType(FILE *fp, char *name)
{
  MyTokenType type = tok_Label;
  char token[TOKENSIZE];

  /* Read up to the description.
   */
  while(type != tok_Description)
    type = ReadNextToken(fp, token);

  /* Now eat the description */
  EatDescriptionString(fp);

  /* And check the next token */
  type = ReadNextToken(fp, token);

  if (type != tok_Equals) {
    /* Need to read the optional REFERENCE text. */
    EatDescriptionString(fp);
    type = ReadNextToken(fp, token);
  }

  /* ASSERT:  Just read ::= OUTSIDE of the description / reference strings.
   */

  return(ParseObjectIdentifier(fp, name));
}

/* ------------------------------------------------------------------*/

/*
 * Parse an entry of the form:
 *
 * label MODULE-COMPLIANCE
 *    STATUS label
 *    DESCRIPTION " ... "
 *    MANDATORY-GROUPS { ... }
 *    GROUP label
 *    DESCRIPTION " ... "
 *    ::= { parent 1 }
 *
 * The "label MODULE-COMPLIANCE" portion has already been parsed.
 *
 * Returns NULL on error.
 */
static struct node *ParseModuleCompliance(FILE *fp, char *name)
{
  MyTokenType type = tok_Label;
  char token[TOKENSIZE];

  /* Eat everything up to the ::= */

  while(type != tok_Equals)
    type = ReadNextToken(fp, token);

  return(ParseObjectIdentifier(fp, name));
}











/*
 * Parse an entry of the form:
 *
 * label OBJECT-GROUP
 *    OBJECTS { ... }
 *    STATUS label
 *    DESCRIPTION " ... "
 *    ::= { parent 1 }
 *
 * The "label OBJECT-GROUP" portion has already been parsed.
 *
 * Returns NULL on error.
 */
static struct node *ParseObjectGroup(FILE *fp, char *name)
{
  MyTokenType type = tok_Label;
  char token[TOKENSIZE];

  /* Eat everything up to the ::= */

  while(type != tok_Equals)
    type = ReadNextToken(fp, token);

  return(ParseObjectIdentifier(fp, name));
}







/*
 * Parse an entry of the form:
 *
 * label NOTIFICATION-GROUP
 *    NOTIFICATIONS { ... }
 *    STATUS label
 *    DESCRIPTION " ... "
 *    ::= { parent 1 }
 *
 * The "label NOTIFICATION-GROUP" portion has already been parsed.
 *
 * Returns NULL on error.
 */
static struct node *ParseNotificationGroup(FILE *fp, char *name)
{
  MyTokenType type = tok_Label;
  char token[TOKENSIZE];

  /* Eat everything up to the ::= */

  while(type != tok_Equals)
    type = ReadNextToken(fp, token);

  return(ParseObjectIdentifier(fp, name));
}





/*
 * Parses an asn type.  This structure is ignored by this parser.
 * Returns NULL on error.
 */
static MyTokenType ParseASNType(FILE *fp, char *name, char *token)
{
  MyTokenType type;
  MyTokenType NextType;
  char NextToken[TOKENSIZE];
  int PCount;

  type = ReadNextToken(fp, token);

  if (type == tok_TextualConvention) {

    /* OLD-TC-PARSER was here */

#ifdef DEBUG_RYAN
    printf("----- Removing TC -----\n");
#endif

    /* Read everything up to SYNTAX.  Then, read it.
     */

    while((type = ReadNextToken(fp, token)) != tok_EndOfFile) {
      if (type == tok_Syntax)
	break;
    }

    /* BEGIN EATING SYNTAX FIELD OF UNKNOWN LENGTH.  GRN */

    /* Read the initial syntax.  It's either:
     *
     * A 1-word textual convention
     * A 1-word textual convention followed by size info
     * A 2-word type (Octet String, Object Identifier)
     * A 3-word type (SEQUENCE OF blah)
     */

    type     = ReadNextToken(fp, token);
    NextType = ReadNextToken(fp, NextToken);

    if (type == tok_Object) {
      if ((NextType == tok_Label) && (!strcmp(NextToken, WIDE("IDENTIFIER")))) {
	/* 2-word type.  Read initial size */
	NextType = ReadNextToken(fp, NextToken);
      }
    }

    else if (type == tok_Octet) {
      if ((NextType == tok_Label) && (!strcmp(NextToken, WIDE("STRING")))) {
	/* 2-word type.  Read initial size */
	NextType = ReadNextToken(fp, NextToken);
      }
    }

    else if (type == tok_Sequence) {
      if ((NextType == tok_Label) && (!strcmp(NextToken, WIDE("OF")))) {
	/* 3-word type.  Fetch next, and then we're ready to look at the
	 * size info. 
	 */
	NextType = ReadNextToken(fp, NextToken);
      }
    }

    else {
      /* Either a 1-word type, or something unknown.  In all cases, we're ready
       * to parse the size. 
       */
    }

    /* ASSERT:  Read all size info.  NextType/Token contains the next 
     * item.
     */
    PCount = 0;
    if ((NextType == tok_LeftParen) ||
	(NextType == tok_LeftBracket)) {
      PCount++;
      while (PCount) {
	NextType = ReadNextToken(fp, NextToken);
	if ((NextType == tok_LeftParen) || 
	    (NextType == tok_LeftBracket)) PCount++;
	if ((NextType == tok_RightParen) || 
	    (NextType == tok_RightBracket)) PCount--;
      }
      NextType = ReadNextToken(fp, NextToken);
    }
    /* ASSERT: NextType/Token contain the next non-TC value */

    /* END EATING SYNTAX FIELD OF UNKNOWN LENGTH.  GRN */


#ifdef DEBUG_RYAN
    printf("----- Done Removing TC -----\n");
#endif

    /* Setup for return */
    strncpy(name, NextToken, TOKENSIZE);
    type = ReadNextToken(fp, token);
    return(type);

  } else if ((type == tok_Octet) ||
	     (type == tok_Integer)) {

    /* SMIv1 Textual Convention. 
     *
     * IPXaddress  ::= OCTET STRING (SIZE(10))
     * EntryStatus ::= INTEGER { ... }
     *
     * Just ignore it. 
     */
    if (type == tok_Octet)
      NextType = ReadNextToken(fp, NextToken); /* STRING */

    /* Check for a size */
    NextType = ReadNextToken(fp, NextToken);
    PCount = 0;
    if ((NextType == tok_LeftParen) ||
	(NextType == tok_LeftBracket)) {
      PCount++;
      while (PCount) {
	NextType = ReadNextToken(fp, NextToken);
	if ((NextType == tok_LeftParen) || 
	    (NextType == tok_LeftBracket)) PCount++;
	if ((NextType == tok_RightParen) || 
	    (NextType == tok_RightBracket)) PCount--;
      }
      NextType = ReadNextToken(fp, NextToken);
    }

    /* ASSERT: NextType/Token contain the next non-TC value */

    /* Setup for return */
    strncpy(name, NextToken, TOKENSIZE);
    type = ReadNextToken(fp, token);
    return(type);

  } else if (type == tok_Label) {

    /* SMIv1 Textual Convention.
     *
     * OwnerString ::= DisplayString
     *
     * Just ignore it.
     */
    return(tok_Continue); /* Continue on to the next token */

  } else if (type == tok_Sequence) {

    while((type = ReadNextToken(fp, token)) != tok_EndOfFile) {
      if (type == tok_RightBracket)
	    return(tok_Continue); /* Just not tok_Continue */
    }
    Error("ParseASNType : Sequence : Expected \"}\", read %s\n", token);
    return(tok_EndOfFile); /* Just not tok_Continue */

  } else {
    Error("Unable to parse ASN type %s (%s ::= %s)\n",
	  token, name, token);
    return(tok_EndOfFile); /* Just not tok_Continue */
  }

}


/*
 * Parse an entry of the form:
 *
 * label TRAP-TYPE
 *    ENTERPRISE label
 *    VARIABLES { ... }
 *    DESCRIPTION " ... "
 *    ::= number
 *
 * The "label TRAP-TYPE" portion has already been parsed.
 *
 * Returns 0 on error.
 */
static int ParseTrapType(FILE *fp)
{
  MyTokenType type = tok_Label;
  char token[TOKENSIZE];

  /* Read up to the description.
   */
  while(type != tok_Description)
    type = ReadNextToken(fp, token);

  /* Now eat the description */
  EatDescriptionString(fp);

  /* Eat up to the equals */
  if (type != tok_Equals)
    type = ReadNextToken(fp, token);

  /* Finally, read the number
   */
  EAT_TOKEN(tok_Number, WIDE("TrapType Value"), 0);

  return(1);
}


/* ------------------------------------------------------------------*/

/* Parse the file into a bunch of nodes */




/*
 * Parses a mib file and returns a linked list of nodes found in the file.
 * Returns NULL on error.
 */

static struct node *parse(FILE *fp, struct node *pStart)
{
   char token[TOKENSIZE];
   char name[TOKENSIZE];
   MyTokenType type = tok_Label; /* Anything but EndOfFile */
   struct node *root;
   struct node *np;

   /* Setup tokenizer */
   hash_Initialize();

   if( pStart )
	   root = np = pStart;
   else
	   root = np = node_New();

   while (type != tok_EndOfFile) {

ReadyToReadNextToken:
#ifdef DEBUG_RYAN
      fprintf(stdout, WIDE("PARSER:  Top of the loop\n"));
#endif
      type = ReadNextToken(fp, token);

      if (type != tok_Label) {

      /* Almost always a label.  Exceptions:
         tok_Include
       */
         /* End of file */

         // JAB : Include Extensions... maybe this will work(?)
         if( type == tok_Include )
         {
            //---vvv--- JAB : FILE INCLUSION EXTENSION 
            #pragma warning( disable: 4013 )
		           // JAB : Added functions to move 'current directory'
		           // to the location specified so that 'include' directive
		           // work without specifying the complete path
		           // assuming all mibs are together, or are relative
		           // to each other ( ascend/blah.mib ) would go into a sub-directory
		           // and (../routers/ascent/blah.mib) would also relatively work...
           char *PathName;
           char SaveCurrent[256];
           char WorkCurrent[256];
           char *pFile;
         //  _asm int 3;
           ReadNextToken( fp, WorkCurrent );  // a " ?
           ReadNextToken( fp, WorkCurrent );  // the name??
//           strcpy( WorkCurrent, filename );
           PathName = WorkCurrent;
           if( ( pFile = strrchr( WorkCurrent, '\\' ) ) ||
		         ( pFile = strrchr( WorkCurrent, '/' ) ) )
           {
              *pFile = 0; // terminate path...
	           pFile++; // point at 'file name'
     
              getcwd( SaveCurrent, sizeof( SaveCurrent ) );
              chdir( PathName );
           }
           else
           {
              SaveCurrent[0] = 0;
              pFile = WorkCurrent; // file name ONLY at this point...
           }
           //---^^^--- JAB : FILE INCLUSION EXTENSION 

           fp = fopen(pFile, WIDE("r"));
           if (fp == NULL)
             return(NULL);
           parse( fp, root );

           //---vvv--- JAB : FILE INCLUSION EXTENSION 
           if( strlen( SaveCurrent ) )
           {
              chdir( SaveCurrent );
           }
         #pragma warning( default: 4013 )
           //---^^^--- JAB : FILE INCLUSION EXTENSION 
         }

         if (type == tok_EndOfFile)
      	   return(root);

         /* Imports */
         else if (type == tok_Imports) {
	         /* Skip it entirely for nwo. */
      	   while(!( (type==tok_Label) && (token[strlen(token)-1]==';')))
	            type = ReadNextToken(fp, token);
	
	         goto ReadyToReadNextToken;
         }

      /* Don't worry about these, as they are probably being defined. */
         else if ((type == tok_TextualConvention) ||
	               (type == tok_ModuleIdentity) ||
	               (type == tok_ObjectGroup) ||
	               (type == tok_NotificationGroup)) {
         }

         else {
	         Error("Label is using a reserved word: %s\n", token);
	         return(NULL);
         }
    } /* End of possible label exceptions */

    else {
      /* Standalone labels */
      if (!strcmp(token, WIDE("END"))) {
	goto ReadyToReadNextToken;
      }
    }



    /* Keep track of this name */
    strncpy(name, token, TOKENSIZE);

    /* Read the next token */
    type = ReadNextToken(fp, token);

  JustReadSecondToken:


    /* ------------------------------------------------------ */

    if (type == tok_Definitions) {

      /* MIB Definitions.
       */
      /* XXXXX -- Keep track of current MIB for new nodes? */

      if (VerboseMIB)
	      fprintf(stdout, WIDE("Loading MIB '%s'\n"), name);

      EAT_TOKEN(tok_Equals, WIDE("DEFINITIONS ::= BEGIN"), NULL);
      EAT_TOKEN(tok_Label,  "DEFINITIONS ::= BEGIN", NULL);
      if (strcmp(token, WIDE("BEGIN"))) {
	      Error("Label should have been BEGIN: %s", token);
	      return(NULL);
      }
    } /* End of tok_Definitions */

    /* ------------------------------------------------------ */

    else if (type == tok_Object) {
      EAT_TOKEN(tok_Label, WIDE("OBJECT ?? Identifier ??"), NULL);
      if (strcmp(token, WIDE("IDENTIFIER"))) {
	Error("Expected IDENTIFIER");
	return(NULL);
      }

      /* org OBJECT IDENTIFIER ::= { iso 3 } */
#ifdef DEBUG_RYAN
printf("== OBJECT IDENTIFIER\n");
#endif

      EAT_TOKEN(tok_Equals, WIDE("OBJECT IDENTIFIER ?? ::= ??"), NULL);

      np->Next = ParseObjectIdentifier(fp, name);
      if (np->Next == NULL) {
	Error("Unable to parse ObjectIdentifier");
	return(NULL);
      }

      /* now find end of chain */
      while(np->Next) { 
#ifdef DEBUG_RYAN
	if(np->Label)printf("--%s--\n", np->Label); else printf("-- NONE --\n"); 
#endif /* DEBUG_RYAN */
	np=np->Next; 
      };
    } /* End of tok_Object */

    /* ------------------------------------------------------ */

    else if (type == tok_ObjectIdentity) {

#ifdef DEBUG_RYAN
printf("== OBJECT IDENTITY\n");
#endif
      np->Next = ParseObjectIdentity(fp, name);
      if (np->Next == NULL) {
	Error("Unable to parse ObjectIdentity");
	return(NULL);
      }

      /* now find end of chain */
      while(np->Next) { 
#ifdef DEBUG_RYAN
	if(np->Label)printf("--%s--\n", np->Label); else printf("-- NONE --\n"); 
#endif /* DEBUG_RYAN */
	np=np->Next; 
      };
    } /* End of tok_ObjectIdentity */

    /* ------------------------------------------------------ */

    else if (type == tok_ModuleIdentity) {

#ifdef DEBUG_RYAN
printf("== MODULE IDENTITY\n");
#endif
      np->Next = ParseModuleIdentity(fp, name);
      if (np->Next == NULL) {
	Error("Unable to parse ModuleIdentity");
	return(NULL);
      }

      /* now find end of chain */
      while(np->Next) { 
#ifdef DEBUG_RYAN
	if(np->Label)printf("--%s--\n", np->Label); else printf("-- NONE --\n"); 
#endif /* DEBUG_RYAN */
	np=np->Next; 
      };
    } /* End of tok_ModuleIdentity */

    /* ------------------------------------------------------ */

    else if (type == tok_ObjectType) {

#ifdef DEBUG_RYAN
printf("== OBJECT TYPE\n");
#endif
      np->Next = ParseObjectType(fp, name);
      if (np->Next == NULL) {
	Error("Unable to parse ObjectType");
	return(NULL);
      }

      /* now find end of chain */
      while(np->Next) { 
#ifdef DEBUG_RYAN
	if(np->Label)printf("--%s--\n", np->Label); else printf("-- NONE --\n"); 
#endif /* DEBUG_RYAN */
	np=np->Next; 
      };
    } /* End of tok_ObjectType */

    /* ------------------------------------------------------ */

    else if (type == tok_NotificationType) {

#ifdef DEBUG_RYAN
printf("== NOTIFICATION TYPE\n");
#endif
      np->Next = ParseNotificationType(fp, name);
      if (np->Next == NULL) {
	Error("Unable to parse NotificationType");
	return(NULL);
      }

      /* now find end of chain */
      while(np->Next) { 
#ifdef DEBUG_RYAN
	if(np->Label)printf("--%s--\n", np->Label); else printf("-- NONE --\n"); 
#endif /* DEBUG_RYAN */
	np=np->Next; 
      };
    } /* End of tok_NotificationType */

    /* ------------------------------------------------------ */

    else if (type == tok_TrapType) {

#ifdef DEBUG_RYAN
      printf("== TRAP TYPE\n");
#endif
      if (ParseTrapType(fp) == 0)
	return(NULL);

    } /* End of tok_TrapType */

    /* ------------------------------------------------------ */

    else if (type == tok_ObjectGroup) {

#ifdef DEBUG_RYAN
printf("== OBJECT GROUP\n");
#endif
      np->Next = ParseObjectGroup(fp, name);
      if (np->Next == NULL) {
	Error("Unable to parse ObjectGroup");
	return(NULL);
      }

      /* now find end of chain */
      while(np->Next) { 
#ifdef DEBUG_RYAN
	if(np->Label)printf("--%s--\n", np->Label); else printf("-- NONE --\n"); 
#endif /* DEBUG_RYAN */
	np=np->Next; 
      };
    } /* End of tok_ObjectGroup */

    /* ------------------------------------------------------ */

    else if (type == tok_ModuleCompliance) {

#ifdef DEBUG_RYAN
printf("== MODULE COMPLIANCE\n");
#endif
      np->Next = ParseModuleCompliance(fp, name);
      if (np->Next == NULL) {
	Error("Unable to parse ModuleCompliance");
	return(NULL);
      }

      /* now find end of chain */
      while(np->Next) { 
#ifdef DEBUG_RYAN
	if(np->Label)printf("--%s--\n", np->Label); else printf("-- NONE --\n"); 
#endif /* DEBUG_RYAN */
	np=np->Next; 
      };
    } /* End of tok_ModuleCompliance */

    /* ------------------------------------------------------ */

    else if (type == tok_NotificationGroup) {

#ifdef DEBUG_RYAN
printf("== NOTIFICATION GROUP\n");
#endif
      np->Next = ParseNotificationGroup(fp, name);
      if (np->Next == NULL) {
	Error("Unable to parse NotificationGroup");
	return(NULL);
      }

      /* now find end of chain */
      while(np->Next) { 
#ifdef DEBUG_RYAN
	if(np->Label)printf("--%s--\n", np->Label); else printf("-- NONE --\n");
#endif /* DEBUG_RYAN */
	np=np->Next; 
      };

    } /* End of tok_NotificationGroup */

    /* ------------------------------------------------------ */

    else if (type == tok_Equals) {

      type = ParseASNType(fp, name, token);

      if (type != tok_Continue) {
	/* We must have read the beginning of something.  Jump
	 * back to the appropriate spot.
	 */

	goto JustReadSecondToken;

      }
    }

    /* ------------------------------------------------------ */

    else if (type == tok_Label) {

      /* Label could be numerous things. */

      if (!strcmp(token, WIDE("MACRO"))) {
	/* Skip MACROs.  I'm not writing something to handle them. */
	while(!( (type==tok_Label) && (!strcmp(token, WIDE("END")))))
	  type = ReadNextToken(fp, token);
      }

      else {
	Error("Unknown label - label: %s %s\n", name, token);
	return(NULL);
      }

    } /* End of tok_Label */

    /* ------------------------------------------------------ */

    else {
      Error("Unknown token type - %d %s, after %s\n", type, token, name);
      return(NULL);
    }


  } /* End of master loop */
  return(root);
}

/* ------------------------------------------------------------------*/

/* For compatibility with functions in mib.c and old parser */

static int TranslateType(MyTokenType t)
{
  switch(t) {
  /* Basic ASN Types */
  case tok_Integer: return(TYPE_INTEGER); break;
  case tok_Octet: return(TYPE_OCTETSTR); break;
  case tok_Object: return (TYPE_OBJID); break;
  case tok_Null: return(TYPE_NULL); break;

  /* Basic SMI Types, defined in RFC 1902 */
  case tok_IPAddress: return(TYPE_IPADDR); break;
  case tok_Gauge32: return(TYPE_GAUGE); break;
  case tok_Counter32: return(TYPE_COUNTER); break;
  case tok_Opaque: return(TYPE_OPAQUE); break;
    /*    tok_Counter64,*/
  case tok_TimeTicks: return(TYPE_TIMETICKS); break;

  default: return(TYPE_OTHER); break;
  }
}

/* ------------------------------------------------------------------*/

/* Turn the nodes into the real list */

/*
 * Find all the children of root in the list of nodes.  Link them into the
 * tree and out of the nodes list.
 */
static void do_subtree(struct snmp_mib_tree *root, struct node **nodes)
{
  struct snmp_mib_tree *tp;
  struct snmp_mib_tree *peer = NULL;
  struct node *np, **headp;
  struct node *oldnp = NULL, *child_list = NULL, *childp = NULL;
  char *cp;
  int hash;
    
    tp = root;
    hash = 0;
    for(cp = tp->label; *cp; cp++)
        hash += *cp;
    headp = &NodeBuckets[NBUCKET(hash)];
    /*
     * Search each of the nodes for one whose parent is root, and
     * move each into a separate list.
     */
    for(np = *headp; np; np = np->Next){
	if ((*tp->label != *np->Parent) || 
	    strcmp(tp->label, np->Parent)) {

	  if ((tp->label) &&
	      (np->Label) &&
	      (*tp->label == *np->Label) && 
	      (!strcmp(tp->label, np->Label))) {
		/* if there is another node with the same label, assume that
		 * any children after this point in the list belong to the other node.
		 * This adds some scoping to the table and allows vendors to
		 * reuse names such as "ip".
		 */
		break;
	    }
	    oldnp = np;
	} else {
	    if (child_list == NULL){
		child_list = childp = np;   /* first entry in child list */
	    } else {
		childp->Next = np;
		childp = np;
	    }
	    /* take this node out of the node list */
	    if (oldnp == NULL){
		*headp = np->Next;  /* fix root of node list */
	    } else {
		oldnp->Next = np->Next;	/* link around this node */
	    }
	}
    }
    if (childp)
	childp->Next = 0;	/* re-terminate list */
    /*
     * Take each element in the child list and place it into the tree.
     */
    for(np = child_list; np; np = np->Next){
	tp = (struct snmp_mib_tree *)malloc(sizeof(struct snmp_mib_tree));
   memset( tp, 0, sizeof( struct snmp_mib_tree ) );

	tp->parent = root;
	tp->next_peer = NULL;
	tp->child_list = NULL;
	if (np->Label)
	  strcpy(tp->label, np->Label);
	else
	  memset(tp->label, '\0', 64);
	tp->subid = np->SubID;
	tp->type = TranslateType(np->Type);
	tp->enums = np->enums;
	np->enums = NULL;	/* so we don't free them later */
	if (root->child_list == NULL){
	    root->child_list = tp;
	} else {
	    peer->next_peer = tp;
	}
	peer = tp;
/*	if (tp->type == TYPE_OTHER) */
	    do_subtree(tp, nodes);	/* recurse on this child if it isn't an end node */
    }
    /* free all nodes that were copied into tree */
    oldnp = NULL;
    for(np = child_list; np; np = np->Next){
	if (oldnp)
	  node_Free(oldnp);
	oldnp = np;
    }
    if (oldnp)
      node_Free(oldnp);
}


/* Build the MIB tree from all of our nodes */
static struct snmp_mib_tree *build_tree(struct node *nodes)
{
  //struct node *np;
  struct snmp_mib_tree *tp;
  int bucket, nodes_left = 0;
    
  /* build root node */
  tp = (struct snmp_mib_tree *)malloc(sizeof(struct snmp_mib_tree));
  memset(tp, '\0', sizeof(struct snmp_mib_tree));

  strcpy(tp->label, WIDE("iso"));
  tp->subid = 1;
  tp->type = 0;

  /*    build_translation_table(); XXXXX */

  /* grow tree from this root node */
  init_node_hash(nodes);
  
  /* XXX nodes isn't needed in do_subtree() ??? */
  do_subtree(tp, &nodes);

  /* If any nodes are left, the tree is probably inconsistent */
  for(bucket = 0; bucket < NHASHSIZE; bucket++){
    if (NodeBuckets[bucket]){
      nodes_left = 1;
      break;
    }
  }
   
  if (nodes_left){
#ifdef STDERR_OUTPUT
    fprintf(stderr, WIDE("The mib description doesn't seem to be consistent.\n"));
    fprintf(stderr, WIDE("Some nodes couldn't be linked under the \")iso\" tree.\n");
    fprintf(stderr, WIDE("these nodes are left:\n"));
    for(bucket = 0; bucket < NHASHSIZE; bucket++){
      for(np = NodeBuckets[bucket]; np; np = np->Next)
	 fprintf(stderr, WIDE("%s ::= { %s %d } (%d)\n"),
		np->Label, np->Parent, np->SubID,
		np->Type);
    }
#endif
  }
  return tp;
}


/* ------------------------------------------------------------------*/

struct snmp_mib_tree *read_mib_v2(char *filename)
{
  FILE *fp;
  struct node *nodes, *np;
  struct snmp_mib_tree *tree;

  //---vvv--- JAB : FILE INCLUSION EXTENSION 
#pragma warning( disable: 4013 )
		  // JAB : Added functions to move 'current directory'
		  // to the location specified so that 'include' directive
		  // work without specifying the complete path
		  // assuming all mibs are together, or are relative
		  // to each other ( ascend/blah.mib ) would go into a sub-directory
		  // and (../routers/ascent/blah.mib) would also relatively work...
  char *PathName;
  char SaveCurrent[256];
  char WorkCurrent[256];
  char *pFile;
//  _asm int 3;
  strcpy( WorkCurrent, filename );
  PathName = WorkCurrent;
  if( ( pFile = strrchr( WorkCurrent, '\\' ) ) ||
		( pFile = strrchr( WorkCurrent, '/' ) ) )
  {
     *pFile = 0; // terminate path...
	  pFile++; // point at 'file name'
     
     getcwd( SaveCurrent, sizeof( SaveCurrent ) );
     chdir( PathName );
  }
  else
  {
     SaveCurrent[0] = 0;
     pFile = WorkCurrent; // file name ONLY at this point...
  }
  //---^^^--- JAB : FILE INCLUSION EXTENSION 

  fp = fopen(pFile, WIDE("r"));
  if (fp == NULL)
    return(NULL);

  nodes = parse(fp, NULL);

  if (!nodes){
#ifdef STDERR_OUTPUT
    fprintf(stderr, WIDE("Mib table is bad.\n"));
#endif
    return(NULL);
  }

  //---vvv--- JAB : FILE INCLUSION EXTENSION 
  if( strlen( SaveCurrent ) )
  {
     chdir( SaveCurrent );
  }
#pragma warning( default: 4013 )
  //---^^^--- JAB : FILE INCLUSION EXTENSION 


  /* Remove top dummy node */
  np = nodes->Next;
  node_Free(nodes);

  tree = build_tree(np);
  fclose(fp);
  return(tree);
}

void tree_Free(struct snmp_mib_tree *p)
{
  enum_Free(p->enums);
  free(p);
}

SNMP_NAMESPACE_END
// $Log: $
