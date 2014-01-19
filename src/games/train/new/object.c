#include <stdhdrs.h>  // PSTR blah...
#include <stdio.h>
#include <sharemem.h>
#include <string.h>

#define OBJECT_SOURCE
#include "object.h" // includes vector.h.....
#include "vectlib.h"

//----------------------------------------------------------------------
// variables which ONLY object may reference.
//----------------------------------------------------------------------

extern INDEX tick;
extern _64 ticks[20];

// doubly linked list only head needed.
//POBJECT pFirstObject; // only one needed....

//-----------------------------------------------------------
void SetRootObject( POBJECT po )
{
	LinkThingAfter( (POBJECT)&FirstObject, po );}

void AddRootObject( POBJECT po )
{
	LinkThingAfter( (POBJECT)&FirstObject, po );}

//-----------------------------------------------------------

POBJECT CreateObject( void )
{
   static PCLUSTER cluster;
	POBJECT po;

	if( !cluster )
	{
		cluster = Allocate( sizeof( CLUSTER ) );
      MemSet( cluster, 0, sizeof( CLUSTER ) );
	}

#ifdef LOG_ALLOC
   printf("Allocate(OBJECT)\n");
#endif
   po = (POBJECT)Allocate( sizeof( OBJECT ) );
	memset( po, 0, sizeof( OBJECT ) );
	MarkTick( ticks[tick++] );

   po->T = CreateTransform();

	po->Ti = CreateTransform();
	MarkTick( ticks[tick++] );

   po->objinfo = GetFromSet( OBJECTINFO, &cluster->objects );
   po->objinfo->ppLinePool = &cluster->LinePool;
	po->objinfo->ppLineSegPPool = &cluster->LineSegPPool;
	po->objinfo->ppFacetPool = &cluster->FacetPool;
   po->objinfo->facets = NULL;
	po->objinfo->cluster = cluster;

   //InitSet( &po->objinfo.LinePool, Lines );
   //InitSet( &po->objinfo->FacetPool, Facets );
   //InitSet( &po->objinfo->FacetSetPool, FacetSets );
   po->pIn    = NULL;
   po->pHolds = NULL;
   po->pOn    = NULL;
	po->pHas   = NULL;
   AddRootObject( po );

   return po;
}

//-----------------------------------------------------------

void FreeObject( POBJECT *po )
{
   POBJECT _po;
	_po = *po;

	UnlinkThing( _po );
	// need to release the members this one is using
   // but, do not need uhmm... to delete the set.
   //DeleteSet( _po->objinfo->ppLinePool );
   {
   	//int p;
   	//for( p = 0; p < _po->objinfo->FacetSetPool.nUsedFacetSets; p++ )
   	//	EmptySet( _po->objinfo->FacetSetPool.pFacetSets+p, Facets );
	}
   DeleteList( &_po->objinfo->facets );
   //EmptySet( &_po->objinfo->FacetPool, Facets );
   //EmptySet( &_po->objinfo->FacetSetPool, FacetSets );
#ifdef LOG_ALLOC
   printf("Release(OBJECT)\n");
#endif
   Release( _po );

   *po = 0;
}

//-----------------------------------------------------------

POBJECT PullOut( POBJECT out )
{
	if( GrabThing(out) )
	{
		out->pIn = NULL;
      out->pOn = NULL;
	}
   return out;
}

//-----------------------------------------------------------

POBJECT PutIn( POBJECT _this, POBJECT in )
{
	if( _this == in ) return NULL;
	if( GrabThing( _this ) )
	{
		_this->pIn = in;
		LinkThingAfter( in->pHolds, _this );
	}
   return _this;
}

//-----------------------------------------------------------

POBJECT PutOn( POBJECT _this, POBJECT on ) 
{
	if( _this == on ) return NULL;
	if( GrabThing( _this ) )
	{
		_this->pOn = on;
      LinkThingAfter( on->pHas, _this );
	}
   return _this;
}

//-----------------------------------------------------------

