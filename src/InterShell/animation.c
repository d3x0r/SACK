#include "intershell_local.h"
#include <image.h>

#include "animation.h"


//  MNG animation player.  
//
//  some problems:
//  - whole animation is loaded into internal mng buffers 
//  - during the first reading of animation some slow down can be observed  



//#define DISABLE_ANIMATION
//#define DEBUG_TIMING


//---------------------------------------------------------------------------
mng_ptr MNG_DECL my_mng_memalloc (mng_size_t iLen)
/*
Memory management callback
*/
{
	mng_ptr mptr;
	mptr =  (mng_ptr)Allocate( iLen );

	MemSet(mptr, 0, iLen);

	return mptr;
}

//---------------------------------------------------------------------------
void MNG_DECL my_mng_memfree (mng_ptr    pPtr,
							  mng_size_t iLen)
							  /*
							  Memory management callback
							  */
{

	Release( pPtr );
}

//---------------------------------------------------------------------------
mng_bool MNG_DECL my_mng_openstream  (mng_handle hHandle)
/*
Open/Close/Read/Write callback
*/
{

	PMNG_ANIMATION animation = (PMNG_ANIMATION)mng_get_userdata( hHandle );

	if( !animation->name )
		return MNG_FALSE; 

	animation->length = 0;


	if( !animation->data )
		animation->data = (P_8)OpenSpace( NULL, animation->name, &animation->length);


	animation->current_index = 0;

	if( animation->data )
		return MNG_TRUE;


	return MNG_TRUE;
}

//---------------------------------------------------------------------------
mng_bool MNG_DECL my_mng_closestream (mng_handle hHandle)
/*
Open/Close/Read/Write callback
*/
{

	PMNG_ANIMATION animation = (PMNG_ANIMATION)mng_get_userdata( hHandle );

	if( animation->data )
		CloseSpace(animation->data);

	animation->data = NULL;
	animation->length = 0;

	return MNG_TRUE;
}

//---------------------------------------------------------------------------
mng_bool MNG_DECL my_mng_readdata (mng_handle  hHandle,
								   mng_ptr     pBuf,
								   mng_uint32  iBuflen,
								   mng_uint32p pRead)
								   /*
								   Open/Close/Read/Write callback
								   */

{
	PMNG_ANIMATION animation = (PMNG_ANIMATION)mng_get_userdata( hHandle );

	if( !animation ) {
		return MNG_FALSE;
	}


	if( animation->data && ( animation->current_index + iBuflen ) <= animation->length )
	{

		MemCpy( pBuf, animation->data + animation->current_index, iBuflen );

		(*pRead) = iBuflen;
		animation->current_index += iBuflen;
		return MNG_TRUE;
	}

	//	lprintf("my_mng_readdata: out of buffer");
	return MNG_FALSE;
}


//---------------------------------------------------------------------------
mng_bool MNG_DECL my_mng_writedata (mng_handle  hHandle,
									mng_ptr     pBuf,
									mng_uint32  iBuflen,
									mng_uint32p pWritten)
									/*
									Open/Close/Read/Write callback
									*/
{
	return MNG_TRUE;
}


//---------------------------------------------------------------------------
mng_bool MNG_DECL my_mng_errorproc (mng_handle  hHandle,
									mng_int32   iErrorcode,
									mng_int8    iSeverity,
									mng_chunkid iChunkname,
									mng_uint32  iChunkseq,
									mng_int32   iExtra1,
									mng_int32   iExtra2,
									mng_pchar   zErrortext)
{
	//PMNG_ANIMATION animation = (PMNG_ANIMATION)mng_get_userdata( hHandle );
	return MNG_TRUE;
}


//---------------------------------------------------------------------------
mng_uint32 MNG_DECL my_mng_gettickcount (mng_handle hHandle )
{
	return tickGetTick();
}

