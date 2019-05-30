
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
//#include <unistd.h>
#include <emscripten/html5.h>
#include <emscripten.h>

#define NULL ((void*)0)

EM_JS(void*, call_alert, (), {
  alert('hello world!');
  throw 'all done';
});

void initFileSystem( void ) {



	int x = EM_ASM_INT({
	console.log('I received: ' + $0);
	return $0 + 1;
}, 100);
printf("%d\n", x);

	EM_ASM((
			  const myCanvas = document.querySelector( "[ID='SACK Display 1']" );


    //FS.mkdir('/IDBFS');
			  r = FS.mount(IDBFS, {}, '/home/web_user');
			  console.log( "Log:", r );
if(0)
    FS.syncfs(true, function (err) {
console.log( "Sync happened?", err );
        assert(!err);
    }); // sync FROM backing store

	//FS.mkdir('/working');
	//FS.mount(WORKERFS, { packages: [{ metadata: meta, blob: blob }] }, '/working');

));

}


//void allowClick


//#define USE_MAIN_LOOP

#ifdef USE_MAIN_LOOP
static void renderLoop( void )
#else
static EM_BOOL renderLoop( double time, void* userData )
#endif
{

	extern int SACK_Vidlib_DoRenderPass( void );

	//printf( "do render pass.\n " );
	if( SACK_Vidlib_DoRenderPass() ) {
#ifndef USE_MAIN_LOOP
	      //printf( "Want immediate\n" );
		emscripten_request_animation_frame( renderLoop, NULL );
#endif
		//printf( "rechedule immediate.\n " );
		// wants reschedule
	}
	else
	{
#ifdef USE_MAIN_LOOP
		emscripten_pause_main_loop();
#endif
	}
	
}

static void wakeMain( void ) {
#ifndef USE_MAIN_LOOP
	//printf( "Wake called...\n" );
  	emscripten_request_animation_frame( renderLoop, NULL );
#else
	emscripten_resume_main_loop();
#endif
}

void initDisplay() {
	EMSCRIPTEN_RESULT r;
	void *myState = NULL;
	{
		extern int EditOptionsEx( void *odbc, void *parent, int wait );
      		EditOptionsEx( NULL, NULL, 0 );
	}
	{
		extern void SACK_Vidlib_SetAnimationWake( void (*)(void) );
		SACK_Vidlib_SetAnimationWake( wakeMain );
	}
#ifndef USE_MAIN_LOOP
  	emscripten_request_animation_frame( renderLoop, NULL );
#else
	emscripten_set_main_loop( renderLoop, 0, 0 );
#endif

}


int main( void ) {
	extern void InvokeDeadstart();
	//char buf[256];
	//getcwd( buf, 256 );
	//printf( "Current Directory: %s\n", buf );
	// default path is = '/'
   	initFileSystem();
	InvokeDeadstart();
	initDisplay();

	EM_ASM((
		//FS.mkdir('/IDBFS');

		FS.syncfs(true, function (err) {
		   //console.log( "Sync happened?", err );
			assert(!err);
		}); // sync FROM backing store
	));
#ifndef USE_MAIN_LOOP
	emscripten_exit_with_live_runtime();
#endif
}
