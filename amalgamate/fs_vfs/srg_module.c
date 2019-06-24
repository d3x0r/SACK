
#include "common.h"


static struct SRG_local_daa {
    PLINKQUEUE signingEntropies;
    int signingThreads;
	struct random_context *entropy;
    struct random_context *(*makeEntropy)(void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user);
} srgl;

struct SRGObject {

	char *seedBuf;
	size_t seedLen;
	struct random_context *entropy;
    struct random_context *(*makeEntropy)(void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user);
	int signingThreads;
	//static v8::Persistent<v8::Function> constructor;
    int seedCallback;
	//Persistent<Function, CopyablePersistentTraits<Function>> *seedCallback;
	//Isolate *isolate;
	//Persistent<Array> seedArray;
	PLINKQUEUE SigningEntropies;
};

#if 0
	static void Init( Isolate *isolate, Local<Object> exports );
	SRGObject( Persistent<Function, CopyablePersistentTraits<Function>> *callback );
	SRGObject( const char *seed, size_t seedLen );
	SRGObject();
#endif

static int vfs_u8xor(char const *xor1, size_t xorlen, char const *keystr, size_t keylen, int step, int key ) EMSCRIPTEN_KEEPALIVE;
int vfs_u8xor( char const *xor1, size_t xorlen, char const *keystr, size_t keylen, int step, int key ){
	char *out = u8xor( xor1, (size_t)xorlen, keystr, keylen, &step );
	//lprintf( "-------- INPUT STRING: %d %d", xorlen, keylen );
	//LogBinary( (uint8_t*)xor1, xorlen );
	//lprintf( "-------- OUTPUT STRING:");
	//LogBinary( (uint8_t*)out, xorlen );
	SET( key, "step", step );
	int result = makeString( out, xorlen );
	Deallocate( char*, out );
	return result;	
}

	static int idGenerator( const char *val, size_t valLength, int version ) EMSCRIPTEN_KEEPALIVE;
	int idGenerator( const char *val, size_t valLength, int version ) {
            if( val ) {
				char *r;
				struct random_context *ctx;
				switch( version ) {
				case 0:
					ctx = SRG_CreateEntropy( NULL, 0 );
					break;
				case 1:

					ctx = SRG_CreateEntropy2_256( NULL, 0 );
					break;
				case 2:
					ctx = SRG_CreateEntropy3( NULL, 0 );
					break;
				default:
				case 3:
					ctx = SRG_CreateEntropy4( NULL, 0 );
					break;
				}

				/* Regenerator version 0 */
				uint32_t buf[256 / 32];
				SRG_FeedEntropy( ctx, (uint8_t*)"\0\0\0\0", 4 );
				SRG_StepEntropy( ctx );
				SRG_FeedEntropy( ctx, (uint8_t*)val, valLength );
				SRG_StepEntropy( ctx );
				SRG_GetEntropyBuffer( ctx, buf, 256 );

				size_t outlen;
				r = EncodeBase64Ex( (uint8_t*)buf, (16 + 16), &outlen, (const char*)1 );
				SRG_DestroyEntropy( &ctx );
                int s = makeString( r, outlen );
				Release( r );
				return s;
			}
			else
			{
				char *r;
				// these are random, and don't specifically need to behave in any way.
				switch( version ) {
				case 0:
					r = SRG_ID_Generator();
					break;
				case 1:
					r = SRG_ID_Generator_256();
					break;
				case 2:
					r = SRG_ID_Generator3();
					break;
				default:
				case 3:
					r = SRG_ID_Generator4();
					break;
				}
                int s = makeString( r, strlen(r) );
				Release( r );
				return s;
			}
	}


	static void srg_getSeed( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
		struct SRGObject* obj = (struct SRGObject*)psv;
		if( obj->seedBuf ) {
			Deallocate( char *, obj->seedBuf );
			obj->seedBuf = NULL;
			obj->seedLen = 0;
		}
		if( obj->seedCallback ) {
            lprintf( "Still need to implement callback");
            EM_ASM( {
                var arr = []; Module.this_.callbacks[$0]( arr );
                var maxlen = 0;
                for( var n = 0; n < arr.length; n++ ) {
                    if( typeof( arr[n]) === "string")
    				    arr[n] = intArrayFromString(arr[n]);
                    else if( !("length" in arr[n]) )
    				    arr[n] = intArrayFromString(arr[n].toString());
                    maxlen += arr[n].length-1;
                }
                var buf = Module._Allocate( maxlen );
                maxlen = 0;
                for( var n = 0; n < arr.length; n++ ) {
                    for( var m = 0; m < arr[n].length; m++ ) {
                        Module.U8HEAP[maxlen+m] = arr[n][m];                    
                    }
                    maxlen += arr[n].length;
                }
                Module.U32HEAP[$1] = buf;
                Module.U32HEAP[$2] = maxlen;
            }, obj->seedCallback-1, salt, salt_size );
		}
		if( obj->seedBuf ) {
			salt[0] = (POINTER)obj->seedBuf;
			salt_size[0] = obj->seedLen;
		}
		else
			salt_size[0] = 0;
	}

	void setVersion( struct SRGObject *obj, int version ) EMSCRIPTEN_KEEPALIVE;
    void setVersion( struct SRGObject *obj, int version ) 
    {   
        if( obj ) {
			//lprintf( "Make will be %d?", version );
			if( version == 0 )
				obj->makeEntropy = SRG_CreateEntropy;
			if( version == 1 )
				obj->makeEntropy = SRG_CreateEntropy2_256;
			if( version == 2 )
				obj->makeEntropy = SRG_CreateEntropy3;
			if( version == 3 )
				obj->makeEntropy = SRG_CreateEntropy4;

			obj->entropy = obj->makeEntropy( srg_getSeed, (uintptr_t)obj );
        } else {
			//lprintf( "Make will be %d?", version );
			if( version == 0 )
				srgl.makeEntropy = SRG_CreateEntropy;
			if( version == 1 )
				srgl.makeEntropy = SRG_CreateEntropy2_256;
			if( version == 2 )
				srgl.makeEntropy = SRG_CreateEntropy3;
			if( version == 3 )
				srgl.makeEntropy = SRG_CreateEntropy4;

			srgl.entropy = srgl.makeEntropy( srg_getSeed, (uintptr_t)0 );

        }
	}
	void reset( struct SRGObject *obj )  EMSCRIPTEN_KEEPALIVE;
	void reset( struct SRGObject *obj ) {
		SRG_ResetEntropy( obj->entropy );
	}
	void seed( struct SRGObject *obj, char *seed ) EMSCRIPTEN_KEEPALIVE;
	void seed( struct SRGObject *obj, char *seed ) {
			if( obj->seedBuf )
				Deallocate( char *, obj->seedBuf );
			obj->seedBuf = StrDup( seed );
			obj->seedLen = strlen( seed );
	}
	int32_t getBits( struct SRGObject *obj, int bits, int sign ) EMSCRIPTEN_KEEPALIVE;
	int32_t getBits( struct SRGObject *obj, int bits, int sign ) {
		return SRG_GetEntropy( obj->entropy, bits, sign );
	}
	void getBuffer( struct SRGObject *obj, int32_t bits, uint32_t *buffer ) EMSCRIPTEN_KEEPALIVE;
	void getBuffer( struct SRGObject *obj, int32_t bits, uint32_t *buffer ) {
			SRG_GetEntropyBuffer( obj->entropy, buffer, bits );
	
	}

	void deleteSRGObject( struct SRGObject *obj ) EMSCRIPTEN_KEEPALIVE;
	void deleteSRGObject( struct SRGObject *obj ) {
		SRG_DestroyEntropy( &obj->entropy );
        Release( obj );
	}

	struct signature {
		const char *id;
		int extent;
		int classifier;
	};


    struct bit_count_entry {
        uint8_t ones, in0, in1, out0, out1, changes;
    };
    static struct bit_count_entry bit_counts[256];


	static int signCheck( uint8_t *buf, int del1, int del2, struct signature *s ) {
		int n;
		int is0 = bit_counts[buf[0]].in0 != 0;
		int is1 = bit_counts[buf[0]].in1 != 0;
		int long0 = 0;
		int long1 = 0;
		int longest0 = 0;
		int longest1 = 0;
		int ones = 0;
		int rval;
		//LogBinary( buf, 32 );
		for( n = 0; n < 32; n++ ) {
			struct bit_count_entry *e = bit_counts + buf[n];
			ones += e->ones;
			if( is0 && e->in0 ) long0 += e->in0;
			if( is1 && e->in1 ) long1 += e->in1;
			if( e->changes ) {
				if( long0 > longest0 ) longest0 = long0;
				if( long1 > longest1 ) longest1 = long1;
				if( long0 = e->out0 ) {
					is0 = 1;
					is1 = 0;
				} 
				else {
					is1 = 1;
					is0 = 0;
				}
				long1 = e->out1;
			}
			else {
				if( !is0 && e->in0 ) {
					if( long1 > longest1 ) longest1 = long1;
					long0 = e->out0;
					long1 = e->out1;
					is0 = 1;
					is1 = 0;
				}
				else if( !is1 && e->in1 ) {
					if( long0 > longest0 ) longest0 = long0;
					long0 = e->out0;
					long1 = e->out1;
					is0 = 0;
					is1 = 1;
				}
			}
		}
		if( long0 > longest0 ) longest0 = long0;
		if( long1 > longest1 ) longest1 = long1;

// 167-128 = 39 = 40+ dif == 30 bits in a row approx
#define overbal (167-128)
		if( longest0 > (29+del1) || longest1 > (29+del1) || ones > (128+overbal+del2) || ones < (128-overbal-del2) ) {
			if( ones > ( 128 + overbal + del2 ) ) {
				s->classifier = rval = 1;
				s->extent = ones-128 - overbal;
			} 
			else if( ones < (128 - overbal - del2) ) {
				s->classifier = rval = 2;
				s->extent = 128-ones - overbal;
			}
			else if( longest0 > ( 29 + del1 ) ) {
				s->classifier = rval = 3;
				s->extent = longest0 - 29;
			}
			else if( longest1 > (29 + del1) ) {
				s->classifier = rval = 4;
				s->extent = longest1 - 29;
			}
			else {
				s->classifier = rval = 5;
				s->extent = 0;
			}
			return rval;
		}
		s->classifier = 0;
		s->extent = 0;
		return 0;
	}


	struct signParams {
		PTHREAD main;
		struct random_context *signEntropy;
		POINTER state; // this thread's entropy working state
		//char *salt;  // this thread's salt.
		//int saltLen;
#ifdef DEBUG_SIGNING
		int tries;
#endif
		char *id;  // result ID
		int pad1;  // extra length 1 to try
		int pad2;  // extra length 2 to try
		int ended; // this thread has ended.
		int *done; // all threads need to be done.
	};


	static uintptr_t signWork( PTHREAD thread ) {
		struct signParams *params = (struct signParams *)GetThreadParam( thread );

		do {
			{
				size_t len;
				uint8_t outbuf[32];
				uint8_t *bytes;
				int passed_as;
				struct signature s;
				params->id = SRG_ID_Generator_256();
				bytes = DecodeBase64Ex( params->id, 44, &len, (const char*)1 );
				SRG_ResetEntropy( params->signEntropy );
				SRG_FeedEntropy( params->signEntropy, bytes, 32 );
				SRG_GetEntropyBuffer( params->signEntropy, (uint32_t*)outbuf, 256 );
				Release( params->id );
				params->id = EncodeBase64Ex( outbuf, 32, &len, (const char*)1 );
				SRG_RestoreState( params->signEntropy, params->state );
				SRG_FeedEntropy( params->signEntropy, outbuf, 32 );
				SRG_GetEntropyBuffer( params->signEntropy, (uint32_t*)outbuf, 256 );

#ifdef DEBUG_SIGNING
				params->tries++;
#endif
				if( (passed_as = signCheck( outbuf, params->pad1, params->pad2, &s )) ) {
#ifdef DEBUG_SIGNING
					lprintf( "FEED %s", params->id );
					LogBinary( bytes, len );
					lprintf( "GOT" );
					LogBinary( outbuf, 256 / 8 );
					printf( " %d  %s  %d\n", params->tries, params->id, passed_as );
#endif
				} 
				else {
					Release( params->id );
					params->id = NULL;
				}
				Release( bytes );
			}
		} while( !params->id && !params->done[0] );
		if( !params->done[0] ) {
			params->done[0] = TRUE;
			WakeThread( params->main );
		}
		params->ended = 1;
		return 0;
	}

	static void sign( struct SRGObject *srg, char *buf, int buflen, int pad1, int pad2 ) {
		static struct signParams threadParams[32];
		int found = 0;
#ifdef DEBUG_SIGNING
		int tries = 0;
#endif
		int done = 0;
		int n = 0;
		int argn = 1;
		int threads = srgl.signingThreads;
		POINTER state = NULL;

#ifdef DEBUG_SIGNING
		lprintf( "RESET ENTROPY TO START" );
		LogBinary( (const uint8_t*)*buf, buflen );
#endif

		for( n = 0; n < threads; n++ ) {
			threadParams[n].main = MakeThread();
			if( !threadParams[n].signEntropy )
				threadParams[n].signEntropy = srg?srg->makeEntropy(NULL, 0):srgl.makeEntropy( NULL, 0 );

			SRG_ResetEntropy( threadParams[n].signEntropy );
			SRG_FeedEntropy( threadParams[n].signEntropy, (const uint8_t*)buf, buflen );
			threadParams[n].state = NULL;
			SRG_SaveState( threadParams[n].signEntropy, &threadParams[n].state, NULL );
			//threadParams[n].salt = SRG_ID_Generator_256(); // give the thread a starting point
			//threadParams[n].saltLen = (int)strlen( threadParams[n].salt );
			threadParams[n].pad1 = pad1;
			threadParams[n].pad2 = pad2;
			threadParams[n].ended = 0;
#ifdef DEBUG_SIGNING
			threadParams[n].tries = 0;
#endif
			threadParams[n].done = &done;
			ThreadTo( signWork, (uintptr_t)(threadParams +n) );
		}
	}

    int verify( const char *buf, size_t buflen, const char *hash ) EMSCRIPTEN_KEEPALIVE;
	int verify( const char *buf, size_t buflen, const char *hash ) {
			//SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
			const char *id;
			int pad1 = 0, pad2 = 0;
			int n = 0;
            int result = JS_VALUE_UNDEFINED;
			int argn = 1;
			struct random_context *signEntropy = (struct random_context *)DequeLink( &srgl.signingEntropies );

			if( !signEntropy )
				signEntropy = srgl.makeEntropy( NULL, (uintptr_t)0 );
			SRG_ResetEntropy( signEntropy );
			SRG_FeedEntropy( signEntropy, (const uint8_t*)buf, buflen );
			{
				size_t len;
				uint8_t outbuf[32];
				uint8_t *bytes;
				id = hash;
				bytes = DecodeBase64Ex( id, 44, &len, (const char*)1 );
#ifdef DEBUG_SIGNING
				lprintf( "FEED INIT %s", id );
				LogBinary( buf, strlen(buf) );
				lprintf( "FEED" );
				LogBinary( bytes,len );
#endif
				SRG_FeedEntropy( signEntropy, bytes, len );
				Release( bytes );
				SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
#ifdef DEBUG_SIGNING
				lprintf( "GET" );
				LogBinary( outbuf, 256 / 8 );
#endif
				result = makeObject();
				struct signature s;
				signCheck( outbuf, pad1, pad2, &s );
				SET( result, "classifier", makeNumber( s.classifier ) );
				SET( result, (const char*)"extent", makeNumber( s.extent ) );
				char *rid = EncodeBase64Ex( outbuf, 256 / 8, &len, (const char *)1 );
				SET( result, (const char*)"key", makeString( rid, strlen(rid) ) );
			}
			EnqueLink( &srgl.signingEntropies, signEntropy );
            return result;
	}

	static void srg_sign_work( struct signParams threadParams[32], int *done, const uint8_t *buf, size_t bufLen, int pad1, int pad2 )
	{
		int n;
		//static signParams threadParams[32];
		for( n = 0; n < srgl.signingThreads; n++ ) {
			threadParams[n].main = MakeThread();
			if( !threadParams[n].signEntropy )
				threadParams[n].signEntropy = SRG_CreateEntropy4( NULL, 0 );

			SRG_ResetEntropy( threadParams[n].signEntropy );
			SRG_FeedEntropy( threadParams[n].signEntropy, buf, bufLen );
			threadParams[n].state = NULL;
			SRG_SaveState( threadParams[n].signEntropy, &threadParams[n].state, NULL );
			//threadParams[n].salt = SRG_ID_Generator_256(); // give the thread a starting point
			//threadParams[n].saltLen = (int)strlen( threadParams[n].salt );
			threadParams[n].pad1 = pad1;
			threadParams[n].pad2 = pad2;
			threadParams[n].ended = 0;
#ifdef DEBUG_SIGNING
			threadParams[n].tries = 0;
#endif
			threadParams[n].done = done;
			ThreadTo( signWork, (uintptr_t)(threadParams + n) );
		}

		while( !done ) {
			WakeableSleep( 500 );
		}
		for( n = 0; n < srgl.signingThreads; n++ ) {
			while( !threadParams[n].ended ) Relinquish();
#ifdef DEBUG_SIGNING
			tries += threadParams[n].tries;
#endif
		}
		int found = 0;
		for( n = 0; n < srgl.signingThreads; n++ ) {
			if( threadParams[n].id ) {
				if( found ) {
				}
				else {
#ifdef DEBUG_SIGNING
					lprintf( " %d  %s \n", tries, threadParams[n].id );
#endif
					//args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), threadParams[n].id ) );
					//threadParams[n].id
				}
				Release( threadParams[n].id );
				found++;
			}
			threadParams[n].id = NULL;
		}
		return;
	}

	static char * wait_for_signing( struct signParams threadParams[32], int *done ) {
		char *result;
		int n;
		while( !(*done) ) {
			WakeableSleep( 500 );
		}
		for( n = 0; n < srgl.signingThreads; n++ ) {
			while( !threadParams[n].ended ) Relinquish();
#ifdef DEBUG_SIGNING
			tries += threadParams[n].tries;
#endif
		}
		int found = 0;
		for( n = 0; n < srgl.signingThreads; n++ ) {
			while( !threadParams[n].ended )
				Relinquish();
			if( threadParams[n].id ) {
				if( found ) {
					Release( threadParams[n].id );
				}
				else {
#ifdef DEBUG_SIGNING
					lprintf( " %d  %s \n", tries, threadParams[n].id );
#endif
					result = threadParams[n].id;
				}
				found++;
			}
			threadParams[n].id = NULL;
		}
		return result;
	}


	static void srg_sign( struct SRGObject *srg, const char *buf, size_t buflen, int pad1, int pad2 ) {
		static struct signParams threadParams[32];
		int found = 0;
#ifdef DEBUG_SIGNING
		int tries = 0;
#endif
		int done = 0;
		int n = 0;
        int result = JS_VALUE_UNDEFINED;
		int argn = 1;
		int threads = srgl.signingThreads;
		POINTER state = NULL;

		srg_sign_work( threadParams, &done, (const uint8_t*)buf, buflen, pad1, pad2 );
#ifdef DEBUG_SIGNING
		lprintf( "RESET ENTROPY TO START" );
		LogBinary( (const uint8_t*)buf, buflen );
#endif

		while( !done ) {
			WakeableSleep( 500 );
		}
		for( n = 0; n < threads; n++ ) {
			while( !threadParams[n].ended ) Relinquish();
#ifdef DEBUG_SIGNING
			tries += threadParams[n].tries;
#endif
		}
		for( n = 0; n < threads; n++ ) {
			if( threadParams[n].id ) {
				if( found ) {
				}
				else {
#ifdef DEBUG_SIGNING
					lprintf( " %d  %s \n", tries, threadParams[n].id );
#endif
                    result = makeString( threadParams[n].id, strlen( threadParams[n].id ));
				}
				Release( threadParams[n].id );
				found++;
			}
			threadParams[n].id = NULL;
		}
	}

	static void srg_setSigningThreads( struct SRGObject *srg, int n ) {
        if( srg )
            srg->signingThreads = n;
        else
            srgl.signingThreads = n;
	}


	static int srg_verify( struct SRGObject *srg, const char *buf, size_t buflen, const char *hash, int pad1, int pad2 ) {
			//SRGObject *obj = ObjectWrap::Unwrap<SRGObject>( args.This() );
			const char *id;
			int n = 0;
			int argn = 1;
            int result = JS_VALUE_UNDEFINED;
			struct random_context *signEntropy = (struct random_context *)DequeLink( &srg->SigningEntropies );

			if( !signEntropy )
				signEntropy = srg->makeEntropy( NULL, (uintptr_t)0 );
			SRG_ResetEntropy( signEntropy );
			SRG_FeedEntropy( signEntropy, (const uint8_t*)buf, buflen );
			{
				size_t len;
				uint8_t outbuf[32];
				uint8_t *bytes;
				id = hash;
				bytes = DecodeBase64Ex( id, 44, &len, (const char*)1 );
				SRG_ResetEntropy( signEntropy );
				SRG_FeedEntropy( signEntropy, bytes, len );
				SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );

				SRG_ResetEntropy( signEntropy );
				SRG_FeedEntropy( signEntropy, (const uint8_t*)buf, buflen );

#ifdef DEBUG_SIGNING
				lprintf( "FEED INIT %s", id );
				LogBinary( (*buf), buflen );
				lprintf( "FEED" );
				LogBinary( bytes, len );
#endif
				SRG_FeedEntropy( signEntropy, outbuf, 32 );
				Release( bytes );
				SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
#ifdef DEBUG_SIGNING
				lprintf( "GET" );
				LogBinary( outbuf, 256 / 8 );
#endif
				result = makeObject();
				struct signature s;
				signCheck( outbuf, pad1, pad2, &s );
				SET( result, "classifier", makeNumber(s.classifier) );
				SET( result, "extent",  makeNumber(s.extent) );
				char *rid = EncodeBase64Ex( outbuf, 256 / 8, &len, (const char *)1 );
				SET( result, "key", makeString( rid, strlen(rid) ) );
				
			}
			EnqueLink( &srg->SigningEntropies, signEntropy );
            return result;
	}

