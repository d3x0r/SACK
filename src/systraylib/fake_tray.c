#include <sharemem.h>
#include <deadstart.h>
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <image.h>
#include <controls.h>
//#include <buttons.h>
//  #include <pssql.h>
//  #include <getopt.h>
#include <systray.h>
#include "msgid.h"

typedef struct {
   uint32_t x, y;
} ICON_LOCATION, *PICON_LOCATION;

typedef struct{
	uint32_t width;
	uint32_t height;
}MAX;



typedef struct {
	PCOMMON pFrame;
	PCOMMON control;
   Image image;
   ICON_LOCATION coord;
	MAX dim;
   MAX off;
} TRAY_ICON, *PTRAY_ICON;


typedef struct {

	struct{
		MAX max;
      MAX dim;
	}win;


	struct{
		uint32_t bInited:1;
		uint32_t bDisplayed:1;
	}flags;

	struct{
		struct{
			uint32_t connected:1;
			uint32_t disconnected:1;
		}flags;
      uint32_t MsgBase;
	}client;
	TRAY_ICON icon;
   //
} GLOBAL;

#define g global_icon_data
static GLOBAL g;

//************************************************************************
//  HandleEvents, the callback function for msgsvr to handle events.
//
//
//
//
//  PARAMETERS:		Name		Description
//					------------------------------------------------------
//             uint32_t source    The numerical identifier of the calling process
//             uint32_t MsgID     messages are enumerated:
//                             1 = Mate Started
//                             2 = Mate Ended
//                           >=4 = Application Defined (in this case, try msgid.h
//             uint32_t *params   A pointer to anything, this is Application Defined, too.
//             uint32_t param_length  The length of the data pointed to.
//             uint32_t *result   The callback must write to this buffer to acknowledge the event.
//             uint32_t *result_length The length of the acknowledge.
//
//
//************************************************************************
//  instead of icondoitlib make it this lib fake_tray.c  in the makefile call it systrayclient and a lib


#include <msgclient.h> 

