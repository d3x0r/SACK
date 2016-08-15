#define USES_INTERSHELL_INTERFACE

#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi

#include <stdhdrs.h>
#include <intershell_export.h>
#include <intershell_registry.h>
#include <render.h>
#include <psi.h>




#define NUM_PICS REEL_LENGTH
#define NUM_PICS_IN_WINDOW 3
#define NUM_BLURS 1
#define NUM_REELS 3
#define NUM_ITERATIONS 15
#define REEL_LENGTH 25
#define NUM_ICONS 10
#define ITERATIONS_OFFSET 3

#define REEL_STEPX 106
#define REEL_OFSX 167
#define REEL_OFSY 114
#define REEL_WIDTH 96
#define REEL_HEIGHT 288

#define DO_NOT_SPIN 32768



//-------------------------------------------------------------------------------------------
// reel as a control

typedef struct reel_tag
{
	struct {
		BIT_FIELD bStarting : 1;
		BIT_FIELD bReelSpinning : 1;
		BIT_FIELD bStopping : 1;
		BIT_FIELD bInit : 1;
	} flags;
	INDEX position; // pos index into reel_idx[][pos];
	INDEX offset; // 96 points...
	Image images[REEL_LENGTH]; // REEL_LENGTH some unique constant to use in this code.
	INDEX reel_idx[REEL_LENGTH]; // REEL_LENGTH some unique constant to use in this code.
	int32_t speed; // positive or negative offset parameter... reels may spin backwards...
	// this is an idea, but I think that hrm..
	uint32_t next_event; // some time that if time now is greater than, do something else

	uint32_t stop_event; // tick at which we will be stopped.
	uint32_t stop_event_now; // the current now that the stop event thinks it is... so it can cat  up to 'now'
	uint32_t stop_event_tick;

	uint32_t start_event; // when started, and spinning...
	uint32_t start_event_now; // set at now, and itereated at
	uint32_t start_event_tick; // tick rate for start_event_now
	uint32_t target_idx;
} REEL, *PREEL;


//-------------------------------------------------------------------------------------------
// status as a control

typedef struct status_tag
{
	struct {
		BIT_FIELD bPlaying : 1;
	} flags;
	Image image;
} STATUS, *PSTATUS;
void SetStatus( PSI_CONTROL pc, int bPlaying );
int GetStatus( PSI_CONTROL pc );


//-------------------------------------------------------------------------------------------
// global ( actually a local since it's not in a .H file. )
typedef struct global_tag {
	struct {
		uint32_t bBackgroundInitialized : 1;
	} flags;
	int32_t ofs;
	uint32_t nReels;
	Image background, playing, playagain;
	Image strip; // raw strip of images loaded from a file.
	// oh I see this is a long lost global variable... wonder why this got lost in the mix....
	Image icons[NUM_ICONS];
	Image blank; // a blank square - may be laoded from a file.. but looks tacky otherwise.
	Image blurs[NUM_BLURS]; // one strip of REEL_LENGTH images blurred
	Image dodges[NUM_BLURS]; // one strip of REEL_LENGTH images dodges
	// this was previously images... but I forgot that icons were icons and images wehre something else.
	// this should be stored within the reel object...
	//Image images[NUM_REELS][REEL_LENGTH]; // REEL_LENGTH some unique constant to use in this code.
	//INDEX reel_idx[NUM_REELS][REEL_LENGTH]; // REEL_LENGTH some unique constant to use in this code.
	//	Image images[NUM_REELS + 2][NUM_PICS + 2];
	PSI_CONTROL frame;
	//PRENDERER render;

	Image surface;
	PSI_CONTROL reel_pc[NUM_REELS];
	PREEL pReel[NUM_REELS];
	//Image subsurface[NUM_REELS];
	PSI_CONTROL status_pc;
	//Image statussurface;
	Image backgroundsurface;
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	uint32_t idx[NUM_REELS];


} GLOBAL;

#ifndef GLOBAL_SOURCE
extern
#endif
	GLOBAL g;

#ifndef REEL_SOURCE
extern 
#endif
	CONTROL_REGISTRATION reel_control;
	
#ifndef STATUS_SOURCE
extern 
#endif
	CONTROL_REGISTRATION status_control;

