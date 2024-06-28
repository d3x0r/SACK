/*
 *  Crafted by James Buckeyne
 *
 *	(c) Freedom Collective 2000-2006++
 *
 *	code to drive standard containers, lists, stacks, queues
 *	auto expanding, auto tracking, threadsafe containers...
 *
 *  standardized to never use int.
 *
 * see also - include/typelib.h
 *
 */

#include <stddef.h> // offsetof
#include <stdhdrs.h> // Sleep
#include <deadstart.h>
#include <procreg.h>

#include <sharemem.h>

#define MY_OFFSETOF( ppstruc, member ) ((uintptr_t)&((*ppstruc)->member)) - ((uintptr_t)(*ppstruc))

#include <sack_types.h>
#include <timers.h>

#ifndef USE_CUSTOM_ALLOCER
#define USE_SACK_CUSTOM_MEMORY_ALLOCATION
 // this has to be a compile option (option from cmake)
#ifdef USE_SACK_CUSTOM_MEMORY_ALLOCATION
#define USE_CUSTOM_ALLOCER 1
#else
#define USE_CUSTOM_ALLOCER 0
#endif
#endif

#ifdef __cplusplus
namespace sack {
	namespace containers {
#endif


		//--------------------------------------------------------------------------
#ifdef __cplusplus
		namespace list {
#endif

			static struct list_local_data
			{
				volatile uint32_t lock;
			} 

#ifdef __STATIC_GLOBALS__
	s_list_local;
#  define list_local  (s_list_local)
#  define list_local_lock (&s_list_local.lock)
#else
 	* _list_local, s_list_local;
#  define list_local  ((_list_local)?(*_list_local):(s_list_local))
#  define list_local_lock ((_list_local)?(&_list_local->lock):(&s_list_local.lock))
#endif

#ifdef UNDER_CE
#define LockedExchange InterlockedExchange
#endif

			PLIST  CreateListEx( DBG_VOIDPASS )
			{
				PLIST pl;
				INDEX size;
				pl = (PLIST)AllocateEx( (size = (INDEX)offsetof( LIST, pNode[0] )) DBG_RELAY );
				MemSet( (POINTER)pl, 0, size );
				return pl;
			}

			//--------------------------------------------------------------------------
			PLIST  DeleteListEx( PLIST* pList DBG_PASS )
			{
				PLIST ppList;
				while (LockedExchange( list_local_lock, 1 ))
					Relinquish();
				if (pList &&
					(ppList = (PLIST)LockedExchangePtrSzVal( (uintptr_t*)pList, 0 ))
					)
				{
					ReleaseEx( (POINTER)ppList DBG_RELAY );
				}
				list_local_lock[0] = 0;
				return NULL;
			}

			//--------------------------------------------------------------------------

			static PLIST ExpandListEx( PLIST* pList, INDEX amount DBG_PASS )
			{
				PLIST old_list = (*pList); //-V595
				PLIST pl;
				uintptr_t size;
				uintptr_t old_size;
				if (!pList)
					return NULL;
				if (*pList)
				{
					old_size = ((uintptr_t) & ((*pList)->pNode[(*pList)->Cnt])) - ((uintptr_t)(*pList));
					size = ((uintptr_t) & ((*pList)->pNode[(*pList)->Cnt + amount])) - ((uintptr_t)(*pList));
					//old_size = offsetof( LIST, pNode[(*pList)->Cnt]));
					pl = (PLIST)AllocateEx( size DBG_RELAY );
				}
				else
				{
					old_size = 0;
					pl = (PLIST)AllocateEx( size = MY_OFFSETOF( pList, pNode[amount] ) DBG_RELAY );
					pl->Cnt = 0;
				}
				if (old_list)
				{
					// copy old list to new list
					MemCpy( (POINTER)pl, (POINTER)*pList, old_size );
					if (amount == 1)
						pl->pNode[pl->Cnt++] = NULL;
					else
					{
						// clear the new additions to the list
						MemSet( (POINTER)(pl->pNode + pl->Cnt), 0, size - old_size );
						pl->Cnt += amount;
					}
					// set the new list before releasing the old one.
					(*pList) = pl;
					// remove the old list...
					ReleaseEx( (POINTER)old_list DBG_RELAY );
				}
				else
				{
					MemSet( (POINTER)pl, 0, size ); // clear whole structure on creation...
					pl->Cnt = amount;  // one more ( always a free )
					// brand new list.
					*pList = pl;
				}
				return pl;
			}

			//--------------------------------------------------------------------------

			PLIST  AddLinkEx( PLIST* pList, POINTER p DBG_PASS )
			{
				INDEX i;
				if (!pList)
					return NULL;
				if (!(*pList))
				{
				retry1:
					ExpandListEx( pList, 8 DBG_RELAY );
				}
				else
				{
					while (LockedExchange( list_local_lock, 1 ))
						Relinquish();
					// cannot trust that the list will exist all the time
					// we may start calling this function and have the
					// list re-allocated.
					if (!(*pList))
					{
						list_local_lock[0] = 0;
						return NULL;
					}
				}

				for (i = 0; i < (*pList)->Cnt; i++)
				{
					if (!(*pList)->pNode[i])
					{
						(*pList)->pNode[i] = p;
						break;
					}
				}
				if (i == (*pList)->Cnt)
					goto retry1;  // pList->Cnt changes - don't test in WHILE
				list_local_lock[0] = 0;
				return *pList; // might be a NEW list...
			}

			//--------------------------------------------------------------------------

			PLIST  SetLinkEx( PLIST* pList, INDEX idx, POINTER p DBG_PASS )
			{
				INDEX sz;
				if (!pList)
					return NULL;
				if (*pList)
				{
					while (LockedExchange( list_local_lock, 1 ))
						Relinquish();
					if (!(*pList))
					{
						list_local_lock[0] = 0;
						return NULL;
					}
				}
				if (idx == INVALID_INDEX)
				{
					list_local_lock[0] = 0;
					return *pList; // not set...
				}
				sz = 0;
				while (!(*pList) || (sz = (*pList)->Cnt) <= idx)
					ExpandListEx( pList, (idx - sz) + 1 DBG_RELAY );
				(*pList)->pNode[idx] = p;
				list_local_lock[0] = 0;
				return *pList; // might be a NEW list...
			}

			//--------------------------------------------------------------------------

			POINTER  GetLink( PLIST* pList, INDEX idx )
			{
				// must lock the list so that it's not expanded out from under us...
				POINTER p;
				if (!pList || !(*pList))
					return NULL;
				if (idx == INVALID_INDEX)
					return NULL; // not set...
				while (LockedExchange( list_local_lock, 1 ))
					Relinquish();
				if (!(*pList))
				{
					list_local_lock[0] = 0;
					return NULL;
				}
				if ((*pList)->Cnt <= idx)
				{
					list_local_lock[0] = 0;
					return NULL;
				}
				p = (*pList)->pNode[idx];
				list_local_lock[0] = 0;
				return p;
			}

