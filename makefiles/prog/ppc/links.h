#ifndef LINKSTUFF
#define LINKSTUFF

#include "./types.h"

//PDATA CreateData( uint32_t size );
//PDATA CreateDataFromText( char *pData );
//PDATA ExpandData( PDATA pData, uint32_t amount );
//PDATA DuplicateData( PDATA pData );

//void ReleaseData( PDATA pd );

PLIST       CreateListEx   ( DBG_VOIDPASS );
PLIST       DeleteListEx   ( PLIST *plist DBG_PASS );
PLIST       AddLinkEx      ( PLIST *pList, POINTER p DBG_PASS );
PLIST       SetLinkEx      ( PLIST *pList, INDEX idx, POINTER p DBG_PASS );
POINTER     GetLinkEx      ( PLIST *pList, INDEX idx );
INDEX       FindLink       ( PLIST *pList, POINTER value );
int         DeleteLinkEx   ( PLIST *pList, POINTER value );
#define CreateList()       ( CreateListEx( DBG_VOIDSRC ) )
#define DeleteList(p)      ( DeleteListEx( &(p) DBG_SRC ) )
#define AddLink(p,v)       ( AddLinkEx( &(p),v DBG_SRC ) )
#define SetLink(p,i,v)     ( SetLinkEx( &(p),i,v DBG_SRC ) )
#define GetLink(p,i)       ( GetLinkEx( &(p),i ) )
#define DeleteLink(p,v)    ( DeleteLinkEx( &(p), v ) )

#define FORALL( l, i, t, v ) if(l) for( (i)=0; (i) < ((l)->Cnt) &&    \
                                      (((v)=(t)(l)->pNode[i]),1); (i)++ )  if( v )

PDATASTACK  CreateDataStack( INDEX size ); // sizeof data elements...
void        DeleteDataStack( PDATASTACK *pds );
PDATASTACK  PushData       ( PDATASTACK *pds, POINTER pdata );
POINTER     PopData        ( PDATASTACK *pds );
POINTER     PeekData       ( PDATASTACK *pds ); // keeps data on stack (can be used)
POINTER     PeekDataEx     ( PDATASTACK *pds, int Item ); // keeps data on stack (can be used)

PLINKSTACK  CreateLinkStack( void );
void        DeleteLinkStack( PLINKSTACK pls );
PLINKSTACK  PushLink       ( PLINKSTACK pls, POINTER p );
#define PushLink(s,p)  ((s)=PushLink((s),(p)))
POINTER     PopLink        ( PLINKSTACK pls );
POINTER     PeekLink       ( PLINKSTACK pls );

PLINKQUEUE  CreateLinkQueue( void );
void        DeleteLinkQueueEx( PLINKQUEUE *pplq DBG_PASS );
#define     DeleteLinkQueue(pplq) DeleteLinkQueueEx( pplq DBG_SRC )
PLINKQUEUE  EnqueLinkEx    ( PLINKQUEUE *pplq, POINTER link DBG_PASS );
#define     EnqueLink(pplq, link) EnqueLinkEx( pplq, link DBG_SRC )
//PLINKQUEUE  EnqueLink      ( PLINKQUEUE *pplq, POINTER link );
POINTER     DequeLink      ( PLINKQUEUE *pplq );
int         IsQueueEmpty   ( PLINKQUEUE *pplq );

#endif

