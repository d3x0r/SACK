//----------------------------------------------------------------------
// Expressions parser, processor
//   Limitation - handles only constant expressions
//   Limitation - expects expression to be on one continuous line
//                which is acceptable for the cpp layer.
//----------------------------------------------------------------------
#define C_PRE_PROCESSOR
#include <stdhdrs.h> // debug only...
#include <math.h>
#include <json_emitter.h> 

#include "json.h"

#define LONGEST_INT uint64_t
#define LONGEST_FLT double

//typedef OPNODE OPNODE;

#ifdef __cplusplus 
#  define O_O v.
#else
#  define O_O
#endif


typedef struct vesl_accumulator {
	struct json_value_container
#ifdef __cplusplus
		v
#endif
		;
	//struct vesl_op_node *parent;
	PDATALIST localData; // list of OPNODE
	PLIST reusable; // anononymous values from localData that are re-usable.
	struct vesl_accumulator *last_value;
} *ACCUMULATOR, accumulator;

typedef struct vesl_op_node {
	struct json_value_container
#ifdef __cplusplus
		v
#endif
		;
	//struct vesl_op_node *parent;
	//PDATALIST localData; // list of OPNODE
	//PLIST reusable;
	//struct vesl_op_node *last_value;
} *OPNODE, opnode;

#define MAXaccumulatorSPERSET 256
DeclareSet( accumulator );
#define MAXopnodeSPERSET 256
DeclareSet( opnode );


#define right(n)  ((OPNODE)GetDataItem( &(n)->O_O contains, 0 ))
#define left(n)  ((n)->last_value)
//#define right(n)  (GetDataItemAddress( OPNODE*, (n)->O_O contains, 1 ))[0]

//#define left_l(n)  ((OPNODE*)GetDataItemAddress( &(n)->O_O contains, 0 ))[0]
//#define right_l(n)  ((OPNODE*)GetDataItemAddress( &(n)->O_O contains, 1 ))[0]

struct vesl_code_parser_state {
	PTEXT input;
	PTEXT _input; // original input start release when input == NULL;
	char *file; // 'filename' of the code 
	int line; // current line counter;
	int col;  // current col counter; // ??
	OPNODE root;
	PopnodeSET nodes;
	PaccumulatorSET closures;
	LOGICAL findElse;
	LOGICAL skipElse;
};


static char pHEX[] = "0123456789ABCDEF";
static char phex[] = "0123456789abcdef";

/*
Javascript operator precedence ( vs C precedence defined below and initially 
for this code... 

20	Grouping	n/a	( ... )
19	Member Access	left-to-right	... . ...
	Computed Member Access	left-to-right	... [ ... ]
	new (with argument list)	n/a	new ... ( ... )
	Function Call	left-to-right	... ( ... )
18	new (without argument list)	right-to-left	new ...
17	Postfix Increment	n/a	... ++
	Postfix Decrement	... --
16	Logical NOT	right-to-left	! ...
	Bitwise NOT	~ ...
	Unary Plus	+ ...
	Unary Negation	- ...
	Prefix Increment	++ ...
	Prefix Decrement	-- ...
	typeof	typeof ...
	void	void ...
	delete	delete ...
	await	await ...
15	Exponentiation	right-to-left	... ** ...
14	Multiplication	left-to-right	... * ...
	Division	... / ...
	Remainder	... % ...
13	Addition	left-to-right	... + ...
	Subtraction	... - ...
12	Bitwise Left Shift	left-to-right	... << ...
	Bitwise Right Shift	... >> ...
	Bitwise Unsigned Right Shift	... >>> ...
11	Less Than	left-to-right	... < ...
	Less Than Or Equal	... <= ...
	Greater Than	... > ...
	Greater Than Or Equal	... >= ...
in	... in ...
	instanceof	... instanceof ...
10	Equality	left-to-right	... == ...
	Inequality	... != ...
	Strict Equality	... === ...
	Strict Inequality	... !== ...
9	Bitwise AND	left-to-right	... & ...
8	Bitwise XOR	left-to-right	... ^ ...
7	Bitwise OR	left-to-right	... | ...
6	Logical AND	left-to-right	... && ...
5	Logical OR	left-to-right	... || ...
4	Conditional	right-to-left	... ? ... : ...
3	Assignment	right-to-left	... = ...
	... += ...
	... -= ...
	... **= ...
	... *= ...
	... /= ...
	... %= ...
	... <<= ...
	... >>= ...
	... >>>= ...
	... &= ...
	... ^= ...
	... |= ...
2	yield	right-to-left	yield ...
	yield*	yield* ...
1	Comma / Sequence	left-to-right	... , ...


 Section Category Operators
*.*   First should of course be ( ) 
7.5 Primary           x.y  f(x)  a[x]  x++  x--  new typeof  checked  unchecked
7.6 Unary             +  -  !  ~  ++x  --x  (T)x
7.7 Multiplicative    *  /  %
7.7 Additive          +  -
7.8 Shift             <<  >>
7.9 Relational and type testing <  >  <=  >=  is  as
7.9 Equality          ==  !=
7.10 Logical      AND &
7.10 Logical      XOR ^
7.10 Logical      OR  |
7.11 Conditional  AND &&
7.11 Conditional  OR ||

// both of these are right-associative
7.12 Conditional     ?:
7.13 Assignment      =  *=  /=  %=  +=  -=  <<=  >>=  &=  ^=  |=
*/

/*
 Section Category Operators (relevant to preprocessor)
 7.5 Primary           a[x] // should apply to const strings
                            // (results in an operand anyhow)
7.6 Unary             +  -  !  ~  // typecast? (T)x
7.7 Multiplicative    *  /  %
7.7 Additive          +  -
7.8 Shift             <<  >>
7.9 Relational and type testing <  >  <=  >=  is  as
7.9 Equality          ==  !=
7.10 Logical      AND &
7.10 Logical      XOR ^
7.10 Logical      OR  |
7.11 Conditional  AND &&
7.11 Conditional  OR ||

// both of these are right-associative
7.12 Conditional     ?:
*/


enum { OP_HANG = VALUE_OP_BASE + 20 /*-1*/  // used to indicate prior value_type complete, please hang on tree
		// after hanging, please re-check current symbol
	, OP_UNSET = VALUE_OP_BASE + 21 /* 0 */
	, OP_NOOP = VALUE_UNSET /* previously 1; is now 0 */
     , OP_SUBEXPRESSION = VALUE_OP_BASE + 21 /* 2 */  // (...)
	, OP_IS_MATH = 0x100

