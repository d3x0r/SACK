

#define RENDER_SOURCE
#define USE_IMAGE_INTERFACE l.image_interface
#include <stdhdrs.h>
#include <configscript.h>
#include <render.h>
#include <spacetree.h>
#include <network.h>

#include <space.h>
#include <commands.h>
#include "../../psilib/controlstruc.h"

IMAGE_NAMESPACE
extern LOGICAL PngImageFile ( Image pImage, _8 ** buf, int *size);
IMAGE_NAMESPACE_END

RENDER_NAMESPACE

struct HVIDEO_tag {
	TEXTSTR name;
   CTEXTSTR alias;
	Image image;
   Image sliding_window; // used to send a sub-portion of the image
	S_32 x, y; // unused?
	// Sprite sprite; // at some point movies should be streamable?
	MouseCallback mouse;
	PTRSZVAL psvMouse;
	RedrawCallback redraw;
	PTRSZVAL psvRedraw;
   PSPACENODE images; // tree of image pointers...
};

typedef struct my_datapath MY_DATAPATH;
typedef struct my_datapath *PMY_DATAPATH;
struct my_datapath {
   DATAPATH common;
};


static struct {
	RENDER_INTERFACE di;
   PIMAGE_INTERFACE image_interface;
	PCLIENT pc_server;
	PTREEROOT image_root;
	INDEX iRender; // the plugin extension index registered for vvidlib(render.nex)
	INDEX iNetwork;
   PRENDERER last_created_renderer; // set application title affects this
} l;

struct network_state
{
   PCLIENT pc;
	PTEXT block;
   PTEXT name; // entities have a name...
	PENTITY entity; // need the woner
   PSENTIENT sentience; // need to be aware ...
   PMY_DATAPATH http_parser;
};


static int CPROC MyStrCmp( PTRSZVAL psv1, PTRSZVAL psv2 )
{
   return StrCmp( (char*)psv1, (*(char**)psv2) );
}

void AddImage( PRENDERER renderer )
{
	if( !l.image_root )
      l.image_root = CreateBinaryTreeEx( MyStrCmp, NULL );
   AddBinaryNode( l.image_root, renderer, (PTRSZVAL)&renderer->name );
}

static CTEXTSTR alias_found;

PTRSZVAL CPROC IsAlias( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CTEXTSTR, name );
	PARAM( args, CTEXTSTR, alias );
   PRENDERER renderer = (PRENDERER)psv;
	if( strcmp( renderer->name, name ) == 0 )
	{
		alias_found = StrDup( alias );
      //EndConfigurationHandler();
	}
   return psv;
}

void GetImageAlias( PRENDERER renderer )
{
	PCONFIG_HANDLER pch_aliases;
   PTRSZVAL result;
   pch_aliases = CreateConfigurationHandler();
	AddConfigurationMethod( pch_aliases, "<alias name=\"%m\" aka=\"%m\"></alias>", IsAlias );
	result = ProcessConfigurationFile( pch_aliases, "window_aliases", (PTRSZVAL)renderer );
	if( alias_found )
	{
		renderer->alias = alias_found;
      alias_found = NULL;
	}
}