			//--------------------------------------------------------------------------

			POINTER* GetLinkAddress( PLIST* pList, INDEX idx )
			{
				// must lock the list so that it's not expanded out from under us...
				POINTER* p;
				if (!pList || !(*pList))
					return NULL;
				if (idx == INVALID_INDEX)
					return NULL; // not set...
				if ((*pList)->Cnt <= idx)
				{
					return NULL;
				}
				p = (POINTER*)((*pList)->pNode + idx);
				return p;
			}

			//--------------------------------------------------------------------------

			uintptr_t  ForAllLinks( PLIST* pList, ForProc func, uintptr_t user )
			{
				INDEX i;
				uintptr_t result = 0;
				while (LockedExchange( list_local_lock, 1 ))
					Relinquish();
				if (pList && *pList)
				{
					for (i = 0; i < ((*pList)->Cnt); i++)
					{
						if ((*pList)->pNode[i])
						{
							result = func( user, i, (POINTER*)((*pList)->pNode + i) );
							if (result)
								break;
						}
					}
				}
				list_local_lock[0] = 0;
				return result;
			}

			//--------------------------------------------------------------------------

			INDEX GetLinkCount( PLIST pList ) {
				INDEX i;
				POINTER p;
				INDEX count = 0;
				LIST_FORALL( pList, i, POINTER, p ) {
					count++;
				}
				return count;
			}

			//--------------------------------------------------------------------------

			static uintptr_t CPROC IsLink( uintptr_t value, INDEX i, POINTER* link )
			{
				if (value == (uintptr_t)(*link))
					return i + 1; // 0 might be value so add one to make it non zero
				return 0;
			}

			//--------------------------------------------------------------------------

			INDEX  FindLink( PLIST* pList, POINTER value )
			{
				if (!pList || !(*pList))
					return INVALID_INDEX;
				return ForAllLinks( pList, IsLink, (uintptr_t)value ) - 1;
			}

			//--------------------------------------------------------------------------

			static uintptr_t CPROC KillLink( uintptr_t value, INDEX i, POINTER* link )
			{
				if (value == (uintptr_t)(*link))
				{
					(*link) = NULL;
					return 1; // stop searching
				}
				return 0;
			}

			LOGICAL  DeleteLink( PLIST* pList, CPOINTER value )
			{
				if (ForAllLinks( pList, KillLink, (uintptr_t)value ))
					return TRUE;
				return FALSE;
			}

			//--------------------------------------------------------------------------

			static uintptr_t CPROC RemoveItem( uintptr_t value, INDEX i, POINTER* link )
			{
				*link = NULL;
				return 0;
			}

			void EmptyList( PLIST* pList )
			{
				ForAllLinks( pList, RemoveItem, 0 );
			}
#ifdef __cplusplus
		};//		namespace list {
		namespace data_list {
#endif
#if 0
// data list blocks were never auto-locked?
			static struct data_list_local_data
			{
				uint32_t lock;
			} s_data_list_local, * _data_list_local;
#ifdef __STATIC_GLOBALS__
#  define data_list_local  ((s_data_list_local))
#  define data_list_local_lock  ((&s_data_list_local.lock))
#else
#  define data_list_local  ((_data_list_local)?(*_data_list_local):(s_data_list_local))
#  define data_list_local_lock  ((_data_list_local)?(&_data_list_local->lock):(&s_data_list_local.lock))
#endif
#endif
			//--------------------------------------------------------------------------

			PDATALIST ExpandDataListEx( PDATALIST* ppdl, INDEX entries DBG_PASS )
			{
				PDATALIST pdl = (*ppdl); //-V595
				PDATALIST pNewList;
				if (!ppdl || !*ppdl)
					return NULL; // can't expand - was not created (no data size)
				if ((*ppdl))
					entries += (*ppdl)->Avail;
				pNewList = (PDATALIST)AllocateEx( sizeof( DATALIST ) + ((*ppdl)->Size * entries) - 1 DBG_RELAY );
				MemCpy( (POINTER)pNewList->data, (POINTER)(*ppdl)->data, (*ppdl)->Avail * (*ppdl)->Size );
				pNewList->Cnt = (*ppdl)->Cnt;
				pNewList->Avail = entries;
				pNewList->Size = (*ppdl)->Size;
				// set the new list int he pointer
				*ppdl = pNewList;
				ReleaseEx( (POINTER)pdl DBG_RELAY );
				return pNewList;
			}

			//--------------------------------------------------------------------------

			PDATALIST  CreateDataListEx( uintptr_t nSize DBG_PASS )
			{
				PDATALIST pdl = (PDATALIST)AllocateEx( sizeof( DATALIST ) + (nSize * 8) - 1 DBG_RELAY );
				pdl->Cnt = 0;
				pdl->Avail = 8;
				pdl->Size = nSize;
				return pdl;
			}

			//--------------------------------------------------------------------------

			void  DeleteDataListEx( PDATALIST* ppdl DBG_PASS )
			{
				if (ppdl)
				{
					if (*ppdl)
					{
						ReleaseEx( (POINTER)*ppdl DBG_RELAY );
						*ppdl = NULL;
					}
				}
			}

			//--------------------------------------------------------------------------

			POINTER SetDataItemEx( PDATALIST* ppdl, INDEX idx, POINTER data DBG_PASS )
			{
				POINTER p = NULL;
				if (!ppdl || !(*ppdl) || idx > 0x1000000)
					return NULL;
				if (idx >= (*ppdl)->Avail)
				{
					ExpandDataListEx( ppdl, idx + 32 DBG_RELAY );
				}
				p = (POINTER)((*ppdl)->data + ((*ppdl)->Size * idx));
				MemCpy( p, data, (*ppdl)->Size );
				if (idx >= (*ppdl)->Cnt)
					(*ppdl)->Cnt = idx + 1;
				return p;
			}

			//--------------------------------------------------------------------------

			POINTER AddDataItemEx( PDATALIST* ppdl, POINTER data DBG_PASS )
			{
				if (ppdl && *ppdl)
					return SetDataItemEx( ppdl, (*ppdl)->Cnt + 1, data DBG_RELAY );
				if (ppdl)
					return SetDataItemEx( ppdl, 0, data DBG_RELAY );
				return NULL;
			}

			void EmptyDataList( PDATALIST* ppdl )
			{
				if (ppdl && (*ppdl))
					(*ppdl)->Cnt = 0;
			}

			//--------------------------------------------------------------------------

			void DeleteDataItem( PDATALIST* ppdl, INDEX idx )
			{
				if (ppdl && *ppdl)
				{
					if (idx < ((*ppdl)->Cnt - 1))
						MemCpy( (POINTER)((*ppdl)->data + ((*ppdl)->Size * idx))
							, (POINTER)((*ppdl)->data + ((*ppdl)->Size * (idx + 1)))
							, (*ppdl)->Size * ( (*ppdl)->Cnt - idx - 1 ) );
					(*ppdl)->Cnt--;
				}
			}

