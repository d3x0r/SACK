package psi
//package psi

/*
#include <stdhdrs.h>
#include <psi.h>


// The gateway function
int RegisterControl_cgo( const char *name )
{
	CONTROL_REGISTRATION reg;
        memset( &reg, 0, sizeof( reg ) );
        reg.name = name;
        reg.stuff.extra = sizeof( void * );
        reg.stuff.default_border = BORDER_NONE;
        reg.stuff.stuff.width = 120;
        reg.stuff.stuff.height = 18;
        DoRegisterControl( &reg );
        return reg.TypeID;
}


void SetUserData_cgo( PSI_CONTROL pc, void *p )
{
	SetCommonUserData( pc, (PTRSZVAL)p );
}

void *GetUserData_cgo( PSI_CONTROL pc )
{
	return (void*)GetCommonUserData( pc );
}

void *GetCustomData_cgo( PSI_CONTROL pc )
{
	void **p = (*(void***)pc);
        return p[0];
}

void SetCustomData_cgo( PSI_CONTROL pc, void *p )
{
	void **tmp = (*(void***)pc);
        tmp[0] = p;
}

int ControlMethodInit_cgo( PSI_CONTROL pc )
{
	return ControlInitMethod( pc );
}

int ControlMethodMouse_cgo( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	void **p = (*(void***)pc);
	return ControlMouseMethod( (*p), x, y, b );
}

int ControlMethodDraw_cgo( PSI_CONTROL pc )
{
	void **p = (*(void***)pc);
	return ControlDrawMethod( (*p) );
}

int MouseCallback_cgo(PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
        return ControlSetMouseMethod(pc, x, y, b );
}

*/
import "C"