uintptr_t makeSRGObject( int cb ) {
    struct SRGObject *obj = New( struct SRGObject );

	obj->makeEntropy = SRG_CreateEntropy2_256;
	obj->seedBuf = NULL;
	obj->seedLen = 0;
	obj->seedCallback = cb;

	obj->entropy = SRG_CreateEntropy2( srg_getSeed, (uintptr_t)obj );
    return (uintptr_t)obj;
}

uintptr_t makeBlankSRGObject(void) {
    struct SRGObject *obj = New( struct SRGObject );
	obj->makeEntropy = SRG_CreateEntropy2_256;
	obj->seedBuf = NULL;
	obj->seedLen = 0;
	obj->seedCallback = 0;
	obj->entropy = SRG_CreateEntropy2( srg_getSeed, (uintptr_t)obj );
    return (uintptr_t)obj;
}

uintptr_t makeSRGObjectWithSeed( const char *seed, size_t seedLen ) {
    struct SRGObject *obj = New( struct SRGObject );
	obj->makeEntropy = SRG_CreateEntropy2_256;
	obj->seedBuf = StrDup( seed );
	obj->seedLen = seedLen;
	obj->seedCallback = 0;
	obj->entropy = SRG_CreateEntropy2_256( srg_getSeed, (uintptr_t)obj );
    return (uintptr_t)obj;
}


