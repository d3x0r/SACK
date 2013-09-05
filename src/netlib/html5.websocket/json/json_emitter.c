#include <stdhdrs.h>

/***********
 Structure member notation...

 ###<T>





 *************/

static json_context
{
	int levels;
   PVARTEXT pvt;
};

static CTEXTSTR tab_filler = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";


void json_begin_object( struct json_context *context, CTEXTSTR name )
{
	if( context->levels && name )
		vtprintf( context->pvt, "\"%s\":{", name );
   else
		vtprintf( context->pvt, "{" );
	context->levels++;
}



void json_end_object( struct json_context *context )
{
	context->levels--;
	vtprintf( context->pvt, "}\n" );
}

void json_add_value( struct json_context *context, CTEXTSTR name, CTEXTSTR value )
{
}

void json_add_value_array( struct json_context *context, CTEXTSTR name, CTEXTSTR* value )
{
}


