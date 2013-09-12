#include <windows.h>  // PSTR blah...
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "object.hpp" // includes vector.h.....
#include <vectlib.h>

//----------------------------------------------------------------------
// variables which ONLY object may reference.
//----------------------------------------------------------------------

// doubly linked list only head needed.
POBJECT pFirstObject; // only one needed....

//-----------------------------------------------------------

POBJECT CreateObject( void )
{
   POBJECT po;
#ifdef LOG_ALLOC
   printf("malloc(OBJECT)\n");
#endif
   po = (POBJECT)malloc( sizeof( OBJECT ) );
   memset( po, 0, sizeof( OBJECT ) );

   po->T.clear();

   po->pLinePool = CreateLineSet();
   po->pPlaneSet = CreatePlaneSet();

   if( pFirstObject )
   {
      po->pPrior = pFirstObject->pPrior;
      po->pPrior->pNext = po;  
      po->pNext = pFirstObject;
      pFirstObject->pPrior = po;
   }
   else
   {
      po->pPrior = po;
      po->pNext = po;
   }
   po->pIn    = NULL;
   po->pHolds = NULL;
   po->pOn    = NULL;
   po->pHas   = NULL;

   pFirstObject = po;

   return po;
}

//-----------------------------------------------------------

void FreeObject( POBJECT *po )
{
   POBJECT _po;
   _po = *po;

   if( _po->pNext )
      _po->pNext->pPrior = _po->pPrior;

   if( _po->pPrior )
      _po->pPrior->pNext = _po->pNext;
   else
      pFirstObject = _po->pNext;

   DestroyPlaneSet( _po->pPlaneSet );

#ifdef LOG_ALLOC
   printf("free(OBJECT)\n");
#endif
   free( _po );

   *po = 0;
}

//-----------------------------------------------------------

POBJECT PullOut( POBJECT _this, POBJECT out )
{
   // take 'out' out of 'this'
   POBJECT po;
   if( !_this )
   {
      out->pPrior->pNext = out->pNext;
      out->pNext->pPrior = out->pPrior;
      out->pNext = out;
      out->pPrior = out;
      return out; // is not IN this...
   }
   po = _this->pHolds; // generic solution...
   if( po )
      do 
      {
         if( po == out )
         {
            po->pPrior->pNext = po->pNext;
            po->pNext->pPrior = po->pPrior;
            po->pNext = po;
            po->pPrior = po;
            po->pIn = NULL;
            return po;
         }
         po = po->pNext;
      } while( po != _this->pHolds );
   return NULL;
}

//-----------------------------------------------------------

POBJECT PullOff( POBJECT _this, POBJECT out )
{
   // take 'out' out of 'this'
   POBJECT po;
   if( !_this )
   {
      out->pPrior->pNext = out->pNext;
      out->pNext->pPrior = out->pPrior;
      out->pNext = out;
      out->pPrior = out;
      return out; // is not ON this...
   }
   FORALL( _this->pHas, &po )
   {
      if( po == out )
      {
         po->pPrior->pNext = po->pNext;
         po->pNext->pPrior = po->pPrior;
         po->pNext = po;
         po->pPrior = po;
         po->pOn = NULL;
         return po;
      }
   }
   return NULL;
}

//-----------------------------------------------------------

POBJECT TakeOut( POBJECT out )
{
   // take 'out' out of 'this'
   if( out )
      return PullOut( out->pIn, out );
   return NULL;
}

//-----------------------------------------------------------

POBJECT TakeOff( POBJECT out )
{
   // take 'out' out of 'this'
   if( out )
      return PullOff( out->pOn, out );
   return NULL;
}

//-----------------------------------------------------------

POBJECT PutIn( POBJECT _this, POBJECT in )
{
   if( _this == in ) return NULL;  
   TakeOut( _this );
   if( _this->pIn ) // cannot happen... taken out of what it was in...
   {
      in->pNext = _this->pIn;
      in->pPrior = _this->pIn->pPrior;
      _this->pIn->pPrior = in;
      _this->pIn = in;
   }
   else
   {
      _this->pIn = in;
      if( in->pHolds )
      {
         _this->pNext = in->pHolds;
         _this->pPrior = in->pHolds->pPrior;
         in->pHolds->pPrior->pNext = _this;
         in->pHolds->pPrior = _this;
      }
      in->pHolds = _this;
   }
   return NULL;
}

//-----------------------------------------------------------

POBJECT PutOn( POBJECT _this, POBJECT on ) 
{
   if( _this == on ) return NULL;  
   TakeOut( _this );
   if( _this->pOn ) // cannot happen... taken out of what it was in...
   {
      on->pNext = _this->pOn;
      on->pPrior = _this->pOn->pPrior;
      _this->pOn->pPrior = on;
      _this->pOn = on;
   }
   else
   {
      _this->pOn = on;
      if( on->pHas )
      {
         _this->pNext = on->pHas;
         _this->pPrior = on->pHas->pPrior;
         on->pHas->pPrior->pNext = _this;
         on->pHas->pPrior = _this;
      }
      on->pHas = _this;
   }
   return NULL;
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
   printf(" Creating Scaled Instance...\n");
   po = CreateObject( );  // create origin basis
   for( i = 0; i < nDefs; i++ )
   {
      PFACET pp;
      scale( n, pDefs[i].n, fSize );
      scale( o, pDefs[i].o, fSize ); // must scale this to move them out....
      add( o, o, n );
      SetPoint( n, pDefs[i].n ); // don't have to scale this...
#pragma message ("sorry the following two lines lost flexability" )
      pp = AddPlane( po->pPlaneSet, o, n, 1 );
      pp->color = 0x7f7f7f;
   }

   // assuming t is clear - or has a rotation factor...
   // we don't need this here...
   // the body ends up rotating the body on creation...
//   po->T.Translate( pv );
//   po->T.RotateTo( pforward, pright );

   IntersectPlanes( po->pLinePool, po->pPlaneSet, TRUE ); // no transformation nicluded.... 
   printf(" object has been created\n");
   return po;
}