static void InitBitCountLookupTables( void ) {
    int in0 = 0;
    int in1 = 0;
    int out0 = 0;
    int out1 = 0;
    int ones;
    int n;
    for( n = 0; n < 256; n++ ) {
        int changed;
        in0 = 0;
        in1 = 0;
        out0 = 0;
        out1 = 0;
        ones = 0;
        changed = 1;
        if( (n & (1 << 7)) ) ones++;
        if( (n & (1 << 6)) ) ones++;
        if( (n & (1 << 5)) ) ones++;
        if( (n & (1 << 4)) ) ones++;
        if( (n & (1 << 3)) ) ones++;
        if( (n & (1 << 2)) ) ones++;
        if( (n & (1 << 1)) ) ones++;
        if( (n & (1 << 0)) ) ones++;
        do {
            if( (n & (1 << 7)) ) {
                out1++;
                if( (n & (1 << 6)) ) out1++; else break;
                if( (n & (1 << 5)) ) out1++; else break;
                if( (n & (1 << 4)) ) out1++; else break;
                if( (n & (1 << 3)) ) out1++; else break;
                if( (n & (1 << 2)) ) out1++; else break;
                if( (n & (1 << 1)) ) out1++; else break;
                if( (n & (1 << 0)) ) out1++; else break;
                changed = 0;
            }
            else {
                out0++;
                if( !(n & (1 << 6)) ) out0++; else break;
                if( !(n & (1 << 5)) ) out0++; else break;
                if( !(n & (1 << 4)) ) out0++; else break;
                if( !(n & (1 << 3)) ) out0++; else break;
                if( !(n & (1 << 2)) ) out0++; else break;
                if( !(n & (1 << 1)) ) out0++; else break;
                if( !(n & (1 << 0)) ) out0++; else break;
                changed = 0;
            }
        } while( 0 );
        if( !changed ) {
            in1 = out1; in0 = out0;
        } else do {
            if( (n & (1 << 0)) ) {
                in1++;
                if( (n & (1 << 1)) ) in1++; else break;
                if( (n & (1 << 2)) ) in1++; else break;
                if( (n & (1 << 3)) ) in1++; else break;
                if( (n & (1 << 4)) ) in1++; else break;
                if( (n & (1 << 5)) ) in1++; else break;
                if( (n & (1 << 6)) ) in1++; else break;
                if( (n & (1 << 7)) ) in1++; else break;
            }
            else {
                in0++;
                if( !(n & (1 << 1)) ) in0++; else break;
                if( !(n & (1 << 2)) ) in0++; else break;
                if( !(n & (1 << 3)) ) in0++; else break;
                if( !(n & (1 << 4)) ) in0++; else break;
                if( !(n & (1 << 5)) ) in0++; else break;
                if( !(n & (1 << 6)) ) in0++; else break;
                if( !(n & (1 << 7)) ) in0++; else break;
            }
        } while( 0 );
        bit_counts[n].in0 = in0;
        bit_counts[n].in1 = in1;
        bit_counts[n].out0 = out0;
        bit_counts[n].out1 = out1;
        bit_counts[n].changes = changed;
        bit_counts[n].ones = ones;
    }
}



