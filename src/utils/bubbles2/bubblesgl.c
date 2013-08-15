
#define DEFINE_DEFAULT_RENDER_INTERFACE
#define DEFINE_DEFAULT_IMAGE_INTERFACE

#include <render.h>
#include <image.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <vectlib.h>
#define MAX_BUBBLES 50

typedef struct bubble_thing
{
	CDATA base;
	CDATA dest;
   CDATA current;
	int length;
	int step;
   VECTOR v; // also contains x, y
	int x, y;

	int over; // boolean, starts as over others... so no colision

	int delx, dely;

   PLIST near_bubbles; // this is a list of what other bubbles are near this one.
} BUBBLE, *PBUBBLE;

struct {
	Image shadow;
	Image shaded;
	Image cover;
	unsigned int gl_shadow;
	unsigned int gl_shaded;
	unsigned int gl_cover;
   _32 w, h;
	PRENDERER render;
	struct {
		BIT_FIELD bFirstPaintDone : 1;
		BIT_FIELD bSecondPaintDone : 1;
	} flags;

   int collision_distance;

	CDATA base;
	CDATA dest;
   CDATA current;

	PLIST bubbles;
	int bubble_scale;
   //PSPACEWEB web;
} l;



void ChooseColorDest( PBUBBLE bubble )
{
	if( bubble->step >= bubble->length )
	{
		bubble->base = bubble->dest;
		bubble->step = 0;
		bubble->length = ( rand() * 12L / RAND_MAX ) + 1;
		bubble->dest = AColor( rand() * 256 / RAND_MAX
									, rand() * 256 / RAND_MAX
									, rand() * 256 / RAND_MAX
									, 255
									 );
	}
	bubble->current = ColorAverage( bubble->base, bubble->dest
											, bubble->step++, bubble->length );
}

void RelateBubble( PBUBBLE bubble )
{
   //AddWebNode( l.web, bubble->v, (PTRSZVAL)bubble );
#if 0
	PBUBBLE check_bubble;
   INDEX idx;
	VECTOR v1;
	v1[0] = bubble->x;
	v1[1] = bubble->y;
	v1[2] = 0;

	LIST_FORALL( l.bubbles, idx, PBUBBLE, check_bubble )
	{
		VECTOR v_check;
		VECTOR tmp;
		INDEX idx2;
      PBUBBLE check_near;
		RCOORD to_here;
	rescan:
      v_check[0] = check_bubble->x;
      v_check[1] = check_bubble->y;
		v_check[2] = 0;
		to_here = Distance( v1, v_check );
		LIST_FORALL( check_bubble->near_bubbles, idx2, PBUBBLE, check_near )
		{
			RCOORD to_there;
			VECTOR v_near;
			v_near[0] = check_near->x;
			v_near[1] = check_near->y;
			v_near[2] = 0;
			to_there = Distance( v1, v_near );
			if( to_there < to_here )
			{
				// close to there than to here... so move...
            check_bubble = check_near;
				idx = FindLink( &l.bubbles, check_near );
            goto rescan;
			}
		}
      break;
	}
	if( check_bubble )
	{
		AddLink( &check_bubble->near_bubbles, bubble );
      // bi-directional linkage.
		AddLink( &bubble->near_bubbles, check_bubble );
	}
#endif
}

