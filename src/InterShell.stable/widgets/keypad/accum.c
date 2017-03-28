#include <stdhdrs.h>
#include <sharemem.h>
#include <stdio.h>

// routines to handle accumulator registers....
#define ACCUMULATOR_STRUCTURE_DEFINED
typedef struct accumulator_tag *PACCUMULATOR;
#include "accum.h"

// maybe these should be 'saved' in 'non-volatile' ram...

#ifdef _WIN32
#define HEAP_BASE 0x2000000
#else
#define HEAP_BASE 0x20000
#endif

typedef struct accumulator_update_tag
{
	struct accumulator_update_tag *next;
	struct accumulator_update_tag **me;
	void (*Updated)( uintptr_t psv, PACCUMULATOR accum );
	uintptr_t psvUpdated;
} ACCUMULATOR_UPDATE, *PACCUMULATOR_UPDATE;

typedef struct accumulator_tag
{
	int64_t value;
	int64_t dec_base; // inversion factor for decimal application...
	uint64_t decimal;
	PVARTEXT pvt_text; // a handle-any-case type text collector...
	struct {
		uint32_t bDecimal : 1; // whether or not accumulator has a decimal.
		uint32_t bHaveDecimal : 1; // whether decimal has been entered...
		uint32_t bDollars : 1; // dollars (formatting)
		uint32_t bText : 1; // uses text operations instead of numeric
	} flags;
	struct accumulator_tag *next;
	struct accumulator_tag **me;
	struct accumulator_update_tag *updates;
	uint64_t digit_mod_mask;
	TEXTCHAR name[];

} ACCUMULATOR;


static PMEM pHeap;
static PACCUMULATOR Accumulators;

static void InvokeUpdates( PACCUMULATOR accum )
{
	ACCUMULATOR_UPDATE *update = accum->updates;
	while( update )
	{
		update->Updated( update->psvUpdated, accum );
		update = update->next;
	}
}

PACCUMULATOR SetAccumulator( PACCUMULATOR accum, int64_t value )
{
	accum->value = value;
	InvokeUpdates( accum );
	return accum;
}

int64_t GetAccumulatorValue( PACCUMULATOR accum )
{
   return accum->value;
}

void KeyIntoAccumulator( PACCUMULATOR accum, int64_t val, uint32_t base )
{
	// adds in to accumulator as if keyed in...
   //lprintf( "Accumulator %s  add %" _64fs "(%d)", accum->name, val, base );
   /*
	if( accum->flags.bHaveDecimal )
	{
      // whatever the maxint over 10 is...
		if( accum->dec_base < (0xFFFFFFFFFFFFFFFFULL / 10 ) )
		{
			accum->decimal *= base;
			accum->decimal += val;
			accum->dec_base *= 10;
		}
	}
	else
	*/
	if( accum->flags.bText )
	{
      vtprintf( accum->pvt_text, WIDE("%")_64fs, val );
	}
	else
	{
		if( accum->value < 0 )
		{
			accum->value = (-accum->value) % accum->digit_mod_mask;
			accum->value *= -(int32_t)base;
			accum->value -= val;
		}
		else
		{
			accum->value %= accum->digit_mod_mask;
			accum->value *= base;
			accum->value += val;
		}
	}
	InvokeUpdates( accum );
}

void KeyDecimalIntoAccumulator( PACCUMULATOR accum )
{
	//lprintf( "Accumulator %s  add '.'", accum->name );
	if( accum->flags.bText )
	{
      KeyTextIntoAccumulator( accum, WIDE( "." ) );
	}
	else if( accum->flags.bDecimal )
	{
		if( !accum->flags.bHaveDecimal )
		{
			accum->flags.bHaveDecimal = 1;
			accum->value *= 100; // shift left two digits (base 10)
			if( accum->flags.bDollars )
				accum->dec_base = 100;
			else
				accum->dec_base = 1;
		}
	}
	InvokeUpdates( accum );
}

void ClearAccumulatorDigit( PACCUMULATOR accum, uint32_t base )
{
	if( accum->flags.bText )
	{
      KeyTextIntoAccumulator( accum, WIDE( "\b" ) );
	}
	else
	{
		// adds in to accumulator as if keyed in...
		accum->value /= base;
	}
	InvokeUpdates( accum );
}


void ClearAccumulator( PACCUMULATOR accum )
{
	// adds in to accumulator as if keyed in...
	if( accum->flags.bText )
		VarTextEmpty( accum->pvt_text );
	else
	{
		accum->value = 0;
		accum->decimal = 0;
	}
	InvokeUpdates( accum );
}

PACCUMULATOR AddAcummulator( PACCUMULATOR accum_dest, PACCUMULATOR accum_source )
{
	if( accum_dest->flags.bText )
	{
		if( accum_source->flags.bText )
		{
			PTEXT text = VarTextPeek( accum_source->pvt_text );
			vtprintf( accum_dest->pvt_text, WIDE( "%s" ), GetText( text ) );
		}
		else
		{
			vtprintf( accum_dest->pvt_text, WIDE("%")_64fs, accum_source->value );
		}
	}
	else
	{
		accum_dest->value += accum_source->value;
	}
	InvokeUpdates( accum_dest );
	return accum_dest;
}

