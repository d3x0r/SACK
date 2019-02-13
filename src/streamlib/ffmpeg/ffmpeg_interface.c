#define NO_UNICODE_C
#  ifdef __cplusplus
#    define __STDC_LIMIT_MACROS
#  endif
#ifdef __ANDROID__
#define ANDROID_MODE 1
#else
#define ANDROID_MODE 0
#endif

#ifdef __WATCOMC__
#pragma enum int 
#endif

//#define DEBUG_LOW_LEVEL_PACKET_IO
//#define DEBUG_LOW_LEVEL_PACKET_READ
//#define DEBUG_VIDEO_PACKET_READ
//#define DEBUG_AUDIO_PACKET_READ
//#define DEBUG_LOG_INFO

#include <stdhdrs.h>
#include <filesys.h>
#include <sqlgetoption.h>
#define USE_RENDER_INTERFACE l.pdi
#define USE_IMAGE_INTERFACE l.pii
#include <render.h>
#include <controls.h>
#include <ffmpeg_interface.h>

#if defined( __ANDROID__ ) && 0
#  define USE_OPENSL
#undef GetInterface
#  include <SLES/OpenSLES.h>
#  include <SLES/OpenSLES_Android.h>
#else
//------------ OpenAL Interface ---------------------

#  include <AL/al.h>
#  include <AL/alc.h>

#  ifdef WIN32
#    define lib_openal WIDE("openal32.dll")
#  endif
#  if defined( __ANDROID__ ) || defined( __LINUX__ )
#    define lib_openal WIDE("libopenal.so")
#  endif

#endif

//------------ FFMpeg Interface ---------------------
#ifdef _MSC_VER
#  define donothing
#  define inline donothing
#endif
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>

#ifdef WIN32
#define PIX_FMT  PIX_FMT_BGRA
#else
#define PIX_FMT  PIX_FMT_RGBA
#endif

#ifdef WIN32
#ifdef __GNUC__
#define lib_psi WIDE("libbag.psi.dll")
#else
#define lib_psi WIDE("bag.psi.dll")
#endif
#define lib_format WIDE("avformat-56.dll")
#define lib_codec WIDE("avcodec-56.dll")
#define lib_device WIDE("avdevice-56.dll")
#define lib_util WIDE("avutil-54.dll")
#define lib_swscale WIDE("swscale-3.dll")
#define lib_resample WIDE("swresample-1.dll")

#endif

#if defined( __ANDROID__) || defined( __LINUX__ )
#define lib_psi WIDE("libbag.psi.so")
#define lib_format WIDE("libavformat.so")
#define lib_codec WIDE("libavcodec.so")
#define lib_device WIDE("libavdevice.so")
#define lib_util WIDE("libavutil.so")
#define lib_swscale WIDE("libswscale.so")
#define lib_resample WIDE("libswresample.so")
#endif
//------------ Interface ---------------------

#include <gsm.h>

//------------ Interface ---------------------

SACK_NAMESPACE
_FFMPEG_INTERFACE_NAMESPACE

static struct fmpeg_interface
{
#define declare_func(a,b,c) a (CPROC *b) c
#define setup_func_test(lib, a,b,c) if( ffmpeg.b=(a(CPROC*)c)LoadFunction( lib, _WIDE(#b) ) )
#define setup_func(lib, a,b,c) do{ ffmpeg.b=(a(CPROC*)c)LoadFunction( lib, _WIDE(#b) ); }while(0)

	declare_func( void, av_register_all,(void) );
	declare_func( int, avformat_open_input, (AVFormatContext **ps, const char *filename, AVInputFormat *fmt, AVDictionary **options) );
	declare_func( int, avformat_close_input, (AVFormatContext **s) );
	declare_func( int, avformat_find_stream_info, (AVFormatContext *ic, AVDictionary **options) );
	declare_func( int, av_seek_frame, (AVFormatContext *s, int stream_index, int64_t timestamp, int flags) );
	declare_func( AVFrame*, av_frame_alloc, (	void 		)	);
	declare_func( void, av_frame_free, (AVFrame **frame) );

	declare_func( int, av_read_frame,(	AVFormatContext * 	s, AVPacket * 	pkt  ) );
	declare_func( int, avcodec_decode_video2,	(	AVCodecContext * 	avctx,
	                                               AVFrame * 	picture,
	                                               int * 	got_picture_ptr,
	                                               const AVPacket * 	avpkt 
	                                             ) );
	declare_func( int, avcodec_decode_audio4,	(	AVCodecContext * 	avctx,
	                                             AVFrame * 	frame,
	                                             int * 	got_frame_ptr,
	                                             const AVPacket * 	avpkt 
	                                             ) );
	declare_func( int, avcodec_open2, (AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options) );
	declare_func( int, avcodec_close, (AVCodecContext *avctx) );
	declare_func( void, avcodec_flush_buffers, (AVCodecContext *avctx) );

	declare_func( int ,av_dup_packet, (AVPacket *pkt) );
	declare_func( void, av_packet_unref, (AVPacket *pkt) );
	declare_func( int, av_packet_ref, (AVPacket *dst, AVPacket *src) );

	declare_func( void, av_free_packet, (AVPacket *pkt) );
	declare_func( AVCodec *, avcodec_find_decoder, (enum AVCodecID id) );


	declare_func( int, avpicture_fill, (AVPicture *picture, const uint8_t *ptr,
                   enum AVPixelFormat pix_fmt, int width, int height) );
	declare_func( int, 	av_image_fill_arrays, (uint8_t *dst_data[4], int dst_linesize[4], const uint8_t *src
															 , enum AVPixelFormat pix_fmt, int width, int height, int align) );
	declare_func( void*, av_malloc, (size_t));
	declare_func( int, av_image_get_buffer_size, (enum AVPixelFormat pix_fmt, int width, int height, int align) );
	declare_func( int, avpicture_get_size, (enum AVPixelFormat pix_fmt, int width, int height) );
	declare_func( struct SwsContext *, sws_getContext, (int srcW, int srcH, enum AVPixelFormat srcFormat,
                                  int dstW, int dstH, enum AVPixelFormat dstFormat,
                                  int flags, SwsFilter *srcFilter,
                                  SwsFilter *dstFilter, const double *param) );
	declare_func( void, sws_freeContext, (struct SwsContext *swsContext) );
	declare_func( int, sws_scale, (struct SwsContext *c, const uint8_t *const srcSlice[],
              const int srcStride[], int srcSliceY, int srcSliceH,
              uint8_t *const dst[], const int dstStride[]) );
	declare_func( int64_t, av_gettime,(void) );
	declare_func( int, av_samples_get_buffer_size, (int *linesize, int nb_channels, int nb_samples,
                               enum AVSampleFormat sample_fmt, int align) );
	declare_func( struct SwrContext *, swr_alloc, (void) );
	declare_func( int, av_get_channel_layout_nb_channels, (uint64_t channel_layout) );

