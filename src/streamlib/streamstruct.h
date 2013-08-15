#include <stdhdrs.h>
#include <image.h>
#include <timers.h>

typedef struct capture_device_tag *PCAPTURE_DEVICE;

typedef struct frame_capture_callback {
   DeclareLink( struct frame_capture_callback );
	int (CPROC *callback)(PTRSZVAL psv, PCAPTURE_DEVICE pDev );
	PTRSZVAL psv;
} *PCAPTURE_CALLBACK, CAPTURE_CALLBACK;

typedef struct simple_queue_tag
{
   PTRSZVAL *frames;
   INDEX head, tail, size;
} SIMPLE_QUEUE;


typedef struct capture_device_tag
{
	_32 width, height; // desired width, height, also result if failed.

	struct {
		//PCLASSROOT data_class;
		void CPROC (*release)( PTRSZVAL psv, POINTER data, INDEX length ); // callback used when data should be replaced.
      PTRSZVAL psv;
		POINTER data;
		INDEX length;
	};

	PCAPTURE_CALLBACK callbacks;
   PTHREAD pCallbackThread;
} CAPTURE_DEVICE;


void GetDeviceData( PCAPTURE_DEVICE pDevice, POINTER *data, INDEX *length );
typedef void CPROC (*ReleaseBitStreamData)(PTRSZVAL,POINTER,INDEX);
void SetDeviceDataEx( PCAPTURE_DEVICE pDevice, POINTER data, INDEX length
						  , ReleaseBitStreamData release,PTRSZVAL);
#define SetDeviceData(dev,data,len) SetDeviceDataEx(dev,data,len,NULL,0)

PTRSZVAL DequeFrameEx( SIMPLE_QUEUE *que, char *quename, int line );
#define DequeFrame(q) DequeFrameEx(q,#q, __LINE__ )
int EnqueFrameEx( SIMPLE_QUEUE *que, PTRSZVAL frame, char *qname, int line );
#define EnqueFrame(q,f) EnqueFrameEx(q,f,#q, __LINE__ )
INDEX FramesQueued( SIMPLE_QUEUE *que );


