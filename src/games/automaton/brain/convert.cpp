#include <stdhdrs.h>
#include "braintypes.h"

//LIBMAIN() { return 1; }
//LIBEXIT() { return 1; }
//LIBMAIN_END();

value::value( enum type settype, ... )
{
	type = settype;
	memcpy( &data, &settype + sizeof( settype ), sizeof( data ) );
}

value::value( value *cloneme )
{
	type = cloneme->type;
	aux = cloneme->aux;
	memcpy( &data, &cloneme->data, sizeof( data ) );
}

NATIVE ANYVALUE::get( void )
{
   if( !this )
      return 0;
   switch( type )
   {
	case VAL_BOOL:
		return (NATIVE)(data.b);
   case VAL_FLOAT:
      return (NATIVE)(data.f);
   case VAL_DOUBLE:
      return (NATIVE)(data.d);
   case VAL_CHAR:
      return (NATIVE)(data.c);
   case VAL_SHORT:
      return (NATIVE)data.s;
   case VAL_LONG:
      return (NATIVE)data.l;
   case VAL_LONGLONG:
      return (NATIVE)data.ll;
   case VAL_UCHAR:
      return (NATIVE)data.uc;
   case VAL_USHORT:
      return (NATIVE)data.us;
   case VAL_ULONG:
      return (NATIVE)data.ul;
   case VAL_ULONGLONG:
      return (NATIVE)(S_64)data.ull;
   case VAL_PTRFLOAT:
      return (NATIVE)*data.pf;
   case VAL_PTRDOUBLE:
      return (NATIVE)*data.pd;
   case VAL_PTRCHAR:
      return (NATIVE)*data.pc;
   case VAL_PTRSHORT:
      return (NATIVE)*data.ps;
   case VAL_PTRLONG:
      return (NATIVE)*data.pl;
   case VAL_PTRLONGLONG:
      return (NATIVE)*data.pll;
   case VAL_PTRUCHAR:
      return (NATIVE)*data.puc;
   case VAL_PTRUSHORT:
      return (NATIVE)*data.pus;
   case VAL_PTRULONG:
      return (NATIVE)*data.pul;
   case VAL_PTRULONGLONG:
      return (NATIVE)(S_64)*data.pull;
   case VAL_EXTERNINPUT:
      return data.Input(aux);
   case VAL_EXTERNOUTPUT:
		break;
   case VAL_EXTERNTRIGGER:
		break;
	case VAL_PANYVALUE:
      return data.pany->get();
   }
	return 0;
}

int ANYVALUE::get_int( void )
{
   if( !this )
      return 0;
   switch( type )
   {
   case VAL_FLOAT:
      return (int)(data.f);
   case VAL_DOUBLE:
      return (int)(data.d);
   case VAL_CHAR:
      return (int)(data.c);
   case VAL_SHORT:
      return (int)data.s;
   case VAL_LONG:
      return (int)data.l;
   case VAL_LONGLONG:
      return (int)data.ll;
   case VAL_UCHAR:
      return (int)data.uc;
   case VAL_USHORT:
      return (int)data.us;
   case VAL_ULONG:
      return (int)data.ul;
   case VAL_ULONGLONG:
      return (int)(S_64)data.ull;
   case VAL_PTRFLOAT:
      return (int)*data.pf;
   case VAL_PTRDOUBLE:
      return (int)*data.pd;
   case VAL_PTRCHAR:
      return (int)*data.pc;
   case VAL_PTRSHORT:
      return (int)*data.ps;
   case VAL_PTRLONG:
      return (int)*data.pl;
   case VAL_PTRLONGLONG:
      return (int)*data.pll;
   case VAL_PTRUCHAR:
      return (int)*data.puc;
   case VAL_PTRUSHORT:
      return (int)*data.pus;
   case VAL_PTRULONG:
      return (int)*data.pul;
   case VAL_PTRULONGLONG:
      return (int)(S_64)*data.pull;
   case VAL_EXTERNINPUT:
      return (int)data.Input(aux);
   case VAL_EXTERNOUTPUT:
		break;
   case VAL_EXTERNTRIGGER:
		break;
	case VAL_PANYVALUE:
      return data.pany->get_int();
   }
	return 0;
}

