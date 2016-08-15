typedef struct {
	struct {
		BIT_FIELD bRunning  :  1;
		BIT_FIELD bExit  :  1;
	} flags;
	PCAPTURE_DEVICE pDev[2];

	int32_t display_x, display_y;
	uint32_t display_width, display_height;
	SOCKADDR *saBroadcast;
	PLIST controls;
	uint32_t capture_thread;
   //PIMAGE_INTERFACE pii;
} GLOBAL;