PACCUMULATOR TransferAccumluator( PACCUMULATOR accum_dest, PACCUMULATOR accum_source )
{
	if( accum_dest->flags.bText )
	{
		if( accum_source->flags.bText )
		{
			PTEXT text = VarTextGet( accum_source->pvt_text );
			vtprintf( accum_dest->pvt_text, WIDE( "%s" ), accum_source->pvt_text );
			Release( text );
		}
		else
		{
			vtprintf( accum_dest->pvt_text, WIDE("%")_64fs, accum_source->value );
			accum_source->value = 0;
		}
	}
	else
	{
		accum_dest->value += accum_source->value;
		accum_source->value = 0;
	}
	InvokeUpdates( accum_source );
	InvokeUpdates( accum_dest );
	return accum_dest;
}

size_t GetAccumulatorText( PACCUMULATOR accum, TEXTCHAR *text, int nLen )
{
	size_t len = 0;
	if( accum->flags.bText )
	{
		PTEXT result = VarTextPeek( accum->pvt_text );
		if( result )
		{
			StrCpyEx( text, GetText( result ), nLen );
			len = GetTextSize( result );
		}
	}
	else if( accum->flags.bDollars )
		len = snprintf( text, nLen, WIDE("$%") _64fs WIDE(".%02") _64fs
				 , accum->value / 100
				  , accum->value % 100 );
	else
		len = snprintf( text, nLen,  WIDE("%") _64fs, accum->value );
	return len;
}

PACCUMULATOR GetAccumulator( CTEXTSTR name, uint32_t flags )
{
	PACCUMULATOR accum;
	if( !pHeap )
	{
		uint32_t size = 0;
		{
			size = 0xFFF0; // don't make this ODD for ARM process
			pHeap = (PMEM)Allocate( size );
			((uintptr_t*)pHeap)[0] = 0;

			InitHeap( pHeap, size );
		}
		if( !pHeap )
		{
			lprintf( WIDE("Abort! Could not allocate accumulators!") );
			exit(1);
		}
	}

	accum = Accumulators;
	while( accum )
	{
		if( strcmp( name, accum->name ) == 0 )
			break;
		accum = accum->next;
	}
	if( !accum )
	{
		size_t len;
		accum = (PACCUMULATOR)HeapAllocate( pHeap, ( sizeof( ACCUMULATOR ) + (len = strlen( name ) + 1 ) ) * sizeof( TEXTCHAR ) );
		StrCpyEx( accum->name, name, len );
		accum->digit_mod_mask = 1000000000000000000LL;
		accum->value = 0;
		if( flags & ACCUM_DOLLARS )
			accum->flags.bDollars = 1;
		else
			accum->flags.bDollars = 0;
		if( flags & ACCUM_DECIMAL )
			accum->flags.bDecimal = 1;
		else
			accum->flags.bDecimal = 0;

		if( flags & ACCUM_TEXT )
		{
			accum->pvt_text = VarTextCreate();
			accum->flags.bText = 1;
		}
		else
		{
			accum->pvt_text = NULL;
			accum->flags.bText = 0;
		}
		accum->updates = NULL;
		if(( accum->next = Accumulators ))
			Accumulators->me = &Accumulators->next;
		Accumulators = accum;
	}
	return accum;
}

void SetAccumulatorUpdateProc( PACCUMULATOR accum
									  , void (*Updated)(uintptr_t psv, PACCUMULATOR accum )
									  , uintptr_t psvUser
									  )
{
	if( accum )
	{
		ACCUMULATOR_UPDATE *update = (PACCUMULATOR_UPDATE)HeapAllocate( pHeap, sizeof( ACCUMULATOR_UPDATE )  );
        update->Updated = Updated;
        update->psvUpdated = psvUser;
		if( update->next = accum->updates )
			accum->updates->me = &update->next;
		update->me = &accum->updates;
		accum->updates = update;
	}
}

void KeyTextIntoAccumulator( PACCUMULATOR accum, CTEXTSTR value ) // works with numeric, if value contains numeric characters
{
	if( accum )
	{
		if( accum->flags.bText )
		{
			CTEXTSTR p = value;
			//lprintf( "Accumulator %s add '%s'", accum->name, value );
			while( p[0] )
			{
				VarTextAddCharacter( accum->pvt_text, p[0] );
				p++;
			}
			InvokeUpdates( accum );
		}
		else
		{
			lprintf( "Accumulator %s is not for text... do not add %s", accum->name, value );
         // evaluate value and operate on numbers
		}
	}
}


void SetAccumulatorMask( PACCUMULATOR accum, uint64_t mask )
{
	if( !mask )
		accum->digit_mod_mask = 1000000000000000000LL;
	else
		accum->digit_mod_mask = mask;

}

