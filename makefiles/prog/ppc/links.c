//#include <windows.h>
//#include <stdio.h>
#include "mem.h"

#include <stddef.h> // offsetof
//#include <string.h> // memset

#include "links.h"

#ifdef _DEBUG
#pragma message ("Setting DBG_PASS and DBG_FORWARD to work" )
#define DBG_PASS , char *pFile, int nLine
#define DBG_SRC , __FILE__, __LINE__
#else
#pragma message ("Setting DBG_PASS and DBG_FORWARD to be ignored" )
#define DBG_PASS
#define DBG_SRC
#endif

//--------------------------------------------------------------------------

PLIST CreateListEx( DBG_VOIDPASS )
{
   PLIST pl;
   INDEX size;
   pl = AllocateEx( ( size = offsetof( LIST, pNode[0] ) ) DBG_RELAY );
   MemSet( pl, 0, size );
   return pl;
}

//--------------------------------------------------------------------------
PLIST DeleteListEx( PLIST *pList DBG_PASS )
{
   if( pList && *pList)
   {
		while( LockedExchange( &((*pList)->Lock), 1 ) )
		{
      //Sleep(0);
		}
		ReleaseExx( (void**)pList DBG_RELAY );
      *pList = NULL;
   }
   return NULL;
}

//--------------------------------------------------------------------------

PLIST ExpandListEx( PLIST *pList, size_t amount DBG_PASS )
{
   PLIST pl;
   INDEX size;
   if( !pList )
      return NULL;
   if( *pList )
      pl = AllocateEx( size = offsetof( LIST, pNode[(*pList)->Cnt+amount] ) DBG_RELAY );
   else
      pl = AllocateEx( size = offsetof( LIST, pNode[amount] ) DBG_RELAY );
	pl->Lock = TRUE; // assume a locked state...
   if( *pList )
   {
      // copy old list to new list
      MemCpy( pl, *pList, size - (4*amount /*don't include new link*/) );
      if( amount == 1 )
         pl->pNode[pl->Cnt++] = NULL;
      else
      {
         // clear the new additions to the list
         MemSet( pl->pNode + pl->Cnt, 0, amount*4 );
         pl->Cnt += amount;
      }
      // remove the old list...
      ReleaseExx( (void**)pList DBG_RELAY );
      *pList = NULL;
//      DeleteListEx( pList DBG_RELAY ); // bypass this cause it locks the list...
   }
   else
   {
      MemSet( pl, 0, size ); // clear whole structure on creation...
      pl->Cnt = amount;  // one more ( always a free )
   }
   *pList = pl;
   return pl;
}

//--------------------------------------------------------------------------

PLIST AddLinkEx( PLIST *pList, POINTER p DBG_PASS )
{
   INDEX i;
   if( !pList )
      return NULL;
	if( *pList )
		while( LockedExchange( &((*pList)->Lock), 1 ) )
		{
      //Sleep(0);
		}
   retry1:
      ExpandListEx( pList, 1 DBG_RELAY );

   for( i = 0; i < (*pList)->Cnt; i++ )
   {
      if( !(*pList)->pNode[i] )
      {
         (*pList)->pNode[i] = p;
         break;
      }
   }
   if( i == (*pList)->Cnt )
      goto retry1;  // pList->Cnt changes - don't test in WHILE
	(*pList)->Lock = 0;
   return *pList; // might be a NEW list...
}

//--------------------------------------------------------------------------

PLIST SetLinkEx( PLIST *pList, INDEX idx, POINTER p DBG_PASS )
{
   INDEX sz;
   if( !pList )
      return NULL;
   if( *pList )
     while( LockedExchange( &((*pList)->Lock), 1 ) )
     {
        //Sleep(0);
     }
   if( idx == INVALID_INDEX )
      return *pList; // not set...
   sz = 0;
   while( !(*pList) || ( sz = (*pList)->Cnt ) <= idx )
      ExpandListEx( pList, (idx - sz) + 1 DBG_RELAY );
   (*pList)->pNode[idx] = p;
	(*pList)->Lock = 0;
   return *pList; // might be a NEW list...
}

//--------------------------------------------------------------------------

POINTER GetLinkEx( PLIST *pList, INDEX idx )
{
   // must lock the list so that it's not expanded out from under us...
	POINTER p;
   if( !pList || !(*pList) )
      return NULL;
   if( idx == INVALID_INDEX )
      return pList; // not set...
   while( LockedExchange( &((*pList)->Lock), 1 ) )
   {
      //Sleep(0);
   }
   if( (*pList)->Cnt <= idx )
	{
		(*pList)->Lock = 0;
      return NULL;
	}
	p = (*pList)->pNode[idx];
	(*pList)->Lock = 0;
   return p;
}

//--------------------------------------------------------------------------