			//--------------------------------------------------------------------------

			POINTER GetDataItem( PDATALIST* ppdl, INDEX idx )
			{
				POINTER p = NULL;
				if (ppdl && *ppdl && (idx < (*ppdl)->Cnt))
					p = (POINTER)((*ppdl)->data + ((*ppdl)->Size * idx));
				return p;
			}

			//--------------------------------------------------------------------------

#ifdef __cplusplus
		};//		namespace data_list {
		namespace link_stack {
#endif

			PLINKSTACK		CreateLinkStackLimitedEx( int max_entries  DBG_PASS )
			{
				PLINKSTACK pls;
				pls = (PLINKSTACK)AllocateEx( sizeof( LINKSTACK ) DBG_RELAY );
				pls->Top = 0;
				pls->Cnt = 0;
				pls->Max = max_entries;
				return pls;
			}

			//--------------------------------------------------------------------------

			PLINKSTACK  CreateLinkStackEx( DBG_VOIDPASS )
			{
				return CreateLinkStackLimitedEx( 0 DBG_RELAY );
			}

			//--------------------------------------------------------------------------

			void  DeleteLinkStackEx( PLINKSTACK* pls DBG_PASS )
			{
				if (pls && *pls)
				{
					ReleaseEx( (POINTER)*pls DBG_RELAY );
					*pls = 0;
				}
			}

			//--------------------------------------------------------------------------

			POINTER  PeekLinkEx( PLINKSTACK* pls, INDEX n )
			{
				// should lock - but it's fast enough?
				POINTER p = NULL;
				if (pls && *pls && ((*pls)->Top > n))
					p = (*pls)->pNode[(*pls)->Top - (n + 1)];
				return p;
			}

			//--------------------------------------------------------------------------

			POINTER  PeekLink( PLINKSTACK* pls )
			{
				return PeekLinkEx( pls, 0 );
			}

			//--------------------------------------------------------------------------

			POINTER  PopLink( PLINKSTACK* pls )
			{
				if (pls && *pls && (*pls)->Top)
					return (*pls)->pNode[--(*pls)->Top];
				return NULL;
			}

			//--------------------------------------------------------------------------

			static PLINKSTACK ExpandStackEx( PLINKSTACK* stack, INDEX entries DBG_PASS )
			{
				PLINKSTACK pNewStack;
				if (*stack)
					entries += (*stack)->Cnt;
				pNewStack = (PLINKSTACK)AllocateEx( my_offsetof( stack, pNode[entries] ) DBG_RELAY ); //-V595
				if (*stack)
				{
					PLINKSTACK pls = (*stack);
					MemCpy( (POINTER)pNewStack->pNode, (POINTER)(*stack)->pNode, (*stack)->Cnt * sizeof( POINTER ) );
					pNewStack->Top = (*stack)->Top;
					pNewStack->Max = (*stack)->Max;
					*stack = pNewStack;
					ReleaseEx( (POINTER)pls DBG_RELAY );
				}
				else
				{
					pNewStack->Top = 0;
					pNewStack->Max = 0;
					*stack = pNewStack;
				}
				pNewStack->Cnt = entries;
				return pNewStack;
			}

			//--------------------------------------------------------------------------

			PLINKSTACK  PushLinkEx( PLINKSTACK* pls, POINTER p DBG_PASS )
			{
				if (!pls)
					return NULL;
				// should lock this thing :)
				if (!*pls ||
					(*pls)->Top == (*pls)->Cnt)
				{
					ExpandStackEx( pls, ((*pls) ? ((*pls)->Max) : 0) + 8 DBG_RELAY );
				}
				if ((*pls)->Max)
					if (((*pls)->Top) >= (*pls)->Max)
					{
						MemCpy( (POINTER)(*pls)->pNode, (POINTER)((*pls)->pNode + 1), (*pls)->Top - 1 );
						(*pls)->Top--;
					}
				(*pls)->pNode[(*pls)->Top] = p;
				(*pls)->Top++;
				return (*pls);
			}
#ifdef __cplusplus
		}//namespace link_stack
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#ifdef __cplusplus
		namespace data_stack {
#endif

			POINTER  PopData( PDATASTACK* pds )
			{
				POINTER p = NULL;
				if ((pds) && (*pds) && (*pds)->Top)
				{
					(*pds)->Top--;
					p = (POINTER)((*pds)->data + ((*pds)->Size * ((*pds)->Top)));
				}
				return p;
			}

			//--------------------------------------------------------------------------

			static PDATASTACK ExpandDataStackEx( PDATASTACK* ppds, INDEX entries DBG_PASS )
			{
				PDATASTACK pNewStack;
				PDATASTACK pds = (*ppds);
				if (!pds)
					return NULL;

				entries += pds->Cnt;
				pNewStack = (PDATASTACK)AllocateEx( sizeof( DATASTACK ) + ((*ppds)->Size * entries) - 1 DBG_RELAY );
				MemCpy( (POINTER)pNewStack->data, (POINTER)(*ppds)->data, (*ppds)->Cnt * (*ppds)->Size );
				pNewStack->Cnt = entries;
				pNewStack->Size = (*ppds)->Size;
				pNewStack->Top = (*ppds)->Top;
				(*ppds) = pNewStack;
				ReleaseEx( (POINTER)pds DBG_RELAY );
				return pNewStack;
			}

			//--------------------------------------------------------------------------

			PDATASTACK  PushDataEx( PDATASTACK* pds, POINTER pdata DBG_PASS )
			{
				if (pds && *pds)
				{
					if ((*pds)->Top == (*pds)->Cnt)
					{
						ExpandDataStackEx( pds, 1 DBG_RELAY );
					}
					if ((*pds)->Max)
						if (((*pds)->Top) >= (*pds)->Max)
						{
							MemCpy( (POINTER)(*pds)->data, (POINTER)((*pds)->data + (*pds)->Size), ((*pds)->Top - 1) * (*pds)->Size );
							(*pds)->Top--;
						}
					MemCpy( (POINTER)((*pds)->data + ((*pds)->Top * (*pds)->Size)), pdata, (*pds)->Size );
					(*pds)->Top++;
					return (*pds);
				}
				if (pds)
					return *pds;
				return NULL;
			}

			//--------------------------------------------------------------------------

			POINTER  PeekDataEx( PDATASTACK* pds, INDEX nBack )
			{
				POINTER p = NULL;
				nBack++;
				if (!(*pds))
					return NULL;
				if (((int)((*pds)->Top) - (int)nBack) >= 0)
					p = (POINTER)((*pds)->data + ((*pds)->Size * ((*pds)->Top - nBack)));
				return p;
			}

			//--------------------------------------------------------------------------