void InitSRG( void ) {
	InitBitCountLookupTables();
    srgl.makeEntropy = SRG_CreateEntropy2_256;
    EM_ASM( (
    	//srgTemplate->SetClassName( String::NewFromUtf8( isolate, "sack.core.srg", v8::NewStringType::kNormal ).ToLocalChecked() );
        function SaltyRNG( a, b ) {
            if( !(this instanceof SaltyRNG )) return new SaltyRNG( a, b );
            if( typeof a === "undefined")
                this.this_ = Module._makeBlankSRGObject();
            else if( typeof a === "function")
                this.this_ = Module._makeSRGObject(Module.defineCallback(a) );
            else {
                this.this_ = Module._makeSRGObjectWithSeed( a, b );
            }
        }
        // read only thing.
        //SaltyRNG.prototype.constructor.name = "sack.org.SRG";

        const saltyRngMethods = {

            seed(s) {
                
                 seed(this.this_, si);
            },
            reset() {
                Module._reset( this.this_);
            },
	        getBits( bits, sign ) {
                if( arguments.length === 0 ) {
                    bits = 32; sign = 1;
                } else if( arguments.length == 1 ) {
                    sign = 0;
                }
                Module._getBits( this.this_, bits, sign );
            },
            getBuffer( bits ) {
                var mem = Module._malloc( bits / 8 );
                Module._getBuffer( this.this_, bits, mem );
                var result = new Uint8Array( bits / 8 );
                for( var n = 0; n < bits/8; n++ ){
                    result[n] = Module.U8HEAP[mem+n];
                }
                Module._free( mem );
            },
            setVersion( version ) {
                Module._setVersion( version || 0 );
            },
            sign( buf ){
                var ba;
   				var si = allocate( ba= intArrayFromString(buf), 'i8', ALLOC_NORMAL);
                if( this instanceof SaltyRNG )
                    Module._srg_sign( this.this_, si, ba.length-1 );
                else
                    Module._srg_sign( 0, si, ba.length-1 );
            },
            setSigningThreads(n) {
                if( this instanceof SaltyRNG )
                    Module._srg_setSigningThreads( this.this_, n );
                else
                    Module._srg_setSigningThreads( 0, n );
            },
            verify( string, hash, pad1, pad2 ) {
        		if( arguments.Length() > 1 ) {
                    var sa;
    				var si = allocate( sa = intArrayFromString(string), 'i8', ALLOC_NORMAL);
                    var hi = allocate( intArrayFromString(hash), 'i8', ALLOC_NORMAL);
                    var r;
                    if( this instanceof SaltyRNG )
                        r = Module._srg_verify( this.this_, si, sa.length-1, hi, pad1||0, pad2||0 );
                    else
                        r = Module._srg_verify( 0, si, sa.length-1, hi, pad1||0, pad2||0 );
                    Module._free( si );
                    Module._free( hi );
                    return r?true:false;
                } return false;
            },
            delete() {
                Module._deleteSRGObject( this.this_ );
            }
        };
		Object.defineProperties( SaltyRNG.prototype, Object.getOwnPropertyDescriptors( saltyRngMethods ));

        SaltyRNG.id = function( string,vers ) {
            if( !arguments.length ) {
    			var r = Module._idGenerator( 0, 0, 0 );
				const string = Module.this_.objects[r];
                return string;
            }
        	var version = -1;
            if( arguments.length > 1 )
                version = vers;
            var sbuf;
			var si = allocate( sbuf = intArrayFromString(string), 'i8', ALLOC_NORMAL);

            var r = Module._idGenerator( si, sbuf.length-1, version );
            Module._free( si );
            return Module.this_.objects[r-1];
        };

		function vfs_u8xor(s,key) {
			var sa;
			var si = allocate( sa = intArrayFromString(s), 'i8', ALLOC_NORMAL);
			var ka;
			var ki = allocate( ka = intArrayFromString(key.key), 'i8', ALLOC_NORMAL);
			var step = key.step;;
			var refKey = Module.this_.objects.push(key)-1;
			//console.log( "s: ", s, sa.length-1, " key : ", key, ka.length-1, step );
			var res = Module._vfs_u8xor( si, sa.length-1, ki, ka.length-1, step, refKey );
			res = Module.this_.objects[res];
			Module._free( si );
			Module._free( ki );
			//console.log( "RESULT IWTH : ", res );
			// throw out all these references
			Module.this_.objects.length = refKey;
			return res;
		}

        SaltyRNG.sign = saltyRngMethods.sign;
        SaltyRNG.setSigningThreads = saltyRngMethods.setSigningThreads;
        SaltyRNG.verify = saltyRngMethods.verify;
		Module.SACK.id = SaltyRNG.id;
		Module.SACK.u8xor = vfs_u8xor;
        Module.SACK.SaltyRNG = SaltyRNG;
    ) );
}