//-----------------------------------------------------------

void SetObjectColor( POBJECT po, CDATA c ) // dumb routine!
{
   PFACETSET pps;
   PFACET pp;
   int p;
   pps = po->pPlaneSet;
   for( p = 0; p < pps->nUsedPlanes; p++ )
   {
      pp = pps->pPlanes + p;
      pp->color = c;
   }
}

//-----------------------------------------------------------

void InvertObject( POBJECT po )
{
   PFACETSET pps;
   PFACET pp;
   int p;
   pps = po->pPlaneSet;
   pp = pps->pPlanes;
   for( p = 0; p < pps->nUsedPlanes; p++ )
   {
      Invert( pp[p].d.n );
   }
   IntersectPlanes( po->pLinePool, 
                    po->pPlaneSet, TRUE );  // recompute intersections...
}

//-----------------------------------------------------------

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

//-----------------------------------------------------------

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
      pp = AddPlane( po->pPlaneSet, vo, vn, 0 );
      ReadFile( hFile, &pp->color, sizeof( CDATA ), &dwRead, NULL );
   }
   IntersectPlanes( po->pLinePool, po->pPlaneSet, TRUE );
   return po;
}

//-----------------------------------------------------------

inline PFACET AddPlane( POBJECT pobj, PCVECTOR po, PCVECTOR pn, int d )
{
   return AddPlane( pobj->pPlaneSet,po, pn, d );
}

//-----------------------------------------------------------

// vo is object's origin
// plane's origin relative to the object is 0, 0, 0
// and the normal is the view projection normal
// the lines created for the square are offset by VectorConst_X, VectorConst_Y vectors
// and... if the vn is parallel to the plane itself
// i'd imagine that texture mappers will fail.
POBJECT CreatePlane( PCVECTOR vo, PCVECTOR vn, PCVECTOR vr, RCOORD size, CDATA c ) // not a real plane....
{
   POBJECT po;
   PFACET pp;
   VECTOR o, n;
   PLINESEG pl;
   po = CreateObject();
   po->T.Translate( vo );
   po->T.RotateTo( vn, vr ); // using point as destination, possibly unscaled normal...

   pp = AddPlane( po, VectorConst_0, VectorConst_Z, 0 ); // don't really NEED a plane...
   // but if when we texture this we can use the normal...
   // and for now the normal points towards a normal viewer...

   scale( n, VectorConst_Y, size/2 );
   scale( o, VectorConst_X, size/2 );
   pl = CreateLine( po->pLinePool, 
                   o, n, -1.0f, 1.0f );
   AddLineToPlane( pl, pp );
   Invert( o );
   pl = CreateLine( po->pLinePool, 
               o, n, -1.0f, 1.0f );
   AddLineToPlane( pl, pp );

   scale( n, VectorConst_X, size/2 );
   scale( o, VectorConst_Y, size/2 );
   pl = CreateLine( po->pLinePool, 
               o, n, -1.0f, 1.0f );
   AddLineToPlane( pl, pp );
   Invert( o );
   pl = CreateLine( po->pLinePool, 
               o, n, -1.0f, 1.0f );
   AddLineToPlane( pl, pp );

   pp->color = c;
   return po;
}


//-----------------------------------------------------------

   bool ObjectOn( POBJECT po, 
                  PCVECTOR vforward,
                   PCVECTOR vright,
                   PCVECTOR vo )
   {
      PFACETSET pps;
      PFACET pp;
      VECTOR o, n;
      int p;


      bool bRet = false;
      RCOORD time;
      RAY rp;

      if( !po ) 
         return false;

      pps = po->pPlaneSet;
      pp = pps->pPlanes;
      for( p = 0; p < pps->nUsedPlanes; p++ )
      {
         pp[p].color = Color( 255, 255, 255 ); // set white :) 
         po->T.ApplyInverse( o, vo ); // move mouse origin to body relative.....
//         po->T.Apply( &rp, &pp[p].d );
         SetRay( &rp, &pp[p].d );
         if( IntersectLineWithPlane( vforward, o,
                                     rp.n, rp.o, &time ) )
         {
            // time * slope + origin is point in plane...
            scale( n, vforward, time );
            add( o, o, n );
            
            if( PointWithin( o, pp[p].pLineSet ) )
            {
                pp[p].color = RGB( 0, 255, 255 );
                bRet = true; // highlight all planes...
            }
            break; // first plane only....
         }
         else
         {
            _asm nop;
         }
      }
      return bRet;
   }

