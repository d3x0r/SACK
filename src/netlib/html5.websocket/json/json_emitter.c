#include <stdhdrs.h>

#ifndef JSON_EMITTER_SOURCE
#  define JSON_EMITTER_SOURCE
#endif
#include <json_emitter.h>

#include "json.h"

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

uintptr_t json_add_object( struct json_context *context, CTEXTSTR name, struct json_context_object *format, POINTER object );

static CTEXTSTR tab_filler = WIDE("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");

//----------------------------------------------------------------------------------------------

struct json_context *json_create_context( void )
{
	struct json_context *context = New( struct json_context );
	MemSet( context, 0, sizeof( struct json_context ) );
	context->pvt = VarTextCreateExx( 1024, 16384 );
	
	return context;
}

//----------------------------------------------------------------------------------------------

void json_begin_object( struct json_context *context, CTEXTSTR name )
{
	if( context->human_readable )
		if( context->levels && name )
			vtprintf( context->pvt, WIDE("%*.*s\"%s\":{\n")
					  , context->levels, context->levels, tab_filler
					  , name );
		else
			vtprintf( context->pvt, WIDE("%*.*s{\n")
					  , context->levels, context->levels, tab_filler );
	else
		if( context->levels && name )
			vtprintf( context->pvt, WIDE("\"%s\":{")
					  , name );
		else
			VarTextAddCharacter( context->pvt, '{' );

	context->levels++;
}


//----------------------------------------------------------------------------------------------

void json_end_object( struct json_context *context )
{
	context->levels--;
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s}\n")
				  , context->levels, context->levels, tab_filler );
	else
		VarTextAddCharacter( context->pvt, '}');
}

//----------------------------------------------------------------------------------------------

void json_begin_array( struct json_context *context, CTEXTSTR name )
{
	if( context->human_readable )
		if( context->levels && name )
			vtprintf( context->pvt, WIDE("%*.*s\"%s\":[\n")
					  , context->levels, context->levels, tab_filler
					  , name );
		else
			vtprintf( context->pvt, WIDE("%*.*s[\n")
					  , context->levels, context->levels, tab_filler );
	else
		if( context->levels && name )
			vtprintf( context->pvt, WIDE("\"%s\":[")
					  , name );
		else
			VarTextAddCharacter( context->pvt, '[');
	context->levels++;
}


//----------------------------------------------------------------------------------------------

void json_end_array( struct json_context *context )
{
	context->levels--;
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s]\n")
				  , context->levels, context->levels, tab_filler );
	else
		VarTextAddCharacter( context->pvt, ']');
}

//----------------------------------------------------------------------------------------------

void json_add_value( struct json_context *context, CTEXTSTR name, CTEXTSTR value )
{
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":\"%s\"\n")
				  , context->levels, context->levels, tab_filler
				  , name, value );
	else
		vtprintf( context->pvt, WIDE("\"%s\":\"%s\"")
				  , name, value );
}

//----------------------------------------------------------------------------------------------

void json_add_list_value( struct json_context_object *object, struct json_context *context, CTEXTSTR name, PLIST values )
{
	INDEX idx;
	POINTER p;
	int first = 1;
	vtprintf( context->pvt, WIDE( "\"%s\":[" ), name );
	LIST_FORALL( values, idx, POINTER, p )
	{
		if( !first )
			vtprintf( context->pvt, WIDE( "," ) );
		else
			first = 0;
		json_build_message( object, p );
	}
	vtprintf( context->pvt, WIDE( "]" ) );
}

void json_add_int_64_value( struct json_context *context, CTEXTSTR name, int64_t value )
{
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":%") _64fs WIDE("\n")
				  , context->levels, context->levels, tab_filler
				  , name, value );
	else
		vtprintf( context->pvt, WIDE("\"%s\":%") _64fs
				  , name, value );
}