void MoveBubbles( void )
{
	INDEX idx;
	PBUBBLE bubble;
	LIST_FORALL( l.bubbles, idx, PBUBBLE, bubble )
	{
      bubble->x += bubble->delx;
		bubble->y += bubble->dely;
		if( !bubble->delx || bubble->x > l.w )
         bubble->delx = -((rand() * 12 / RAND_MAX)+1);
		if( !bubble->dely || bubble->y > l.h )
         bubble->dely = -((rand() * 12 / RAND_MAX)+1);
		if( bubble->x < 0 )
         bubble->delx = ((rand() * 12 / RAND_MAX)+1);
		if( bubble->y < 0 )
			bubble->dely = ((rand() * 12 / RAND_MAX)+1);
		bubble->v[0] = bubble->x;
      bubble->v[1] = bubble->y;
	}

	LIST_FORALL( l.bubbles, idx, PBUBBLE, bubble )
	{
      VECTOR v;
		// correct relations with other bubbles...
		INDEX idx2, idx3;
		PBUBBLE b2, b3;
		v[0] = bubble->x;
		v[1] = bubble->y;
      v[2] = 0;
		LIST_FORALL( bubble->near_bubbles, idx2, PBUBBLE, b2 )
		{
			VECTOR v2;
			v2[0] = b2->x;
			v2[1] = b2->y;
         v2[2] = 0;
			LIST_FORALL( b2->near_bubbles, idx3, PBUBBLE, b3 )
			{
				VECTOR v3;
            // bubble -> b2 -> next is bubble again.
				if( b3 == bubble )
               continue;
				v3[0] = b3->x;
				v3[1] = b3->y;
				v3[2] = 0;
				if( Distance( v, v2 ) > Distance( v, v3 ) )
				{
					INDEX unlink_idx = FindLink( &b2->near_bubbles, bubble);
					if( unlink_idx == INVALID_INDEX )
                  DebugBreak();
               //DebugBreak();
					SetLink( &bubble->near_bubbles, idx2, 0 );
					if( bubble->near_bubbles->Lock )
                  DebugBreak();
					SetLink( &b2->near_bubbles, unlink_idx, 0 );
					if( b2->near_bubbles->Lock )
                  DebugBreak();

					AddLink( &b3->near_bubbles, bubble );
               AddLink( &bubble->near_bubbles, b3 );
				}
			}
		}
	}

}

void DrawLine( Image pImage, PCVECTOR p, PCVECTOR m, RCOORD t1, RCOORD t2, CDATA c )
{
	VECTOR v1,v2;
	glBegin( GL_LINES );
	c = GLColor( c );
	glColor4ubv( (unsigned char *)&c );
	glVertex3dv( add( v1, scale( v1, m, t1 ), p ) );
	glVertex3dv( add( v2, scale( v2, m, t2 ), p ) );
	glEnd();
}