INDEX FindLink( PLIST *pList, POINTER value )
{
   INDEX i, r;
   POINTER v;
	r = INVALID_INDEX;
	/*
   while( LockedExchange( &((*pList)->Lock), 1 ) )
   {
      //Sleep(0);
   }
	*/
   FORALL( (*pList), i, POINTER, v )
      if( v == value )
      {
			r = i;
			break;
      }
	//(*pList)->Lock = 0;
   return r;
}

//--------------------------------------------------------------------------

int DeleteLinkEx( PLIST *pList, POINTER value )
{
   INDEX idx;
   if( !pList )
      return 0;
   if( !(*pList ) )
      return 0;
   while( LockedExchange( &((*pList)->Lock), 1 ) )
   {
      //Sleep(0);
   }
   if( ( idx = FindLink( pList, value ) ) != INVALID_INDEX )
   {
      (*pList)->pNode[idx] = NULL;
		(*pList)->Lock = 0;
      return TRUE;
   }
	(*pList)->Lock = 0;
   return FALSE;
}

//--------------------------------------------------------------------------

PLINKSTACK CreateLinkStack( void )
{
   PLINKSTACK pls;
   pls = Allocate( sizeof( LINKSTACK ) );
   pls->Top = 0;
   pls->Cnt = 0;
   return pls;
}

//--------------------------------------------------------------------------

void DeleteLinkStack( PLINKSTACK pls )
{
   Release( pls );
}

//--------------------------------------------------------------------------


POINTER PeekLink( PLINKSTACK pls )
{
   POINTER p = NULL;
   if( pls && pls->Top )
      p = pls->pNode[pls->Top-1];
   return p;
}

//--------------------------------------------------------------------------

POINTER PopLink( PLINKSTACK pls )
{
	POINTER p = NULL;
	if( !pls )
      return NULL;
   if( pls->Top )
   {
      pls->Top--;
      p = pls->pNode[pls->Top];
      pls->pNode[pls->Top] = (POINTER)0x1BEDCAFE;
   }
   return p;
}

//--------------------------------------------------------------------------

PLINKSTACK ExpandStack( PLINKSTACK stack, size_t entries )
#define ExpandStack(s,n)  ((s)=ExpandStack((s),(n)))
{
   PLINKSTACK pNewStack;
   if( stack )
      entries += stack->Cnt;
   pNewStack = Allocate( offsetof( LINKSTACK, pNode[entries] ) );
   if( stack )
   {
      MemCpy( pNewStack->pNode, stack->pNode, stack->Cnt * sizeof(POINTER) );
      pNewStack->Top = stack->Top;
      Release( stack );
   }
   else
      pNewStack->Top = 0;
   pNewStack->Cnt = entries;
   return pNewStack;
}

//--------------------------------------------------------------------------