	declare_func( int, av_opt_set       ,(void *obj, const char *name, const char *val, int search_flags));
	declare_func( int, av_opt_set_int   ,(void *obj, const char *name, int64_t     val, int search_flags));
	declare_func( int, av_opt_set_double,(void *obj, const char *name, double      val, int search_flags));
	declare_func( int, av_opt_set_q     ,(void *obj, const char *name, AVRational  val, int search_flags));
	declare_func( int, av_opt_set_bin   ,(void *obj, const char *name, const uint8_t *val, int size, int search_flags));
	declare_func( int, av_opt_set_image_size,(void *obj, const char *name, int w, int h, int search_flags));
	declare_func( int, av_opt_set_pixel_fmt ,(void *obj, const char *name, enum AVPixelFormat fmt, int search_flags));
	declare_func( int, av_opt_set_sample_fmt,(void *obj, const char *name, enum AVSampleFormat fmt, int search_flags));
	declare_func( int, av_opt_set_video_rate,(void *obj, const char *name, AVRational val, int search_flags));
	declare_func( int, av_opt_set_channel_layout,(void *obj, const char *name, int64_t ch_layout, int search_flags));
	declare_func( int, swr_init, (struct SwrContext *s) );
	declare_func( int, swr_convert, (struct SwrContext *s, uint8_t **out, int out_count,
                                const uint8_t **in , int in_count) );
	declare_func( int, av_samples_alloc_array_and_samples, (uint8_t ***audio_data, int *linesize, int nb_channels,
                                       int nb_samples, enum AVSampleFormat sample_fmt, int align) );
	declare_func( int, av_samples_alloc, (uint8_t **audio_data, int *linesize, int nb_channels,
                     int nb_samples, enum AVSampleFormat sample_fmt, int align) );
	declare_func( void, av_free, (void *ptr) );
	declare_func( AVIOContext *, avio_alloc_context, (
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (CPROC*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (CPROC*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (CPROC*seek)(void *opaque, int64_t offset, int whence)) );
	declare_func( AVFormatContext *, avformat_alloc_context, (void) );
	declare_func( AVInputFormat *, av_probe_input_format, (AVProbeData *pd, int is_opened) );
	declare_func( int, avio_close, (AVIOContext *s) );
	declare_func( void, avformat_free_context, (AVFormatContext *s) );
	declare_func( int, av_strerror,	(	int 	errnum,char * 	errbuf,size_t 	errbuf_size ) );
	declare_func( Image, GetControlSurface, ( PSI_CONTROL pc ) ); 
	declare_func( void, GetFrameSize, ( PSI_CONTROL pf, uint32_t *w, uint32_t *h ) );
	declare_func( void, UpdateFrameEx, ( PSI_CONTROL pc, int x, int y, int w, int h DBG_PASS ) );
} ffmpeg;

#ifndef USE_OPENSL

static struct openal_interface
{
#define declare_func2(a,b,c) a (ALC_APIENTRY *b) c
#define setup_func2(lib, a,b,c) openal.b=(a(ALC_APIENTRY*)c)LoadFunction( lib, _WIDE(#b) )
#define declare_func3(a,b,c) a (AL_APIENTRY *b) c
#define setup_func3(lib, a,b,c) openal.b=(a(AL_APIENTRY*)c)LoadFunction( lib, _WIDE(#b) )
	declare_func2( ALCdevice *, alcOpenDevice, (CPOINTER) );
	declare_func2( ALCdevice* ,alcCaptureOpenDevice, (const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize) );
	declare_func2( void       ,alcCaptureStart, (ALCdevice *device) );
	declare_func2( void       ,alcCaptureStop, (ALCdevice *device) );
	declare_func2( void       ,alcCaptureSamples, (ALCdevice *device, ALCvoid *buffer, ALCsizei samples) );
	declare_func2( ALCboolean, alcCloseDevice, (ALCdevice *device) );
	declare_func2( ALCcontext *, alcCreateContext, (ALCdevice *,POINTER) );
	declare_func2( void, alcDestroyContext, (ALCcontext *context) );

	declare_func2( ALCboolean, alcMakeContextCurrent, (ALCcontext *context) );
	declare_func2( ALCboolean , alcIsExtensionPresent, (ALCdevice *device, const ALCchar *extname));
	declare_func2( const ALCchar*, alcGetString, (ALCdevice *device, ALCenum param) );
	declare_func2( void          , alcGetIntegerv, (ALCdevice *device, ALCenum param, ALCsizei size, ALCint *values) );
	declare_func3( void, alBufferData,(ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq));
	declare_func3( ALenum, alGetError, (void) );
	declare_func3( void ,  alGenSources, (ALsizei n, ALuint *sources) );
	declare_func3( void , alSourcef, (ALuint source, ALenum param, ALfloat value) );
	declare_func3( void, alSource3f, (ALuint source, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3) );
	declare_func3( void, alSourcefv, (ALuint source, ALenum param, const ALfloat *values) );
	declare_func3( void, alSourcei, (ALuint source, ALenum param, ALint value) );
	declare_func3( void, alSource3i, (ALuint source, ALenum param, ALint value1, ALint value2, ALint value3) );
	declare_func3( void, alSourceiv, (ALuint source, ALenum param, const ALint *values) );
	declare_func3( void,  alGenBuffers, (ALsizei n, ALuint *buffers) );
	declare_func3( void, alSourceQueueBuffers, (ALuint source, ALsizei nb, const ALuint *buffers) );
	declare_func3( void, alSourceUnqueueBuffers, (ALuint source, ALsizei nb, ALuint *buffers) );

	declare_func3( void, alGetSourcef,(ALuint source, ALenum param, ALfloat *value));
	declare_func3( void,alGetSource3f,(ALuint source, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3));
	declare_func3( void, alGetSourcefv,(ALuint source, ALenum param, ALfloat *values));
	declare_func3( void, alGetSourcei,(ALuint source,  ALenum param, ALint *value));
	declare_func3( void, alGetSource3i,(ALuint source, ALenum param, ALint *value1, ALint *value2, ALint *value3));
	declare_func3( void, alGetSourceiv,(ALuint source,  ALenum param, ALint *values));
	declare_func3( void, alSourcePlay, (ALuint source) );
	declare_func3( void, alSourceStop, (ALuint source) );
	declare_func3( void, alDeleteSources, (ALsizei n, const ALuint *sources) );
	declare_func3( void, alDeleteBuffers, (ALsizei n, const ALuint *buffers) );

} openal;
#endif

struct al_buffer
{
#ifdef USE_OPENSL
	uint8_t* buffer; // bytes to oplay
	int samples; // so we know how much of the sound has been played
#else
	ALuint buffer;
	ALuint samples; // so we know how much of the sound has been played
#endif
	size_t size;
	POINTER samplebuf;
};

struct sound_device
{
#ifdef USE_OPENSL
	SLObjectItf engineObject;
	SLEngineItf engineEngine;
	SLObjectItf bqPlayerObject;
	SLObjectItf outputMixObject;
	SLPlayItf bqPlayerPlay;
	SLBufferQueueItf bqPlayerBufferQueue;
#else
	ALCdevice *alc_device;
	ALCcontext *alc_context;
	ALuint al_source;
#endif
};

struct ffmpeg_video_frame {
	uint8_t *rgb_buffer;
	int64_t video_current_pts_time;
	Image output_control_image;
	AVFrame *pVideoFrameRGB;

};

struct ffmpeg_file
{
	struct ffmpeg_file_flag
	{
		BIT_FIELD reading_frames/* : 1*/;
		BIT_FIELD need_video_frame/* : 1*/;
		BIT_FIELD using_video_frame/* : 1*/;
		BIT_FIELD need_audio_frame/* : 1*/;
		BIT_FIELD audio_sleeping/* : 1*/;
		BIT_FIELD no_more_packets/* : 1*/;
		BIT_FIELD sent_end_event/* : 1*/;
		BIT_FIELD close_processing/* : 1*/;
		BIT_FIELD allow_tick/* : 1*/;
		//BIT_FIELD use_internal_tick/* : 1*/;
		BIT_FIELD paused/* : 1*/;   // this is the pause request
		BIT_FIELD need_audio_dequeue/* : 1*/;  // this is a resume(start) condition to attach video to sound
		BIT_FIELD audio_paused/* : 1*/;  // acknowledgement of pause on audio device
		BIT_FIELD video_paused/* : 1*/; // acknowledgement of pause on video device
		BIT_FIELD seek_read/* : 1*/;
		BIT_FIELD force_pkt_pts_in_ms/* : 1*/; // video frame rate correction 1
		BIT_FIELD force_pkt_pts_in_ticks/* : 1*/; // video frame rate correction 1
		BIT_FIELD is_last_block/* : 1*/;
		BIT_FIELD use_soft_queue/* : 1*/;
		BIT_FIELD set_play_anyway/* : 1*/;
		BIT_FIELD clear_pending_video/* : 1*/;
		BIT_FIELD was_playing/* : 1*/; // seek to 0 should check this to prevent seeking when already at the start; set after a single packet is read
		BIT_FIELD video_decoder_sleeping_on_output; // there is already enough frames ready to put on the display... indiciator for display to wake decoder

		struct file_video_flags
		{
			BIT_FIELD first_flush/* : 1*/;
			BIT_FIELD flushing/* : 1*/;
		} video;
	} flags;
	TEXTSTR filename;
	PRENDERER output;
	/* output to a control... this is the control drawn to and updated */
	PSI_CONTROL output_control;
	/* pre_video_render : allows user code to fill in a background in case of transparent videos */
	void (*pre_video_render)( PSI_CONTROL pc );
	/* post_video_render : called after the video is updated to the control... allowing the user to overlay on top of video */
	void (*post_video_render)( PSI_CONTROL pc );
	/* image used with output_control to allow transparent blending */
	//Image output_control_image;
	uint32_t output_width;
	uint32_t output_height;
	uint32_t input_width;
	uint32_t input_height;
	uint32_t video_frame_num_bytes;

	uint64_t seek_position;
	uintptr_t file_size;
	FILE *file_device;
	uint8_t* file_memory;  // instead of a FILE* file might already be in memory.
	uintptr_t file_position_index; // byte index into memory that is current read pointer.
	
	AVIOContext *ffmpeg_io;

	int ffmpeg_buffers_avail;
	int ffmpeg_buffers_count;
	int ffmpeg_buffers_first;
	size_t *ffmpeg_buffer_sizes;
	uint8_t* *ffmpeg_buffers;

	uint8_t* volatile ffmpeg_buffer;
	uint8_t* ffmpeg_buffer_partial;
	size_t ffmpeg_buffer_used_total;
	size_t ffmpeg_buffer_size;
	size_t ffmpeg_buffer_used;
	size_t ffmpeg_buffer_partial_avail;

	AVFormatContext *pFormatCtx;
	int videoStream;
	int audioStream;

	AVCodecContext *pVideoCodecCtx;
	AVCodecContext *pAudioCodecCtx;

	AVCodec *pVideoCodec;
	AVCodec *pAudioCodec;

	int64_t video_decode_start;
	AVFrame *pVideoFrame;
	AVFrame *pAudioFrame;


	PTHREAD stopThread;

	PTHREAD readThread;
	PLINKQUEUE packet_queue; // AVPacket* packets that are available to read into

	PTHREAD videoThread;
	PLINKQUEUE pVideoPackets; //AVPacket*  packets that have been read into and ready to decode
	int video_time_failures;

	PTHREAD audioThread;
	PLINKQUEUE pAudioPackets; //AVPacket* packets that have been read into and ready to decode

	PTHREAD videoOutputThread;
	PLINKQUEUE pEmptyVideoFrames; // struct ffmpeg_video_frame * can be used to decode into
	PLINKQUEUE pDecodedVideoFrames; // struct ffmpeg_video_frame * that have been decoded, ready to play
	LOGICAL output_waiting;
	LOGICAL output_waiting_pause;
	uint32_t video_post_tick; // when we sent the last redraw

	uint64_t videoFrame;
	int64_t frame_del;
				

	struct  SwsContext *img_convert_ctx;

	// for streams that don't have on in-stream...
	int64_t internal_pts; 

	int64_t media_start_time;
	int64_t video_next_pts_time;
	int64_t video_current_pts_time;
	int64_t video_current_pts;
	int64_t video_adjust_ticks;
	int64_t audio_current_pts_time;
	int64_t audio_current_pts;
	int64_t video_current_pts_offset;
	int video_high_water; // default 12 - how many outstanding to force sleeping
	int video_low_water; // default 4 - how many outstanding to start sleeping/to request more input packets


	int64_t audioFrame;
	int64_t audioSamplesPending;
	int64_t audioSamplesPlayed;
	int64_t audioSamples;

	struct SwrContext *audio_converter;
	uint8_t **audio_out;
	uint32_t max_out_samples; // biggest conversion buffer allocated
	int use_channels;

	struct sound_device *sound_device;

	uint64_t al_last_buffer_reclaim;
	int al_format;
	PLINKQUEUE al_free_buffer_queue; // queue of struct al_buffer
	PLINKQUEUE al_used_buffer_queue; // queue of struct al_buffer
	PLINKQUEUE al_pending_buffer_queue; // queue of struct al_buffer
	PLINKQUEUE al_reque_buffer_queue; // queue of struct al_buffer
	int max_in_queue;

	//ALCenum sample_format;
       /** The number of bytes between two consecutive samples of the same channel/component. **/
     //   ALCint sample_step;
        /** If true, print a list of capture devices on this system and exit. **/
       int list_devices;

	TEXTCHAR szTime[256];
	uint64_t last_tick_update;
	void (CPROC*video_position_update)( uintptr_t psv, uint64_t tick );
	uintptr_t psvUpdateParam;
	void (CPROC*video_ended)( uintptr_t psv );
	uintptr_t psvEndedParam;
	void (CPROC*external_video_failure)( CTEXTSTR );
};

static struct ffmpeg_interface_local
{
	PRENDER_INTERFACE pdi;
	PIMAGE_INTERFACE pii;
	PLIST files;
	LOGICAL initialized_video;
	LOGICAL initialized_audio;
	CRITICALSECTION cs_audio_out;
	int x_ofs, y_ofs;
	int default_outstanding_audio;
	int default_outstanding_video;
	int playing; // how many videos are playing
	int stopping; // how many videos are stopping; if stopping, don't being a play
	PLINKQUEUE sound_devices;
	AVPacket blank_packet; // static blank packet
}l;


//---------------------------------------------------------------------------------------------


static void InitFFMPEG_audio( void )
{
	if( l.initialized_audio )
		return;
	l.initialized_audio = TRUE;

	InitializeCriticalSec( &l.cs_audio_out );

	setup_func( lib_psi, Image,GetControlSurface, ( PSI_CONTROL pc ) );
	setup_func( lib_psi, void, GetFrameSize, ( PSI_CONTROL pf, uint32_t *w, uint32_t *h ) );
	setup_func( lib_psi, void, UpdateFrameEx, ( PSI_CONTROL pc, int x, int y, int w, int h DBG_PASS ) );

#ifndef USE_OPENSL

	setup_func2( lib_openal, ALCdevice *, alcOpenDevice, (CPOINTER) );
	setup_func2( lib_openal, ALCdevice* ,alcCaptureOpenDevice, (const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize) );
	setup_func2( lib_openal, void       ,alcCaptureStart, (ALCdevice *device) );
	setup_func2( lib_openal, void       ,alcCaptureStop, (ALCdevice *device) );
	setup_func2( lib_openal, void       ,alcCaptureSamples, (ALCdevice *device, ALCvoid *buffer, ALCsizei samples) );
	setup_func2( lib_openal, ALCboolean, alcCloseDevice, (ALCdevice *device) );
	setup_func2( lib_openal, ALCcontext *, alcCreateContext, (ALCdevice *,POINTER) );
	setup_func2( lib_openal, ALCboolean, alcMakeContextCurrent, (ALCcontext *context) );
	setup_func2( lib_openal, void, alcDestroyContext, (ALCcontext *context) );
	setup_func2( lib_openal, ALCboolean , alcIsExtensionPresent, (ALCdevice *device, const ALCchar *extname));
	setup_func2( lib_openal, const ALCchar*, alcGetString, (ALCdevice *device, ALCenum param) );
	setup_func2( lib_openal, void          , alcGetIntegerv, (ALCdevice *device, ALCenum param, ALCsizei size, ALCint *values) );
	setup_func3( lib_openal, void, alBufferData,(ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq));
	setup_func3( lib_openal, ALenum, alGetError, (void) );
	setup_func3( lib_openal, void,  alGenSources, (ALsizei n, ALuint *sources) );
	setup_func3( lib_openal, void, alSourcef, (ALuint source, ALenum param, ALfloat value) );
	setup_func3( lib_openal, void, alSource3f, (ALuint source, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3) );
	setup_func3( lib_openal, void, alSourcefv, (ALuint source, ALenum param, const ALfloat *values) );
	setup_func3( lib_openal, void, alSourcei, (ALuint source, ALenum param, ALint value) );
	setup_func3( lib_openal, void, alSource3i, (ALuint source, ALenum param, ALint value1, ALint value2, ALint value3) );
	setup_func3( lib_openal, void, alSourceiv, (ALuint source, ALenum param, const ALint *values) );
	setup_func3( lib_openal, void, alGenBuffers, (ALsizei n, ALuint *buffers) );
	setup_func3( lib_openal, void, alSourceQueueBuffers, (ALuint source, ALsizei nb, const ALuint *buffers) );
	setup_func3( lib_openal, void, alSourceUnqueueBuffers, (ALuint source, ALsizei nb, ALuint *buffers) );
	setup_func3( lib_openal, void, alGetSourcef,(ALuint source, ALenum param, ALfloat *value));
	setup_func3( lib_openal, void, alGetSource3f,(ALuint source, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3));
	setup_func3( lib_openal, void, alGetSourcefv,(ALuint source, ALenum param, ALfloat *values));
	setup_func3( lib_openal, void, alGetSourcei,(ALuint source,  ALenum param, ALint *value));
	setup_func3( lib_openal, void, alGetSource3i,(ALuint source, ALenum param, ALint *value1, ALint *value2, ALint *value3));
	setup_func3( lib_openal, void, alGetSourceiv,(ALuint source,  ALenum param, ALint *values));
	setup_func3( lib_openal, void, alSourcePlay, (ALuint source) );
	setup_func3( lib_openal, void, alSourceStop, (ALuint source) );
	setup_func3( lib_openal, void, alDeleteSources, (ALsizei n, const ALuint *sources) );
	setup_func3( lib_openal, void, alDeleteBuffers, (ALsizei n, const ALuint *buffers) );
#endif
}

static void InitFFMPEG_video( void )
{
	if( l.initialized_video )
		return;
	l.initialized_video = TRUE;

	l.pdi = GetDisplayInterface();
	l.pii = GetImageInterface();
	if( !l.pdi || !l.pii )
		return;

	setup_func_test( lib_util, AVFrame*, av_frame_alloc, (	void ) )
				  ; else return;
	setup_func( lib_util, void, av_frame_free, (AVFrame **frame) );
	setup_func( lib_util, void*, av_malloc, (size_t));
	setup_func( lib_util, int64_t, av_gettime,(void) );
	setup_func( lib_util, int, av_samples_get_buffer_size, (int *linesize, int nb_channels, int nb_samples,
                               enum AVSampleFormat sample_fmt, int align) );
	setup_func( lib_codec, int, avcodec_decode_video2,	(	AVCodecContext * 	,
	                                               AVFrame * 	,
	                                               int * 	,
	                                               const AVPacket * 	 
	                                             ) );
	setup_func( lib_format, void, av_register_all,(void) );
	setup_func( lib_format, int, avformat_open_input, (AVFormatContext **, const char *, AVInputFormat *, AVDictionary **) );
	setup_func( lib_format, int, avformat_close_input, (AVFormatContext **) );
	setup_func( lib_format, int, avformat_find_stream_info, (AVFormatContext *, AVDictionary **) );
	setup_func( lib_format, int, av_seek_frame, (AVFormatContext *s, int stream_index, int64_t timestamp, int flags) );

	setup_func( lib_format, int, av_read_frame,(	AVFormatContext * 	, AVPacket * 	  ) );
	setup_func( lib_codec, int, avcodec_decode_audio4,	(	AVCodecContext * 	,
	                                             AVFrame * 	,
	                                             int * 	,
	                                             const AVPacket * 	 
	                                             ) );
	setup_func( lib_codec, int, avcodec_open2, (AVCodecContext *, const AVCodec *, AVDictionary **) );
	setup_func( lib_codec, int, avcodec_close, (AVCodecContext *avctx) );
	setup_func( lib_codec, void, avcodec_flush_buffers, (AVCodecContext *avctx) );
	setup_func( lib_codec, int, av_packet_ref, (AVPacket *dst, AVPacket *src) );
	setup_func( lib_codec, int ,av_dup_packet, (AVPacket *pkt) );
	setup_func( lib_codec, void, av_packet_unref, (AVPacket *pkt) );

	setup_func( lib_codec, void, av_free_packet, (AVPacket *) );
	setup_func( lib_codec, AVCodec *, avcodec_find_decoder, (enum AVCodecID ) );

	setup_func( lib_util, int, 	av_image_fill_arrays, (uint8_t *[4], int [4], const uint8_t *
															 , enum AVPixelFormat , int , int , int ) );
	setup_func( lib_codec, int, av_image_get_buffer_size, (enum AVPixelFormat , int, int, int ) );
	setup_func( lib_codec, int, avpicture_get_size, (enum AVPixelFormat , int , int ) );
	setup_func( lib_swscale, struct SwsContext *, sws_getContext, (int , int , enum AVPixelFormat ,
                                  int , int , enum AVPixelFormat ,
                                  int , SwsFilter *,
                                  SwsFilter *, const double *) );
	setup_func( lib_swscale, void, sws_freeContext, (struct SwsContext *swsContext) );

	setup_func( lib_swscale, int, sws_scale, (struct SwsContext *c, const uint8_t *const srcSlice[],
              const int srcStride[], int srcSliceY, int srcSliceH,
              uint8_t *const dst[], const int dstStride[]) );
	setup_func( lib_resample, struct SwrContext *, swr_alloc, (void) );

   setup_func( lib_util, int, av_get_channel_layout_nb_channels, (uint64_t channel_layout) );
	setup_func( lib_util, int, av_opt_set       ,(void *obj, const char *name, const char *val, int search_flags));
	setup_func( lib_util, int, av_opt_set_int   ,(void *obj, const char *name, int64_t     val, int search_flags));
	setup_func( lib_util, int, av_opt_set_double,(void *obj, const char *name, double      val, int search_flags));
	setup_func( lib_util, int, av_opt_set_q     ,(void *obj, const char *name, AVRational  val, int search_flags));
	setup_func( lib_util, int, av_opt_set_bin   ,(void *obj, const char *name, const uint8_t *val, int size, int search_flags));
	setup_func( lib_util, int, av_opt_set_image_size,(void *obj, const char *name, int w, int h, int search_flags));
	setup_func( lib_util, int, av_opt_set_pixel_fmt ,(void *obj, const char *name, enum AVPixelFormat fmt, int search_flags));
	setup_func( lib_util, int, av_opt_set_sample_fmt,(void *obj, const char *name, enum AVSampleFormat fmt, int search_flags));
	setup_func( lib_util, int, av_opt_set_video_rate,(void *obj, const char *name, AVRational val, int search_flags));
	setup_func( lib_util, int, av_opt_set_channel_layout,(void *obj, const char *name, int64_t ch_layout, int search_flags));
	setup_func( lib_resample, int, swr_init, (struct SwrContext *s) );
	setup_func( lib_resample, int, swr_convert, (struct SwrContext *s, uint8_t **out, int out_count,
                                const uint8_t **in , int in_count) );
	setup_func( lib_util, int, av_samples_alloc_array_and_samples, (uint8_t ***audio_data, int *linesize, int nb_channels,
                                       int nb_samples, enum AVSampleFormat sample_fmt, int align) );
	setup_func( lib_util, int, av_samples_alloc, (uint8_t **audio_data, int *linesize, int nb_channels,
                     int nb_samples, enum AVSampleFormat sample_fmt, int align) );
	setup_func( lib_util, void, av_free, (void *ptr) );
	setup_func( lib_format, AVIOContext *,avio_alloc_context,(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (CPROC*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (CPROC*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (CPROC*seek)(void *opaque, int64_t offset, int whence)) );
	setup_func( lib_format, int, avio_close, (AVIOContext *s) );
	setup_func( lib_format, AVFormatContext *, avformat_alloc_context, (void) );
	setup_func( lib_format, AVInputFormat *, av_probe_input_format, (AVProbeData *pd, int is_opened) );
	setup_func( lib_format, void, avformat_free_context, (AVFormatContext *s) );
	setup_func( lib_util, int, av_strerror,	(	int 	errnum,char * 	errbuf,size_t 	errbuf_size ) );

	setup_func( lib_psi, Image,GetControlSurface, ( PSI_CONTROL pc ) );
	setup_func( lib_psi, void, GetFrameSize, ( PSI_CONTROL pf, uint32_t *w, uint32_t *h ) );
	setup_func( lib_psi, void, UpdateFrameEx, ( PSI_CONTROL pc, int x, int y, int w, int h DBG_PASS ) );

	ffmpeg.av_register_all();


   l.default_outstanding_audio = SACK_GetProfileInt( WIDE("Stream Library"), WIDE("Default high water audio"), 12 );
   l.default_outstanding_video = 32;// SACK_GetProfileInt( WIDE( "Stream Library" ), WIDE( "Default high water video" ), 4 );
}

//---------------------------------------------------------------------------------------------

#ifdef USE_OPENSL
static void SLAPIENTRY bqPlayerCallback(
													 SLBufferQueueItf caller,
													 void * pContext
													)
{
	struct ffmpeg_file * file = (struct ffmpeg_file *)pContext;
	struct al_buffer *buffer;
	SLresult result;
	//lprintf( WIDE("completed a packet....") );

	{
#ifdef DEBUG_AUDIO_PACKET_READ
		//if( file->flags.paused || file->flags.need_audio_dequeue )
		//lprintf( WIDE("Found 1 processed buffers...%d  %d  %d"), GetQueueLength( file->al_free_buffer_queue ), GetQueueLength( file->al_used_buffer_queue ), GetQueueLength( file->al_pending_buffer_queue ) );
#endif
		buffer = (struct al_buffer *)DequeLink( &file->al_used_buffer_queue );
		if( !buffer )
		{
			//lprintf( WIDE("no buffer... re-set play... ") );
			file->flags.set_play_anyway = 1;
		}
		if( !file->flags.paused )
		{
			{
				struct al_buffer *outbuf;
				outbuf = (struct al_buffer*)DequeLink( &file->al_pending_buffer_queue );
				if( !outbuf )
				{
					lprintf( WIDE("ran out of queued buffers... and none pending...") );
					// ran out of buffers, and there's one immediately available...
					// add it...
					file->flags.use_soft_queue = 0;
					file->flags.set_play_anyway = 1;
				}
				else
				{
					result = (*file->bqPlayerBufferQueue)->Enqueue(file->bqPlayerBufferQueue, outbuf->buffer, outbuf->size );
					if( result )
					{
						if( result == SL_RESULT_BUFFER_INSUFFICIENT )
						{
							lprintf( WIDE("Still no room... maybe next time...") );
							PrequeLink( &file->al_pending_buffer_queue, outbuf );
						}
						else
						{
							lprintf( WIDE("Unhandled error enqueue new sample... %d"), result );
					}
					}
					else
					{
						//lprintf( WIDE("use buffer...") );
						EnqueLink( &file->al_used_buffer_queue, outbuf );
					}
				}
			}
			if( buffer )
			{
				file->audioSamplesPlayed += buffer->samples;
				file->audioSamplesPending -= buffer->samples;
#ifdef DEBUG_AUDIO_PACKET_READ
				//lprintf( WIDE("Samples : %d  %d %d"), buffer->samples, file->audioSamplesPlayed, file->audioSamplesPending );
#endif
				//if( buffer->buffer != bufID )
				{
					//lprintf( WIDE("audio queue out of sync") );
					//DebugBreak();
				}
#ifdef DEBUG_AUDIO_PACKET_READ
				//lprintf( WIDE("release audio buffer.. %p"), buffer->samplebuf );
#endif
				ffmpeg.av_free( buffer->samplebuf );
				//LogTime(file, FALSE, WIDE("audio deque"), 1 DBG_SRC );
				EnqueLink( &file->al_free_buffer_queue, buffer );
				if( GetQueueLength( file->al_free_buffer_queue ) > file->max_in_queue )
					file->max_in_queue = GetQueueLength( file->al_free_buffer_queue );
			}
		}
		else
		{
			//lprintf( WIDE("Keeping the buffer to requeue...") );
			EnqueLink( &file->al_reque_buffer_queue, buffer );
		}
	}
	//lprintf( WIDE("save reclaim tick...") );
	file->al_last_buffer_reclaim = ffmpeg.av_gettime();
	// audio got a new play; after a resume, so now wake video thread
	if( file->flags.need_audio_dequeue )
	{
		//lprintf( WIDE("Valid resume state...") );
		file->flags.need_audio_dequeue = 0;
		WakeThread( file->videoThread );
	}
}
#endif

static void RequeueAudio( struct ffmpeg_file *file )
{
#ifdef USE_OPENSL
	{
		SLresult result;
		{
			struct al_buffer *buffer;
			int samples_added = 0;
			while( buffer = (struct al_buffer*)DequeLink( &file->al_reque_buffer_queue ) )
			{
				samples_added++;
				result = (*file->bqPlayerBufferQueue)->Enqueue(file->bqPlayerBufferQueue, buffer->buffer, buffer->size );
				if( result )
				{
					if( result == SL_RESULT_BUFFER_INSUFFICIENT )
					{
						EnqueLink( &file->al_pending_buffer_queue, buffer );
						file->flags.use_soft_queue = 1;
					}
				}
				else
				{
					EnqueLink( &file->al_used_buffer_queue, buffer );
				}
			}
		}
		result = (*file->bqPlayerPlay)->SetPlayState(file->bqPlayerPlay,
																	SL_PLAYSTATE_PLAYING);
	}
#else
	struct al_buffer *buffer;
	int samples_added = 0;
	while( buffer = (struct al_buffer*)DequeLink( &file->al_reque_buffer_queue ) )
	{
		samples_added++;
		openal.alBufferData(buffer->buffer, file->al_format, buffer->samplebuf, (ALsizei)buffer->size, file->pAudioCodecCtx->sample_rate);
		openal.alSourceQueueBuffers(file->sound_device->al_source, 1, &buffer->buffer);
		EnqueLink( &file->al_used_buffer_queue, buffer );
	}
	if( samples_added )
	{
		int val;
		openal.alGetSourcei(file->sound_device->al_source, AL_SOURCE_STATE, &val);
		if(val != AL_PLAYING)
			openal.alSourcePlay(file->sound_device->al_source);
	}
#endif
}

//---------------------------------------------------------------------------------------------

static void dropSoundDevice( struct sound_device **device )
{
	if( device && device[0] )
	{
		EnqueLink( &l.sound_devices, device[0] );
		device[0] = NULL;
	}
}

//---------------------------------------------------------------------------------------------

static struct sound_device *getSoundDevice( struct ffmpeg_file * file )
{
	struct sound_device *sound_device;
	sound_device = (struct sound_device*)DequeLink( &l.sound_devices );
	if( !sound_device )
	{
		sound_device = New( struct sound_device );
		MemSet( sound_device, 0, sizeof( struct sound_device ) );

#ifdef USE_OPENSL
		do
		{
			SLresult result;
			result = slCreateEngine(&sound_device->engineObject, 0, NULL, 0, NULL, NULL);


			result = (*sound_device->engineObject)->Realize(sound_device->engineObject, SL_BOOLEAN_FALSE);

			result = (*sound_device->engineObject)->GetInterface(sound_device->engineObject,
																	SL_IID_ENGINE, &(sound_device->engineEngine));
			{
				//const SLInterfaceID ids[] = {SL_IID_VOLUME};
				//const SLboolean req[] = {SL_BOOLEAN_FALSE};
				result = (*sound_device->engineEngine)->CreateOutputMix(sound_device->engineEngine,
																			&(sound_device->outputMixObject), 0, NULL, NULL /*ids, req*/);
				result = (*sound_device->outputMixObject)->Realize(sound_device->outputMixObject,
																	 SL_BOOLEAN_FALSE);
				{
					SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
					SLDataFormat_PCM format_pcm;// = {SL_DATAFORMAT_PCM
													 
					SLDataSource audioSrc = {&loc_bufq, &format_pcm};
					format_pcm.formatType = SL_DATAFORMAT_PCM;
															format_pcm.numChannels= file->use_channels;
															format_pcm.samplesPerSec = SL_SAMPLINGRATE_44_1;//file->pAudioCodecCtx->sample_rate;
															format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
															format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
															format_pcm.channelMask = file->use_channels==2?0:1;
															format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
					{
						SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX
																		 ,	file->outputMixObject};
						SLDataSink audioSnk = {&loc_outmix, NULL};
						{
							const SLInterfaceID ids1[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
							const SLboolean req1[] = {SL_BOOLEAN_TRUE};
							result = (*file->engineEngine)->CreateAudioPlayer(sound_device->engineEngine,
																							  &(sound_device->bqPlayerObject), &audioSrc, &audioSnk,
																							  1, ids1, req1);
							if( result )
							{
								lprintf( WIDE("Create Audio Player failed.") );
								break;
							}
							result = (*sound_device->bqPlayerObject)->Realize(sound_device->bqPlayerObject,
																					SL_BOOLEAN_FALSE);
							if( result )
							{
								lprintf( WIDE("Realize Audio Player failed.") );
								break;
							}
						}
					}
				}
				result = (*sound_device->bqPlayerObject)->GetInterface(sound_device->bqPlayerObject,
																			  SL_IID_PLAY,&(sound_device->bqPlayerPlay));
				result = (*sound_device->bqPlayerObject)->GetInterface(sound_device->bqPlayerObject,
																			  SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &(sound_device->bqPlayerBufferQueue));

				// setup callbacks
				lprintf( WIDE(".... buffer queue add playback callback...") );
				result = (*sound_device->bqPlayerBufferQueue)->RegisterCallback( sound_device->bqPlayerBufferQueue, bqPlayerCallback, file);
			}
		}
		while( 0 );
#else

		sound_device->alc_device = openal.alcOpenDevice( NULL );
		if( 0 )
		{
			ALCboolean enumeration;

			enumeration = openal.alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
			if (enumeration == AL_FALSE)
			{
				lprintf( WIDE("Failed to be able to enum..") );
					// enumeration not supported
			}
			else
			{
				const ALCchar *devices = openal.alcGetString(NULL, ALC_DEVICE_SPECIFIER );
				const ALCchar *device = devices, *next = devices + 1;
				size_t len = 0;

				while (device && *device != '\0' && next && *next != '\0') {
					lprintf(WIDE("OpenAL Device: %s"), device);
					len = strlen(device);
					device += (len + 1);
					next += (len + 2);
				}
			}
		}

		sound_device->alc_context = openal.alcCreateContext(sound_device->alc_device, NULL);
		if( openal.alcMakeContextCurrent(sound_device->alc_context) )
		{
			openal.alGenSources((ALuint)1, &sound_device->al_source);
			// check for errors
			openal.alSourcef(sound_device->al_source, AL_PITCH, 1);
			// check for errors
			openal.alSourcef(sound_device->al_source, AL_GAIN, 1);
			// check for errors
			openal.alSource3f(sound_device->al_source, AL_POSITION, 0, 0, 0);
			// check for errors
			openal.alSource3f(sound_device->al_source, AL_VELOCITY, 0, 0, 0);
			// check for errors
			openal.alSourcei(sound_device->al_source, AL_LOOPING, AL_FALSE);
			// check for errros
		}

#endif
	}

	return sound_device;
}

//---------------------------------------------------------------------------------------------

static void ClearRequeueAudio( struct ffmpeg_file *file )
{
#ifndef USE_OPENSL
	struct al_buffer *buffer;
	{
		int frameFinished;
		AVPacket flush_packet;
		MemSet( &flush_packet, 0, sizeof( AVPacket ) );
		//flush_packet.data = NULL;
		//flush_packet.size = 0;
		//flush_packet.buf = NULL;
		//flush_packet.side_data = NULL;
		//lprintf( WIDE("flush audio codec.") );
		if( file->pAudioCodecCtx ) do
		{
			ffmpeg.avcodec_decode_audio4(file->pAudioCodecCtx, file->pAudioFrame, &frameFinished, &flush_packet );
		}
		while( frameFinished );
	}
	while( buffer = (struct al_buffer*)DequeLink( &file->al_reque_buffer_queue ) )
	{
		//lprintf( WIDE("release audio buffer..") );
		ffmpeg.av_free( buffer->samplebuf );
		//LogTime(file, FALSE, WIDE("audio deque") DBG_SRC );
		EnqueLink( &file->al_free_buffer_queue, buffer );
		if( GetQueueLength( file->al_free_buffer_queue ) > file->max_in_queue )
			file->max_in_queue = (int)GetQueueLength( file->al_free_buffer_queue );
	}
	{
		AVPacket *packet;
		while( packet = (AVPacket*)DequeLink( &file->pAudioPackets ) )
		{
			//lprintf( WIDE("Call free packet..") );
			ffmpeg.av_free_packet(packet);
			EnqueLink( &file->packet_queue, packet );
		}
	}
#endif
}

//---------------------------------------------------------------------------------------------

struct ffmpeg_video_frame *GetVideoFrame( struct ffmpeg_file *file ) {

	struct ffmpeg_video_frame *frame = (struct ffmpeg_video_frame *)DequeLink( &file->pEmptyVideoFrames );
	if( !frame ) {
		frame = New( struct ffmpeg_video_frame );
		MemSet( frame, 0, sizeof( struct ffmpeg_video_frame ) );
		{
			//if( frame->rgb_buffer )
			//	ffmpeg.av_free( file->rgb_buffer );
			frame->pVideoFrameRGB = ffmpeg.av_frame_alloc();
			frame->rgb_buffer = (uint8_t *)ffmpeg.av_malloc(file->video_frame_num_bytes*sizeof(uint8_t));

			ffmpeg.av_image_fill_arrays( frame->pVideoFrameRGB->data, frame->pVideoFrameRGB->linesize
												, frame->rgb_buffer, PIX_FMT
												, file->output_width, file->output_height, 32);
			//ffmpeg.avpicture_fill((AVPicture*)frame->pVideoFrameRGB, frame->rgb_buffer, PIX_FMT
			//							, file->output_width, file->output_height);
		}
		frame->output_control_image = RemakeImage( frame->output_control_image, (PCOLOR)frame->rgb_buffer, file->output_width, file->output_height );
#ifdef WIN32
		frame->output_control_image->flags &= ~IF_FLAG_INVERTED;
#endif
	}
	return frame;
}


//---------------------------------------------------------------------------------------------


static void SetAudioPlay( struct ffmpeg_file *file )
{
#ifdef USE_OPENSL
	{
		SLresult result;
		SLuint32 playState;
		result = (*file->bqPlayerPlay)->GetPlayState(file->bqPlayerPlay, &playState);
		if( ( playState != SL_PLAYSTATE_PLAYING || file->flags.set_play_anyway ) )
		{
			lprintf( WIDE("SET PLAYING") );
			file->flags.set_play_anyway = 0;
			result = (*file->bqPlayerPlay)->SetPlayState(file->bqPlayerPlay,
																		SL_PLAYSTATE_PLAYING);
			if( result != SL_RESULT_SUCCESS )
			{
				lprintf( WIDE("error to play.... %d"), result );
			}
		}
	}
#endif
}

static void GetAudioBuffer( struct ffmpeg_file * file, POINTER data, size_t sz
#ifdef USE_OPENSL
                           , int samples
#else
								  , ALuint samples
#endif
								  )
{
#ifdef USE_OPENSL
	SLresult result;
#else
	ALenum error;
#endif
	struct al_buffer * buffer;
	if( !( buffer = (struct al_buffer*)DequeLink( &file->al_free_buffer_queue ) ) )
	{
		buffer = New( struct al_buffer );
#ifdef USE_OPENSL
		buffer->buffer = NewArray( uint8_t, sz );
		MemCpy( buffer->buffer, data, sz );
#else
		openal.alGenBuffers( 1, &buffer->buffer );
#endif
	}
	buffer->samplebuf = data;
	buffer->size = sz;
	buffer->samples = samples;

	if( !file->sound_device )
		file->sound_device = getSoundDevice( file );

#ifdef DEBUG_AUDIO_PACKET_READ
	//lprintf( WIDE("use buffer (%d)  %p  for %d(%d)"), file->in_queue, buffer, sz, samples );
#endif

#ifdef USE_OPENSL
	if( file->flags.use_soft_queue )
	{
		//lprintf( WIDE("just pend this in the soft queue....") );
		EnqueLink( &file->al_pending_buffer_queue, buffer );
	}
	else if( file->bqPlayerBufferQueue )
	{
		result = (*file->bqPlayerBufferQueue)->Enqueue(file->sound_device->bqPlayerBufferQueue, data, sz );
		if( result  )
		{
			if( result == SL_RESULT_BUFFER_INSUFFICIENT )
			{
				lprintf( WIDE("overflow driver, enable soft queue...") );
				file->flags.use_soft_queue = 1;
				EnqueLink( &file->al_pending_buffer_queue, buffer );
			}
			else
				lprintf( WIDE("failed to enqueue: %d"), result );
		}
		else
		{
			EnqueLink( &file->al_used_buffer_queue, buffer );
		}
	}
	else
	{
		Release( buffer->buffer );
		EnqueLink( &file->al_free_buffer_queue, buffer );
	}
	if( file->videoFrame )
		SetAudioPlay( file );
#else
	openal.alBufferData(buffer->buffer, file->al_format, data, (ALsizei)sz, file->pAudioCodecCtx->sample_rate);
	openal.alSourceQueueBuffers(file->sound_device->al_source, 1, &buffer->buffer);
	if(( error = openal.alGetError()) != AL_NO_ERROR)
	{
			lprintf( WIDE("error adding audio buffer to play.... %d"), error );
	}
	EnqueLink( &file->al_used_buffer_queue, buffer );

	{
		int val;
		openal.alGetSourcei(file->sound_device->al_source, AL_SOURCE_STATE, &val);
		if(val != AL_PLAYING)
		{
			openal.alSourcePlay(file->sound_device->al_source);
			if(( error = openal.alGetError()) != AL_NO_ERROR)
			{
				lprintf( WIDE("error to play.... %d"), error );
			}
		}
	}
#endif
}

//---------------------------------------------------------------------------------------------

static void EnableAudioOutput( struct ffmpeg_file * file )
{
	EnterCriticalSec( &l.cs_audio_out );

	file->audio_converter = ffmpeg.swr_alloc();
	if( !file->pAudioCodecCtx->channels )
	{
		lprintf( WIDE("input audio had no channels... %x"), file->pAudioCodecCtx->channel_layout );
		file->pAudioCodecCtx->channels = ffmpeg.av_get_channel_layout_nb_channels( file->pAudioCodecCtx->channel_layout );
		lprintf( WIDE("Channel found in channel_layout (%d)"),file->pAudioCodecCtx->channels );
		if( !file->pAudioCodecCtx->channels )
			file->pAudioCodecCtx->channels = 1;
		//file->pAudioCodecCtx->channels = 1;
		//= file->pAudioCodecCtx->request_channels;
	}
	if (file->pAudioCodecCtx->channels == 1)
	{
		file->use_channels = 1;
		file->al_format = AL_FORMAT_MONO16;
	}
	else if( file->pAudioCodecCtx->channels >= 2)
	{
	}
	file->use_channels = 2;
	file->al_format = AL_FORMAT_STEREO16;


	if( !file->pAudioCodecCtx->sample_rate )
		file->pAudioCodecCtx->sample_rate = file->pAudioCodecCtx->frame_size;

	//lprintf( WIDE("thing %s"), openal.alcGetString(file->alc_device, ALC_DEVICE_SPECIFIER) );
#ifdef DEBUG_LOG_INFO
	lprintf( WIDE("Pretending channels is %d"), file->pAudioCodecCtx->channels );
#endif
	if( file->pAudioCodecCtx->channels == 2 )
	{
#ifdef DEBUG_LOG_INFO
		lprintf( WIDE("Stereo format") );
#endif
		ffmpeg.av_opt_set_int(file->audio_converter, "in_channel_layout",  AV_CH_LAYOUT_STEREO, 1 );
	}
	else if( file->pAudioCodecCtx->channels == 4 )
	{
#ifdef DEBUG_LOG_INFO
		lprintf( WIDE("quad input format, using Stereo format") );
#endif
		ffmpeg.av_opt_set_int(file->audio_converter, "in_channel_layout",  AV_CH_LAYOUT_STEREO, 1 );
	}
	else if( file->pAudioCodecCtx->channels == 1 )
	{
		ffmpeg.av_opt_set_int(file->audio_converter, "in_channel_layout",  AV_CH_LAYOUT_MONO, 1 );
	}
	else if( file->pAudioCodecCtx->channels == 6 )
		ffmpeg.av_opt_set_int(file->audio_converter, "in_channel_layout",  AV_CH_LAYOUT_5POINT1, 1 );
	else
		ffmpeg.av_opt_set_int(file->audio_converter, "in_channel_layout",  file->pAudioCodecCtx->channel_layout, 1 );


	if( file->use_channels == 2 )
		ffmpeg.av_opt_set_int(file->audio_converter, ("out_channel_layout"), AV_CH_LAYOUT_STEREO,  1);
	else
		ffmpeg.av_opt_set_int(file->audio_converter, ("out_channel_layout"), AV_CH_LAYOUT_MONO,  1);

	ffmpeg.av_opt_set_int(file->audio_converter, ("in_sample_rate"),     file->pAudioCodecCtx->sample_rate,                0);
	ffmpeg.av_opt_set_int(file->audio_converter, ("out_sample_rate"),    file->pAudioCodecCtx->sample_rate,                0);
	//DebugBreak();
#ifdef DEBUG_LOG_INFO
	lprintf( WIDE("Sample rate is %d"), file->pAudioCodecCtx->sample_rate );
	lprintf( WIDE("codec is : %d %d"), file->pAudioCodecCtx->codec_id, CODEC_ID_MP3 );
	lprintf( WIDE("codec is : %d %d"), file->pAudioCodecCtx->codec_id, AV_CODEC_ID_AAC );
	lprintf( WIDE(" sample is : %d  %d"), file->pAudioCodecCtx->sample_fmt, AV_SAMPLE_FMT_S16 );
#endif
	if(file->pAudioCodecCtx->codec_id == CODEC_ID_MP3 )
	{
		//ffmpeg.av_opt_set_sample_fmt(file->audio_converter, WIDE("in_sample_fmt"),  AV_SAMPLE_FMT_S16, 0);
		ffmpeg.av_opt_set_sample_fmt(file->audio_converter, "in_sample_fmt",  AV_SAMPLE_FMT_S16P, 0);
		//ffmpeg.av_opt_set_sample_fmt(file->audio_converter, WIDE("in_sample_fmt"),  AV_SAMPLE_FMT_U8, 0);
		//ffmpeg.av_opt_set_sample_fmt(file->audio_converter, WIDE("in_sample_fmt"),  AV_SAMPLE_FMT_U8P, 0);
		//ffmpeg.av_opt_set_sample_fmt(file->audio_converter, WIDE("in_sample_fmt"),  AV_SAMPLE_FMT_FLTP, 0);
		//ffmpeg.av_opt_set_sample_fmt(file->audio_converter, WIDE("in_sample_fmt"),  AV_SAMPLE_FMT_FLT, 0);
	}
	else if( file->pAudioCodecCtx->codec_id, AV_CODEC_ID_AAC )
		ffmpeg.av_opt_set_sample_fmt(file->audio_converter, "in_sample_fmt",  AV_SAMPLE_FMT_FLTP, 0);
	else if( file->pAudioCodecCtx->codec_id, AV_CODEC_ID_AC3 )
		ffmpeg.av_opt_set_sample_fmt(file->audio_converter, "in_sample_fmt",  AV_SAMPLE_FMT_S16P, 0);
	else

		ffmpeg.av_opt_set_sample_fmt(file->audio_converter, "in_sample_fmt",  file->pAudioCodecCtx->sample_fmt, 0);
	ffmpeg.av_opt_set_sample_fmt(file->audio_converter, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
	//lprintf( WIDE("Init swr") );
	ffmpeg.swr_init( file->audio_converter );

	file->sound_device = getSoundDevice( file );

	// sound device should be a constant format... 
	dropSoundDevice( &file->sound_device );

	LeaveCriticalSec( &l.cs_audio_out );
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

static void PushBuffer( struct ffmpeg_file *file )
{
	if( file->ffmpeg_buffers_count == file->ffmpeg_buffers_avail )
	{
		size_t *new_sizes;
		uint8_t* *new_bufs;
		int n;
		file->ffmpeg_buffers_avail += 16;
		new_bufs = NewArray( uint8_t*, file->ffmpeg_buffers_avail );
		new_sizes = NewArray( size_t, file->ffmpeg_buffers_avail );
		for( n = file->ffmpeg_buffers_first; n < file->ffmpeg_buffers_count; n++ )
		{
			new_bufs[n] = file->ffmpeg_buffers[n];
			new_sizes[n] = file->ffmpeg_buffer_sizes[n];
		}
		//lprintf( WIDE("push buffer, release %p"), file->ffmpeg_buffers );
		//lprintf( WIDE("push buffer, release %p"), file->ffmpeg_buffer_sizes );
		Release( file->ffmpeg_buffers );
		Release( file->ffmpeg_buffer_sizes );
		file->ffmpeg_buffers = new_bufs;
		file->ffmpeg_buffer_sizes = new_sizes;
	}

	if( file->ffmpeg_buffer_used_total == 0 )
	{
		FILE *fileout = fopen( "movie.mp4", "wb" );
		if( fileout )
		{
			fwrite( file->ffmpeg_buffer, 1, file->ffmpeg_buffer_size, fileout );
			fclose( fileout );
		}
		else
			lprintf( WIDE("Failed to create file to out") );
	}
	else
	{
		FILE *fileout = fopen( "movie.mp4", "ab+" );
		if( fileout )
		{
			int pos = ftell( fileout );
			lprintf( WIDE("file size is now %d"), pos );
			fwrite( file->ffmpeg_buffer, 1, file->ffmpeg_buffer_size, fileout );
			fclose( fileout );
		}
		else
			lprintf( WIDE("Failed to append file to out") );
	}
	// need a new buffer, this is returned to the input library...
	file->ffmpeg_buffers[file->ffmpeg_buffers_count] = NewArray( uint8_t, file->ffmpeg_buffer_size );
	MemCpy( file->ffmpeg_buffers[file->ffmpeg_buffers_count], file->ffmpeg_buffer, file->ffmpeg_buffer_size );
	file->ffmpeg_buffer_sizes[file->ffmpeg_buffers_count] = file->ffmpeg_buffer_size;
	file->ffmpeg_buffer_used_total += file->ffmpeg_buffer_size;

	file->ffmpeg_buffers_count++;
	file->ffmpeg_buffer = NULL;
}

int CPROC ffmpeg_read_packet(void *opaque, uint8_t *buf, int buf_size)
{
	struct ffmpeg_file *file = 	(struct ffmpeg_file *)opaque;
#ifdef DEBUG_LOW_LEVEL_PACKET_IO
	lprintf( WIDE("read %d at %d of %d (%d remain)"), buf_size, file->ffmpeg_buffer_used, file->ffmpeg_buffer_size, file->ffmpeg_buffer_size - file->ffmpeg_buffer_used );
#endif
	if( ANDROID_MODE )
	{
		size_t result_size = buf_size;
		if( buf_size > ( file->ffmpeg_buffer_size - file->ffmpeg_buffer_used ) )
		{
			if( file->ffmpeg_buffer_partial )
			{
				//lprintf( WIDE("partial release %d"), file->ffmpeg_buffer_partial );
				Release( file->ffmpeg_buffer_partial );
			}
			if( file->ffmpeg_buffer_size - file->ffmpeg_buffer_used )
			{
#ifdef DEBUG_LOW_LEVEL_PACKET_IO
				lprintf( WIDE("need part of the end and the next...") );
#endif
				file->ffmpeg_buffer_partial = NewArray( uint8_t, file->ffmpeg_buffer_size - file->ffmpeg_buffer_used );
				file->ffmpeg_buffer_partial_avail = file->ffmpeg_buffer_size - file->ffmpeg_buffer_used;
				MemCpy( file->ffmpeg_buffer_partial, file->ffmpeg_buffer, file->ffmpeg_buffer_partial_avail );
			}
			else
			{
				file->ffmpeg_buffer_partial = NULL;
				file->ffmpeg_buffer_partial_avail = 0;
			}

			// block is used entirely, so used it as a base number
			file->ffmpeg_buffer_used_total += file->ffmpeg_buffer_size;

			if( ( file->ffmpeg_buffers_first == file->ffmpeg_buffers_count && ( !file->flags.is_last_block ) ) 
				|| ( file->ffmpeg_buffers_count == file->ffmpeg_buffers_avail ) )
			{
				do
				{
				}while( !file->ffmpeg_buffer );

				if( file->ffmpeg_buffer )
				{
					FILE *fileout = fopen( "movie.mp4", "ab+" );
					if( fileout )
					{
						int pos = ftell( fileout );
						lprintf( WIDE("file size is now %d (will be %d)"), pos, pos + file->ffmpeg_buffer_size );
						fwrite( file->ffmpeg_buffer, 1, file->ffmpeg_buffer_size, fileout );
						fclose( fileout );
					}
					else
						lprintf( WIDE("Failed to append file to out") );
				}
			}
			else
			{
				if( file->ffmpeg_buffers_first < file->ffmpeg_buffers_count )
				{
					file->ffmpeg_buffer = file->ffmpeg_buffers[file->ffmpeg_buffers_first];
					file->ffmpeg_buffer_size = file->ffmpeg_buffer_sizes[file->ffmpeg_buffers_first];
					file->ffmpeg_buffers_first++;
				}
				else
				{
					file->ffmpeg_buffer = NULL;
					file->ffmpeg_buffer_size = 0;
				}
			}
			file->ffmpeg_buffer_used = 0;
		}

		{
			if( file->ffmpeg_buffer_partial )
			{
				memcpy( buf, file->ffmpeg_buffer_partial, file->ffmpeg_buffer_partial_avail );
				buf_size -= (int)file->ffmpeg_buffer_partial_avail;
				buf += file->ffmpeg_buffer_partial_avail;
				//lprintf( WIDE("release partial %d"), file->ffmpeg_buffer_partial_avail );
				Release( file->ffmpeg_buffer_partial );
				file->ffmpeg_buffer_partial = NULL;
			}
			if( file->ffmpeg_buffer )
			{
				if( buf_size >= ( file->ffmpeg_buffer_size - file->ffmpeg_buffer_used ) )
				{
					result_size -= buf_size - ( file->ffmpeg_buffer_size - file->ffmpeg_buffer_used );
					buf_size = (int)( file->ffmpeg_buffer_size - file->ffmpeg_buffer_used );
					lprintf( WIDE("adjusted sizes %d %d"), result_size, buf_size );
				}
				if( buf_size )
				{
					memcpy( buf, file->ffmpeg_buffer + file->ffmpeg_buffer_used, buf_size );
				}
				file->ffmpeg_buffer_used += buf_size;
				return (int)result_size;
			}
			return (int)file->ffmpeg_buffer_partial_avail;
		}
		//lprintf( WIDE("data copied to read buffer...%d"), result_size );
		//LogBinary( buf, (256<result_size)?256:result_size );
	}
	else
	{
		size_t result_size;
		if( file->file_memory ) {
			if( (file->file_position_index + buf_size) > file->file_size )
				buf_size = (int)(file->file_size - file->file_position_index);
			memcpy( buf, file->file_memory + file->file_position_index, result_size = buf_size );
			file->file_position_index += buf_size;
		} 
		else
			result_size = sack_fread( buf, buf_size, 1, file->file_device );

		//lprintf( WIDE("data copied to read buffer...") );
		//LogBinary( buf, (256<result_size)?256:result_size );
		return (int)result_size;
	}
}

int CPROC ffmpeg_write_packet(void *opaque, uint8_t *buf, int buf_size)
{
	struct ffmpeg_file *file = 	(struct ffmpeg_file *)opaque;
	if( file->file_device )
		return (int)sack_fwrite( buf, buf_size, 1, file->file_device );
	return 0;
}

int64_t CPROC ffmpeg_seek(void *opaque, int64_t offset, int whence)
{
	struct ffmpeg_file *file = 	(struct ffmpeg_file *)opaque;
#ifdef DEBUG_LOW_LEVEL_PACKET_IO
	lprintf( WIDE("Seek %lld  %lld %d %s"), offset, file->file_size- offset, whence,  (whence==AVSEEK_SIZE)?WIDE("size"):WIDE("") );
#endif
	if( whence == AVSEEK_SIZE )
	{
		if( ANDROID_MODE )
		{
			return  file->file_size = 119605674;
		}
		else
		{
			if( file->file_memory ) {
				return file->file_size;
			}
			else {
				size_t here = sack_ftell( file->file_device );
				size_t length;
				sack_fseek( file->file_device, 0, SEEK_END );
				length = sack_ftell( file->file_device );
				sack_fseek( file->file_device, here, SEEK_SET );
				file->file_size = length;
				return length;
			}
		}
	}
	else
	{
		if( ANDROID_MODE )
		{
			if( whence == 0 && offset == 0 )
			{
				if( file->ffmpeg_buffers )
				{
					file->ffmpeg_buffer = file->ffmpeg_buffers[0];
					file->ffmpeg_buffer_size = file->ffmpeg_buffer_sizes[0];
				}
				file->ffmpeg_buffer_used_total = 0;
				file->ffmpeg_buffer_used = 0;
				return 0;
			}
			else
			{
				if( whence == 0 && file->ffmpeg_buffers_count )
				{
					size_t tmpofs = offset;
					int n;
					lprintf( WIDE("seeking into existing buffers...") );
					for( n = 0; n < file->ffmpeg_buffers_count; n++ )
						if( tmpofs > file->ffmpeg_buffer_sizes[n] )
							tmpofs -= file->ffmpeg_buffer_sizes[n];
						else
							break;
					file->ffmpeg_buffer = file->ffmpeg_buffers[n];
					file->ffmpeg_buffer_size = file->ffmpeg_buffer_sizes[n];
					file->ffmpeg_buffer_used = tmpofs;
					return offset;
				}
				if( whence == 0 && offset < file->ffmpeg_buffer_size )
					file->ffmpeg_buffer_used = offset;
				else if( whence == 1 && (offset+file->ffmpeg_buffer_used) < file->ffmpeg_buffer_size )
					file->ffmpeg_buffer_used = offset+file->ffmpeg_buffer_used;
				else
				{
					size_t need_blocks = whence==0?offset:whence==2?(file->file_size-offset):whence==1?(file->ffmpeg_buffer_used+offset ):0;
					lprintf( WIDE("only valid if this seek happens during the first block! this size is %d"), file->ffmpeg_buffer_size );
					need_blocks = need_blocks - file->ffmpeg_buffer_size;
					need_blocks = need_blocks - file->ffmpeg_buffer_used_total;
					PushBuffer( file );

					lprintf( WIDE("buffer used total is %d (%d)"), file->ffmpeg_buffer_used_total, file->file_size - (file->ffmpeg_buffer_used_total+need_blocks) );

					do
					{
						lprintf( WIDE("got buf and size %p %d"), file->ffmpeg_buffer, file->ffmpeg_buffer_size );
						if( !file->ffmpeg_buffer )
						{
							if( !file->flags.is_last_block )
								lprintf( WIDE("waiting...%s"), file->flags.is_last_block?WIDE("for last..."):WIDE("") );
							WakeableSleep(1000);
						}
						else
						{
							if( need_blocks >= file->ffmpeg_buffer_size )
							{
								lprintf( WIDE("still need %d ... got another %d "), need_blocks, file->ffmpeg_buffer_size );
								need_blocks -=  file->ffmpeg_buffer_size;
								PushBuffer( file );
								lprintf( WIDE("buffer used total is %d (%d)"), file->ffmpeg_buffer_used_total, file->file_size - (file->ffmpeg_buffer_used_total+need_blocks) );
							}
							else
							{
								{
									FILE *fileout = fopen( "movie.mp4", "ab+" );
									int pos = ftell( fileout );
									lprintf( WIDE("file size is now %d"), pos );
									fwrite( file->ffmpeg_buffer, 1, file->ffmpeg_buffer_size, fileout );
									fclose( fileout );
								}
							}
						}
					}while( !file->ffmpeg_buffer );
					file->ffmpeg_buffer_used = need_blocks;
					lprintf( WIDE("resulting position is %d (%d) of %d (%d)"), file->ffmpeg_buffer_used, file->ffmpeg_buffer_size - file->ffmpeg_buffer_used, file->ffmpeg_buffer_size, file->ffmpeg_buffer_used_total );
					lprintf( WIDE("... longer seek need read? %d"), file->file_size - ( file->ffmpeg_buffer_used_total + file->ffmpeg_buffer_used ) );
				}
				lprintf( WIDE("result %d should be %d"), 14833013, file->ffmpeg_buffer_used_total + file->ffmpeg_buffer_used );
				return file->ffmpeg_buffer_used_total + file->ffmpeg_buffer_used;
			}

		}
		else
		{
			if( file->file_memory ) {
				if( whence == SEEK_SET )
					return file->file_position_index = offset;
				else if( whence == SEEK_CUR )
					return file->file_position_index += offset;
				else if( whence == SEEK_END )
					return file->file_position_index = file->file_size + offset;
				return 0;
			}
			else
				return sack_fseek( file->file_device, offset, whence);
		}
	}
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

static void UpdateSwsContext( struct ffmpeg_file *file, struct ffmpeg_video_frame *frame )
{
	if( ( file->input_width != file->pVideoFrame->width ) ||
		( file->input_height != file->pVideoFrame->height ) )
	{
		ffmpeg.sws_freeContext( file->img_convert_ctx );
		file->img_convert_ctx = ffmpeg.sws_getContext( file->input_width = file->pVideoFrame->width
																	, file->input_height = file->pVideoFrame->height
																	, file->pVideoCodecCtx->pix_fmt
																	, file->output_width, file->output_height
																	, PIX_FMT, SWS_BICUBIC, NULL, NULL, NULL);

	}
}

//---------------------------------------------------------------------------------------------

void ffmpeg_UpdateControlLayout( struct ffmpeg_file *file )
{
	if( file )
	{
		uint32_t width, height;
		ffmpeg.GetFrameSize( file->output_control, &width, &height );
		if( file->img_convert_ctx )
			ffmpeg.sws_freeContext( file->img_convert_ctx );
		file->output_width = width;
		file->output_height = height;
		file->input_width = file->pVideoCodecCtx->width;
		file->input_height = file->pVideoCodecCtx->height;
		file->img_convert_ctx = ffmpeg.sws_getContext(file->pVideoCodecCtx->width, file->pVideoCodecCtx->height
			, file->pVideoCodecCtx->pix_fmt 
			, width, height
			, PIX_FMT, SWS_BICUBIC, NULL, NULL, NULL);
		file->video_frame_num_bytes = ffmpeg.av_image_get_buffer_size(PIX_FMT, file->output_width, file->output_height, 32);

			ffmpeg.avpicture_get_size(PIX_FMT, file->output_width, file->output_height);
	}
}

//---------------------------------------------------------------------------------------------

void ffmpeg_SetPrePostDraw( struct ffmpeg_file *file, void (*pre_render)(PSI_CONTROL), void (*post_render)(PSI_CONTROL) )
{
	if( file )
	{
		file->pre_video_render = pre_render;
		file->post_video_render = post_render;
	}
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

struct ffmpeg_file * ffmpeg_LoadFile( CTEXTSTR filename
                                    , PRENDERER (CPROC*getDisplay)(uintptr_t psv,uint32_t w, uint32_t h), uintptr_t psvGetDisplay 
                                    , PSI_CONTROL output_control
                                    , void (CPROC*video_position_update)( uintptr_t psv, uint64_t tick ), uintptr_t psvUpdate
                                    , void (CPROC*video_ended)( uintptr_t psv ), uintptr_t psvEnded
                                    , void (CPROC*play_error)( CTEXTSTR message )
                                    )

{
	struct ffmpeg_file *file;
	AVDictionary *options = NULL;
	int err;
	// this sort of thing is used for a pre-allocated pFormatCtx
	// http://www.ffmpeg.org/doxygen/trunk/group__lavf__decoding.html
	//av_dict_set(&options, WIDE("video_size"), WIDE("640x480"), 0);
	//av_dict_set(&options, WIDE("pixel_format"), WIDE("rgb32"), 0);
	InitFFMPEG_video();
	InitFFMPEG_audio();
	file = New( struct ffmpeg_file );
	MemSet( file, 0, sizeof( struct ffmpeg_file ) );
	file->video_low_water = 4;
	file->video_high_water = 12;
	file->filename = StrDup( filename );
	file->al_free_buffer_queue = CreateLinkQueue( );
	file->al_used_buffer_queue = CreateLinkQueue( );
	file->al_pending_buffer_queue = CreateLinkQueue( );
	file->video_position_update = video_position_update;
	file->psvUpdateParam = psvUpdate;
	file->video_ended = video_ended;
	file->psvEndedParam = psvEnded;
	file->external_video_failure = play_error;
	file->flags.video_paused = 1;
	file->flags.audio_paused = 1;
	file->flags.paused = 1;

	// stream movie reference; so we can use normal file IO also
	if( filename[0] == '$' )
	{
		filename++;
		// IStream-Interface that was already created elsewhere:
		//file->ffmpeg_buffer = NewArray( uint8_t, file->ffmpeg_buffer_size = 32*1024 );
 
		// Allocate the AVIOContext:
		// The fourth parameter (pStream) is a user parameter which will be passed to our callback functions
		//lprintf( WIDE("new avio alloc context...") );
		file->ffmpeg_io = ffmpeg.avio_alloc_context(file->ffmpeg_buffer
		                                           , file->ffmpeg_buffer_size  // internal Buffer and its size
		                                           , 0                  // bWriteable (1=true,0=false)
		                                           , file          // user data ; will be passed to our callback functions
		                                           , ffmpeg_read_packet
		                                           , ffmpeg_write_packet                  // Write callback function (not used in this example)
		                                           , ffmpeg_seek
		                                           );
		//lprintf( WIDE("....") );
		// Allocate the AVFormatContext:
		file->pFormatCtx = ffmpeg.avformat_alloc_context();
#ifdef DEBUG_LOG_INFO
		lprintf( WIDE("still good... %p %p"), file, file->pFormatCtx );
#endif
		// Set the IOContext; attaches here for the IO table...
		file->pFormatCtx->pb = file->ffmpeg_io;
#if __ANDROID__
#  ifdef WIN32
		{
			int rc;
			// Test format code...
			lprintf( WIDE("Movie RC is %08x"), rc );
			file->ffmpeg_buffer_size = 32768;
		}
#  else
		{
#ifdef UNICODE
			char *tmpname = CStrDup( filename );
#else
			const char *tmpname = filename;
#endif
			lprintf( "Movie to connect %s", tmpname );
			if( 0 ) //!KWconnectMovie( tmpname ) )
			{
				lprintf( WIDE("Movie %s is %p"), filename, file->ffmpeg_buffer );
				do
				{
					lprintf( WIDE("got buf and size %p %d"), file->ffmpeg_buffer, file->ffmpeg_buffer_size );
					if( !file->ffmpeg_buffer )
					{
						lprintf( WIDE("waiting...") );
						WakeableSleep(1000);
					}
				}while( !file->ffmpeg_buffer );

				{
					FILE *fileout = fopen( "movie.mp4", "wb" );
					if( fileout )
					{
						fwrite( file->ffmpeg_buffer, 1, file->ffmpeg_buffer_size, fileout );
						fclose( fileout );
					}
					else
						lprintf( WIDE("Failed to create file to out") );
				}
			}
#ifdef UNICODE
			Release( tmpname );
#endif
		}
#  endif
#else
#  ifdef WIN32
		file->file_size = 0;
		file->file_memory = (uint8_t*)OpenSpace( NULL, filename, &file->file_size );
		if( !file->file_memory ) {
			file->file_device = sack_fopen( 0, filename, WIDE( "rb" ) );
			file->ffmpeg_buffer_size = 32000;
		}
#  endif
#endif
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
		lprintf( WIDE("Open vaulted movie...") );
		lprintf( WIDE("buffer is %p ... vault is %d"), file->ffmpeg_buffer, file->file_device );
#endif
		if( file->ffmpeg_buffer || file->file_device || file->file_memory )
		{
			AVProbeData probeData;
			// Determining the input format:
			size_t ulReadBytes = 0;
			//memcpy( file->
#ifdef WIN32
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
			lprintf( WIDE("buffer is %d"), file->ffmpeg_buffer_size );
#endif
			file->ffmpeg_buffer = NewArray( uint8_t, file->ffmpeg_buffer_size );
			//file->ffmpeg_buffer_size = 0;
			if( file->file_memory ) 
				memcpy( file->ffmpeg_buffer
					, file->file_memory + file->file_position_index
					, ulReadBytes = file->ffmpeg_buffer_size );
			else
				ulReadBytes = sack_fread(file->ffmpeg_buffer, 1, file->ffmpeg_buffer_size, file->file_device );
			//ulReadBytes = file->ffmpeg_buffer_size;
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
			lprintf( WIDE("did a read %d"), ulReadBytes );
#endif
			// Error Handling...
			if( file->file_device )
				sack_fseek( file->file_device, 0, 0 );
#endif
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
			lprintf( WIDE("probe size is %d"), file->ffmpeg_buffer_size );
#endif
			// Now we set the ProbeData-structure for av_probe_input_format:
			probeData.buf = file->ffmpeg_buffer;
			probeData.buf_size = file->ffmpeg_buffer_size;//ulReadBytes;
			probeData.filename = "";
#ifdef DEBUG_LOG_INFO
			LogBinary( probeData.buf, 256 );
#endif
			// Determine the input-format:
			file->pFormatCtx->iformat = 0;
			//file->pFormatCtx->iformat = ffmpeg.av_probe_input_format(&probeData, 1);
#ifdef DEBUG_LOG_INFO
			lprintf( WIDE("format is %d"), file->pFormatCtx->iformat );
#endif
			file->pFormatCtx->flags = AVFMT_FLAG_CUSTOM_IO;
		}
		else
		{
			//lprintf( WIDE("fail open, release file") );
			Release( file );
			return NULL;
		}
#ifdef DEBUG_LOG_INFO
		lprintf( WIDE("do the open..") );
#endif
		err = ffmpeg.avformat_open_input( &file->pFormatCtx, "", NULL, &options );
	}
	else
	{
#ifdef UNICODE
		char *tmpname = CStrDup( filename );
#else
		const char *tmpname = filename;
#endif
		err = ffmpeg.avformat_open_input( &file->pFormatCtx, tmpname, NULL, &options );
#ifdef UNICODE
		Release( tmpname );
#endif
	}

	if( err )
	{
		char buf[256];
		ffmpeg.av_strerror( err, buf, 256 );
		err = -err;
		lprintf( WIDE("Failed to open file. %08x:%s %4.4s"), err, buf, &err );
		Deallocate( struct ffmpeg_file *, file );
		return NULL;
	}
	//av_dict_free(&options);5

	if( ffmpeg.avformat_find_stream_info(file->pFormatCtx, NULL )<0)
	{
		ffmpeg.avformat_close_input( &file->pFormatCtx );
		lprintf( WIDE("getting stream info failed; release file") );
		Deallocate( struct ffmpeg_file *, file );
		return NULL; // Couldn't find stream information
	}

	{
		int i;

		// Find the first video stream
		file->videoStream=-1;
		file->audioStream=-1;
		for(i = 0; i < file->pFormatCtx->nb_streams; i++)
		{
			if(file->pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) 
			{
				if( file->videoStream > -1 )
					lprintf( WIDE("Found a second video stream original:%d  this:%d"), file->videoStream, i );
				else
					file->videoStream = i;
			}
			if(file->pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) 
			{
				if( file->audioStream > -1 )
					lprintf( WIDE("Found a second audio stream original:%d  this:%d"), file->audioStream, i );
				else
					file->audioStream = i;
			}

		}


		if( file->videoStream == -1 && file->audioStream == -1 )
		{
			lprintf( WIDE("Failed to find an audio or video stream...") );
			ffmpeg.avformat_close_input( &file->pFormatCtx );
			Deallocate( struct ffmpeg_file *, file );
			return NULL;
		}

		if( file->videoStream >= 0 )
		{
			// Get a pointer to the codec context for the video stream
			file->pVideoCodecCtx = file->pFormatCtx->streams[file->videoStream]->codec;
			if( file->pVideoCodecCtx->ticks_per_frame == 1 )
				file->frame_del = ( 1000* 1000LL ) * 1000LL *  file->pVideoCodecCtx->ticks_per_frame * (uint64_t)file->pVideoCodecCtx->time_base.num / (uint64_t)file->pVideoCodecCtx->time_base.den;
			else
				file->frame_del = (1000LL) * 1000LL * file->pVideoCodecCtx->ticks_per_frame * (uint64_t)file->pVideoCodecCtx->time_base.num / (uint64_t)file->pVideoCodecCtx->time_base.den;

			//lprintf( WIDE("scale %d "), ( 1000 * file->pVideoCodecCtx->time_base.num ) / file->pVideoCodecCtx->time_base.den );
			// Find the decoder for the video stream
			file->pVideoCodec = ffmpeg.avcodec_find_decoder(file->pVideoCodecCtx->codec_id);
			if( file->pVideoCodec == NULL ) 
			{
				lprintf(WIDE("Unsupported codec!"));
				Deallocate( struct ffmpeg_file *, file );
				return FALSE;
			}
			// Open codec
			if( ffmpeg.avcodec_open2(file->pVideoCodecCtx, file->pVideoCodec, NULL ) < 0 )
			{
				Deallocate( struct ffmpeg_file *, file );
				return FALSE;
			}
#ifdef DEBUG_LOG_INFO
			lprintf( WIDE("video is %dx%d"), file->pVideoCodecCtx->width, file->pVideoCodecCtx->height );
#endif
			if( getDisplay )
				file->output = getDisplay( psvGetDisplay, file->pVideoCodecCtx->width, file->pVideoCodecCtx->height );
			if( !file->output )
				file->output_control = output_control;
			// Dump information about file onto standard error
			// ffmpeg.dump_format( file->pFormatCtx, 0, filename, 0);
			file->pVideoFrame = ffmpeg.av_frame_alloc();
			if( file->output )
			{
				file->output_width = file->pVideoCodecCtx->width;
				file->output_height = file->pVideoCodecCtx->height;
				file->img_convert_ctx = ffmpeg.sws_getContext(file->pVideoCodecCtx->width, file->pVideoCodecCtx->height
																			, file->pVideoCodecCtx->pix_fmt
																			, file->pVideoCodecCtx->width, file->pVideoCodecCtx->height
																			, PIX_FMT, SWS_BICUBIC, NULL, NULL, NULL);
				file->video_frame_num_bytes = ffmpeg.avpicture_get_size(PIX_FMT, file->output_width, file->output_height);
			}
			else if( file->output_control )
			{
				ffmpeg_UpdateControlLayout( file );
			}
#ifdef DEBUG_LOG_INFO
			lprintf( "Initialize stream - first log; set need_video_frame." );
#endif
			file->flags.need_video_frame = 1;
			//file->videoFrame = file->pVideoCodecCtx->delay;
		}
		if( file->audioStream >= 0 )
		{
			// Get a pointer to the codec context for the video stream
			file->pAudioCodecCtx = file->pFormatCtx->streams[file->audioStream]->codec;

			// Find the decoder for the video stream
			file->pAudioCodec = ffmpeg.avcodec_find_decoder(file->pAudioCodecCtx->codec_id);
			if( file->pAudioCodec == NULL ) 
			{
				lprintf( WIDE("Unsupported codec!") );
				Deallocate( struct ffmpeg_file *, file );
				return FALSE;
			}
			// Open codec
			if( ffmpeg.avcodec_open2(file->pAudioCodecCtx, file->pAudioCodec, NULL ) < 0 )
			{
				Deallocate( struct ffmpeg_file *, file );
				return FALSE;
			}

			file->pAudioFrame = ffmpeg.av_frame_alloc();

			EnableAudioOutput( file );
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
			lprintf( WIDE("Mark we need audio frame...") );
#endif
			file->flags.need_audio_frame = 1;
		}

		{
			// how long is this thing?
			// formatctx->duration
		}
		{
			{
				/* calculated for 1/ to get frame delta */
#define r2d(a) (((a).den)/((a).num))

				AVStream *ic = file->pFormatCtx->streams[file->videoStream];
				file->frame_del = 1000000LL * ic->r_frame_rate.den / ic->r_frame_rate.num;

//#if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(52, 111, 0)
				if( file->frame_del <= 0 )
				{
					file->frame_del = 1000 * r2d( ic->avg_frame_rate );
				}
//#endif

				if( file->frame_del < 0.0001 )
				{
					file->frame_del = 1000.0 / r2d( ic->codec->time_base );
				}

				//return fps;
			}

			//lprintf( WIDE("testing for fixups for PTS") );
			if( file->pFormatCtx->duration_estimation_method == AVFMT_DURATION_FROM_PTS )
			{
				//lprintf( WIDE("This implies that pkt_pts should be correct...") );
			}
			if( strcmp( file->pFormatCtx->iformat->long_name, "FLV (Flash Video)" ) == 0 
				|| strcmp( file->pFormatCtx->iformat->long_name, "ASF (Advanced / Active Streaming Format)" ) == 0 
				)
			{
				file->flags.force_pkt_pts_in_ms = 1;
			}
			else if( strcmp( file->pFormatCtx->iformat->long_name, "Matroska / WebM" ) == 0 )
			{
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
				lprintf( WIDE("is mkv.. %d %d"), file->pFormatCtx->duration_estimation_method, AVFMT_DURATION_FROM_PTS );
#endif
				if( file->pFormatCtx->duration_estimation_method == AVFMT_DURATION_FROM_PTS )
				{
					if( file->pVideoCodec && file->pVideoCodec->id == CODEC_ID_H264 )
					{
						if( file->pVideoCodecCtx->time_base.num == 1 && file->pVideoCodecCtx->time_base.den > 50000 )
						{
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
							lprintf( WIDE("known issue (battle beyond stars) forcing pkt_pts (in Milliseconds!)") );
#endif
							file->flags.force_pkt_pts_in_ms = 1;
						}
					}
				}
				else //if( file->pFormatCtx->duration_estimation_method == 0 )
				{
					if( file->pVideoCodecCtx->time_base.num == 1 && file->pVideoCodecCtx->time_base.den > 50000 )
					{
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
						lprintf( WIDE("Excessive divisor...") );
#endif
						file->flags.force_pkt_pts_in_ms = 1;
					}
				}
				//lprintf( WIDE("mkv is just always in ms?") );

			}
			else if( strcmp( file->pFormatCtx->iformat->long_name, "QuickTime / MOV" ) == 0 )
			{
				if( file->pFormatCtx->duration_estimation_method == AVFMT_DURATION_FROM_PTS )
				{
					if( file->pVideoCodec && file->pVideoCodec->id == CODEC_ID_H264 )
					{
						if( file->pVideoCodecCtx->time_base.num == 1 && file->pVideoCodecCtx->time_base.den > 5000 )
						{
							lprintf( WIDE("known issue (wolf of wallstreet, secret life of nikola tesla confirm) forcing pkt_pts (in Ticks!)") );
							file->flags.force_pkt_pts_in_ticks = 1;
						}
					}
				}
			}
			//lprintf( WIDE("and think we're complete...") );
		}
	}
	return file;
}

//---------------------------------------------------------------------------------------------

static void LogTime( struct ffmpeg_file *file, LOGICAL video, CTEXTSTR leader, int pause_wait DBG_PASS )
{
	//- file->pVideoCodecCtx->delay
	static int64_t prior_video_time;
	int64_t video_time = ( file->videoFrame - 1 ) * file->frame_del;
	int64_t video_time_now = ( file->videoFrame ) * file->frame_del;
	// in MS
	uint64_t audio_time = file->pAudioCodecCtx?(1000 * file->audioSamplesPlayed / file->pAudioCodecCtx->sample_rate):0;
	uint64_t audio_time2 = file->al_last_buffer_reclaim - file->media_start_time;
	uint64_t audio_time_pending = file->pAudioCodecCtx ? (1000 * file->audioSamplesPending / file->pAudioCodecCtx->sample_rate ):0;
	uint64_t clock_time = ffmpeg.av_gettime();
	uint64_t real_time = ( clock_time - file->media_start_time ) / 1000;
	// between now and last audio finish
	int64_t delta_audio = (int64_t)clock_time - (int64_t)file->al_last_buffer_reclaim;

	// the (av) time of 1 video frame
	int64_t video_time_tick = ( 1000LL ) * 1000LL *  file->pVideoCodecCtx->ticks_per_frame * (uint64_t)file->pVideoCodecCtx->time_base.num / (uint64_t)file->pVideoCodecCtx->time_base.den;
	// the (av)real time of 1k sound samples
	int64_t audio_time_pending_1000_tick = 1000 * 1000 * 1000 / (file->pAudioCodecCtx ? (file->pAudioCodecCtx->sample_rate):44100);

	// the 1k samples needs to be included this is in single-samples
	int64_t video_time_in_audio_frames;

	if( file->videoFrame > 0 )
		video_time_in_audio_frames	= 1000 * video_time_now / audio_time_pending_1000_tick;
	else
		video_time_in_audio_frames	= 0;

	if( pause_wait )
	{
		//lprintf( WIDE("ticks : %d, num : %")_32fs WIDE(" den: %")_32fs, file->pVideoCodecCtx->ticks_per_frame, file->pVideoCodecCtx->time_base.num, file->pVideoCodecCtx->time_base.den );


		lprintf( WIDE("video in frames = %")_64fs WIDE(" played = %")_64fs WIDE("  video_tick = %")_64fs WIDE("  audio 1k tick= %")_64fs
				 , video_time_in_audio_frames
				 , file->audioSamplesPlayed
				 , video_time_tick, audio_time_pending_1000_tick );

		//lprintf( WIDE("audio time : %32") _64fs WIDE(" %")_64fs, audio_time, file->audioSamplesPlayed );
		//lprintf( WIDE("audio time2: %32") _64fs, audio_time2 );
		//lprintf( WIDE("video time : %32") _64fs WIDE(" %")_64fs, video_time, file->videoFrame );
		//lprintf( WIDE("Video time2: %32") _64fs, file->video_current_pts_time - file->media_start_time  );
		//lprintf( WIDE("real  time : %32") _64fs, real_time );
	}

	if( file->videoFrame > 0 )
	{
		// Post .5 second updates to position feedback controls
		if( video_time > (( file->last_tick_update + 500 ) ) )
		{
			uint64_t tick = ( video_time * 1000000000000 )
									/ file->pFormatCtx->duration ;
			file->last_tick_update = video_time;
			if( file->video_position_update ) 
				file->video_position_update( file->psvUpdateParam, tick + file->seek_position );
		}
	}

	//if( file->videoFrame > 100 )
	{
#if 0
		//if( real_time < ( audio_time + delta_audio ) || real_time )
		if( file->audioSamplesPlayed  )
		{
			//lprintf( WIDE("adjusting real time from %") _64fs, real_time - ( audio_time + delta_audio / 1000 ) );
			real_time = audio_time + delta_audio / 1000;
		}
		else
			real_time = video_time;  // time of the last displayed frame...
#endif

		if( video 
			&& ( file->videoFrame > 10 )
			)
		{
			// track video to itself...

			//file->video_current_pts_time = file->media_start_time + video_time * 1000;;
			if( file->pAudioCodecCtx ) {
#ifdef DEBUG_WIN8_REDRAW
				lprintf( "next tick is built from %" _64fs "  %"  _64fs  " %" _64fs, file->media_start_time, video_time_now, ((video_time_in_audio_frames - file->audioSamplesPlayed) * 1000LL) / file->pAudioCodecCtx->sample_rate );
#endif
				file->video_next_pts_time = file->media_start_time + (video_time_now)
					+((video_time_in_audio_frames - file->audioSamplesPlayed) * 1000LL) / file->pAudioCodecCtx->sample_rate
					+ (video_time_tick * file->video_adjust_ticks * 1000)
					;
			} 
			else {
				int64_t tmp = clock_time - file->media_start_time;
					//		+(video_time_tick * file->video_adjust_ticks)) - file->video_next_pts_time;
#ifdef DEBUG_WIN8_REDRAW
				lprintf( "next tick is built from %" _64fs "  %"  _64fs  " %" _64fs, file->media_start_time, video_time_now, video_time_tick * 1000 );
#endif
				file->video_next_pts_time = file->video_decode_start
					+ ( video_time_now ) + file->frame_del
					//+( video_time_tick * 1000 )
					+ ( video_time_tick * file->video_adjust_ticks * 1000 )
					;
					//;
			}
			// delta in ms....
			 //lprintf( WIDE("something %")_64fs , (  ( video_time_in_audio_frames - file->audioSamplesPlayed ) * 1000LL ) / file->pAudioCodecCtx->sample_rate );
			 //lprintf( WIDE("something %")_64fs , file->video_next_pts_time - file->media_start_time );
			 //lprintf( WIDE("something %")_64fs , file->video_next_pts_time - clock_time );

			//lprintf( WIDE("Delta in samples: %")_64fs, video_time_in_audio_frames - file->audioSamplesPlayed );
			//lprintf( WIDE("Time1 = %")_64fs, file->media_start_time + video_time_now * 1000 - file->media_start_time);
			//lprintf( WIDE("Time2 = %")_64fs, file->video_current_pts_time );
			//lprintf( WIDE("Time2 = %")_64fs, file->media_start_time );
			//lprintf( WIDE("Time2 = %")_64fs, file->al_last_buffer_reclaim );
			//lprintf( WIDE("Time2 = %")_64fs, file->video_current_pts_time - file->media_start_time );
		}
	}
	//if( pause_wait )
	//	_lprintf(DBG_RELAY)( WIDE("%s %s"), leader, file->szTime );
	tnprintf( file->szTime, 256, WIDE("%s%s%s %zd %zd %zd %02")_64fs WIDE(":%02")_64fs WIDE(":%03")_64fs WIDE("A(%02")_64fs WIDE(":%02")_64fs WIDE(".%03")_64fs WIDE(") (%02")_64fs WIDE(".%03")_64fs WIDE(") V: %")_64fs WIDE(" (%02")_64fs WIDE(":%02")_64fs WIDE(".%03")_64fs WIDE(") (%02")_64fs WIDE(":%02")_64fs WIDE(".%03")_64fs WIDE(")")
		, file->flags.need_audio_frame?WIDE("(A)"):WIDE("")
		, file->flags.need_video_frame?WIDE("(V)"):WIDE("")
		, file->flags.reading_frames?WIDE("(R)"):WIDE("")
		, GetQueueLength( file->al_free_buffer_queue )//free_packets
		, GetQueueLength( file->al_used_buffer_queue )//video_packets
		, GetQueueLength( file->pAudioPackets )
		, ( real_time / ( 1000 * 60 ) ) % 60
		, ( real_time % ( 1000 * 60 ) ) / 1000
		, ( real_time % ( 1000 ) )
		, ( audio_time / ( 1000 * 60 ) ) % 60
		, ( audio_time % ( 1000 * 60 ) ) / 1000
		, ( audio_time % ( 1000 ) )
		, ( audio_time_pending % ( 1000 * 60 ) ) / 1000
		, ( audio_time_pending % ( 1000 ) )
		, file->videoFrame
		, (( video_time ) / ( 1000 * 60 ) ) % 60
		, (( video_time ) % ( 1000 * 60 ) ) / 1000
		, ( (video_time ) % ( 1000 ) )
		, ( ( clock_time - prior_video_time ) / ( 1000*1000 * 60 ) ) % 60
		, ( (( clock_time - prior_video_time ) /1000 ) % (1000 * 60 ) ) / 1000
			  , ( ( ( clock_time - prior_video_time ) / 1000 )% ( 1000 ) )
			  );
	prior_video_time = clock_time;
	if( pause_wait )
		_lprintf(DBG_RELAY)( WIDE("%s %s"), leader, file->szTime );
}

//---------------------------------------------------------------------------------------------



//---------------------------------------------------------------------------------------------


static uintptr_t CPROC ProcessAudioFrame( PTHREAD thread )
{
	struct ffmpeg_file * file = (struct ffmpeg_file *)GetThreadParam( thread );
	AVPacket *packet;
	int frameFinished;
#ifdef DEBUG_LOG_INFO
	lprintf(WIDE(" This thread is playing audio for %s"), file->filename );
#endif
	file->media_start_time = ffmpeg.av_gettime();
	while( !file->flags.close_processing )
	{
		// reclaim processed buffers...
		int processed = 0;
#ifdef USE_OPENSL
		int bufID = 0;
#else
		ALuint bufID = 0;
#endif
		
		EnterCriticalSec( &l.cs_audio_out );
#ifdef USE_OPENSL
		{
			SLresult result;
			if( file->flags.paused )
			{
				int stopped = 0;
				SLuint32 playState;
				do
				{
					result = (*file->bqPlayerPlay)->GetPlayState(file->bqPlayerPlay, &playState);
					if( playState == SL_PLAYSTATE_PLAYING && !stopped )
					{
						stopped = 1;
						lprintf( WIDE("Stop playback and clear buffers...") );
						result = (*file->bqPlayerPlay)->SetPlayState(file->bqPlayerPlay,
																					SL_PLAYSTATE_STOPPED );
						result = (*file->bqPlayerBufferQueue)->Clear(file->bqPlayerBufferQueue );
						{
							struct al_buffer *buffer;
							while( buffer = (struct al_buffer*)DequeLink( &file->al_used_buffer_queue ) )
							{
								// out of queue count remains unchanged because requeue should
								// put them back onto the card.
								EnqueLink( &file->al_reque_buffer_queue, buffer );
							}
						}

					}
					Relinquish();
				}
				while( playState == SL_PLAYSTATE_PLAYING );
			}
		}
#endif

#ifndef USE_OPENSL
		if( file->sound_device )
		{
			openal.alcMakeContextCurrent(file->sound_device->alc_context);

			//lprintf( WIDE("make current...") );
			if( file->flags.paused )
			{
				int stopped = 0;
				int state;
				do
				{
					openal.alGetSourcei(file->sound_device->al_source, AL_SOURCE_STATE, &state);
					if(state==AL_PLAYING && !stopped)
					{
						stopped = 1;
						openal.alSourceStop(file->sound_device->al_source);
					}
					Relinquish();
				}
				while( state==AL_PLAYING );
			}

			do
			{
				openal.alGetSourcei(file->sound_device->al_source, AL_BUFFERS_PROCESSED, &processed);
				if( processed )
				{
	#ifdef DEBUG_AUDIO_PACKET_READ
					//if( file->flags.paused || file->flags.need_audio_dequeue )
					lprintf( WIDE("Found %d processed buffers... %d  %d"), processed
							 , GetQueueLength( file->al_free_buffer_queue )
							 , GetQueueLength( file->al_used_buffer_queue ) );
	#endif
					while (processed--) {
						struct al_buffer *buffer;
						buffer = (struct al_buffer *)DequeLink( &file->al_used_buffer_queue );
						openal.alSourceUnqueueBuffers(file->sound_device->al_source, 1, &bufID);
						if( !file->flags.paused )
						{
							file->audioSamplesPlayed += buffer->samples;
							file->audioSamplesPending -= buffer->samples;
	#ifdef DEBUG_AUDIO_PACKET_READ
							lprintf( WIDE("Samples : %d  %d %d"), buffer->samples, file->audioSamplesPlayed, file->audioSamplesPending );
	#endif
							if( buffer->buffer != bufID )
							{
								lprintf( WIDE("audio queue out of sync") );
								//DebugBreak();
							}
	#ifdef DEBUG_AUDIO_PACKET_READ
							lprintf( WIDE("release audio buffer.. %p"), buffer->samplebuf );
	#endif
							ffmpeg.av_free( buffer->samplebuf );
							//LogTime(file, FALSE, WIDE("audio deque"), 1 DBG_SRC );
							EnqueLink( &file->al_free_buffer_queue, buffer );
							if( GetQueueLength( file->al_free_buffer_queue ) > file->max_in_queue )
								file->max_in_queue = GetQueueLength( file->al_free_buffer_queue );
						}
						else
						{
							//lprintf( WIDE("Keeping the buffer to requeue...") );
							EnqueLink( &file->al_reque_buffer_queue, buffer );
						}
					}
					//lprintf( WIDE("save reclaim tick...") );
					file->al_last_buffer_reclaim = ffmpeg.av_gettime();
					// audio got a new play; after a resume, so now wake video thread
					if( file->flags.need_audio_dequeue )
					{
						//lprintf( WIDE("Valid resume state...") );
						file->flags.need_audio_dequeue = 0;
						WakeThread( file->videoThread );
					}
				}
				else
				{
					//if( file->flags.paused || file->flags.need_audio_dequeue )
					//lprintf( WIDE("Found %d processed buffers... %d  %d"), 0, file->in_queue, GetQueueLength( file->al_used_buffer_queue ) );
					Relinquish();
				}
			} while( ( file->flags.paused && GetQueueLength( file->al_used_buffer_queue ) ) );
	#endif
		}

		while( file->flags.paused )
		{
			dropSoundDevice( &file->sound_device );
			file->flags.audio_paused = 1;
			file->flags.audio_sleeping = 1;
			LeaveCriticalSec( &l.cs_audio_out );
			WakeableSleep( SLEEP_FOREVER );
			if( file->flags.close_processing )
				break;
			EnterCriticalSec( &l.cs_audio_out );
			if( !file->flags.paused )
			{
				file->sound_device = getSoundDevice( file );
				file->flags.audio_paused = 0;
				file->flags.audio_sleeping = 0;
				file->flags.need_audio_dequeue = 1;
				RequeueAudio( file );
			}
		}
		if( file->flags.close_processing )
			break;

		//lprintf( WIDE("get all packets...") );
		if( !file->flags.need_audio_dequeue || ( file->flags.need_audio_dequeue && !GetQueueLength( file->al_used_buffer_queue ) ) )
			while( packet = (AVPacket*)DequeLink( &file->pAudioPackets ) )
			{
				//lprintf( WIDE("audio stream packet. %d %d"), GetQueueLength( file->pAudioPackets ), GetQueueLength( file->packet_queue ) );
				ffmpeg.avcodec_decode_audio4(file->pAudioCodecCtx, file->pAudioFrame, &frameFinished, packet);
				// Did we get a video frame?
				if( frameFinished) {
					int dst_linesize;
					//LogTime(file, FALSE, WIDE("audio add") DBG_SRC );
					//lprintf( WIDE("audio stream packet.") );
					if( !file->audio_out )
					{
						// allocates the audio_out plane buffer element
						ffmpeg.av_samples_alloc_array_and_samples(&file->audio_out
									, &dst_linesize, file->use_channels
									, file->pAudioFrame->nb_samples, AV_SAMPLE_FMT_S16P, 1);
						file->max_out_samples = file->pAudioFrame->nb_samples;
					}
					else
					{
						ffmpeg.av_samples_alloc(file->audio_out, &dst_linesize, file->use_channels,
													file->pAudioFrame->nb_samples, AV_SAMPLE_FMT_S16P, 1);
					}
					{
						int dst_bufsize = ffmpeg.av_samples_get_buffer_size(&dst_linesize, file->use_channels,
													 file->pAudioFrame->nb_samples, AV_SAMPLE_FMT_S16P, 1);
						ffmpeg.swr_convert( file->audio_converter, file->audio_out, file->max_out_samples,(const uint8_t**)file->pAudioFrame->data, file->pAudioFrame->nb_samples ); 

						file->audioSamplesPending += file->pAudioFrame->nb_samples;
						file->audioFrame++;
						// add the real time...
						GetAudioBuffer( file, file->audio_out[0], dst_bufsize, file->pAudioFrame->nb_samples );
						file->audio_out[0] = NULL;
					}
				}
				//lprintf( WIDE("Free packet.") );
#ifdef DEBUG_AUDIO_PACKET_READ
				//lprintf( "Call free packet.." );
#endif
				ffmpeg.av_free_packet(packet);
				EnqueLink( &file->packet_queue, packet );
				Relinquish();
			}
		LeaveCriticalSec( &l.cs_audio_out );

		// need to do this always not just if packet; will always be a packet...
		{
			//lprintf( WIDE("(moved all fom in(file/avail) to out(openal) in : %d  out: %d   max : %d"), GetQueueLength( file->al_free_buffer_queue ), GetQueueLength( file->al_used_buffer_queue ), file->max_in_queue );
			if( GetQueueLength( file->al_used_buffer_queue ) < (file->max_in_queue / 4) )
			{
				// need more packets...
				file->flags.need_audio_frame = 1;
				if( !file->flags.reading_frames )
				{
					//lprintf( WIDE("read thread isn't reading...so wake it up") );
					WakeThread( file->readThread );
				}
					// nothing pending to reclaim, and not resuming a pause/seek...
				if( !file->flags.need_audio_dequeue && !GetQueueLength( file->al_used_buffer_queue ) )
				{
					dropSoundDevice( &file->sound_device );
					//lprintf( WIDE("Long sleep") );
					file->flags.audio_paused = 1;
					file->flags.audio_sleeping = 1;
					WakeableSleep( SLEEP_FOREVER );
					file->flags.audio_paused = 0;
				}
				else
				{
					//lprintf( WIDE("need a dequeue...") );
					Relinquish();
				}
			}
			else
			{
				// already have enough playing... don't technically need any
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
				lprintf( WIDE("Clear need audio frame") );
#endif
				file->flags.need_audio_frame = 0;
				//file->audioSamples / file->pAudioCodecCtx->sample_rate
				if( !packet )
				{
					// no packets either, so wait for a smple to play to recollect the buffer
					// might still have no buffers...
					struct al_buffer * buffer = (struct al_buffer *)PeekQueue( file->al_used_buffer_queue );
					if( buffer )
					{
						//lprintf( WIDE("wait %d"), 1000 * buffer->samples / file->pAudioCodecCtx->sample_rate );
						WakeableSleep( 1000 * buffer->samples / file->pAudioCodecCtx->sample_rate );
						//lprintf( WIDE("awake...") );
					}
				}
			}
		}
		if( 0 )
		lprintf( WIDE("audio holding at %lld  %d %d  %s%s%s")
			, file->audioFrame
			, GetQueueLength( file->al_used_buffer_queue ), GetQueueLength( file->al_free_buffer_queue )
			, file->flags.need_audio_frame?WIDE("audio"):WIDE("")
			, file->flags.need_video_frame?WIDE(" video"):WIDE("")
			, file->flags.reading_frames?WIDE(" reading"):WIDE("")
			);
	}
	lprintf(WIDE(" This thread is no longer playing audio for %p %s"), file, file->filename );
	file->audioThread = NULL;
	return 0;
}

//---------------------------------------------------------------------------------------------

static void ClearPendingVideo( struct ffmpeg_file *file )
{
	AVPacket *packet;
	if( file->pVideoCodecCtx )
	{
		int frameFinished;
		AVPacket flush_packet;
		MemSet( &flush_packet, 0, sizeof( AVPacket ) );
		do
		{
			ffmpeg.avcodec_decode_video2( file->pVideoCodecCtx, file->pVideoFrame, &frameFinished, &flush_packet );
		}
		while( frameFinished );
	}
	while( packet = (AVPacket*)DequeLink( &file->pVideoPackets ) )
	{
#ifdef DEBUG_VIDEO_PACKET_READ
		lprintf( WIDE("Call free packet..") );
#endif
		ffmpeg.av_free_packet(packet);
		EnqueLink( &file->packet_queue, packet );
	}
	// the last decoded frame is still scheduled... so make sure we drop that one too.
	file->flags.clear_pending_video = 1;
}

//---------------------------------------------------------------------------------------------

static uintptr_t CPROC internal_stop_thread( PTHREAD thread )
{
	ffmpeg_StopFile((struct ffmpeg_file *)GetThreadParam( thread  ) );
	return 0;
}

//---------------------------------------------------------------------------------------------

static void stop( struct ffmpeg_file *file )
{
	if( !file->stopThread )
	{
		file->stopThread = ThreadTo( internal_stop_thread, (uintptr_t)file );
		if( file->external_video_failure )
			file->external_video_failure( WIDE("Video Decode\n Too Slow\nOn This Device\n :(") );
	}

}

//---------------------------------------------------------------------------------------------

static void OutputFrame( struct ffmpeg_file * file, struct ffmpeg_video_frame *frame ) {

					Image out_surface;
					if( file->output )
						out_surface = GetDisplayImage( file->output );
					else
						out_surface = ffmpeg.GetControlSurface( file->output_control );
					{
						CDATA *surface = GetImageSurface( out_surface );

						if( file->output_control )
						{
							if( file->pre_video_render )
							{
								file->pre_video_render( file->output_control );
								BlotImageEx( out_surface, frame->output_control_image, 0, 0, ALPHA_TRANSPARENT, BLOT_COPY );
							}
							else
								BlotImageEx( out_surface, frame->output_control_image, 0, 0, 0, BLOT_COPY );
						}
						else if( surface )
						{
#ifdef _INVERT_IMAGE
							uint32_t row;
							for( row = 0; row < file->output_height; row++ )
							{
								memcpy( surface + ( out_surface->y + file->output_height - row - 1 ) * out_surface->pwidth//file->output_width
										, frame->rgb_buffer + row * sizeof(CDATA) * file->output_width, file->output_width * sizeof( CDATA ) );
							}
#else
							memcpy( surface
									, frame->rgb_buffer , file->output_height * file->output_width * sizeof( CDATA ) );
#endif
						}
#ifdef DEBUG_VIDEO_PACKET_READ
						lprintf( WIDE("put string on frame...") );
#endif
						//PutString( out_surface, 0, 0, BASE_COLOR_WHITE, AColor( 0,0,1,64 ), file->szTime );
#ifdef DEBUG_VIDEO_PACKET_READ
						lprintf( WIDE("Update Display.") );
#endif

						file->video_post_tick = timeGetTime();

						if( file->output )
							Redraw( file->output );
						else if( file->output_control )
						{
							if( file->post_video_render )
								file->post_video_render( file->output_control );
							ffmpeg.UpdateFrameEx( file->output_control, 0, 0, file->output_width, file->output_height DBG_SRC );
						}
					}
}

//---------------------------------------------------------------------------------------------

static uintptr_t CPROC UpdateOutputFrames( PTHREAD thread )
{
	struct ffmpeg_file * file = (struct ffmpeg_file *)GetThreadParam( thread );
	while( !file->flags.close_processing )
	{
		struct ffmpeg_video_frame *frame;
		if( file->flags.paused )
		{
			file->output_waiting_pause = TRUE;
			WakeableSleep( SLEEP_FOREVER );
			file->output_waiting_pause = FALSE;
			continue;
		}

		if( GetQueueLength( file->pDecodedVideoFrames ) < ((file->video_high_water + file->video_low_water) / 2) ) {
			if( file->flags.video_decoder_sleeping_on_output 
				|| !file->flags.using_video_frame ) {
				//lprintf( "display has some room for more packets... so wake decoder " );
				WakeThread( file->videoThread );
			}
		}

		frame = (struct ffmpeg_video_frame *)DequeLink( &file->pDecodedVideoFrames );

		if( frame ) {
			int64_t now = ffmpeg.av_gettime();
			if( now < frame->video_current_pts_time ) {
				//lprintf( "sleep for %" _64fs, (frame->video_current_pts_time - now) / 1000 );
				WakeableSleep( (uint32_t)(( frame->video_current_pts_time - now ) / 1000) );
			}
			OutputFrame( file, frame );
			EnqueLink( &file->pEmptyVideoFrames, frame );
		}
		else {
			file->output_waiting = TRUE;
			WakeableSleep( SLEEP_FOREVER );
			file->output_waiting = FALSE;
		}
	}
	file->videoOutputThread = NULL;
	return 0;
}

//---------------------------------------------------------------------------------------------


static uintptr_t CPROC ProcessVideoFrame( PTHREAD thread )
{
	struct ffmpeg_file * file = (struct ffmpeg_file *)GetThreadParam( thread );
	int frameFinished;
	AVPacket *packet;
	// allow the first tick.
#ifdef DEBUG_LOG_INFO
	lprintf(WIDE(" This thread is playing video for %p %s"), file, file->filename );
#endif
	file->flags.allow_tick = 1;
	file->flags.using_video_frame = 1;
	while( !file->flags.close_processing )
	{
		if( file->flags.paused ) {
			file->flags.using_video_frame = 0;
			file->flags.video_paused = 1;
			WakeableSleep( SLEEP_FOREVER );
			file->flags.using_video_frame = 1;
			file->flags.video_paused = 0;
			continue;
		}
		//lprintf( "video packets to process and output is at %d", GetQueueLength( file->pDecodedVideoFrames ) );
		if( GetQueueLength( file->pDecodedVideoFrames ) > file->video_high_water ) {
			//lprintf( "Sleeping, there's already enough queued in the future" );
			file->flags.video_decoder_sleeping_on_output = 1;
			file->flags.using_video_frame = 0;
			file->flags.video_paused = 1;
			WakeableSleep( SLEEP_FOREVER );
			file->flags.video_decoder_sleeping_on_output = 0;
			file->flags.using_video_frame = 1;
			file->flags.video_paused = 0;
			continue;
		}

		packet = (AVPacket*)DequeLink( &file->pVideoPackets );
		if( !packet )
		{
			//lprintf( "no video packets to process and output is at %d", GetQueueLength( file->pDecodedVideoFrames ) );
			if( !file->flags.paused && 
					GetQueueLength( file->pDecodedVideoFrames ) > file->video_low_water ) {
				//lprintf( "Sleeping, there's already enough queued in the future" );
				file->flags.video_decoder_sleeping_on_output = 1;
				WakeableSleep( SLEEP_FOREVER );
				file->flags.video_decoder_sleeping_on_output = 0;
				continue;
			}

			//lprintf( "failed to have a pcket...%d", file->flags.no_more_packets );
			if( file->flags.no_more_packets )
				if( !file->flags.video.first_flush )
				{
					//lprintf( "first flush, so setup blank packet to get last frame..." );
					file->flags.video.first_flush = 1;
					file->flags.video.flushing = 1;
					packet = &l.blank_packet;
				}
				else
				{
					// allow pause...
					//lprintf( "second flush, leave packet NULL" );
					file->flags.video.flushing = 0;
				}

		}
		else
		{
			file->flags.video.first_flush = 0;
			file->flags.video.flushing = 0;
		}
		if( packet )
		{
			struct ffmpeg_video_frame *frame;
			uint64_t packet_pts = packet->pts;
			// Decode video frame
#ifdef DEBUG_VIDEO_PACKET_READ
			lprintf( WIDE("Video process Packet %p (%d)"), packet, GetQueueLength( file->pVideoPackets) );
#endif

			ffmpeg.avcodec_decode_video2(file->pVideoCodecCtx, file->pVideoFrame, &frameFinished, packet);

			if( !file->flags.video.flushing ) {
				ffmpeg.av_free_packet( packet );
				EnqueLink( &file->packet_queue, packet );
			}
			// Did we get a video frame?
			if( frameFinished) 
			{
				int pause_resume =
#ifdef DEBUG_VIDEO_PACKET_READ
               2
#else
               0
#endif
               ;
				uint64_t processed_time = ffmpeg.av_gettime();
				if( file->flags.video.flushing )
					file->flags.video.first_flush = 0;
#ifdef DEBUG_VIDEO_PACKET_READ
				lprintf( WIDE("for the record we're lookin at %"_64fs" %"_64fs), processed_time - file->video_decode_start, video_time_tick );
#endif
				if( pause_resume )
					pause_resume--;

				file->flags.allow_tick = 1;
#ifdef DEBUG_VIDEO_PACKET_READ
				if( file->flags.paused )
					lprintf( WIDE("Begin with a new frame...") );
#endif
				while( file->flags.paused && !file->flags.video.flushing )
				{
					//Image out_surface = GetDisplayImage( file->output );
					file->flags.using_video_frame = 0;
					file->flags.video_paused = 1;
#ifdef DEBUG_VIDEO_PACKET_READ
					lprintf( WIDE("Begin pause wait (video)") );
#endif
					WakeableSleep( SLEEP_FOREVER );
					if( file->flags.close_processing ) {
						goto video_done;
					}
					//lprintf( WIDE("woke up(video)") );
					if( !file->flags.paused )
					{
						file->flags.using_video_frame = 1;
						file->flags.video_paused = 0;
					}

					pause_resume = 4;
				}
				if( file->flags.clear_pending_video )
				{
#ifdef DEBUG_VIDEO_PACKET_READ
					lprintf( WIDE("Skipping frame that was already held.") );
#endif
					file->flags.clear_pending_video = 0;
					continue;
				}
				SetAudioPlay( file );

				//file->video_frame_time = ffmpeg.av_gettime();
				LogTime(file, TRUE, WIDE("video"), pause_resume DBG_SRC );

				if( pause_resume )
					lprintf(WIDE("Frame [%")_64fs WIDE("][%d](%d): s.pts=%")_64fs WIDE(" p.pts=%")_64fs WIDE(" pts=%")_64fx WIDE(", pkt_pts=%")_64fs WIDE(", pkt_dts=%")_64fs
						 , file->videoFrame
						 , file->pVideoFrame->coded_picture_number
						 , file->pVideoFrame->repeat_pict

					, file->video_current_pts
					, packet_pts
					, file->pVideoFrame->pts
					, file->pVideoFrame->pkt_pts
						 , file->pVideoFrame->pkt_dts);
				if( file->flags.force_pkt_pts_in_ms ) {
					lprintf( "forcing time in milliseconds" );
					file->video_next_pts_time = file->pVideoFrame->pkt_pts * 1000 + file->media_start_time;
				}
				if( file->flags.force_pkt_pts_in_ticks )
				{
					lprintf( "Forcing time from ticks." );
					file->video_next_pts_time = ( 1000000LL * file->pVideoFrame->pkt_pts
														  * file->pVideoCodecCtx->ticks_per_frame * file->pVideoCodecCtx->time_base.num ) / file->pVideoCodecCtx->time_base.den
						+ file->media_start_time;
#ifdef DEBUG_VIDEO_PACKET_READ
					lprintf( WIDE("Setting next time to %")_64fs WIDE(" %")_64fs, file->video_next_pts_time, file->video_next_pts_time - file->media_start_time );
#endif
				}

				if( !file->videoFrame )
				{
					// sync to the video at 0.
					// equate real time to video time....
					//lprintf( WIDE("First frame") );
					//if( file->flags.use_internal_tick )
					//	file->video_current_pts = file->pVideoFrame->pkt_pts;
					file->video_decode_start = file->media_start_time;
					file->video_current_pts_time = file->video_decode_start; // when we started decoding.
					//lprintf( "Set current to %" _64fs "  %" _64fs, file->video_current_pts_time, file->video_current_pts_time - ffmpeg.av_gettime() );
					file->video_next_pts_time = file->video_current_pts_time + file->frame_del;
					//lprintf( "no frame, so set the video tick start at %" _64fs " %" _64fs, file->video_next_pts_time, file->video_decode_start );
				}
				else
				{
					if( pause_resume )
						lprintf( WIDE(" something %")_64fs WIDE("  %")_64fs
								, ( file->pVideoFrame->pkt_pts - file->video_current_pts )
								, file->video_next_pts_time - processed_time );

					{
						if( pause_resume )
							lprintf( WIDE("now is %")_64fs WIDE("  %") _64fs WIDE("  to be is %") _64fs WIDE(" and that's a span of %") _64fs
							       , file->frame_del
							       , file->video_current_pts
							       , file->pVideoFrame->pkt_pts 
									 , file->frame_del - ((ffmpeg.av_gettime() - file->video_current_pts_time)/1000) );

					}

					//lprintf( WIDE( "Next wb %" )_64fs WIDE( "  time is %" )_64fs WIDE( "  and del is %" )_64fs
				//			, file->video_next_pts_time, time
					//		, file->video_next_pts_time - time );
					file->video_current_pts_time = file->video_next_pts_time;
					//lprintf( "Set current to %" _64fs "  %" _64fs, file->video_current_pts_time, file->video_current_pts_time - ffmpeg.av_gettime() );
					file->video_next_pts_time = file->video_current_pts_time + file->frame_del;
				}
				// add the real time...
				file->videoFrame++;
				
#ifdef DEBUG_VIDEO_PACKET_READ
				lprintf( WIDE("Begin output") );
#endif
				frame = GetVideoFrame( file );
				UpdateSwsContext( file, frame );

				//lprintf( "scale output image......" );
				ffmpeg.sws_scale(file->img_convert_ctx
									 , (const uint8_t*const*)file->pVideoFrame->data, file->pVideoFrame->linesize
									 , 0, file->pVideoCodecCtx->height
									 , frame->pVideoFrameRGB->data
									 , frame->pVideoFrameRGB->linesize);
				frame->video_current_pts_time = file->video_current_pts_time + file->video_current_pts_offset;
				//lprintf( "Set FRAME current to %" _64fs "  %" _64fs, frame->video_current_pts_time, frame->video_current_pts_time - ffmpeg.av_gettime() );

				EnqueLink( &file->pDecodedVideoFrames, frame );
				{
					if( !file->videoOutputThread ) {
						file->videoOutputThread = ThreadTo( UpdateOutputFrames, (uintptr_t)file );
					}
					else {
						if( file->output_waiting )
							WakeThread( file->videoOutputThread );
#ifdef DEBUG_WIN8_REDRAW
							lprintf( "post redraw?" );
#endif
					}
				}
			}
#ifdef DEBUG_VIDEO_PACKET_READ
			lprintf( WIDE("Call free packet..") );
#endif
			file->flags.video.flushing = 0;
			if( GetQueueLength( file->pVideoPackets ) < 4 )
			{
				file->flags.need_video_frame = 1;
				if( !file->flags.reading_frames ) {
					//lprintf( "Need some more video packets from reader, am running low; and he's sleeping" );
					WakeThread( file->readThread );
				}
			}
		}
		else
		{
			// need more packets...
			if( file->flags.video.first_flush ) {
				// was flushing the buffers... and no more packets, done.
				break;
			}
			file->flags.need_video_frame = 1;
			//lprintf( WIDE("need a video packet...%p"), file, file->flags );
			if( !file->flags.reading_frames )
				WakeThread( file->readThread );
			file->flags.using_video_frame = 0;
			// might as well be paused here; I didn't have packets...
			file->flags.video_paused = 1;
			//lprintf( "out of packets... wait for more" );
			WakeableSleep( SLEEP_FOREVER );
			//lprintf( "Should have some packets now?" );
			file->flags.video_paused = 0;
			file->flags.using_video_frame = 1;
			//lprintf( WIDE("needed a video packet...and woke up") );
		}
	}
	video_done:
#ifdef DEBUG_LOG_INFO
	lprintf(WIDE(" This thread is no longer playing video for %p %s"), file, file->filename );
#endif
	file->flags.using_video_frame = 0;
	file->flags.need_video_frame = 0;
	file->flags.video_paused = 1;
	file->videoThread = NULL;
	return 0;
}

//---------------------------------------------------------------------------------------------


static uintptr_t CPROC ProcessFrames( PTHREAD thread )
{
	struct ffmpeg_file * file = (struct ffmpeg_file *)GetThreadParam( thread );
	AVPacket *packet;
	int pending = 0;
	file->packet_queue = CreateLinkQueue();
#ifdef DEBUG_LOG_INFO
	lprintf(WIDE(" This thread is playing %p %s"), file, file->filename );
#endif
	if( !file->pFormatCtx->duration )
		file->pFormatCtx->duration = 1000000000;
	while( !file->flags.close_processing )
	{
		while( file->flags.no_more_packets || file->flags.paused )
		{
			file->flags.reading_frames = 0;
			WakeableSleep( SLEEP_FOREVER );
			if( file->flags.close_processing ) {
				ffmpeg.av_free_packet( packet );
				EnqueLink( &file->packet_queue, packet );
				packet = NULL;
				break;
			}
		}

		if( !file->flags.no_more_packets )
		{
			packet = (AVPacket*)DequeLink( &file->packet_queue );
			if( !packet )
				packet = New( AVPacket );

			while( !file->flags.need_audio_frame && !file->flags.need_video_frame )
			{
				file->flags.reading_frames = 0;
				//lprintf( WIDE("stop reading...") );
				if( file->flags.paused ) 
					WakeableSleep( SLEEP_FOREVER );
				else {
					//lprintf( "sleep some for threads to use packets" );
					WakeableSleep( 250 );
					//lprintf( "wake up for more packets" );
					//if( (!file->flags.need_audio_frame && !file->flags.need_video_frame) )
					//   lprintf( "Stalled waiting for someone to need something... " );
				}
				if( file->flags.close_processing ){
					if( packet ) {
						ffmpeg.av_free_packet( packet );
						EnqueLink( &file->packet_queue, packet );
						packet = NULL;
					}
					break;
				}
				//lprintf( WIDE("woke up to read more...for %s %s"), file->flags.need_audio_frame?WIDE("audio"):WIDE(""), file->flags.need_video_frame?WIDE("video"):WIDE("") );
			}

			if( !packet )
				break;

			if( file->flags.seek_read )
			{
				int result;
				/*AVSEEK_FLAG_ANY: Seek to any frame, not just keyframes
					AVSEEK_FLAG_BACKWARD: Seek backward
					AV: Seeking based on position in bytes
					*/
				file->videoFrame = 0;// (file->seek_position * (file->pFormatCtx->duration/1000) * file->pVideoCodecCtx->time_base.den) / ( 1000LL * file->pVideoCodecCtx->ticks_per_frame * file->pVideoCodecCtx->time_base.num * (1000000000LL)  );
				file->audioSamplesPlayed = 0;//file->seek_position * file->pFormatCtx->duration * file->pAudioCodecCtx->sample_rate / 1000000000;
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
				lprintf( WIDE("---------- Set Start Time -------------") );
#endif
				file->media_start_time = ffmpeg.av_gettime();
				//file->pFo
				if( file->flags.was_playing || file->seek_position || 1 )
				{
					if( file->pAudioCodecCtx )
						ffmpeg.avcodec_flush_buffers( file->pAudioCodecCtx );
					if( file->pVideoCodecCtx )
						ffmpeg.avcodec_flush_buffers( file->pVideoCodecCtx );
					result = ffmpeg.av_seek_frame(file->pFormatCtx, -1/*file->videoStream*/, (file->pFormatCtx->duration * file->seek_position) / 1000000000, (file->seek_position==0)?AVSEEK_FLAG_BACKWARD: /*AVSEEK_FLAG_BYTE*/AVSEEK_FLAG_ANY );
					if( result < 0 )
						result = ffmpeg.av_seek_frame(file->pFormatCtx, -1/*file->videoStream*/, (file->pFormatCtx->duration * file->seek_position) / 1000000000, AVSEEK_FLAG_ANY );
				}
				else
					result = 0;
				if( result < 0 )
				{
					// error in seek; nothing to play
					if( !file->flags.sent_end_event )
					{
						while( !file->flags.need_video_frame 
							|| !file->flags.need_audio_frame )
						{
							WakeableSleep( 10 );
							lprintf( "wake up for more packets" );
						}
						file->flags.sent_end_event = 1;
						if( file->video_ended )
							file->video_ended( file->psvEndedParam );
					}
					lprintf( WIDE("Seek was in error; flag no more packets.") );
					file->flags.no_more_packets = 1;

					if( packet ) {
						ffmpeg.av_free_packet( packet );
						EnqueLink( &file->packet_queue, packet );
						packet = NULL;
					}
					continue;  // go back to the top
				}
				else
				{
					file->flags.sent_end_event = 0;
					file->flags.no_more_packets = 0;
				}
				file->flags.seek_read = 0;
			}

			file->flags.reading_frames = 1;

			if( ffmpeg.av_read_frame(file->pFormatCtx, packet) < 0 )
			{
				// loop around to go to sleep
				// a seek might enable playing again...
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
				lprintf( WIDE("packet reader; no more packets.  %d  %d"), GetQueueLength( file->pVideoPackets ), GetQueueLength( file->al_used_buffer_queue ) );
#endif
				file->flags.no_more_packets = 1;
				while( ( GetQueueLength( file->pVideoPackets ) ) || ( GetQueueLength( file->al_used_buffer_queue ) ) )
				{
					//lprintf( WIDE("waiting for video and audio to end.... %d %d %d %d"), file->flags.need_video_frame, file->video_packets,  file->flags.need_audio_frame, GetQueueLength( file->al_used_buffer_queue ) );
					WakeableSleep( 10 );
					//lprintf( "wake up for more packets" );
				}
#ifdef DEBUG_LOW_LEVEL_PACKET_IO
				lprintf( WIDE("threads are both like empty") );
#endif
				if( !file->flags.sent_end_event )
				{
#ifdef DEBUG_LOW_LEVEL_PACKET_IO
					lprintf( WIDE("Send end event...") );
#endif
					file->flags.sent_end_event = 1;
					if( file->video_ended )
						file->video_ended( file->psvEndedParam );
				}

				if( packet ) {
					ffmpeg.av_free_packet( packet );
					EnqueLink( &file->packet_queue, packet );
					packet = NULL;
				}
				continue;
			}
			file->flags.was_playing = 1;
			// Is this a packet from the video stream?
			if( packet->stream_index==file->videoStream ) 
			{
				//if( GetQueueLength( file->pVideoPackets ) > l.default_outstanding_video  )
				//	lprintf( "queue is %d   %d", GetQueueLength( file->pVideoPackets ), GetQueueLength( file->packet_queue ) );

				EnqueLink( &file->pVideoPackets, packet );
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
				lprintf( WIDE("add packet to video... %p(%d)"), packet, GetQueueLength( file->pVideoPackets ) );
#endif
				if( file->flags.need_video_frame )
				{
					if( GetQueueLength( file->pVideoPackets ) > l.default_outstanding_video )
					{
						if( !file->flags.video_paused ) // paused is sleeping for packets...
							file->flags.need_video_frame = 0;
						if( file->flags.using_video_frame || file->flags.video_decoder_sleeping_on_output ) {
							// it's using a frame, give it some time ... 
							//lprintf( "sleep...." );
							WakeableSleep( (uint32_t)(file->frame_del / 1000) );
						}
					}
					if( file->videoThread )
					{
						if( !file->flags.using_video_frame ) {
							//lprintf( "Video decoder isn't using packets; wake it up." );
							WakeThread( file->videoThread );
						}
					}
					else
					{
						file->flags.video.first_flush = 1;
						file->videoThread = ThreadTo( ProcessVideoFrame, (uintptr_t)file );
						file->flags.video.first_flush = 0;
					}

					if( GetQueueLength( file->pVideoPackets ) > ( l.default_outstanding_video * 2 ) )
					{
						if( !file->flags.video_paused )
							file->flags.need_video_frame = 0;
					}
				}
			}
			else if(packet->stream_index==file->audioStream) 
			{
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
				lprintf( WIDE("add packet to audio...") );
#endif
				EnqueLink( &file->pAudioPackets, packet );

				if( file->flags.need_audio_frame )
				{
					if( ( GetQueueLength( file->pAudioPackets ) > l.default_outstanding_audio )
						&& ( l.default_outstanding_audio > 1 ) )
					{
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
						// this usually happens once... the other one happens more.
						// might be an error at 0
						lprintf( WIDE("Clear need audio frame") );
#endif
						file->flags.need_audio_frame = 0;
					}
					if( file->audioThread )
					{
						// it doesn't HAVE to wake up to take this;
						// it's probably on a medium-frequency wake on processed buffer...
						if( file->flags.audio_sleeping )
						{
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
							lprintf( WIDE("wake audio; new audio...") );
#endif
							WakeThread( file->audioThread );
						}
					}
					else
						file->audioThread = ThreadTo( ProcessAudioFrame, (uintptr_t)file );
				}
			}
			else
			{
#ifdef DEBUG_LOW_LEVEL_PACKET_READ
				lprintf( WIDE("Call free packet..") );
#endif
				if( packet ) {
					ffmpeg.av_free_packet( packet );
					EnqueLink( &file->packet_queue, packet );
					packet = NULL;
				}
			}
		}
	}
#ifdef DEBUG_LOG_INFO
	lprintf(WIDE(" This thread is no longer playing %p %s"), file, file->filename );
#endif
	file->readThread = NULL;
	return 0;
}

//---------------------------------------------------------------------------------------------

void ffmpeg_UnloadFile( struct ffmpeg_file *file )
{
	if( file )
	{
		uint32_t tick = timeGetTime();
		file->flags.close_processing = 1;
		// previously was going to go to end-of-stream with no_more_packets...
		// but decided to leave threads if player is at end of file in case of seek/rewind
		file->flags.no_more_packets = 1;
		WakeThread( file->readThread );
		WakeThread( file->audioThread );
		WakeThread( file->videoThread );
		WakeThread( file->videoOutputThread );
		while( file->readThread || file->audioThread || file->videoThread || file->videoOutputThread )
		{
			if( ( timeGetTime() > tick + 300 ) )
			{
				lprintf( WIDE("threads did not exit timely") );
				break;
			}
			Relinquish();
		}
		//lprintf( WIDE("lock audio out....") );
		EnterCriticalSec( &l.cs_audio_out );
		// lock so we don't mess up current context
#ifndef USE_OPENSL
		{
			dropSoundDevice( &file->sound_device );
			/*
			openal.alDeleteSources(1,&file->al_source);
 
			openal.alcMakeContextCurrent(NULL);
 
			openal.alcDestroyContext( file->alc_context );
 
			openal.alcCloseDevice( file->alc_device );
			*/
		}
#endif
		LeaveCriticalSec( &l.cs_audio_out );
		//lprintf( WIDE("clean buffers....") );
		{
			struct al_buffer *buffer;
			while( buffer = (struct al_buffer *)DequeLink( &file->al_free_buffer_queue ) )
			{
#ifndef USE_OPENSL
				openal.alDeleteBuffers( 1, &buffer->buffer );
#endif
				Release( buffer );
			}
			DeleteLinkQueue( &file->al_free_buffer_queue );
			while( buffer = (struct al_buffer *)DequeLink( &file->al_used_buffer_queue ) )
			{
#ifndef USE_OPENSL
				openal.alDeleteBuffers( 1, &buffer->buffer );
#endif
				ffmpeg.av_free( buffer->samplebuf );
				Release( buffer );
			}
			DeleteLinkQueue( &file->al_used_buffer_queue );

			while( buffer = (struct al_buffer *)DequeLink( &file->al_pending_buffer_queue ) )
			{
#ifndef USE_OPENSL
				openal.alDeleteBuffers( 1, &buffer->buffer );
#endif
				ffmpeg.av_free( buffer->samplebuf );
				Release( buffer );
			}
			DeleteLinkQueue( &file->al_pending_buffer_queue );
		}
		{
			AVPacket *packet;
			while( packet = (AVPacket *)DequeLink( &file->packet_queue ) )
			{
				// packets in this queue are already emptied
				Release( packet );
			}
			DeleteLinkQueue( &file->packet_queue );
			while( packet = (AVPacket *)DequeLink( &file->pVideoPackets ) )
			{
				//lprintf( "Call free packet.." );
				ffmpeg.av_free_packet(packet);
				Release( packet );
			}
			DeleteLinkQueue( &file->pVideoPackets );
			while( packet = (AVPacket *)DequeLink( &file->pAudioPackets ) )
			{
				//lprintf( "Call free packet.." );
				ffmpeg.av_free_packet(packet);
				Release( packet );
			}
			DeleteLinkQueue( &file->pAudioPackets );
		}

		if( file->pAudioCodecCtx )
			ffmpeg.avcodec_close( file->pAudioCodecCtx );
		if( file->pVideoCodecCtx )
			ffmpeg.avcodec_close( file->pVideoCodecCtx );

		ffmpeg.sws_freeContext( file->img_convert_ctx );
		ffmpeg.av_frame_free( &file->pAudioFrame );
		{
			struct ffmpeg_video_frame *frame;
			while( frame = (struct ffmpeg_video_frame *)DequeLink( &file->pEmptyVideoFrames ) ) {
				ffmpeg.av_free( frame->rgb_buffer );
				ffmpeg.av_frame_free( &frame->pVideoFrameRGB );
				Release( frame );
			}
			DeleteLinkQueue( &file->pEmptyVideoFrames );
			while( frame = (struct ffmpeg_video_frame *)DequeLink( &file->pDecodedVideoFrames ) ) {
				ffmpeg.av_free( frame->rgb_buffer );
				ffmpeg.av_frame_free( &frame->pVideoFrameRGB );
				Release( frame );
			}
			DeleteLinkQueue( &file->pDecodedVideoFrames );
			ffmpeg.av_frame_free( &file->pVideoFrame );
		}

		ffmpeg.avformat_free_context( file->pFormatCtx );
		Release( file->filename );
		Release( file );
	}
}

//---------------------------------------------------------------------------------------------

void ffmpeg_PlayFile( struct ffmpeg_file *file )
{
	if( file->flags.paused )
	{
		int64_t video_frame_time = 1000000LL * file->pVideoCodecCtx->ticks_per_frame * file->pVideoCodecCtx->time_base.num / file->pVideoCodecCtx->time_base.den;
		int64_t video_time_now = ( file->videoFrame ) * video_frame_time;
		int64_t audio_time_pending_1000_tick = file->pAudioCodecCtx?(1000 * 1000 * 1000 / file->pAudioCodecCtx->sample_rate):video_frame_time;
		int64_t video_time_in_audio_frames	= 1000 * video_time_now / audio_time_pending_1000_tick;
		int64_t audio_time_now = file->pAudioCodecCtx?(( ( video_time_in_audio_frames - file->audioSamplesPlayed ) * 1000LL ) / file->pAudioCodecCtx->sample_rate):video_time_now;
		int64_t video_time_tick = ( 1000LL ) * 1000LL *  file->pVideoCodecCtx->ticks_per_frame * (uint64_t)file->pVideoCodecCtx->time_base.num / (uint64_t)file->pVideoCodecCtx->time_base.den;
		int64_t frame_tick_adjust =  ( video_time_tick * file->video_adjust_ticks );

		file->media_start_time = ffmpeg.av_gettime();// - ( video_time_now + audio_time_now + frame_tick_adjust );
		file->videoFrame = 0;
		file->audioSamplesPlayed = 0;
		file->last_tick_update = 0;
		file->flags.paused = 0;
	}
	if( !file->readThread )
	{
		//lprintf( WIDE("and... play the file...") );
		file->readThread = ThreadTo( ProcessFrames, (uintptr_t)file );
	}
	else
	{
		// there won't be any data to process if we seek... need to wake the reader first
		if( file->flags.seek_read )
			WakeThread( file->readThread );
		else
		{
			// otherwise, already have audio data (probably) and video frame for next... so start audio, 
			// which after starting, will wake video, which will both wake read if needed.
			WakeThread( file->audioThread );
			//WakeThread( file->videoThread );
			WakeThread( file->videoOutputThread );
		}
	}
}

//---------------------------------------------------------------------------------------------

void ffmpeg_PauseFile( struct ffmpeg_file *file )
{
	file->flags.paused = 1;
	WakeThread( file->videoThread );
	WakeThread( file->audioThread );
	WakeThread( file->videoOutputThread );  // move to paused state; so close is graceful
	while( file->flags.using_video_frame )
		Relinquish();
}

//---------------------------------------------------------------------------------------------

void ffmpeg_SeekFile( struct ffmpeg_file *file, int64_t target_time )
{
	int was_paused = file->flags.paused;
#ifdef DEBUG_LOW_LEVEL_PACKET_IO
	lprintf( WIDE("Do Seek to %")_64fs, target_time );
#endif
	if( !file->readThread )
	{
		// have to resume reader (probably)
		file->flags.paused = 1;
		file->readThread = ThreadTo( ProcessFrames, (uintptr_t)file );
	}
	else
	{
		if( !was_paused )
		{
			// the wake in this could cause an un-paused state in the threads
			ffmpeg_PauseFile( file );
		}
		while( !file->flags.audio_paused || !file->flags.video_paused || !file->output_waiting_pause )
		{
			Relinquish();
		}
		ClearRequeueAudio( file );
		ClearPendingVideo( file );
		{
			struct ffmpeg_video_frame *frame;
			while( frame = (struct ffmpeg_video_frame *)DequeLink( &file->pDecodedVideoFrames ) )
				EnqueLink( &file->pEmptyVideoFrames, frame );
		}

	}
	file->flags.no_more_packets = 0;  // clear end condition
	file->flags.need_audio_frame = 1;
	file->flags.need_video_frame = 1;
	file->flags.seek_read = 1;
	file->media_start_time = ffmpeg.av_gettime();
	file->internal_pts = 0;
	file->seek_position = target_time;
	if( !was_paused )
		ffmpeg_PlayFile( file );
}

void ffmpeg_SetPositionUpdateCallback( struct ffmpeg_file *file, void (CPROC*video_position_update)( uintptr_t psv, uint64_t tick ), uintptr_t psvUpdate )
{
	file->video_position_update = video_position_update;
	file->psvUpdateParam = psvUpdate;
}

void ffmpeg_StopFile( struct ffmpeg_file *file )
{
	file->flags.no_more_packets = 1;
	ffmpeg_PauseFile( file );
	Relinquish();
	if( !file->flags.sent_end_event )
	{
		file->flags.sent_end_event = 1;
		if( file->video_ended )
			file->video_ended( file->psvEndedParam );
	}
}

void ffmpeg_AdjustVideo( struct ffmpeg_file *file, int frames )
{
	//lprintf( "Frame adjust: %d", frames );

	file->video_adjust_ticks -= frames;
}

void audio_GetCaptureDevices( PLIST *ppList )
{
	const ALchar *pDeviceList;
	InitFFMPEG_audio();

	pDeviceList = openal.alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
	if (pDeviceList)
	{
		//ALFWprintf("\nAvailable Capture Devices are:-\n");

		while (*pDeviceList)
		{
			//ALFWprintf("%s\n", pDeviceList);
			AddLink( ppList, DupCStr( pDeviceList ) );
			pDeviceList += strlen(pDeviceList) + 1;
		}
	}
}

void audio_GetPlaybackDevices( PLIST *ppList )
{
	const ALchar *pDeviceList;
	InitFFMPEG_audio();
	//pDeviceList = openal.alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

	pDeviceList = openal.alcGetString(NULL, ALC_DEVICE_SPECIFIER);
	if (pDeviceList)
	{
		//ALFWprintf("\nAvailable Capture Devices are:-\n");

		while (*pDeviceList)
		{
			//ALFWprintf("%s\n", pDeviceList);
			AddLink( ppList, DupCStr( pDeviceList ) );
			pDeviceList += strlen(pDeviceList) + 1;
		}
	}
}

//---------------------------------------------------------------------
// voice I/O
//---------------------------------------------------------------------

#define DEFAULT_SAMPLE_RATE (44100 / 4)

#define DEFAULT_SAMPLE_TIME_LENGTH 100
#define DEFAULT_SAMPLE_SEGMENTS ( ( ( ( DEFAULT_SAMPLE_TIME_LENGTH * DEFAULT_SAMPLE_RATE )  \
                                    / 1000 )  \
                                  + 159 )     \
                                / 160 ) 

struct audio_device {
	ALCdevice *device;
	ALCcontext *alc_context;
	ALuint al_source;
	int frame_size;
	short *input_data;
	uintptr_t psvCallback;
	struct al_buffer buffer;
	void (CPROC*callback)( uintptr_t, int max_level, POINTER data, size_t );

	gsm gsm_inst;
	uint8_t *compress_buffer;
	size_t compress_segments;
	gsm_signal compress_partial_buffer[160];
	int compress_partial_buffer_idx;

	// playback buffer management
	uint64_t al_last_buffer_reclaim;
	int al_format;
	PLINKQUEUE al_free_buffer_queue; // queue of struct al_buffer
	PLINKQUEUE al_used_buffer_queue; // queue of struct al_buffer
	PLINKQUEUE al_pending_buffer_queue; // queue of struct al_buffer
	PLINKQUEUE al_reque_buffer_queue; // queue of struct al_buffer
	int in_queue;
	int max_in_queue;
	int out_of_queue;
	struct {
		BIT_FIELD paused/* : 1*/;
		BIT_FIELD capturing/* : 1*/;
	} flags;
	PTHREAD audioThread;
	int max_sample_level;
	int input_trigger_level;
	uint32_t last_compress_block_time;
};

static uintptr_t CPROC audio_ReadCaptureDevice( PTHREAD thread )
{
	struct audio_device *ad = (struct audio_device*)GetThreadParam( thread );
	while( 1 )
	{
		int sample;
		int sampleval;
		int max_sample_level;
		int val;
		int skip_sleep;
		if( !ad->flags.paused )
		{
			if( !ad->flags.capturing )
			{
				openal.alcCaptureStart(ad->device);
				ad->flags.capturing = 1;
			}
			openal.alcGetIntegerv(ad->device, ALC_CAPTURE_SAMPLES, 1, &val);
			//lprintf( "capture samples is %d", val );
			if( val > 0 )
			{
				if( val > ad->frame_size )
				{
					skip_sleep = 1;
					val = ad->frame_size;
				}
				else
					skip_sleep = 0;
				openal.alcCaptureSamples( ad->device, ad->input_data, val );
				{
					int n;
					n = 0;
					if( ad->compress_partial_buffer_idx )
					{
						for( n = n; n < val; n++ ) //-V570
						{
							ad->compress_partial_buffer[ad->compress_partial_buffer_idx++] 
								= ((gsm_signal*)ad->input_data)[n];
							if( ad->compress_partial_buffer_idx == 160 )
							{
								//lprintf( "prior encode %d %d", n, p * 33 );
								for( max_sample_level = 0, sample = 0; sample < 160; sample++ )
								{
									sampleval = ad->compress_partial_buffer[sample]; if( sampleval < 0 ) sampleval = -sampleval;
									if( sampleval > max_sample_level )
										max_sample_level = sampleval;
								}
								//lprintf( "block max sample is %d", max_sample_level );
								if( max_sample_level > ad->max_sample_level )
									ad->max_sample_level = max_sample_level;
								if( max_sample_level > ad->input_trigger_level
									|| ad->last_compress_block_time > ( timeGetTime() - 1000 ) )
								{
									ad->last_compress_block_time = timeGetTime();
									gsm_encode( ad->gsm_inst
											  , ad->compress_partial_buffer
											  , ad->compress_buffer + (ad->compress_segments * 33) );
									ad->compress_segments++;
								}
								n++; // did use this sample
								ad->compress_partial_buffer_idx = 0;
								break;
							}
						}
					}
					for( n = n; n < val; n += 160 )
					{
						if( ( val - n ) >= 160 )
						{
							//lprintf( "full block encode %d %d", n, p * 33 );
							for( max_sample_level = 0, sample = 0; sample < 160; sample++ )
							{
								sampleval = ((gsm_signal*)ad->input_data)[n + sample]; if( sampleval < 0 ) sampleval = -sampleval;
								if( sampleval > max_sample_level )
									max_sample_level = sampleval;
							}
							if( max_sample_level > ad->max_sample_level )
								ad->max_sample_level = max_sample_level;
							//lprintf( "block max sample is %d", max_sample_level );
							if( max_sample_level > ad->input_trigger_level
								|| ad->last_compress_block_time > ( timeGetTime() - 1000 ) )
							{
								ad->last_compress_block_time = timeGetTime();
								gsm_encode( ad->gsm_inst
										  , ((gsm_signal*)ad->input_data) + n
										  , ad->compress_buffer + (ad->compress_segments * 33) );
								ad->compress_segments++;
							}
							if( ad->compress_segments == DEFAULT_SAMPLE_SEGMENTS )
							{
								if( ad->callback )
									ad->callback( ad->psvCallback
									            , ad->max_sample_level
									            , ad->compress_buffer
									            , ad->compress_segments * 33 );
								ad->compress_segments = 0;
								ad->max_sample_level = 0;
							}
						}
						else 
						{
							//lprintf( "%d remain", datalen - n );
							break;
						}
					}
					for( n = n; n < val; n++ ) //-V570
					{
						ad->compress_partial_buffer[ad->compress_partial_buffer_idx++] 
							= ((gsm_signal*)ad->input_data)[n];
						if( ad->compress_partial_buffer_idx == 160 )
						{
							for( max_sample_level = 0, sample = 0; sample < 160; sample++ )
							{
								sampleval = ((gsm_signal*)ad->input_data)[n + sample]; if( sampleval < 0 ) sampleval = -sampleval;
								if( sampleval > max_sample_level )
									max_sample_level = sampleval;
							}
							//lprintf( "block max sample is %d", max_sample_level );
							//lprintf( "end block encode %d %d", n, p * 33 );
							if( max_sample_level > ad->max_sample_level )
								ad->max_sample_level = max_sample_level;
							if( max_sample_level > ad->input_trigger_level
								|| ad->last_compress_block_time > ( timeGetTime() - 1000 ) )
							{
								ad->last_compress_block_time = timeGetTime();
								gsm_encode( ad->gsm_inst
										  , ((gsm_signal*)ad->compress_partial_buffer) + n
										  , ad->compress_buffer + (ad->compress_segments * 33) );
								ad->compress_segments++;
							}
							ad->compress_partial_buffer_idx = 0;
						}
					}
				}
				if( ad->compress_segments > (DEFAULT_SAMPLE_SEGMENTS * 2 ) )
				{
					if( ad->callback )
						ad->callback( ad->psvCallback
						            , ad->max_sample_level
						            , ad->compress_buffer
						            , ad->compress_segments * 33 );
					ad->compress_segments = 0;
					ad->max_sample_level = 0;
				}
				if( !skip_sleep )
					WakeableSleep( 50 );
			}
			else
			{
				// was configured by default to have 10 buffers a second... so.. wait 0.1 seconds
				WakeableSleep( 80 );
			}
		}
		else // if( paused )... 
		{
			if( ad->flags.capturing)
			{
				openal.alcCaptureStop(ad->device);
				ad->flags.capturing = 0;
			}
			WakeableSleep( 250 );
		}
	}
}

struct audio_device *audio_OpenCaptureDevice( CTEXTSTR devname, void (CPROC*callback)( uintptr_t psvInst, int max_level, POINTER data, size_t ), uintptr_t psvInst )
{
	struct audio_device *ad = New( struct audio_device );
#ifdef UNICODE
	char *_devname = CStrDup( devname );
#  define devname _devname
#endif
	MemSet( ad, 0, sizeof( struct audio_device ) );
	InitFFMPEG_audio();

	ad->device = openal.alcCaptureOpenDevice( devname, DEFAULT_SAMPLE_RATE, AL_FORMAT_MONO16, ad->frame_size = DEFAULT_SAMPLE_RATE/10 );
#ifdef UNICODE
	Deallocate( char *, _devname  );
#  undef devname
#endif
	if( ad->device )
	{
		ad->gsm_inst = gsm_create();
		ad->compress_buffer = NewArray( uint8_t, 33 * DEFAULT_SAMPLE_SEGMENTS * 2 );
		ad->psvCallback = psvInst;
		ad->callback = callback;
		ad->input_data = NewArray( short, ad->frame_size );
		ThreadTo( audio_ReadCaptureDevice, (uintptr_t)ad );
	}
	return ad;
}

void audio_SuspendCapture( struct audio_device *device )
{
	if( device )
		device->flags.paused = 1;
}

void audio_ResumeCapture( struct audio_device *device )
{
	if( device )
		device->flags.paused = 0;
}


static uintptr_t CPROC ProcessPlaybackAudioFrame( PTHREAD thread )
{
	struct audio_device * file = (struct audio_device*)GetThreadParam( thread );
#ifdef DEBUG_LOG_INFO
	lprintf(WIDE(" This thread is playing audio for %s"), file->filename );
#endif
	while( 1 )
	{
		// reclaim processed buffers...
		int processed = 0;
#ifdef USE_OPENSL
		int bufID = 0;
#else
		ALuint bufID = 0;
#endif
		
		EnterCriticalSec( &l.cs_audio_out );
#ifdef USE_OPENSL
		{
			SLresult result;
			if( file->flags.paused )
			{
				int stopped = 0;
				SLuint32 playState;
				do
				{
					result = (*file->bqPlayerPlay)->GetPlayState(file->bqPlayerPlay, &playState);
					if( playState == SL_PLAYSTATE_PLAYING && !stopped )
					{
						stopped = 1;
						lprintf( WIDE("Stop playback and clear buffers...") );
						result = (*file->bqPlayerPlay)->SetPlayState(file->bqPlayerPlay,
																					SL_PLAYSTATE_STOPPED );
						result = (*file->bqPlayerBufferQueue)->Clear(file->bqPlayerBufferQueue );
						{
							struct al_buffer *buffer;
							while( buffer = (struct al_buffer*)DequeLink( &file->al_used_buffer_queue ) )
							{
								// out of queue count remains unchanged because requeue should
								// put them back onto the card.
								EnqueLink( &file->al_reque_buffer_queue, buffer );
							}
						}

					}
					Relinquish();
				}
				while( playState == SL_PLAYSTATE_PLAYING );
			}
		}
#endif

#ifndef USE_OPENSL
		if( file->device )
		{
			openal.alcMakeContextCurrent(file->alc_context);

			//lprintf( WIDE("make current...") );
			if( file->flags.paused )
			{
				int stopped = 0;
				int state;
				do
				{
					openal.alGetSourcei(file->al_source, AL_SOURCE_STATE, &state);
					if(state==AL_PLAYING && !stopped)
					{
						stopped = 1;
						openal.alSourceStop(file->al_source);
					}
					Relinquish();
				}
				while( state==AL_PLAYING );
			}
			do
			{
				openal.alGetSourcei(file->al_source, AL_BUFFERS_PROCESSED, &processed);
				if( processed )
				{
	#ifdef DEBUG_AUDIO_PACKET_READ
					//if( file->flags.paused || file->flags.need_audio_dequeue )
					lprintf( WIDE("Found %d processed buffers... %d  %d"), processed, GetQueueLength( file->al_free_buffer_queue ), GetQueueLength( file->al_used_buffer_queue ) );
	#endif
					while (processed--) {
						struct al_buffer *buffer;
						buffer = (struct al_buffer *)DequeLink( &file->al_used_buffer_queue );
						openal.alSourceUnqueueBuffers(file->al_source, 1, &bufID);
						file->out_of_queue--;
						if( !file->flags.paused )
						{
							//file->audioSamplesPlayed += buffer->samples;
							//file->audioSamplesPending -= buffer->samples;
	#ifdef DEBUG_AUDIO_PACKET_READ
							lprintf( WIDE("Samples : %d  %d %d"), buffer->samples, file->audioSamplesPlayed, file->audioSamplesPending );
	#endif
							if( buffer->buffer != bufID )
							{
								lprintf( WIDE("audio queue out of sync") );
								//DebugBreak();
							}
	#ifdef DEBUG_AUDIO_PACKET_READ
							lprintf( WIDE("release audio buffer.. %p"), buffer->samplebuf );
	#endif
							//LogTime(file, FALSE, WIDE("audio deque"), 1 DBG_SRC );
							EnqueLink( &file->al_free_buffer_queue, buffer );
							if( GetQueueLength( file->al_free_buffer_queue ) > file->max_in_queue )
								file->max_in_queue = (int)GetQueueLength( file->al_free_buffer_queue );
						}
						else
						{
							//lprintf( WIDE("Keeping the buffer to requeue...") );
							EnqueLink( &file->al_reque_buffer_queue, buffer );
						}
					}
					//lprintf( WIDE("save reclaim tick...") );
					//file->al_last_buffer_reclaim = ffmpeg.av_gettime();
					// audio got a new play; after a resume, so now wake video thread
				}
				else
				{
					//if( file->flags.paused || file->flags.need_audio_dequeue )
	#ifdef DEBUG_AUDIO_PACKET_READ
					lprintf( WIDE("Found %d processed buffers... %d  %d"), 0, GetQueueLength( file->al_free_buffer_queue ), file->out_of_queue );
	#endif
					LeaveCriticalSec( &l.cs_audio_out );
					WakeableSleep( 250 );
					EnterCriticalSec( &l.cs_audio_out );
				}
			} while( ( file->flags.paused && file->out_of_queue ) );
	#endif
		}

		//if( file->flags.close_processing )
		//	break;
		LeaveCriticalSec( &l.cs_audio_out );

		// need to do this always not just if packet; will always be a packet...
		{
			//lprintf( WIDE("(moved all fom in(file/avail) to out(openal) in : %d  out: %d   max : %d"), file->in_queue, file->out_of_queue, file->max_in_queue );
		}
	}
#ifdef DEBUG_LOG_INFO
	lprintf(WIDE(" This thread is no longer playing playback audio") );
#endif
	return 0;
}

static void GetPlaybackAudioBuffer( struct audio_device * file, POINTER data, size_t sz
#ifdef USE_OPENSL
                                  , int samples
#else
                                  , ALuint samples
#endif
                                  )
{
#ifdef USE_OPENSL
	SLresult result;
#else
	ALenum error;
#endif
	struct al_buffer * buffer;
	if( !( buffer = (struct al_buffer*)DequeLink( &file->al_free_buffer_queue ) ) )
	{
		buffer = New( struct al_buffer );
#ifdef USE_OPENSL
		buffer->buffer = NewArray( uint8_t, sz );
		MemCpy( buffer->buffer, data, sz );
#else
		openal.alGenBuffers( 1, &buffer->buffer );
#endif
	}
	buffer->size = sz;
	buffer->samples = samples;

#ifdef DEBUG_AUDIO_PACKET_READ
	//lprintf( WIDE("use buffer (%d)  %p  for %d(%d)"), file->in_queue, buffer, sz, samples );
#endif

#ifdef USE_OPENSL
	if( file->flags.use_soft_queue )
	{
		//lprintf( WIDE("just pend this in the soft queue....") );
		EnqueLink( &file->al_pending_buffer_queue, buffer );
	}
	else if( file->bqPlayerBufferQueue )
	{
		result = (*file->bqPlayerBufferQueue)->Enqueue(file->sound_device->bqPlayerBufferQueue, data, sz );
		if( result  )
		{
			if( result == SL_RESULT_BUFFER_INSUFFICIENT )
			{
				lprintf( WIDE("overflow driver, enable soft queue...") );
				file->flags.use_soft_queue = 1;
				EnqueLink( &file->al_pending_buffer_queue, buffer );
			}
			else
				lprintf( WIDE("failed to enqueue: %d"), result );
		}
		else
		{
			EnqueLink( &file->al_used_buffer_queue, buffer );
			file->out_of_queue++;
		}
	}
	else
	{
		Release( buffer->buffer );
		EnqueLink( &file->al_free_buffer_queue, buffer );
	}
	if( file->videoFrame )
		SetAudioPlay( file );
#else
	EnterCriticalSec( &l.cs_audio_out );
	openal.alcMakeContextCurrent(file->alc_context);
	openal.alBufferData(buffer->buffer, file->al_format, data, (ALsizei)sz, DEFAULT_SAMPLE_RATE );
	openal.alSourceQueueBuffers(file->al_source, 1, &buffer->buffer); // expects array, so address of buffer int
	if(( error = openal.alGetError()) != AL_NO_ERROR)
	{
			lprintf( WIDE("error adding audio buffer to play.... %d"), error );
	}
	EnqueLink( &file->al_used_buffer_queue, buffer );
	file->out_of_queue++;

	{
		int val;
		openal.alGetSourcei(file->al_source, AL_SOURCE_STATE, &val);
		if(val != AL_PLAYING)
		{
			openal.alSourcePlay(file->al_source);
			if(( error = openal.alGetError()) != AL_NO_ERROR)
		{	
				lprintf( WIDE("error to play.... %d"), error );
			}
		}
	}
	LeaveCriticalSec( &l.cs_audio_out );
#endif
	if( !file->audioThread )
		file->audioThread = ThreadTo( ProcessPlaybackAudioFrame, (uintptr_t)file );
}


struct audio_device *audio_OpenPlaybackDevice( CTEXTSTR devname )
{
	struct audio_device *ad = New( struct audio_device );
	MemSet( ad, 0, sizeof( struct audio_device ) );
	InitFFMPEG_audio();
	ad->device = openal.alcOpenDevice( devname );
	if( ad->device )
	{
		ad->gsm_inst = gsm_create();
		ad->al_format = AL_FORMAT_MONO16;
		ad->alc_context = openal.alcCreateContext(ad->device, NULL);
		if( openal.alcMakeContextCurrent(ad->alc_context) )
		{
			openal.alGenSources((ALuint)1, &ad->al_source);
			// check for errors
			openal.alSourcef(ad->al_source, AL_PITCH, 1);
			// check for errors
			openal.alSourcef(ad->al_source, AL_GAIN, 1);
			// check for errors
			openal.alSource3f(ad->al_source, AL_POSITION, 0, 0, 0);
			// check for errors
			openal.alSource3f(ad->al_source, AL_VELOCITY, 0, 0, 0);
			// check for errors
			openal.alSourcei(ad->al_source, AL_LOOPING, AL_FALSE);
			// check for errros
		}
	}
	return ad;
}

void audio_PlaybackBuffer( struct audio_device *ad, POINTER data, size_t datalen )
{
	int n;
	short *decompress_buffer;
	if( datalen % 33 )
	{
		lprintf( WIDE("Invalid bufffer received %d (%d:%d)"), datalen, datalen/33, datalen%33 );
		return;
	}
	datalen /= 33;
	decompress_buffer = NewArray( short, datalen * 160 );
	for( n = 0; n < datalen; n ++ )
	{
		gsm_decode( ad->gsm_inst, ((gsm_byte*)data) + n*33, decompress_buffer + n * 160 );
	}
	GetPlaybackAudioBuffer( ad, decompress_buffer, n * 320, n * 160 );
	Deallocate( short *, decompress_buffer );
}

void audio_SetInputLevel( struct audio_device *ad, int level )
{
	if( ad )
		ad->input_trigger_level = level;
}

_FFMPEG_INTERFACE_NAMESPACE_END
SACK_NAMESPACE_END
