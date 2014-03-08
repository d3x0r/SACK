
#include <stdhdrs.h>
#include <psi/console.h>

int main( int argc, char **argv )
{
	PVARTEXT pvt = VarTextCreate();
	TEXTSTR result;
	int n;
	for( n= 1; n < argc; n++ )
		vtprintf( pvt, WIDE("%s%s"), n==1?WIDE(""):WIDE(" "), argv[n] );

	printf( "Test input: (%s)\n", GetText( VarTextPeek( pvt ) ) );

	//lprintf( "-------------------- BEGIN ----------------------" );
	//DebugDumpMemEx( 1 );

	FormatTextToBlock( GetText( VarTextPeek( pvt ) ), &result, 20, 5 );
	printf( "output : (%s)\n", result );
	Release( result );
#if 0
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
#endif
	//lprintf( "-------------------- END ----------------------" );
	//DebugDumpMemEx( 1 );

	return 0;
}



