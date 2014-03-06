//#include <afxwin.h>
#include <windows.h>
#include <stdio.h>

#include <vectlib.h>

extern "C" {
#include "vidlib.h"
};

#include "brain.hpp"
#include "bboard.hpp" // includes vidlib.h... 

#include "body.hpp" // my own structures...

#define BOUND_COLOR RGB(255,255,255)
#define RADAR_COLOR RGB(0,0,255)  // this IS red...

extern POBJECT pFlatWorld;  // this should be uhmm... 
                            // like variable surfaces...
extern POBJECT pWorld; // environment.cpp

//-------------------------------------------

 // display characteristics - cannot be 
// defined quite the way I was gonna do it
// but we should only have one world per program
// multiple boards though - and I'm not entirely
// sure whether they are there or not....duh ?
// how to wrap these correctly...

extern HVIDEO hVideo;
extern int nScale;
#define SCALE nScale

#define UNSCALE_X(x)  ( ( ((x)*4) / SCALE )) 
#define UNSCALE_Y(y)  ( ( ((y)*4) / SCALE ))

#define DISPLAY_X(x)  ( UNSCALE_X( (x) - display_x ))
#define DISPLAY_Y(y)  ( UNSCALE_Y( (y) - display_y ))

#define SCALE_X(x) ( (( x ) * SCALE ) / 4 )
#define SCALE_Y(y) ( (( y ) * SCALE ) / 4 )

#define REAL_X(x) (SCALE_X(x) + display_x)
#define REAL_Y(y) (SCALE_Y(y) + display_y)

//extern int world_x;
//extern int world_y;

extern int display_x;
extern int display_y;

//----------------------------------------------------------------
//------------------------------------------

static BOOL SelectExistingFile( HWND hParent, PSTR szFile)
{
   
   OPENFILENAME ofn;       // common dialog box structurechar szFile[260];       // buffer for filenameHWND hwnd;              // owner windowHANDLE hf;              // file handle// Initialize OPENFILENAMEZeroMemory(&ofn, sizeof(OPENFILENAME));
   int x;
   szFile[0] = 0;
   memset( &ofn, 0, sizeof( OPENFILENAME ) );
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hParent;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = 256;
   ofn.lpstrFilter = "Bodies\0*.Body\0";
   ofn.nFilterIndex = 1;
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST
              | OFN_NOREADONLYRETURN;// Display the Open dialog box. 

   x = GetOpenFileName(&ofn);
   return x;
   
 //  return FALSE;
}


//------------------------------------------
static BOOL SelectNewFile( HWND hParent, PSTR szFile )
{
   
   OPENFILENAME ofn;       // common dialog box structurechar szFile[260];       // buffer for filenameHWND hwnd;              // owner windowHANDLE hf;              // file handle// Initialize OPENFILENAMEZeroMemory(&ofn, sizeof(OPENFILENAME));
   szFile[0] = 0;
   memset( &ofn, 0, sizeof( OPENFILENAME ) );
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hParent;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = 256;
   ofn.lpstrFilter = "Bodies\0*.Body\0";
   ofn.nFilterIndex = 1;
   ofn.Flags = OFN_NOTESTFILECREATE
              | OFN_NOREADONLYRETURN ;// Display the Open dialog box. 

   return GetOpenFileName(&ofn);
}

//------------------------------------------


BOUNDRY Forms[2] = { { 3, {-10, -10, 0 },
                       { {10, 30, 0 },
                         { 10, -30, 0 },
                         { -20, 0, 0 } } },
                     { 4, {10, 20, 0 },
                        { { 0, -40, 0 },
                         { -20, 0, 0 },
                         { 0, 40, 0 },
                         { 20, 0, 0 } } } 
                     };


#include "body.hpp"

//----------------------------------------------------------------

