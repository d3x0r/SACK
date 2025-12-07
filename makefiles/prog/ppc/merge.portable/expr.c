//----------------------------------------------------------------------
// Expressions parser, processor
//   Limitation - handles only constant expressions
//   Limitation - expects expression to be on one continuous line
//                which is acceptable for the cpp layer.
//----------------------------------------------------------------------
#define C_PRE_PROCESSOR
#include <stdio.h> // debug only...

#include "sack_ucb_filelib.h"

//#include "text.h"
#include "fileio.h"
#include "define.h"
#include "expr.h"
#include "global.h"

typedef struct opnode {
	int op;
	union {
		uint64_t i;
		double f;
		PTEXT string;
		struct opnode *sub;
	} data;
	struct opnode *left, *right;
} OPNODE, *POPNODE;


static char pHEX[] = "0123456789ABCDEF";
static char phex[] = "0123456789abcdef";

/*
 Section Category Operators
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


enum { OP_HANG = -1 // used to indicate prior op complete, please hang on tree
		// after hanging, please re-check current symbol
     , OP_NOOP = 0
     , OP_SUBEXPRESSION // (...)

     , OP_INT_OPERAND_8
     , OP_INT_OPERAND_16
     , OP_INT_OPERAND_32
     , OP_INT_OPERAND_64

     , OP_SINT_OPERAND_8
     , OP_SINT_OPERAND_16
     , OP_SINT_OPERAND_32
     , OP_SINT_OPERAND_64

     , OP_FLT_OPERAND_32
     , OP_FLT_OPERAND_64

     , OP_CHARACTER_STRING
     , OP_CHARACTER_CONST

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
     , OP_LOGICALAND                   // &&
     , OP_ANDEQUAL                     // &=

     , OP_BINARYOR                     // |
     , OP_LOGICALOR                    // ||
     , OP_OREQUAL                      // |=

     , OP_COMPARISON                   // ?
     , OP_ELSE_COMPARISON					// :
     , OP_COMMA
     //, OP_DOT
     //, OP_
};

char *fullopname[] = { "noop", "sub-expr"
                     ,  "uint8_t",  "uint16_t",  "uint32_t",  "uint64_t" // unsigned int
                     , "int8_t", "int16_t", "int32_t", "int64_t" // signed int
                     , "float", "double" // float ops
                     , "string", "character"
                     , "=", "=="
                     , "+", "++", "+="
                     , "-", "--", "-="
                     , "*", "*="
                     , "%", "%="
                     , "/", "/="
                     , "^", "^="
                     , "~"
                     , "!", "!="
                     , ">", ">>", ">=", ">>="
                     , "<", "<<", "<=", "<<="
                     , "&", "&&", "&="
                     , "|", "||", "|="
                     , "?", ":", ","
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
                       , { OP_LOGICALNOT, { { '=', OP_NOTEQUAL } } }
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

//--------------------------------------------------------------------------

static POPNODE GetOpNodeEx( DBG_VOIDPASS )
#define GetOpNode() GetOpNodeEx( DBG_VOIDSRC )
{
	POPNODE pOp = AllocateEx( sizeof( OPNODE ) DBG_RELAY );
	memset( pOp, 0, sizeof( OPNODE ) );
	pOp->op     = OP_NOOP;
	return pOp;
}

//--------------------------------------------------------------------------
void DestroyExpressionEx( POPNODE root DBG_PASS );
#define DestroyExpression(r) DestroyExpressionEx(r DBG_SRC )

void DestroyOpNodeEx( POPNODE node DBG_PASS )
#define DestroyOpNode(n) DestroyOpNodeEx(n DBG_SRC)
{
	// delete any allocated content...
	if( node->op == OP_CHARACTER_STRING )
		LineRelease( node->data.string );
	else if( node->op == OP_SUBEXPRESSION )
		DestroyExpressionEx( node->data.sub DBG_RELAY );

	if( node->left )
		node->left->right = node->right;
	if( node->right )
		node->right->left = node->left;
	ReleaseEx( (void**)&node DBG_RELAY );
}

//--------------------------------------------------------------------------

void DestroyExpressionEx( POPNODE root DBG_PASS )
{
	POPNODE next;
	// go to the start of the expression...
	if( !root )
		return;
	while( root->left )
		root = root->left;
	next = root;
	while( root = next )
	{
		next = root->right;
		DestroyOpNodeEx( root DBG_RELAY );
	}
}

//--------------------------------------------------------------------------

void ExpressionBreak( POPNODE breakbefore )
{
	if( breakbefore->left )
	{
		breakbefore->left->right = NULL;
		breakbefore->left = NULL;
	}
}

//--------------------------------------------------------------------------

POPNODE SubstNodes( POPNODE _this_left, POPNODE _this_right, POPNODE that )
{
	if( _this_left && _this_right && that )
	{
		POPNODE that_left = that;
		POPNODE that_right = that;

		while( that_left->left )
			that_left = that_left->left;
		while( that_right->right )
			that_right = that_right->right;

		if( _this_left->left )
			_this_left->left->right = that_left;
		that_left->left = _this_left->left;
		_this_left->left = NULL;

		if( _this_right->right )
			_this_right->right->left = that_right;
		that_right->right = _this_right->right;
		_this_right->right = NULL;
		return _this_left;
	}
	return NULL;
}

//--------------------------------------------------------------------------

POPNODE SubstNode( POPNODE _this, POPNODE that )
{
	return SubstNodes( _this, _this, that );
}

//--------------------------------------------------------------------------

POPNODE GrabNodes( POPNODE start, POPNODE end )
{
	if( start->left )
		start->left->right = end->right;
	if( end->right )
		end->right->left = start->left;
	end->right = NULL;
	start->left = NULL;
	return start;
}

//--------------------------------------------------------------------------

POPNODE GrabNode( POPNODE _this )
{
	return GrabNodes( _this, _this );
}

//--------------------------------------------------------------------------

static int RelateOpNode( POPNODE *root, POPNODE node )
{
	if( !node )
	{
		fprintf( stderr, "Fatal Error: cannot relate a NULL node\n" );
		return 0;
	}
	if( !root )
	{
		fprintf( stderr, "Fatal error: Cannot build expression tree with NULL root.\n" );
		return 0;
	}
#ifdef C_PRE_PROCESSOR
	switch( node->op )
	{
	case OP_FLT_OPERAND_32:
	case OP_FLT_OPERAND_64:
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
		fprintf( stderr, "%s(%d) Error: preprocessor expression may not use operand %s\n"
		       , GetCurrentFileName(), GetCurrentLine(), fullopname[ node->op ] );
		DestroyOpNode( node );
		return 0;
		break;
	}
#endif
	if( !*root )
		*root = node;
	else
	{
		POPNODE last = *root;
		while( last && last->right )
		{
			last = last->right;
		}
		if( last )
		{
			node->left = last;
			last->right = node;
		}
	}
	return 1;
}

//--------------------------------------------------------------------------


// result : 0 = okay value
//          1 = float number ?
//          2 = invalid number...
static int GetInteger( uint64_t *result, int *length )
{
	char *p = GetText( GetCurrentWord() );
	uint64_t accum = 0;
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
  						fprintf( stderr, "%s(%d) Error: Hexadecimal may not be used to define a float.\n", GetCurrentFileName(), GetCurrentLine() );
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
			 		fprintf( stderr, "%s(%d) Error: Octal constant has invalid character 0x%02x\n", GetCurrentFileName(), GetCurrentLine() , p[0] );
				else
			 		fprintf( stderr, "%s(%d) Error: Octal constant has invalid character '%c'\n", GetCurrentFileName(), GetCurrentLine() , p[0] );
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
					fprintf( stderr, "%s(%d) Error: U or u qualifiers specifed more than once on a constant.\n", GetCurrentFileName(), GetCurrentLine()  );
					return 2;
				}
				unsigned_value = 1;
				p++;
			}
			else if( p[0] == 'l' || p[0] == 'L' )
			{
				if( long_value )
				{
					fprintf( stderr, "%s(%d) Error: too many L or l qualifiers specifed on a constant.\n", GetCurrentFileName(), GetCurrentLine()  );
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
					fprintf( stderr, "%s(%d) Error: Invalid type specification 0x%02x.\n"
					       , GetCurrentFileName(), GetCurrentLine(), p[1] );
				else
					fprintf( stderr, "%s(%d) Error: Invalid type specification '%c'.\n"
					       , GetCurrentFileName(), GetCurrentLine(), p[1] );
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
static int GetFloat( double *result, int *length )
{
	//double accum = 0;
	//fprintf( stderr, "At this time 'expr.c' does not do float conversion...\n" );
	return 0;
}

//--------------------------------------------------------------------------

void LogExpression( POPNODE root )
{
	static int level;
	level++;
	while( root )
	{
		if( root->op == OP_SUBEXPRESSION )
		{
			fprintf( stderr, "( " );
			LogExpression( root->data.sub );
			fprintf( stderr, " )" );
		}
		else
		{
			fprintf( stderr, "(%s = %lld)", fullopname[root->op],root->data.i );
		}
#ifdef __LINUX__
		if( root->right )
			if( root->right->left != root )
#  if !defined( __ARM__ )
				asm( "int $3\n" )
#  endif
				;
#endif
		root = root->right;
	}
	level--;
	if( !level )
		fprintf( stderr, "\n" );
}

//--------------------------------------------------------------------------
// one might suppose that
// expressions are read ... left, current, right....
// and things are pushed off to the left and right of myself, and or rotated
// appropriately....
//
//     NULL
//     (op)+/-/##/(
//--------------------------------------------------------------------------

POPNODE BuildExpression( void ) // expression is queued
{
	char *pExp;
	int quote = 0;
	int overflow = 0;
	POPNODE ThisOp = GetOpNode();
	POPNODE branch = NULL;
	PTEXT thisword;
	//return 0; // force false output ... needs work on substitutions...

	//if( g.bDebugLog )
	//{
	//	fprintf( stddbg, "Build expression for: " );
	//	DumpSegs( GetCurrentWord() );
	//}

	while( ( thisword = GetCurrentWord() ) )
	{
		int n;
		pExp = GetText( thisword );
		//printf( "word: %s\n", pExp );
		if( pExp[0] == '\'' )
		{
			if( quote == '\'' )
			{
				overflow = 0;
				RelateOpNode( &branch, ThisOp );
			  	ThisOp = GetOpNode();
				quote = 0;
			}
			else if( !quote )
			{
				ThisOp->op = OP_CHARACTER_CONST;
				quote = pExp[0];
			}
			StepCurrentWord();
			continue;
		}
		else if( pExp[0] == '\"' )
		{
			if( quote == '\"' )
			{
				PTEXT tmp = BuildLine( ThisOp->data.string );
				LineRelease( ThisOp->data.string );
				ThisOp->data.string = tmp;
				RelateOpNode( &branch, ThisOp );
			  	ThisOp = GetOpNode();
				quote = 0;
			}
			else if( !quote )
			{
				ThisOp->op = OP_CHARACTER_STRING;
				quote = pExp[0];
			}
			StepCurrentWord();
			continue;
		}

		if( quote )
		{
			if( ThisOp->op == OP_CHARACTER_STRING )
			{
				ThisOp->data.string =
					SegAppend( ThisOp->data.string
					         , SegDuplicate( thisword ) );
			}
			else if( ThisOp->op == OP_CHARACTER_CONST )
			{
				int n, len = (int)GetTextSize( thisword );
				for( n = 0; n < thisword->format.position.offset.spaces; n++ )
				{
					if( !overflow &&
						 (ThisOp->data.i & 0xFF00000000000000LL) )
					{
						overflow = 1;
						fprintf( stderr, "%s(%d): warning character constant overflow.\n"
						       , GetCurrentFileName(), GetCurrentLine() );
					}
					ThisOp->data.i *= 256;
					ThisOp->data.i += ' ';
				}
				for( n = 0; n < len; n++ )
				{
					ThisOp->data.i *= 256;
					ThisOp->data.i += pExp[n];
				}
			}
			StepCurrentWord();
			continue;
		}

		if( pExp[0] == '(' )
		{
			POPNODE subexpression;

			if( ThisOp->op != OP_NOOP )
			{
			//if( g.bDebugLog )
			//{
			//	fprintf( stddbg, "Adding operation: " );
			//	LogExpression( ThisOp );
			//}
				RelateOpNode( &branch, ThisOp );
				ThisOp = GetOpNode();
			}

			StepCurrentWord();
			subexpression = BuildExpression();
			pExp = GetText( GetCurrentWord() );
			if( pExp && pExp[0] != ')' )
			{
				fprintf( stderr, "(%s)%d Error: Invalid expression\n", GetCurrentFileName(), GetCurrentLine() );
				DestroyExpression( branch );
				DestroyOpNode( ThisOp );
				return NULL;
				// invalid pairing of parens in expression
			}
			ThisOp->op = OP_SUBEXPRESSION;
			ThisOp->data.sub = subexpression;
			RelateOpNode( &branch, ThisOp );
		  	ThisOp = GetOpNode();
			// pExp = GetText( GetCurrentWord() );
			// on return check current token as ')'
		}
		else if( pExp[0] == ')' )
		{
			if( ThisOp->op != OP_NOOP )
			{
				RelateOpNode( &branch, ThisOp );
			}
			else
				DestroyOpNode( ThisOp );
			//if( g.bDebugLog )
			//{
			//	fprintf( stddbg, "Built Expression: ") ;
			//	LogExpression( branch );
			//}
			return branch;
		}
		else if( ( pExp[0] >= '0' && pExp[0] <= '9' ) ||
					( pExp[0] == '.' ) )
		{
			uint64_t i;
	  		double f;
			int len;
			switch( GetInteger( &i, &len ) )
			{
			case 0: // good integer.
				if( ThisOp->op != OP_NOOP )
				{
					RelateOpNode( &branch, ThisOp );
					ThisOp = GetOpNode();
				}
				ThisOp->op = OP_INT_OPERAND_64;
				ThisOp->data.i = i;
				RelateOpNode( &branch, ThisOp );
				ThisOp = GetOpNode();
				break;
			case 1: // might be a float...
				switch( GetFloat( &f, &len ) )
				{
				case 0:

				case 1: // invalid conversion (invalid number)
					DestroyExpression( branch ); // also if preprocessor - this always must fail
					DestroyOpNode( ThisOp );
					//if( g.bDebugLog )
					//{
					//	fprintf( stddbg, "Built Expression 2: ") ;
					//	LogExpression( branch );
					//}
					return branch;
				}
				break;
			case 2: // invalid number
				break;
			}
		}
		else if( pExp[0] == '_' 
		       || ( pExp[0] >= 'A' && pExp[0] <= 'Z' )
		       || ( pExp[0] >= 'a' && pExp[0] <= 'z' )  )
		{
			// this is unsubstituted, is not a predefined thing, etc,
			// therefore this is a 0.
			if( quote )
			{
			}
			else
			{
				if( ThisOp->op != OP_NOOP )
				{
					RelateOpNode( &branch, ThisOp );
					ThisOp = GetOpNode();
				}
				ThisOp->op = OP_INT_OPERAND_64;
				ThisOp->data.i = 0;
				RelateOpNode( &branch, ThisOp );
				ThisOp = GetOpNode();
			}
		}
		else {
			if( !thisword->format.position.offset.spaces || ThisOp->op == OP_NOOP )
			{
			retry_this_operator:
				for( n = 0; n < NUM_RELATIONS; n++ )
				{
				 	if( Relations[n].thisop == ThisOp->op )
			 		{
			 			int o;
			 			for( o = 0; Relations[n].trans[o].ch; o++ )
				 		{
				 			if( Relations[n].trans[o].ch == pExp[0] )
							{
								//if( g.bDebugLog )
								//{
								//	fprintf( stddbg, "%s becomes %s\n",
								//			  ThisOp->op<0?"???":fullopname[ThisOp->op], fullopname[Relations[n].trans[o].becomes] );
								//}
		 						ThisOp->op = Relations[n].trans[o].becomes;
			 					break;
			 				}
				 		}
				 		if( !Relations[n].trans[o].ch )
			 			{
			 				//fprintf( stddbg, "Invalid expression addition\n" );
							fprintf( stderr, "%s(%d): Error invalid operator: %s\n"
							       , GetCurrentFileName()
							       , GetCurrentLine()
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
				if( ThisOp->op != OP_NOOP )
				{
					RelateOpNode( &branch, ThisOp );
					ThisOp = GetOpNode();
					goto retry_this_operator;
				}
				DestroyExpression( branch );
				return NULL;
			}
		}
		StepCurrentWord();
	}

	//if( g.bDebugLog )
	//{
	//	fprintf( stddbg, "Deleting: " );
	//	LogExpression( ThisOp );
	//}
	DestroyOpNode( ThisOp );
	//if( g.bDebugLog )
	//{
	//	fprintf( stddbg, "Built Expression: ") ;
	//	LogExpression( branch );
	//}
	return branch;
}

//--------------------------------------------------------------------------

POPNODE ResolveExpression( POPNODE *expr );

//--------------------------------------------------------------------------

int IsValue( POPNODE *node, int collapse_sub )
{
	POPNODE temp;
	if( !node )
		return FALSE;
	switch( (*node)->op )
	{
	case OP_INT_OPERAND_8:
	case OP_INT_OPERAND_16:
	case OP_INT_OPERAND_32:
	case OP_INT_OPERAND_64:

	case OP_SINT_OPERAND_8:
	case OP_SINT_OPERAND_16:
	case OP_SINT_OPERAND_32:
	case OP_SINT_OPERAND_64:
		return TRUE;

	case OP_FLT_OPERAND_32:
	case OP_FLT_OPERAND_64:
		fprintf( stderr, "%s(%d): Floating point operand is not supported\n"
		       , GetCurrentFileName(), GetCurrentLine() );
		return FALSE;

	case OP_SUBEXPRESSION:
		if( collapse_sub )
		{
			temp = ResolveExpression( &(*node)->data.sub );
			(*node)->data.sub = NULL;
			DestroyOpNode( SubstNode( *node, temp ) );
			*node = temp;
		}
		return TRUE;
	}
	return FALSE;
}

//--------------------------------------------------------------------------

void ApplyBinaryNot( POPNODE node )
{
	switch( node->op )
	{
	case OP_INT_OPERAND_8:
	case OP_INT_OPERAND_16:
	case OP_INT_OPERAND_32:
	case OP_INT_OPERAND_64:

	case OP_SINT_OPERAND_8:
	case OP_SINT_OPERAND_16:
	case OP_SINT_OPERAND_32:
	case OP_SINT_OPERAND_64:
		node->data.i = ~node->data.i;
		break;
	default:
		fprintf( stderr, "Dunno how we got here...\n" );
	}
}

//--------------------------------------------------------------------------

void ApplyLogicalNot( POPNODE node )
{
	switch( node->op )
	{
	case OP_INT_OPERAND_8:
	case OP_INT_OPERAND_16:
	case OP_INT_OPERAND_32:
	case OP_INT_OPERAND_64:

	case OP_SINT_OPERAND_8:
	case OP_SINT_OPERAND_16:
	case OP_SINT_OPERAND_32:
	case OP_SINT_OPERAND_64:
		node->data.i = !node->data.i;
		break;
	default:
		fprintf( stderr, "Dunno how we got here...\n" );
	}
}

//--------------------------------------------------------------------------

void ApplyNegative( POPNODE node )
{
	switch( node->op )
	{
	case OP_INT_OPERAND_8:
	case OP_INT_OPERAND_16:
	case OP_INT_OPERAND_32:
	case OP_INT_OPERAND_64:

	case OP_SINT_OPERAND_8:
	case OP_SINT_OPERAND_16:
	case OP_SINT_OPERAND_32:
	case OP_SINT_OPERAND_64:
		node->data.i = -node->data.i;
		break;
	default:
		fprintf( stderr, "Dunno how we got here...\n" );
	}
}

//--------------------------------------------------------------------------

POPNODE ApplyMultiply( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	result->data.i = node1->data.i * node2->data.i;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyDivide( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	if( node2->data.i != 0 )
		result->data.i = node1->data.i * node2->data.i;
	else
	{
		fprintf( stderr, "Right hand operator of divide is 0! - returning MAXINT\n" );
#if defined( _MSC_VER ) || defined( __WATCOMC__ )
		result->data.i = 0xFFFFFFFFFFFFFFFFU;
#else
		result->data.i = 0xFFFFFFFFFFFFFFFFULL;
#endif
	}
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyModulus( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	result->data.i = node1->data.i % node2->data.i;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyShiftRight( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	result->data.i = node1->data.i >> node2->data.i;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyShiftLeft( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	result->data.i = node1->data.i << node2->data.i;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyGreater( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	if( node1->data.i > node2->data.i )
		result->data.i = 1;
	else
		result->data.i = 0;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyLesser( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	if( node1->data.i < node2->data.i )
		result->data.i = 1;
	else
		result->data.i = 0;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyGreaterEqual( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	if( node1->data.i >= node2->data.i )
		result->data.i = 1;
	else
		result->data.i = 0;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyLesserEqual( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	if( node1->data.i <= node2->data.i )
		result->data.i = 1;
	else
		result->data.i = 0;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyIsEqual( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	if( node1->data.i == node2->data.i )
		result->data.i = 1;
	else
		result->data.i = 0;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyIsNotEqual( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	if( node1->data.i != node2->data.i )
		result->data.i = 1;
	else
		result->data.i = 0;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyBinaryAnd( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	result->data.i = node1->data.i & node2->data.i;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyBinaryOr( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	result->data.i = node1->data.i | node2->data.i;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyXor( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	result->data.i = node1->data.i ^ node2->data.i;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyLogicalAnd( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	result->data.i = node1->data.i && node2->data.i;
	//if( g.bDebugLog )
	//{
	//	fprintf( stddbg, "%Ld && %Ld == %Ld\n", node1->data.i, node2->data.i, result->data.i );
	//}
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyLogicalOr( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	result->data.i = node1->data.i || node2->data.i;
	return result;
}

//--------------------------------------------------------------------------

POPNODE ApplyAddition( POPNODE node1, POPNODE node2 )
{
	POPNODE result = GetOpNode();
	result->op = OP_SINT_OPERAND_64;
	result->data.i = node1->data.i + node2->data.i;
	return result;
}

//--------------------------------------------------------------------------


POPNODE ResolveExpression( POPNODE *expr )
{
	// find highest operand... next next next next....
	POPNODE node;
	node = (*expr);
	//if( g.bDebugLog )
	//	LogExpression( node );
	while( node )
	{
		 // first loop - handle ! and ~
		if( node->op == OP_SUBEXPRESSION )
		{
			POPNODE sub;
			ResolveExpression( &node->data.sub );
			SubstNode( node, sub = node->data.sub );
			node->data.sub = NULL;
			DestroyExpression( node );
			if( node == (*expr) )
				(*expr) = sub;
			node = (*expr);
			continue;
		}
		else if( node->op == OP_BINARYNOT )
		{
			if( !node->right )
			{
				fprintf( stderr, "%s(%d): Error binary not operator(~) with no right hand operand\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
				return NULL;
			}
			{
				POPNODE right = node->right;
				if( IsValue( &right, TRUE ) )
				{
					ApplyBinaryNot( right );
					DestroyOpNode( GrabNode( node ) );
					if( node == (*expr) )
						(*expr) = right;
					node = (*expr);
				}
				else
				{
					fprintf( stderr, "%s(%d): Error binary not(~) is not followed by an integer...\n"
					       , GetCurrentFileName()
					       , GetCurrentLine() );
					g.ErrorCount++;
				}
			}
			continue;
		}
		else if( node->op == OP_LOGICALNOT )
		{
			if( !node->right )
			{
				fprintf( stderr, "%s(%d): Error logical not operator with no right hand operand\n"
				       , GetCurrentFileName(), GetCurrentLine() );
				g.ErrorCount++;
				return NULL;
			}
			{
				POPNODE right = node->right;
				if( IsValue( &right, TRUE ) )
				{
					ApplyLogicalNot( right );
					DestroyOpNode( GrabNode( node ) );
					if( node == (*expr) )
						(*expr) = right;
					node = (*expr);
				}
				else
				{
					fprintf( stderr, "%s(%d): Logical not is not followed by an integer...\n"
					       , GetCurrentFileName()
					       , GetCurrentLine() );
					g.ErrorCount++;
				}
			}
			continue;
		}
		else if( node->op == OP_PLUS )
		{
			POPNODE right = node->right;
			DestroyOpNode( node );
			if( node == (*expr) )
				(*expr) = right;
			node = (*expr);
			continue;
		}
		else if( node->op == OP_MINUS )
		{
			POPNODE right = node->right;
			if( IsValue( &right, TRUE ) )
			{
				ApplyNegative( right );
				DestroyOpNode( node );
				if( node == (*expr) )
					(*expr) = right;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Negative operator is not followed by a value\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
				return NULL;
			}
		}
		node = node->right;
	}
	//if( g.bDebugLog )
	//	fprintf( stddbg, "Done with unary +,-,!,~,()" );
	node = (*expr);
	//if( g.bDebugLog )
	//	LogExpression(node);
	while( node )
	{
		// Second loop handle * / %
		if( node->op == OP_MULTIPLY )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyMultiply( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to multiply?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		if( node->op == OP_DIVIDE )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyDivide( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to divide?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		if( node->op == OP_MOD )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyModulus( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to mod?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		node = node->right;
	}
	// +/- additive operators would be next - but already done as unary.
	node = (*expr);
	//if( g.bDebugLog )
	//	LogExpression(node);
	while( node )
	{
		// third loop handle >> <<
		if( node->op == OP_SHIFTRIGHT )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyShiftRight( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to shift right?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		if( node->op == OP_SHIFTLEFT )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyShiftLeft( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to shift left?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		node = node->right;
	}
	node = (*expr);
	while( node )
	{
		// 4th loop handle > < >= <= comparisons (result in 0/1)
		if( node->op == OP_GREATER )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyGreater( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to greater?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		if( node->op == OP_LESSER )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyLesser( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to lesser?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		if( node->op == OP_GREATEREQUAL )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyGreaterEqual( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to greater equal?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				LogExpression( *expr );
				g.ErrorCount++;
			}
		}
		if( node->op == OP_LESSEREQUAL )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyLesserEqual( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Invalid operands to lesser equal?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		node = node->right;
	}
	node = (*expr);
	//if( g.bDebugLog )
	//	LogExpression(node);
	while( node )
	{
		// 5th loop handle == !=
		if( node->op == OP_ISEQUAL )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyIsEqual( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to equal?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		if( node->op == OP_NOTEQUAL )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyIsNotEqual( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to not equal?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				LogExpression( *expr );
				g.ErrorCount++;
			}
		}
		node = node->right;
	}
	node = (*expr);
	//if( g.bDebugLog )
	//	LogExpression(node);
	while( node )
	{
		// 6th loop
		if( node->op == OP_BINARYAND )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyBinaryAnd( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to not equal?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		node = node->right;
	}
	node = (*expr);
	//if( g.bDebugLog )
	//	LogExpression(node);
	while( node )
	{
		// 7th loop
		if( node->op == OP_XOR )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyXor( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to not equal?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		node = node->right;
	}
	node = (*expr);
	//if( g.bDebugLog )
	//	LogExpression(node);
	while( node )
	{
		// 8th loop
		if( node->op == OP_BINARYOR )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyBinaryOr( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to not equal?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		node = node->right;
	}
	node = (*expr);
	//if( g.bDebugLog )
	//	LogExpression(node);
	while( node )
	{
		// 8th loop
		if( node->op == OP_LOGICALAND )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyLogicalAnd( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to not equal?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		node = node->right;
	}
	node = (*expr);
	//if( g.bDebugLog )
	//	LogExpression(node);
	while( node )
	{
		// 9th loop
		if( node->op == OP_LOGICALOR )
		{
			POPNODE left, right;
			left = node->left;
			right = node->right;
			if( IsValue( &left, TRUE )
				&& IsValue( &right, TRUE ) )
			{
				POPNODE result;
				result = ApplyLogicalOr( left, right );
				DestroyExpression( SubstNodes( left, right, result ) );
				if( (*expr) == left )
					(*expr) = result;
				node = (*expr);
				continue;
			}
			else
			{
				fprintf( stderr, "%s(%d): Error invalid operands to not equal?\n"
				       , GetCurrentFileName()
				       , GetCurrentLine() );
				g.ErrorCount++;
			}
		}
		node = node->right;
	}
	node = (*expr);
	while( node )
	{
		// and finally - add all subsequent operators...
		POPNODE right;
		right = node->right;
		if( node && right )
		{
			POPNODE result;
			result = ApplyAddition( node, right );
			DestroyExpression( SubstNodes( node, right, result ) );
			(*expr) = result;
			node = (*expr);
			continue;
		}
		node = node->right;
	}
	//if( g.bDebugLog )
	//	LogExpression(*expr);
	return *expr;
}

//--------------------------------------------------------------------------

int IsValidExpression( POPNODE *ppexpr )
{
	// check to see if any operands are next to any other operands...
	// like 3 4 2 is not valid 3 + 4 + 2 is though
	// though the processing will cause the +'s to dissappear and
	// subsequently when done - any operands next to each other are
	// implied +'s
	POPNODE node = *ppexpr;
	int prior_operand = 0;
	while( node )
	{
		if( node->op == OP_COMMA )
		{
			*ppexpr = node->right;
			ExpressionBreak( node->right );
			DestroyExpression( node );
			node = *ppexpr;
			prior_operand = 0;
			continue;
		}
		if( node->op == OP_SUBEXPRESSION )
		{
			if( !IsValidExpression( &node->data.sub ) )
				return FALSE;
			prior_operand = 1;
		}
		else if( IsValue( &node, FALSE ) )
		{
			if( prior_operand )
			{
				LogExpression( *ppexpr );
				fprintf( stderr, "%s(%d): Multiple operands with no operator!\n"
				       , GetCurrentFileName(), GetCurrentLine() );
				return FALSE;
			}
			prior_operand = 1;
		}
		else
			prior_operand = 0;
		node = node->right;
	}
	return TRUE;
}

//--------------------------------------------------------------------------

uint64_t ProcessExpression( void )
{
	POPNODE tree = BuildExpression();
	if( IsValidExpression( &tree ) )
	{
		ResolveExpression( &tree );
		if( tree->left || tree->right )
		{
			fprintf( stderr, "%s(%d): Expression failed to resolve completely...\n"
			       , GetCurrentFileName(), GetCurrentLine() );
		}
		{
			uint64_t resultval = 0;
			if( tree )
			{
				resultval = tree->data.i;
			}
			DestroyExpression( tree );
			return resultval;
		}
	}
	DestroyExpression( tree );
	return 0;
}