#undef PushLink
PLINKSTACK PushLink( PLINKSTACK pls, POINTER p )
#define PushLink(s,p)  ((s)=PushLink((s),(p)))
{
   if( !pls ||
       pls->Top == pls->Cnt )
   {
      ExpandStack( pls, 1 );
   }
   pls->pNode[pls->Top] = p;
   pls->Top++;
   return pls;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

POINTER PopData( PDATASTACK *pds )
{
   POINTER p = NULL;
   if( (*pds)->Top )
   {
      (*pds)->Top--;
      if( (*pds)->Top )
      {
         p = (*pds)->data + ( (*pds)->Size * ((*pds)->Top-1) );
      }
   }
   return p;
}

//--------------------------------------------------------------------------

PDATASTACK ExpandDataStack( PDATASTACK *pds, size_t entries )
{
   PDATASTACK pNewStack;
   if( (*pds) )
      entries += (*pds)->Cnt;
   pNewStack = Allocate( sizeof( DATASTACK ) + ( (*pds)->Size * entries ) - 1 );
   MemCpy( pNewStack->data, (*pds)->data, (*pds)->Cnt * (*pds)->Size );
   pNewStack->Cnt = entries;
   pNewStack->Size = (*pds)->Size;
   pNewStack->Top = (*pds)->Top;
   Release( (*pds) );
   *pds = pNewStack;
   return pNewStack;
}

//--------------------------------------------------------------------------

PDATASTACK PushData( PDATASTACK *pds, POINTER pdata )
{
	if( pds && *pds )
   {
	   if( (*pds)->Top == (*pds)->Cnt )
   	{
      	ExpandDataStack( pds, 1 );
	   }
   	MemCpy( (*pds)->data + ((*pds)->Top * (*pds)->Size ), pdata, (*pds)->Size );
	   (*pds)->Top++;
   	return (*pds);
   }
   if( pds )
   	return *pds;
   return NULL;
}

//--------------------------------------------------------------------------

POINTER PeekDataEx( PDATASTACK *pds, int nBack )
{
   POINTER p = NULL;
   if( ( (int)((*pds)->Top) - nBack ) >= 0 )
      p = (*pds)->data + ( (*pds)->Size * ((*pds)->Top - nBack) );
   return p;
}

//--------------------------------------------------------------------------

POINTER PeekData( PDATASTACK *pds )
{
   POINTER p = NULL;
   if( (*pds)->Top )
      p = (*pds)->data + ( (*pds)->Size * ((*pds)->Top-1) );
   return p;
}

//--------------------------------------------------------------------------

PDATASTACK CreateDataStack( INDEX size )
{
   PDATASTACK pds;
   pds = Allocate( sizeof( DATASTACK ) );
   pds->Cnt = 0;
   pds->Top = 0;
   pds->Size = size;
   return pds;
}

//--------------------------------------------------------------------------

void DeleteDataStack( PDATASTACK *pds )
{
   Release( *pds );
   *pds = NULL;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

PLINKQUEUE CreateLinkQueue( void )
{
   PLINKQUEUE plq;
   plq = Allocate( sizeof( LINKQUEUE ) );
   plq->Top      = 0;
   plq->Bottom   = 0;
   plq->Lock     = 0;
   plq->Cnt      = 2;
   plq->pNode[0] = NULL;
   plq->pNode[1] = NULL; // shrug
   return plq;
}

//--------------------------------------------------------------------------

void DeleteLinkQueueEx( PLINKQUEUE *pplq DBG_PASS )
{
   if( pplq )
   {
      if( *pplq )
         ReleaseExx( (void**)pplq DBG_RELAY );
      *pplq = NULL;
   }
}

//--------------------------------------------------------------------------

PLINKQUEUE ExpandLinkQueue( PLINKQUEUE *pplq, int entries )
{
   PLINKQUEUE plqNew = NULL;
   if( pplq )
   {
      PLINKQUEUE plq = *pplq;

      plqNew = Allocate( offsetof( LINKQUEUE, pNode[plq->Cnt + entries] ) );
      plqNew->Cnt = plq->Cnt + entries;
      plqNew->Bottom = 0;
      if( plq->Bottom > plq->Top )
      {
         uint32_t bottom_half;
         plqNew->Top = (bottom_half = plq->Cnt - plq->Bottom ) + plq->Top;
         MemCpy( plqNew->pNode, plq->pNode + plq->Bottom, sizeof(POINTER)*bottom_half );
         MemCpy( plqNew->pNode + bottom_half, plq->pNode, sizeof(POINTER)*plq->Top );
      }
      else
      {
         plqNew->Top = plq->Top - plq->Bottom;
         MemCpy( plqNew->pNode, plq->pNode + plq->Bottom, sizeof(POINTER)*plqNew->Top );
      }
      Release( plq );
   }
   return *pplq = plqNew;
}

//--------------------------------------------------------------------------

PLINKQUEUE EnqueLinkEx( PLINKQUEUE *pplq, POINTER link DBG_PASS )
{
   INDEX tmp;
   PLINKQUEUE plq;
   if( !pplq )
      return NULL;
   if( !(*pplq) )
      *pplq = CreateLinkQueue();

   while( LockedExchange( &(*pplq)->Lock, 1 ) )
   {
		char byMsg[256];
#ifdef _DEBUG
		sprintf( byMsg, WIDE("The queue may have changed locations...(1)%s(%d)\n"),
								pFile, nLine );
#else
		sprintf( byMsg, WIDE("The queue may have changed locations...(1)\n") );
#endif
      //OutputDebugString( byMsg );
      //Sleep(0);
   }
   plq = *pplq;

   if( link )
   {
      tmp = plq->Top + 1;
      if( tmp >= plq->Cnt )
         tmp -= plq->Cnt;
      if( tmp == plq->Bottom ) // collided with self...
      {
         plq = ExpandLinkQueue( &plq, 16 );
         tmp = plq->Top + 1; // should be room at the end of phsyical array....
      }
      plq->pNode[plq->Top] = link;
      plq->Top = tmp;
   }
   *pplq = plq;
   plq->Lock = 0;
   return plq;
}

//--------------------------------------------------------------------------

int IsQueueEmpty( PLINKQUEUE *pplq  )
{
   if( !pplq || !(*pplq) ||
       (*pplq)->Bottom == (*pplq)->Top )
      return TRUE;
   return FALSE;
}

//--------------------------------------------------------------------------

POINTER DequeLink( PLINKQUEUE *pplq )
{
   POINTER p;
   INDEX tmp;
   if( pplq && *pplq )
      while( LockedExchange( &((*pplq)->Lock), 1 ) )
      {
         //Sleep(0);
      }
   else
      return NULL;
   p = NULL;
   if( (*pplq)->Bottom != (*pplq)->Top )
   {
      tmp = (*pplq)->Bottom + 1;
      if( tmp >= (*pplq)->Cnt )
         tmp -= (*pplq)->Cnt;
      p = (*pplq)->pNode[(*pplq)->Bottom];
      (*pplq)->Bottom = tmp;
   }
   (*pplq)->Lock = 0;
   return p;
}

