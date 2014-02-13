#include <sack_types.h>
#include <imglib/imagestruct.h>
#include <sharemem.h>

#include "streamstruct.h"
#include "filters.h"
#include <xvid.h>

#define MAX_ZONES   64

static xvid_enc_zone_t ZONES[MAX_ZONES];
static int NUM_ZONES = 0;
#define SMALL_EPS (1e-10)

static const int motion_presets[] = {
	/* quality 0 */
	0,

	/* quality 1 */
	XVID_ME_ADVANCEDDIAMOND16,

	/* quality 2 */
	XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16,

	/* quality 3 */
	XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
	XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8,

	/* quality 4 */
	XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
	XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 |
	XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,

	/* quality 5 */
	XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
	XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 |
	XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,

	/* quality 6 */
	XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 | XVID_ME_EXTSEARCH16 |
	XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 | XVID_ME_EXTSEARCH8 |
	XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,

};

/* Maximum number of frames to encode */
#define ABS_MAXFRAMENR 9999
#define FRAMERATE_INCR 1001

#define ME_ELEMENTS (sizeof(motion_presets)/sizeof(motion_presets[0]))

static int ARG_STATS = 0;
static int ARG_DUMP = 0;
static int ARG_LUMIMASKING = 0;
static int ARG_BITRATE = 0;
static int ARG_SINGLE = 0;
static char *ARG_PASS1 = 0;
static char *ARG_PASS2 = 0;
static int ARG_QUALITY = ME_ELEMENTS - 1;
static float ARG_FRAMERATE = 25.00f;
static int ARG_MAXFRAMENR = ABS_MAXFRAMENR;
static int ARG_MAXKEYINTERVAL = 0;
static char *ARG_INPUTFILE = NULL;
static int ARG_INPUTTYPE = 0;
static int ARG_SAVEMPEGSTREAM = 0;
static int ARG_SAVEINDIVIDUAL = 0;
static char *ARG_OUTPUTFILE = NULL;
static int XDIM = 0;
static int YDIM = 0;
static int ARG_BQRATIO = 150;
static int ARG_BQOFFSET = 100;
static int ARG_MAXBFRAMES = 0;
static int ARG_PACKED = 0;
static int ARG_DEBUG = 0;
static int ARG_VOPDEBUG = 0;
static int ARG_GMC = 0;
static int ARG_QPEL = 0;
static int ARG_CLOSED_GOP = 0;







typedef struct compress_tag {
	struct {
		_32 bInited : 1;
	} flags;
   /*XVID_INIT_PARAM*/ xvid_gbl_init_t xinit;
   xvid_gbl_info_t xinfo;
	void *handle;
   POINTER bitstream;
	/*XVID_ENC_PARAM */ xvid_enc_create_t xparam;
	/*XVID_ENC_FRAME */ xvid_enc_frame_t xframe;
   /*XVID_ENC_STATS */ xvid_enc_stats_t xstats;
} *PCOMPRESS;


#define COMPRESS_STRUCT_DEFINED
#include "compress.h"

