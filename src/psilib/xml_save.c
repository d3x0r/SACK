#include <stdhdrs.h>
#include <sharemem.h>
#include <../src/genx/genx.h>
#include "controlstruc.h"
#include <psi.h>
#include <filedotnet.h>
PSI_XML_NAMESPACE

typedef struct context_tag
{
   // name of the file we're building..
	TEXTCHAR *name;
	PVARTEXT vt;
	_32 nChildren;
	PSI_CONTROL pc; // current control...

	genxWriter w;

	genxElement eControl;

	genxAttribute aPosition;
	genxAttribute aSize;
	genxAttribute aBorder;
	genxAttribute aChildren;
	genxAttribute aType, aID, aIDName, aCaption, aPrivate, aExtraPrivate;
	genxAttribute aEdit;


} XML_CONTEXT, *PXML_CONTEXT;

typedef struct local_tag
{
	PDATASTACK contexts; // list of PXML_CONTEXTs
	PXML_CONTEXT current_context;
	//char *file;
	PVARTEXT current_vt;
} LOCAL;

static LOCAL l;

void WriteCommonData( PSI_CONTROL pc )
{
	PVARTEXT out = VarTextCreateExx( 10000, 5000 );
	for( ; pc; pc = pc->next )
	{
		TEXTCHAR buf[256];
		// next please.
		if( pc->flags.private_control || pc->flags.bAdoptedChild )
			continue;
		l.current_context->pc = pc;
		genxAddText(l.current_context->w, (constUtf8)"\n");
		genxStartElement( l.current_context->eControl );
		snprintf( buf, sizeof( buf ), PSI_ROOT_REGISTRY WIDE("/control/%d"), pc->nType );
		genxAddAttribute( l.current_context->aType, (constUtf8)GetRegisteredValue( buf, WIDE("Type") ) );
		snprintf( buf, sizeof( buf ), WIDE("%") _32f WIDE(",") WIDE("%") _32f, pc->original_rect.x, pc->original_rect.y );
		genxAddAttribute( l.current_context->aPosition, (constUtf8)buf );
		snprintf( buf, sizeof( buf ), WIDE("%") _32f WIDE(",") WIDE("%") _32f, pc->original_rect.width, pc->original_rect.height );
		genxAddAttribute( l.current_context->aSize, (constUtf8)buf );
		if( pc->flags.bSetBorderType )
		{
			snprintf( buf, sizeof( buf ), WIDE("%") _32fx WIDE(""), pc->BorderType );
			genxAddAttribute( l.current_context->aBorder, (constUtf8)buf );
		}

		// Let's not write the number of this ID anymore...
		//sprintf( buf, WIDE("%d"), pc->nID );
		//genxAddAttribute( l.current_context->aID, (constUtf8)buf );
		if( pc->pIDName )
		{
			PCLASSROOT pcr = GetClassRootEx( (PCLASSROOT)WIDE("psi/resources"), pc->pIDName );
			TEXTCHAR buffer[256];
			TEXTSTR skip;
			GetClassPath( buffer, 256, pcr );
			skip = buffer + 15;
			genxAddAttribute( l.current_context->aIDName, (constUtf8)skip );
		}
		if( pc->flags.bEditLoaded )
		{
			snprintf( buf, sizeof( buf ), WIDE("%d"), pc->flags.bNoEdit );
			genxAddAttribute( l.current_context->aEdit, (constUtf8)buf);
		}

		if( pc->caption.text )
			genxAddAttribute( l.current_context->aCaption, (constUtf8)GetText( pc->caption.text ) );


		// call the control's custom data stuff...
		// which should start another tag within the control?
		{
			int (CPROC *Save)(PCONTROL,PVARTEXT);
			TEXTCHAR id[32];
			PVARTEXT out = VarTextCreate();
			snprintf( id, sizeof( id ), PSI_ROOT_REGISTRY WIDE("/control/%d/rtti"), pc->nType );
			if( ( Save=GetRegisteredProcedure( id, int, save,(PSI_CONTROL,PVARTEXT)) ) )
			{
				PTEXT data;
				Save( pc, out );
				data = VarTextGet( out );
				if( data )
				{
					genxAddAttribute( l.current_context->aPrivate, (constUtf8)GetText( data ) );
					LineRelease( data );
				}
			}
			if( ( Save=GetRegisteredProcedure( id, int, extra save,(PSI_CONTROL,PVARTEXT)) ) )
			{
				PTEXT data;
				Save( pc, out );
				data = VarTextGet( out );
				if( data )
				{
					genxAddAttribute( l.current_context->aExtraPrivate, (constUtf8)GetText( data ) );
					LineRelease( data );
				}
			}
			VarTextDestroy( &out );
		}
		if( l.current_context->nChildren )
		{
			l.current_context->pc = pc;
			snprintf( buf, sizeof( buf ), WIDE("%") _32f WIDE(""), l.current_context->nChildren );
			genxAddAttribute( l.current_context->aChildren, (constUtf8)buf );
			l.current_context->nChildren = 0;
		}
		if( pc->child )
		{
			WriteCommonData( pc->child );
		}
		genxEndElement( l.current_context->w );
		//pc = pc->next;
	}
	VarTextDestroy( &out );
}