void BODY::AddRadar( RCOORD power, PCVECTOR vo )
   {
       char pName[64];
       Radar[nRadar] = new RADAR( power, vo );
       Radar[nRadar]->PutOn( pO );
       sprintf( pName, "Radar(%d) Distance", nRadar + 1 );
       Brain->AddInput( &Radar[nRadar]->distance, pName );
       sprintf( pName, "Radar(%d) Angle", nRadar + 1 );
       Brain->AddInput( &Radar[nRadar]->neural_p, pName );
       sprintf( pName, "Radar(%d) Left", nRadar + 1 );
       Brain->AddOutput( &Radar[nRadar]->Left, pName );
       sprintf( pName, "Radar(%d) Right", nRadar + 1 );
       Brain->AddOutput( &Radar[nRadar]->Right, pName );
       nRadar++;
   }


//----------------------------------------------------------------

void BODY::Save( char *pFile )
   {
      HANDLE hFile;

   	hFile = CreateFile( pFile, GENERIC_WRITE, 0,
   						NULL, CREATE_ALWAYS,
   						FILE_ATTRIBUTE_NORMAL,
   						NULL );
      if( hFile != INVALID_HANDLE_VALUE )
      { 
         DWORD dwWritten, dwW;
         dwWritten = sizeof( DWORD ) * 3;
         WriteFile( hFile, &dwWritten, 4, &dwW, NULL );
         WriteFile( hFile, &dwWritten, 4, &dwW, NULL );
         WriteFile( hFile, &pO, 4, &dwW, NULL );

         if( pO )
         {
            dwWritten += SaveObject( (int)hFile, pO );
         }
         else // only a 2d body....
         {
//            WriteFile( hFile, &B, sizeof( B ), &dwW, NULL );
            dwWritten += dwW;
         }

       // this should probably update Object vs brain vs other 
       // file positionings....
         SetFilePointer( hFile, 4, 0, FILE_BEGIN );
         WriteFile( hFile, &dwWritten, 4, &dwW, NULL );
         SetFilePointer( hFile, dwWritten, 0, FILE_BEGIN );

         WriteFile( hFile, &nRadar, sizeof( nRadar), &dwW, NULL );         
         dwWritten += dwW;

         for( int i = 0; i < nRadar; i++ )
         {
            WriteFile( hFile, Radar[i]->Lines[0].d.o, sizeof(VECTOR), &dwW, NULL );
            dwWritten += dwW;
            WriteFile( hFile, &Radar[i]->power, 4, &dwW, NULL );
            dwWritten += dwW;
         }         

         SetFilePointer( hFile, 0, 0, FILE_BEGIN );
         WriteFile( hFile, &dwWritten, 4, &dwW, NULL );
         SetFilePointer( hFile, dwWritten, 0, FILE_BEGIN );

         dwWritten += Brain->Save( hFile );
         CloseHandle( hFile );
      }
   }

//----------------------------------------------------------------

BODY::~BODY( void )
   {
      // delete other extraneous allocations.....
      FreeObject( &pO ); // should clear the pointer too...
      if( Brain )
      {
	      delete Brain; 
         Brain = NULL;
      }
   }

