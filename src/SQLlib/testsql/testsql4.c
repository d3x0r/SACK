#include <stdhdrs.h>
#include <stdio.h>
#include <pssql.h>
#include <sharemem.h>

int main( int argc, char **argv )
{
	//TEXTCHAR buf[4096];
	//int offset = 0;
	//CTEXTSTR select_into;
	//PLIST output = NULL;
	//PLIST outputs = NULL;
	//SetAllocateDebug( TRUE );
	//SetAllocateLogging( TRUE );
	SetHeapUnit( 4096 * 1024 ); // 4 megs expansion if needed...
	{
		CTEXTSTR *results;
      DoSQLRecordQueryf( NULL, &results,NULL, "show create table link_hall_state" );
      GetFieldsInSQL( results[1], 0 );
	}
	return 0;
}


