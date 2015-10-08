package org.d3x0r.sack.core;

public class NativeStaticLib {
// think I might have to catch landscape event here for some android revisions

   public native static void update_keyboard_size( int size );
   public native static void setDataPath( String package_name );
   public native static int  sendKeyEvent( int pressed, int keyval, int keymod, int keyscan, String key );
   public native static void disableNdkKeyInput( );
}
