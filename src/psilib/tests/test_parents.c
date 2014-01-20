
#include <psi.h>


PSI_CONTROL frame;
PSI_CONTROL children[5];

int main( void )
{
	int done = 0;
   int okay = 0;
   lprintf( "--------------- create parent ---------------------" );
	frame = CreateFrame( "whatver", 0, 0, 0, 0, BORDER_NONE|BORDER_NOCAPTION, NULL );
	AddCommonButtons( frame, &done, &okay );
   lprintf( "--------------- create child ---------------------" );
	children[0] = CreateFrame( "child 1", 400, 0, 0, 0, BORDER_NORMAL, frame );
	AddCommonButtons( children[0], &done, &okay );
	lprintf( "---------------------- show parent ------------------------------" );
   DumpFrameContents( frame );
	DisplayFrame( frame );
   lprintf( "--------------------- show child -----------------------------" );
   DumpFrameContents( children[0] );
	DisplayFrameOver( children[0], frame );
	CommonWait( children[0] );
	DestroyFrame( &children[0] );
	okay = 0;
   done = 0;
	CommonWait( frame );
   DestroyFrame( &frame );


	okay = 0;
   done = 0;

   lprintf( "--------------- create parent ---------------------" );
	frame = CreateFrame( "whatver", 0, 0, 0, 0, BORDER_NORMAL, NULL );
	AddCommonButtons( frame, &done, &okay );
   lprintf( "--------------- create child ---------------------" );
	children[0] = CreateFrame( "child 1", 400, 0, 0, 0, BORDER_NORMAL, frame );
	AddCommonButtons( children[0], &done, &okay );
	lprintf( "---------------------- show parent ------------------------------" );
   DumpFrameContents( frame );
	DisplayFrame( frame );
   lprintf( "--------------------- show child -----------------------------" );
   DumpFrameContents( children[0] );
	DisplayFrameOver( children[0], frame );
	CommonWait( children[0] );
	DestroyFrame( &children[0] );
	okay = 0;
   done = 0;
	CommonWait( frame );
   DestroyFrame( &frame );

	okay = 0;
   done = 0;

	frame = CreateFrame( "whatver", 0, 0, 0, 0, BORDER_NORMAL, NULL );
	AddCommonButtons( frame, &done, &okay );
   DisplayFrame( frame );
	children[0] = CreateFrame( "child 1", 400, 0, 0, 0, BORDER_NORMAL, frame );
	AddCommonButtons( children[0], &done, &okay );
	DisplayFrameOver( children[0], frame );
	CommonWait( children[0] );
	DestroyFrame( &children[0] );
	okay = 0;
   done = 0;
	CommonWait( frame );
   DestroyFrame( &frame );



	okay = 0;
   done = 0;

	frame = CreateFrame( "whatver", 0, 0, 0, 0, BORDER_NORMAL, NULL );
	AddCommonButtons( frame, &done, &okay );
	children[0] = CreateFrame( "child 1", 400, 0, 0, 0, BORDER_NORMAL, NULL );
	AddCommonButtons( children[0], &done, &okay );
	DisplayFrame( frame );
	DisplayFrameOver( children[0], frame );
	CommonWait( children[0] );
	DestroyFrame( &children[0] );
	okay = 0;
   done = 0;
	CommonWait( frame );
   DestroyFrame( &frame );

	okay = 0;
   done = 0;

	frame = CreateFrame( "whatver", 0, 0, 0, 0, BORDER_NORMAL, NULL );
	AddCommonButtons( frame, &done, &okay );
   DisplayFrame( frame );
	children[0] = CreateFrame( "child 1", 400, 0, 0, 0, BORDER_NORMAL, NULL );
	AddCommonButtons( children[0], &done, &okay );
	DisplayFrameOver( children[0], frame );
	CommonWait( children[0] );
	DestroyFrame( &children[0] );
	okay = 0;
   done = 0;
	CommonWait( frame );
   DestroyFrame( &frame );

	return 0;
}
