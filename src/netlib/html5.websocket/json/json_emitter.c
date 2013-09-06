#include <stdhdrs.h>

#define JSON_EMITTER_SOURCE
#include <json_emitter.h>
/***********
 <Alternative, instead of object format thing, provide a registration mechanism to specify
  bindings similar to ODBC>



 Structure member notation...

 ###<T>

 [padding]name:[offset][type][(p/a)count][{subformat}],name:[offset][type][(p/a)count][{subformat}],...
 padding is an optional number that is minimum offset in the object (structure)
 offset is an optional number before the type; otherwise the offset is relative from prior members
 count is an optional number after the type for array types (is the size of the array)
    if count is (*) then the next member describes the array length
    if count is (-*) then the prior member describes the array length

 type can be one of
      i  (integer)
      ip (integer array (int*) )
      ia (integer array (int[]) )
      s  (string (char*))
		sa (string array (char**))
      c  (char; unused?)
		ca (char aray (string, but not from a pointer)(char[]))
		f  (float)
		fa (float array float[])
      fp (float array float* )
		d  (float)
		da (float array float[])
      dp (float array float* )

      o  (object) (begin another level of breakdown (void*) )
      op  (object) (begin another level of breakdown (void*) )


		struct outer {
          int a;
		    struct inner {
              char *name;
			 } *substruct;
			 }

		4a:i,substruct:op{name:s}
 *************/

#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace json {
#endif


struct json_context_object_element
{
	int type;     // type of the element at this offset
	int offset;   // offset into the structure
	CTEXTSTR name; // name of this element in the object
	int count; // at offset, this number of these is there; (array)
	int count_offset; // at count_offset, is the number of elements that the pointer at this offset
   struct json_context_object *object;
}

struct json_context_object
{
   PLIST members;   // list of members of this object
   CTEXTSTR name;   // name of this object (might be an object within an object)
};

struct json_context
{
	int levels;
	PVARTEXT pvt;
   PLIST object_types;
};

static CTEXTSTR tab_filler = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

//----------------------------------------------------------------------------------------------

struct json_context *json_create_context( void )
{
	struct json_context *context = New( struct json_context );
	MemSet( context, 0, sizeof( struct json_context ) );
   context->pvt = VarTextCreate();
   return context;
}

//----------------------------------------------------------------------------------------------

void json_begin_object( struct json_context *context, CTEXTSTR name )
{
	if( context->levels && name )
		vtprintf( context->pvt, "%*.*s\"%s\":{\n"
				  , context->levels, context->levels, tab_filler
				  , name );
   else
		vtprintf( context->pvt, "%*.*s{\n"
				  , context->levels, context->levels, tab_filler );
	context->levels++;
}


//----------------------------------------------------------------------------------------------

void json_end_object( struct json_context *context )
{
	context->levels--;
	vtprintf( context->pvt, "%*.*s}\n"
			  , context->levels, context->levels, tab_filler );
}

//----------------------------------------------------------------------------------------------

void json_add_value( struct json_context *context, CTEXTSTR name, CTEXTSTR value )
{
	vtprintf( context->pvt, "%*.*s\"%s\":\"%s\"\n"
			  , context->levels, context->levels, tab_filler
			  , name, value );
}

//----------------------------------------------------------------------------------------------

void json_add_int_value( struct json_context *context, CTEXTSTR name, int value )
{
  vtprintf( context->pvt, "%*.*s\"%s\":%d\n"
			  , context->levels, context->levels, tab_filler
			  , name, value );
}

//----------------------------------------------------------------------------------------------

void json_add_float_value( struct json_context *context, CTEXTSTR name, double value )
{
  vtprintf( context->pvt, "%*.*s\"%s\":%g\n"
			  , context->levels, context->levels, tab_filler
			  , name, value );
}

//----------------------------------------------------------------------------------------------

void json_add_value_array( struct json_context *context, CTEXTSTR name, CTEXTSTR* pValue, int nValues )
{
	int n;
	vtprintf( context->pvt, "%*.*s\"%s\":["
			  , context->levels, context->levels, tab_filler
			  , name );
   for( n = 0; n < nValues; n++ )
		vtprintf( context->pvt, "%s%s", (n==0)?"":",", pValue[nValues] );
	vtprintf( context->pvt, "]\n" );
}

