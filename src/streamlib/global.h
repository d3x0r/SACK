typedef struct {
	struct {
		BIT_FIELD bRunning  :  1;
		BIT_FIELD bExit  :  1;
	} flags;
	PCAPTURE_DEVICE pDev[2];

	S_32 display_x, display_y;
	_32 display_width, display_height;
	SOCKADDR *saBroadcast;
	PLIST controls;
	_32 capture_thread;
   //PIMAGE_INTERFACE pii;
} GLOBAL;


