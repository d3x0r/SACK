#include <stdhdrs.h> // GetTickCount()
#include <sack_types.h>
#include <timers.h>
#include <imglib/imagestruct.h>

#ifdef __LINUX__
#include <libmng.h>
#else
#include <mng/libmng.h>
#endif

typedef struct mng_file {
   mng_handle handle;
	CTEXTSTR name;
	P_8 data;
   _32 length;
	Image image;
   _32 timer; // timer ID
   _32 current_index;
} *PMNG_SPRITE;


mng_ptr MNG_DECL my_mng_memalloc (mng_size_t iLen)
{
   return (mng_ptr)Allocate( iLen );
}

void MNG_DECL my_mng_memfree (mng_ptr    pPtr,
						mng_size_t iLen)
{
   Release( pPtr );
}

mng_bool MNG_DECL my_mng_openstream  (mng_handle hHandle)
{
   /*
	PMNG_SPRITE sprite = mng_get_userdata( hHandle );
	if( !sprite->data )
		sprite->data = OpenSpace( sprite->name, NULL );
	if( sprite->data )
	return MNG_TRUE;
   */
   return MNG_TRUE;
}

mng_bool MNG_DECL my_mng_closestream (mng_handle hHandle)
{
   /*
	PMNG_SPRITE sprite = mng_get_userdata( hHandle );
	Release( sprite->data );
	sprite->data = NULL;
   */
   return MNG_TRUE;
}

mng_bool MNG_DECL my_mng_readdata (mng_handle  hHandle,
							  mng_ptr     pBuf,
							  mng_uint32  iBuflen,
							  mng_uint32p pRead)
{
	PMNG_SPRITE sprite = mng_get_userdata( hHandle );
	if( ( sprite->current_index + iBuflen ) < sprite->length )
	{
		MemCpy( pBuf, sprite->data + sprite->current_index, iBuflen );
		(*pRead) = iBuflen;
		sprite->current_index += iBuflen;
		return MNG_TRUE;
	}
   return MNG_FALSE;
}


mng_bool MNG_DECL my_mng_writedata (mng_handle  hHandle,
                          mng_ptr     pBuf,
                          mng_uint32  iBuflen,
									mng_uint32p pWritten)
{
   PMNG_SPRITE sprite = mng_get_userdata( hHandle );
}


mng_bool MNG_DECL my_mng_errorproc (mng_handle  hHandle,
                          mng_int32   iErrorcode,
                          mng_int8    iSeverity,
                          mng_chunkid iChunkname,
                          mng_uint32  iChunkseq,
                          mng_int32   iExtra1,
                          mng_int32   iExtra2,
                          mng_pchar   zErrortext)
{
   PMNG_SPRITE sprite = mng_get_userdata( hHandle );
}

mng_ptr MNG_DECL my_mng_getbkgdline   (mng_handle hHandle
								  , mng_uint32 iLinenr)
{
//The application is responsible for returning a pointer to a line of
//pixels, which should be in the exact format as defined by the call
//to mng_set_canvasstyle() and mng_set_bkgdstyle(), without gaps between
//the representation of each pixel.
}

mng_ptr MNG_DECL my_mng_getalphaline  ( mng_handle hHandle
								  , mng_uint32 iLinenr)
{
//The application is responsible for returning a pointer to a line of
//pixels, which should be in the exact format as defined by the call
//to mng_set_canvasstyle() and mng_set_bkgdstyle(), without gaps between
//the representation of each pixel.

}


mng_uint32 MNG_DECL my_mng_gettickcount (mng_handle hHandle )
{
   return GetTickCount();
}

mng_bool MNG_DECL my_mng_refresh (mng_handle hHandle
							, mng_uint32 iX
							, mng_uint32 iY
							, mng_uint32 iWidth
							, mng_uint32 iHeight )
{
	PMNG_SPRITE sprite = mng_get_userdata( hHandle );
   return TRUE;
}

void CPROC MNGTimer( PTRSZVAL psv )
{
	PMNG_SPRITE sprite = (PMNG_SPRITE)psv;
   mng_retcode myretcode;
	myretcode = mng_display_resume (sprite->handle);
	switch( myretcode )
	{
	case MNG_NEEDTIMERWAIT:
		return;
	default:
		lprintf( "Unhandled result from mng_display_resume:%d", myretcode );
      break;
	}

}

