
#include <stdhdrs.h>

#include "flatland_global.h"

#include <psi.h>
#include <controls.h>
#include "display.h"

#include <world.h>

#define EDT_NAME   1000
#define CHK_INVERT 1001

#define BTN_NEWTEXTURE   1002
#define BTN_TEXTURECOLOR 1003
#define LST_TEXTURES     1004
#define BTN_DELTEXTURE   1005

void SimpleMessage( TEXTCHAR *msg )
{
	//MessageBox( NULL, msg, "Cheesey simple message", MB_OK );
}



int MySimpleUserQuery( TEXTCHAR *result, int reslen, TEXTCHAR *question, PCOMMON pAbove )
{
	PCOMMON pf;
	PCONTROL edit;
	S_32 mouse_x, mouse_y;

	int Done = FALSE, Okay = FALSE;
	pf = CreateFrame( NULL, 0, 0, 280, 60, 0, pAbove );
	MakeTextControl( pf, 5, 1, 270, 14, TXT_STATIC, question, TEXT_NORMAL );
	edit = MakeEditControl( pf, 5, 16, 270, 14, TXT_STATIC, NULL, 0 );
	AddCommonButtons( pf, &Done, &Okay );
	GetMousePosition( &mouse_x, &mouse_y );
	MoveFrame( pf, mouse_x - 140, mouse_y - 30 );
	SetCommonFocus( edit );
	DisplayFrame( pf );
	CommonWait( pf );
	if( Okay )
	{
		GetControlText( edit, result, reslen );
	}
	DestroyFrame( &pf );
	return Okay;
}



void CPROC NewTexture( PTRSZVAL psvParent, PCONTROL button )
{
	TEXTCHAR name[256];
	INDEX texture;
	PLISTITEM hli;
	struct arg_tag{
		PDISPLAY display;
		PCOMMON parent;
	} *args = (struct arg_tag *)psvParent;
	name[0] = 0;
	if( SimpleUserQuery( name, 256, WIDE("Enter new texture name..."), args->parent ) )
	{
		PCONTROL pcList;
		if( !StrLen( name ) )
		{
			SimpleMessage( WIDE("Hey - gotta put in a name :) ") );
			return;
		}
		texture = MakeTexture( args->display->pWorld, MakeName( args->display->pWorld, name ) );
		SetSolidColor( args->display->pWorld, texture, AColor( 128, 128, 128, 128 ) );
		hli = AddListItem( pcList = GetNearControl( button, LST_TEXTURES ), name );
		SetItemData( hli, (PTRSZVAL)texture );
		SetCurrentItem( pcList, hli );
		SetSelectedItem( pcList, hli );
	}
}

void CPROC SetTextureColor( PTRSZVAL unused, PCONTROL button  )
{
	PCONTROL pList;
	PLISTITEM hli;
	PFLATLAND_TEXTURE pTexture;
	INDEX iTexture;
	INDEX iWorld = (INDEX)unused;
	pList = GetNearControl( button, LST_TEXTURES ); 
	hli = GetSelectedItem( pList );
	GetTextureData( iWorld, iTexture = (INDEX)GetItemData( hli ), &pTexture );
	//pTexture = (PFLATLAND_TEXTURE)GetItemData(hli);
	if( pTexture )
	{
		CDATA result;
		if( PickColor( &result, pTexture->data.color, GetFrame( pList ) ) )
			SetSolidColor( iWorld, iTexture, result );
	}
}

static INDEX CPROC AddTextureName( INDEX texture, PTRSZVAL pcList )
{
	TEXTCHAR name[128];
	PLISTITEM hli;
	PFLATLAND_TEXTURE pTexture;
	PDISPLAY display = (PDISPLAY)GetCommonUserData( GetFrame( pcList ) );
	GetTextureData( display->pWorld, texture, &pTexture );
	GetNameText( display->pWorld, pTexture->iName, name, 256 );
	hli = AddListItem( (PCONTROL)pcList, name );
	if( hli )
		SetItemData( hli, (PTRSZVAL)texture );
	return INVALID_INDEX;
}