void json_add_int_32_value( struct json_context *context, CTEXTSTR name, int32_t value )
{
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":%") _32fs WIDE("\n")
				  , context->levels, context->levels, tab_filler
				  , name, value );
	else
		vtprintf( context->pvt, WIDE("\"%s\":%") _32fs 
				  , name, value );
}

void json_add_int_16_value( struct json_context *context, CTEXTSTR name, int16_t value )
{
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":%") _16fs WIDE("\n")
				  , context->levels, context->levels, tab_filler
				  , name, value );
	else
		vtprintf( context->pvt, WIDE("\"%s\":%") _16fs
				  , name, value );
}

void json_add_int_8_value( struct json_context *context, CTEXTSTR name, int8_t value )
{
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":%") _8fs WIDE("\n")
				  , context->levels, context->levels, tab_filler
				  , name, value );
	else
		vtprintf( context->pvt, WIDE("\"%s\":%") _8fs
				  , name, value );
}

//----------------------------------------------------------------------------------------------

void json_add_uint_64_value( struct json_context *context, CTEXTSTR name, uint64_t value )
{
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":%") _64f WIDE("\n")
				  , context->levels, context->levels, tab_filler
				  , name, value );
	else
		vtprintf( context->pvt, WIDE("\"%s\":%") _64f 
				  , name, value );
}

void json_add_uint_32_value( struct json_context *context, CTEXTSTR name, uint32_t value )
{
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":%") _32f WIDE("\n")
				  , context->levels, context->levels, tab_filler
				  , name, value );
	else
		vtprintf( context->pvt, WIDE("\"%s\":%") _32f 
				  , name, value );
}

void json_add_uint_16_value( struct json_context *context, CTEXTSTR name, uint16_t value )
{
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":%") _16f WIDE("\n")
				  , context->levels, context->levels, tab_filler
				  , name, value );
	else
		vtprintf( context->pvt, WIDE("\"%s\":%") _16f
				  , name, value );
}

void json_add_uint_8_value( struct json_context *context, CTEXTSTR name, uint8_t value )
{
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":%") _8f WIDE("\n")
				  , context->levels, context->levels, tab_filler
				  , name, value );
	else
		vtprintf( context->pvt, WIDE("\"%s\":%") _8f 
				  , name, value );
}

//----------------------------------------------------------------------------------------------

void json_add_float_value( struct json_context *context, CTEXTSTR name, double value )
{
	if( context->human_readable )
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":%g\n")
				  , context->levels, context->levels, tab_filler
				  , name, value );
	else
		vtprintf( context->pvt, WIDE("\"%s\":%g")
				  , name, value );

}

//----------------------------------------------------------------------------------------------

void json_add_value_array( struct json_context *context, CTEXTSTR name, CTEXTSTR* pValue, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%s"), (n==0)?WIDE(""):WIDE(","), pValue[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("\"%s\":[")
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%s"), (n==0)?WIDE(""):WIDE(","), pValue[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_int_64_value_array( struct json_context *context, CTEXTSTR name, int64_t* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%") _64fs, (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("\"%s\":[")
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%") _64fs, (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_int_32_value_array( struct json_context *context, CTEXTSTR name, int32_t* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%") _32fs, (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("\"%s\":[")
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%") _32fs, (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_int_16_value_array( struct json_context *context, CTEXTSTR name, int16_t* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%d"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("\"%s\":[")
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%d"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_int_8_value_array( struct json_context *context, CTEXTSTR name, int8_t* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%d"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("\"%s\":[")
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%d"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_uint_64_value_array( struct json_context *context, CTEXTSTR name, uint64_t* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%") _64f, (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("\"%s\":[")
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%") _64f, (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_uint_32_value_array( struct json_context *context, CTEXTSTR name, uint32_t* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%") _32f, (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("\"%s\":[")
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%") _32f, (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_uint_16_value_array( struct json_context *context, CTEXTSTR name, uint16_t* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%d"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("\"%s\":[")
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%d"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_uint_8_value_array( struct json_context *context, CTEXTSTR name, uint8_t* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%d"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("\"%s\":[")
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%d"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_int_array( struct json_context *context, CTEXTSTR name, int* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%d"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("\"%s\":[")
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%d"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_float_value_array( struct json_context *context, CTEXTSTR name, float* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%g"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%g"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
}