			POINTER  PeekData( PDATASTACK* pds )
			{
				POINTER p = NULL;
				if (pds && *pds && (*pds)->Top)
					p = (POINTER)((*pds)->data + ((*pds)->Size * ((*pds)->Top - 1)));
				return p;
			}

			//--------------------------------------------------------------------------

			void  EmptyDataStack( PDATASTACK* pds )
			{
				if (pds && *pds)
					(*pds)->Top = 0;
			}

			//--------------------------------------------------------------------------

			PDATASTACK  CreateDataStackEx( size_t size DBG_PASS )
			{
				return CreateDataStackLimitedEx( size, 0 DBG_RELAY );
			}

			//--------------------------------------------------------------------------

			PDATASTACK  CreateDataStackLimitedEx( size_t size, INDEX max_items DBG_PASS )
			{
				PDATASTACK pds;
				pds = (PDATASTACK)AllocateEx( sizeof( DATASTACK ) + (10 * size) DBG_RELAY );
				pds->Cnt = 10;
				pds->Top = 0;
				pds->Size = size;
				pds->Max = max_items;
				return pds;
			}

			//--------------------------------------------------------------------------

			void DeleteDataStackEx( PDATASTACK* pds DBG_PASS )
			{
				ReleaseEx( (POINTER)(*pds) DBG_RELAY );
				*pds = NULL;
			}
#ifdef __cplusplus
		}//		namespace data_stack {
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#ifdef __cplusplus
		namespace queue {
#endif

			static struct link_queue_local_data
			{
				volatile uint32_t lock;
				//#if !USE_CUSTOM_ALLOCER
				volatile PTHREAD thread;
				//#endif
			} s_link_queue_local, * _link_queue_local;

#ifdef __STATIC_GLOBALS__
#  define link_queue_local  ((s_link_queue_local))
#  define link_queue_local_thread  ((s_link_queue_local.thread))
#  define link_queue_local_lock  ((&s_link_queue_local.lock))
#else
#  define link_queue_local  ((_link_queue_local)?(*_link_queue_local):(s_link_queue_local))
#  define link_queue_local_thread  ((_link_queue_local)?(*_link_queue_local).thread:(s_link_queue_local.thread))
#  define link_queue_local_lock  ((_link_queue_local)?(&_link_queue_local->lock):(&s_link_queue_local.lock))
#endif


			PLINKQUEUE CreateLinkQueueEx( DBG_VOIDPASS )
			{
				PLINKQUEUE plq = 0;
				plq = (PLINKQUEUE)AllocateEx( MY_OFFSETOF( &plq, pNode[8] ) DBG_RELAY ); //-V557
#if USE_CUSTOM_ALLOCER
				plq->Lock = 0;
#endif
				plq->Top = 0;
				plq->Bottom = 0;
				plq->Cnt = 8;
				plq->pNode[0] = NULL;
				plq->pNode[1] = NULL; // shrug
				return plq;
			}

			//--------------------------------------------------------------------------

			void DeleteLinkQueueEx( PLINKQUEUE* pplq DBG_PASS )
			{
				if (!pplq)
					return;

#if USE_CUSTOM_ALLOCER
				retry_lock :
#endif
				while (LockedExchange( link_queue_local_lock, __LINE__ ))
				{
					Relinquish();
				}
#if USE_CUSTOM_ALLOCER
				if (_link_queue_local)
					_link_queue_local->thread = MakeThread();
				if (!(*pplq))
				{
					link_queue_local_lock[0] = 0;
					return;
				}
				if ((*pplq)->Lock)
				{
					link_queue_local_lock[0] = 0;
					Relinquish();
					goto retry_lock;
				}
				(*pplq)->Lock = 1;
#endif
				link_queue_local_lock[0] = 0;

				if (pplq)
				{
					if (*pplq)
						ReleaseEx( (POINTER)(*pplq) DBG_RELAY );
					*pplq = NULL;
				}
#if USE_CUSTOM_ALLOCER
				if (_link_queue_local)
					_link_queue_local->thread = NULL;
#endif
				//link_queue_local_lock[0] = 0;
			}

			//--------------------------------------------------------------------------

			static PLINKQUEUE ExpandLinkQueueEx( PLINKQUEUE* pplq, INDEX entries DBG_PASS )
			{
				PLINKQUEUE plqNew = NULL;
#if USE_CUSTOM_ALLOCER
				while (LockedExchange( link_queue_local_lock, __LINE__ ))
				{
					Relinquish();
				}
				if (_link_queue_local)
					_link_queue_local->thread = MakeThread();
#endif
				if (pplq)
				{
					PLINKQUEUE plq = *pplq;
					INDEX size;
					int prior_logging;
					size = MY_OFFSETOF( pplq, pNode[plq->Cnt + entries] );
					prior_logging = SetAllocateLogging( FALSE );
					plqNew = (PLINKQUEUE)AllocateEx( size DBG_RELAY );
					plqNew->Cnt = plq->Cnt + entries;
					plqNew->Bottom = 0;
					if (plq->Bottom > plq->Top)
					{
						INDEX bottom_half;
						plqNew->Top = (bottom_half = plq->Cnt - plq->Bottom) + plq->Top;
						MemCpy( (POINTER)plqNew->pNode, (POINTER)(plq->pNode + plq->Bottom), sizeof( POINTER ) * bottom_half );
						MemCpy( (POINTER)(plqNew->pNode + bottom_half), (POINTER)plq->pNode, sizeof( POINTER ) * plq->Top );
					}
					else
					{
						plqNew->Top = plq->Top - plq->Bottom;
						MemCpy( (POINTER)plqNew->pNode, (POINTER)(plq->pNode + plq->Bottom), sizeof( POINTER ) * plqNew->Top );
					}
					//need to make sure plq is always valid; can be trying to get a lock
					(*pplq) = plqNew;
					Release( plq );
					SetAllocateLogging( prior_logging );
				}
#if USE_CUSTOM_ALLOCER
				if (_link_queue_local)
					_link_queue_local->thread = NULL;
				link_queue_local_lock[0] = 0;
#endif
				return plqNew;
			}

			//--------------------------------------------------------------------------

