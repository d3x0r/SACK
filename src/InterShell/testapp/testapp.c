
/*
*
* This is a test application which demonstrates
* how to communicate between components
*
*
*/

#include <controls.h>
#include <stdhdrs.h>
#include <stdio.h>
#include <pssql.h>
#include "InterShell_export.h"
#include "InterShell_registry.h"
#include "widgets/buttons.h"

#define MODULE_NAME "TestApp"


#define OnCreateSlider(name) \
	DefineRegistryMethod(TASK_PREFIX,CreateSlider,"control",name,"slider_create",uintptr_t,(PSI_CONTROL))




enum {
	BTN_PLUS = 4000
	, BTN_MINUS
	, LISTBOXSAMPLE
};


static struct {
	PLIST controllist;      //samle list 
	uint32_t   value;           //current slider's value
} l;





PRELOAD( RegisterTaskControls )
{

	EasyRegisterResource( "InterShell/TestApp", BTN_PLUS , NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/TestApp", BTN_MINUS , NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/TestApp", LISTBOXSAMPLE    , LISTBOX_CONTROL_NAME );

	DoSQLCommand("use database");

}

//---------------------------------------------------------------------------




OnCreateMenuButton( MODULE_NAME "/button plus" )( PMENU_BUTTON button )
{
//	InterShell_SetButtonImage( button, "images/plus.png");
//	InterShell_SetButtonAnimation( button, "images/AnimationWizard1.mng");
//	UpdateButton( button );

	return (uintptr_t)button;
}



OnCreateMenuButton( MODULE_NAME "/button minus" )( PMENU_BUTTON button )
{
//	InterShell_SetButtonImage( button, "images/minus.png");

	InterShell_SetButtonAnimation( button, "images/AnimationWizard1.mng");
	UpdateButton( button );

	return (uintptr_t)button;
}


OnCreateSlider( MODULE_NAME "/Slider" )( PSI_CONTROL slider )
/*
* It really doesn't work, slider is in separate isom library
*/
{
	lprintf("I am here: OnCreateSlider");
	AddLink( &l.controllist, slider );

	return 1;
}

OnCreateListbox( MODULE_NAME "/samplelist" )( PSI_CONTROL listbox )
{

	AddLink( &l.controllist, listbox );

	return (uintptr_t)listbox;
}




OnSelectListboxItem( MODULE_NAME "/samplelist", MODULE_NAME "" )( uintptr_t psvList, PLISTITEM pli )
/*
* Update number of selected item
*/
{
	uint32_t n = 0;
	PLISTITEM pli_;
	PSI_CONTROL list = (PSI_CONTROL)psvList;

	for( n = 0; ; n++)
	{
		pli_ = GetNthItem(list, n);

		if(pli_ == pli)
		{
			l.value = n;
			break;
		}
	}

}



OnShowControl( MODULE_NAME "/samplelist" )( uintptr_t psvList )
/*
* Populate list box whith data fetched from table system 
*/
{
	PLISTITEM pli;
	int cols;
	CTEXTSTR *results;

	PSI_CONTROL list = (PSI_CONTROL)psvList;

	ResetList( list );


	for( DoSQLRecordQueryf( &cols, &results, NULL
		, "select name from systems limit 100"
		);
	results;
	GetSQLRecord( &results ) )
	{
		char buffer[256];

		if( results[0]) 
		{ 
			snprintf( buffer, sizeof( buffer ), "%s", results[0] );
			AddListItem( list, buffer );
		}
	}

	//Higlight firt element on the list
	l.value = 0;
	pli = GetNthItem(list, l.value);
	SetSelectedItem( list, pli );

}


OnKeyPressEvent( MODULE_NAME "/button plus" )( uintptr_t psvButton )
/*
* Highlight next element on the list
*/
{
	PLISTITEM pli;
	PSI_CONTROL list;
	INDEX idx;
	PMENU_BUTTON button = (PMENU_BUTTON)psvButton;
	static int  animplay = 0;


	LIST_FORALL( l.controllist, idx, PSI_CONTROL, list )   //Search for listbox control
	{
		if(list)
		{
			pli = GetNthItem(list, l.value + 1); //Get pointer to next element

			if( pli ) 
			{
				l.value++; 
				SetSelectedItem( list, pli);
			}
			break;

		}
	}

	if(!animplay)
		InterShell_SetButtonAnimation( button, "images/AnimationWizard1.mng");
	else
		InterShell_SetButtonAnimation( button, "");

    ++animplay;
	animplay &= 1;

	UpdateButton( button );

}





OnKeyPressEvent( MODULE_NAME "/button minus" )( uintptr_t psvButton )
/*
* Highlight previous element on the list
*/
{
	PLISTITEM pli;
	PSI_CONTROL list;
	INDEX idx;


	LIST_FORALL( l.controllist, idx, PSI_CONTROL, list )   //Search for listbox control
	{
		if(list)
		{
			pli = GetNthItem(list, l.value - 1); //Get pointer to previous element

			if( pli ) 
			{
				l.value--;
				SetSelectedItem( list, pli);  
			}
			break;
		}
	}

}