void DrawBubbles( PBUBBLE bubble, Image image, int x, int y, CDATA c )
{
	//BlotImageAlpha( image, l.shadow, x+20, y+20, ALPHA_TRANSPARENT_INVERT+128 );

   //BlotScaledImageSizedToShadedAlpha( image, l.shaded, x, y, 150, 150, 1, c );
   //BlotScaledImageSizedToAlpha( image, l.cover, x, y, 150, 150, ALPHA_TRANSPARENT );
   //BlotImageAlpha( image, l.cover, x+0, y+0, ALPHA_TRANSPARENT );
	//BlotImageAlphaShaded( image, l.shaded, x+0, y+0, ALPHA_TRANSPARENT, c );

	_try
	{

	{
		float x_size, y_size, y_size2;
		/*
		 * only a portion of the image is actually used, the rest is filled with blank space
		 *
		 */
		x_size = 1.0;//(double)image->width / (double)image->width;
		y_size = 0.0;//1.0 - (double)image->height / (double)image->height;
		y_size2 = 1.0;// - (double)hVideo->pAppImage->height / (double)hVideo->pImage->height;
		// Front Face
		//glColor4ub( 255,120,32,192 );
#define scale 128.0f
#define depth 10.0f
		if(0)
		{
			glBindTexture(GL_TEXTURE_2D, l.gl_cover);				// Select Our Texture
			glBegin(GL_QUADS);
			glColor4ub( 255,255,255,220 );
			glTexCoord2f(0.0f, y_size); glVertex3f(x+-scale,y+ -scale,  depth-1);	// Bottom Left Of The Texture and Quad
			glTexCoord2f(x_size, y_size); glVertex3f( x+scale,y+ -scale,  depth-1);	// Bottom Right Of The Texture and Quad
			glTexCoord2f(x_size, y_size2); glVertex3f(x+ scale, y+ scale,  depth-1);	// Top Right Of The Texture and Quad
			glTexCoord2f(0.0f, y_size2); glVertex3f(x+-scale,  y+scale,  depth-1);	// Top Left Of The Texture and Quad
			glEnd();
		}

		{
			glBindTexture(GL_TEXTURE_2D, l.gl_shaded);				// Select Our Texture
			glBegin(GL_QUADS);
			glColor4ubv( (P_8)&c );
			//lprintf( "Texture size is %g,%g", x_size, y_size );
			glTexCoord2f(0.0f, y_size); glVertex3f(x+-scale,y+ -scale,  depth);	// Bottom Left Of The Texture and Quad
			glTexCoord2f(x_size, y_size); glVertex3f( x+scale,y+ -scale,  depth);	// Bottom Right Of The Texture and Quad
			glTexCoord2f(x_size, y_size2); glVertex3f(x+ scale, y+ scale,  depth);	// Top Right Of The Texture and Quad
			glTexCoord2f(0.0f, y_size2); glVertex3f(x+-scale,  y+scale,  depth);	// Top Left Of The Texture and Quad


		//lprintf( "quads.." );
		// Back Face
		glEnd();
		}

	}
	{
		INDEX idx;
		PBUBBLE check;
		// unbind the texture, so we get the actual output of the line...
      // otherwise it's all fucked.
		glBindTexture(GL_TEXTURE_2D, 0);				// Select Our Texture
		LIST_FORALL( bubble->near_bubbles, idx, PBUBBLE, check )
		{

			glBegin( GL_LINE_STRIP );
			glColor4ub( 255,255,255,255);
			glVertex3f( (float)check->x, (float)check->y, depth/2 );
			glVertex3f( (float)bubble->x, (float)bubble->y, depth/2 );
         glEnd();
		}
	}

   		}
		_except( EXCEPTION_EXECUTE_HANDLER )
		{
            DebugBreak();
			lprintf( "Caught exception in video output window" );
			;
		}

}

void DrawAllBubbles( Image image )
{
	INDEX idx;
	PBUBBLE bubble;
   //lprintf( "..." );
	LIST_FORALL( l.bubbles, idx, PBUBBLE, bubble )
	{
      ChooseColorDest( bubble );
      DrawBubbles( bubble, image, bubble->x, bubble->y, bubble->current );
	}
   //lprintf( "...2" );
}


void BeginVisPersp( void )
{
	//if( mode != MODE_PERSP )
	{
		//mode = MODE_PERSP;
		glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		glLoadIdentity();									// Reset The Projection Matrix
		// Calculate The Aspect Ratio Of The Window
		glOrtho( -10, l.w+10, -10, l.h+10, -100, 100 );
		//gluPerspective(45.0f,1.0f,0.1f,10000.0f);
		glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
		glLoadIdentity();									// Reset The Modelview Matrix
	}
}

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
//	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
//   glClearAlpha(

	//glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	//glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	glClear(GL_COLOR_BUFFER_BIT
			  | GL_DEPTH_BUFFER_BIT
			 );	// Clear Screen And Depth Buffer
	BeginVisPersp();

   glEnable( GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   //glLineWidth( 5 );
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//   glDisable(GL_COLOR_MATERIAL);
//   glEnable(GL_ALPHA_TEST);
//  	glColor4f(1.0f,1.0f,1.0f,0.5f);			// Full Brightness, 50% Alpha ( NEW )
//	glBlendFunc(GL_SRC_ALPHA,GL_ONE);		// Blending Function For Translucency Based On Source Alpha Value ( NEW )

	return TRUE;										// Initialization Went OK
}


