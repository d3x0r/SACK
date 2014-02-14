
#include <stdhdrs.h>
#include <psi/console.h>

int main( int argc, char **argv )
{
	PVARTEXT pvt = VarTextCreate();
	TEXTSTR result;
	int n;
	for( n= 1; n < argc; n++ )
		vtprintf( pvt, WIDE("%s%s"), n==1?WIDE(""):WIDE(" "), argv[n] );

   //lprintf( "-------------------- BEGIN ----------------------" );
   //DebugDumpMemEx( 1 );

	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
   Release( result );
	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
	Release( result );

   //lprintf( "-------------------- END ----------------------" );
	//DebugDumpMemEx( 1 );

   return 0;
}