POBJECT CreateScaledInstance( BASIC_PLANE *pDefs, int nDefs, RCOORD fSize, 
                              PCVECTOR pv, 
                              PCVECTOR pforward, 
                              PCVECTOR pright,
                              PCVECTOR pup )
{
   POBJECT po;
   VECTOR o;
	VECTOR n;
	int i;
   tick = 0;
   MarkTick( ticks[tick++] );
   MarkTick( ticks[tick++] );
   MarkTick( ticks[tick++] );
   //printf(" Creating Scaled Instance...\n");
	po = CreateObject( );  // create origin basis
   MarkTick( ticks[tick++] );
   //nfs = GetFacetSet( po->objinfo );
   for( i = 0; i < nDefs; i++ )
   {
      scale( n, pDefs[i].n, fSize );
		scale( o, pDefs[i].o, fSize ); // must scale this to move them out....
      //add( o, o, pv );
		add( o, o, n );
      SetPoint( n, pDefs[i].n ); // don't have to scale this...
#pragma message ("sorry the following two lines lost flexability" )
      AddPlaneToSet( po->objinfo, o, n, 1 );
   }

   MarkTick( ticks[tick++] );
   // assuming t is clear - or has a rotation factor...
   // we don't need this here...
   // the body ends up rotating the body on creation...
   TranslateV( po->Ti, pv );
   //TranslateV( po->T, pv );
//   RotateTo( po->T, pforward, pright );

	IntersectPlanes( po->objinfo, TRUE ); // no transformation nicluded....
	MarkTick( ticks[tick++] );

   //  137619656380044, 1101158|train@object.c(179):3000 4422 17653 543025 608 465565 47066 3833 61 -137619656379692 0 0
   //  137614708677114, 148679|train@object.c(179):1973 3375 444 8325 537 68497 46228 3920 39 -137614708676916 0 0


	lprintf( "%7Ld %7Ld %7Ld %7Ld %7Ld %7Ld %7Ld %7Ld %7Ld %7Ld %7Ld %7Ld"
			 , ticks[1]-ticks[0]
			 , ticks[2]-ticks[1]
			 , ticks[3]-ticks[2]
			 , ticks[4]-ticks[3]
			 , ticks[5]-ticks[4]
			 , ticks[6]-ticks[5]
			 , ticks[7]-ticks[6]
			 , ticks[8]-ticks[7]
			 , ticks[9]-ticks[8]
			 , ticks[10]-ticks[9]
			 , ticks[11]-ticks[10]
			 , ticks[12]-ticks[11]
			 );
   //printf(" object has been created\n");
   return po;
}

//-----------------------------------------------------------

void SetObjectColor( POBJECT po, CDATA c ) // dumb routine!
{
	po->color = c;
}

//-----------------------------------------------------------

void InvertObject( POBJECT po )
{
   po->bInvert = !po->bInvert;
}

//-----------------------------------------------------------
/*
DWORD SaveObject( HANDLE hFile, POBJECT po )
{
   DWORD dwWritten;
   DWORD dwWrite;
   PFACETSET pps;
   int p;
   dwWritten = 0;

   WriteFile( hFile, &po->T, sizeof( po->T ), &dwWrite, NULL );

   dwWritten += dwWrite;

   pps = po->pPlaneSet;
   
   WriteFile( hFile, &pps->nUsedPlanes, sizeof( pps->nUsedPlanes ), &dwWrite, NULL );
   dwWritten += dwWrite;
   for( p = 0; p < pps->nUsedPlanes; p++ )
   {
      WriteFile( hFile, &pps->pPlanes[p].d, sizeof( RAY ), &dwWrite, NULL );
      dwWritten += dwWrite;
      WriteFile( hFile, &pps->pPlanes[p].color, sizeof( CDATA ), &dwWrite, NULL );
      dwWritten += dwWrite;
   }

   // shouldn't have to save lines cause we can re-intersect - RIGHT?
   return dwWritten;
}
*/
//-----------------------------------------------------------
/*
POBJECT LoadObject( HANDLE hFile )
{
   DWORD dwUsed;
//   PLANE p;
   VECTOR vn, vo;
   PFACET pp;

   DWORD c;
   DWORD dwRead;
   POBJECT po;
   po = CreateObject();
   ReadFile( hFile, &po->T, sizeof( po->T ), &dwRead, NULL );
   ReadFile( hFile, &dwUsed, sizeof( dwUsed ), &dwRead, NULL );
   for( c = 0; c < dwUsed; c++ )
   {
      ReadFile( hFile, vn, sizeof( VECTOR ), &dwRead, NULL );
      ReadFile( hFile, vo, sizeof( VECTOR ), &dwRead, NULL );
      pp = AddPlaneToSet( po->pPlaneSet, vo, vn, 0 );
      ReadFile( hFile, &pp->color, sizeof( CDATA ), &dwRead, NULL );
   }
   IntersectPlanes( &po->LinePool, &po->pPlaneSet, TRUE );
   return po;
}
*/
//---------------------------------------------------------

