#include <sack_types.h>
#define USE_IMAGE_INTERFACE GetImageInterface()
#include <imglib/imagestruct.h>
#include <image.h>
#include <sharemem.h>

#include "streamstruct.h"
#include <xvid.h>



typedef struct decompress_tag {
	struct {
		_32 bInited : 1;
	} flags;
   /*XVID_INIT_PARAM*/ xvid_gbl_init_t xinit;
   xvid_gbl_info_t xinfo;
   void *handle;
	/*XVID_ENC_PARAM */ xvid_dec_create_t xparam;
	/*XVID_ENC_FRAME */ xvid_dec_frame_t xframe;
   /*XVID_ENC_STATS */ xvid_dec_stats_t xstats;
} *PDECOMPRESS;

#define DECOMPRESS_STRUCT_DEFINED
#include "decompress.h"

void InitDecompress( PDECOMPRESS pDecompress, int width, int height )
{
	if( !pDecompress->flags.bInited )
	{
		{
			int version;
			pDecompress->xinit.version = XVID_VERSION;
			pDecompress->xinit.cpu_flags = 0;

			if( XVID_VERSION != (version = xvid_global( NULL, XVID_GBL_INIT, &pDecompress->xinit, NULL ) ) )
			{
				lprintf( WIDE("warning: xvid api version(%d) and compiled version(%d) do not match.")
						 , version, XVID_VERSION );
			}
         pDecompress->xinfo.version = XVID_VERSION;
			xvid_global( NULL, XVID_GBL_INFO, &pDecompress->xinfo, NULL );
		}
		{
			int xerr;
         pDecompress->xparam.version = XVID_VERSION;
	/*
	 * Image dimensions -- set to 0, xvidcore will resize when ever it is
	 * needed
	 */
			pDecompress->xparam.width = 0;
			pDecompress->xparam.height = 0;

			xerr = xvid_decore( NULL, XVID_DEC_CREATE, &pDecompress->xparam, NULL );
			lprintf( WIDE("xerr of xvid_decore: %d handle:%d"), xerr, pDecompress->xparam.handle );
		}
		pDecompress->xstats.version = XVID_VERSION;
      pDecompress->flags.bInited = 1;
	}
}

int CPROC DecompressFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice )
{
   PDECOMPRESS pDecompress = (PDECOMPRESS)psv;
	static Image frame;
	char *data;
	INDEX len;
	int xerr;
   if( !frame )
		frame = MakeImageFile( pDecompress->xparam.width, pDecompress->xparam.height );
   GetDeviceData( pDevice, &data, &len );
   InitDecompress( pDecompress, pDecompress->xparam.width, pDecompress->xparam.height );
	pDecompress->xframe.version = XVID_VERSION;
	pDecompress->xframe.general = 0;
	pDecompress->xframe.bitstream = data;
	pDecompress->xframe.length = len;
	pDecompress->xframe.output.csp = XVID_CSP_BGRA
#ifdef _WIN32
		| XVID_CSP_VFLIP
#endif
		;
	pDecompress->xframe.output.plane[0] = frame->image;
	pDecompress->xframe.output.stride[0] = frame->pwidth*sizeof(CDATA);
	while( len )
	{
		pDecompress->xframe.bitstream = data;
		pDecompress->xframe.length = len;
		xerr = xvid_decore( pDecompress->xparam.handle
								, XVID_DEC_DECODE
								, &pDecompress->xframe
								, &pDecompress->xstats );
		if( xerr < 0 )
			break;
		len -= xerr;
      data += xerr;
		//lprintf( WIDE("xvid_decore type = %d"), pDecompress->xstats.type );
		if(pDecompress->xstats.type == XVID_TYPE_VOL)
		{
			lprintf( WIDE("XVID_TYPE_VOL! %d,%d")
					 , pDecompress->xstats.data.vol.width
					  , pDecompress->xstats.data.vol.height);
		/* Check if old buffer is smaller */
			if(pDecompress->xparam.width* pDecompress->xparam.height <
				pDecompress->xstats.data.vol.width*pDecompress->xstats.data.vol.height)
			{
            int XDIM,YDIM;
				XDIM = pDecompress->xparam.width = pDecompress->xstats.data.vol.width;
				YDIM = pDecompress->xparam.height = pDecompress->xstats.data.vol.height;
            lprintf( WIDE("resizing image... very bad idea...") );
				ResizeImage( frame, pDecompress->xparam.width, pDecompress->xparam.height );
													/* Free old output buffer*/
													//if(out_buffer) free(out_buffer);

													/* Allocate the new buffer */
													//out_buffer = (unsigned char*)malloc(XDIM*YDIM*4);
					//if(out_buffer == NULL)
					//	goto free_all_memory;

					lprintf("Resized frame buffer to %dx%d\n", XDIM, YDIM);
				}
		}
	}
	if( xerr < 0 )
		lprintf( WIDE("error decompressing image...") );
	SetDeviceData( pDevice
					 , frame
					 , ( pDecompress->xparam.width * pDecompress->xparam.height ) );
  // lprintf( WIDE("stats! %d %d %d %d %d")
  // 		 , pDecompress->xstats.length
  // 		 , pDecompress->xstats.hlength
  // 		 , pDecompress->xstats.kblks
  // 		 , pDecompress->xstats.mblks
	// 		 , pDecompress->xstats.ublks
	return 1;
}


PTRSZVAL CPROC OpenDecompressor( PCAPTURE_DEVICE pDevice, _32 width, _32 height )
{
	PDECOMPRESS pDecompress = Allocate( sizeof( *pDecompress ) );
	MemSet( pDecompress, 0, sizeof( *pDecompress ) );
	// xvid_decore( pDecompress->handle, XVID_DEC_DESTROY, NULL, NULL );

	pDecompress->xparam.width = width;
	pDecompress->xparam.height = height;

	return (PTRSZVAL)pDecompress;

}