void InitCompress( PCOMPRESS pCompress, int width, int height )
{
	if( !pCompress->flags.bInited )
	{
		{
			int version;
         pCompress->bitstream = Allocate( width * height * 4 );
			pCompress->xinit.version = XVID_VERSION;
			if( XVID_VERSION != (version = xvid_global( NULL, XVID_GBL_INIT, &pCompress->xinit, NULL ) ) )
			{
				lprintf( WIDE("warning: xvid api version(%d) and compiled version(%d) do not match.")
						 , version, XVID_VERSION );
			}
         pCompress->xinfo.version = XVID_VERSION;
			xvid_global( NULL, XVID_GBL_INFO, &pCompress->xinfo, NULL );
		}
		{
	int xerr;
#define xvid_enc_create pCompress->xparam
	/* Version again */
	memset(&xvid_enc_create, 0, sizeof(xvid_enc_create));
	xvid_enc_create.version = XVID_VERSION;

	/* Width and Height of input frames */
	xvid_enc_create.width = width;
	xvid_enc_create.height = height;
	xvid_enc_create.profile = XVID_PROFILE_AS_L4;

	/* init plugins  */
    xvid_enc_create.zones = ZONES;
    xvid_enc_create.num_zones = NUM_ZONES;

	//xvid_enc_create.plugins = plugins;
	xvid_enc_create.num_plugins = 0;
/*
	if (ARG_SINGLE) {
		memset(&single, 0, sizeof(xvid_plugin_single_t));
		single.version = XVID_VERSION;
		single.bitrate = ARG_BITRATE;

		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_single;
		plugins[xvid_enc_create.num_plugins].param = &single;
		xvid_enc_create.num_plugins++;
	}

	if (ARG_PASS2) {
		memset(&rc2pass2, 0, sizeof(xvid_plugin_2pass2_t));
		rc2pass2.version = XVID_VERSION;
		rc2pass2.filename = ARG_PASS2;
		rc2pass2.bitrate = ARG_BITRATE;

		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass2;
		plugins[xvid_enc_create.num_plugins].param = &rc2pass2;
		xvid_enc_create.num_plugins++;
	}

	if (ARG_PASS1) {
		memset(&rc2pass1, 0, sizeof(xvid_plugin_2pass1_t));
		rc2pass1.version = XVID_VERSION;
		rc2pass1.filename = ARG_PASS1;

		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass1;
		plugins[xvid_enc_create.num_plugins].param = &rc2pass1;
		xvid_enc_create.num_plugins++;
	}

	if (ARG_LUMIMASKING) {
		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_lumimasking;
		plugins[xvid_enc_create.num_plugins].param = NULL;
		xvid_enc_create.num_plugins++;
	}

	if (ARG_DUMP) {
		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_dump;
		plugins[xvid_enc_create.num_plugins].param = NULL;
		xvid_enc_create.num_plugins++;
	}

#if 0
	if (ARG_DEBUG) {
		plugins[xvid_enc_create.num_plugins].func = rawenc_debug;
		plugins[xvid_enc_create.num_plugins].param = NULL;
		xvid_enc_create.num_plugins++;
	}
#endif
*/
	/* No fancy thread tests */
	xvid_enc_create.num_threads = 0;

	/* Frame rate - Do some quick float fps = fincr/fbase hack */
	if ((ARG_FRAMERATE - (int) ARG_FRAMERATE) < SMALL_EPS) {
		xvid_enc_create.fincr = 1;
		xvid_enc_create.fbase = (int) ARG_FRAMERATE;
	} else {
		xvid_enc_create.fincr = FRAMERATE_INCR;
		xvid_enc_create.fbase = (int) (FRAMERATE_INCR * ARG_FRAMERATE);
	}

	/* Maximum key frame interval */
    if (ARG_MAXKEYINTERVAL > 0) {
        xvid_enc_create.max_key_interval = ARG_MAXKEYINTERVAL;
    }else {
	    xvid_enc_create.max_key_interval = (int) ARG_FRAMERATE *10;
    }

	/* Bframes settings */
	xvid_enc_create.max_bframes = ARG_MAXBFRAMES;
	xvid_enc_create.bquant_ratio = ARG_BQRATIO;
	xvid_enc_create.bquant_offset = ARG_BQOFFSET;

	/* Dropping ratio frame -- we don't need that */
	xvid_enc_create.frame_drop_ratio = 0;

	/* Global encoder options */
	xvid_enc_create.global = 0;

	if (ARG_PACKED)
		xvid_enc_create.global |= XVID_GLOBAL_PACKED;

	if (ARG_CLOSED_GOP)
		xvid_enc_create.global |= XVID_GLOBAL_CLOSED_GOP;

	if (ARG_STATS)
		xvid_enc_create.global |= XVID_GLOBAL_EXTRASTATS_ENABLE;

	/* I use a small value here, since will not encode whole movies, but short clips */
	xerr = xvid_encore(NULL, XVID_ENC_CREATE, &xvid_enc_create, NULL);
			lprintf( WIDE("xerr of xvid_encore: %d handle:%d"), xerr, pCompress->xparam.handle );
		}
      if(0)
		{
			int xerr;
         pCompress->xparam.version = XVID_VERSION;
			pCompress->xparam.width = width;
			pCompress->xparam.height = height;
							  // frame rate - scaled 1000, 29.718 (or whatever)
			pCompress->xparam.fincr = 1000;
			pCompress->xparam.fbase = 29718;
			//pCompress->xparam.rc_bitrate = 770;
			//pCompress->xparam.rc_reaction_delay_factor = -1;
			//pCompress->xparam.rc_averaging_period = -1;
			//pCompress->xparam.rc_buffer = -1;

			//pCompress->xparam.min_quantizer = 1;
			//pCompress->xparam.max_quantizer = 15; // default 31

			//pCompress->xparam.max_key_interfal = -1; // default 10sec (10*fps)

			xerr = xvid_encore( NULL, XVID_ENC_CREATE, &pCompress->xparam, NULL );
			lprintf( WIDE("xerr of xvid_encore: %d handle:%d"), xerr, pCompress->xparam.handle );
		}
		pCompress->xstats.version = XVID_VERSION;
      pCompress->flags.bInited = 1;
	}
}

