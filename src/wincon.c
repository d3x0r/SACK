#include <windows.h>

void gotoxy( int x, int y )
{ COORD p; p.X = x; p.Y=y;
 SetConsoleCursorPosition( GetStdHandle( STD_OUTPUT_HANDLE ), p ); 
}

void setattr( int attr )
{
SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), attr );
}

void getxy( int *x, int *y ) 
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE )
                             , &csbi );
   if( x )
   	*x = csbi.dwCursorPosition.X;
   if( y )
   	*y = csbi.dwCursorPosition.Y;
}


void clrscr( void )
{
	
}

int main( void )
{
	gotoxy( 25,25 );
	printf( WIDE("at 25,25") );
	return 0;
}

// Shell_NotifyIcon// $Log: $