//----------------------------------------------------------------------------------------------

void json_add_int_value_array( struct json_context *context, CTEXTSTR name, int* pValues, int nValues )
{
	int n;
	vtprintf( context->pvt, "%*.*s\"%s\":["
			  , context->levels, context->levels, tab_filler
			  , name );
   for( n = 0; n < nValues; n++ )
		vtprintf( context->pvt, "%s%d", (n==0)?"":",", pValue[nValues] );
	vtprintf( context->pvt, "]\n" );
}

//----------------------------------------------------------------------------------------------

void json_add_float_value_array( struct json_context *context, CTEXTSTR name, float* pValues, int nValues )
{
	int n;
	vtprintf( context->pvt, "%*.*s\"%s\":["
			  , context->levels, context->levels, tab_filler
			  , name );
   for( n = 0; n < nValues; n++ )
		vtprintf( context->pvt, "%s%g", (n==0)?"":",", pValue[nValues] );
	vtprintf( context->pvt, "]\n" );
}

//----------------------------------------------------------------------------------------------

void json_add_double_value_array( struct json_context *context, CTEXTSTR name, double* pValues, int nValues )
{
	int n;
	vtprintf( context->pvt, "%*.*s\"%s\":["
			  , context->levels, context->levels, tab_filler
			  , name );
   for( n = 0; n < nValues; n++ )
		vtprintf( context->pvt, "%s%g", (n==0)?"":",", pValue[nValues] );
	vtprintf( context->pvt, "]\n" );
}

//----------------------------------------------------------------------------------------------

static int GetNumber( CTEXTSTR *start )
{
   int result = 0;
	while( (*start)[0] <= '9' && (*start)[0] >= '0' )
	{
      result *= 10;
		result += (*start)[0] - '0';
      (*start)++;
	}
   return result;
}

//----------------------------------------------------------------------------------------------

PTRSZVAL ParseFormat( struct json_context *context, CTEXTSTR format, PTRSZVAL object )
{
	int padding = 0;
	int member_offset;
   PTRSZVAL current_obj_ofs = object;
	CTEXTSTR start = format;
   CTEXTSTR _start;
	TEXTCHAR namebuf[256];
#define namebuf_size (sizeof(namebuf)/sizeof(namebuf[0]))
   int name_end;

   padding = GetNumber( &start );

	name_end = 0;
   _start = start;
	while( name_end < namebuf_size && start[0] && start[0] != ':' )
	{
		namebuf[name_end++] = start[0];
      start++;
	}
	namebuf[name_end] = 0;

	if( !start[0] )
	{
		lprintf( WIDE("Object description format error: no colon after (%s)"), _start );
		return;
	}

	start++; // skip the ':'

   member_offset = GetNumber( &start );
	if( member_offset )
      current_obj_ofs = ((PTRSZVAL)object)+member_offset;


#define padded_add(n)   ((n)<padding?padding:(n))
	switch( start[0] )
	{
	case 'i':
		{
         int int_size;
			start++;
			int_size = GetNumber( &start );
			switch( start[0] )
			{
			case 'p':
            break;
			case 'a':
				break;
			default:
				switch( int_size )
				{
				case 1:
					current_obj_ofs += padded_add( 1 );
               break;
				case 2:
					current_obj_ofs += padded_add( 2 );
               break;
				case 4:
					current_obj_ofs += padded_add( 4 );
               break;
				case 8:
					current_obj_ofs += padded_add( 8 );
               break;

				default:
					current_obj_ofs += padded_add( sizeof(int) );
				}
			}
		}
      break;
	case 's':
      current_obj_ofs += padded_add( sizeof(char*) );
      break;
	case 'c':
		current_obj_ofs += padded_add( sizeof(char) );
      break;
	case 'f':
      current_obj_ofs += padded_add( sizeof(float) );
      break;
	case 'd':
      current_obj_ofs += padded_add( sizeof(double) );
      break;
	case 'o':
		switch( start[1] )
		{
		case 'p':
			if( start[2] == '{' )
			{
				json_add_object( context, namebuf, start + 3, *(POINTER*)current_obj_ofs );
			}
         current_obj_ofs + padded_add( sizeof( POINTER ) );
			break;
		case '{':
         current_obj_ofs = json_add_object( context, namebuf, start + 2, (POINTER)current_obj_ofs );
         break;
		default:
         lprintf( "Didn't find object description for object (%s) near (%s)", namebuf, start );
         break;
		}
		break;
	default:
      lprintf( WIDE("Unrecognize type format character at (%s) for value (%s)"), start, namebuf );
	}
   return current_obj_ofs;
}