     , OP_SETEQUAL                     // =  equality
     , OP_ISEQUAL                      // == comparison

     , OP_PLUS                         // +
     , OP_INCREMENT                    // ++
     , OP_PLUSEQUAL                    // +=

     , OP_MINUS                        // -
     , OP_DECREMENT                    // --
     , OP_MINUSEQUAL                   // -=

     , OP_MULTIPLY                     // *
     , OP_MULTIPLYEQUAL                // *=
     , OP_MOD                          // %
     , OP_MODEQUAL                     // %=
     , OP_DIVIDE                       // /
     , OP_DIVIDEEQUAL                  // /=

     , OP_XOR                          // ^
     , OP_XOREQUAL                     // ^=

     , OP_BINARYNOT                    // ~
     , OP_LOGICALNOT                   // !
     , OP_NOTEQUAL                     // !=

     , OP_GREATER                      // >
     , OP_SHIFTRIGHT                   // >>
     , OP_GREATEREQUAL                 // >=
     , OP_SHREQUAL                     // >>=

     , OP_LESSER                       // <
     , OP_SHIFTLEFT                    // <<
     , OP_LESSEREQUAL                  // <=
     , OP_SHLEQUAL                     // <<=

     , OP_BINARYAND                    // &
     , OP_NAND                         // !&
     , OP_LOGICALAND                   // &&
     , OP_ANDEQUAL                     // &=

     , OP_BINARYOR                     // |
     , OP_NOR                          // !|
     , OP_LOGICALOR                    // ||
     , OP_OREQUAL                      // |=

     , OP_COMPARISON                   // ?
     , OP_ELSE_COMPARISON					// :
     , OP_COMMA
	//, OP_DOT
     //, OP_
};

char *fullopname[] = { "noop", WIDE("sub-expr")
                     ,  "uint8_t",  "uint16_t",  "uint32_t",  "uint64_t" // unsigned int
                     , WIDE("int8_t"), WIDE("int16_t"), WIDE("int32_t"), WIDE("int64_t") // signed int
                     , WIDE("float"), WIDE("double") // float ops
                     , WIDE("string"), WIDE("character")
                     , WIDE("="), WIDE("==")
                     , WIDE("+"), WIDE("++"), WIDE("+=")
                     , WIDE("-"), WIDE("--"), WIDE("-=")
                     , WIDE("*"), WIDE("*=")
                     , WIDE("%"), WIDE("%=")
                     , WIDE("/"), WIDE("/=")
                     , WIDE("^"), WIDE("^=")
                     , WIDE("~")
                     , WIDE("!"), WIDE("!=")
                     , WIDE(">"), WIDE(">>"), WIDE(">="), WIDE(">>=")
                     , WIDE("<"), WIDE("<<"), WIDE("<="), WIDE("<<=")
                     , WIDE("&"), WIDE("&&"), WIDE("&=")
                     , WIDE("|"), WIDE("||"), WIDE("|=")
                     , WIDE("?"), WIDE(":"), WIDE(",")
                     };


struct keyword {
	int thisop;
	const char *word;
};
#define NUM_KEYWORDS (sizeof(Keywords)/sizeof(struct keyword))

struct keyword Keywords[] = { { VALUE_OP_DO, "do" }
                            , { VALUE_OP_BREAK, "break" }
                            , { VALUE_OP_CASE, "case" }
                            , { VALUE_OP_SWITCH, "switch" }
                            , { VALUE_OP_WHILE, "while" }
                            , { VALUE_OP_IF, "if" }
                            , { VALUE_OP_BREAK, "break" }
                            , { VALUE_OP_STOP, "stop" }
                            , { VALUE_OP_THIS, "this" }
                            , { VALUE_OP_BASE, "base" }
                            , { VALUE_OP_CONTINUE, "continue" }
                            };

typedef struct relation RELATION, *PRELATION;

struct relation {
	int thisop;
	struct {
		char ch;
		int becomes;
	}trans[16];
};

#define NUM_RELATIONS (sizeof(Relations)/sizeof(RELATION))

RELATION Relations[] = { { OP_NOOP      , { { '=', OP_SETEQUAL }
                                          , { '<', OP_LESSER }
                                          , { '>', OP_GREATER }
                                          , { '+', OP_PLUS }
                                          , { '-', OP_MINUS }
                                          , { '*', OP_MULTIPLY }
                                          , { '/', OP_DIVIDE }
                                          , { '%', OP_MOD }
                                          , { '^', OP_XOR }
                                          , { '~', OP_BINARYNOT }
                                          , { '!', OP_LOGICALNOT }
                                          , { '&', OP_BINARYAND }
                                          , { '|', OP_BINARYOR }
                                          , { '?', OP_COMPARISON }
                                          , { ':', OP_ELSE_COMPARISON }
                                          , { ',', OP_COMMA } } }
                       , { OP_SETEQUAL  , { { '=', OP_ISEQUAL } } }
                       , { OP_PLUS      , { { '+', OP_INCREMENT }
                                          , { '=', OP_PLUSEQUAL } } }
                       , { OP_MINUS     , { { '-', OP_DECREMENT }
                                          , { '=', OP_MINUSEQUAL } } }
                       , { OP_MULTIPLY  , { { '=', OP_MULTIPLYEQUAL } } }
                       , { OP_MOD       , { { '=', OP_MODEQUAL } } }
                       , { OP_DIVIDE    , { { '=', OP_DIVIDEEQUAL } } }
                       , { OP_XOR       , { { '=', OP_XOREQUAL } } }
                       , { OP_LOGICALNOT, { { '=', OP_NOTEQUAL }
                                          , { '>', OP_LESSEREQUAL }
                                          , { '<', OP_GREATEREQUAL }
                                          , { '&', OP_NAND }
                                          , { '|', OP_NOR }
                                          } }
                       , { OP_GREATER   , { { '>', OP_SHIFTRIGHT }
                                          , { '=', OP_GREATEREQUAL } } }
                       , { OP_SHIFTRIGHT, { { '=', OP_SHREQUAL } } }
                       , { OP_LESSER    , { { '<', OP_SHIFTLEFT }
                                          , { '=', OP_LESSEREQUAL } } }
                       , { OP_SHIFTLEFT , { { '=', OP_SHLEQUAL } } }
                       , { OP_BINARYAND , { { '&', OP_LOGICALAND }
                                          , { '=', OP_ANDEQUAL } } }
                       , { OP_BINARYOR  , { { '|', OP_LOGICALOR }
                                          , { '=', OP_OREQUAL } } }
                       };