void ANYVALUE::set( NATIVE native )
{
   if( !this )
      return;
   switch( type )
	{
	case VAL_PANYVALUE:
		data.pany->set(native);
      break;
   case VAL_FLOAT:
      data.f = native;
      break;
   case VAL_DOUBLE:
      data.d = native;
      break;
   case VAL_CHAR:
      data.c = (S_8)native;
      break;
   case VAL_SHORT:
      data.s = (S_16)native;
      break;
   case VAL_LONG:
      data.l = (S_32)native;
      break;
   case VAL_LONGLONG:
      data.ll = (S_64)native;
      break;
   case VAL_UCHAR:
      data.uc = (_8)native;
      break;
   case VAL_USHORT:
      data.us = (_16)native;
      break;
   case VAL_ULONG:
      data.ul = (_32)native;
      break;
   case VAL_ULONGLONG:
      data.ull = (_64)native;
      break;
   case VAL_PTRFLOAT:
      *data.pf = (float)native;
      break;
   case VAL_PTRDOUBLE:
      *data.pd = (double)native;
      break;
   case VAL_PTRCHAR:
      *data.pc = (char)native;
      break;
   case VAL_PTRSHORT:
      *data.ps = (S_16)native;
      break;
   case VAL_PTRLONG:
      *data.pl = (S_32)native;
      break;
   case VAL_PTRLONGLONG:
      *data.pll = (S_64)native;
      break;
   case VAL_PTRUCHAR:
      *data.puc = (_8)native;
      break;
   case VAL_PTRUSHORT:
      *data.pus = (_16)native;
      break;
   case VAL_PTRULONG:
      *data.pul = (_32)native;
      break;
   case VAL_PTRULONGLONG:
      *data.pull = (_64)native;
      break;
   case VAL_EXTERNINPUT:
   case VAL_EXTERNOUTPUT:
      data.Output( aux, native );
      break;
   case VAL_EXTERNTRIGGER:
		break;
   }
}


void value::set( enum type type, ... )
{
	va_list args;
	va_start( args, type );
	value::type = type;
	switch( type )
	{
	case VAL_FLOAT:
		value::data.f = (float)va_arg( args, double );
		break;
	case VAL_DOUBLE:
		value::data.d = va_arg( args, double );
		break;
	case VAL_CHAR:
		value::data.c = va_arg( args, char);
		break;
	case VAL_SHORT:
		value::data.s = va_arg( args, short);
		break;
	case VAL_LONG:
		value::data.l = va_arg( args, long);
		break;
	case VAL_LONGLONG:
		value::data.ll = va_arg( args, _64);
		break;
	case VAL_UCHAR:
		value::data.uc = va_arg( args, unsigned char);
		break;
	case VAL_USHORT:
		value::data.us = va_arg( args, unsigned short);
		break;
	case VAL_ULONG:
		value::data.ul = va_arg( args, unsigned long);
		break;
	case VAL_ULONGLONG:
		value::data.ull = va_arg( args, _64);
		break;
	case VAL_PTRFLOAT:
		value::data.pf = va_arg( args, float*);
		break;
	case VAL_PTRDOUBLE:
		value::data.pd = va_arg( args, double*);
		break;
	case VAL_PTRCHAR:
		value::data.pc = va_arg( args, char *);
		break;
	case VAL_PTRSHORT:
		value::data.ps = va_arg( args, short*);
		break;
	case VAL_PTRLONG:
		value::data.pl = va_arg( args, long*);
		break;
	case VAL_PTRLONGLONG:
		value::data.pll = va_arg( args, S_64 *);
		break;
	case VAL_PTRUCHAR:
		value::data.puc = va_arg( args, unsigned char *);
		break;
	case VAL_PTRUSHORT:
		value::data.pus = va_arg( args, unsigned short *);
		break;
	case VAL_PTRULONG:
		value::data.pul = va_arg( args, unsigned long *);
		break;
	case VAL_PTRULONGLONG:
		value::data.pull = va_arg( args, _64*);
		break;
	case VAL_EXTERNINPUT:
		value::data.Input = va_arg( args, InputFunction);
		value::aux = va_arg( args, PTRSZVAL );
		break;
	case VAL_EXTERNOUTPUT:
		value::data.Output = va_arg( args, OutputFunction);
		value::aux = va_arg( args, PTRSZVAL );
		break;
	case VAL_EXTERNTHRUPUT:
		value::data.Thruput = va_arg( args, ThruputFunction);
		value::aux = va_arg( args, PTRSZVAL );
		break;
	case VAL_EXTERNTRIGGER:
		value::data.Trigger = va_arg( args, TriggerFunction);
		value::aux = va_arg( args, PTRSZVAL );
		break;
	case VAL_PANYVALUE:
		value::data.pany = va_arg( args, PANYVALUE);
		break;
   }
}

void value::set( struct value &value )
{
	value::type = value.type;
	value::data = value.data;
}

void value::set( struct value *value )
{
	value::type = value->type;
	value::data = value->data;
}