//----------------------------------------------------------------
// always called once for each new body...
   void BODY::Init( BOOL b3D, POBJECT pForm )
   {
      static BODY *pFirst;
      static BODY *pLast;

      if( !pLast )
         pLast = this;
      pNext = pFirst;
      pFirst = this; 
      pLast->pNext = pFirst;


      bs.nInputs = 20;
      bs.Input = (PCONNECTOR)malloc( sizeof( bs.Input[0] ) * 10 );   
      memset( bs.Input, 0, sizeof( bs.Input[0] ) * 10 );
      bs.nOutputs = 20;
      bs.Output = (PCONNECTOR)malloc( sizeof( bs.Output[0] ) * bs.nOutputs );
      memset( bs.Output, 0, sizeof( bs.Output[0] ) * bs.nOutputs );

      Brain = new BRAIN( &bs );

      nRadar = 0; // no radars mounted...
      nWeapons = 0;

      //T.clear(); // sepearte from POBJECT 

      MaxRotateSpeed = _5;
      MaxMoveSpeed = 10.0;  // 100 seconds across board(?)?

      Up = 0;   // along Y axis...
      Down = 0;
      Left = 0; // along X axis...
      Right = 0;
      RollLeft = 0;  // turn y->x... ROLL
      RollRight = 0; // turn x->y... is roll of sorts
//      minx = 0.0f;
//      miny = 0.0f;
//      maxx = 0.0f;
//      maxy = 0.0f;

      if( b3D )
      {
         Forward = 0;  // into z axis...
         Backward = 0; // into z axis...
         TurnUp = 0;
         TurnDown = 0;
         TurnLeft = 0;
         TurnRight = 0;
//         minz = 0.0f;
//         maxz = 0.0f;
      }


      // all bodies exibit at least this 
      // much behavior... plus the boundry
      // inputs which are added later...

      pO = pForm;   // internal mimic of 2dness..???

      if( b3D )
      {
         PutIn( pO, pWorld );
         Brain->AddOutput( &Forward, "Move Forward" );
         Brain->AddOutput( &Backward, "Move Backward" );
         Brain->AddOutput( &Right, "Move Right" );
         Brain->AddOutput( &Left, "Move Left" );
         Brain->AddOutput( &Up, "Move Up" );
         Brain->AddOutput( &Down, "Move Down" );

         Brain->AddOutput( &TurnRight, "Turn Right" );
         Brain->AddOutput( &TurnLeft, "Turn Left" );
         Brain->AddOutput( &TurnUp, "Turn Up" );
         Brain->AddOutput( &TurnDown, "Turn Down" );
         Brain->AddOutput( &RollLeft, "Roll Left" );
         Brain->AddOutput( &RollRight, "Roll Right" );
      }
      else if( !b3D )
      {
         PutOn( pO, pFlatWorld );
         Brain->AddOutput( &Up, "Move Forward" );
         Brain->AddOutput( &Down, "Move Backward" );
         Brain->AddOutput( &Right, "Move Right" );
         Brain->AddOutput( &Left, "Move Left" );
         // perhaps these are backwards always?
         // perhaps they are not...
         Brain->AddOutput( &RollLeft, "Turn Right" );
         Brain->AddOutput( &RollRight, "Turn Left" );
      }
   }

//----------------------------------------------------------------

   void BODY::AddBoundries( void )
   {
      int p;
      PFACETSET pps;
      char byBuffer[256];
      pps = pO->pPlaneSet;

      for( p = 0; p < pps->nUsedPlanes; p++ )
      {
         sprintf( byBuffer, "Plane Bound %d", p+1);
         Brain->AddInput( &pps->pPlanes[p].nBrain, byBuffer );
      }
   }

//----------------------------------------------------------------

   BODY::BODY( char *pFile, PCVECTOR vo, PCVECTOR vforward, PCVECTOR vright )
   {
      HANDLE hFile;
      int n, i;
// also set current orientation..... 

   	hFile = CreateFile( pFile, GENERIC_READ, 0,
      						NULL, OPEN_EXISTING,
      						FILE_ATTRIBUTE_NORMAL,
      						NULL );
      if( hFile != INVALID_HANDLE_VALUE )
      {
         DWORD dwBrain, dwR, dwRadar, dwFlag;
//         int n, i;

         ReadFile( hFile, &dwBrain, sizeof(dwBrain), &dwR, NULL ); 
         ReadFile( hFile, &dwRadar, sizeof( dwRadar), &dwR, NULL );
         ReadFile( hFile, &dwFlag, sizeof( dwFlag ), &dwR, NULL );

         {
            POBJECT po;
            po = LoadObject( (int)hFile );
            Init( TRUE, po );  // uhmmm - maybe???
         }

         AddBoundries();  // lines or planes?????

         pO->T.Translate( vo ); // set current position...
         pO->T.RotateTo( vforward, vright ); // set current position...

         
         SetFilePointer( hFile, dwRadar, 0, FILE_BEGIN );
         ReadFile( hFile, &n, sizeof(n), &dwR, NULL );
         for( i = 0; i < n; i++ )
         {
            struct {
               VECTOR v;
               RCOORD power;
            } s;

            ReadFile( hFile, s.v, sizeof(VECTOR) , &dwR, NULL );
            ReadFile( hFile, &s.power, sizeof( s.power ), &dwR, NULL );
            AddRadar( s.power, s.v );
         }
           
         SetFilePointer( hFile, dwBrain, 0, FILE_BEGIN );
         Brain->Load( hFile );

         CloseHandle( hFile );
      }
   }