//int CPROC CompressFrame( PCOMPRESS pCompress, POINTER *data, INDEX *len )
int CPROC CompressFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice )
{
   PCOMPRESS pCompress = (PCOMPRESS)psv;
	int xerr;
   Image frame;
   GetDeviceData( pDevice, &frame, NULL );

	{  // setup constant factors about encoding behavior
      pCompress->xframe.version = XVID_VERSION;
		//pCompress->xframe.general = 0;
		pCompress->xframe.motion = 0; /*PMV_ADVANCEDDIAMOND16;*/
		pCompress->xframe.quant = 0; //1-31 specify quantizer...
		//pCompress->xframe.intra = -0; // 1- force keyframe, 0-force not keyframe
		pCompress->xframe.quant_intra_matrix = NULL;
		pCompress->xframe.quant_inter_matrix = NULL;

														 /* setup the input frame here... */
      pCompress->xframe.type = XVID_TYPE_AUTO;
		pCompress->xframe.input.csp = XVID_CSP_BGRA;
		pCompress->xframe.input.plane[0] = frame->image;
		pCompress->xframe.input.stride[0] = frame->pwidth*sizeof(CDATA);

		if( !pCompress->xframe.bitstream )
		{
			pCompress->xframe.length = frame->pwidth * frame->height; //frame->width * frame->height * 4;
			pCompress->xframe.bitstream = Allocate( frame->pwidth * frame->height );
		}

	}
   pCompress->xstats.version = XVID_VERSION;
	xerr = xvid_encore( pCompress->xparam.handle, XVID_ENC_ENCODE, &pCompress->xframe, &pCompress->xstats );
   if(0)
	lprintf( WIDE("stats! %d %d %d %d %d %d")
          , xerr
			 , pCompress->xstats.length
			 , pCompress->xstats.hlength
			 , pCompress->xstats.kblks
			 , pCompress->xstats.mblks
			 , pCompress->xstats.ublks );
	if( xerr < 0 || xerr > 512000 )
	{
		lprintf( WIDE("Resulting %d data was %p(%d)")
				 , xerr
				 , pCompress->xframe.bitstream
				 , pCompress->xframe.length );
	}
   MemCpy( pCompress->bitstream, pCompress->xframe.bitstream, xerr );
   SetDeviceData( pDevice, pCompress->bitstream, xerr );
   // at this point pCompress->xframe.intra 0 did not result in keyframe
	// 1 is a keyframe (scene change) 2 is keyframe (max keyfram interval)
   return 1;
}


PTRSZVAL OpenCompressor( PCAPTURE_DEVICE pDevice, _32 width, _32 height )
{
	PCOMPRESS pCompress = Allocate( sizeof( *pCompress ) );
	MemSet( pCompress, 0, sizeof( *pCompress ) );
	// xvid_encore( pCompress->handle, XVID_DEC_DESTROY, NULL, NULL );

	pCompress->xparam.width = width;
	pCompress->xparam.height = height;
	InitCompress( pCompress, width, height );

	return (PTRSZVAL)pCompress;
}

void DestroyCompressor( PCOMPRESS *ppCompressor )
{
	if( ppCompressor )
	{
      PCOMPRESS pCompressor = *ppCompressor;
      xvid_encore( pCompressor->handle, XVID_DEC_DESTROY, NULL, NULL );
      Release( pCompressor );
      (*ppCompressor) = NULL;
	}
}
