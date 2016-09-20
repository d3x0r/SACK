#include <stdhdrs.h>
#include <stdio.h>

#include "space.h"

#include "input.h" // burst/build
//-----------------------------------------------------------------------------
// Command Pack 3 contains routins to declare, remove, and manipulate or compare
// variables
//-----------------------------------------------------------------------------

#define DECL_ALLOC 0
#define DECL_DECLARE 1
#define DECL_BINARY 2
int DeclareVar( int nOp, PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	if( temp = GetParam( ps, &parameters ) )
	{
		if( TextLike( temp, WIDE("in") ) )
		{
			PTEXT pWhat;
			PENTITY pe;
			pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL );
			if( !pe )
			{
				pWhat = GetParam( ps, &parameters );
				if( pWhat == ps->Current->pName )
					pe = ps->Current;
			}
			else
				pWhat = pe->pName;
			temp = GetParam( ps, &parameters );
			if( temp && pWhat )
			{
				if( pe )
				{
					if( nOp == DECL_ALLOC )
					{
						PTEXT pSize, buffer;
						int size;
						pSize = GetParam( ps, &parameters );
						if( pSize )
						{
						 if( IsNumber( pSize ) )
						 {
							 size = (int)IntCreateFromSeg( pSize );
							 AddBinary( ps, pe, temp, buffer = SegCreate( size ) );
							 buffer->flags |= TF_BINARY;
						 }
						 else
						 {
								DECLTEXT( msg, WIDE("Size specified is not a number") );
							EnqueLink( &ps->Command->Output, &msg );
						 }
						}
						else
						{
							DECLTEXT( msg, WIDE("Must supply size of buffer to allocate...") );
							EnqueLink( &ps->Command->Output, &msg );
						}
					}
					else
					{
					// seems I'm constantly changing from NULL to ps and back
					// unsure how this should actually behave and why?
						AddVariableExx( ps, pe, temp, parameters, (nOp == DECL_BINARY), TRUE );
					}
				}
				else
				{
					S_MSG( ps, WIDE("Could not see %s to declare %s in...")
						 , GetText( pWhat )
						 , GetText( temp ) );
				}
			}
			else
			{
				DECLTEXT( msg, WIDE("Declare IN requires more parameters...") );
				EnqueLink( &ps->Command->Output, &msg );
			}

		}
		else
		{
			if( nOp == DECL_ALLOC )
			{
				PTEXT pSize, buffer;
				int size;
				pSize = GetParam( ps, &parameters );
				if( pSize )
				{
					if( IsNumber( pSize ) )
					{
						size = (int)IntCreateFromSeg( pSize );
					AddBinary( ps, ps->Current, temp, buffer = SegCreate( size ) );
					 buffer->flags |= TF_BINARY;
						 }
						 else
					{
					DECLTEXT( msg, WIDE("Size specified is not a number") );
					 EnqueLink( &ps->Command->Output, &msg );
					}
				}
				else
				{
					DECLTEXT( msg, WIDE("Must supply size of buffer to allocate...") );
					EnqueLink( &ps->Command->Output, &msg );
				}
			}
			else
			{
				AddVariableEx( ps, ps->Current, temp, parameters, ( nOp == DECL_BINARY ) );
			}
		}
	}
	return FALSE;
}

int CPROC ALLOCATE( PSENTIENT ps, PTEXT parameters )
{
	return DeclareVar( DECL_ALLOC, ps, parameters );
}
int CPROC DECLARE( PSENTIENT ps, PTEXT parameters )
{
	return DeclareVar( DECL_DECLARE, ps, parameters );
}
int CPROC BINARY( PSENTIENT ps, PTEXT parameters )
{
	return DeclareVar( DECL_BINARY, ps, parameters );
}