void _BuildSpaceTree( PSPACENODE *tree, int level, _32 x_origin, _32 y_origin, PSI_CONTROL pc )
{
	//while( image )
	{
		SPACEPOINT min, max;
		// min and max are INCLUSIVE of
		// all data.
		if( pc->child )
		{
			_BuildSpaceTree( tree, level+1
								, (level)?x_origin + pc->surface_rect.x + pc->rect.x:x_origin
								, (level)?y_origin + pc->surface_rect.y + pc->rect.y:y_origin, pc->child );
		}
		if( pc->next )
         _BuildSpaceTree( tree, level+1, x_origin, y_origin, pc->next );
		//if( !mag->common.flags.invisible )
		{
			min[0] = level?pc->rect.x + x_origin:0;
			min[1] = level?pc->rect.y + y_origin:0;
			max[0] = min[0] + pc->rect.width - 1;
			max[1] = min[1] + pc->rect.height - 1;
         lprintf( "Adding node for image %p %d,%d %d,%d", pc, min[0], min[1], max[0], max[1] );
         /*
			if( min[0] < 0 )
				min[0] = 0;
			if( min[1] < 0 )
				min[1] = 0;
			if( max[0] >= 1024 )
				max[1] = g.width - 1;
			if( max[1] >= g.height )
			max[1] = g.height - 1;
         */
			// only add tracked spaces which are on the screen.
			// nothing that overflows the edge will count.
			// this will provide auto clipping for image functions done through
						 // thirs.
//#ifdef DEBUG_DIRTY_RECT
			lprintf( WIDE("Node is %p (%") _32fs WIDE(",%") _32fs WIDE(")-(%") _32f WIDE(",%") _32f WIDE(")"), pc, min[0], min[1], max[0], max[1] );
//#endif
	   	AddSpaceNode( tree, pc, min, max );
		}
      //image = image->next;
	}
}

void BuildSpaceTree( PRENDERER renderer )
{
	PSI_CONTROL pc = GetFrameFromRenderer( renderer );
	DeleteSpaceTree( &renderer->images );
   _BuildSpaceTree( &renderer->images, 0, 0, 0, pc );
}

#undef RENDER_PROC_PTR
#define RENDER_PROC_PTR( type, name ) l.di._##name = (type (CPROC*)

int DoNothing( void )
{
   return 0;
}

TEXTCHAR CheapOK[] = "HTTP-1.1 200 OK\r\n"
   ;

void SendWindowList( struct network_state *state )
{
	PVARTEXT pvt = VarTextCreate();

	PRENDERER renderer;
	vtprintf( pvt, "<HTML><HEAD></HEAD><BODY>" );
	vtprintf( pvt, "List of windows: <br>" );
	for( renderer = (PRENDERER)GetLeastNode( l.image_root );
		 renderer;
		  renderer = (PRENDERER)GetGreaterNode( l.image_root )
		)
	{
		vtprintf( pvt, "<a href=\"wnd@%p\">%s</a><br>\n", renderer, renderer->name?renderer->name:"Unnamed Window" );
	}
	vtprintf( pvt, "Done.</BODY></HTML>" );
	{
		PTEXT out = VarTextGet( pvt );
		{
			char buf[256];
			int chars;
			SendTCP( state->pc, CheapOK, sizeof( CheapOK ) - 1);
			chars = snprintf( buf, sizeof( buf )
								 , "Content-Type: text/html\r\n"
								  "Content-Length: %d\r\n\r\n", GetTextSize( out ) );
			SendTCP( state->pc, buf, chars );
		}
		SendTCP( state->pc, GetText( out ), GetTextSize( out ) );
		LineRelease( out );
	}
	VarTextDestroy( &pvt );
}

void AddMyData( PTRSZVAL psv, PVARTEXT pvt, POINTER node_data, PSPACEPOINT min, PSPACEPOINT max )
{
   PRENDERER renderer = (PRENDERER)psv;
	//Image image = node_data;
	vtprintf( pvt, "<img src=\"wnd@%p,%p,%d,%d,%d,%d\">", renderer, node_data, min[0], min[1], max[0]-min[0]+1, max[1]-min[1]+1 );
}