struct sequence {
	int opFrom;
	int opTo[17];
} Sequences[] = { { VALUE_OP_DO, OP_SUBEXPRESSION }
	, { VALUE_NUMBER, { OP_NOOP, OP_IS_MATH } }
	
};


//--------------------------------------------------------------------------

void ExpressionBreak( OPNODE breakbefore )
{
	DeleteDataList( &breakbefore->O_O contains );
}

//--------------------------------------------------------------------------

static const char *GetCurrentFileName( struct vesl_code_parser_state *l ) {
	return "(buffer)";
}
static int GetCurrentLine( struct vesl_code_parser_state *l ) {
	return 123;
}

static PTEXT GetCurrentWord( struct vesl_code_parser_state *l ) {
	lprintf( "have to get words from a buffer somewhere..." );
	return l->input;
}

static void StepCurrentWord( struct vesl_code_parser_state *l ) {
	PTEXT next = l->input;
	l->input = NEXTLINE( l->input );
	if( l->input ) {
		LineRelease( l->_input );
	}
}

static OPNODE GetAccumulator( struct vesl_code_parser_state *l, ACCUMULATOR from, char *name, size_t namelen ) {
	struct vesl_accumulator newOp;
	newOp.O_O value_type = VALUE_UNSET;
	newOp.O_O name = name;
	newOp.O_O nameLen = namelen;
	newOp.O_O contains = CreateDataList( sizeof( struct vesl_accumulator ) );
	newOp.O_O _contains = NULL;
	newOp.localData = CreateDataList( sizeof( struct vesl_accumulator ) );
	newOp.last_value = from;
	if( name )
		return (OPNODE)AddDataItem( &from->O_O contains, &newOp );
	else
		return (OPNODE)AddDataItem( &from->localData, &newOp );
}

static OPNODE GetOpNode( struct vesl_code_parser_state *l, OPNODE from, char *name, size_t namelen ) {
	opnode newOp;
	newOp.O_O value_type = VALUE_UNSET;
	newOp.O_O name = name;
	newOp.O_O nameLen = namelen;
	newOp.O_O contains = CreateDataList( sizeof( opnode ) );
	newOp.O_O _contains = NULL;
	if( name )
		return (OPNODE)AddDataItem( &from->O_O contains, &newOp );
}

static void DestroyExpression( struct vesl_code_parser_state *l, OPNODE node ) {
	//OPNODE node = l->root;

	if( node )
		_json_dispose_message( &node->O_O contains );
	Release( node );
}

static void DestroyOpNode( struct vesl_code_parser_state *l,  OPNODE val ) {
	if( val )
		_json_dispose_message( &val->O_O contains );
	Release( val );
}

#if 0
OPNODE SubstNodes( OPNODE _this_left, OPNODE _this_right, OPNODE that )
{
	if( _this_left && _this_right && that )
	{
		OPNODE that_left = that;
		OPNODE that_right = that;

		while( left( that_left ) )
			that_left = left(that_left);
		while( right( that_right ) )
			that_right = right( that_right );

		if( left(_this_left) )
			right( left(_this_left)) = that_left;
		left(that_left) = left(_this_left);
		left(_this_left) = NULL;

		if( right(_this_right) )
			left(right(_this_right)) = that_right;
		right(that_right) = right(_this_right);
		right(_this_right) = NULL;
		return _this_left;
	}
	return NULL;
}

//--------------------------------------------------------------------------

OPNODE SubstNode( OPNODE _this, OPNODE that )
{
	return SubstNodes( _this, _this, that );
}
#endif

//--------------------------------------------------------------------------

#if 0
OPNODE GrabNodes( OPNODE start, OPNODE end )
{
	if( left(start) )
		right( left(start)) = right(end);
	if( right(end) )
		left(right( end ) ) = left(start);
	right(end) = NULL;
	left(start) = NULL;
	return start;
}

//--------------------------------------------------------------------------

OPNODE GrabNode( OPNODE _this )
{
	return GrabNodes( _this, _this );
}
#endif

//--------------------------------------------------------------------------

static int RelateOpNode( struct vesl_code_parser_state *l, OPNODE root, OPNODE const node )
{
	if( !node )
	{
		lprintf( WIDE("Fatal Error: cannot relate a NULL node\n") );
		return 0;
	}
	if( !root )
	{
		lprintf( WIDE("Fatal error: Cannot build expression tree with NULL root.\n") );
		return 0;
	}
#ifdef C_PRE_PROCESSOR
	switch( node->O_O value_type )
	{
	case VALUE_NUMBER:
	case OP_SETEQUAL:
	case OP_INCREMENT:
	case OP_PLUSEQUAL:
	case OP_DECREMENT:
	case OP_MINUSEQUAL:
	case OP_MULTIPLYEQUAL:
	case OP_MODEQUAL:
	case OP_XOREQUAL:
	case OP_SHREQUAL:
	case OP_SHLEQUAL:
	case OP_ANDEQUAL:
	case OP_OREQUAL:
		lprintf( WIDE("%s(%d) Error: preprocessor expression may not use operand %s\n")
		       , GetCurrentFileName( l ), GetCurrentLine(l), fullopname[ node->O_O value_type ] );
		//DestroyOpNode( node );
		return 0;
		break;
	}
#endif
	if( !root )
		lprintf( "INVALID STATE" );//*root = node;
	else
	{
		AddDataItem( &root->O_O contains, node );
	}
	return 1;
}

//--------------------------------------------------------------------------