			PLINKQUEUE  EnqueLinkEx( PLINKQUEUE* pplq, POINTER link DBG_PASS )
			{
				INDEX tmp;
				PLINKQUEUE plq;
#if USE_CUSTOM_ALLOCER
				int keep_lock = 0;
#endif
				if (!pplq)
					return NULL;
				if (!(*pplq))
					*pplq = CreateLinkQueueEx( DBG_VOIDRELAY );
#if USE_CUSTOM_ALLOCER
				retry_lock :
#endif
				while (LockedExchange( link_queue_local_lock, __LINE__ ))
				{
#if USE_CUSTOM_ALLOCER
					if (link_queue_local_thread == MakeThread())
					{
						keep_lock = 1;
						break;
					}
#endif
					Relinquish();
				}
#if USE_CUSTOM_ALLOCER
				if (_link_queue_local)
					_link_queue_local->thread = MakeThread();
				if (!(*pplq))
				{
					if (!keep_lock)
						link_queue_local_lock[0] = 0;
					return (*pplq);
				}
				if ((*pplq)->Lock)
				{
					if (!keep_lock)
						link_queue_local_lock[0] = 0;
					Relinquish();
					goto retry_lock;
				}
				(*pplq)->Lock = 1;
				if (!keep_lock)
				{
					if (_link_queue_local)
						_link_queue_local->thread = NULL;
					link_queue_local_lock[0] = 0;
				}
#else
				if (!(*pplq))
				{
					//it could have been deallocated
					link_queue_local_lock[0] = 0;
					return (*pplq);
				}
#endif


				plq = *pplq;
				//else
				//	s_link_queue_local.thread = MakeThread();
				if (link)
				{
					tmp = plq->Top + 1;
					if (tmp >= plq->Cnt)
						tmp -= plq->Cnt;
					if (tmp == plq->Bottom) // collided with self...
					{
						plq = ExpandLinkQueueEx( pplq, 16 DBG_RELAY );
						tmp = plq->Top + 1; // should be room at the end of phsyical array....
					}
					plq->pNode[plq->Top] = link;
					plq->Top = tmp;
				}
				*pplq = plq;
#if USE_CUSTOM_ALLOCER
				plq->Lock = 0;
#endif
				link_queue_local_lock[0] = 0;
				return plq;
			}

			//--------------------------------------------------------------------------
			void EnqueLinkNLEx( PLINKQUEUE* pplq, POINTER link DBG_PASS )
			{
				INDEX tmp, t, c;
				PLINKQUEUE plq;
				if (!pplq)
					return;
				if (!(*pplq))
					*pplq = CreateLinkQueueEx( DBG_VOIDRELAY );

				plq = *pplq;
				if (link)
				{
					tmp = (t = plq->Top) + 1;
					if (tmp >= (c = plq->Cnt))
						tmp -= c;
					if (tmp == (plq->Bottom)) // collided with self...
					{
						plq = ExpandLinkQueueEx( pplq, 16 DBG_RELAY );
						tmp = (t = plq->Top) + 1; // should be room at the end of phsyical array....
					}
					plq->pNode[t] = link;
					plq->Top = tmp;
				}
				*pplq = plq;
			}

			//--------------------------------------------------------------------------

			PLINKQUEUE  PrequeLinkEx( PLINKQUEUE* pplq, POINTER link DBG_PASS )
			{
				INDEX tmp;
				PLINKQUEUE plq;
				if (!pplq)
					return NULL;
				if (!(*pplq))
					*pplq = CreateLinkQueueEx( DBG_VOIDRELAY );
#if USE_CUSTOM_ALLOCER
				retry_lock :
#endif
				while (LockedExchange( link_queue_local_lock, __LINE__ ))
					Relinquish();
#if USE_CUSTOM_ALLOCER
				if (!(*pplq))
				{
					link_queue_local_lock[0] = 0;
					return NULL;
				}
				if ((*pplq)->Lock)
				{
					link_queue_local_lock[0] = 0;
					Relinquish();
					goto retry_lock;
				}
				(*pplq)->Lock = 1;
				link_queue_local_lock[0] = 0;
#else
				if (!(*pplq))
				{
					//it could have been deallocated
					link_queue_local_lock[0] = 0;
					return (*pplq);
				}
#endif

				plq = *pplq;

				if (link)
				{
					tmp = plq->Bottom - 1;
					if (tmp & 0x80000000)
						tmp += plq->Cnt;
					if (tmp == plq->Top) // collided with self...
					{
						plq = ExpandLinkQueueEx( pplq, 16 DBG_RELAY );
						tmp = plq->Cnt - 1; // should be room at the end of phsyical array....
					}
					plq->pNode[tmp] = link;
					plq->Bottom = tmp;
				}
#if USE_CUSTOM_ALLOCER
				plq->Lock = 0;
#endif
				link_queue_local_lock[0] = 0;
				return plq;
			}

			//--------------------------------------------------------------------------

			LOGICAL  IsQueueEmpty( PLINKQUEUE* pplq )
			{
				if (!pplq || !(*pplq) ||
					(*pplq)->Bottom == (*pplq)->Top)
					return TRUE;
				return FALSE;
			}

			//--------------------------------------------------------------------------

			INDEX  GetQueueLength( PLINKQUEUE plq )
			{
				INDEX used = 0;
				if (plq)
				{
					used = plq->Top - plq->Bottom;
					if (plq->Top < plq->Bottom)
						used += plq->Cnt;
				}
				return used;
			}

			//--------------------------------------------------------------------------
			POINTER  PeekQueueEx( PLINKQUEUE plq, int idx )
			{
				size_t top;
				if (!plq)
					return NULL;
				if (idx < 0)
				{
					idx++;
					for (top = plq->Top ? (plq->Top - 1) : (plq->Cnt - 1)
						; idx && top != plq->Bottom
						; )
					{
						idx++;
						if (!top) top = plq->Cnt - 1;
						else top--;
					}
					if (idx == 0)
					{
						if (plq->Top == plq->Bottom)
							return NULL;
						return plq->pNode[top];
					}
				}
				else
				{
					for (top = plq->Bottom
						; idx != -1 && top != plq->Top
						; )
					{
						if (idx) {
							top++;
							if (top >= plq->Cnt)
								top -= plq->Cnt;
							idx--;
						}
						else { idx = -1; break; }
					}
					if (idx == -1)
						return plq->pNode[top];
				}
				return NULL;
			}

			POINTER  PeekQueue( PLINKQUEUE plq )
			{
				return PeekQueueEx( plq, 0 );
			}

			//--------------------------------------------------------------------------

			POINTER  DequeLink( PLINKQUEUE* pplq )
			{
				POINTER p;
				INDEX tmp;
				if (pplq && *pplq)
				{
#if USE_CUSTOM_ALLOCER
					int keep_lock = 0;
#endif
					uint32_t priorline;
#if USE_CUSTOM_ALLOCER
					retry_lock :
#endif
					while ((priorline = LockedExchange( link_queue_local_lock, __LINE__ )))
					{
#if USE_CUSTOM_ALLOCER
						if (link_queue_local_thread == MakeThread())
						{
							keep_lock = 1;
							break;
						}
#endif
						Relinquish();
					}
#if USE_CUSTOM_ALLOCER
					if (!pplq)
					{
						if (!keep_lock)
							link_queue_local_lock[0] = 0;
						return NULL;
					}
					if ((*pplq)->Lock)
					{
						if (!keep_lock)
							link_queue_local_lock[0] = 0;
						Relinquish();
						goto retry_lock;
					}
					(*pplq)->Lock = 1;
					if (!keep_lock)
						link_queue_local_lock[0] = 0;
#else
					if (!(*pplq))
					{
						//it could have been deallocated
						link_queue_local_lock[0] = 0;
						return NULL;
					}
#endif
				}
				else
					return NULL;

				p = NULL;
				if ((*pplq)->Bottom != (*pplq)->Top)
				{
					tmp = (*pplq)->Bottom + 1;
					if (tmp >= (*pplq)->Cnt)
						tmp -= (*pplq)->Cnt;
					p = (*pplq)->pNode[(*pplq)->Bottom];
					(*pplq)->Bottom = tmp;
				}
#if USE_CUSTOM_ALLOCER
				( *pplq )->Lock = 0;
#endif
				link_queue_local_lock[0] = 0;
				return p;
			}