void CPROC DelTexture( PTRSZVAL unused, PCONTROL button )
{
	PCONTROL pcList = GetNearControl( button, LST_TEXTURES );
	PLISTITEM hli = GetSelectedItem( pcList );
	if( hli )
	{
		PFLATLAND_TEXTURE texture = (PFLATLAND_TEXTURE)GetItemData( hli );
		if( texture->refcount )
		{
			SimpleMessage( WIDE("Texture is referenced - cannot delete") );
			return;
		}
		//DeleteTexture( texture, display->pWorld );
		DeleteListItem( pcList, hli );
	}
}

void ShowSectorProperties( PCOMMON pc, int nSectors, INDEX *ppsectors, int x, int y )
{
	int OK = FALSE, Fail = FALSE;
	TEXTCHAR oldname[256];
	//char text[256];
	PCOMMON pf;
	PDISPLAY display = ControlData( PDISPLAY, pc );
	PCONTROL pcName, pcVertical, pcList;
	pf = CreateFrame( WIDE("Sector Properties"), 0, 0, 255, 235, 0, pc );
	MoveFrame( pf, x, y );

	if( nSectors == 1 )
	{
		//PWORLD world = GetSetMember( WORLD, &g.worlds, display->pWorld );
		GetNameText( display->pWorld, GetSectorName( display->pWorld, ppsectors[0] )
				, oldname, sizeof( oldname ) );
		pcName = MakeEditControl( pf, 50, 5, 200, 14, EDT_NAME, oldname, 0 );
		MakeTextControl( pf, 5, 6, 45, 12, -1, WIDE("Name:"), 0 );
		pcVertical = MakeCheckButton( pf, 5, 24, 78, 14, CHK_INVERT, WIDE("Vertical"), 0, NULL, 0 );
		//if( ppsectors[0]->iName )
		//	SetCheckState( pcVertical, ppsectors[0]->name->flags.bVertical );
	}
	pcList = MakeListBox( pf, 5, 42, 170, 100, LST_TEXTURES, 0 );
	SetCommonUserData( pf, (PTRSZVAL)display );
	ForAllTextures( display->pWorld, AddTextureName, (PTRSZVAL)pcList );

	if( nSectors == 1 )
	{
		TEXTCHAR name[256];
		PLISTITEM hli;
		GetTextureNameText( display->pWorld, GetSectorTexture( display->pWorld, ppsectors[0] ), name, 256 );
		hli = FindListItem( pcList, name );
		if( hli )
			SetSelectedItem( pcList, hli );
	}
	// need to set current texture name from first sector
	// didn't think of set item focus before :) 

	{
		struct arg_tag{
			PDISPLAY display;
			PCOMMON parent;
		} args;
		args.display = display;
		args.parent = pf;
		MakeButton( pf, 5, 147, 105, 19, BTN_NEWTEXTURE, WIDE("New Texture"), 0, NewTexture, (PTRSZVAL)&args );
	}
	MakeButton( pf, 5, 169, 105, 19, BTN_TEXTURECOLOR, WIDE("Texture Color"), 0, SetTextureColor, display->pWorld );
	MakeButton( pf, 115, 147, 115, 19, BTN_DELTEXTURE, WIDE("Delete Texture"), 0, DelTexture, 0 );

	AddCommonButtons( pf, &Fail, &OK );

	if( nSectors == 1 )
		SetCommonFocus( pcName );

	DisplayFrame( pf );

	CommonWait( pf );

	if( OK )
	{
		TEXTCHAR newname[256];
		PLISTITEM hli;
		if( nSectors == 1 )
		{
			GetCommonTextEx( (PCOMMON)pcName, newname, 256, TRUE );
			if( strlen( newname ) )
			{
				if( StrCmp( oldname, newname ) )
				{
					INDEX iName = MakeName( display->pWorld, newname );
					SetSectorName( display->pWorld, ppsectors[0], iName );
				}				
				//ppsectors[0]->name->flags.bVertical = GetCheckState( pcVertical );
			}
			else
				SetSectorName( display->pWorld, ppsectors[0], INVALID_INDEX );
		}
		hli = GetSelectedItem( pcList );
		if( hli ) // else no change even on Okay...
		{
			 int i;
			 INDEX texture = (INDEX)GetItemData( hli );
			 for( i = 0; i < nSectors; i++ )
				 SetTexture( display->pWorld, ppsectors[i], texture );
		 }
	}

	DestroyFrame( &pf );
}