void GenTexture( unsigned int *gl, Image img )
{
	glGenTextures(1, gl);					// Create The Texture
	{
		int err = glGetError();
		if( err )
		{
			DebugBreak();
			lprintf( "Error is %d", err );
		}
	}
	glBindTexture(GL_TEXTURE_2D, (*gl));
	{
		int err = glGetError();
		if( err )
		{
			DebugBreak();
			lprintf( "Error is %d", err );
		}
	}
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0
					, GL_RGBA
					, GL_UNSIGNED_BYTE
					, img->image );
	{
		int err = glGetError();
		if( err )
		{
			DebugBreak();
			lprintf( "Error is %d", err );
		}
	}

}

void CPROC UpdateImage( PTRSZVAL psv, PRENDERER renderer )
{
	Image image = GetDisplayImage( renderer );

	if( !l.flags.bFirstPaintDone )
		l.flags.bFirstPaintDone = 1;
	else if( !l.flags.bSecondPaintDone )
	{

		l.flags.bSecondPaintDone = 1;
	}
	else

	{
   InitGL();

	//ClearImage( image );
   DrawAllBubbles( image );
//	DrawBubbles( image, 300, 100, 0 );
  // DrawBubbles( image, 0, 0, 1 );
	//DrawBubbles( image, 100, 300, 2 );
	//UpdateDisplay( renderer );



   // render to a hidden, back buffer, only.
	//glFlush();

   // read the buffer out to display
//   glReadPixels();
	//lprintf( "...3" );

   // then update the layered display.
   //UpdateDisplay( l.render );
   //lprintf( "...4" );
	}
}

void CPROC Ticker( PTRSZVAL psv )
{
   MoveBubbles();
   Redraw( l.render );
}

int main( int argc, char **argv )
{
	int bubbles;
	int x, y, w, h;

	lprintf( "Usage: %s <x> <y> <width> <height> <bubble count> <bubble scale (1000=1:1)>", argv[0] );

	x = argc > 1?atoi(argv[1]):0;
	y = argc > 2?atoi(argv[2]):0;
	w = argc > 3?atoi(argv[3]):0;
	h = argc > 4?atoi(argv[4]):0;

	//l.web = CreateSpaceWeb();

	bubbles = argc > 5?atoi(argv[5]):0;

	l.bubble_scale = argc > 6?atoi(argv[6]):0;

	l.shadow = LoadImageFile( "117.png" );
	l.cover = LoadImageFile( "123.png" );
	l.shaded = LoadImageFile( "121.png" );
	{
		GetDisplaySize( &l.w, &l.h );
		if( w )
			l.w = w;
		if( h )
         l.h = h;
		l.render = OpenDisplaySizedAt( 0
												| DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS
												| DISPLAY_ATTRIBUTE_NO_MOUSE
												|DISPLAY_ATTRIBUTE_CHILD
											  // |DISPLAY_ATTRIBUTE_LAYERED
											  , l.w, l.h, x, y );

		EnableOpenGL( l.render );

		SetActiveGLDisplay( l.render );
      GenTexture( &l.gl_shadow, l.shadow );
      GenTexture( &l.gl_cover, l.cover );
		GenTexture( &l.gl_shaded, l.shaded );
		SetActiveGLDisplay( NULL );


		SetRedrawHandler( l.render, UpdateImage, 0 );
      //MakeTopmost( l.render );
		UpdateDisplay( l.render );
      //MakeTopmost( l.render );

		{
			int n;
			int x;
			if( bubbles == 0 )
            bubbles = MAX_BUBBLES;
			for( n = 0; n < bubbles; n++ )

			{
				PBUBBLE bubble = New( BUBBLE );
				MemSet( bubble, 0, sizeof( BUBBLE ) );
				bubble->x = n * 160;
				while( bubble->x > 1650 )
				{
					bubble->x -= 1650;
               bubble->y += 160;
				}
            RelateBubble( bubble );
				AddLink( &l.bubbles, bubble );
			}
		}


      AddTimer( 1000/10, Ticker, 0 );
		while( DisplayIsValid( l.render ) )
         WakeableSleep( 1000 );
	}
   return 0;
}