			POINTER  DequeLinkNL( PLINKQUEUE* pplq )
			{
				INDEX b, t, c, tmp;
				POINTER p;
				if (!pplq || !*pplq)
					return NULL;

				p = NULL;
				if ((b = (*pplq)->Bottom) != (t = (*pplq)->Top))
				{
					tmp = b + 1;
					if (tmp >= (c = (*pplq)->Cnt))
						tmp -= c;
					p = (*pplq)->pNode[b];
					(*pplq)->Bottom = tmp;
				}
				return p;
			}

#ifdef __cplusplus
		}//		namespace queue {
#endif


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
#ifdef __cplusplus
		namespace data_queue {
#endif

			static struct data_queue_local_data
			{
				volatile uint32_t lock;
			} s_data_queue_local, * _data_queue_local;

#ifdef __STATIC_GLOBALS__
#  define data_queue_local  ((s_data_queue_local))
#  define data_queue_local_lock ((&s_data_queue_local.lock))
#else
#  define data_queue_local  ((_data_queue_local)?(*_data_queue_local):(s_data_queue_local))
#  define data_queue_local_lock ((_data_queue_local)?(&_data_queue_local->lock):(&s_data_queue_local.lock))
#endif

			PDATAQUEUE CreateDataQueueEx( INDEX size DBG_PASS )
			{
				PDATAQUEUE pdq;
				pdq = (PDATAQUEUE)AllocateEx( ((sizeof( DATAQUEUE ) + (2 * size)) - 1) DBG_RELAY );
				pdq->Top = 0;
				pdq->Bottom = 0;
				pdq->ExpandBy = 16;
				pdq->Size = size;
				pdq->Cnt = 2;
				return pdq;
			}

			//--------------------------------------------------------------------------

			void DeleteDataQueueEx( PDATAQUEUE* ppdq DBG_PASS )
			{
				if (ppdq)
				{
					if (*ppdq)
						ReleaseEx( (POINTER)*ppdq DBG_RELAY );
					*ppdq = NULL;
				}
			}

			//--------------------------------------------------------------------------

			static PDATAQUEUE ExpandDataQueueEx( PDATAQUEUE* ppdq, INDEX entries DBG_PASS )
			{
				PDATAQUEUE pdqNew = NULL;
				if (ppdq)
				{
					PDATAQUEUE pdq = *ppdq;
					//pdq->Cnt += entries;
					pdqNew = (PDATAQUEUE)AllocateEx( (uint32_t)offsetof( DATAQUEUE, data[0] ) + ((pdq->Cnt + entries) * pdq->Size) DBG_RELAY );
					pdqNew->Cnt = pdq->Cnt + entries;
					pdqNew->ExpandBy = pdq->ExpandBy;
					pdqNew->Bottom = 0;
					pdqNew->Size = pdq->Size;
					if (pdq->Bottom > pdq->Top)
					{
						INDEX bottom_half;
						/* if you see '- entries' in a diff... it was decided to not add it to the original queue above, instead */
						pdqNew->Top = (bottom_half = (pdq->Cnt) - pdq->Bottom) + pdq->Top;
						MemCpy( (POINTER)pdqNew->data
							, (POINTER)(pdq->data + (pdq->Bottom * pdq->Size))
							, pdq->Size * bottom_half );
						MemCpy( (POINTER)(pdqNew->data + (bottom_half * pdq->Size))
							, (POINTER)pdq->data
							, pdq->Size * pdq->Top );
					}
					else
					{
						pdqNew->Top = pdq->Top - pdq->Bottom;
						MemCpy( (POINTER)pdqNew->data
							, (POINTER)(pdq->data + (pdq->Bottom * pdq->Size))
							, pdq->Size * pdqNew->Top );
					}
					(*ppdq) = pdqNew;
					Release( pdq );
				}
				return pdqNew;
			}

			PDATAQUEUE  CreateLargeDataQueueEx( INDEX size, INDEX entries, INDEX expand DBG_PASS )
			{
				PDATAQUEUE pdq = CreateDataQueueEx( size DBG_RELAY );
				pdq->ExpandBy = expand;
				ExpandDataQueueEx( &pdq, entries DBG_RELAY );
				return pdq;
			}

			//--------------------------------------------------------------------------

			PDATAQUEUE  EnqueDataEx( PDATAQUEUE* ppdq, POINTER link DBG_PASS )
			{
				INDEX tmp;
				PDATAQUEUE pdq;
				if (!ppdq)
					return NULL;
				if (!(*ppdq))
					return NULL; // cannot create this - no idea how big.

				while (LockedExchange( data_queue_local_lock, 1 ))
					Relinquish();

				pdq = *ppdq;

				if (link)
				{
					tmp = pdq->Top + 1;
					if (tmp >= pdq->Cnt)
						tmp -= pdq->Cnt;
					if (tmp == pdq->Bottom) // collided with self...
					{
						pdq = ExpandDataQueueEx( ppdq, (*ppdq)->ExpandBy DBG_RELAY );
						tmp = pdq->Top + 1; // should be room at the end of phsyical array....
					}
					MemCpy( (POINTER)(pdq->data + (pdq->Top * pdq->Size)), link, pdq->Size );
					pdq->Top = tmp;
				}
				data_queue_local_lock[0] = 0;
				return pdq;
			}

			//--------------------------------------------------------------------------

			PDATAQUEUE  PrequeDataEx( PDATAQUEUE* ppdq, POINTER link DBG_PASS )
			{
				INDEX tmp;
				PDATAQUEUE pdq;
				if (!ppdq)
					return NULL;
				if (!(*ppdq))
					return NULL; // cannot create this - no idea how big.

				while (LockedExchange( data_queue_local_lock, 1 ))
					Relinquish();

				pdq = *ppdq;

				if (link)
				{
					tmp = pdq->Bottom - 1;
					if (tmp > 0x80000000)
						tmp += pdq->Cnt;
					if (tmp == pdq->Top) // collided with self...
					{
						// expand re-aligns queue elements so bottom is 0 and top is N
						// so the bottom will always wrap when we try to add to the beginning...
						pdq = ExpandDataQueueEx( ppdq, (*ppdq)->ExpandBy DBG_RELAY );
						tmp = pdq->Cnt - 1; // should be room at the end of phsyical array....
					}
					MemCpy( (POINTER)(pdq->data + (tmp * pdq->Size)), link, pdq->Size );
					pdq->Bottom = tmp;
				}
				data_queue_local_lock[0] = 0;
				return pdq;
			}

