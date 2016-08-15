#include <stdhdrs.h>
#include <image.h>
#include <timers.h>

typedef struct capture_device_tag *PCAPTURE_DEVICE;

typedef struct frame_capture_callback {
   DeclareLink( struct frame_capture_callback );
	int (CPROC *callback)(uintptr_t psv, PCAPTURE_DEVICE pDev );
	uintptr_t psv;
} *PCAPTURE_CALLBACK, CAPTURE_CALLBACK;

typedef struct simple_queue_tag
{
   uintptr_t *frames;
   INDEX head, tail, size;
} SIMPLE_QUEUE;


typedef struct capture_device_tag
{
	uint32_t width, height; // desired width, height, also result if failed.

	struct {
		//PCLASSROOT data_class;
		void CPROC (*release)( uintptr_t psv, POINTER data, INDEX length ); // callback used when data should be replaced.
      uintptr_t psv;
		POINTER data;
		INDEX length;
	};

	PCAPTURE_CALLBACK callbacks;
   PTHREAD pCallbackThread;
} CAPTURE_DEVICE;


void GetDeviceData( PCAPTURE_DEVICE pDevice, POINTER *data, INDEX *length );
typedef void CPROC (*ReleaseBitStreamData)(uintptr_t,POINTER,INDEX);
void SetDeviceDataEx( PCAPTURE_DEVICE pDevice, POINTER data, INDEX length
						  , ReleaseBitStreamData release,uintptr_t);
#define SetDeviceData(dev,data,len) SetDeviceDataEx(dev,data,len,NULL,0)

uintptr_t DequeFrameEx( SIMPLE_QUEUE *que, char *quename, int line );
#define DequeFrame(q) DequeFrameEx(q,#q, __LINE__ )
int EnqueFrameEx( SIMPLE_QUEUE *que, uintptr_t frame, char *qname, int line );
#define EnqueFrame(q,f) EnqueFrameEx(q,f,#q, __LINE__ )
INDEX FramesQueued( SIMPLE_QUEUE *que );