// vo is object's origin
// plane's origin relative to the object is 0, 0, 0
// and the normal is the view projection normal
// the lines created for the square are offset by VectorConst_X, VectorConst_Y vectors
// and... if the vn is parallel to the plane itself
// i'd imagine that texture mappers will fail.
POBJECT CreatePlane( PCVECTOR vo, PCVECTOR vn, PCVECTOR vr, RCOORD size, CDATA c ) // not a real plane....
{
   POBJECT po;
   PFACET pf;
   PMYLINESEG pl;
   VECTOR o, n;

   po = CreateObject();
   po->color = c;
   //nfs = GetFacetSet( po->objinfo );

   TranslateV( po->T, vo );
   RotateTo( po->T, vn, vr ); // using point as destination, possibly unscaled normal...

   pf = AddPlaneToSet( po->objinfo, VectorConst_0, VectorConst_Z, 0 );

   // but if when we texture this we can use the normal...
   // and for now the normal points towards a normal viewer...

   scale( n, VectorConst_Y, size/2 );
   scale( o, VectorConst_X, size/2 );
   pl = CreateLine( po->objinfo,
                   o, n, -1.0f, 1.0f );
   AddLineToPlane( po->objinfo, pf, pl );

   Invert( o );
   pl = CreateLine( po->objinfo,
               o, n, -1.0f, 1.0f );
   AddLineToPlane( po->objinfo, pf, pl );

   scale( n, VectorConst_X, size/2 );
   scale( o, VectorConst_Y, size/2 );
   pl = CreateLine( po->objinfo,
               o, n, -1.0f, 1.0f );
   AddLineToPlane( po->objinfo, pf, pl );

   Invert( o );
   pl = CreateLine( po->objinfo,
               o, n, -1.0f, 1.0f );
   AddLineToPlane( po->objinfo, pf, pl );

   return po;
}


//-----------------------------------------------------------

LOGICAL ObjectOn( POBJECT po, 
                  PCVECTOR vforward,
                   PCVECTOR vright,
                   PCVECTOR vo )
   {
      int bRet = FALSE;

      if( !po ) 
         return FALSE;
         /*
		for( nfs = 0; nfs < po->FacetSetPool.nUsedFacetSets && !bRet; nfs++ )
		{
   	   for( nf = 0; nf < pfps->nUsedFacets; nf++ )
      	{
      		pf = po->FacetSet.pFacets + pfps->pFacets[nf].nFacet;
	         ApplyInverse( po->T, o, vo ); // move mouse origin to body relative.....
//            Apply( po->T, &rp, &pp[nf].d );
   	      SetRay( &rp, &pf->d );
      	   if( IntersectLineWithPlane( vforward, o,
         	                            rp.n, rp.o, &time ) )
	         {
   	         // time * slope + origin is point in plane...
      	      scale( n, vforward, time );
         	   add( o, o, n );
      	      	
   	         if( PointWithin( o, &po->LinePool, &pf->pLineSet ) )
	            {
            	    bRet = TRUE; // highlight all planes...
         	   }
      	      break; // first plane only....
   	      }
	         else
         	{
      	   #ifdef MSVC
   	         _asm nop;
	         #endif
         	}
         
         }
      }
      */
      return bRet;
   }