// result : 0 = okay value
//          1 = float number ?
//          2 = invalid number...
static int GetInteger( struct vesl_code_parser_state *l, LONGEST_INT *result, int *length )
{
	TEXTCHAR *p = GetText( GetCurrentWord( l ) );
	LONGEST_INT accum = 0;
	int neg = 0;
	int unsigned_value = 0;
	int long_value = 0;

	if( p[0] >= '0' && p[0] <= '9' )
	{
  		accum = 0;
  		if( p[0] == '0' && p[1] && ( p[1] == 'x' || p[1] == 'X' ) )
  		{
  			char *hexchar;
  			int okay = 1;
  			p += 2;
  			while( p[0] && okay )
  			{
	  			if( hexchar = strchr( pHEX, p[0] ) )
				{
  					accum *= 16;
  					accum += hexchar - pHEX;
  				}
  				else if( hexchar = strchr( phex, p[0] ) )
  				{
  					accum *= 16;
  					accum += hexchar - phex;
  				}
  				else
  				{
  					okay = 0;
  					if( *p == '.' )
  					{
  						lprintf( WIDE("%s(%d) Error: Hexadecimal may not be used to define a float.\n"), GetCurrentFileName( l ), GetCurrentLine( l ) );
  						return 2; // invalid number.
  					}
  				}
				p++;
			}
	  	}
	  	else if( p[0] == '0' )
	  	{
	  		// octal
			while( p[0] >= '0' && p[0] <= '7' )
  			{
	  			accum *= 8;
	  			accum += p[0] - '0';
			 	p++;
		 	}
		 	if( p[0] == '.' )
		 	{
		 		return 1;
		 	}
		 	else if( p[0] )
		 	{
		 		if( p[0] < 32 )
			 		lprintf( WIDE("%s(%d) Error: Octal constant has invalid character 0x%02x\n"), GetCurrentFileName( l ), GetCurrentLine( l ) , p[0] );
				else
			 		lprintf( WIDE("%s(%d) Error: Octal constant has invalid character '%c'\n"), GetCurrentFileName( l ), GetCurrentLine( l ) , p[0] );
		 		return 2;
		 	}
	  	}
  		else
  		{
			while( p[0] >= '0' && p[0] <= '9' )
  			{
	  			accum *= 10;
	  			accum += p[0] - '0';
			 	p++;
		 	}
  			if( *p == '.' )
  				return 1; // invalid number... should consider as float after.
		}
  		while( *p )
  		{
			if( *p == 'U' || *p == 'u' )
			{
				if( unsigned_value )
				{
					lprintf( WIDE("%s(%d) Error: U or u qualifiers specifed more than once on a constant.\n"), GetCurrentFileName( l ), GetCurrentLine( l )  );
					return 2;
				}
				unsigned_value = 1;
				p++;
			}
			else if( p[0] == 'l' || p[0] == 'L' )
			{
				if( long_value )
				{
					lprintf( WIDE("%s(%d) Error: too many L or l qualifiers specifed on a constant.\n"), GetCurrentFileName( l ), GetCurrentLine( l )  );
					return 2;
				}
				if( p[1] && ( p[1] == 'l' || p[1] == 'L' ) )
				{
					long_value = 2;
					p++;
				}
				else
					long_value = 1;
				p++;
			}
			else
			{
				if( p[0] < 32 )
					lprintf( WIDE("%s(%d) Error: Invalid type specification 0x%02x.\n")
					       , GetCurrentFileName( l ), GetCurrentLine( l ), p[1] );
				else
					lprintf( WIDE("%s(%d) Error: Invalid type specification '%c'.\n")
					       , GetCurrentFileName(l), GetCurrentLine( l ), p[1] );
				return 2;

			}
		}
	}
	if( result )
		*result = accum;
	if( length )
		*length = long_value;
	return 0; // valid result now.
}

//--------------------------------------------------------------------------
// result 0: float value okay
//   	  1: invalid conversion
//  this may require multiple tokens to resolve... '.' '-' '+' are all
// seperate symbols.
static int GetFloat( LONGEST_FLT *result, int *length )
{
	LONGEST_FLT accum = 0;
	//lprintf( WIDE("At this time 'expr.c' does not do float conversion...\n") );
	return 0;
}

//--------------------------------------------------------------------------

void LogExpression( OPNODE root )
{
	static int level;
	level++;
	while( root )
	{
		if( root->O_O value_type == OP_SUBEXPRESSION )
		{
			lprintf( WIDE("( ") );
			LogExpression( root );
			lprintf( WIDE(" )") );
		}
		else
		{
			lprintf( WIDE("(%s = %lld)"), fullopname[root->O_O value_type],root->O_O result_n );
		}
#ifdef __LINUX__
		if( right(root) )
			if( left(root)(right) != root )
				asm( WIDE("int $3\n") );
#endif
		root = right(root);
	}
	level--;
	if( !level )
		lprintf( WIDE("\n") );
}

//--------------------------------------------------------------------------
// one might suppose that
// expressions are read ... left, current, right....
// and things are pushed off to the left and right of myself, and or rotated
// appropriately....
//
//     NULL
//     (value_type)+/-/##/(
//--------------------------------------------------------------------------

