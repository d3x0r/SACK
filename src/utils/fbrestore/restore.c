
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <linux/vt.h>

int keyboard_fd = -1;
int current_vt;

#define DEBUG_KEYBOARD

int FB_OpenKeyboard( char *tty )
{
	/* Open only if not already opened */
 	if ( keyboard_fd < 0 ) {
		static const char * const tty0[] = { "/dev/tty0", "/dev/vc/0", NULL };
		//static const char * const vcs[] = { "/dev/vc/%d", "/dev/tty%d", NULL };
		int i, tty0_fd;

		/* Try to query for a free virtual terminal */

		{
			//for ( i=0; vcs[i] && (keyboard_fd < 0); ++i )
			{
				/* This needs to be our controlling tty
				   so that the kernel ioctl() calls work
				*/
			}
		}
		/* Make sure that our input is a console terminal */

		/* Set up keymap */
		//FB_vgainitkeymaps(keyboard_fd);
 	}
 	return(keyboard_fd);
}


int main( int argc, char **argv )
{
   int i;
	// open the key device... set KB_TRANSLATED or whatever

	// open the display, set VT_TEXT
	//for( i = 1; i < 10; i++ )
	if( argc < 2 )
	{
		printf( ("Defaulting to clear FB 7") );
		i = 7;
	}
	else
      i = atoi( argv[1] );
	{
		char tty[32];
      sprintf( tty, ("/dev/tty%d"), i );
		if( ( keyboard_fd = open( tty, O_RDWR, 0) ) < 0 )
		{
			sprintf( tty, ("/dev/vc/%d"), i );
			if( ( keyboard_fd = open( tty, O_RDWR, 0) ) < 0 )
			{
            printf( ("Failed to open /dev/tty%d and/or /dev/vc/%d"), i, i );
            return 0;
			}
		}
		printf( "resetting keyboard and graphic mode on: %s\n", tty );
		ioctl(keyboard_fd, KDSETMODE, KD_TEXT) &&
         (perror( "Failed reset text mode" ),1);
		ioctl(keyboard_fd, KDSKBMODE, K_XLATE) &&
         (perror( "Failed reset keyboard mode" ),1);
		close( keyboard_fd );
      keyboard_fd = -1;
	}
		//tcsetattr(keyboard_fd, TCSAFLUSH, &saved_kbd_termios);
   return 0;

}
