#define CONSOLE_SHELL
#include "sack_ucb_filelib.h"

static void salt_generator(uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	static struct tickBuffer {
		uint32_t tick;
		uint64_t cputick;
	} tick;
	(void)psv;
	tick.cputick = GetCPUTick();
	tick.tick = GetTickCount();
	salt[0] = &tick;
	salt_size[0] = sizeof( tick );
}


SaneWinMain( argc, argv )
{
	char message[] = "This is a test, This is Only a test." ;
	char output[sizeof(message)];
	char output2[sizeof(message)];
	SetSystemLog( SYSLOG_FILE, stdout );
	{
		struct byte_shuffle_key *key = BlockShuffle_ByteShuffler( SRG_CreateEntropy4(salt_generator,0) );
		BlockShuffle_SubBytes( key, message, output, sizeof( message ) );
		BlockShuffle_BusBytes( key, output, output2, sizeof( message ) );

		printf( "Fun:%s\n", output2 );
	}

	{
		struct block_shuffle_key *key = BlockShuffle_CreateKey( SRG_CreateEntropy4(salt_generator,0), sizeof(message), 1 );
		
		BlockShuffle_SetData( key, output, 0, sizeof(message) , message, 0 );
		LogBinary( output, sizeof( output ) );
		printf( "SD Fun:%s %d\n", output );
		BlockShuffle_GetData( key, output2, 0, sizeof(message) , output, 0 );
		printf( "Fun:%s\n", output2 );
	}

   return 0;
}
EndSaneWinMain();