OPNODE BuildExpression(  struct vesl_code_parser_state *l, PDATALIST *container ) // expression is queued
{
	char *pExp;
	int nLastLogical = 0;
	int nResult = 0;
	int quote = 0;
	int overflow = 0;
	opnode  ThisOp;
	OPNODE branch = NULL;
	PTEXT thisword;
	//return 0; // force false output ... needs work on substitutions...

	//if( g.bDebugLog )
	//{
	//	fprintf( stddbg, WIDE("Build expression for: ") );
	//	DumpSegs( GetCurrentWord() );
	//}

	while( ( thisword = GetCurrentWord(l) ) )
	{
		int n;
		pExp = GetText( thisword );
		//printf( WIDE("word: %s\n"), pExp );
		if( pExp[0] == '\'' || pExp[0] == '\"' || pExp[0] == '`' )
		{
			if( quote )
			{
				// temporarily the string collected is a PTEXT, restore it to a char *
				PTEXT merge = BuildLine( (PTEXT)ThisOp.O_O string );
				ThisOp.O_O string = StrDup( GetText( merge ) );
				LineRelease( merge );
				overflow = 0;
				RelateOpNode( l, branch, &ThisOp );
				ThisOp.O_O value_type = VALUE_UNSET;
			  	//ThisOp.O_O value_type = VALUE_UNSET;
				quote = 0;
			}
			else if( !quote )
			{
				ThisOp.O_O value_type = VALUE_STRING;
				quote = pExp[0];
			}
			StepCurrentWord(l);
			continue;
		}

		if( quote )
		{
			if( ThisOp.O_O value_type == VALUE_STRING )
			{
				lprintf( "adding to collecting string?" );
				ThisOp.O_O string =
					(char*)SegAppend( (PTEXT)ThisOp.O_O string
					         , SegDuplicate( thisword ) );
			}
			StepCurrentWord(l);
			continue;
		}

		if( pExp[0] == '(' || pExp[0] == '{' )
		{
			OPNODE subexpression;

			if( ThisOp.O_O value_type != OP_NOOP )
			{
				//if( g.bDebugLog )
				//{
				//	fprintf( stddbg, WIDE("Adding operation: ") );
				//	LogExpression( ThisOp );
				//}
				RelateOpNode( l, branch, &ThisOp );
				ThisOp.O_O value_type = VALUE_UNSET;
			}

			StepCurrentWord(l);
			subexpression = BuildExpression(l, &ThisOp.O_O contains );
			pExp = GetText( GetCurrentWord(l) );
			if( pExp && pExp[0] != ')' )
			{
				lprintf( WIDE("(%s)%d Error: Invalid expression\n"), GetCurrentFileName(l), GetCurrentLine(l) );
				DestroyExpression( l, branch );
				//DestroyOpNode( ThisOp );
				return NULL;
				// invalid pairing of parens in expression
			}
			ThisOp.O_O value_type = (enum json_value_types)(int)OP_SUBEXPRESSION;

			//AddDataItem( &ThisOp.O_O contains, &ThisOp );
			//AddDataItem( &ThisOp.O_O contains, subexpression );

			RelateOpNode( l, branch, &ThisOp );
		  	ThisOp.O_O value_type = VALUE_UNSET;
			// pExp = GetText( GetCurrentWord() );
			// on return check current token as ')'
		}
		else if( pExp[0] == ')' )
		{
			if( ThisOp.O_O value_type != OP_NOOP ) {
				RelateOpNode( l, branch, &ThisOp );
			} else
				;// DestroyOpNode( ThisOp );
			//if( g.bDebugLog )
			//{
			//	fprintf( stddbg, WIDE("Built Expression: ")) ;
			//	LogExpression( branch );
			//}
			return branch;
		}
		else if( ( pExp[0] >= '0' && pExp[0] <= '9' ) 
		       ||( pExp[0] == '.' ) )
		{
			LONGEST_INT i;
	  		LONGEST_FLT f;
			int len;
			switch( GetInteger( l, &i, &len ) )
			{
			case 0: // good integer.
				if( ThisOp.O_O value_type != OP_NOOP )
				{
					RelateOpNode( l, branch, &ThisOp );
					ThisOp.O_O value_type = VALUE_UNSET;
				}
				ThisOp.O_O value_type = VALUE_NUMBER;
				ThisOp.O_O result_n = i;
				RelateOpNode( l, branch, &ThisOp );
				ThisOp.O_O value_type = VALUE_UNSET;
				break;
			case 1: // might be a float...
				switch( GetFloat( &f, &len ) )
				{
				case 0:

				case 1: // invalid conversion (invalid number)
					DestroyExpression( l, branch ); // also if preprocessor - this always must fail
					/////DestroyOpNode( l, ThisOp );
					//if( g.bDebugLog )
					//{
					//	fprintf( stddbg, WIDE("Built Expression 2: ")) ;
					//	LogExpression( branch );
					//}
					return branch;
				}
				break;
			case 2: // invalid number
				break;
			}
		}
		else if( pExp[0] == '_'|| pExp[0] == '$' || ( pExp[0] >= 'A' && pExp[0] <= 'Z' )
										|| ( pExp[0] >= 'a' && pExp[0] <= 'z' )  )
		{
			// this is unsubstituted, is not a predefined thing, etc,
			// therefore this is a 0.
			if( quote )
			{
			}
			else
			{
				if( ThisOp.O_O value_type != OP_NOOP )
				{
					RelateOpNode( l, branch, &ThisOp );
					ThisOp.O_O value_type = VALUE_UNSET;
				}
				ThisOp.O_O value_type = VALUE_NUMBER;
				ThisOp.O_O result_n = 0;
				RelateOpNode( l, branch, &ThisOp );
				ThisOp.O_O value_type = VALUE_UNSET;
			}
		}
		else {
			if( ThisOp.O_O value_type == OP_NOOP ) {
				for( n = 0; n < NUM_KEYWORDS; n++ ) {
					if( strcmp( Keywords[n].word, pExp ) == 0 ) {
						ThisOp.O_O value_type = (enum json_value_types)(int)Keywords[n].thisop;
						break;
					}
				}
			}
			if( !thisword->format.position.offset.spaces || ThisOp.O_O value_type == OP_NOOP )
			{
			retry_this_operator:
				for( n = 0; n < NUM_RELATIONS; n++ )
				{
				 	if( Relations[n].thisop == ThisOp.O_O value_type )
			 		{
			 			int o;
			 			for( o = 0; Relations[n].trans[o].ch; o++ )
				 		{
				 			if( Relations[n].trans[o].ch == pExp[0] )
							{
								//if( g.bDebugLog )
								//{
								//	fprintf( stddbg, WIDE("%s becomes %s\n"),
								//			  ThisOp.O_O value_type<0?"???":fullopname[ThisOp.O_O value_type], fullopname[Relations[n].trans[o].becomes] );
								//}
		 						ThisOp.O_O value_type = (enum json_value_types)(int)Relations[n].trans[o].becomes;
			 					break;
			 				}
				 		}
				 		if( !Relations[n].trans[o].ch )
			 			{
			 				//fprintf( stddbg, WIDE("Invalid expression addition\n") );
							lprintf( WIDE("%s(%d): Error invalid operator: %s\n")
							       , GetCurrentFileName(l)
							       , GetCurrentLine(l)
							       , pExp );
			 				// invalid expression addition....
			 				n = NUM_RELATIONS;
				 		}
				 		break;
				 	}
				}
			}
			else // spaces seperate operators.
				n = NUM_RELATIONS;
			// then this operator does not add to the prior operator...
			// therefore hang the old, create the new...
			if( n == NUM_RELATIONS ) // unfound
			{
				if( ThisOp.O_O value_type != OP_NOOP )
				{
					RelateOpNode( l, branch, &ThisOp );
					ThisOp.O_O value_type = VALUE_UNSET;
					goto retry_this_operator;
				}
				DestroyExpression( l, branch );
				return NULL;
			}
		}
		StepCurrentWord(l);
	}

	//if( g.bDebugLog )
	//{
	//	fprintf( stddbg, WIDE("Deleting: ") );
	//	LogExpression( ThisOp );
	//}
	/////DestroyOpNode( l, ThisOp );
	//if( g.bDebugLog )
	//{
	//	fprintf( stddbg, WIDE("Built Expression: ")) ;
	//	LogExpression( branch );
	//}
	return branch;
}

//--------------------------------------------------------------------------

OPNODE ResolveExpression( struct vesl_code_parser_state*l, PDATALIST expr, ACCUMULATOR result, ACCUMULATOR arguments );

//--------------------------------------------------------------------------