			//--------------------------------------------------------------------------

			LOGICAL  IsDataQueueEmpty( PDATAQUEUE* ppdq )
			{
				if (!ppdq || !(*ppdq) ||
					(*ppdq)->Bottom == (*ppdq)->Top)
					return TRUE;
				return FALSE;
			}

			//--------------------------------------------------------------------------

			LOGICAL  DequeData( PDATAQUEUE* ppdq, POINTER result )
			{
				LOGICAL p;
				INDEX tmp;
				if (ppdq && *ppdq)
					while (LockedExchange( data_queue_local_lock, 1 ))
						Relinquish();
				else
					return 0;

				p = 0;
				if ((*ppdq)->Bottom != (*ppdq)->Top)
				{
					tmp = (*ppdq)->Bottom + 1;
					if (tmp >= (*ppdq)->Cnt)
						tmp -= (*ppdq)->Cnt;
					if (result)
						MemCpy( result
							, (POINTER)((*ppdq)->data + (*ppdq)->Bottom * (*ppdq)->Size)
							, (*ppdq)->Size );
					p = 1;
					(*ppdq)->Bottom = tmp;
				}
				data_queue_local_lock[0] = 0;
				return p;
			}

			//--------------------------------------------------------------------------

			LOGICAL  UnqueData( PDATAQUEUE* ppdq, POINTER result )
			{
				LOGICAL p;
				INDEX tmp;
				if (ppdq && *ppdq)
					while (LockedExchange( data_queue_local_lock, 1 ))
						Relinquish();
				else
					return 0;

				p = 0;
				if ((*ppdq)->Bottom != (*ppdq)->Top)
				{
					tmp = (*ppdq)->Top;
					if (tmp)
						tmp--;
					else
						tmp = ((*ppdq)->Cnt) - 1;
					if (result)
						MemCpy( result
							, (POINTER)((*ppdq)->data + tmp * (*ppdq)->Size)
							, (*ppdq)->Size );
					p = 1;
					(*ppdq)->Top = tmp;
				}
				data_queue_local_lock[0] = 0;
				return p;
			}

			//--------------------------------------------------------------------------

			// zero is the first,
#undef PeekDataQueueEx
			LOGICAL  PeekDataQueueEx( PDATAQUEUE* ppdq, POINTER result, INDEX idx )
			{
				INDEX top;
				if (ppdq && *ppdq)
					while (LockedExchange( data_queue_local_lock, 1 ))
						Relinquish();
				else
					return 0;

				// cannot get invalid id.
				if (idx != INVALID_INDEX)
				{
					for (top = (*ppdq)->Bottom;
						idx != INVALID_INDEX && top != (*ppdq)->Top
						; )
					{
						idx--;
						if (idx != INVALID_INDEX)
						{
							top++;
							if ((top) >= (*ppdq)->Cnt)
								top = top - (*ppdq)->Cnt;
						}
					}
					if (idx == INVALID_INDEX)
					{
						MemCpy( result, (POINTER)((*ppdq)->data + top * (*ppdq)->Size), (*ppdq)->Size );
						data_queue_local_lock[0] = 0;
						return 1;
						//return (*ppdq)->pNode + top;
					}
				}
				data_queue_local_lock[0] = 0;
				return 0;
			}

#undef PeekDataQueue
			LOGICAL  PeekDataQueue( PDATAQUEUE* ppdq, POINTER result )
			{
				return PeekDataQueueEx( ppdq, result, 0 );
			}

			// zero is the first,
			POINTER  PeekDataInQueueEx( PDATAQUEUE* ppdq, INDEX idx )
			{
				INDEX top;
				if (ppdq && *ppdq)
					while (LockedExchange( data_queue_local_lock, 1 ))
						Relinquish();
				else
					return 0;

				// cannot get invalid id.
				if (idx != INVALID_INDEX)
				{
					for (top = (*ppdq)->Bottom;
						idx != INVALID_INDEX && top != (*ppdq)->Top
						; )
					{
						idx--;
						if (idx != INVALID_INDEX)
						{
							top++;
							if ((top) >= (*ppdq)->Cnt)
								top = top - (*ppdq)->Cnt;
						}
					}
					if (idx == INVALID_INDEX)
					{
						data_queue_local_lock[0] = 0;
						return (POINTER)((*ppdq)->data + top * (*ppdq)->Size);
					}
				}
				data_queue_local_lock[0] = 0;
				return NULL;
			}

			POINTER  PeekDataInQueue( PDATAQUEUE* ppdq )
			{
				return PeekDataInQueueEx( ppdq, 0 );
			}

			void  EmptyDataQueue( PDATAQUEUE* ppdq )
			{
				if (ppdq && *ppdq)
				{
					while (LockedExchange( data_queue_local_lock, 1 ))
						Relinquish();
					(*ppdq)->Bottom = (*ppdq)->Top = 0;
					data_queue_local_lock[0] = 0;
				}
			}


#ifdef __cplusplus
		};//		namespace data_queue {
#endif

#ifndef __STATIC_GLOBALS__
		PRIORITY_PRELOAD( InitLocals, NAMESPACE_PRELOAD_PRIORITY + 1 )
		{
#  ifdef __cplusplus
			RegisterAndCreateGlobal( (POINTER*)&list::_list_local, sizeof( *list::_list_local ), "_list_local" );
			//RegisterAndCreateGlobal( (POINTER*)&data_list::_data_list_local, sizeof( *data_list::_data_list_local ), "_data_list_local" );
			RegisterAndCreateGlobal( (POINTER*)&queue::_link_queue_local, sizeof( *queue::_link_queue_local ), "_link_queue_local" );
			RegisterAndCreateGlobal( (POINTER*)&data_queue::_data_queue_local, sizeof( *data_queue::_data_queue_local ), "_data_queue_local" );

#  else
			SimpleRegisterAndCreateGlobal( _list_local );
			//SimpleRegisterAndCreateGlobal( _data_list_local );
			SimpleRegisterAndCreateGlobal( _link_queue_local );
			SimpleRegisterAndCreateGlobal( _data_queue_local );
#  endif
		}
#endif

#ifdef __cplusplus
	} //namespace sack {
} //	namespace containers {
#endif

// restore this def in case of amalgamation
#define PeekDataQueueEx( q, type, result, idx ) PeekDataQueueEx( q, (POINTER)result, idx )


