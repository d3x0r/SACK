#include <stdhdrs.h>

#include "global.h"

#include <image.h>
#include <render.h>
#include <vectlib.h>
#include <controls.h>

#include <sharemem.h>

#include "virtuality/view.h"


#define MNU_ADDLINE			  1000
#define MNU_ADDPREDEF			1010
#define MNU_ADDPREDEF_LAST	 1020

VECTOR vToScreen;

#define MOUSE_LOCK 7


typedef struct EDITOR_tag {
	PTRANSFORM TView;
	PRENDERER hVideo;
	PMENU  hMenu;  // first one should have 'add line'
	POBJECT pCurrent;
	PFACET pCurrentFacet;
	PLINESEG  pCurrentLine;
	IMAGE_POINT ptO, ptN;
	struct {
		int bLocked:1;  // mouse is locked to ptMouse...
		int bDragOrigin:1;
		int bDragNormal:1;
		int bDragWorld:1;
	};
	IMAGE_POINT ptMouse; // current point of mouse....
	IMAGE_POINT ptMouseDel; // accumulator of mouse delta (if locked)
	IMAGE_POINT ptUpperLeft; // upper left screen coordinate of window...
} EDITOR, *PEDITOR;


static void GetRealPoint( Image pImage, PVECTOR vresult, IMAGE_POINT pt );
void CPROC EditResizeCallback( PTRSZVAL dwUser );


LOGICAL CPROC PageUpKey( PTRSZVAL psv, _32 keycode )
{
	PEDITOR pe = (PEDITOR)psv;
	TranslateRel( pe->TView, 0, 0, -1.0 );
	EditResizeCallback( (PTRSZVAL)pe );
	return 1;
}


LOGICAL CPROC PageDownKey( PTRSZVAL psv, _32 keycode )
{
	PEDITOR pe = (PEDITOR)psv;
	TranslateRel( pe->TView, 0, 0, 1.0 );
	EditResizeCallback( (PTRSZVAL)pe );
	return 1;
}

