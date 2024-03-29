#define USE_RENDER3D_INTERFACE g.pr3i
#define USE_IMAGE_3D_INTERFACE g.pi3i
#define USE_IMAGE_INTERFACE g.pii


#include <stdhdrs.h>
#include <filesys.h>

#define OBJECT_SOURCE
#include <virtuality.h>
#include "global.h" 
//----------------------------------------------------------------------
// variables which ONLY object may reference.
//----------------------------------------------------------------------

extern INDEX tick;
extern uint64_t ticks[20];

// doubly linked list only head needed.
//POBJECT pFirstObject; // only one needed....

//-----------------------------------------------------------
void SetRootObject( POBJECT po )
{
	UnlinkThing( po ); // take it out of whatever it is in..
	po->pIn = NULL;
	po->pOn = NULL;
	LinkThing( g.pFirstRootObject, po );}

void AddRootObject( POBJECT po )
{
	LinkThing( g.pFirstRootObject, po );}

//-----------------------------------------------------------

#ifdef USE_DATA_STORE
static INDEX iObject;
static INDEX iCluster;
static INDEX iClusterObjects, iClusterLines, iClusterLineSegs, iClusterFacets;
static INDEX iObjectT;
static INDEX iObjectTi;
static INDEX iObjectInfo;
static INDEX iObjectInfoCluster;
#endif
POBJECT CreateObject( void )
{
   static PCLUSTER cluster;
	POBJECT po;

	if( !cluster )
	{
#ifdef USE_DATA_STORE
		iCluster         = DataStore_RegisterNamedDataType( "Object Cluster", sizeof( CLUSTER ) );
		iClusterObjects  = DataStore_CreateDataSet( iCluster, OBJECT, objects );
		iClusterFacets   = DataStore_CreateDataSet( iCluster, FACET, FacetPool );
		iClusterLines    = DataStore_CreateDataSet( iCluster, LINE, LinePool );
		iClusterLineSegs = DataStore_CreateDataSet( iCluster, LINESEGP, LineSegPPool );

		iObject    = DataStore_RegisterNamedDataType( "Object (Shell)", sizeof( OBJECT ) );
		iObjectT   = DataStore_CreateLink( iObject, offsetof( OBJECT, T ), iTransform );
		iObjectTi  = DataStore_CreateLink( iObject, offsetof( OBJECT, Ti ), iTransform );

		iObjectIn     = DataStore_CreateLink( iObject, offsetof( OBJECT, pIn ), iObject );
		iObjectHolds  = DataStore_CreateLink( iObject, offsetof( OBJECT, pHolds ), iObject );
		iObjectOn     = DataStore_CreateLink( iObject, offsetof( OBJECT, pOn ), iObject );
		iObjectHas    = DataStore_CreateLink( iObject, offsetof( OBJECT, pHas ), iObject );

		iObjectInfo        = DataStore_RegisterNamedDataType( "Object Shape Info", sizeof( OBJECT_INFO ) );
		iObjectInfoCluster = DataStore_CreateLink( iObjectInfo, offsetof( OBJECT_INFO, cluster ), iCluster );

		cluster = DataStore_CreateDataType( iCluster ); // result will be 0 initialized.
#else
		cluster = (PCLUSTER)Allocate( sizeof( CLUSTER ) );
		MemSet( cluster, 0, sizeof( CLUSTER ) );
#endif
	}

#ifdef LOG_ALLOC
   printf("Allocate(OBJECT)\n");
#endif

#ifdef USE_DATA_STORE
	po = CreateDataType( iObject );
#else
	po = (POBJECT)Allocate( sizeof( OBJECT ) );
	memset( po, 0, sizeof( OBJECT ) );
#endif
	MarkTick( ticks[tick++] );
	po->hud_icon = MakeImageFile( 8, 8 );
	SetImageTransformRelation( po->hud_icon, IMAGE_TRANSFORM_RELATIVE_CENTER, NULL );
	ClearImageTo( po->hud_icon, BASE_COLOR_RED );

#ifdef USE_DATA_STORE
	DataStore_SetLink( iObject, po, iObjectT, iTransform, CreateDataType( iTransform ) );
#else
	po->T = CreateTransform();
#endif


#ifdef USE_DATA_STORE
	DataStore_SetLink( iObject, po, iObjectTi, iTransform, CreateDataType( iTransform ) );
#else
	po->Ti = CreateTransform();
#endif

	MarkTick( ticks[tick++] );

#ifdef USE_DATA_STORE
	DataStore_GetFromDataSet( iCluster, cluster, iClusterObjects );
	// pplinepool, pplinesegpool, and ppfacetpool are
	// not needed because we have cluster in objectinfo
   DataStore_SetLink( iObjectInfo, po->objinfo, iObjectInfoCluster, iCluster, cluster );
	po->objinfo->cluster = cluster;
#else
   po->objinfo = GetFromSet( OBJECTINFO, &cluster->objects );
   po->objinfo->ppLinePool = &cluster->LinePool;
	po->objinfo->ppLineSegPPool = &cluster->LineSegPPool;
	po->objinfo->ppFacetPool = &cluster->FacetPool;
   po->objinfo->facets = NULL;
	po->objinfo->cluster = cluster;
#endif
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
	UnlinkThing( out );
	//if( GrabThing(out) )
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
	//DebugBreak();
	UnlinkThing( _this );
//	if( GrabThing( _this ) )
	{
		_this->pIn = in;
		LinkThing( in->pHolds, _this );
	}
	IntersectObjectPlanes( _this );
	return _this;
}