int IsValue( struct vesl_code_parser_state*l, OPNODE node, ACCUMULATOR accumulator, int collapse_sub )
{
	//OPNODE temp;
	if( !node ) {
		return FALSE;
	}
	switch( node->O_O value_type )
	{
	case VALUE_NUMBER:
	case VALUE_TRUE:
	case VALUE_FALSE:
	case VALUE_NULL:
	case VALUE_UNDEFINED:
	case VALUE_STRING: // could be?
	case VALUE_OBJECT: // could be?
	case VALUE_ARRAY: // could be?
		if( (ACCUMULATOR)node != accumulator ) {
			//( ( struct json_value_container* )accumulator )[0] = ( ( struct json_value_container* )node )[0];
			accumulator->O_O value_type = node->O_O value_type;
			accumulator->O_O name = node->O_O name;
			accumulator->O_O nameLen = node->O_O nameLen;
			accumulator->O_O string = node->O_O string;
			accumulator->O_O stringLen = node->O_O stringLen;
			accumulator->O_O float_result = node->O_O float_result;
			accumulator->O_O result_d = node->O_O result_d;
			accumulator->O_O result_n = node->O_O result_n;
			// contains should not be copied.... 
		}
		return TRUE;

	case OP_SUBEXPRESSION:
		if( accumulator && collapse_sub )
		{
			ResolveExpression( l, node->O_O contains, accumulator, NULL );
			// substitute resolved value for this node...

			if( accumulator->O_O value_type == VALUE_NUMBER ) {
				return TRUE;
			}
			//(*node)->O_O contains = NULL;
			lprintf( "this subst still needs to happen... (for output)" );
			////////DestroyOpNode( l, SubstNode( *node, temp ) );
		}
		return TRUE;
	}
	return FALSE;
}

//--------------------------------------------------------------------------

void ApplyBinaryNot( OPNODE node )
{
	if( node->O_O value_type == VALUE_NUMBER )
	{
		if( !node->O_O float_result ) {
			node->O_O result_n = ~node->O_O result_n;
		}
	}
}

//--------------------------------------------------------------------------

void ApplyLogicalNot( ACCUMULATOR result, OPNODE node )
{
	switch( node->O_O value_type )
	{
	case VALUE_TRUE:
		result->O_O value_type = VALUE_FALSE;
		break;
	case VALUE_FALSE:
		result->O_O value_type = VALUE_TRUE;
		break;
	case VALUE_NUMBER:
		if( !node->O_O float_result ) {
			node->O_O result_n = !node->O_O result_n;
		}
		else
			node->O_O result_d = !node->O_O result_d;
		break;
	default:
		lprintf( WIDE("Dunno how we got here...\n") );
	}
}

//--------------------------------------------------------------------------

void ApplyNegative( ACCUMULATOR result, OPNODE node )
{
	switch( node->O_O value_type ) {
	case VALUE_TRUE:
		result->O_O value_type = VALUE_FALSE;
		break;
	case VALUE_FALSE:
		result->O_O value_type = VALUE_TRUE;
		break;
	case VALUE_NUMBER:
		if( !node->O_O float_result ) {
			node->O_O result_n = !node->O_O result_n;
		} else
			node->O_O result_d = !node->O_O result_d;
		break;
	default:
		lprintf( WIDE("Dunno how we got here...\n") );
	}
}

//--------------------------------------------------------------------------


//--------------------------------------------------------------------------