//----------------------------------------------------------------------------------------------

void json_add_double_value_array( struct json_context *context, CTEXTSTR name, double* pValues, size_t nValues )
{
	size_t n;
	if( context->human_readable )
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%g"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		vtprintf( context->pvt, WIDE("]\n") );
	}
	else
	{
		vtprintf( context->pvt, WIDE("%*.*s\"%s\":[")
				  , context->levels, context->levels, tab_filler
				  , name );
		for( n = 0; n < nValues; n++ )
			vtprintf( context->pvt, WIDE("%s%g"), (n==0)?WIDE(""):WIDE(","), pValues[nValues] );
		VarTextAddCharacter( context->pvt, ']' );
	}
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

uintptr_t ParseFormat( struct json_context *context, CTEXTSTR format, uintptr_t object )
{
	size_t padding = 0;
	int member_offset;
	uintptr_t current_obj_ofs = object;
	CTEXTSTR start = format;
	CTEXTSTR _start;
	TEXTCHAR namebuf[256];
#define namebuf_size (sizeof(namebuf)/sizeof(namebuf[0]))
	size_t name_end;

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
		return 0;
	}

	start++; // skip the ':'

	member_offset = GetNumber( &start );
	if( member_offset )
		current_obj_ofs = ((uintptr_t)object)+member_offset;


#define padded_add(n)	((n)<padding?padding:(n))
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
				// add_object doesn't work like this now
				//json_add_object( context, namebuf, start + 3, *(POINTER*)current_obj_ofs );
			}
			current_obj_ofs += padded_add( sizeof( POINTER ) );
			break;
		case '{':
			// add_object doesn't work like this now
			//current_obj_ofs = json_add_object( context, namebuf, start + 2, (POINTER)current_obj_ofs );
			break;
		default:
			lprintf( WIDE("Didn't find object description for object (%s) near (%s)"), namebuf, start );
			break;
		}
		break;
	default:
		lprintf( WIDE("Unrecognize type format character at (%s) for value (%s)"), start, namebuf );
	}
	return current_obj_ofs;
}


static size_t GetDefaultObjectSize( enum JSON_ObjectElementTypes type )
{
	switch( type )
	{
	case JSON_Element_Array:
	case JSON_Element_UserRoutine:
	case JSON_Element_Raw_Object:
		lprintf( "Returning invalid object size for this type." );
		return 0;

	case JSON_Element_Integer_8:
		return sizeof( int8_t );
	case JSON_Element_Integer_16:
		return sizeof( int16_t );
	case JSON_Element_Integer_32:
 		return sizeof( int32_t );
	case JSON_Element_Integer_64:
 		return sizeof( int64_t );
	case JSON_Element_Unsigned_Integer_8:
 		return sizeof( uint8_t );
	case JSON_Element_Unsigned_Integer_16:
		return sizeof( uint16_t );
	case JSON_Element_Unsigned_Integer_32:
		return sizeof( uint32_t );
	case JSON_Element_Unsigned_Integer_64:
		return sizeof( uint64_t );
	case JSON_Element_String:
		return sizeof( POINTER );
	case JSON_Element_CharArray:
		return 0;
	case JSON_Element_Float:
		return sizeof( float );
	case JSON_Element_Double:
		return sizeof( double );
	case JSON_Element_Object:
		return 0;
	case JSON_Element_ObjectPointer:
	case JSON_Element_List:
	case JSON_Element_Text:  // ptext type
		return sizeof( POINTER );
	case JSON_Element_PTRSZVAL:  
	case JSON_Element_PTRSZVAL_BLANK_0:
		return sizeof( uintptr_t );
		break;
	}
	return 0;
}
//----------------------------------------------------------------------------------------------

uintptr_t json_add_object( struct json_context *context, CTEXTSTR name, struct json_context_object *format, POINTER object )
{
	return 0;
}