void SendImagePage( struct network_state *state, PRENDERER renderer )
{
	PVARTEXT pvt = VarTextCreate();

	vtprintf( pvt, "<HTML><HEAD></HEAD><BODY>" );
	//vtprintf( pvt, "<table>\n" );
	vtprintf( pvt, "<a href=\"clk@%p\"><img vspace=\"0\" hspace=\"0\" border=\"0\" src=\"wnd@%px\" ismap></a>", renderer, renderer );
   vtprintf( pvt, "</area></map>\n" );
   //vtprintf( pvt, "</table>\n" );

   /*
	BuildSpaceTree( renderer );

	OutputHTMLSpaceTable( renderer->images
							  , pvt
							  , AddMyData, (PTRSZVAL)renderer );
							  */
	vtprintf( pvt, "</BODY></HTML>" );
	{
      int chars;
		PTEXT out = VarTextGet( pvt );
		TEXTCHAR buf[256];
		{
			FILE *outf = fopen( "blah.txt", "wt" );
			fprintf( outf, "%s", GetText( out ) );
         fclose( outf );
		}
		SendTCP( state->pc, CheapOK, sizeof( CheapOK ) - 1);
		chars = snprintf( buf, sizeof( buf )
							 , "Content-Type: text/html\r\n"
							  "Content-Length: %d\r\n\r\n", GetTextSize( out ) );
      SendTCP( state->pc, buf, chars );
		SendTCP( state->pc, GetText( out ), GetTextSize( out ) );
		LineRelease( out );
	}
	VarTextDestroy( &pvt );
}


int MyCommand( PSENTIENT ps, PTEXT params )
{
	// not sure I care about any of the parameters...
	// but here I can reference the names of objects...
	PTEXT value = GetVariable( ps, "page" );
	CTEXTSTR page = GetText( value );
	PVARTEXT pvt = VarTextCreate();
   int sent = 0;
	struct network_state *state = (struct network_state*)
		GetLink( &ps->Current->pPlugin, l.iNetwork );
   DoCommandf( ps, "/vars\n" );
	DoCommandf( ps, "/vvars\n" );
	// groovy .
	// how to get from a ps to a .... renderer/tcp client to get the image to send
	recheck_page:
   if( value && page )
	{
		int click = 0;
      int get_image = 0;
      Image image = NULL;
		PRENDERER window;
		{
			int x, y;
			if( page && strncmp( page, "clk@", 4 ) == 0 )
			{
				PTEXT CGI = GetIndirect( GetVariable( ps, "CGI" ) );
				if( CGI )
				{
					x = atoi( GetText( CGI ) );
					CGI = NEXTLINE( CGI );
					if( CGI && GetText( CGI )[0] == ',' )
					{
						CGI = NEXTLINE( CGI );
						if( CGI )
						{
							y = atoi( GetText( CGI ) );
							click = 1;
						}
					}
				}
			}
			else
				if( page && strncmp( page, "wnd@", 4 ) == 0 )
				{
					get_image = 1;
				}
			if( click || get_image )
			{
				sscanf( page+4, "%p", &window );
				{
					PRENDERER renderer;
					for( renderer = (PRENDERER)GetLeastNode( l.image_root );
						 renderer;
						  renderer = (PRENDERER)GetGreaterNode( l.image_root )
						)
					{
						if( renderer == (PRENDERER)window )
                     break;
					}
               // renderer MAY be null,...
					window = renderer;
				}
				if( click )
				{
					if( window )
					{
						if( window->mouse )
						{
                     char tmpbuf[32];
                     lprintf( "Generate window mouse event %d,%d", x, y );
                     window->mouse( window->psvMouse, x, y, MK_LBUTTON );
							window->mouse( window->psvMouse, x, y, 0 );
							snprintf( tmpbuf, sizeof( tmpbuf ),"wnd@%p", window );
							page = tmpbuf;
                     goto recheck_page;
						}
					}
				}

				{
					if( window && !(page[12]) )
					{
						SendImagePage( state, window );
						sent = 1;
						window = NULL;
					}
					else if( window )
					{
						if( page[12] == ',' )
						{
							POINTER p;
							SPACEPOINT min;
							SPACEPOINT max;
							//DebugBreak();
							sscanf( page+13, "%p,%d,%d,%d,%d", &p, min+0, min+1, max+0, max+1 );
							image = MakeSubImage( window->image, min[0], min[1], max[0], max[1] );
							//DebugBreak();
						}
						else
						{
                  // invalid requestor...
						}
					}
				}
			}
			else
			{
				window = (PRENDERER)FindInBinaryTree( l.image_root, (PTRSZVAL)GetText( value ) );
			}
		}
      if( !window )
		{
         if( !sent )
				SendWindowList( state );
		}
		else
		{
			POINTER data = NULL;
         int length;
			if( !image && window->redraw )
			{
            //DebugBreak();
				window->redraw( window->psvRedraw, window );
			}
			PngImageFile( image?image:window->image, (P_8*)&data, &length );
			if( image )
            UnmakeImageFile( image );
			{
				char buf[256];
            int chars;
				SendTCP( state->pc, CheapOK, sizeof( CheapOK ) - 1);
				chars = snprintf( buf, sizeof( buf )
									 , "Content-Type: image/png\r\n"
									  "Content-Length: %d\r\n\r\n", length );
				SendTCP( state->pc, buf, chars );
			}
			SendTCP( state->pc, data, length );
			Release( data );
		}
	}
	else
	{
		SendWindowList( state );
	}
	RemoveClient( state->pc );


   return 0;
}