int CPROC COLLAPSE( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pName, temp;
	temp = parameters;
	pName = GetParam( ps, &parameters );
	if( !pName )
	{
		 if( !ps->pLastResult )
		 {
				DECLTEXT( msg, WIDE("Must provide variable reference to collapse, and no content in macro result") );
				EnqueLink( &ps->Command->Output, &msg );
				return FALSE;
		 }
		 else
		 {
				temp = ps->pLastResult;
				ps->pLastResult = BuildLine( temp );
				LineRelease( temp );
				if( ps->CurrentMacro )
					ps->CurrentMacro->state.flags.bSuccess = TRUE;
		 }
		return FALSE;
	}
	if( temp == pName )
	{
		DECLTEXT( msg, WIDE("Invalid variable reference.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	{
		 PTEXT pNew, pOld;
		 pNew = BuildLine( pOld = GetIndirect( pName ) );
		 SetIndirect( pName, pNew );
		 LineRelease( pOld );
		 if( ps->CurrentMacro )
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
	}
	return FALSE;
}

int CPROC UNDEFINE( PSENTIENT ps, PTEXT parameters )

{
	PLIST pVars;
	PTEXT pVar;
	PTEXT pName;
	INDEX idx;
	int bFound;
	pName = GetParam( ps, &parameters );
	if( !pName )
	{
		DECLTEXT( msg, WIDE("Must specify the name of the variable to undefine.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	bFound = FALSE;
	if( ps->CurrentMacro )
	{
		pVars = ps->CurrentMacro->pVars;
		LIST_FORALL( pVars, idx, PTEXT, pVar )
		{
			if( SameText( pVar, pName ) == 0 )
			{
				SetLink( &pVars, idx, NULL );
				LineRelease( pVar );
				bFound = TRUE;
				break;
			}
		}
	}
	if( !bFound )
	{
		pVars = ps->Current->pVars;
		LIST_FORALL( pVars, idx, PTEXT, pVar )
		{
			if( SameText( pVar, pName ) == 0 )
			{
				SetLink( &pVars, idx, NULL );
				LineRelease( pVar );
				bFound = TRUE;
				break;
			}
		}
	}
	if( !bFound )
	{
		DECLTEXT( msg, WIDE("Name specified did not match a variable...") );
		EnqueLink( &ps->Command->Output, &msg );
	}
	return FALSE;
}	
			//--------------------------------------
int CPROC BOUND( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pSave, pBoundType, temp;
	int one_bound = FALSE;
	pSave = parameters;
	temp = GetParam( ps, &parameters );
	if( pSave == temp )
	{
		DECLTEXT( msg, WIDE("Invalid variable reference passed to BOUND.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	if( !temp )
	{
		DECLTEXT( msg, WIDE("Must specify variable and boundry type to set.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	while( ( pBoundType = GetParam( ps, &parameters ) ) )
	{
		one_bound = TRUE;
		if( TextLike( pBoundType, WIDE("lower") ) )
		{
			temp->flags |= TF_LOWER;
		}
		else if( TextLike( pBoundType, WIDE("upper") ) )
		{
			temp->flags |= TF_UPPER;
		}
		else if( TextLike( pBoundType, WIDE("equal") ) )
		{
			temp->flags |= TF_EQUAL;
		}
		else if( TextLike( pBoundType, WIDE("clear") ) )
		{
			temp->flags &= ~(TF_LOWER|TF_UPPER|TF_EQUAL);
		}
	}
	if( !one_bound )
	{
		DECLTEXT( msg, WIDE("Must specify boundry type to set...clear, lower, upper, or equal") );
		EnqueLink( &ps->Command->Output, &msg );
	}
	return FALSE;
}

//--------------------------------------
#define CHANGE_UP		0
#define CHANGE_DOWN	 1
#define CHANGE_PROPER	2
void ChangeCase( PTEXT pVar, int nMethod )
{
	TEXTCHAR *p;
	size_t n, len;
	while( pVar )
	{
		if( pVar->flags & TF_INDIRECT )
			ChangeCase( GetIndirect( pVar ), nMethod );
		else if( !( pVar->flags & ( TF_BINARY | TF_SENTIENT | TF_ENTITY ) ) )
		{
			if( nMethod == CHANGE_UP )
			{
				len = GetTextSize( pVar );
				p = GetText( pVar );
				for( n = 0; n < len; n++ )
				{
					if( p[n] >= 'a' && p[n] <= 'z' )
						p[n] -= 'a' - 'A';
				}
			}
			else if( nMethod == CHANGE_DOWN )
			{
				len = GetTextSize( pVar );
				p = GetText( pVar );
				for( n = 0; n < len; n++ )
				{
					if( p[n] >= 'A' && p[n] <= 'Z' )
						p[n] += 'a' - 'A';
				}
			}
			else if( nMethod == CHANGE_PROPER )
			{
				len = GetTextSize( pVar );
				p = GetText( pVar );
				for( n = 0; n < len; n++ )
				{
					if( !n )
					{
						if( p[n] >= 'a' && p[n] <= 'z' )
							p[n] -= 'a' - 'A';
					}
					else
						if( p[n] >= 'A' && p[n] <= 'Z' )
							p[n] += 'a' - 'A';
				}
			}
		}
		pVar = NEXTLINE( pVar );
	}
}

//--------------------------------------------------------------------------

TEXTCHAR * GetBound( PTEXT pText )
{
	if( !pText )
		return WIDE("(Bad Variable)");
	if( pText->flags & ( TF_LOWER | TF_UPPER | TF_EQUAL ) )
	{
		if( (pText->flags & ( TF_LOWER | TF_UPPER ) ) == ( TF_LOWER | TF_UPPER ) )
			return WIDE("<>");
		if( (pText->flags & ( TF_LOWER | TF_EQUAL ) ) == ( TF_LOWER | TF_EQUAL ) )
			return WIDE(">=");
		if( (pText->flags & ( TF_EQUAL | TF_UPPER ) ) == ( TF_EQUAL | TF_UPPER ) )
			return WIDE("<=");
		if( pText->flags & ( TF_LOWER ) )
			return WIDE(">");
		if( pText->flags & ( TF_UPPER ) ) 
			return WIDE("<");
	}

	return WIDE("=");
}

//--------------------------------------------------------------------------

//--------------------------------------
int CPROC UCASE( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pSave, pChange;
	while( parameters )
	{
		pSave = parameters;
		pChange = GetParam( ps, &parameters );
		if( pChange != pSave )
		{
			ChangeCase( pChange, CHANGE_UP );
		}
		else
		{
			DECLTEXT( msg, WIDE("Attempt to change case of a non-variable") );
			EnqueLink( &ps->Command->Output, &msg );
		}
	}
	return FALSE;
}
int CPROC LCASE( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pSave, pChange;
	while( parameters )
	{
		pSave = parameters;
		pChange = GetParam( ps, &parameters );
		if( pChange != pSave )
		{
			ChangeCase( pChange, CHANGE_DOWN );
		}
		else
		{
			DECLTEXT( msg, WIDE("Attempt to change case of a non-variable") );
			EnqueLink( &ps->Command->Output, &msg );
		}
	}
	return FALSE;
}
int CPROC PCASE( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pSave, pChange;
	while( parameters )
	{
		pSave = parameters;
		pChange = GetParam( ps, &parameters );
		if( pChange != pSave )
		{
			ChangeCase( pChange, CHANGE_PROPER );
		}
		else
		{
			DECLTEXT( msg, WIDE("Attempt to change case of a non-variable") );
			EnqueLink( &ps->Command->Output, &msg );
		}
	}
	return FALSE;
}
//--------------------------------------
int CPROC LALIGN( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	temp = GetParam( ps, &parameters );
	if( temp )
	{
		if( temp->flags & TF_INDIRECT )
			temp = GetIndirect( temp );
		temp->format.position.offset.spaces = 0;
	}
	return FALSE;
}
//--------------------------------------
int CPROC RALIGN( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	temp = GetParam( ps, &parameters );
	if( temp )
	{
		if( temp->flags & TF_INDIRECT )
			temp = GetIndirect( temp );
		/*
		if( temp )
		temp->flags |= TF_RALIGN;
		*/
	}
	return FALSE;
}
//--------------------------------------
int CPROC NOALIGN( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	temp = GetParam( ps, &parameters );
	if( temp )
	{
		if( temp->flags & TF_INDIRECT )
			temp = GetIndirect( temp );
		/*
		if( temp )
		temp->flags &= ~(TF_RALIGN|TF_LALIGN);
		*/
	}
	return FALSE;
}
//--------------------------------------

#define DO_OPERATOR( op )	\
			if( flags.bIntAmount && flags.bIntNumber )					 \
			{																			\
				pResult = SegCreateFrom_64( iNumber op iAmount );		\
			}																			\
			else if( !flags.bIntAmount && flags.bIntNumber )			 \
			{																			\
				pResult = SegCreateFromFloat( fAmount op iNumber );	 \
			}																			\
			else if( flags.bIntAmount && !flags.bIntNumber )			 \
			{																			\
				pResult = SegCreateFromFloat( iAmount op fNumber );	 \
			}																			\
			else /*if( !flags.bIntAmount && !flags.bIntNumber ) */	 \
			{																			\
				pResult = SegCreateFromFloat( fAmount op fNumber );	 \
			}


#define GET_AMOUNT( defaultamount )					\
			pAmount = GetParam( ps, &parameters );								 \
			if( !pAmount || !IsSegAnyNumber( &pAmount, &fAmount, &iAmount, &bInt ) )								 \
			{																					\
				DECLTEXT( msg, WIDE("Second Value is not a number.") );				\
				EnqueLink( &ps->Command->Output, &msg );							\
				return FALSE;																\
			}																					\
	flags.bIntAmount = bInt;													\

#define GET_AMOUNT_DEFAULT_INT( defaultamount )					\
	pAmount = GetParam( ps, &parameters );								 \
	if( !pAmount ) {iAmount = defaultamount; flags.bIntAmount = 1;  }	\
	else {\
			if( !pAmount || !IsSegAnyNumber( &pAmount, &fAmount, &iAmount, &bInt ) )								 \
			{																					\
				DECLTEXT( msg, WIDE("Second Value is not a number.") );				\
				EnqueLink( &ps->Command->Output, &msg );							\
				return FALSE;																\
			}																					\
			flags.bIntAmount = bInt;													\
	}


#define GET_NUMBERS( op, amountfunc, defaultamount )		 \
		if( IsSegAnyNumber( &temp, &fNumber, &iNumber, &bInt ) )															\
		{																						\
				flags.bIntNumber = bInt;													\
	amountfunc( defaultamount );											\
			DO_OPERATOR(op);																\
		}																						\
		else																					\
		{																						\
			DECLTEXT( msg, WIDE("Primary Operand is not a value.") );				\
			EnqueLink( &ps->Command->Output, &msg );								\
			return FALSE;																	\
		}

int CPROC INCREMENT( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pAmount, temp, varname;
	varname = temp = GetParam( ps, &parameters );
	if( temp )
	{
		struct {
			uint32_t bIntNumber : 1;
			uint32_t bIntAmount : 1;
		} flags;
		int64_t iNumber, iAmount;
		double fNumber, fAmount;
		int bInt;
		PTEXT pResult;
		GET_NUMBERS( +, GET_AMOUNT_DEFAULT_INT, 1 );
		if( varname->flags & TF_INDIRECT )
		{
			PTEXT pInd;
			pInd = GetIndirect( varname );
			SetIndirect( varname, pResult );
			LineRelease( pInd );
		}
		else
			SegSubst( temp, pResult ); // self modifying code... :)
	}
	return FALSE;
}
//--------------------------------------
int CPROC DECREMENT( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pAmount, temp, varname;
	varname = temp = GetParam( ps, &parameters );
	if( temp )
	{
		struct {
			uint32_t bIntNumber : 1;
			uint32_t bIntAmount : 1;
		} flags;
		int64_t iNumber, iAmount;
		double fNumber, fAmount;
		int bInt;
		PTEXT pResult;
		GET_NUMBERS( -, GET_AMOUNT_DEFAULT_INT, 1 );
		if( varname->flags & TF_INDIRECT )
		{
			PTEXT pInd;
			pInd = GetIndirect( varname );
			SetIndirect( varname, pResult );
			LineRelease( pInd );
		}
		else
			SegSubst( temp, pResult ); // self modifying code... :)
	}
	return FALSE;
}
//--------------------------------------
int CPROC MULTIPLY( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pAmount, temp, varname;
	varname = temp = GetParam( ps, &parameters );
	if( temp )
	{
		struct {
			uint32_t bIntNumber : 1;
			uint32_t bIntAmount : 1;
		} flags;
		int64_t iNumber, iAmount;
		double fNumber, fAmount;
		int bInt;
		PTEXT pResult;
		GET_NUMBERS( *, GET_AMOUNT, 0 );

		if( temp->flags & TF_INDIRECT )
		{
			PTEXT pInd;
			pInd = GetIndirect( varname );
			SetIndirect( varname, pResult );
			LineRelease( pInd );
		}
		else
			SegSubst( temp, pResult ); // self modifying code... :)
	}
	return FALSE;
}
//--------------------------------------
int CPROC DIVIDE( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pAmount, temp, varname;
	varname = temp = GetParam( ps, &parameters );
	if( temp )
	{
		struct {
			uint32_t bIntNumber : 1;
			uint32_t bIntAmount : 1;
		} flags;
		int64_t iNumber, iAmount;
		double fNumber, fAmount;
		int bInt;
		PTEXT pResult;
		GET_NUMBERS( /, GET_AMOUNT_DEFAULT_INT, 1 );
		if( varname->flags & TF_INDIRECT )
		{
			PTEXT pInd;
			pInd = GetIndirect( varname );
			SetIndirect( varname, pResult );
			LineRelease( pInd );
		}
		else
			SegSubst( temp, pResult ); // self modifying code... :)
	}
	return FALSE;
}
//--------------------------------------
int HeadTail( int bHead, PSENTIENT ps, PTEXT parameters )
{
	PTEXT pSave, pFrom, pTo, pCount;
	PTEXT pInd; // pindirect....
	pSave = parameters;
	pFrom = GetParam( ps, &parameters );
	if( !pFrom ||
		pFrom == pSave ||
		!( pFrom->flags & TF_INDIRECT ) )
	{
		DECLTEXT( msg, WIDE("Invalid variable source reference.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	pSave = parameters;
	pTo = GetParam( ps, &parameters );
	if( !pTo ||
		pTo == pSave ||
		!( pTo->flags & TF_INDIRECT ) )
	{
		DECLTEXT( msg, WIDE("Invalid variable destination reference.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	if( !bHead )
	{
		pSave = pFrom;
		pInd = NULL;
		while( pFrom && ( pFrom->flags & TF_INDIRECT ) )
		{
			pInd = pFrom;
			pFrom = GetIndirect( pFrom );
			pSave = pFrom;
			SetEnd( pFrom );
		}
		if( pSave == pFrom )  // stayed on same word at SetEnd
									// or pFrom was not an indirect...
			SetIndirect( pInd, NULL ); // no longer anywhere...
		else
			SegGrab( pFrom );

		if( ps->CurrentMacro && pFrom )
			ps->CurrentMacro->state.flags.bSuccess = TRUE;
	}
	else // get head word....
	{
		PTEXT pStart = pFrom;
	find_first:
		pInd = pSave = pFrom = pStart;
		while( pFrom && ( pFrom->flags & TF_INDIRECT ) )
		{
			pSave = pInd;
			pInd = pFrom;
			pFrom = GetIndirect( pFrom );
		}
		if( !pFrom ) {
			if( pInd != pSave )
			{
				SetIndirect( pSave, NEXTLINE(pInd) );
				SegGrab( pInd );
				LineRelease( pInd );
				goto find_first;
			}
			// else consider an error - head/tail from empty string...
		}
		else
		{
			SetIndirect( pInd, NEXTLINE( pFrom ) );
			SegGrab( pFrom );
			// so far this is good....
			if( ps->CurrentMacro && pFrom )
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
		}
	}
	pCount = GetParam( ps, &parameters );
	if( pCount )
	{
		int Len;
		if( !IsNumber( pCount ) )
		{
			DECLTEXT( msg, WIDE("Character count is not valid.") );
			EnqueLink( &ps->Command->Output, &msg );
			return FALSE;
		}
		Len = IntCreateFromSeg( pCount );
		if( Len < 0 )
		{
			DECLTEXT( msg, WIDE("Character count cannot be negative.") );
			EnqueLink( &ps->Command->Output, &msg );
			return FALSE;
		}
		LineRelease( GetIndirect( pTo ) );
		SetIndirect( pTo, NULL );
		if( !bHead )
		{
			int FromLen;
			PTEXT pSplit, pSave;
			FromLen = GetTextSize( pFrom );
			if( Len >= FromLen ) // grab the whole word
			{
				SetIndirect( pTo, pFrom );
			}
			else
			{
				pSave = SegSplit( &pFrom, FromLen - Len );
				pSplit = NEXTLINE( pSave );
				SegBreak( pSplit );
				SetIndirect( pTo, pSplit );
				SetIndirect( pInd, 
					SegAppend( GetIndirect( pInd ), pSave ) );
				//LineRelease( pFrom );
			}
		}
		else
		{
			int FromLen;
			PTEXT pSplit, pSave;
			FromLen = GetTextSize( pFrom );
			if( Len >= FromLen ) // grab the whole word...
			{
				SetIndirect( pTo, pFrom );
			}
			else
			{
				pSplit = SegSplit( &pFrom, Len );
				pSave = NEXTLINE( pSplit );
				SegBreak( pSave );
				SetIndirect( pTo, pSplit );
				SetIndirect( pInd
							 , SegAppend( pSave
											, GetIndirect( pInd ) ) );
				//LineRelease( pFrom );
			}
		}
	}
	else
	{
		LineRelease( GetIndirect( pTo ) );
		SetIndirect( pTo, pFrom );
	}
	return FALSE;
}
int CPROC VAR_HEAD( PSENTIENT ps, PTEXT parameters )
{
	return HeadTail( TRUE, ps, parameters );
}
int CPROC VAR_TAIL( PSENTIENT ps, PTEXT parameters )
{
	return HeadTail( FALSE, ps, parameters );
}


int CPROC CMD_BURST( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	if( ( temp = GetParam( ps, &parameters ) ) )
	{
		if( ps->Data )
		{
			PTEXT pInto;

			if( !(pInto = GetParam( ps, &parameters )) )
			{
				EnqueLink( &ps->Data->Input
							, burst( GetIndirect( temp ) ) );
			}
			else
			{
				PTEXT pBurst;
				pBurst = burst( GetIndirect( temp ) );
				AddVariableExxx( ps, ps->Current, pInto, pBurst, FALSE, FALSE, TRUE DBG_SRC);
				LineRelease( pBurst );
			}
		}
		else // must specify a secondary variable... to receive the parse.
		{
			PTEXT pInto;
			if( !(pInto = GetParam( ps, &parameters )) )
			{
				DECLTEXT( msg, WIDE("No Data channel - must specify a second param varname to read into.") );
				EnqueLink( &ps->Command->Output, &msg );
			}
			else
			{
				PTEXT pBurst;
				pBurst = burst( GetIndirect( temp ) );
				AddVariableExxx( ps, ps->Current, pInto, pBurst, FALSE, FALSE, TRUE  DBG_SRC);
				LineRelease( pBurst );
			}
		}
	}
	else
	{
		 if( ps->pLastResult )
		 {
				PTEXT old = ps->pLastResult;
				ps->pLastResult = burst( old );
				LineRelease( old );
				if( ps->CurrentMacro )
					ps->CurrentMacro->state.flags.bSuccess = TRUE;
		 }
	}
	return FALSE;
}
//--------------------------------------

int CPROC VAR_PUSH( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pSave, pTo, pFrom;
	pSave = parameters;
	if( pTo = GetParam( ps, &parameters ) )
	{
		if( pTo == pSave )
		{
			DECLTEXT( msg, WIDE("Invalid variable reference to PUSH") );
			EnqueLink( &ps->Command->Output, &msg );
		}
		else if( pFrom = GetParam( ps, &parameters ) )
		{
			SetIndirect( pTo, SegAppend( GetIndirect( pTo ),
												 TextDuplicate( pFrom, FALSE  ) ) );
		}
		else
		{
			DECLTEXT( msg, WIDE("Nothing specified to push into variable...") );
			EnqueLink( &ps->Command->Output, &msg );
		}
	}
	return FALSE;
}
//--------------------------------------

int CPROC VAR_POP( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pSave, pFrom, pTo;
	PTEXT pInd; // pindirect....

	pSave = parameters;
	pFrom = GetParam( ps, &parameters );
	if( !pFrom ||
		pFrom == pSave ||
		!( pFrom->flags & TF_INDIRECT ) )
	{
		DECLTEXT( msg, WIDE("Invalid variable source reference.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	pSave = parameters;
	pTo = GetParam( ps, &parameters );
	if( !pTo ||
		pTo == pSave ||
		!( pTo->flags & TF_INDIRECT ) )
	{
		DECLTEXT( msg, WIDE("Invalid variable destination reference.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	LineRelease( GetIndirect( pTo ) );
	SetIndirect( pTo, pInd = GetIndirect( pFrom ));
	SetIndirect( pFrom, pInd = NEXTLINE( pInd ) );
	SegBreak( pInd );
	if( GetIndirect( pTo ) && ps->CurrentMacro )
	ps->CurrentMacro->state.flags.bSuccess = TRUE;
	return TRUE;	
}

//--------------------------------------
int CPROC VARS( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	PVARTEXT vt;
	INDEX idx;
	PENTITY pe = ps->Current; // resonable default.
	vt = VarTextCreate();
	if( parameters )
	{
		if( ( pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL ) ) )
		{
			INDEX idx;
			PTEXT var;
			DECLTEXT( msg, WIDE("--- Object's Variables ---") );
			EnqueLink( &ps->Command->Output, &msg );
			LIST_FORALL( pe->pVars, idx, PTEXT, var )
			{
				PTEXT pText, pNext;
				pNext = NEXTLINE( var );
				vtprintf( vt, WIDE("%s %s "),
							GetText( var ),
							GetBound( pNext )
						 );
				if( NEXTLINE( var )->flags&TF_BINARY )
				{
					PTEXT pBuffer;
					int total;
					pBuffer = GetIndirect( NEXTLINE( var ) );
					total = 0;
					while( pBuffer )
					{
						total += GetTextSize( pBuffer );
						pBuffer = NEXTLINE( pBuffer );
					}
					pText = NULL;
					vtprintf( vt, WIDE("(%d) Cannot display data"), total );
				}
				else
				{
					if( var && NEXTLINE(var ) )
					{
						pText = BuildLine( NEXTLINE( var ) );
						if( pText )
							pText->format.position.offset.spaces = 1;
					}
					else
					{
						DECLTEXT( msg, WIDE("Invalid variable.") );
						pText = SegDuplicate( (PTEXT)&msg );
					}
				}
				EnqueLink( &ps->Command->Output
							, SegAppend( VarTextGet( vt ), pText ) );
			}
		}
		else
		{
			if( !ps->CurrentMacro )
			{
				PTEXT pOut;
				DECLTEXT( msg, WIDE(" is an unknown object.") );
				pOut = SegAppend( SegDuplicate( parameters )
									, SegCreateIndirect( (PTEXT)&msg ) );
				EnqueLink( &ps->Command->Output, pOut );
			}
		}
	}
	else
	{
		PTEXT var;
		DECLTEXT( msg, WIDE("--- Common Variables ---") );
		EnqueLink( &ps->Command->Output, &msg );
		LIST_FORALL( ps->Current->pVars, idx, PTEXT, var )
		{
			PTEXT pText = NULL, pNext;
			pNext = NEXTLINE( var );
			vtprintf( vt, WIDE("%s %s"),
						GetText( var ),
						GetBound( pNext )
					 );
			if( pNext )
			{
				if( pNext->flags&TF_BINARY )
				{
					PTEXT pBuffer;
					int total;
					pText = NULL;
					pBuffer = GetIndirect( NEXTLINE( var ) );
					total = 0;
					while( pBuffer )
					{
						total += GetTextSize( pBuffer );
						pBuffer = NEXTLINE( pBuffer );
					}
					vtprintf( vt, WIDE("(%d) Cannot display data"), total );
				}
				else
				{
					vtprintf( vt, WIDE("%s"),
								GetText( pText = BuildLine( NEXTLINE( var ) ) ) );
					LineRelease( pText );
				}
			}
			else
				vtprintf( vt, WIDE("Value holder missing...") );
			EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
		}
		if( ps->CurrentMacro )
		{
			DECLTEXT( msg, WIDE("--- Local Variables ---") );
			EnqueLink( &ps->Command->Output, &msg );
			LIST_FORALL( ps->CurrentMacro->pVars, idx, PTEXT, var )
			{
				PTEXT pText, pNext;
				pNext = NEXTLINE( var );
				vtprintf( vt, WIDE("%s %s"),
											GetText( var ),
											GetBound( pNext )
										);
				if( NEXTLINE( var )->flags&TF_BINARY )
				{
					PTEXT pBuffer;
					size_t total;
					pText = NULL;
					pBuffer = GetIndirect( NEXTLINE( var ) );
					total = 0;
					while( pBuffer )
					{
						total += GetTextSize( pBuffer );
						pBuffer = NEXTLINE( pBuffer );
					}
					vtprintf( vt, WIDE("(%d) Cannot display data"), total );
				}
				else
				{
					pText = BuildLine( NEXTLINE( var ) );
				}
				EnqueLink( &ps->Command->Output, SegAppend( VarTextGet( vt ), pText ) );
			}
			{
				PTEXT pVarName, pVarValue;
				DECLTEXT( msg, WIDE("--- Macro Variables ---") );
				EnqueLink( &ps->Command->Output, &msg );
				pVarName = ps->CurrentMacro->pMacro->pArgs;
				pVarValue = ps->CurrentMacro->pArgs;
				while( pVarName && pVarValue )
				{
					PTEXT pValue;
					int bRelease;
					bRelease = FALSE;
					if( pVarValue->flags & TF_INDIRECT )
					{
						bRelease = TRUE;
						pValue = BuildLine( GetIndirect(pVarValue) );
						if( !(pVarValue->flags & TF_DEEP ) )
						{
							DECLTEXT( msg, WIDE("Possible memory loss in macro parameter") );
							EnqueLink( &ps->Command->Output, &msg );
						}
					}
					else
						pValue = pVarValue;
					vtprintf( vt, WIDE("%s = %s"),
											GetText( pVarName )
										 , GetText( pValue )
											);
					if( bRelease )
						LineRelease( pValue );
					EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
					pVarName = NEXTLINE( pVarName );
					pVarValue = NEXTLINE( pVarValue );
				}
				if( pVarName )
				{
					DECLTEXT( msg, WIDE("--- not enough arguments passed") );
					EnqueLink( &ps->Command->Output, &msg );
					while( pVarName )
					{
						vtprintf( vt, WIDE("%s = ???"), 
													GetText( pVarName )
												);
						EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
						pVarName = NEXTLINE( pVarName );
					}
				}
				if( pVarValue )
				{
					DECLTEXT( msg, WIDE("--- too many arguments passed") );
					EnqueLink( &ps->Command->Output, &msg );
				}
			}
		}
	}
	VarTextDestroy( &vt );
	return FALSE;
}

//--------------------------------------
int CPROC VVARS( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	PVARTEXT vt;
	INDEX idx;
	PENTITY pe = ps->Current; // resonable default.
	vt = VarTextCreate();
	if( parameters )
	{
		if( !( pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL ) ) )
		{
			if( !ps->CurrentMacro )
			{
				PTEXT pOut;
				DECLTEXT( msg, WIDE(" is an unknown object.") );
				pOut = SegAppend( SegDuplicate( parameters )
									, SegCreateIndirect( (PTEXT)&msg ) );
				EnqueLink( &ps->Command->Output, pOut );
			}
		}
	}
	if( pe )
	{
		volatile_variable_entry *pvve;
		DECLTEXT( msg, WIDE("--- Volatile Variables ---") );
		EnqueLink( &ps->Command->Output, &msg );
		LIST_FORALL( pe->pVariables, idx, volatile_variable_entry *, pvve )
		{
			 PTEXT result;
			 DECLTEXT( blank, WIDE(" ") );
			 if( pvve->get )
			 {
						result = pvve->get( pe, &pvve->pLastValue );
					if( result )
						result = BuildLine( result );
					else
						result = (PTEXT)&blank;
			 }
			 else
					result = (PTEXT)&blank;
			 vtprintf( vt, WIDE("%s = %s"), pvve->pName, GetText( result ) );
			 EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
			 LineRelease( result );
		}
	}
	VarTextDestroy( &vt );
	return FALSE;
}


// $Log: varcmds.c,v $
// Revision 1.26  2005/08/08 09:30:24  d3x0r
// Fixed listing and handling of ON behaviors.  Protected against some common null spots.  Fixed databath destruction a bit....
//
// Revision 1.25  2005/04/20 06:20:25  d3x0r
//
// Revision 1.24  2004/04/06 17:40:57  d3x0r
// Minor cleanup on unchecked NULL situations
//
// Revision 1.23  2004/04/06 15:12:01  d3x0r
// Checkpoint
//
// Revision 1.22  2004/01/19 23:42:26  d3x0r
// Misc fixes for merging cursecon with psi/win con interfaces
//
// Revision 1.21  2003/12/10 21:59:51  panther
// Remove all LIST_ENDFORALL
//
// Revision 1.20  2003/11/07 23:45:28  panther
// Port to new vartext abstraction
//
// Revision 1.19  2003/08/02 17:39:15  panther
// Implement some of the behavior methods.
//
// Revision 1.18  2003/04/06 23:26:13  panther
// Update to new SegSplit.  Handle new formatting option (delete chars) Issue current window size to launched pty program
//
// Revision 1.17  2003/04/02 06:43:27  panther
// Continued development on ANSI position handling.  PSICON receptor.
//
// Revision 1.16  2003/03/25 08:59:03  panther
// Added CVS logging
//