//-----------------------------------------------------------

POBJECT PutOn( POBJECT _this, POBJECT on ) 
{
	if( _this == on ) return NULL;
	UnlinkThing( _this );
	//if( GrabThing( _this ) )
	{
		_this->pOn = on;
      LinkThing( on->pHas, _this );
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
//#pragma message ("sorry the following two lines lost flexability" )
		AddPlaneToSet( po->objinfo, o, n, 1 );
	}

	MarkTick( ticks[tick++] );
	// assuming t is clear - or has a rotation factor...
	// we don't need this here...
	// the body ends up rotating the body on creation...
	TranslateV( po->Ti, pv );
	//TranslateV( po->T, pv );
//   RotateTo( po->T, pforward, pright );

	IntersectPlanes( po->objinfo, po->pIn?po->pIn->objinfo:NULL, TRUE ); // no transformation nicluded....
	MarkTick( ticks[tick++] );

   //  137619656380044, 1101158|train@object.c(179):3000 4422 17653 543025 608 465565 47066 3833 61 -137619656379692 0 0
   //  137614708677114, 148679|train@object.c(179):1973 3375 444 8325 537 68497 46228 3920 39 -137614708676916 0 0


	lprintf( "%7" _64fs " %7" _64fs " %7" _64fs " %7" _64fs " %7" _64fs " %7" _64fs " %7" _64fs " %7" _64fs " %7" _64fs " %7" _64fs " %7" _64fs " %7" _64fs
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

struct copyseg_args
{
	PDATALIST pdl_lines; // data of line_copied
	RCOORD scale;
	POBJECT old_object;
	POBJECT new_object;
	PFACET old_facet;
	PFACET new_facet;
};

struct line_copied{
	PMYLINESEG oldseg;
	PMYLINESEG newseg;
};

uintptr_t CPROC copyseg( POINTER p, uintptr_t psv )
{
	struct copyseg_args *args = (struct copyseg_args *)psv;
	struct line_copied *mapped_line;
	INDEX idx;
	PLINESEGP lineseg = (PLINESEGP)p;
	PLINESEGP newseg = GetFromSet( LINESEGP, &args->new_facet->pLineSet );
	newseg[0] = lineseg[0];
	DATA_FORALL( args->pdl_lines, idx, struct line_copied *, mapped_line )
	{
		if( mapped_line->oldseg == lineseg[0].pLine )
		{
			newseg[0].pLine = mapped_line->newseg;
			break;
		}
	}
	return 0;
}

uintptr_t CPROC copymyseg( POINTER p, uintptr_t psv )
{
	struct copyseg_args *args = (struct copyseg_args *)psv;
	PMYLINESEG lineseg = (PMYLINESEG)p;
	PMYLINESEG newseg = GetFromSet( MYLINESEG, args->new_object->objinfo->ppLinePool );

	newseg[0] = lineseg[0];
	newseg->l.dFrom = lineseg->l.dFrom * args->scale;
	newseg->l.dTo = lineseg->l.dTo * args->scale;
	scale( newseg->l.r.o, lineseg->l.r.o, args->scale );
	return 0;
}

uintptr_t CPROC copyfacet( POINTER p, uintptr_t psv )
{
	struct copyseg_args *args = (struct copyseg_args *)psv;
	PLINESEGPSET segments;
	args->old_facet = (PFACET)p;
	args->new_facet = GetFromSet( FACET, args->new_object->objinfo->ppFacetPool );

	segments = args->old_facet[0].pLineSet;
	args->old_facet[0].pLineSet = NULL;
	args->new_facet[0] = args->old_facet[0];
	args->old_facet[0].pLineSet = segments;

	AddLink( &args->new_object->objinfo->facets, args->new_facet );
	ForAllInSet( LINESEGP, args->old_facet->pLineSet, copyseg, psv );
	return 0;
}


POBJECT MakeScaledInstance( POBJECT pDefs, RCOORD fSize, 
                              PCVECTOR pv, 
                              PCVECTOR pforward,
                              PCVECTOR pright,
                              PCVECTOR pup )
{
	POBJECT po = CreateObject( );  // create origin basis
	INDEX idx;
	PFACET facet;
	PMYLINESEG line;
	struct line_copied line_copy;
	struct copyseg_args args;
	//TRANSFORM T_tmp;
	PLIST list = pDefs->objinfo->facets;
	args.pdl_lines = CreateDataList( sizeof( struct line_copied ) );
	
	TranslateV( po->Ti, pv );
	args.scale = fSize;
	args.new_object = po;
	args.old_object = pDefs;
	LIST_FORALL( pDefs->objinfo->lines, idx, PMYLINESEG, line )
	{
		line_copy.oldseg = line;
		line_copy.newseg = GetFromSet( MYLINESEG, pDefs->objinfo->ppLinePool );
		line_copy.newseg[0] = line_copy.oldseg[0];

		line_copy.newseg->l.dFrom = line_copy.oldseg->l.dFrom * args.scale;
		line_copy.newseg->l.dTo = line_copy.oldseg->l.dTo * args.scale;
		scale( line_copy.newseg->l.r.o, line_copy.oldseg->l.r.o, args.scale );
		/*
		{

			int n;
			for( n = 0; n < 3; n ++ )
			{
				INDEX facet = GetMemberIndex( FACET, args.old_object->objinfo->ppFacetPool, line_copy.newseg[0].facets[n] );
				if( facet != INVALID_INDEX )
					line_copy.newseg[0].facets[0] = GetSetMember( FACET, args.new_object->objinfo->ppFacetPool, facet );
			}

		}
		*/
		AddDataItem( &args.pdl_lines, &line_copy );
	}
	//fforward right and up are unused
	LIST_FORALL( pDefs->objinfo->facets, idx, PFACET, facet )
	{
		copyfacet( facet, (uintptr_t)&args );
	}
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

uint32_t SaveObject( CTEXTSTR filename, POBJECT po )
{
	int p;
   FILE *file;

   file = sack_fopen( 0, filename, "wb" );
	if( file )
	{
      PFACET pf;
      uint32_t tmp = 0;
		fwrite( &po->T, sizeof( po->T ), 1, file );
      tmp = 0;
		LIST_FORALL( po->objinfo->facets, p, PFACET, pf )
         tmp++;
		fwrite( &tmp, sizeof( tmp ), 1, file );
      LIST_FORALL( po->objinfo->facets, p, PFACET, pf )
		{
			fwrite( &pf->d, sizeof( RAY ), 1, file );
			fwrite( &pf->color, sizeof( CDATA ), 1, file );
		}
      fclose( file );
	}
   // shouldn't have to save lines cause we can re-intersect - RIGHT?
	return 0;
}

//-----------------------------------------------------------

POBJECT LoadObject( CTEXTSTR filename )
{
	uint32_t dwUsed;
	FILE *file;
//   PLANE p;
	VECTOR vn, vo;
	PFACET pp;

	uint32_t c;
	POBJECT po;
	file = sack_fopen( 0, filename, "rb" );
	if( file )
	{
		po = CreateObject();
		sack_fread( &po->T, sizeof( po->T ), 1, file );
		sack_fread( &dwUsed, sizeof( dwUsed ), 1, file );
		for( c = 0; c < dwUsed; c++ )
		{
			sack_fread( vn, sizeof( VECTOR ), 1, file );
			sack_fread( vo, sizeof( VECTOR ), 1,file );
			pp = AddPlaneToSet( po->objinfo, vo, vn, 0 );
			sack_fread( &pp->color, sizeof( CDATA ), 1, file );
		}
		IntersectPlanes( po->objinfo, NULL, TRUE );
		sack_fclose( file );
		return po;
	}
   return NULL;
}

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


int IntersectObjectPlanes( POBJECT object )
{
	int result = IntersectPlanes( object->objinfo, object->pIn?object->pIn->objinfo:NULL, TRUE ); // no transformation nicluded....

	return result;
}


PMYLINESEG CreateFacetLine( POBJECT pobj,
                  	PCVECTOR po, PCVECTOR pn,
                  	RCOORD rFrom, RCOORD rTo )
{
	if( pobj )
		return CreateLine( pobj->objinfo, po, pn, rFrom, rTo );
	return NULL;
}

PMYLINESEG CreateNormaledFacetLine( POBJECT pobj,
                  	PCVECTOR po, PCVECTOR pn,
                  	RCOORD rFrom, RCOORD rTo
					, PCVECTOR normal_at_from, PCVECTOR normal_at_to )
{
	if( pobj )
	{
		PMYLINESEG plms = CreateLine( pobj->objinfo, po, pn, rFrom, rTo );
		SetPoint( plms->normals.at_from, normal_at_from );
		SetPoint( plms->normals.at_to, normal_at_to );
		return plms;
	}
	return NULL;
}