//----------------------------------------------------------------

   BODY::BODY( POBJECT pobj, PCVECTOR vo, PCVECTOR vforward, PCVECTOR vright, char *name) 
   {
      Init( TRUE, pobj );
      pO->T.Translate( vo );
      pO->T.RotateTo( vforward, vright );
      AddBoundries();  // different structure from 2d ....
  }

//----------------------------------------------------------------

   BODY::BODY( int x, int y, int Form, char *name) 
   {
      VECTOR n, o;
      PFACET pp;
      POBJECT po;
      po = CreateObject();

      Init( FALSE, po );

      pp = AddPlane( pO->pPlaneSet, VectorConst_0, VectorConst_Z, 0 ); // don't really NEED a plane...
      // but if when we texture this we can use the normal...
      // and for now the normal points towards a normal viewer...
      pp->color = Color( 43, 43, 150 );

      pO->T.Translate( (RCOORD)x, (RCOORD)y, 0 ); // set current position...

      SetPoint( o, Forms[Form].BoundStart );
      for( int i = 0; i < Forms[Form].nBounds; i++ )
      {
         char Name[256];
         
         wsprintf( (char*)Name, "Boundry %d", i );

         Bounds[i] = 0;  // no collision value yet...
         Brain->AddInput( Bounds + i, (char*)Name );

         SetPoint( n, Forms[Form].pBounds[i] );

         AddLineToPlane( CreateLine( pO->pLinePool, 
                                     o, n, 0.0f, 1.0f ), pp );

         add( o, o, n );
      }
  }

//----------------------------------------------------------------
 // draw in flat view mode....
   void BODY::DrawBoundry( void )  // screen coordinates??
   {
      VECTOR v1, v2;  
      RAY r;
      int i;
      CDATA c;

      c = BOUND_COLOR;

      for( i = 0; i < pO->pLinePool->nUsedLines; i++ )
      {
         pO->T.Apply( &r, &pO->pLinePool->pLines[i].d );
         SetPoint( v1, r.o );
         add( v2, v1, r.n );
         do_line( GetDisplayImage( hVideo ), 
                           (int)DISPLAY_X(v1[0]),
                           (int)DISPLAY_Y(v1[1]),
                           (int)DISPLAY_X(v2[0]), 
                           (int)DISPLAY_Y(v2[1]),
                           c );
      }
   }

//----------------------------------------------------------------
#define MIN(a,b) ( ((a)<(b))?(a):(b))
#define MAX(a,b) ( ((a)>(b))?(a):(b))

   BODY *BODY::Other( BODY *pb )
   {
      static BODY *ps;
      if( !pb )
         pb = ps = this;
      pb = pb->pNext;
      if( pb != ps )
         return pb;
      return NULL;
   }

//----------------------------------------------------------------
   void BODY::DrawRadar( int which )
   {
      RAY d;
      CDATA c;
      RCOORD dist = 1.0; // result is within portion of radar LINE...

      c = RADAR_COLOR;

      pO->T.Apply( &d, &Radar[which]->Lines[0].d ); // rotate and move with body... 
      add( d.n, d.n, d.o ); // okay - slope + origin end of line...
      do_line( hVideo->pImage, (int)DISPLAY_X( d.o[0] ),
                               (int)DISPLAY_Y( d.o[1] ),
                               (int)DISPLAY_X( d.n[0] ),
                               (int)DISPLAY_Y( d.n[1] ),
                               c );
      {
         BODY *b;
         for( b = Other(0); 
              b ; 
              b = Other(b) )
         {
            RAY rbound;
            int bound;
            VECTOR v;
            PLINESEGSET pls;
            // should maybe consider using bounds to mass
            // sort body behind view Plane...
            pls = b->pO->pLinePool;
            b->pO->T.Apply( v, pls->pLines[0].d.o );
            pO->T.ApplyInverse( rbound.o, v );
            for( bound = 0; bound < pls->nUsedLines; bound++ )
            {
               VECTOR dx;
               b->pO->T.ApplyRotation( v, pls->pLines[bound].d.n );
               pO->T.ApplyInverseRotation( rbound.n, v );
               add( dx, rbound.o, rbound.n );
               do_line( hVideo->pImage, (int)DISPLAY_X( rbound.o[0] )
                                      , (int)DISPLAY_Y( rbound.o[1] )
                                      , (int)DISPLAY_X( dx[0] ) 
                                      , (int)DISPLAY_Y( dx[1] ), Color( 0, 0, 127 ) );
               Radar[which]->Hits( &dist, &rbound );
               add( rbound.o, rbound.o, rbound.n );
            }
         }
      }
      Radar[which]->distance = MAX_INPUT_VALUE - (int)( ( dist ) * (RCOORD) MAX_INPUT_VALUE ) ;
   }

