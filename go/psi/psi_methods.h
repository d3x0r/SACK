

typedef int (*mouse_method)( void *inst, int32_t x, int32_t y, uint32_t b );

typedef struct method_tracker {
	void *instance;
	mouse_method mouse;
} *psi_method_tracker;
