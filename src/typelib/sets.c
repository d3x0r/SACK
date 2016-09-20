/*
 *
 *   Crafted by Jim Buckeyne
 *    Purpose: Provide slab allocated set objects
 *      things like points, lines, etc, are cheaper to store
 *      in sets of 128, 256, instead of one at a time, since the
 *      allocation tracking block is larger than the object itself.
 *      Secondarily, this can result in compact, indexable, arrays
 *      for saving data - these resemble a PDATALIST
 *
 *  (c)1999-2006++ Freedom Collective
 */


#include <stdhdrs.h>
#include <deadstart.h>
#include <sqlgetoption.h>
#include <sharemem.h>
#include <logging.h>

#include <sack_typelib.h>

//#undef GetFromSet
//#undef GetArrayFromSet
//#undef DeleteFromSet
//#undef DeleteArrayFromSet
//#undef CountUsedInSet
//#undef GetLinearSetArray
//#undef ForAllInSet

//----------------------------------------------------------------------------
static int bLog; // put into a global structure, and configure.

#ifdef __cplusplus 
namespace sack {
	namespace containers {
	namespace sets {
		using namespace sack::memory;
		using namespace sack::logging;

#endif

#ifndef __NO_OPTIONS__
PRELOAD( InitSetLogging )
{
	bLog = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "type library/sets/Enable Logging" ), 0, TRUE );
}
#endif

void DeleteSet( GENERICSET **ppSet )
{
	GENERICSET *pSet;
 	if( !ppSet )
 		return;
 	pSet = *ppSet;
	if( bLog ) lprintf( WIDE( "Deleted set %p" ), pSet );
	while( pSet )
	{
		GENERICSET *next;
		next = pSet->next;
		Release( pSet );
		pSet = next;
	}
	*ppSet = NULL;
}

//----------------------------------------------------------------------------

PGENERICSET GetFromSetPoolEx( GENERICSET **pSetSet, int setsetsizea, int setunitsize, int setmaxcnt
							 , GENERICSET **pSet, int setsizea, int unitsize, int maxcnt DBG_PASS ){
	PGENERICSET set;
	uint32_t maxbias = 0;
	void *unit = NULL;

	if( !pSet )
		return NULL; // can never return something from nothing.

	if( !(*pSet) )
	{
		if( pSetSet )
		{
			set = GetFromSetPoolEx( NULL, 0, 0, 0, pSetSet, setsetsizea, setunitsize, setmaxcnt DBG_RELAY );
			set->nBias = 0;
		}
		else
		{
			set = (PGENERICSET)AllocateEx( setsizea DBG_RELAY );
			set->nBias = 0;
			//Log4( WIDE("Allocating a Set for %d elements sized %d total %d %08x"), maxcnt, unitsize, setsize, set );
			MemSet( set, 0, setsizea );
		}
		*pSet = set;
	}
	{
		int n;
		set = *pSet;
ExtendSet:
		while( set->nUsed == maxcnt )
		{
			if( !set->next )
			{
				PGENERICSET newset;
				if( pSetSet )
				{
					newset = GetFromSetPoolEx( NULL, 0, 0, 0, pSetSet, setsetsizea, setunitsize, setmaxcnt DBG_RELAY );
					if( set->nBias > maxbias )
						maxbias = set->nBias;
					newset->nBias = maxbias + maxcnt;
				}
				else
				{
					newset = (PGENERICSET)AllocateEx( setsizea DBG_RELAY );
					//Log4( WIDE("Allocating a Set for %d elements sized %d total %d %08x"), maxcnt, unitsize, setsize, set );
					MemSet( newset, 0, setsizea );
					if( set->nBias > maxbias )
						maxbias = set->nBias;
					newset->nBias = maxbias + maxcnt;
				}
#if 1
				if( ( newset->next = (*pSet) ) )
				{
					newset->next->me = &newset->next;
				}
				// insert newset at nead of list - then next time through
				// free ones are the first checked...
				(*(newset->me = pSet)) = newset;
#else
				set->next = newset;
				newset->me = &set->next;
#endif
				set = newset;
				// new, empty set, it's going to fail nUsed == maxcnt
				break;
			}
			else
			{
				if( set->nBias > maxbias )
					maxbias = set->nBias;
			}
			set = set->next;
		}

		while( !unit && set )
		{
			// quick skip for 32 bit blocks of used members...
			n = 0;
			for( n = 0; n < maxcnt && ((maxcnt-n) >= 32) && AllUsed( set,n ); n+=32 );
			if( n == maxcnt )
			{
				// occastionally the 'nUsed' counter may not be in sync with actual usage.
				// this set, after inspecing the bitmasks is actually full.   Update
				// the usage counter and go back up to where the set gets extended.
				set->nUsed = n;
				goto ExtendSet;
			}
			for( n = n; n < maxcnt; n++ )
			{
				if( !IsUsed( set, n ) )
				{
					unit = (void*)( ((uintptr_t)(set->bUsed))
										+ ( ( (maxcnt +31) / 32 ) * 4 ) // skip over the bUsed bitbuffer
										+ n * unitsize ); // go to the appropriate offset
					SetUsed( set, n );
					//set->nUsed++;
					break;
				}
			}
			if( n == maxcnt )
			{
				if( !set->next ) {
					// synchronize this; obviusly every member IS used.
					set->nUsed = n;
					goto ExtendSet; // for some reason didn't find anything; maybe it's a small set of less than 32 elements?
				}
				set = set->next;
			}
		}
		if( bLog ) _lprintf( DBG_RELAY )( WIDE( "Unit result: %p from %p %d %d %d %d" ), unit, set, unitsize, maxcnt, n, ( ( (maxcnt +31) / 32 ) * 4 )  );
	}
	return (PGENERICSET)unit;
}

//----------------------------------------------------------------------------

void *GetFromSetEx( GENERICSET **pSet, int setsizea, int unitsize, int maxcnt DBG_PASS )
{
	return GetFromSetPoolEx( NULL, 0, 0, 0
								  , pSet, setsizea, unitsize, maxcnt DBG_RELAY );
}

static POINTER GetSetMemberExx( GENERICSET **pSet, INDEX nMember, int setsize, int unitsize, int maxcnt, int *bUsed DBG_PASS )
{
	PGENERICSET set;
	uint32_t maxbias = 0;
	if( nMember == INVALID_INDEX )
		return NULL;
	if( !pSet )
		return NULL; // can never return something from nothing.

	if( !(*pSet) )
	{
		set = (PGENERICSET)AllocateEx( setsize DBG_RELAY );
		//Log4( WIDE("Allocating a Set for %d elements sized %d total %d %08x"), maxcnt, unitsize, setsize, set );
		MemSet( set, 0, setsize );
		set->nBias = 0;
		*pSet = set;
	}
	else
		set = (*pSet );
	while( 1 )
	{
		if( nMember >= set->nBias &&
			nMember < ( set->nBias + maxcnt ) )
		{
			nMember -= set->nBias;
			break;
		}
		if( !set->next )
		{
			PGENERICSET newset = (PGENERICSET)AllocateEx( setsize DBG_RELAY );
			//Log4( WIDE("Allocating a Set for %d elements sized %d total %d %08x"), maxcnt, unitsize, setsize, set );
			MemSet( newset, 0, setsize );
			if( set->nBias > maxbias )
				maxbias = set->nBias;
			newset->nBias = maxbias + maxcnt;
			set->next = newset;
			newset->me = &set->next;
		}
		else
		{
			if( set->nBias > maxbias )
				maxbias = set->nBias;
		}
		//nMember -= maxcnt;
		set = set->next;
	}
	if( !IsUsed( set, nMember ) )
		(*bUsed) = 0;
	else
		(*bUsed) = 1;
	if( bLog ) _lprintf(DBG_RELAY)( WIDE( "Resulting unit %" ) _PTRSZVALfs,  ((uintptr_t)(set->bUsed))
						+ ( ( (maxcnt +31) / 32 ) * 4 ) // skip over the bUsed bitbuffer
						+ nMember * unitsize );
	return (void*)( ((uintptr_t)(set->bUsed))
						+ ( ( (maxcnt +31) / 32 ) * 4 ) // skip over the bUsed bitbuffer
						+ nMember * unitsize ); // go to the appropriate offset
}

//----------------------------------------------------------------------------

POINTER GetUsedSetMemberEx( GENERICSET **pSet, INDEX nMember, int setsize, int unitsize, int maxcnt DBG_PASS )
{
	POINTER result;
	int bUsed;
	result = GetSetMemberExx( pSet, nMember, setsize, unitsize, maxcnt, &bUsed DBG_RELAY );
	if( !bUsed )
		return NULL;
	return result;
}

//----------------------------------------------------------------------------

POINTER GetSetMemberEx( GENERICSET **pSet, INDEX nMember, int setsize, int unitsize, int maxcnt DBG_PASS )
{
	POINTER result;
	int bUsed;
	if( nMember == INVALID_INDEX )
		return NULL;
	//if( nMember > 1000 )
	//	DebugBreak();
	result = GetSetMemberExx( pSet, nMember, setsize, unitsize, maxcnt, &bUsed DBG_RELAY );
	if( !bUsed )
		SetUsed( *pSet, nMember );
	return result;
}

//----------------------------------------------------------------------------

#undef GetMemberIndex
INDEX GetMemberIndex(GENERICSET **ppSet, POINTER unit, int unitsize, int max )
{
	GENERICSET *pSet = ppSet?*ppSet:NULL;
	uintptr_t nUnit = (uintptr_t)unit;
	int ofs = ( ( max + 31 ) / 32) * 4;
	int base = 0;
	while( pSet )
	{
		if( nUnit >= ((uintptr_t)(pSet->bUsed) + ofs ) &&
			 nUnit <= ((uintptr_t)(pSet->bUsed) + ofs + unitsize*max ) )
		{
			uintptr_t n = nUnit - ( ((uintptr_t)(pSet->bUsed)) + ofs );
			if( n % unitsize )
			{
				lprintf( WIDE("Error in set member alignment! %") _PTRSZVALfs WIDE(" of %d"), n % unitsize, unitsize );
				DebugBreak();
				return INVALID_INDEX;
			}
			n /= unitsize;
			return (INDEX)(n + pSet->nBias);
		}
		base += max;
		pSet = pSet->next;
	}
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------
#undef MemberValidInSet
int MemberValidInSet( GENERICSET *pSet, void *unit, int unitsize, int max )
{
	uintptr_t nUnit = (uintptr_t)unit;
	int ofs = ( ( max + 31 ) / 32) * 4;
	while( pSet )
	{
		if( nUnit >= ((uintptr_t)(pSet->bUsed) + ofs ) &&
			 nUnit <= ((uintptr_t)(pSet->bUsed) + ofs + unitsize*max ) )
		{
			uintptr_t n = nUnit - ( ((uintptr_t)(pSet->bUsed)) + ofs );
			if( n % unitsize )
			{
				lprintf( WIDE("Error in set member alignment! %") _PTRSZVALfs WIDE(" of %d"), n % unitsize, unitsize );
				DebugBreak();
				return FALSE;
			}
			n /= unitsize;
			return IsUsed( pSet, n );
		}
		pSet = pSet->next;
	}
	return FALSE;
}

//----------------------------------------------------------------------------

void DeleteFromSetExx( GENERICSET *pSet, void *unit, int unitsize, int max DBG_PASS )
{
	uintptr_t nUnit = (uintptr_t)unit;
	int ofs = ( ( max + 31 ) / 32) * 4;
	if( bLog ) _lprintf(DBG_RELAY)( WIDE("Deleting from  %p of %p "), pSet, unit );
	while( pSet )
	{
		if( bLog ) lprintf( WIDE( "range to check is %")_PTRSZVALfx WIDE("(%d) to %")_PTRSZVALfx WIDE("(%d)" )
				 ,	  ((uintptr_t)(pSet->bUsed) + ofs )
			     ,(nUnit >= ((uintptr_t)(pSet->bUsed) + ofs ))
				  , ((uintptr_t)(pSet->bUsed) + ofs + unitsize*max )
		        , (nUnit <= ((uintptr_t)(pSet->bUsed) + ofs + unitsize*max ))
				 );
		if( (nUnit >= ((uintptr_t)(pSet->bUsed) + ofs )) &&
		    (nUnit <= ((uintptr_t)(pSet->bUsed) + ofs + unitsize*max )) )
		{
			uintptr_t n = nUnit - ( ((uintptr_t)(pSet->bUsed)) + ofs );
			//Log1( WIDE("Found item in set at %d"), n / unitsize );
			if( n % unitsize )
			{
				lprintf( WIDE("Error in set member alignment! %p %p %p  %d %")_PTRSZVALfs WIDE(" %")_PTRSZVALfs WIDE(" of %d")
						 , unit
						 , pSet
						 , &pSet->bUsed
						, ofs
						 , n, n % unitsize, unitsize );
				DebugBreak();
				return;
			}
			n /= unitsize;
			ClearUsed( pSet, n );
			//pSet->nUsed--; // one not used - quick reference counter
			break;
		}
		pSet = pSet->next;
	}
	if( !pSet )
		Log( WIDE("Failed to find node in set!") );
}


//----------------------------------------------------------------------------

void DeleteSetMemberEx( GENERICSET *pSet, INDEX iMember, uintptr_t unitsize, INDEX max )
{
	//Log2( WIDE("Deleting from  %08x of %08x "), pSet, iMember );
	while( pSet )
	{
		if( iMember >= max )
		{
			iMember -= max;
			pSet = pSet->next;
			continue;
		}
		break;
	}
	if( pSet )
	{
		if( !IsUsed( pSet, iMember ) )
		{
			DebugBreak();
			lprintf( WIDE("Deleting set member which is already released? not decrementing used counter") );
		}
		else
		{
			ClearUsed( pSet, iMember );
			//pSet->nUsed--; // one not used - quick reference counter
		}
	}
	else
		Log( WIDE("Failed to find node in set!") );
}

#undef DeleteSetMember
void DeleteSetMember( GENERICSET *pSet, INDEX iMember, int unitsize, int max )
{
	DeleteSetMemberEx( pSet, iMember, unitsize, max );
}
//----------------------------------------------------------------------------

int CountUsedInSetEx( GENERICSET *pSet, int max )
{
	int cnt = 0, n;
	while( pSet )
	{
		for( n = 0; n < max; n++ )
			if( IsUsed( pSet, n ) )
				cnt++;
		pSet = pSet->next;
	}
	return cnt;
}

//----------------------------------------------------------------------------

void **GetLinearSetArrayEx( GENERICSET *pSet, int *pCount, int unitsize, int max )
{
	void  **array;
	int items, cnt, n, ofs;
	INDEX nMin, nNewMin;
	GENERICSET *pCur, *pNewMin;
	//Log2( WIDE("Building Array unit size: %d(%08x)"), unitsize, unitsize );
	items = CountUsedInSetEx( pSet, max );
	if( pCount )
		*pCount = items;
	ofs = ( ( max + 31) / 32 ) * 4;
	array = (void**)Allocate( sizeof( void* ) * items );
	nMin = 0; // 0
	do
	{
		pCur = pSet;
		nNewMin = INVALID_INDEX; // 0xFFFFFFFF (max)
		while( pCur )
		{
			// maybe instead of ordering elements
			// by ID - order by physical memory?
			// that allows findinarray to work better...
			if( (uintptr_t)pCur->nBias < nNewMin &&
				 (uintptr_t)pCur->nBias >= nMin )
			{
				pNewMin = pCur;
				nNewMin = pCur->nBias;
			}
			pCur = pCur->next;
		}
		if( (uintptr_t)nNewMin != INVALID_INDEX )
		{
			cnt = 0;
			for( n = 0; n < max; n++ )
				if( IsUsed( pNewMin, n ) )
				{
					array[cnt] = (void*)( ((uintptr_t)(pNewMin->bUsed))
												  + ofs
												  + n * unitsize );
					cnt++;
				}
		}
		nMin = nNewMin+1;
	}while( nNewMin != INVALID_INDEX );
	return array;
}

//----------------------------------------------------------------------------

int FindInArray( void **pArray, int nArraySize, void *unit )
{
	//int32_t idx;
	if( pArray )
	{
		int i, j, m;
		uintptr_t psvUnit, psvArray;
		i = 0;
		j = nArraySize-1;
		psvUnit = (uintptr_t)unit;
		do
		{
			m = (i+j)/2;
			psvArray = (uintptr_t)pArray[m];
			if( psvUnit < psvArray )
				j = m - 1;
			else if( psvUnit > psvArray )
				i = m + 1;
			else
				break;
		}
		while( i <= j );

		if( i > j )
			return -1;
		return m;
	}
	return -1;
}

//----------------------------------------------------------------------------

uintptr_t _ForAllInSet( GENERICSET *pSet, int unitsize, int max, FAISCallback f, uintptr_t psv )
{
	//Log2( WIDE("Doing all in set - size: %d setsize: %d"), unitsize, max );
	if( f )
	{
		int ofs, n;
		ofs = ( ( max + 31) / 32 ) * 4;
		while( pSet )
		{
			for( n = 0; n < max; n++ )
				if( IsUsed( pSet, n ) )
				{
					uintptr_t psvReturn;
					psvReturn = f( (void*)( ((uintptr_t)(pSet->bUsed))	
											  + ofs 
											  + n * unitsize ), psv );
					if( psvReturn )
					{
						//Log( WIDE("Return short? "));
						return psvReturn;
					}
				}
			pSet = pSet->next;
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
#undef ForEachSetMember
uintptr_t ForEachSetMember( GENERICSET *pSet, int unitsize, int max, FESMCallback f, uintptr_t psv )
{
	//Log2( WIDE("Doing all in set - size: %d setsize: %d"), unitsize, max );
	if( f )
	{
		int total = 0;
		int ofs, n;
		ofs = ( ( max + 31) / 32 ) * 4;
		while( pSet )
		{
			int nFound = 0;
			for( n = 0; nFound < (int)pSet->nUsed && n < max; n++ )
				if( IsUsed( pSet, n ) )
				{
					uintptr_t psvReturn;
					nFound++;
					psvReturn = f( total+n, psv );
					if( psvReturn )
					{
						//Log( WIDE("Return short? "));
						return psvReturn;
					}
				}
			total += n;
			pSet = pSet->next;
		}
	}
	return 0;
}

#ifdef __cplusplus 
	};//	namespace sets {
	}; //	namespace containers {
}; //namespace sack {
#endif

// $Log: sets.c,v $
// Revision 1.15  2005/05/20 21:47:10  jim
// Add base to get member index... so we don't get index of member in a set, but the actual index of the member in order... also fix a spot of set slab linking.  Also, fix resulting of the member for an index...
//
// Revision 1.14  2005/05/18 21:19:32  jim
// Define a method which will only get a valid set member from a set.
//
// Revision 1.13  2005/03/07 12:53:15  panther
// Only check what's used in a set instead of all memebers.
//
// Revision 1.12  2005/02/09 22:40:22  panther
// allow timers library to steal sets code....
//
// Revision 1.11  2005/02/04 19:25:30  panther
// Added iterator for sets that's a little different
//
// Revision 1.10  2005/01/10 21:43:42  panther
// Unix-centralize makefiles, also modify set container handling of getmember index
//
// Revision 1.9  2004/12/19 15:44:40  panther
// Extended set methods to interact with raw index numbers
//
// Revision 1.8  2004/10/04 03:56:26  d3x0r
// protect against null array passed to find_in_array
//
// Revision 1.7  2004/02/18 20:47:04  d3x0r
// Undef MemberInSet
//
// Revision 1.6  2004/02/14 01:19:04  d3x0r
// Extensions of Set structure in containers, C++ interface extension
//
// Revision 1.5  2003/04/11 16:03:53  panther
// Added  LogN for gcc.  Fixed set code to search for first available instead of add at end always.  Added MKCFLAGS MKLDFLAGS for lnx makes.
// Fixed target of APP_DEFAULT_DATA.
// Updated display to use a meta buffer between for soft cursors.
//
// Revision 1.4  2003/03/25 09:37:58  panther
// Fix file tails mangled by CVS logging
//
// Revision 1.3  2003/03/25 08:45:58  panther
// Added CVS logging tag
//