PRENDERER OpenDisplayAboveSizedAt( _32 attributes, _32 w, _32 h, S_32 x, S_32 y, PRENDERER parent )
{
	PRENDERER renderer = New( RENDERER );
	// parent = blah
   l.last_created_renderer = renderer;
	renderer->image = MakeImageFile( w, h );
	ClearImage( renderer->image ); // transparentize it.
   renderer->images = NULL; // no tree.
	renderer->x = x; // unused?
	renderer->y = y; // why care?
	renderer->name = NULL;
	renderer->alias = NULL;
	AddImage( renderer );
   return renderer;
}

PRENDERER OpenDisplaySizedAt( _32 attributes, _32 w, _32 h, S_32 x, S_32 y )
{
   return OpenDisplayAboveSizedAt( attributes, w, h, x, y, NULL );
}

void UpdateDisplayPortionEx( PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h DBG_PASS )
{
	if( renderer->redraw )
	{
		//renderer->redraw( renderer->psvRedraw, renderer );
	}
}

void CloseDisplay( PRENDERER renderer )
{
	PRENDERER test;
	for( test = (PRENDERER)GetLeastNode( l.image_root );
		 test;
		  test = (PRENDERER)GetGreaterNode( l.image_root )
		)
	{
		if( test == (PRENDERER)renderer )
			break;
	}
	if( test )
	{
      RemoveBinaryNode( l.image_root, test, (PTRSZVAL)renderer->name );
	}
}

void UpdateDisplayEx( PRENDERER renderer DBG_PASS )
{
   UpdateDisplayPortion( renderer, 0, 0, 0, 0 );
}

Image GetDisplayImage( PRENDERER renderer )
{
   return renderer->image;
}

void CPROC ReadComplete( PCLIENT pc, POINTER buffer, int size )
{
   struct network_state *state = (struct network_state *)GetNetworkLong( pc, 0 );
	if( !buffer )
	{
	}
	else
	{
      // set last byte as a NUL.
		GetText( state->block )[size] = 0;
      // set the block's actual size.
      SetTextSize( state->block, size );
		EnqueLink( &state->http_parser->common.Input, state->block );
      WakeAThread( NULL ); // process block.
		state->block = SegCreate( 4096 ); // new block... old one is unusable anymore... it's reliqnuished at the enque, please.
	}
   ReadTCP( pc, GetText( state->block ), GetTextSize( state->block ) - 1 );
}

void CPROC ObjectDestroy( PENTITY pe )
{
	struct network_state *state = (struct network_state*)
		GetLink( &pe->pPlugin, l.iNetwork );
   RemoveClient( state->pc );
	LineRelease( state->block );
}

void CPROC OnClose( PCLIENT pc )
{
	struct network_state *state = (struct network_state *)GetNetworkLong( pc, 0 );
   DestroyEntity( state->entity );
}

