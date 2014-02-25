

#include <jni.h>

#include "engine.h"
extern struct engine engine;

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
        lJNIEnv->GetStaticFieldID(ClassContext, 
            "INPUT_METHOD_SERVICE", "Ljava/lang/String;"); 
    jobject INPUT_METHOD_SERVICE = 
        lJNIEnv->GetStaticObjectField(ClassContext, 
            FieldINPUT_METHOD_SERVICE); 
    //jniCheck(INPUT_METHOD_SERVICE);

    // Runs getSystemService(Context.INPUT_METHOD_SERVICE). 
    jclass ClassInputMethodManager = lJNIEnv->FindClass( 
        "android/view/inputmethod/InputMethodManager"); 
    jmethodID MethodGetSystemService = lJNIEnv->GetMethodID( 
        ClassNativeActivity, "getSystemService", 
        "(Ljava/lang/String;)Ljava/lang/Object;"); 
    jobject lInputMethodManager = lJNIEnv->CallObjectMethod( 
        lNativeActivity, MethodGetSystemService, 
        INPUT_METHOD_SERVICE); 

    // Runs getWindow().getDecorView(). 
    jmethodID MethodGetWindow = lJNIEnv->GetMethodID( 
        ClassNativeActivity, "getWindow", 
        "()Landroid/view/Window;"); 
    jobject lWindow = lJNIEnv->CallObjectMethod(lNativeActivity, 
        MethodGetWindow); 
    jclass ClassWindow = lJNIEnv->FindClass( 
        "android/view/Window"); 
    jmethodID MethodGetDecorView = lJNIEnv->GetMethodID( 
        ClassWindow, "getDecorView", "()Landroid/view/View;"); 
    jobject lDecorView = lJNIEnv->CallObjectMethod(lWindow, 
        MethodGetDecorView); 

    if (pShow) { 
        // Runs lInputMethodManager.showSoftInput(...). 
        jmethodID MethodShowSoftInput = lJNIEnv->GetMethodID( 
            ClassInputMethodManager, "showSoftInput", 
            "(Landroid/view/View;I)Z"); 
        jboolean lResult = lJNIEnv->CallBooleanMethod( 
            lInputMethodManager, MethodShowSoftInput, 
            lDecorView, lFlags); 
    } else { 
        // Runs lWindow.getViewToken() 
        jclass ClassView = lJNIEnv->FindClass( 
            "android/view/View"); 
        jmethodID MethodGetWindowToken = lJNIEnv->GetMethodID( 
            ClassView, "getWindowToken", "()Landroid/os/IBinder;"); 
        jobject lBinder = lJNIEnv->CallObjectMethod(lDecorView, 
            MethodGetWindowToken); 

        // lInputMethodManager.hideSoftInput(...). 
        jmethodID MethodHideSoftInput = lJNIEnv->GetMethodID( 
            ClassInputMethodManager, "hideSoftInputFromWindow", 
            "(Landroid/os/IBinder;I)Z"); 
        jboolean lRes = lJNIEnv->CallBooleanMethod( 
            lInputMethodManager, MethodHideSoftInput, 
            lBinder, lFlags); 
    } 

    // Finished with the JVM. 
	 lJavaVM->DetachCurrentThread();
}


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