//----------------------------------------------------------------

   void BODY::Draw( void )  // screen coordinates??
   {
      DrawBoundry( );
      for( int i = 0; i < nRadar; i++ )
      {
         DrawRadar( i );
      }
   }

//----------------------------------------------------------------

   void BODY::CollideWorld( void ) 
   {
      PFACETSET pps;
      PFACETSET ppsthis;
      PFACET pp,pwp;
      PLINESEGPSET plps;
      PLINESEG *ppl;
      int p, l, wp;
      RCOORD t;

      POBJECT po;
      if( pO->pIn ) // 3d method of collision...
      {
         po = pO->pIn;
         pps = po->pPlaneSet;
         ppsthis = pO->pPlaneSet;

         pp = ppsthis->pPlanes;
         for( p = 0; p < ppsthis->nUsedPlanes; p++ )
            pp[p].nBrain = 0; 
         //tl.Slope, 
         //tl.Origin, 

         for( wp = 0; wp < pps->nUsedPlanes; wp++ )
         {
            pwp = pps->pPlanes+wp;
            for( p = 0; p < ppsthis->nUsedPlanes; p++ )
            {
               RAY rp;
               pp = ppsthis->pPlanes + p;
               pO->T.Apply( &rp, &pp->d );
               if( IntersectLineWithPlane( pwp->d.n, pwp->d.o,
                                           rp.n, rp.o,                                   
                                           &t ) 
                                         )
               {
                  int l;
                  bool intersect;
                  //RCOORD timeFrom, timeTo;
                     RCOORD time;
                  RCOORD mintime;
                  VECTOR vlo;
                  RAY rl; //vector line origin;

                  plps = pp->pLineSet;
                  ppl = plps->pLines;
                  intersect = false;
                  mintime = 0;  
                  for( l = 0; l < plps->nUsedLines; l++ ) 
                  {
                     pO->T.Apply( &rl, &ppl[l]->d );

                     scale( vlo, rl.n, ppl[l]->dFrom );
                     add( vlo, vlo, rl.o );

                     if( IntersectLineWithPlane( pwp->d.n, pwp->d.o, 
                                                rl.n, vlo,
                                                &time ) )
                     {
                        if( time <= 0 ) // on or behind the plane...
                        {
                           if( time )
                           {
                              VECTOR v;
                              scale( v, pp->d.n, t );
                              pO->T.TranslateRel( v );
                           }
                           intersect = true;
                        }
                     }

                     pO->T.Apply( &rl, &ppl[l]->d );
                     scale( vlo, rl.n, ppl[l]->dTo );
                     add( vlo, vlo, rl.o );

                     if( IntersectLineWithPlane( pwp->d.n, pwp->d.o, 
                                         rl.n, vlo,
                                         &time ) )
                     {
                        if( time <= 0 ) // on or behind the plane...
                        {
                           if( time ) // could be done already...
                           {
                              VECTOR v;
                              scale( v, pp->d.n, t );
                              pO->T.TranslateRel( v );
                           }
                           intersect = true;
                        }
                     }
                  }
                  if( !intersect )
                  {
                     // this plane went through - but non of its 
                     // lines intersected... 
                     // directly shift plane bacwareds along normal...

                     // works for the world.... 

                     // never happens now...
                     if( t <= 0 ) 
                     {
                        VECTOR v;
                        if( t ) // if on - no correction...
                        {
                           scale( v, pp->d.n, t );
                           pO->T.TranslateRel( v );
                        }
                        intersect = true;
                        continue;
                     }
                  }

                  if( intersect ) // this plane collided definatly...
                  {
                     pp->nBrain = MAX_INPUT_VALUE;
                     continue;
                  }
   //               pp->p = t; // can save distance from this plane....
               }
            }
         }
      }
      else if( pO->pOn ) // 2d method of collision
      {
         po = pO->pOn;
      }


   
      return;

         for( p = 0; p < ppsthis->nUsedPlanes; p++ )
            ppsthis->pPlanes[p].nBrain = 0; 

         for( wp = 0; wp < pps->nUsedPlanes; wp ++ ) {
   //         RAY rp;
   //         pwp = pps->pPlanes + wp;
   //         po->T.Apply( &rp, pwp->d );

            for( p = 0; p < ppsthis->nUsedPlanes; p++ ){
               pp = ppsthis->pPlanes + p;
               plps = pp->pLineSet;
               ppl = plps->pLines;
               for( l = 0; l < plps->nUsedLines; l++ )
               {
                  RCOORD time;
                  RAY rl;
                  po->T.Apply( &rl, &ppl[l]->d );
                  if(IntersectLineWithPlane(
                           rl.n, rl.o,
                           pwp->d.n, pwp->d.o, &time ) )
                  {
                     if( time <= ppl[l]->dTo  &&
                         time >= ppl[l]->dFrom )
                     {
                        ppsthis->pPlanes[p].nBrain = MAX_INPUT_VALUE;
                        break; // this line/plane / object?? no :(
                     }
                  }
               }
            }
         }
   //   }
   }

