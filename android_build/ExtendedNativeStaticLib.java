package org.d3x0r.sack.core;



public class NativeStaticLib {
// think I might have to catch landscape event here for some android revisions

   public class KCOM_Message
   {
      String msg;
      String from;
   }

   public interface NativeMessageCallback {
      void ReceivedMessage( KCOM_Message msg );
   }

   NativeMessageCallback receive_callback;

   public native static void update_keyboard_size( int size );
   public native static void setDataPath( String package_name );

   public native static bool reconnect( );
   public native static void login( String name, String password );
   public native static void enqueMessage( String destination, String package_name );
   public static void SetCallback( NativeMessageCallback callback )
   {
      receive_callback = callback;
   }

   public static void TriggerCallback( String message, String from )
   {
      KCOM_Message msg;
      msg.msg = message;
      msg.from = from;
      if( received_callback )
         receive_callback( msg );
   }

}