//---------------------------------------------------------------------------
void CPROC MNGTimer( PTRSZVAL psv )
/*
Called when prcessing needs to be resumed
*/
{
	static int ontimer = 0;

	PMNG_ANIMATION animation = (PMNG_ANIMATION)psv;
	mng_retcode myretcode;
	PCanvasData canvas = GetCanvas( GetCommonParent( animation->control ) );

	if(!animation || !animation->flags.initialized || ontimer )
		return;

	if( canvas && canvas->flags.bEditMode ) // don't do fixup/reveal if editing...
		return;


	ontimer = 1; 

	//
	if(animation->flags.stop != ANIM_RUN) 
	{
		RemoveTimer(animation->timer);   
		animation->flags.stop = ANIM_STOPEND;
		ontimer = 0; 
		return;
	}

	ontimer = 1; 

	if( !animation->flags.initialized )
	{
		ontimer = 0; 
		return;
	}

	myretcode = mng_display_resume (animation->handle);


	switch( myretcode )
	{
	case MNG_NEEDTIMERWAIT:
		ontimer = 0; 
		return;
	default:
		lprintf( "Unhandled result from mng_display_resume:%d", myretcode );
		ontimer = 0; 
		return;
	}

	ontimer = 0; 
}


//---------------------------------------------------------------------------
mng_bool MNG_DECL my_mng_processheader (mng_handle hHandle, mng_uint32 iWidth, mng_uint32 iHeight )
/*
Called once wheb MNG header is processed 
*/
{
	PMNG_ANIMATION animation = (PMNG_ANIMATION)mng_get_userdata( hHandle );

	if( !animation->image )
		animation->image = MakeImageFile(iWidth, iHeight);

	if(animation->iw == 0 && animation->ih == 0)
	{
		animation->iw = iWidth;
		animation->ih = iHeight;
	}


	//	lprintf( "my_mng_processheader size %d x %d ", iWidth, iHeight);

	return MNG_TRUE;
}


//---------------------------------------------------------------------------
mng_ptr my_mng_getcanvasline(mng_handle hHandle, mng_uint32 line)
/*
Callback to fill image data buffer
*/
{

	PMNG_ANIMATION animation = (PMNG_ANIMATION)mng_get_userdata( hHandle );

	//	lprintf( "my_mng_getcanvasline line %d", line);

	if(animation->image)
		return IMG_ADDRESS(animation->image, 0, line);


	return NULL;
}



//---------------------------------------------------------------------------
mng_bool MNG_DECL my_mng_refresh (mng_handle hHandle
								  , mng_uint32 iX
								  , mng_uint32 iY
								  , mng_uint32 iWidth
								  , mng_uint32 iHeight )
								  /*
								  Called when animation window has to be refreshed
								  */

{
	PRENDERER renderer;
	PMNG_ANIMATION animation = (PMNG_ANIMATION)mng_get_userdata( hHandle );


#ifdef DEBUG_TIMING
	Log( "BeginDraw" );
#endif

	//Skip frames if there is no renderer yet
	if( !animation->control || !(renderer = GetFrameRenderer(animation->control)))
		return MNG_TRUE;




	//Set and resize image to player window
	//BlotScaledImageSizedToAlpha( GetDisplayImage( animation->renderer ), animation->image, animation->ix, animation->iy, animation->iw, animation->ih, 0 );
	BlotScaledImageSizedToAlpha( GetDisplayImage( renderer ), animation->image, animation->ix, animation->iy, animation->iw, animation->ih, ALPHA_TRANSPARENT );


#ifdef DEBUG_TIMING
	Log( "EndDraw" );
#endif
	//UpdateDisplay( renderer );
	UpdateDisplayPortion(renderer, animation->ix, animation->iy, animation->iw, animation->ih);

	//	ProcessDisplayMessages();

#ifdef DEBUG_TIMING
	Log( "EndUpdate" );
#endif

	return MNG_TRUE;
}



//---------------------------------------------------------------------------
mng_bool MNG_DECL my_mng_settimer (mng_handle hHandle, mng_uint32 timeout )
/*
Set timer to resume playing animation
*/
{


	PMNG_ANIMATION animation = (PMNG_ANIMATION)mng_get_userdata( hHandle );

	if( animation->timer == 0 )
		animation->timer = AddTimer( timeout, MNGTimer, (PTRSZVAL)animation );
	else
		RescheduleTimerEx( animation->timer, timeout );

	return MNG_TRUE;
}

/*
static PTRSZVAL CPROC ThreadRunAnuimation( PTHREAD thread )
{
	PMNG_ANIMATION animation = (PMNG_ANIMATION)thread->param;	

	if(animation->flags.stop == ANIM_STOPBEGIN)
	{
		animation->flags.stop = ANIM_STOPEND;
		return 0;
	}

	mng_readdisplay( animation->handle );            

	animation->flags.initialized = 1;

	return 0;
}
*/