void ApplyMultiply( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node2 )
{
	if( result->O_O value_type == VALUE_NUMBER ) {
		if( result->O_O float_result ) {
			if( node2->O_O value_type == VALUE_NUMBER ) {
				if( node2->O_O float_result )
					result->O_O result_d *= node2->O_O result_d;
				else
					result->O_O result_d *= node2->O_O result_n;
			} else {
				lprintf( "Multiple Operator unsupported Operand: *(1)" );
				
			}
		} else {
			if( node2->O_O value_type == VALUE_NUMBER ) {
				if( node2->O_O float_result ) {
					result->O_O float_result = TRUE;
					result->O_O result_d = result->O_O result_n * node2->O_O result_d;
				} else
					result->O_O result_n *= node2->O_O result_n;
			} else {
				lprintf( "Multiple Operator unsupported Operand *(2)" );
			}
		}
	} else if( result->O_O value_type == VALUE_UNSET ) {
	} else {
		lprintf( "Multiple Operator unsupported Operand *(3)" );
	}
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyDivide( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	if( node2->O_O result_n != 0 )
		result->O_O result_n = node1->O_O result_n * node2->O_O result_n;
	else
	{
		lprintf( WIDE("Right hand operator of divide is 0! - returning MAXINT\n") );
#if defined( _MSC_VER ) || defined( __WATCOMC__ )
		result->O_O result_n = 0xFFFFFFFFFFFFFFFFU;
#else
		result->O_O result_n = 0xFFFFFFFFFFFFFFFFULL;
#endif
	}
	return result;
}

//--------------------------------------------------------------------------

void ApplyModulus( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node2 )
{
	if( result->O_O value_type == VALUE_NUMBER ) {
		if( result->O_O float_result ) {
			if( node2->O_O value_type == VALUE_NUMBER ) {
				if( node2->O_O float_result ) {
					result->O_O result_d = fmod( result->O_O result_d, node2->O_O result_d );
				}  else
					result->O_O result_d = fmod( result->O_O result_d, (double)node2->O_O result_n );
			} else {
				lprintf( "Multiple Operator unsupported Operand: %(1)" );

			}
		} else {
			if( node2->O_O value_type == VALUE_NUMBER ) {
				if( node2->O_O float_result ) {
					result->O_O float_result = TRUE;
					result->O_O result_d = fmod( (double)result->O_O result_n, node2->O_O result_d );
				} else
					result->O_O result_n %= node2->O_O result_n;
			} else {
				lprintf( "Multiple Operator unsupported Operand %(2)" );
			}
		}
	} else if( result->O_O value_type == VALUE_UNSET ) {
	} else {
		lprintf( "Multiple Operator unsupported Operand %(3)" );
	}
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyShiftRight( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	result->O_O result_n = node1->O_O result_n >> node2->O_O result_n;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyShiftLeft( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	result->O_O result_n = node1->O_O result_n << node2->O_O result_n;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyGreater( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	if( node1->O_O result_n > node2->O_O result_n )
		result->O_O result_n = 1;
	else
		result->O_O result_n = 0;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyLesser( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	if( node1->O_O result_n < node2->O_O result_n )
		result->O_O result_n = 1;
	else
		result->O_O result_n = 0;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyGreaterEqual( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	if( node1->O_O result_n >= node2->O_O result_n )
		result->O_O result_n = 1;
	else
		result->O_O result_n = 0;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyLesserEqual( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	if( node1->O_O result_n <= node2->O_O result_n )
		result->O_O result_n = 1;
	else
		result->O_O result_n = 0;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyIsEqual( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	if( node1->O_O result_n == node2->O_O result_n )
		result->O_O result_n = 1;
	else
		result->O_O result_n = 0;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyIsNotEqual( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	if( node1->O_O result_n != node2->O_O result_n )
		result->O_O result_n = 1;
	else
		result->O_O result_n = 0;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyBinaryAnd( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	result->O_O result_n = node1->O_O result_n & node2->O_O result_n;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyBinaryOr( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	result->O_O result_n = node1->O_O result_n | node2->O_O result_n;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyXor( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	result->O_O result_n = node1->O_O result_n ^ node2->O_O result_n;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyLogicalAnd( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	result->O_O result_n = node1->O_O result_n && node2->O_O result_n;
	//if( g.bDebugLog )
	//{
	//	fprintf( stddbg, WIDE("%Ld && %Ld == %Ld\n"), node1->O_O result_n, node2->O_O result_n, result->O_O result_n );
	//}
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyLogicalOr( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	result->O_O result_n = node1->O_O result_n || node2->O_O result_n;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR ApplyAddition( struct vesl_code_parser_state *l, ACCUMULATOR result, OPNODE node1, OPNODE node2 )
{
	result->O_O value_type = VALUE_NUMBER;
	result->O_O result_n = node1->O_O result_n + node2->O_O result_n;
	return result;
}

//--------------------------------------------------------------------------

ACCUMULATOR FindFunction( struct vesl_code_parser_state *l, OPNODE func ) {

	return NULL;
}

//--------------------------------------------------------------------------

OPNODE ResolveExpression( struct vesl_code_parser_state *l, PDATALIST subExpr, ACCUMULATOR accumulator, ACCUMULATOR arguments )
{
	// find highest operand... next next next next....
	OPNODE node = l->root;
	//OPNODE expr;
	PDATALIST exprList = subExpr;
	//INDEX idx;
#if 0
	//node = (*expr);
	//if( g.bDebugLog )
	//	LogExpression( node );
	DATA_FORALL( exprList, idx, OPNODE, expr ) {
		OPNODE var;
		OPNODE lvar = NULL;
		node = expr;
		while( node ) {
			lvar = var;
			if( node->O_O name ) {
				INDEX idx;
				if( node->O_O name[0] == '.' ) {
					if( accumulator->last_value ) {
						ACCUMULATOR container = accumulator;
						int n;
						while( node->O_O name[0] == '.' ) {
							container = container->last_value;
						}
						if( container )
							DATA_FORALL( container->localData, idx, OPNODE, var ) {
							if( var->O_O name == node->O_O name ) {
								break;
							}
						}
						if( container && !var ) DATA_FORALL( container->O_O contains, idx, OPNODE, var ) {
							if( var->O_O name == node->O_O name ) {
								break;
							}
						}
					}
					if( !var ) DATA_FORALL( accumulator->localData, idx, OPNODE, var ) {
						if( var->O_O name == node->O_O name ) {
							break;
						}
					}
					if( !var ) DATA_FORALL( accumulator->localData, idx, OPNODE, var ) {
						if( var->O_O name == node->O_O name ) {
							break;
						}
					}
					if( !var ) {
						// have to create it in the localData list, and name it.
						var = GetAccumulator( l, accumulator, NULL, 0 );
						var->O_O name = node->O_O name;
						var->O_O nameLen = node->O_O nameLen;
					}
				} else {
					if( arguments )
						DATA_FORALL( arguments->O_O contains, idx, OPNODE, var ) {
							if( var->O_O name == node->O_O name ) {
								break;
							}
						}
					if( !var ) DATA_FORALL( accumulator->O_O contains, idx, OPNODE, var ) {
						if( var->O_O name == node->O_O name ) {
							break;
						}
					}
					if( !var )
						var = GetAccumulator( l, accumulator, node->O_O name, node->O_O nameLen );
				}
			} else {
				var = GetAccumulator( l, accumulator, NULL, 0 );
			}

			//OPNODE expr = 
			 // first loop - handle ! and ~
			if( node->O_O value_type == OP_SUBEXPRESSION ) {
				ResolveExpression( l, node->O_O contains, var, NULL );
				//node = (*expr);
				continue;
			} else if( node->O_O value_type == VALUE_FUNCTION_CALL ) {
				ACCUMULATOR arguments = GetAccumulator( l, NULL, NULL, 0 );

				FindFunction( l, node );
				ResolveExpression( l, node->O_O contains, var, arguments );
				//node = (*expr);
				continue;
			} else if( node->O_O value_type == VALUE_FUNCTION ) {
				//FindFunction( l, node );
				//ResolveExpression( l, node->O_O contains, var );
				//node = (*expr);
				continue;
			} else if( node->O_O value_type == OP_BINARYNOT ) {
				if( !right( node ) ) {
					lprintf( WIDE( "%s(%d): Error binary not operator(~) with no right hand operand\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
					return NULL;
				}
				{
					OPNODE right = right( node );
					if( IsValue( l, right, var, TRUE ) ) {
						ApplyBinaryNot( var );
					} else {
						lprintf( WIDE( "%s(%d): Error binary not(~) is not followed by an integer...\n" )
							, GetCurrentFileName( l )
							, GetCurrentLine( l ) );
						//g.ErrorCount++;
					}
				}
				continue;
			} else if( node->O_O value_type == OP_LOGICALNOT ) {
				if( !right( node ) ) {
					lprintf( WIDE( "%s(%d): Error logical not operator with no right hand operand\n" )
						, GetCurrentFileName( l ), GetCurrentLine( l ) );
					//g.ErrorCount++;
					return NULL;
				}
				{
					OPNODE right = right( node );
					if( IsValue( l, right, var, TRUE ) ) {
						ApplyLogicalNot( var, right );
					} else {
						lprintf( WIDE( "%s(%d): Logical not is not followed by an integer...\n" )
							, GetCurrentFileName( l )
							, GetCurrentLine( l ) );
						//g.ErrorCount++;
					}
				}
				continue;
			} else if( node->O_O value_type == OP_PLUS ) {
				OPNODE right = right( node );
				if( IsValue( l, right, var, TRUE ) ) {
					ApplyNegative( var, right );
					continue;
				}
				continue;
			} else if( node->O_O value_type == OP_MINUS ) {
				OPNODE right = right( node );
				if( IsValue( l, right, var, TRUE ) ) {
					ApplyNegative( var, right );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Negative operator is not followed by a value\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
					return NULL;
				}
			}

			node = right( node );
			var = right( var );
		}


		//if( g.bDebugLog )
		//	fprintf( stddbg, WIDE("Done with unary +,-,!,~,()") );
		//if( g.bDebugLog )
		//	LogExpression(node);
		node = expr;
		while( node ) {
			// Second loop handle * / %
			if( node->O_O value_type == OP_MULTIPLY ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, var, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ApplyMultiply( l, var, right );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to multiply?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			if( node->O_O value_type == OP_DIVIDE ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyDivide( l, var, left, right );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to divide?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			if( node->O_O value_type == OP_MOD ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ApplyModulus( l, lvar, right );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to mod?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			node = right( node );
		}
		// +/- additive operators would be next - but already done as unary.
		//if( g.bDebugLog )
		//	LogExpression(node);
		node = expr;
		while( node ) {
			// third loop handle >> <<
			if( node->O_O value_type == OP_SHIFTRIGHT ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ApplyShiftRight( l, lvar, lvar, var );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to shift right?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			if( node->O_O value_type == OP_SHIFTLEFT ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyShiftLeft( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to shift left?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			node = right( node );
		}
		node = expr;
		while( node ) {
			// 4th loop handle > < >= <= comparisons (result in 0/1)
			if( node->O_O value_type == OP_GREATER ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyGreater( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to greater?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			if( node->O_O value_type == OP_LESSER ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyLesser( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to lesser?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			if( node->O_O value_type == OP_GREATEREQUAL ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyGreaterEqual( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to greater equal?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					LogExpression( expr );
					//g.ErrorCount++;
				}
			}
			if( node->O_O value_type == OP_LESSEREQUAL ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyLesserEqual( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Invalid operands to lesser equal?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			node = right( node );
		}
		//if( g.bDebugLog )
		//	LogExpression(node);
		node = expr;
		while( node ) {
			// 5th loop handle == !=
			if( node->O_O value_type == OP_ISEQUAL ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyIsEqual( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to equal?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			if( node->O_O value_type == OP_NOTEQUAL ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyIsNotEqual( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to not equal?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					LogExpression( expr );
					//g.ErrorCount++;
				}
			}
			node = right( node );
		}
		//if( g.bDebugLog )
		//	LogExpression(node);
		node = expr;
		while( node ) {
			// 6th loop
			if( node->O_O value_type == OP_BINARYAND ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, var, TRUE )
					&& IsValue( l, right, lvar, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyBinaryAnd( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to not equal?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			node = right( node );
		}

		node = expr;
		while( node ) {
			// 7th loop
			if( node->O_O value_type == OP_XOR ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyXor( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to not equal?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			node = right( node );
		}
		//if( g.bDebugLog )
		//	LogExpression(node);
		node = expr;
		while( node ) {
			// 8th loop
			if( node->O_O value_type == OP_BINARYOR ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyBinaryOr( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to not equal?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			node = right( node );
		}
		node = expr;
		while( node ) {
			// 8th loop
			if( node->O_O value_type == OP_LOGICALAND ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyLogicalAnd( l, lvar, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to not equal?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			node = right( node );
		}
		//if( g.bDebugLog )
		//	LogExpression(node);
		node = expr;
		while( node ) {
			// 9th loop
			if( node->O_O value_type == OP_LOGICALOR ) {
				OPNODE left, right;
				right = right( node );
				if( IsValue( l, NULL, lvar, TRUE )
					&& IsValue( l, right, var, TRUE ) ) {
					ACCUMULATOR result;
					result = ApplyLogicalOr( l, result, lvar, right );
					node = right( right ); lvar = var; var = right( var );
					continue;
				} else {
					lprintf( WIDE( "%s(%d): Error invalid operands to not equal?\n" )
						, GetCurrentFileName( l )
						, GetCurrentLine( l ) );
					//g.ErrorCount++;
				}
			}
			node = right( node );
		}

#if 0
		// accumulator has now been built with the result.
		node = expr;
		while( node ) {
			// and finally - add all subsequent operators...
			OPNODE right;
			right = right( node );
			if( node && right ) {
				ACCUMULATOR result;
				result = ApplyAddition( l, NULL, lvar, right );
				node = right( right ); lvar = var; var = right( var );
				continue;
			}
			node = right( node );
		}
#endif
	}
#endif
	//if( g.bDebugLog )
	//	LogExpression(*expr);
	//return *expr;
	return NULL;
}

//--------------------------------------------------------------------------

int IsValidExpression( struct vesl_code_parser_state*l, PDATALIST terms )
{
	OPNODE *ppexpr = &l->root;
	// check to see if any operands are next to any other operands...
	// like 3 4 2 is not valid 3 + 4 + 2 is though
	// though the processing will cause the +'s to dissappear and
	// subsequently when done - any operands next to each other are
	// implied +'s
	OPNODE node;
	INDEX idx;
	int prior_operand = 0;
	DATA_FORALL( terms, idx, OPNODE , node )
	{
		if( node->O_O value_type == OP_COMMA )
		{
			*ppexpr = right(node);
			//DATA_FORALL();
			//LIST_FORALL
			ExpressionBreak( right(node) );
			DestroyExpression( l, node );
			node = *ppexpr;
			prior_operand = 0;
			continue;
		}
		if( node->O_O value_type == OP_SUBEXPRESSION )
		{
			if( !IsValidExpression( l, node->O_O contains ) )
				return FALSE;
			prior_operand = 1;
		}
		else if( IsValue( l, node, NULL, FALSE ) )
		{
			if( prior_operand )
			{
				LogExpression( *ppexpr );
				lprintf( WIDE("%s(%d): Multiple operands with no operator!\n")
				       , GetCurrentFileName(l), GetCurrentLine(l) );
				return FALSE;
			}
			prior_operand = 1;
		}
		else
			prior_operand = 0;
	}
	return TRUE;
}

//--------------------------------------------------------------------------

LONGEST_INT ProcessExpression(  )
{
	struct vesl_code_parser_state _l;
	struct vesl_code_parser_state *l = &_l;
	BuildExpression( l, &l->root->O_O contains );
	if( IsValidExpression( l, l->root->O_O contains ) )
	{
		OPNODE tree = l->root;
		accumulator result;
		result.O_O value_type = VALUE_UNSET;
		result.O_O contains = CreateDataList( sizeof( struct json_value_container ) );
		ResolveExpression( l, l->root->O_O contains, &result, NULL );

		{
			LONGEST_INT resultval = 0;
			if( tree )
			{
				resultval = tree->O_O result_n;
			}
			DestroyExpression( l, tree );
			return resultval;
		}
	}
	DestroyExpression( l, _l.root );
	return 0;
}


#undef left
#undef right