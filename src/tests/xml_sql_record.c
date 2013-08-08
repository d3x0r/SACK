
#include <stdhdrs.h>

#include <genxml/genx.h>
#include <sexpat/sexpat.h>


typedef struct context_tag
{
   // name of the file we're building..
	TEXTCHAR *name;
	//PVARTEXT vt;
	//_32 nChildren;
	//PCOMMON pc; // current control...

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


			l.current_context->w = genxNew(NULL,NULL,NULL);
			l.current_context->eControl = genxDeclareElement( l.current_context->w, NULL, (constUtf8)"control", &status );


void WriteRecord( PCTEXTSTR record )
{
	genxStartElement( l.current_context->eTableRow );
	genxStartElement( l.current_context->eTableData );

}



int xmlSQLRecordQuery(

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
			ext = current_context->name + strlen( current_context->name );
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


