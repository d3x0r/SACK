#define PSI_HTML5_CANVAS_SOURCE

#include <stdhdrs.h>

#include <http.h>

#include "../controlstruc.h"
#include <html5.canvas.h>


PSI_HTML5_CANVAS_NAMESPACE

struct html5_canvas
{
	// this is a collector for the HTML5 canvas render system
	// it should also provide support for defining event callbacks.
	// need to poke at a websocket service with a javascript connection and see
	// that would be HTML parsing... (http.h?)

   PSI_CONTROL root;
};


HTML5Canvas CreateCanvas( PSI_CONTROL root_control )
//void RenderToCanvas( PSI_CONTROL pc )
{
	HTML5Canvas canvas = New( struct html5_canvas );
	MemSet( canvas, 0, sizeof( struct html5_canvas ) );

   canvas->root = root_control;

   return canvas;
}

static void InvokeControlWrite( HTML5Canvas canvas, PSI_CONTROL pc )
{
	void (CPROC *OnHandler)(HTML5Canvas,PSI_CONTROL);
	PCLASSROOT data = NULL;
	PCLASSROOT event_root = GetClassRootEx( pc->class_root, WIDE( "draw_to_canvas" ) );
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( event_root, &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		OnHandler = GetRegisteredProcedureExx( data,(CTEXTSTR)NULL,void,name,(HTML5Canvas,PSI_CONTROL));
		if( OnHandler )
		{
			OnHandler( canvas, pc );
		}
	}
}


static void EmitCanvas( HTML5Canvas canvas, FILE *output )
{

}

LOGICAL WriteCanvas( HTML5Canvas canvas, CTEXTSTR filename )
{
	PSI_CONTROL current;
	PSI_CONTROL next;
	PLINKSTACK backtrace = CreateLinkStack();
	for( current = canvas->root; current; current = next )
	{

		InvokeControlWrite( canvas, current );

		if( current->child )
		{
         PushLink( &backtrace, current->next );
		}
		next = current->next;
		if( !next )
         next = (PSI_CONTROL)PopLink( &backtrace );
	}
	{
		FILE *output = sack_fopen( 0, filename, WIDE("wt") );
		EmitCanvas( canvas, output );
		fclose( output );
	}
	return 0;
}


void AddResourceToCanvas( HTML5Canvas canvas, CTEXTSTR resource_name )
{

}


void h5printf( HTML5Canvas canvas, CTEXTSTR format, ... )
{

}

PSI_HTML5_CANVAS_NAMESPACE_END