void CPROC Connected( PCLIENT pc_server, PCLIENT pc_client )
{
	struct network_state *state = New( struct network_state );
	SetNetworkLong( pc_client, 0, (PTRSZVAL)state );
   state->pc = pc_client;
	state->block = SegCreate( 4096 );
   state->name = SegCreateFromText( "http processor" );
	state->entity = CreateEntityIn( NULL, state->name );
	state->sentience = CreateAwareness( state->entity );
	SetLink( &state->entity->pPlugin, l.iNetwork, state );
   SetLink( &state->entity->pDestroy, l.iNetwork, ObjectDestroy );
   state->http_parser = CreateDataPath( &state->sentience->Data, MY_DATAPATH );
	{
		//DECLTEXT( device, "http" );
      DoCommandf( state->sentience, "/open http http" );
		//OpenDevice( &state->http_parser, state->sentience, (PTEXT)&device, NULL );
	}
	{
		// and then what?
		// need to get the behavioral invokation to launch code here...
		DoCommandf( state->sentience, "/on http_request" );
      // invalid command results in 'macro disappeared...' wonder what that's about.
      DoCommandf( state->sentience, "/vvidlib_request" );
		DoCommandf( state->sentience, "/endmac" );
	}
   UnlockAwareness( state->sentience );
   SetReadCallback( pc_client, ReadComplete );
}


void SetMouseHandler( PRENDERER renderer, MouseCallback mouse, PTRSZVAL psvMouse )
{
	// invoke this so that draws can be retriggered...
   // referesh has to happen on the page.
	renderer->mouse = mouse;
   renderer->psvMouse = psvMouse;
}

void SetRedrawHandler( PRENDERER renderer, RedrawCallback redraw, PTRSZVAL psvRedraw )
{
	renderer->redraw = redraw;
   renderer->psvRedraw = psvRedraw;
}

void SizeDisplay( PRENDERER renderer, _32 w, _32 h )
{
   //DebugBreak();
	ResizeImage( renderer->image, w, h );
   ClearImage( renderer->image );
	//if( renderer->redraw )
	//{
	//	renderer->redraw( renderer->psvRedraw, renderer );
	//}
}



void SizeDisplayRel( PRENDERER renderer, S_32 w, S_32 h )
{
	ResizeImage( renderer->image
				  , renderer->image->width + w
				  , renderer->image->height + h );
   ClearImage( renderer->image );
	//if( renderer->redraw )
	//{
	//	renderer->redraw( renderer->psvRedraw, renderer );
	//}
}

void MoveSizeDisplayRel( PRENDERER renderer, S_32 x, S_32 y, S_32 w, S_32 h )
{
   SizeDisplayRel( renderer, w, h );
}

void MoveSizeDisplay( PRENDERER renderer, S_32 x, S_32 y, S_32 w, S_32 h )
{
   SizeDisplay( renderer, w, h );
}


void GetDisplaySize( _32 *w, _32 *h )
{
	if( w )
		(*w) = 1024;
	if( h )
      (*h) = 768;
}

#undef GetDisplayInterface
#undef DropDisplayInterface

RENDER_PROC (POINTER, GetDisplayInterface) (void)
{
   //InitDisplay();
   return (POINTER)&l.di;
}

RENDER_PROC (void, DropDisplayInterface) (POINTER p)
{
}

void CPROC SetApplicationTitle( CTEXTSTR title )
{
	if( l.last_created_renderer )
	{
      l.last_created_renderer->name = StrDup( title );
	}
}