int RegisterIconEx( char *icon DBG_PASS )
{
   Image imgTemp = NULL;
   xlprintf(LOG_ALWAYS)("Welcome to fake_tray's RegisterIconEx for %s", icon );
//icon = Allocate( sizeof( *icon ) ); //?? why use a pointer??
	if( !g.flags.bInited)
	{
		g.icon.pFrame = CreateFrame( WIDE("icon")
											, 10, 10 //ok, this is still hard coded, for really no good reason right now.
											, g.win.max.width,	g.win.max.height
											, BORDER_NOMOVE | BORDER_INVERT | BORDER_NOCAPTION|BORDER_BUMP, NULL
											);
		g.flags.bInited = 1;
      g.flags.bDisplayed = 0;
	}
	// add , 0, 0, 0, 0 at the end cause controls expect parameters...
	// should really revise this someday...
	//icon->image = LoadImageFile( icon_name );

	imgTemp = LoadImageFile( icon );
          
//	if( !icon->image )
	if( !imgTemp )
	{
		 char icon_name[244];
		 char displaybuffer[244];
		 int len, offset, retval, x;
 
		 memcpy(icon_name, icon, 244 );
		 memset( displaybuffer,  ' '  , (sizeof (displaybuffer) ));
 
		 offset  = x = retval = 0;
 
		 if( ( len = strlen( icon_name) ) > 6 )
		 {
			 char szBuffer[12]; //shouldn't be larger than this....
 
			 while( ( ( len + offset)  - retval )  > 0 )
			 {
 
				 strncpy( szBuffer, icon_name + ( retval - offset)  , 7);
 
				 xlprintf(LOG_NOISE)("\n szBuffer(%d) is now %s  and len(%d)-retval(%d) is %d"
										  , (strlen(szBuffer))
										  , szBuffer
										  , len
										  , retval
										  , (len - retval)
										  );
				 retval += sprintf( displaybuffer +   retval
										,  "A%s\n"
										, szBuffer
										);
 
				 offset += 2;
				 xlprintf(LOG_NOISE)("\n the display buffer is now \n\n%s\n", displaybuffer );
			 }
 
			 xlprintf(LOG_NOISE)( WIDE("\nMy display buffer is now\n\n%s\n\n"), displaybuffer);
 
			 x++;
		 }// if( ( len = strlen( icon_name) ) > 6 )
		 else
		 {
			 retval = sprintf( displaybuffer
								  ,  "A%s\n"
								  , icon_name
								  );
		 }
 
 
		 displaybuffer[++retval] = 0;
		 displaybuffer[++retval] = 0;  //shagadellic! do it again!
 
		 xlprintf(LOG_NOISE)("There is no such file as %s making a button"
								  , icon_name );
 
 
								  //#define      MakeButton(f,x,y,w,h,id,c,a,p,d)
		 g.icon.control = MakeButton( g.icon.pFrame  //icon->pFrame
											 , g.icon.coord.x, g.icon.coord.y
											 , g.icon.dim.width, g.icon.dim.height
											 , -1
											 , displaybuffer
											 , 0
											 , 0
											 , 0
											 );
 
		 if( g.icon.control )
			 retval = 1;
		 else
			 xlprintf(LOG_NOISE)("Could not make button for %s"
									  , icon_name
									  );

	}
	else
	{
      uint32_t w = 0, h = 0;
		xlprintf(LOG_NOISE)("Found %s making image button, blotting."
				  , icon
				  );


		g.icon.image = MakeImageFile(g.icon.dim.width, g.icon.dim.height);

  		BlotScaledImage( g.icon.image
							 , imgTemp
							 );

//#define MakeImageButton(f,x,y,w,h,id,c,a,p,d)
 		g.icon.control = MakeImageButton( g.icon.pFrame//icon->pFrame
  												  , g.icon.coord.x, g.icon.coord.y
  												  , g.icon.dim.width, g.icon.dim.height
  												  , -1
  												  , g.icon.image
  												  , 0
  												  , 0
  												  , 0
  												  );
	}
	xlprintf(LOG_NOISE)("g.icon.control is now %p drawn at X%lu x Y%lu"
							 , g.icon.control
							 , g.icon.coord.x
							 , g.icon.coord.y
							 );

	g.icon.coord.y += ( g.icon.dim.height + g.icon.off.height );
	if ( g.icon.coord.y > g.win.max.height)
	{
		g.icon.coord.y = 0;
		g.icon.coord.x += ( g.icon.dim.width + g.icon.off.width ) ;
		if( g.icon.coord.x > g.win.max.width )
		{
			g.icon.coord.x = 0; //boo hoo.
		}

	}//if ( g.icon.coord.y > 650)

	if( g.flags.bDisplayed )
	{

		xlprintf(LOG_NOISE)("SmudgeCommoning %p and %p"
								 , g.icon.pFrame
								 , g.icon.control
								 );

      SmudgeCommon( g.icon.control );
      SmudgeCommon( g.icon.pFrame );
	}
	else
	{
		DisplayFrame( g.icon.pFrame );
      g.flags.bDisplayed = 1;
	}

 
	return 99;
}



//  void UnregisterIcon( void )
//  {
//     // uhmm do something here...
//  }

void UnregisterIconEx( char *a DBG_PASS )
{

	if( !TransactServerMessage( g.client.MsgBase + MSG_UnregisterIcon
//									  , icon , (strlen(icon)+1)
									  , a, ( strlen( a ) + 1 )
									  , NULL, NULL, 0 ) )
	{
	}
	else
	{
		xlprintf(LOG_NOISE)("Consider the icon unregistered." );
	}

}

void ChangeIconEx( char *icon DBG_PASS )
{
   lprintf( WIDE("Okay, update the image here...") );
   return;
}


PRELOAD( SysTrayInit )
{
	g.icon.coord.x = g.icon.coord.y = g.flags.bDisplayed = g.flags.bInited = 0;
	g.icon.image =  (Image)NULL;
	g.icon.control = (PCOMMON)NULL;
	g.win.max.width = 200;
	g.win.max.height = 700;
	g.icon.dim.width = 80;
	g.icon.dim.height = 80;
	g.icon.off.height = g.icon.off.width = 20;

}