int CPROC EditMouseCallback( PTRSZVAL dwUser, S_32 x, S_32 y, _32 b )
{
	static int _x, _y, _right, _left;
	int right, left;
	PEDITOR pe = (PEDITOR)dwUser;
	Image surface = GetDisplayImage( pe->hVideo );
	right = b & MK_RBUTTON;
	left = b & MK_LBUTTON;

	//if( pe->bLocked )
	//{/
	//}
	pe->ptMouseDel[0] = x - pe->ptMouse[0];
	pe->ptMouseDel[1] = y - pe->ptMouse[1];
	pe->ptMouse[0] = x;
	pe->ptMouse[1] = y;

	if( !(left + right + _left + _right) ) // just moving mouse..
	{
		if( pe->bLocked )
		{
			if( pe->ptMouseDel[0] > MOUSE_LOCK || pe->ptMouseDel[0] < -MOUSE_LOCK ||
				 pe->ptMouseDel[1] > MOUSE_LOCK || pe->ptMouseDel[1] < -MOUSE_LOCK )
			{
				pe->bLocked = FALSE;
				SetMousePosition( pe->hVideo, pe->ptMouse[0], pe->ptMouse[1] );
				return 1; // done....
			}
		}
		else
		{
			if( ( abs( pe->ptO[0] - x ) < MOUSE_LOCK ) &&
				 ( abs( pe->ptO[1] - y ) < MOUSE_LOCK ) )
			{
				pe->bLocked = TRUE;
				pe->ptMouseDel[0] = 0;
				pe->ptMouseDel[1] = 0;
				pe->ptMouse[0] = pe->ptO[0];
				pe->ptMouse[1] = pe->ptO[1];
			}
			else
			if( ( abs( pe->ptN[0] - x) < MOUSE_LOCK ) &&
				 ( abs( pe->ptN[1] - y ) < MOUSE_LOCK ) )
			{
				pe->bLocked = TRUE;
				pe->ptMouseDel[0] = 0;
				pe->ptMouseDel[1] = 0;
				pe->ptMouse[0] = pe->ptN[0];
				pe->ptMouse[1] = pe->ptN[1];
			}
		}

		if( pe->bLocked ) // only reset mouse if not dragging??
		{
			pe->ptMouse[0] -= pe->ptMouseDel[0];
			pe->ptMouse[1] -= pe->ptMouseDel[1];
			SetMousePosition( pe->hVideo, pe->ptMouse[0],
											 pe->ptMouse[1] );
		}

	}

	if( pe->bLocked )
	{
		x = pe->ptMouse[0] - pe->ptMouseDel[0];
		y = pe->ptMouse[1] - pe->ptMouseDel[0];
	}

	if( !right && _right )
	{
#ifdef __WINDOWS__
		IMAGE_POINT p;
		int nCmd;
		GetCursorPos( (POINT*)&p );
		nCmd = TrackPopup( pe->hMenu
								//, TPM_CENTERALIGN
								//| TPM_TOPALIGN
								//| TPM_RIGHTBUTTON
								//| TPM_RETURNCMD
								//| TPM_NONOTIFY
								//,p[0]
								//,p[1]
								,0
							  // ,NULL // pe->hVideo->hWndOutput
							  //,NULL
							  );
		switch( nCmd )
		{
		case MNU_ADDLINE:
			break;
		}
#endif
	}

	if( left && !_left ) // left click select...
	{
		if( x == pe->ptO[0] && y == pe->ptO[1] )
		{
			pe->bDragOrigin = TRUE;
		} 
		else if( x == pe->ptN[0] && y == pe->ptN[1] )
		{
			pe->bDragNormal = TRUE;
		}
		else
		{
			pe->bDragWorld = TRUE;
		}	
	}
	else if( !left && _left ) // end click...
	{
		pe->bDragOrigin = FALSE; // cancel all operations...
		pe->bDragNormal = FALSE;
		pe->bDragWorld = FALSE;
	}
	else if( left && _left ) // probably dont want to do this if !left && !_left
	{
		if( pe->bDragOrigin )
		{
			VECTOR vo;
			if( pe->ptMouseDel[0] || pe->ptMouseDel[1] )
			{
				GetOriginV( pe->TView, vo );
				Invert( vo );
				GetRealPoint(surface, (PVECTOR)vo, pe->ptMouse );
				Apply( (PCTRANSFORM)pe->TView, pe->pCurrentLine->r.o, vo ); // apply inverse -> Apply...
				// update Intersecting lines......
				/*
				{
					PLINESEG pl;
					PLINESEGPSET plps;
					int l;
					plps = pe->pCurrentFacet->pLineSet;
					pl = pe->pCurrentLine->pTo;
					
					for( l = 0; l < plps->nUsedLines; l++ )
					{
						pl = plps->pLines[l];
						if( pl == pe->pCurrentLine )
							continue;

					}

				}
				*/
				EditResizeCallback( (PTRSZVAL)pe ); // update screen...
			}
		} 
		else if( pe->bDragNormal )
		{
			VECTOR vn;
			if( pe->ptMouseDel[0] || pe->ptMouseDel[1] )
			{
				GetOriginV( pe->TView, vn );
				Invert( vn );
				GetRealPoint(surface, (PVECTOR)vn, pe->ptMouse );
				sub( vn, vn, pe->pCurrentLine->r.o );
				Apply( (PCTRANSFORM)pe->TView, pe->pCurrentLine->r.n, vn ); // apply inverse -> Apply...
				// update Intersecting lines......
				EditResizeCallback( (PTRSZVAL)pe ); // update screen...
			}
		}
		else if( pe->bDragWorld )
		{
			VECTOR v1, v2;
			if( pe->ptMouseDel[0] || pe->ptMouseDel[1] )
			{
				GetOriginV( pe->TView, v1 );
				Invert( v1 );
				v2[vForward] = v1[vForward];
				pe->ptMouseDel[0] = pe->ptMouse[0] - pe->ptMouseDel[0];
				pe->ptMouseDel[1] = pe->ptMouse[1] - pe->ptMouseDel[1];

				GetRealPoint(surface, (PVECTOR)v1, pe->ptMouse );
				GetRealPoint(surface, (PVECTOR)v2, pe->ptMouseDel );
				sub( v1, v2, v1 );
				v1[vForward] = 0; 
				TranslateRelV( pe->TView, v1 );
				EditResizeCallback( (PTRSZVAL)pe ); // update screen...
			}
		}

	}

	_x = x;
	_y = y;
	_right = right;
	_left  = left;
	return 1;
}