//---------------------------------------------------------------------------
void GenerateAnimation( PMNG_ANIMATION animation, PSI_CONTROL control, CTEXTSTR mngfilename, int x, int y, int w, int h)
/*
Set and play animation 

Input:
animation	- pointer to animation structure, previously allocated by InitAnimationEngine()
control		- screen surface handler
mngfilename	- mng file name
x,y,w,h		- pley window (intershell coords)

*/
{

#ifdef DISABLE_ANIMATION
	return;
#endif
	{
		mng_retcode myretcode;


		if( !animation )
			return;

		animation->name = mngfilename;
		animation->ix = x;
		animation->iy = y;
		animation->iw = w;
		animation->ih = h;

//		lprintf("GenerateAnimation : %s %d %d %d %d", mngfilename, x, y, w, h);

		animation->control = control;

		myretcode = mng_setcb_openstream (animation->handle, my_mng_openstream);

		myretcode = mng_setcb_readdata (animation->handle, my_mng_readdata);

		myretcode = mng_setcb_closestream (animation->handle, my_mng_closestream);
		myretcode = mng_set_storechunks( animation->handle, MNG_TRUE );

		//callback to update display
		myretcode = mng_setcb_refresh( animation->handle, my_mng_refresh );            

		myretcode = mng_setcb_gettickcount( animation->handle, my_mng_gettickcount );

		//setting timer to resume processing animation
		myretcode = mng_setcb_settimer( animation->handle, my_mng_settimer );          

		//called after heder is processed
		myretcode = mng_setcb_processheader( animation->handle, my_mng_processheader ); 

		//callback to fill image data
		myretcode = mng_setcb_getcanvasline( animation->handle, my_mng_getcanvasline ); 

		/* tell the library we want RGBA8*/
		mng_set_canvasstyle( animation->handle, MNG_CANVAS_RGBA8 );

		//set animation speed
		//		mng_set_speed( animation->handle, mng_st_slowest );

		//this avoid flickering
		mng_set_doprogressive( animation->handle, MNG_FALSE );             
		mng_set_suspensionmode(animation->handle, MNG_TRUE);

		//		mng_set_cacheplayback( animation->handle, MNG_FALSE );
		//      mng_set_storechunks(animation->handle, MNG_FALSE);


//			ThreadTo( ThreadRunAnuimation, (PTRSZVAL)animation );

		mng_readdisplay( animation->handle );            
		animation->flags.initialized = 1;

	}
}



//---------------------------------------------------------------------------
PMNG_ANIMATION InitAnimationEngine( void )
/*
Create animation object 
*/
{

#ifdef DISABLE_ANIMATION
	return NULL;
#endif
	{


		PMNG_ANIMATION animation;

		SystemLogTime( SYSLOG_TIME_CPU| SYSLOG_TIME_DELTA );


		animation = New( MNG_ANIMATION );
		animation->flags.initialized = 0;
		animation->timer = 0;
		animation->image = NULL;
		animation->data = NULL;
		animation->length = 0;
		animation->current_index = 0;
		animation->name = NULL;
		animation->flags.stop = ANIM_RUN;
		animation->handle = NULL;
		animation->control = NULL;


		animation->handle = mng_initialize ( (mng_ptr)animation
			, my_mng_memalloc
			, my_mng_memfree
			, MNG_NULL // trace callback
			);

		if( animation->handle == MNG_NULL )
		{
			lprintf("Error InitAnimationEngine, can not do mng_initialize");
			Release( animation );
			return NULL;
		}


		return animation;
	}

}


//---------------------------------------------------------------------------
void DeInitAnimationEngine( PMNG_ANIMATION animation )
/*
Destroy animation object 
*/
{
#ifdef DISABLE_ANIMATION
	return;
#endif


	if( !animation )
		return;

	animation->flags.stop = ANIM_STOPBEGIN;     //telling the timer to stop itself

	if(animation->timer != 0)  //wait until timer stops
	{
		while(animation->flags.stop != ANIM_STOPEND) 
			Relinquish();

		//		RemoveTimer(animation->timer);   
	}

	if(animation->handle)
		mng_cleanup( &animation->handle );

	animation->handle = NULL;


	if( animation->image )
		UnmakeImageFile(animation->image);

	Release( animation );

}

OnFinishInit( "Animation" )( void )
{
#ifdef DISABLE_ANIMATION
	return;
#else
	//	Animation = InitAnimationEngine();

#endif
}