PRELOAD( InitMe )
{
//DebugBreak();
   l.image_interface = (PIMAGE_INTERFACE)GetInterface( "image" );
   NetworkWait( NULL, 1024, 16 );
	l.pc_server = OpenTCPListenerEx( 2580, Connected );

   l.iRender = RegisterExtension( "vvidlib" );
   l.iNetwork = RegisterExtension( "vvidlib_network" );
	RegisterRoutine( "vvidlib_request", "Invoke this command on http.request(NULL)", MyCommand );
	RENDER_PROC_PTR( int , InitDisplay) (void) )DoNothing; // does not HAVE to be called but may

	RENDER_PROC_PTR( void , SetApplicationTitle) (const char *title ) )SetApplicationTitle;
	RENDER_PROC_PTR( void , SetApplicationIcon)  (Image Icon) )DoNothing; //
	RENDER_PROC_PTR( void , GetDisplaySize)      ( _32 *width, _32 *height ) )GetDisplaySize;
	RENDER_PROC_PTR( void , SetDisplaySize)      ( _32 width, _32 height ) )DoNothing;
	RENDER_PROC_PTR( int , ProcessDisplayMessages)      (void) )DoNothing;

	RENDER_PROC_PTR( PRENDERER , OpenDisplaySizedAt)     ( _32 attributes, _32 width, _32 height, S_32 x, S_32 y ) )OpenDisplaySizedAt;
	RENDER_PROC_PTR( PRENDERER , OpenDisplayAboveSizedAt)( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above ) )OpenDisplayAboveSizedAt;

	RENDER_PROC_PTR( void        , CloseDisplay) ( PRENDERER ) )CloseDisplay;

	RENDER_PROC_PTR( void, UpdateDisplayPortionEx) ( PRENDERER, S_32 x, S_32 y, _32 width, _32 height DBG_PASS ) )UpdateDisplayPortionEx;
	RENDER_PROC_PTR( void, UpdateDisplayEx)        ( PRENDERER DBG_PASS) )UpdateDisplayEx;


	RENDER_PROC_PTR( void, GetDisplayPosition)   ( PRENDERER, S_32 *x, S_32 *y, _32 *width, _32 *height ) )DoNothing;
	RENDER_PROC_PTR( void, MoveDisplay)          ( PRENDERER, S_32 x, S_32 y ) )DoNothing;
	RENDER_PROC_PTR( void, MoveDisplayRel)       ( PRENDERER, S_32 delx, S_32 dely ) )DoNothing;
	RENDER_PROC_PTR( void, SizeDisplay)          ( PRENDERER, _32 w, _32 h ) )SizeDisplay;
	RENDER_PROC_PTR( void, SizeDisplayRel)       ( PRENDERER, S_32 delw, S_32 delh ) )SizeDisplay;
	RENDER_PROC_PTR( void, MoveSizeDisplayRel )  ( PRENDERER hVideo
																, S_32 delx, S_32 dely
																, S_32 delw, S_32 delh ) )DoNothing;
	RENDER_PROC_PTR( void, PutDisplayAbove)      ( PRENDERER, PRENDERER ) )DoNothing; // this that - put this above that

	RENDER_PROC_PTR( Image, GetDisplayImage)     ( PRENDERER ) )GetDisplayImage;

	RENDER_PROC_PTR( void, SetCloseHandler)      ( PRENDERER, CloseCallback, PTRSZVAL ) )DoNothing;
	RENDER_PROC_PTR( void, SetMouseHandler)      ( PRENDERER, MouseCallback, PTRSZVAL ) )SetMouseHandler;
	RENDER_PROC_PTR( void, SetRedrawHandler)     ( PRENDERER, RedrawCallback, PTRSZVAL ) )SetRedrawHandler;
	RENDER_PROC_PTR( void, SetKeyboardHandler)   ( PRENDERER, KeyProc, PTRSZVAL ) )DoNothing;
	RENDER_PROC_PTR( void, SetLoseFocusHandler)  ( PRENDERER, LoseFocusCallback, PTRSZVAL ) )DoNothing;
	RENDER_PROC_PTR( void, SetDefaultHandler)    ( PRENDERER, GeneralCallback, PTRSZVAL ) )DoNothing;

	RENDER_PROC_PTR( void, GetMousePosition)     ( S_32 *x, S_32 *y ) )DoNothing;
	RENDER_PROC_PTR( void, SetMousePosition)     ( PRENDERER, S_32 x, S_32 y ) )DoNothing;

	RENDER_PROC_PTR( LOGICAL, HasFocus)          ( PRENDERER ) )DoNothing;

	RENDER_PROC_PTR( int, SendActiveMessage)     ( PRENDERER dest, PACTIVEMESSAGE msg ) )DoNothing;
	RENDER_PROC_PTR( PACTIVEMESSAGE , CreateActiveMessage) ( int ID, int size, ... ) )DoNothing;

	RENDER_PROC_PTR( char, GetKeyText)           ( int key ) )DoNothing;
	RENDER_PROC_PTR( _32, IsKeyDown )        ( PRENDERER display, int key ) )DoNothing;
	RENDER_PROC_PTR( _32, KeyDown )         ( PRENDERER display, int key ) )DoNothing;
	RENDER_PROC_PTR( LOGICAL, DisplayIsValid )  ( PRENDERER display ) )DoNothing;
	// own==0 release else mouse owned.
	RENDER_PROC_PTR( void, OwnMouseEx )            ( PRENDERER display, _32 Own DBG_PASS) )DoNothing;
	RENDER_PROC_PTR( int, BeginCalibration )       ( _32 points ) )DoNothing;
	RENDER_PROC_PTR( void, SyncRender )            ( PRENDERER pDisplay ) )DoNothing;
	RENDER_PROC_PTR( int, EnableOpenGL )           ( PRENDERER hVideo ) )DoNothing;
	RENDER_PROC_PTR( int, SetActiveGLDisplay )     ( PRENDERER hDisplay ) )DoNothing;
	//IsKeyDown
	//KeyDown
	//KeyDouble
	//GetKeyText
	RENDER_PROC_PTR( void, MoveSizeDisplay )( PRENDERER hVideo
														 , S_32 x, S_32 y
														 , S_32 w, S_32 h ) )DoNothing;
	RENDER_PROC_PTR( void, MakeTopmost )    ( PRENDERER hVideo ) )DoNothing;
	RENDER_PROC_PTR( void, HideDisplay )      ( PRENDERER hVideo ) )DoNothing;
	RENDER_PROC_PTR( void, RestoreDisplay )   ( PRENDERER hVideo ) )DoNothing;

	RENDER_PROC_PTR( void, ForceDisplayFocus )( PRENDERER display ) )DoNothing;
	RENDER_PROC_PTR( void, ForceDisplayFront )( PRENDERER display ) )DoNothing;
	RENDER_PROC_PTR( void, ForceDisplayBack )( PRENDERER display ) )DoNothing;

	RENDER_PROC_PTR( int, BindEventToKey )( PRENDERER pRenderer, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv ) )DoNothing;
	RENDER_PROC_PTR( int, UnbindKey )( PRENDERER pRenderer, _32 scancode, _32 modifier ) )DoNothing;
	RENDER_PROC_PTR( int, IsTopmost )( PRENDERER hVideo ) )DoNothing;
	RENDER_PROC_PTR( void, OkaySyncRender )            ( void ) )DoNothing;
	RENDER_PROC_PTR( int, IsTouchDisplay )( void ) )DoNothing;
	RENDER_PROC_PTR( void , GetMouseState )        ( S_32 *x, S_32 *y, _32 *b ) )DoNothing;
	RENDER_PROC_PTR ( PSPRITE_METHOD, EnableSpriteMethod )(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv ) )DoNothing;

   RegisterInterface( 
//#ifdef SACK_BAG_EXPORTS  // symbol defined by visual studio sack_bag.vcproj
	   WIDE("virtual_http_render")
//#else
//	   WIDE("video")
//#endif
	   , GetDisplayInterface, DropDisplayInterface );

}

RENDER_NAMESPACE_END