mng_bool MNG_DECL my_mng_settimer (mng_handle hHandle, mng_uint32 timeout )
{
	PMNG_SPRITE sprite = mng_get_userdata( hHandle );
	if( sprite->timer == INVALID_INDEX )
		sprite->timer = AddTimer( timeout, MNGTimer, (PTRSZVAL)sprite );
   else
		RescheduleTimerEx( sprite->timer, timeout );
   //sprite->timeout = timeout;
   return MNG_TRUE;
}

PMNG_SPRITE DecodeMNG( _8 *buf, _32 size )
{
   mng_retcode myretcode;
   PMNG_SPRITE sprite;
	struct mng_file;
	sprite = Allocate( sizeof( *sprite ) );
   sprite->timer = INVALID_INDEX;
	sprite->image = NULL;
	sprite->data = buf;
   sprite->length = size;
	//mng_get_userdata();

	sprite->handle = mng_initialize ( (mng_ptr)sprite
									  , my_mng_memalloc
									  , my_mng_memfree
									  , MNG_NULL // trace callback
									  );
	if( sprite->handle == MNG_NULL )
	{
      Release( sprite );
		return NULL;
	}
	myretcode = mng_setcb_openstream (sprite->handle, my_mng_openstream);
	//if (myretcode != MNG_NOERROR)
		/* process error */;
	myretcode = mng_setcb_readdata (sprite->handle, my_mng_readdata);
	//if (myretcode != MNG_NOERROR)
		/* process error */;
	myretcode = mng_setcb_closestream (sprite->handle, my_mng_closestream);
		myretcode = mng_set_storechunks( sprite->handle, MNG_TRUE );
	//if (myretcode != MNG_NOERROR)
		/* process error */;

		// get background might have worked to grab the prior display surface
		// and render the image directly against the other...
      // otherwise I end up with a alpha shaded stamp to output as a Image
   //myretcode = mng_setcb_getbkgdline( sprite->handle, my_mng_getbkgdline );
   myretcode = mng_setcb_getalphaline( sprite->handle, my_mng_getalphaline );
   myretcode = mng_setcb_refresh( sprite->handle, my_mng_refresh );

	myretcode = mng_setcb_gettickcount( sprite->handle, my_mng_gettickcount );
	myretcode = mng_setcb_settimer( sprite->handle, my_mng_settimer );

	myretcode = mng_read (sprite->handle);
	if (myretcode != MNG_NOERROR)
	{
      lprintf( "Header did not decode." );
		return NULL;
		/* process error */;
	}
	while( myretcode == MNG_NEEDMOREDATA )
	{
		myretcode = mng_read (sprite->handle);
		if (myretcode != MNG_NOERROR)
		{
			lprintf( "Other block in error!" );
			//return NULL;
			/* process error */;
		}
		else switch( myretcode )
		{
		case MNG_UNEXPECTEDEOF:
         // cleanup all over
			return NULL;
		case MNG_NEEDMOREDATA:
			break;
		default:
			lprintf( "Unhandled return code mng_read:%d", myretcode );
         break;
		}
	}

	myretcode = mng_set_canvasstyle( sprite->handle, MNG_CANVAS_RGB8_A8 );
   myretcode = mng_set_bkgdstyle( sprite->handle, MNG_CANVAS_RGB8_A8 );
	myretcode = mng_display (sprite->handle);

 //   while (myretcode == MNG_NEEDTIMERWAIT) {
 //     /* wait for timer interval */;
 //     myretcode = mng_display_resume (sprite->handle);
 //   }
//
 //   if (myretcode != MNG_NOERROR)
 //     /* process error */;
 //
 //  myretcode = mng_setcb_xxxxxx (sprite->handle, my_xxxxxx);
 //  if (myretcode != MNG_NOERROR)
 //  	/* process error */;


	// this is to be done when the sprite is destroyed
   // the data for the sprite needs to be stored...
   //mng_cleanup( sprite->handle );
	return sprite;
}