/* this is a problem, I could save the function reference if I knew what it was
 * what exact type of user object, what exact user function....
 * I could ocmmunicate such things with procreg I guess, and saave the naem I lookup
 * to fill in the function....
 */
/*
_32 value::save( FILE *file )
{
   _32 out;
   NATIVE value;
	out = fwrite( &type, 1, sizeof( type ), file );
	switch( type )
	{
   case VAL_FLOAT:
   case VAL_DOUBLE:
   case VAL_CHAR:
   case VAL_SHORT:
   case VAL_LONG:
   case VAL_LONGLONG:
   case VAL_UCHAR:
   case VAL_USHORT:
   case VAL_ULONG:
	case VAL_ULONGLONG:
		value = get();
      out += fwrite( &value, 1, sizeof( value ), file );
      break;
   case VAL_PTRFLOAT:
   case VAL_PTRDOUBLE:
   case VAL_PTRCHAR:
   case VAL_PTRSHORT:
   case VAL_PTRLONG:
   case VAL_PTRLONGLONG:
   case VAL_PTRUCHAR:
   case VAL_PTRUSHORT:
   case VAL_PTRULONG:
	case VAL_PTRULONGLONG:
      lprintf( "Cannot save value of pointer type..." );
      break;
   case VAL_EXTERNINPUT:
   case VAL_EXTERNOUTPUT:
   case VAL_EXTERNTHRUPUT:
	case VAL_EXTERNTRIGGER:
		lprintf( "Cannot save value of function type..." );
      break;
	case VAL_PANYVALUE:
		// pointer to another value
      lprintf( "Cannot save value of pointer to another anyvalue" );
      break;
	}
   return out;
}

INDEX value::save( PODBC odbc, INDEX iParent )
{
   _32 out;
   NATIVE value;
	//out = fwrite( &type, 1, sizeof( type ), file );
	switch( type )
	{
   case VAL_FLOAT:
   case VAL_DOUBLE:
   case VAL_CHAR:
   case VAL_SHORT:
   case VAL_LONG:
   case VAL_LONGLONG:
   case VAL_UCHAR:
   case VAL_USHORT:
   case VAL_ULONG:
	case VAL_ULONGLONG:
		value = get();
      //out += fwrite( &value, 1, sizeof( value ), file );
      break;
   case VAL_PTRFLOAT:
   case VAL_PTRDOUBLE:
   case VAL_PTRCHAR:
   case VAL_PTRSHORT:
   case VAL_PTRLONG:
   case VAL_PTRLONGLONG:
   case VAL_PTRUCHAR:
   case VAL_PTRUSHORT:
   case VAL_PTRULONG:
	case VAL_PTRULONGLONG:
      lprintf( "Cannot save value of pointer type..." );
      break;
   case VAL_EXTERNINPUT:
   case VAL_EXTERNOUTPUT:
   case VAL_EXTERNTHRUPUT:
	case VAL_EXTERNTRIGGER:
		lprintf( "Cannot save value of function type..." );
      break;
	case VAL_PANYVALUE:
		// pointer to another value
      lprintf( "Cannot save value of pointer to another anyvalue" );
      break;
	}

   return INVALID_INDEX;
}

_32 value::load( FILE *file )
{
   NATIVE value;
	fread( &type, 1, sizeof( type ), file );
	switch( type )
	{
   case VAL_FLOAT:
   case VAL_DOUBLE:
   case VAL_CHAR:
   case VAL_SHORT:
   case VAL_LONG:
   case VAL_LONGLONG:
   case VAL_UCHAR:
   case VAL_USHORT:
   case VAL_ULONG:
	case VAL_ULONGLONG:
		fread( &value, 1, sizeof( value ), file );
      set( value );
      break;
   case VAL_PTRFLOAT:
   case VAL_PTRDOUBLE:
   case VAL_PTRCHAR:
   case VAL_PTRSHORT:
   case VAL_PTRLONG:
   case VAL_PTRLONGLONG:
   case VAL_PTRUCHAR:
   case VAL_PTRUSHORT:
   case VAL_PTRULONG:
	case VAL_PTRULONGLONG:
      lprintf( "Cannot load value of pointer type..." );
      break;
   case VAL_EXTERNINPUT:
   case VAL_EXTERNOUTPUT:
   case VAL_EXTERNTHRUPUT:
	case VAL_EXTERNTRIGGER:
		lprintf( "Cannot load value of function type..." );
      break;
	case VAL_PANYVALUE:
		// pointer to another value
      lprintf( "Cannot load value of pointer to another anyvalue" );
      break;
	}
   return 0;
}
*/