

#include <jni.h>
//#include <motionDetector.h>
#include "engine.h"
extern struct engine engine;
		 extern "C" void *LoadMemoryLibrary( char *libname, char * real_memory, size_t source_memory_length );

extern "C" void displayKeyboard(int pShow)
{
    // Attaches the current thread to the JVM. 
    jint lResult; 
    jint lFlags = 0; 

    JavaVM* lJavaVM = engine.app->activity->vm; 
    JNIEnv* lJNIEnv = engine.app->activity->env; 

    JavaVMAttachArgs lJavaVMAttachArgs; 
    lJavaVMAttachArgs.version = JNI_VERSION_1_6; 
    lJavaVMAttachArgs.name = "NativeThread"; 
    lJavaVMAttachArgs.group = NULL; 

    lResult=lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
    if (lResult == JNI_ERR) { 
        return; 
    } 

    // Retrieves NativeActivity. 
    jobject lNativeActivity = engine.app->activity->clazz; 
    jclass ClassNativeActivity = lJNIEnv->GetObjectClass(lNativeActivity);

    // Retrieves Context.INPUT_METHOD_SERVICE. 
    jclass ClassContext = lJNIEnv->FindClass("android/content/Context");
    jfieldID FieldINPUT_METHOD_SERVICE = 
        lJNIEnv->GetStaticFieldID(    ClassContext,  "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jobject INPUT_METHOD_SERVICE = 
        lJNIEnv->GetStaticObjectField(ClassContext,  FieldINPUT_METHOD_SERVICE);
    //jniCheck(INPUT_METHOD_SERVICE);

    // Runs getSystemService(Context.INPUT_METHOD_SERVICE). 
    jclass ClassInputMethodManager = lJNIEnv->FindClass(  "android/view/inputmethod/InputMethodManager");
    jmethodID MethodGetSystemService = lJNIEnv->GetMethodID(    ClassNativeActivity, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject lInputMethodManager = lJNIEnv->CallObjectMethod(    lNativeActivity, MethodGetSystemService,  INPUT_METHOD_SERVICE);

    // Runs getWindow().getDecorView(). 
    jmethodID MethodGetWindow = lJNIEnv->GetMethodID(        ClassNativeActivity, "getWindow",   "()Landroid/view/Window;");
    jobject lWindow = lJNIEnv->CallObjectMethod(             lNativeActivity,         MethodGetWindow);
    jclass ClassWindow = lJNIEnv->FindClass(   "android/view/Window");
    jmethodID MethodGetDecorView = lJNIEnv->GetMethodID(       ClassWindow, "getDecorView", "()Landroid/view/View;");
    jobject lDecorView = lJNIEnv->CallObjectMethod(lWindow,  MethodGetDecorView);

    if (pShow) { 
        // Runs lInputMethodManager.showSoftInput(...). 
        jmethodID MethodShowSoftInput = lJNIEnv->GetMethodID( ClassInputMethodManager, "showSoftInput", "(Landroid/view/View;I)Z");
        jboolean lResult = lJNIEnv->CallBooleanMethod(   lInputMethodManager, MethodShowSoftInput,     lDecorView, lFlags);
    } else { 
        // Runs lWindow.getViewToken() 
        jclass ClassView = lJNIEnv->FindClass( "android/view/View");
        jmethodID MethodGetWindowToken = lJNIEnv->GetMethodID( ClassView, "getWindowToken", "()Landroid/os/IBinder;");
        jobject lBinder = lJNIEnv->CallObjectMethod(lDecorView,  MethodGetWindowToken);

        // lInputMethodManager.hideSoftInput(...). 
        jmethodID MethodHideSoftInput = lJNIEnv->GetMethodID(  ClassInputMethodManager, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
        jboolean lRes = lJNIEnv->CallBooleanMethod(   lInputMethodManager, MethodHideSoftInput,  lBinder, lFlags);
    } 

    // Finished with the JVM. 
	 lJavaVM->DetachCurrentThread();
}


// getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
// getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
extern "C" void SuspendSleep( int bStopSleep )
{
	static jobject  WindowManager_LayoutParams_FLAG_KEEP_SCREEN_ON;

    // Attaches the current thread to the JVM. 
    jint lResult; 
    jint lFlags = 0; 

    JavaVM* lJavaVM = engine.app->activity->vm; 
    JNIEnv* lJNIEnv = engine.app->activity->env; 

    JavaVMAttachArgs lJavaVMAttachArgs; 
    lJavaVMAttachArgs.version = JNI_VERSION_1_6; 
    lJavaVMAttachArgs.name = "NativeThread"; 
    lJavaVMAttachArgs.group = NULL; 

    lResult=lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
    if (lResult == JNI_ERR) { 
        return; 
    } 

    // Retrieves NativeActivity. 
    jobject lNativeActivity = engine.app->activity->clazz; 
	jclass ClassNativeActivity = lJNIEnv->GetObjectClass(lNativeActivity);

	if( bStopSleep )
	{
		jmethodID MethodSetFlags = lJNIEnv->GetMethodID(      ClassNativeActivity, "setSuspendSleep", "()V");
      LOGI( "suspend Sleep" );
		if( MethodSetFlags )
			lJNIEnv->CallVoidMethod( lNativeActivity,  MethodSetFlags );
	}
	else
	{
		jmethodID MethodSetFlags = lJNIEnv->GetMethodID(      ClassNativeActivity, "setAllowSleep", "()V");
		if( MethodSetFlags )
			lJNIEnv->CallVoidMethod( lNativeActivity,  MethodSetFlags );
	}



	// Finished with the JVM. 
	lJavaVM->DetachCurrentThread();
}

extern "C" void* AndroidLoadSharedLibrary( char *libname )
{

    // Attaches the current thread to the JVM. 
    jint lResult; 
    jint lFlags = 0; 

    JavaVM* lJavaVM = engine.app->activity->vm; 
    JNIEnv* lJNIEnv = engine.app->activity->env; 

	 engine.loader.waiting_for_load = 1;

    JavaVMAttachArgs lJavaVMAttachArgs; 
    lJavaVMAttachArgs.version = JNI_VERSION_1_6; 
    lJavaVMAttachArgs.name = "NativeThread"; 
    lJavaVMAttachArgs.group = NULL; 

    lResult=lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
    if (lResult == JNI_ERR) { 
        return NULL;
    } 

    // Retrieves NativeActivity. 
    jobject lNativeActivity = engine.app->activity->clazz; 
	 jclass ClassNativeActivity = lJNIEnv->GetObjectClass(lNativeActivity);

	 jstring sendstring = lJNIEnv->NewStringUTF( libname);
	 jmethodID MethodSetFlags = lJNIEnv->GetMethodID(      ClassNativeActivity, "loadLibrary", "(Ljava/lang/String;)V");
	 LOGI( "wait startup (loadlibrarY)" );
	 if( MethodSetFlags )
			lJNIEnv->CallVoidMethod( lNativeActivity,  MethodSetFlags, sendstring );


	 // Finished with the JVM.
	 lJavaVM->DetachCurrentThread();

	 while( engine.loader.waiting_for_load )
	 {
		 sched_yield();
	 }
	 {
       lprintf( "Got a mapped file... pas it to actual load..." );
       return LoadMemoryLibrary( libname, (char*)engine.loader.loaded_address, engine.loader.loaded_size );
	 }
}



extern "C" int AndroidGetStatusMetric( void )
{
   int result = 0;
    // Attaches the current thread to the JVM. 
    jint lResult; 
    jint lFlags = 0; 

    JavaVM* lJavaVM = engine.app->activity->vm; 
    JNIEnv* lJNIEnv = engine.app->activity->env; 

    JavaVMAttachArgs lJavaVMAttachArgs; 
    lJavaVMAttachArgs.version = JNI_VERSION_1_6; 
    lJavaVMAttachArgs.name = "NativeThread"; 
    lJavaVMAttachArgs.group = NULL; 

    lResult=lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
    if (lResult == JNI_ERR) { 
        return 0;
    } 

	 // Retrieves NativeActivity.
	 jobject lNativeActivity = engine.app->activity->clazz;
	 jclass ClassNativeActivity = lJNIEnv->GetObjectClass(lNativeActivity);

	 jmethodID MethodSetFlags = lJNIEnv->GetMethodID(      ClassNativeActivity, "getStatusMetric", "()I");
	 if( MethodSetFlags )
		 result = lJNIEnv->CallIntMethod( lNativeActivity,  MethodSetFlags );


	 // Finished with the JVM.
	 lJavaVM->DetachCurrentThread();
	 return result;
}

extern "C" int AndroidGetKeyText( AInputEvent *event )
{
   int result = 0;
    // Attaches the current thread to the JVM. 
    jint lResult; 
    jint lFlags = 0; 

    JavaVM* lJavaVM = engine.app->activity->vm; 
    JNIEnv* lJNIEnv = engine.app->activity->env; 

    JavaVMAttachArgs lJavaVMAttachArgs; 
    lJavaVMAttachArgs.version = JNI_VERSION_1_6; 
    lJavaVMAttachArgs.name = "NativeThread"; 
    lJavaVMAttachArgs.group = NULL; 

    lResult=lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
    if (lResult == JNI_ERR) { 
        return 0;
    } 

    // Retrieves NativeActivity. 
    jobject lNativeActivity = engine.app->activity->clazz; 
	jclass ClassNativeActivity = lJNIEnv->GetObjectClass(lNativeActivity);

	jmethodID MethodSetFlags = lJNIEnv->GetMethodID(      ClassNativeActivity, "getKeyText", "(JJIIIIIII)I");
   lprintf( "..." );
		if( MethodSetFlags )
			result = lJNIEnv->CallIntMethod( lNativeActivity,  MethodSetFlags
													 , AKeyEvent_getDownTime(event)
													 , AKeyEvent_getEventTime(event)
													 , AKeyEvent_getAction(event)
													 , AKeyEvent_getKeyCode(event)
													 , AKeyEvent_getRepeatCount(event)
													 , AKeyEvent_getMetaState(event)
													 , AInputEvent_getDeviceId(event)
													 , AKeyEvent_getScanCode(event)
													 , AKeyEvent_getFlags(event)
													 );
		else
		{
         lprintf( "Failed to get method." );
			result = 0;
		}


	// Finished with the JVM. 
		lJavaVM->DetachCurrentThread();
      return result;
}

#if 0
int GetUnicodeChar(struct android_app* app, int eventType, int keyCode, int metaState)
{
JavaVM* javaVM = app->activity->vm;
JNIEnv* jniEnv = app->activity->env;

JavaVMAttachArgs attachArgs;
attachArgs.version = JNI_VERSION_1_6;
attachArgs.name = "NativeThread";
attachArgs.group = NULL;

jint result = javaVM->AttachCurrentThread(&jniEnv, &attachArgs);
if(result == JNI_ERR)
{
    return 0;
}

jclass class_key_event = jniEnv->FindClass("android/view/KeyEvent");
int unicodeKey;

if(metaState == 0)
{
    jmethodID method_get_unicode_char = jniEnv->GetMethodID(class_key_event, "getUnicodeChar", "()I");
    jmethodID eventConstructor = jniEnv->GetMethodID(class_key_event, "<init>", "(II)V");
    jobject eventObj = jniEnv->NewObject(class_key_event, eventConstructor, eventType, keyCode);

    unicodeKey = jniEnv->CallIntMethod(eventObj, method_get_unicode_char);
}

else
{
    jmethodID method_get_unicode_char = jniEnv->GetMethodID(class_key_event, "getUnicodeChar", "(I)I");
    jmethodID eventConstructor = jniEnv->GetMethodID(class_key_event, "<init>", "(II)V");
    jobject eventObj = jniEnv->NewObject(class_key_event, eventConstructor, eventType, keyCode);

    unicodeKey = jniEnv->CallIntMethod(eventObj, method_get_unicode_char, metaState);
}

javaVM->DetachCurrentThread();

LOGI("Unicode key is: %d", unicodeKey);
return unicodeKey;
}
#endif

/*

private void hideStatusBar() {
        WindowManager.LayoutParams attrs = getWindow().getAttributes();
        attrs.flags |= WindowManager.LayoutParams.FLAG_FULLSCREEN;
        getWindow().setAttributes(attrs);
    }

    private void showStatusBar() {
        WindowManager.LayoutParams attrs = getWindow().getAttributes();
        attrs.flags &= ~WindowManager.LayoutParams.FLAG_FULLSCREEN;
        getWindow().setAttributes(attrs);
    }


the trick is
getWindow().addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN);
getWindow().addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
in onCreate


alternative full screen method?
public class FullScreen extends Activity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.main);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
                                WindowManager.LayoutParams.FLAG_FULLSCREEN);

    }
}
// see http://androidsnippets.com/how-to-make-an-activity-fullscreen

android manifest feature:
android:theme="@android:style/Theme.NoTitleBar.Fullscreen"

*/

