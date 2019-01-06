#include "sack_ucb_full.h"


void callNewTimeTest() {
	int z;
	for( z = 0; z < 10; z++ ) {
		uint64_t time = GetTimeOfDay();
		int tmp = ((int8_t)time*15);
		int sign = tmp < 0 ?  -1 : 1;
		int tz = GetTimeZone();
		if( tmp < 0 ) tmp = -tmp;
		tmp = sign * ( ( tmp / 60 ) * 100 + ( tmp % 60 ) );
		printf( "time? %" PRId64" timezone:%d   realms:%"PRId64"  recovTZ:%d\n", time, tz, time>>8, tmp );
		//Sleep(1);
	}


	{
		uint64_t tick = GetTimeOfDay();
		uint64_t tick2;
		uint64_t tick3;
		uint64_t tick4;
		SACK_TIME st;
		SACK_TIME st2;
		SACK_TIME st3;
		ConvertTickToTime( tick, &st );
		tick2 = ConvertTimeToTick( &st );
		ConvertTickToTime( tick2, &st2 );
		tick3 = ConvertTimeToTick( &st2 );
		ConvertTickToTime( tick3, &st3 );
		tick4 = ConvertTimeToTick( &st3 );
		if( tick != tick4 ) printf( "FATALITY\n" );

		printf( "%" PRId64 "\n", tick4 );
		printf( "%02d/%02d/%4d %02d:%02d:%02d:%03d  %d  %d\n", st.mo, st.dy, st.yr, st.hr, st.mn, st.sc, st.ms, st.zhr, st.zmn );
		printf( "%02d/%02d/%4d %02d:%02d:%02d:%03d  %d  %d\n", st3.mo, st3.dy, st3.yr, st3.hr, st3.mn, st3.sc, st3.ms, st3.zhr, st3.zmn );
	}


	{
		int64_t testTick = 395983339959264ULL;
		SACK_TIME testTime = { 715, 21, 25, 21, 6, 1, 2019, -8, 0 };
		SACK_TIME outTime;
		int64_t outTick;
		outTick = ConvertTimeToTick( &testTime );
		ConvertTickToTime( testTick, &outTime );
		if( testTick != outTick ) 
			printf( "FAILURE TO CONVERT TO TICK\n" );
		if( memcmp( &testTime, &outTime, sizeof( SACK_TIME ) ) ) 
			printf( "FAILURE TO CONVERT TO TIME\n" );
	}

}

int main( void ) {
	PLIST list = NULL;
	INDEX idx;
	char *name;
	AddLink( &list, "asdf" );
	LIST_FORALL( list, idx, char *, name ) {
		printf( "list has: %d = %s\n", idx, name );
	}

	callNewTimeTest();

	return 0;
}
