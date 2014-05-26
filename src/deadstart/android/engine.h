#define NO_FILEOP_ALIAS
#include <render.h> // PINPUT_POINT

#include <android/sensor.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/native_activity.h>
#include "android_native_app_glue.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "org.d3x0r.sack.native-activity[" __FILE__"]", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "org.d3x0r.sack.native-activity[" __FILE__"]", __VA_ARGS__))

#ifdef lprintf
#undef lprintf
#endif
#define lprintf LOGI


/**
 * Our saved state data.
 */
struct saved_state {
	int32_t x;
	int32_t y;
	int animating;
	int closed; // set this, next event loop, trigger native exit.
	int restarting;
	int rendering;
	int opened;
	int orientation;
};

/**
 * Shared state for our app.
 */
struct engine {
	struct android_app* app;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	ASensorEventQueue* sensorEventQueue;

#if __USE_NATIVE_APP_EGL_MODULE__
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
#endif
	int32_t width;
	int32_t height;
	struct saved_state state;
	volatile int wait_for_startup;
   volatile int have_focus;  // allow animating callback
	//volatile int wait_for_display_init;
	struct input_point points[10];
	struct input_point tmp_points[10]; // need some for computation
	int input_point_map[10];
	int nPoints;
	int did_finish_once;
	int key_text;
	CTEXTSTR data_path;
	int resumed;

	struct library_loader {
		LOGICAL waiting_for_load;
		POINTER loaded_address;
      size_t loaded_size;
	}loader;
};
