#include <stdhdrs.h>

#define USE_RENDER_INTERFACE pActImage
#define USE_IMAGE_INTERFACE pImageInterface

#include <image.h>
#include <render.h>
extern PRENDER_INTERFACE pActImage;
extern PIMAGE_INTERFACE pImageInterface;
#include <plugin.h>

extern INDEX iRender;

//---------------------------------------------------------------------------

PTEXT RenderGetX( uintptr_t psv, PENTITY pe, PTEXT *pData )
{
	return *pData;
}

//---------------------------------------------------------------------------

PTEXT RenderGetY( uintptr_t psv, PENTITY pe, PTEXT *pData )
{
	return *pData;
}

//---------------------------------------------------------------------------

PTEXT RenderSetX( uintptr_t psv, PENTITY pe, PTEXT pData )
{
	return pData;
}

//---------------------------------------------------------------------------

PTEXT RenderSetY( uintptr_t psv, PENTITY pe, PTEXT pData )
{
	return pData;
}

//---------------------------------------------------------------------------

PTEXT RenderGetWidth( uintptr_t psv, PENTITY pe, PTEXT *pData )
{
	return *pData;
}

//---------------------------------------------------------------------------

PTEXT RenderGetHeight( uintptr_t psv, PENTITY pe, PTEXT *pData )
{
	return *pData;
}

//---------------------------------------------------------------------------

void DestroyDisplay( PENTITY pe )
{
	INDEX idx;
	PENTITY peCreated;
	LIST_FORALL( pe->pCreated, idx, PENTITY, peCreated )
	{
		DestroyEntity( peCreated );
	}
}


//---------------------------------------------------------------------------

#define NUM_DISPLAY_VARS ( sizeof( DisplayVars ) / sizeof( DisplayVars[0] ) )
volatile_variable_entry DisplayVars[] = 
   { { DEFTEXT( "x" ), RenderGetX, RenderSetX }
   , { DEFTEXT( "y" ), RenderGetY, RenderSetY }
   , { DEFTEXT( "width" ), RenderGetWidth, NULL }
	, { DEFTEXT( "height" ), RenderGetHeight, NULL }
	};
int nDisplayVars = NUM_DISPLAY_VARS;

//---------------------------------------------------------------------------

int InitDisplayObject( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	PSENTIENT psNew;
	INDEX idx;
	DebugBreak();
	SetLink( &pe->pPlugin, iRender, 
			(POINTER)OpenDisplaySizedAt( 0,0,0,0,0 ) );
	if( !GetLink( &pe->pPlugin, iRender ) )
	{
		DECLTEXT( msg, "Failed to open physical display." );
		EnqueLink( &ps->Command->Output, &msg );
		return 1; // return failure.
	}
	psNew = CreateAwareness( pe );
	UnlockAwareness( psNew );
	for( idx = 0; idx < nDisplayVars; idx++ )
	{
		AddVolatileVariable( pe, DisplayVars + idx, 0 );
	}
	AddLink( &pe->pDestroy, DestroyDisplay );
	return 0; // return success
}