//----------------------------------------------------------------------------------------------

void json_add_object_array( struct json_context *context, CTEXTSTR name, struct json_context_object *format, POINTER pValues, size_t nValues  )
{
}

//----------------------------------------------------------------------------------------------

struct json_context_object *json_create_object( struct json_context *context, size_t object_size )
{
	struct json_context_object *format = New( struct json_context_object );
	MemSet( format, 0, sizeof( struct json_context_object ) );
	format->context = context;
	if( object_size )
		format->object_size = object_size;
	else
		format->flags.dynamic_size = 1;
	// keep a reference for cleanup
	AddLink( &context->object_types, format );
	return format;
}


//----------------------------------------------------------------------------------------------

struct json_context_object *json_create_array( struct json_context *context
															, size_t offset
															, enum JSON_ObjectElementTypes type
															, size_t count
															, size_t count_offset
											 )
{
	struct json_context_object *format = New( struct json_context_object );
	format->context = context;
	format->members = NULL;
	format->is_array = TRUE;
	{
		struct json_context_object_element *element;
		element = New( struct json_context_object_element );
		MemSet( element, 0, sizeof( struct json_context_object_element ) );
		element->name = NULL;
		element->object = format;
		element->offset = offset;
		element->type = type;
		element->count = count;
		element->count_offset = count_offset;
		AddLink( &format->members, element );
	}
	// keep a reference for cleanup
	AddLink( &context->object_types, format );
	return format;
}


//----------------------------------------------------------------------------------------------

struct json_context_object *json_add_object_member_array( struct json_context_object *format
																		  , CTEXTSTR name
																		  , size_t offset
																		  , enum JSON_ObjectElementTypes type
																		  , size_t object_size
																		  , size_t count
																		  , size_t count_offset
																		  )
{
	struct json_context *context = format->context;
	struct json_context_object_element *member = New( struct json_context_object_element );
	MemSet( member, 0, sizeof( struct json_context_object_element ) );
	if( !object_size )
		object_size = GetDefaultObjectSize(type);
	if( format->flags.dynamic_size )
	{
		struct json_context_object *parent;
		for( parent = format; parent; parent = parent->parent )
		{
			parent->object_size += object_size;
		}
	}
	member->name = StrDup( name );
	member->offset = offset;
	member->type = type;
	member->count = count;
	member->count_offset = count_offset;
	switch( type )
	{
	case JSON_Element_Object:
	case JSON_Element_ObjectPointer:
		member->object = json_create_object( context, object_size );
		member->object->parent = format;
		member->object->offset = offset;
		member->object->flags.keep_phrase = TRUE;
		break;
	}
	AddLink( &format->members, member );
	if( member->object )
		return member->object;
	return format;
}

struct json_context_object *json_add_object_member_user_routine( struct json_context_object *object
																, CTEXTSTR name
																  , size_t offset, enum JSON_ObjectElementTypes type
																  , size_t object_size 
																  , void (*user_formatter)(PVARTEXT,CPOINTER) )
{
	struct json_context_object *new_object = json_add_object_member_array( object
	                                                                     , name
	                                                                     , offset
	                                                                     , JSON_Element_UserRoutine
	                                                                     , object_size?object_size:GetDefaultObjectSize( type )
	                                                                     , 0
	                                                                     , JSON_NO_OFFSET );
	struct json_context_object_element *last_element = NULL;
	struct json_context_object_element *element;
	INDEX idx;
	LIST_FORALL( object->members, idx, struct json_context_object_element *, element )
	{
		last_element = element;
	}
	if( last_element )
	{
		last_element->content_type = type;
		last_element->user_formatter = user_formatter;
	}
	return new_object;
}

//----------------------------------------------------------------------------------------------

struct json_context_object *json_add_object_member( struct json_context_object *format
																  , CTEXTSTR name
																  , size_t offset, enum JSON_ObjectElementTypes type
																  , size_t object_size )
{
	return json_add_object_member_array( format, name, offset, type, object_size, 0, JSON_NO_OFFSET );
}