//--------------------------------------------------------------
// $Log: typecode.c,v $
// Revision 1.47  2005/05/25 16:50:30  d3x0r
// Synch with working repository.
//
// Revision 1.56  2005/05/20 23:15:13  jim
// Remove a noisy logging statement
//
// Revision 1.55  2005/05/16 23:18:22  jim
// Allocate the correct amount of space for the message queue - it's a MSGQUEUE not a DATAQUEUE.  Also implement DequeMessage() in such a way that the waited for message ID can change.
//
// Revision 1.54  2005/05/02 17:02:58  jim
// Moved the process-wait information to a seperate queue... does not work inline with normal messages...
//
// Revision 1.53  2005/04/20 23:38:20  jim
// Fixed leaving the critical section under a loop condition that resulted in error.
//
// Revision 1.52  2005/04/18 15:55:59  jim
// Much logging added to sack's implementation of SYSV msgq communications.
//
// Revision 1.51  2005/03/22 12:33:50  panther
// Restore disabled message queue logging
//
// Revision 1.50  2005/03/17 02:23:53  panther
// Checkpoint - working on message server abstraction interface... some of this seems to work quite well, some of this is still broken very badly...
//
// Revision 1.49  2005/03/14 16:04:03  panther
// If someone is waiting for any message, then they are definatly waiting for the currently enquing message.
//
// Revision 1.48  2005/01/27 07:18:34  panther
// Linux cleaned.
//
// Revision 1.47  2004/12/19 15:44:40  panther
// Extended set methods to interact with raw index numbers
//
// Revision 1.46  2004/10/25 10:40:00  d3x0r
// Linux compilation cleaning requirements...
//
// Revision 1.45  2004/10/02 19:49:57  d3x0r
// Fix logging... trying to track down multiple update display issues.... keys are queued, events are locally queued...
//
// Revision 1.44  2004/09/30 22:02:43  d3x0r
// checkpoing
//
// Revision 1.43  2004/09/30 09:42:52  d3x0r
// Fixed message queues for single app, all wraps, and nearly for two apps, but when removing logging, lost stability :(
//
// Revision 1.42  2004/09/30 01:14:48  d3x0r
// Cleaned up consistancy of PID and thread ID... extended message service a bit to supply event PID both ways.
//
// Revision 1.41  2004/09/29 16:43:03  d3x0r
// fixed queues a bit - added a test wait function for timers/threads
//
// Revision 1.40  2004/09/29 00:49:00  d3x0r
// Store waiting thread IDs IN the message queue... need to figure out how to shuffle these around.
//
// Revision 1.39  2004/09/24 08:09:49  d3x0r
// Test tial meeting the head...
//
// Revision 1.38  2004/09/23 11:07:33  d3x0r
// Minor adjustments...
//
// Revision 1.37  2004/09/23 00:36:55  d3x0r
// Fix result code when error no message and no wait... fix test for read messages and end of queue messages.
//
// Revision 1.36  2004/08/16 06:32:10  d3x0r
// Fix message queue routines... protect against no handle
//
// Revision 1.35  2004/07/13 04:17:49  d3x0r
// clean some warnings, and fix definiton of PRELOAD to be compiler friendly.
//
// Revision 1.34  2004/06/12 09:09:41  d3x0r
// ug - if queue is empty peek must be NULL...
//
// Revision 1.33  2004/05/24 16:40:29  d3x0r
// Add PeekQueue and GetQUeueLength
//
// Revision 1.32  2003/11/28 20:21:35  panther
// Add and fix EmptyList
//
// Revision 1.31  2003/10/31 02:24:53  panther
// Modified test to take variable msg count.
//
// Revision 1.30  2003/10/26 23:40:46  panther
// minor type fixes
//
// Revision 1.29  2003/10/26 23:32:17  panther
// Looks like most issues with simple message queuing are done.
//
// Revision 1.28  2003/10/22 10:45:40  panther
// Handle null lists in find
//
// Revision 1.27  2003/10/21 01:39:37  panther
// Fixed some issues with new perma-wait critical sections...
//
// Revision 1.26  2003/10/20 03:01:21  panther
// Fix getmythreadid - split depending if getpid returns ppid or pid.
// Fix memory allocator to init region correctly...
// fix initial status of found thred to reflect sleeping
// in /proc/#/status
//
// Revision 1.25  2003/10/20 00:04:21  panther
// Extend OpenSpace in SharedMem
// revise msgqueue operations to more resemble sysVipc msgq
//
// Revision 1.24  2003/10/18 23:41:04  panther
// Checkpoint... probably defuct
//
// Revision 1.23  2003/10/18 04:43:00  panther
// Quick patch...
//
// Revision 1.22  2003/10/17 00:56:05  panther
// Rework critical sections.
// Moved most of heart of these sections to timers.
// When waiting, sleep forever is used, waking only when
// released... This is preferred rather than continuous
// polling of section with a Relinquish.
// Things to test, event wakeup order uner linxu and windows.
// Even see if the wake ever happens.
// Wake should be able to occur across processes.
// Also begin implmeenting MessageQueue in containers....
// These work out of a common shared memory so multiple
// processes have access to this region.
//
// Revision 1.21  2003/08/20 08:07:13  panther
// some fixes to blot scaled... fixed to makefiles test projects... fixes to export containters lib funcs
//
// Revision 1.20  2003/07/25 10:21:57  panther
// Fix callback for foralllinks
//
// Revision 1.19  2003/05/12 01:31:52  panther
// Fix return
//
// Revision 1.18  2003/05/02 01:11:26  panther
// Many minor fixes, tweaks....
//
// Revision 1.17  2003/04/21 11:46:52  panther
// Ug - forgot a commit somewhere... return pointer at set data item
//
// Revision 1.16  2003/04/20 08:14:07  panther
// *** empty log message ***
//
// Revision 1.15  2003/04/12 20:52:46  panther
// Added new type contrainer - data list.
//
// Revision 1.14  2003/03/31 01:11:28  panther
// Tweaks to work better under service application
//
// Revision 1.13  2003/03/30 21:15:57  panther
// Added EX functions to pass application source to DataStack allocations
//
// Revision 1.12  2003/03/30 00:14:36  panther
// fix pop stack data function
//
// Revision 1.11  2003/01/28 16:37:48  panther
// More logging extended logging
//
// Revision 1.10  2003/01/27 09:20:34  panther
// Error in passing debug argument to create queue
//
// Revision 1.9  2003/01/22 17:10:09  panther
// Added forwarding in EnqueLink To CreateQueue
//
// Revision 1.8  2002/11/06 09:49:17  panther
// Fixed data-queue allocation/copy.
//
// Revision 1.7  2002/11/04 09:29:50  panther
// Added container class - DATAQUEUE.
//
//
//  - Added DataQueue to compliment LinkQueue  (datastack/linkstack)
//  - Added EmptyDataStack method to quickly remove all items on stack.
// Revision 1.6  2002/07/15 08:28:56  panther
// Fixed some debug passing to allocate.
//
//