//----------------------------------------------------------------------------------------------

void json_add_object( struct json_context *context, CTEXTSTR name, struct json_context_object *format, POINTER object )
{
}

//----------------------------------------------------------------------------------------------

void json_add_object_array( struct json_context *context, CTEXTSTR name, struct json_context_object *format, POINTER pValues, int nValues  )
{
}

//----------------------------------------------------------------------------------------------

struct json_context_object *json_create_object( struct json_context *context, CTEXTSTR name )
{
	struct json_context_object *format = New( struct json_context_object );
   format->name = StrDup( name ); // just used for debugging/diagnostics
	format->members = NULL;
   AddLink( &context->object_types, format );
   return format;
}


//----------------------------------------------------------------------------------------------

struct json_context_object *json_add_object_member_array( struct json_context *context
											, struct json_context_object *format
											, CTEXTSTR name
											, int offset, int type, int count )
{
	struct json_context_object_member *member = New( struct json_context_object_member );
	member->name = StrDup( name );
	member->offset = offset;
	member->type = type;
   member->count = count;
	switch( type )
	{
	case JSON_Element_Object:
	case JSON_Element_ObjectPointer:
		member->object = json_create_object( context, name );
      break;
	}
   AddLink( &format->members, member );
	if( member->object )
		return member->object;
   return format;
}

//----------------------------------------------------------------------------------------------

struct json_context_object *json_add_object_member( struct json_context *context
									, struct json_context_object *format
									, CTEXTSTR name
									, int offset, int type )
{
   return json_add_object_member_array( context, format, name, offset, type, 1 );
}

//----------------------------------------------------------------------------------------------
void json_add_object_member_array_pointer( struct json_context *context
													  , struct json_context_object *format
													  , CTEXTSTR name
													  , int offset, int type
													  , int count_offset )
{
	struct json_context_object_member *member = New( struct json_context_object_member );
	member->name = StrDup( name );
	member->offset = offset;
	member->type = type;
   member->count_offset = count_offset;
}

//----------------------------------------------------------------------------------------------

CTEXTSTR json_build_message( struct json_context *context
									, struct json_context_object *format
									, POINTER msg )
{
	CTEXTSTR result;

	INDEX idx;
	struct json_context_object_member *member;
   VarTextEmpty( context->pvt );
   json_begin_object( context );
	LIST_FORALL( format->members, idx, struct json_context_object_member *, member )
	{
		switch( member->type )
		{
		case JSON_Element_Integer:
			if( member->count )
				json_add_int_value_array( context, member->name, *(int*)(((PTRSZVAL)msg)+member->offset), member->count );
			else if( member->count_offset >= 0 )
				json_add_int_value_array( context, member->name
										, *(int*)(((PTRSZVAL)msg)+member->offset)
										, *(int*)(((PTRSZVAL)msg)+member->count_offset)
										);
			else
				json_add_int_value( context, member->name, *(int*)(((PTRSZVAL)msg)+member->offset) );
			break;
		case JSON_Element_String:
			if( member->count )
				json_add_value_array( context, member->name, (CTEXTSTR)(((PTRSZVAL)msg)+member->offset), member->count );
			else if( member->count_offset >= 0 )
				json_add_value_array( context, member->name
										, (CTEXTSTR)(((PTRSZVAL)msg)+member->offset)
										, *(int*)(((PTRSZVAL)msg)+member->count_offset)
										);
			else
				json_add_value( context, member->name, (CTEXTSTR)(((PTRSZVAL)msg)+member->offset) );
         break;
		case JSON_Element_ObjectPointer:

         break;
		}
	}

	json_end_object( context );

	{
		PTEXT tmp = VarTextGet( context->pvt );
		result = StrDup( GetText( tmp ) );
		LineRelease( tmp );
		return result;
	}
}

//----------------------------------------------------------------------------------------------

CTEXTSTR json_parse_message( struct json_context *context
									, struct json_context_object *format
                           , CTEXTSTR msg
									, POINTER msg_output )
{
}

#ifdef __cplusplus
} } }
#endif

