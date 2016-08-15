#ifndef InterShellANIMATIONHEADER
#define InterShellANIMATIONHEADER

#ifndef __cplusplus

#ifdef WIN32
#include <mng/libmng.h>
#else
#include <libmng.h>
#endif

/*---- private ------*/
typedef enum {ANIM_RUN = 0, ANIM_STOPBEGIN, ANIM_STOPEND} ANIM_MODES;

typedef struct mng_file {

	mng_handle handle;   //mng library handle

	CTEXTSTR name;       //mng file name

	uint8_t* data;            //internal buffer for mng file 
	uint32_t length;          // 
	uint32_t current_index;

	Image image;         //internal image buffer


	uint32_t timer;			// timer ID

	uint32_t ix;				//position and size of played animation
	uint32_t iy;
	uint32_t iw;
	uint32_t ih;

	struct {
		volatile BIT_FIELD initialized : 1;  //
		volatile ANIM_MODES stop;
	} flags;

//	PRENDERER renderer;
	PSI_CONTROL control;

} *PMNG_ANIMATION;
typedef struct mng_file MNG_ANIMATION;



/*---- public ------*/

PMNG_ANIMATION 	InitAnimationEngine( void );
/*
	Create animation object 
*/

//---------------------------------------------------------------------------
void	GenerateAnimation( PMNG_ANIMATION animation, PSI_CONTROL control /*PRENDERER renderer*/, CTEXTSTR mngfilename, int x, int y, int w, int h);
/*
	Set and play animation 
	
	Input:
		animation	- pointer to animation structure, previously allocated by InitAnimationEngine()
		renderer	- Screen surface handler
		mngfilename	- mng file name
		x,y,w,h		- pley window (intershell coords)

*/

//---------------------------------------------------------------------------
void 	DeInitAnimationEngine( PMNG_ANIMATION animation);
/*
	Destroy animation object 
*/
#endif
#endif