// adds a reference to a PLIST as an array with the content of the array specified as the type
struct json_context_object *json_add_object_member_list( struct json_context_object *object
																		 , CTEXTSTR name
																		 , size_t offset
																		 , enum JSON_ObjectElementTypes content_type
																		 , size_t object_size
																		 )
{
	// this is a double pointer... implement as a specific type?
	//return json_add_object_member_array( format, name, offset, content_type, object_size, 0, offsetof(
	struct json_context *context = object->context;
	struct json_context_object_element *member = New( struct json_context_object_element );
	MemSet( member, 0, sizeof( struct json_context_object_element ) );
	member->name = StrDup( name );
	if( object->flags.dynamic_size )
	{
		struct json_context_object *parent;
		for( parent = object; parent; parent = parent->parent )
			parent->object_size += object_size;
	}
	member->object_size = object_size;
	member->offset = offset;
	member->type = JSON_Element_List;
	member->content_type = content_type;
	member->count = 0;
	member->count_offset = offsetof( LIST, Cnt );
	member->object = json_create_object( context, 0 );
	member->object->flags.keep_phrase = TRUE;
	AddLink( &object->members, member );
	if( member->object )
		return member->object;
	return object;
}

// this allows recursive structures, so the structure may contain a reference to itself.
// this allows buildling other objects and referencing them instead of building them in-place
JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member_object_array )( struct json_context_object *object
																												  , CTEXTSTR name
																												  , size_t offset
																												  , enum JSON_ObjectElementTypes type
																												  , struct json_context_object *child_object
																												  , int count
																												  , size_t count_offset
																												  )
{
	//struct json_context *context = object->context;
	struct json_context_object_element *member = New( struct json_context_object_element );
	MemSet( member, 0, sizeof( struct json_context_object_element ) );
	member->name = StrDup( name );
	member->offset = offset;
	member->type = type;
	member->count = count;
	member->count_offset = count_offset;
	switch( type )
	{

	case JSON_Element_Object:
	case JSON_Element_ObjectPointer:
		member->object = child_object;
		break;
	case JSON_Element_Integer_8:
	case JSON_Element_Integer_16:
	case JSON_Element_Integer_32:
	case JSON_Element_Integer_64:
	case JSON_Element_Unsigned_Integer_8:
	case JSON_Element_Unsigned_Integer_16:
	case JSON_Element_Unsigned_Integer_32:
	case JSON_Element_Unsigned_Integer_64:
	case JSON_Element_Float:
	case JSON_Element_Double:
	case JSON_Element_Array:
	case JSON_Element_List:
	case JSON_Element_Text:
	case JSON_Element_CharArray:
	case JSON_Element_String:
	case JSON_Element_PTRSZVAL:
	case JSON_Element_PTRSZVAL_BLANK_0:
	case JSON_Element_UserRoutine:
	case JSON_Element_Raw_Object:
	default:
		lprintf( WIDE("incompatible type") );
		break;
	}
	AddLink( &object->members, member );
	if( member->object )
		return member->object;
	return object;
}


JSON_EMITTER_PROC( struct json_context_object *, json_add_object_member_object )( struct json_context_object *object
																								 , CTEXTSTR name
																								 , size_t offset
																								 , enum JSON_ObjectElementTypes type
																								 , struct json_context_object *child_object
																								 )
{
	return json_add_object_member_object_array( object, name, offset, type, child_object, 0, JSON_NO_OFFSET );
}
//----------------------------------------------------------------------------------------------
struct json_context_object * json_add_object_member_array_pointer( struct json_context_object *object
													  , CTEXTSTR name
													  , size_t offset, enum JSON_ObjectElementTypes type
													  , size_t count_offset )
{
	struct json_context *context = object->context;
	struct json_context_object_element *member = New( struct json_context_object_element );
	MemSet( member, 0, sizeof( struct json_context_object_element ) );
	member->name = StrDup( name );
	member->offset = offset;
	member->type = type;
	member->count_offset = count_offset;
   return object;
}