//----------------------------------------------------------------

   void BODY::Update( void )
   {
      // read current inputs and output and 
      // make up what to do...
      BOOL bUnmove = FALSE;
      BOOL bStopped = FALSE;
      int i;

      Brain->Process();

#define SPEED(a,b) (RCOORD)( ( ( (a) - (b) ) * MaxMoveSpeed ) / (RCOORD)100.0)
#define TURN(a,b) (RCOORD)( ( ( (a) - (b) ) * MaxRotateSpeed ) / (RCOORD)100.0)

      pO->T.SetSpeed( SPEED( Left, Right ),
                   SPEED( Up, Down ),
                   SPEED( Forward, Backward ) ); 

      pO->T.RotateRel( TURN( TurnDown , TurnUp ),
                    TURN( TurnLeft, TurnRight ),
                    TURN( RollLeft, RollRight ) );

      pO->T.Move(); // move then turn....
                // option for turn then move?
                // also spline turn across move....

      for( i = 0; i < nRadar; i++ )
         Radar[i]->Move();  // update rotate left and right...

   if( pO ) //..... only good in 2qqd with a body...
   {
      CollideWorld( );
      // Collide( pWithObjects );
   }
//   else
   {
      // keep collision information once 
      // it is set within the next loop....
      for( i = 0; i < 16; i++ )
         Bounds[i] = 0;                

re_move:
      bUnmove = FALSE;
      {
         VECTOR v1, v2;
         PLINESEGSET pls;

         pls = pO->pLinePool;

         pO->T.Apply( v1, pls->pLines[0].d.o );

         for( i = 0; i < pls->nUsedLines; i++ )
         {
            pO->T.ApplyRotation( v2, pls->pLines[i].d.n );
            add( v2, v1, v2 );
/*
            if( ( v2[0] < 0 || v2[0] > world_x ) 
              ||( v2[1] < 0 || v2[1] > world_y ) 
              )
            {
               Bounds[i] = MAX_INPUT_VALUE;  
               bUnmove = TRUE;
            }
            */
            SetPoint( v1, v2 );
            // test this line versus surrounding boundries...
         }

         if( bUnmove )
         {
            // Unmove should probably be 
            // replaced with something like
            // an absolute translation
            // but result can also provide
            // further rotation about the center of gravity...
            // cause if it's NOT moving - it WON'T UNMOVE

            if( !bStopped )
            {
               pO->T.Unmove();
               bStopped = TRUE;
               goto re_move;  
            }
         }
      }
   }
   }

//----------------------------------------------------------------

   bool BODY::BodyOn( PCVECTOR vforward,
                      PCVECTOR vright,
                      PCVECTOR vo )
   {
      if( pO )
         return ObjectOn( pO, vforward, vright, vo );

      return false;
   }