#define MARK_SIZE 3
void MarkOrigin( PEDITOR pe, Image pImage )
{
	do_line( pImage, pe->ptO[0] - MARK_SIZE,
						  pe->ptO[1] - MARK_SIZE,
						  pe->ptO[0] + MARK_SIZE, 
						  pe->ptO[1] - MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( pImage, pe->ptO[0] + MARK_SIZE,
						  pe->ptO[1] - MARK_SIZE,
						  pe->ptO[0] + MARK_SIZE, 
						  pe->ptO[1] + MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( pImage, pe->ptO[0] + MARK_SIZE,
						  pe->ptO[1] + MARK_SIZE,
						  pe->ptO[0] - MARK_SIZE, 
						  pe->ptO[1] + MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( pImage, pe->ptO[0] - MARK_SIZE,
						  pe->ptO[1] + MARK_SIZE,
						  pe->ptO[0] - MARK_SIZE, 
						  pe->ptO[1] - MARK_SIZE, Color( 200, 200, 200 ) );
}


void MarkSlope( PEDITOR pe, Image pImage )
{
	do_line( pImage, pe->ptN[0],
						  pe->ptN[1] - MARK_SIZE,
						  pe->ptN[0] + MARK_SIZE, 
						  pe->ptN[1] + MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( pImage, pe->ptN[0] + MARK_SIZE,
						  pe->ptN[1] + MARK_SIZE,
						  pe->ptN[0] - MARK_SIZE, 
						  pe->ptN[1] + MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( pImage, pe->ptN[0] - MARK_SIZE,
						  pe->ptN[1] + MARK_SIZE,
						  pe->ptN[0], 
						  pe->ptN[1] - MARK_SIZE, Color( 200, 200, 200 ) );
}

// SetClip sets viewport x, y, 1 so that 1, 1, 1 is visible at outer corner
static RAY vc1 = { {0, 0, 1.0f}, { -1.0f, 0, 2.0f } },
			  vc2 = { {0, 0, 1.0f}, { 1.0f, 0, 2.0f}}, 
			  vc3 = { {0, 0, 1.0f}, { 0, -1.0f, 2.0f }},
			  vc4 = { {0, 0, 1.0f}, { 0, 1.0f, 2.0f }}, 
			  vc0 = { {0, 0, 1.0f}, { 0, 0, 1.0f }}; // allow microscopic closenesss....

static void SetClip( RCOORD width, RCOORD height )
{
	vc1.o[0] = width;
	vc2.o[0] = -width;
	vc3.o[1] = height;
	vc4.o[1] = -height;
}


#define ProjectX( v )  ((pImage->width/2)  + (int)( ( ((RCOORD)pImage->width)  * (v)[0] ) / ( (v)[2] * 2 ) ) )
#define ProjectY( v )  ((pImage->height/2) - (int)( ( ((RCOORD)pImage->height) * (v)[1] ) / ( (v)[2] * 2 ) ) )
static void GetViewPoint( Image pImage, IMAGE_POINT presult, PVECTOR vo )
{
	presult[0] = ProjectX( vo );
	presult[1] = ProjectY( vo );
}

static void GetRealPoint( Image pImage, PVECTOR vresult, IMAGE_POINT pt )
{
	if( !vresult[2] )
		vresult[2] = 1.0f;  // dumb - but protects result...
	// use vresult Z for unprojection...
	vresult[0] = (pt[0] - (pImage->width/2)) * (vresult[2] * 2.0f) / ((RCOORD)pImage->width);
	vresult[1] = ((pImage->height/2) - pt[1] ) * (vresult[2] * 2.0f) / ((RCOORD)pImage->height);
}


static void DrawLine( Image pImage, PCVECTOR p, PCVECTOR m, RCOORD t1, RCOORD t2, CDATA c )
{
	VECTOR v1,v2;
	add( v1, scale( v1, m, t1 ), p );
	add( v2, scale( v2, m, t2 ), p );
	/*
	glBegin( GL_LINES );
	glColor4ubv( (unsigned char *)&c );
	glVertex3dv( v1 );
	glVertex3dv( v2 );
	glEnd();
	glFlush();
	*/
	//printf("end points = ");
	//PrintVector(v1);
	//PrintVector(v2);
	/*if( Clip( v1, v2, vc0.n, vc0.o ) &&
		 Clip( v1, v2, vc1.n, vc1.o ) &&
		 Clip( v1, v2, vc2.n, vc2.o ) &&
		 Clip( v1, v2, vc3.n, vc3.o ) &&
		 Clip( v1, v2, vc4.n, vc4.o ) )
		 */
	{

		do_line( pImage, ProjectX( v1 ), ProjectY( v1 ),
				  ProjectX( v2 ), ProjectY( v2 ), c );
	}
}


void CPROC EditResizeCallback( PTRSZVAL dwUser  )
{
	PEDITOR pe = (PEDITOR)dwUser;
	//PFACETSET *ppfs;
	//PFACET pf;

	//PLINESEGPSET *pplps;
	//PMYLINESEGSET *ppls;
	//PMYLINESEG pl;
	//int l;
	// ShowCurrent();
	//ClearImage( surface );
	{
		//IMAGE_RECTANGLE r;
		//GetDisplayPosition( pe->hVideo, pe->ptUpperLeft, pe->ptUpperLeft + 1, NULL, NULL );
		/*
		GetWindowRect( pe->hVideo->hWndOutput, &r );
		pe->ptUpperLeft[0] = r.left //+ GetSystemMetrics( SM_CXBORDER )
									  + GetSystemMetrics( SM_CXFRAME );
		pe->ptUpperLeft[1] = r.top + GetSystemMetrics( SM_CYBORDER )
									 + GetSystemMetrics( SM_CYFRAME )
									 + GetSystemMetrics( SM_CYCAPTION );
									 */
	}
#if 0
	SetClip( (RCOORD)GetDisplayImage( pe->hVideo )->width,
				(RCOORD)GetDisplayImage( pe->hVideo )->height );
	ppfs = pe->pCurrent->objinfo->ppFacetPool;
	ppls = GetFromSet( LINEPSEG, pe->pCurrent->objinfo->ppLinePool );
	//ForAllInSet( FACET, &pe->pCurrent->objinfo->ppFacetPool, DrawLineSegs );
	//int f;
	//for( f = 0; f < ; f++ )
	{
		int lines;
		//pf = GetFromSet( FACET, pfs->pFacets + f;
		pplps = &pf->pLineSet;
		lines = CountUsedInSet( LINESEGP, pf->pLineSet );
		for( l = 0; l < lines; l++ )
		{
			RAY ld; // local directional...
			PLINESEGP plsp = GetSetMember( LINESEGP, pplps, l );
			pl = GetSetMember( MYLINESEG, ppls, plsp->nLine );
			if( pl == pe->pCurrentLine )
			{
				ApplyInverseR( pe->TView, &ld, &pl->r );
				DrawLine(surface, ld.o, ld.n, pl->dFrom-5, pl->dFrom, Color( 192, 192, 0 ) );
				DrawLine(surface, ld.o, ld.n, pl->dFrom, pl->dTo, Color( 0, 150, 0 ) );
				DrawLine(surface, ld.o, ld.n, pl->dTo, pl->dTo+5, Color( 192, 192, 0 ) );
				GetViewPoint(surface, &pe->ptO, ld.o );
				add( ld.o, ld.o, ld.n ); // add one slope to origin to get slope point
				GetViewPoint(surface, &pe->ptN, ld.o );
				MarkOrigin( pe, surface );
				MarkSlope( pe,surface );
			}
			else
			if( pf = pe->pCurrentFacet )
			{
				ApplyInverseR( pe->TView, &ld, &pl->r );
				DrawLine(surface, ld.o, ld.n, pl->dFrom, pl->dTo, Color( 0, 0, 150 ) );
			}
			else
			{
				ApplyInverseR( pe->TView, &ld, &pl->r );
				DrawLine(surface, ld.o, ld.n, pl->dFrom, pl->dTo, Color( 92, 92, 92 ) );
			}
		}
	}
	UpdateDisplay( pe->hVideo );
#endif
}
	



PEDITOR CreateEditor( POBJECT po )
{
	PEDITOR pe;
	pe = (PEDITOR)Allocate( sizeof( EDITOR ) );
	MemSet( pe, 0, sizeof( EDITOR ) );
	pe->TView = CreateTransform();
	pe->hVideo = OpenDisplaySizedAt( 0, -1, -1, -1, -1 );
	//InitVideo( "World Editor" );
	SetRedrawHandler( pe->hVideo, (void (CPROC*)(PTRSZVAL,PRENDERER))EditResizeCallback, (PTRSZVAL)pe );
	SetMouseHandler( pe->hVideo, EditMouseCallback, (PTRSZVAL)pe );
	BindEventToKey( pe->hVideo, KEY_PGUP, 0, PageUpKey, (PTRSZVAL)pe );
	BindEventToKey( pe->hVideo, KEY_PGDN, 0, PageDownKey, (PTRSZVAL)pe );
  // SetCloseHandler( pe->hVideo, EditCloseCallback, (PTRSZVAL)pe );

	// menu for this instance....
#ifdef __WINDOWS__
	pe->hMenu = CreatePopup(); //CreatePopupMenu();
	AppendPopupItem( pe->hMenu, MF_STRING, MNU_ADDPREDEF+0, "Add Square" );
	AppendPopupItem( pe->hMenu, MF_STRING, MNU_ADDPREDEF+1, "Add Triangle" );
	AppendPopupItem( pe->hMenu, MF_STRING, MNU_ADDLINE, "Add Line" );
#else
	pe->hMenu = NULL;
#endif
	SetPoint( vToScreen, VectorConst_Z ); // facing into screen...
	Invert( vToScreen );	 // facing out of screen...

	pe->pCurrent = po;
	pe->pCurrentFacet = NULL; //po->objinfo.FacetPool.pFacets + 0;
	pe->pCurrentLine = NULL; //po->objinfo.LinePool.pLines + 0;

	ClearTransform( pe->TView );
	Translate( pe->TView, 0, 
						  0, 
						  (RCOORD)-100.0 );

	pe->bLocked = FALSE;

	EditResizeCallback( (PTRSZVAL)pe);
	UpdateDisplay( pe->hVideo );
	return pe;
}

void DestroyEditor( PEDITOR pe )
{
	DestroyTransform( pe->TView );
	CloseDisplay( pe->hVideo );
	Release( pe );
}