//----------------------------------------------------------------------------------------------

TEXTSTR json_build_message( struct json_context_object *object
									, POINTER msg )
{
	struct json_context *context = object->context;
	TEXTSTR result;
	int n = 0;
	INDEX idx;
	struct json_context_object_element *member;
	if( !object->flags.keep_phrase )
		VarTextEmpty( context->pvt );
	if( object->is_array )
		json_begin_array( context, NULL );
	else
		json_begin_object( context, NULL );

	LIST_FORALL( object->members, idx, struct json_context_object_element *, member )
	{
		if( n && ( member->type != JSON_Element_PTRSZVAL_BLANK_0 ) )
			vtprintf( context->pvt, WIDE(",") );
		n++;
		switch( member->type )
		{
		default:
			lprintf( WIDE("Unhandled json_emitter type: %d"), member->type );
			break;
		case JSON_Element_List:
			//if( member->count )
			//	;
			//else if( member->count_offset != JSON_NO_OFFSET )
			//	;
			//else
				json_add_list_value( member->object, context, member->name, *(PLIST*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_Integer_64:
			if( member->count )
				json_add_int_64_value_array( context, member->name, (int64_t*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_int_64_value_array( context, member->name
										, *(int64_t**)(((uintptr_t)msg)+member->offset)
										, *(size_t*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_int_64_value( context, member->name, *(int64_t*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_Integer_32:
			if( member->count )
				json_add_int_32_value_array( context, member->name, (int32_t*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_int_32_value_array( context, member->name
										, *(int32_t**)(((uintptr_t)msg)+member->offset)
										, *(size_t*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_int_32_value( context, member->name, *(int32_t*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_Integer_16:
			if( member->count )
				json_add_int_16_value_array( context, member->name, (int16_t*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_int_16_value_array( context, member->name
										, *(int16_t**)(((uintptr_t)msg)+member->offset)
										, *(size_t*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_int_16_value( context, member->name, *(int16_t*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_Integer_8:
			if( member->count )
				json_add_int_8_value_array( context, member->name, (int8_t*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_int_8_value_array( context, member->name
										, *(int8_t**)(((uintptr_t)msg)+member->offset)
										, *(size_t*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_int_8_value( context, member->name, *(int8_t*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_Unsigned_Integer_64:
			if( member->count )
				json_add_uint_64_value_array( context, member->name, (uint64_t*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_uint_64_value_array( context, member->name
										, *(uint64_t**)(((uintptr_t)msg)+member->offset)
										, *(size_t*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_uint_64_value( context, member->name, *(uint64_t*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_Unsigned_Integer_32:
			if( member->count )
				json_add_uint_32_value_array( context, member->name, (uint32_t*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_uint_32_value_array( context, member->name
										, *(uint32_t**)(((uintptr_t)msg)+member->offset)
										, *(size_t*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_uint_32_value( context, member->name, *(uint32_t*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_Unsigned_Integer_16:
			if( member->count )
				json_add_uint_16_value_array( context, member->name, (uint16_t*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_uint_16_value_array( context, member->name
										, *(uint16_t**)(((uintptr_t)msg)+member->offset)
										, *(size_t*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_uint_16_value( context, member->name, *(uint16_t*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_Unsigned_Integer_8:
			if( member->count )
				json_add_uint_8_value_array( context, member->name, (uint8_t*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_uint_8_value_array( context, member->name
										, *(uint8_t**)(((uintptr_t)msg)+member->offset)
										, *(size_t*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_uint_8_value( context, member->name, *(uint8_t*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_Float:
			if( member->count )
				json_add_float_value_array( context, member->name, (float*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_float_value_array( context, member->name
										, *(float**)(((uintptr_t)msg)+member->offset)
										, *(size_t*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_float_value( context, member->name, *(float*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_Double:
			if( member->count )
				json_add_double_value_array( context, member->name, (double*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_double_value_array( context, member->name
										, *(double**)(((uintptr_t)msg)+member->offset)
										, *(size_t*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_float_value( context, member->name, *(double*)(((uintptr_t)msg)+member->offset) );
			break;

		case JSON_Element_String:
			if( member->count )
				json_add_value_array( context, member->name, *(CTEXTSTR**)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_value_array( context, member->name
										, (CTEXTSTR*)(((uintptr_t)msg)+member->offset)
										, *(int*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_value( context, member->name, *(CTEXTSTR*)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_CharArray:
			if( member->count )
				json_add_value_array( context, member->name, (CTEXTSTR*)(((uintptr_t)msg)+member->offset), member->count );
			else if( member->count_offset != JSON_NO_OFFSET )
				json_add_value_array( context, member->name
										, (CTEXTSTR*)(((uintptr_t)msg)+member->offset)
										, *(int*)(((uintptr_t)msg)+member->count_offset)
										);
			else
				json_add_value( context, member->name, (CTEXTSTR)(((uintptr_t)msg)+member->offset) );
			break;
		case JSON_Element_ObjectPointer:
			{
				vtprintf( context->pvt, WIDE("\"%s\":")
						  , member->name );
				json_build_message( member->object, *((POINTER*)(((uintptr_t)msg)+member->offset)) );
			}
			break;
		case JSON_Element_Object:
			{
				vtprintf( context->pvt, WIDE("\"%s\":")
						  , member->name );
				json_build_message( member->object, (POINTER)(((uintptr_t)msg)+member->offset) );
			}
			break;
		case JSON_Element_PTRSZVAL:
			{
				uintptr_t psv;

#ifdef __64__
				if( ( psv = *(int64_t*)(((uintptr_t)msg)+member->offset) ) == INVALID_INDEX )
					json_add_int_64_value( context, member->name, *(int64_t*)(((uintptr_t)msg)+member->offset) );
				else
					json_add_uint_64_value( context, member->name, *(int64_t*)(((uintptr_t)msg)+member->offset) );
#else
				if( ( psv = *(int32_t*)(((uintptr_t)msg)+member->offset) ) == INVALID_INDEX )
					json_add_int_32_value( context, member->name, *(int32_t*)(((uintptr_t)msg)+member->offset) );
				else
					json_add_uint_32_value( context, member->name, *(int32_t*)(((uintptr_t)msg)+member->offset) );
#endif
			}
			break;
		case JSON_Element_PTRSZVAL_BLANK_0:
			{
				uintptr_t psv;
#ifdef __64__
				if( psv = *(int64_t*)(((uintptr_t)msg)+member->offset) )
				{
					if( n )
						vtprintf( context->pvt, WIDE(",") );
					if( psv == INVALID_INDEX )
						json_add_int_64_value( context, member->name, psv );
					else
						json_add_uint_64_value( context, member->name, psv );
				}
#else
				if( psv = *(int32_t*)(((uintptr_t)msg)+member->offset) )
				{
					if( n )
						vtprintf( context->pvt, WIDE(",") );
					if( psv == INVALID_INDEX )
						json_add_int_32_value( context, member->name, psv );
					else
						json_add_uint_32_value( context, member->name, psv );
				}
#endif
			}
			break;
		case JSON_Element_UserRoutine:
			vtprintf( context->pvt, WIDE("\"%s\":"), member->name );
			member->user_formatter( context->pvt, (CPOINTER)(((uintptr_t)msg)+member->offset) );
			break;
		}
	}

	if( object->is_array )
		json_end_array( context );
	else
		json_end_object( context );

	if( !object->flags.keep_phrase )
	{
		PTEXT tmp = VarTextGet( context->pvt );
		result = StrDup( GetText( tmp ) );
		LineRelease( tmp );
		return result;
	}
	// will be incomplete... 
	return NULL; 
}

//----------------------------------------------------------------------------------------------


#ifdef __cplusplus
} } }
#endif