static genxStatus WriteBuffer( void *UserData, constUtf8 s )
{
	vtprintf( l.current_vt, WIDE("%s"), s );
	return GENX_SUCCESS;
}

static genxStatus WriteBufferBounded( void *UserData, constUtf8 s, constUtf8 end )
{
	vtprintf( l.current_vt, WIDE("%*.*s"), end-s, end-s, s );
	return GENX_SUCCESS;
}

static genxStatus Flush( void *UserData )
{
	return GENX_SUCCESS;
}

genxSender senderprocs = { WriteBuffer
								 , WriteBufferBounded
								 , Flush };

int SaveXMLFrame( PSI_CONTROL frame, CTEXTSTR file )
{
	//FILE *out = fopen( file, WIDE("wb") );
	XML_CONTEXT context;
	PXML_CONTEXT current_context;
	if( !file )
		file = frame->save_name;
	if( !file )
	{
		lprintf( WIDE("Failure to save XML Frame... no filename passed, no filename available.") );
		return 0;
	}
	MemSet( &context, 0, sizeof( context ) );
	if( !l.contexts )
      l.contexts = CreateDataStack( sizeof( context ) );
	if( !file && ( current_context= (PXML_CONTEXT)PeekData( &l.contexts ) ) )
	{
		TEXTCHAR *ext = strrchr( current_context->name, '.' );
		PTEXT text;
		context.vt = VarTextCreateExx( 10000, 5000 );
		if( !ext )
			ext = current_context->name + StrLen( current_context->name );
		current_context->nChildren++;
		vtprintf( context.vt, WIDE("%*.*s.%d-%d-child-%d%s")
				  , ext - current_context->name
				  , ext - current_context->name
				  , current_context->name
              , current_context->pc->nType
              , current_context->pc->nID
              , current_context->nChildren
				  , ext );
      text = VarTextGet( context.vt );
      context.name = StrDup( GetText( text ) );
		LineRelease( text );
	}
	else
	{
		if( !file )
		{
			lprintf( WIDE("NULL filename passed to save frame.  Aborting.") );
			return 0;
		}
		context.name = StrDup( file );
		context.vt = VarTextCreateExx( 10000, 5000 );
	}
	PushData( &l.contexts, &context );
	l.current_context = (PXML_CONTEXT)PeekData( &l.contexts );
	if( l.current_context )
		l.current_vt = l.current_context->vt;
	//if( out )
	{
		genxStatus status;
		//genxElement eFrame = genxDeclareElement( l.w, NULL, WIDE("frame"), &status );
		if( !l.current_context->w )
		{
			l.current_context->w = genxNew(NULL,NULL,NULL);
			l.current_context->eControl = genxDeclareElement( l.current_context->w, NULL, (constUtf8)"control", &status );
			l.current_context->aPosition = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"position", &status );
			l.current_context->aSize = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"size", &status );
			l.current_context->aType = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"type", &status );
			l.current_context->aEdit = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"edit", &status );
			l.current_context->aBorder = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"border", &status );
			l.current_context->aCaption = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"caption", &status );
			l.current_context->aID = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"ID", &status );
			l.current_context->aIDName = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"IDName", &status );
			l.current_context->aPrivate = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"PrivateData", &status );
			l.current_context->aExtraPrivate = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"ExtraPrivateData", &status );
			l.current_context->aChildren = genxDeclareAttribute( l.current_context->w, NULL, (constUtf8)"children", &status );
		}

		//genxStartDocFile( l.w, out );

		genxStartDocSender( l.current_context->w, &senderprocs );

		WriteCommonData( frame );

		genxEndDocument( l.current_context->w );

		{
			FILE *out;
			out = sack_fopen( 0, l.current_context->name, WIDE("wt")
#ifdef _UNICODE
								  WIDE(", ccs=UNICODE")
#endif
								 );
			if( out )
			{
				PTEXT text = VarTextGet( l.current_vt );
				sack_fwrite( GetText( text ), sizeof( TEXTCHAR ), GetTextSize( text ), out );
				LineRelease( text );
				// this is just a shot hand copy
				//VarTextDestroy( &l.current_vt );
				sack_fclose( out );
			}
			else
			{
				SimpleMessageBox( frame, WIDE("FAILED TO OPEN"), l.current_context->name );
			}
		}
		PopData( &l.contexts );
		Release( l.current_context->name );
		VarTextDestroy( &l.current_context->vt );
		genxDispose( l.current_context->w );
		if( ( l.current_context = (PXML_CONTEXT)PeekData( &l.contexts ) ) )
			l.current_vt = l.current_context->vt;
	}
   return 1;
}
PSI_XML_NAMESPACE_END

