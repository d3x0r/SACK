#include <stdhdrs.h>
#include <windows.h>
#include <sharemem.h>
#include <stubcomm.h>
#include <systray.h>

typedef struct com_port_tag
{
	HWND hWndRecv;
   int handle; // handle that stubcomm results with.
	// work buffer, callback results are read into here.
   _32 dwLastMsg;
	char buffer[4096];
   int buflen;
   DeclareLink( struct com_port_tag );
} COM_PORT, *PCOM_PORT;

typedef struct global_tag
{
	HWND hWnd;
   _32 nWriteTimeout;
   PCOM_PORT pComPorts;
} GLOBAL;

static GLOBAL g;

void CPROC ReadCallback( PTRSZVAL psv, int iCommId, POINTER buffer, int nReadLen )
{
	PCOM_PORT pComPort = (PCOM_PORT)psv;
	ATOM Atom;
	int ofs, c;
   char *p = buffer;
	char szTmp[241];
	//Log( WIDE("Received data...") );
   //LogBinary( buffer, nReadLen );
   for( ofs = 0; ofs < nReadLen; ofs += c )
	{
		int len = 120;
		if( nReadLen - ofs < len )
         len = nReadLen - ofs;
		for( c = 0; c < len; c++ )
		{
			szTmp[c*2] = (*p & 0x0F) + 'A';
			szTmp[c*2+1] = ((*p >> 4 )&0x0F) + 'A';
         p++;
		}
      szTmp[c*2] = 0;
		Atom = GlobalAddAtom( szTmp );
		//Log1( WIDE("Posting atom for data: %s"), szTmp );
		PostMessage( pComPort->hWndRecv, WM_COMM_DATA, pComPort->handle, Atom );
	}
	Log( WIDE("...read data.") );
   Relinquish();
}

int CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_CREATE:
      SetTimer( hWnd, 100, 1000, NULL );
		return TRUE;
	case WM_TIMER:
      while( 1 )
		{
			PCOM_PORT pComPort ;
			for( pComPort = g.pComPorts;
				  pComPort;
				  pComPort = pComPort->next )
			{
            _32 tick = GetTickCount();
				if( ( pComPort->dwLastMsg + 20000 ) < tick )
				{
					Log3( WIDE("Com port last is: %lu cur is: %lu (%lu)"),pComPort->dwLastMsg, tick, tick - pComPort->dwLastMsg );
					if( ( pComPort->dwLastMsg + 40000 ) < tick )
					{
					Log3( WIDE("Com port last is: %lu cur is: %lu (%lu)"),pComPort->dwLastMsg, tick, tick - pComPort->dwLastMsg );
						if( ( pComPort->dwLastMsg + 60000 ) < tick )
						{
					Log3( WIDE("Com port last is: %lu cur is: %lu (%lu)"),pComPort->dwLastMsg, tick, tick - pComPort->dwLastMsg );
							lprintf( WIDE("Receiving window died, closing his comport") );
							if( StubCloseComm( pComPort->handle ) )
							{
								Release( UnlinkThing( pComPort ) );
								break;
							}
						}
						else
						{
							Log( WIDE("Com inactive , ping about it...") );
							SendMessage( pComPort->hWndRecv, WM_COMM_PING, pComPort->handle, 0 );
						}
					}
					else
					{
                  Log( WIDE("Com inactive , ping about it...") );
						SendMessage( pComPort->hWndRecv, WM_COMM_PING, pComPort->handle, 0 );
					}
				}
			}
			if( !pComPort )
            break;
		}
		return TRUE;
	case WM_COMM_PING:
		{
			PCOM_PORT pComPort ;
			for( pComPort = g.pComPorts;
				  pComPort;
				  pComPort = pComPort->next )
			{
				if( pComPort->handle == wParam )
				{
					pComPort->dwLastMsg = GetTickCount();
				}
			}
		}
      return TRUE;
	case WM_COMM_OPEN:
		// wParam == hWnd Receiver
		// lParam == atom of string of port to open.
		{
         PCOM_PORT pComPort = Allocate( sizeof( COM_PORT ) );
			char szPort[64];
         MemSet( pComPort, 0, sizeof( COM_PORT ) );
			GlobalGetAtomName( lParam, szPort, sizeof( szPort ) );
         pComPort->hWndRecv = (HWND)wParam;
			pComPort->handle = StubOpenCommEx( szPort, 4096, 4096, ReadCallback, (PTRSZVAL)pComPort );
         pComPort->dwLastMsg = GetTickCount();
         Log1( WIDE("Resulted with %d"), pComPort->handle );
			GlobalDeleteAtom( lParam );
			if( pComPort->handle >= 0 )
			{
            Log( WIDE("Linking in com port...") );
            LinkThing( g.pComPorts, pComPort );
				return pComPort->handle;
			}
         Log( WIDE("Remove this allocated com port..") );
			Release( pComPort );
         Log( WIDE("Released and resulting with error status...") );
         return -1;
		}
		break;
	case WM_COMM_CLOSE:
		// wParam == handle of com port
      Log1( WIDE("Closing com port %d"), wParam );
		{
			PCOM_PORT pComPort = g.pComPorts;
			while( pComPort )
			{
				if( pComPort->handle == wParam )
				{
					if( StubCloseComm( pComPort->handle ) )
						Release( UnlinkThing( pComPort ) );
               return TRUE;
				}
            pComPort = pComPort->next;
			}
		}
      return STUBCOMM_ERR_NOTOPEN;
	case WM_COMM_CLOSE_ALL:
      Log1( WIDE("Closing all com ports assocated with %d"), wParam );
		while( g.pComPorts )
		{
			if( g.pComPorts->hWndRecv == (HWND)wParam )
			{
				if( StubCloseComm( g.pComPorts->handle ) )
				{
               PCOM_PORT pComPort = g.pComPorts;
					Release( UnlinkThing( pComPort ) );
				}
			}
		}
		return TRUE;
	case WM_COMM_DATA:
		{
			PCOM_PORT pComPort = g.pComPorts;
			while( pComPort )
			{
            //Log( WIDE("Checking a com port for match...") );
				if( pComPort->handle == wParam )
				{
					unsigned char tmp;
					char szData[257];
					char *p = szData;
					pComPort->dwLastMsg = GetTickCount();
               //Log( WIDE("Matched the port, now build the atom...") );
					GlobalGetAtomName( lParam, szData, sizeof( szData ) );
					szData[256] = 0;
					//Log2( WIDE("Got the atom: %d %s"), pComPort->buflen, szData );
					while( p[0] )
					{
                  tmp = ( p[0] - 'A' ) | ((p[1] - 'A') << 4);
						p+=2;
                  pComPort->buffer[pComPort->buflen++] = tmp;
					}
					GlobalDeleteAtom( lParam );
               return STUBCOMM_ERR_NONE;
				}
				pComPort = pComPort->next;
			}
			Log( WIDE("Failed to find handle for write data...") );
		}
      return STUBCOMM_ERR_NOTOPEN;
	case WM_COMM_WRITE:
		{
			PCOM_PORT pComPort = g.pComPorts;
			while( pComPort )
			{
            //Log( WIDE("Checking a com port for match...") );
				if( pComPort->handle == wParam )
				{
					unsigned char tmp;
               int result;
					char szData[257];
					char *p = szData;
					pComPort->dwLastMsg = GetTickCount();
               //Log( WIDE("Matched the port, now build the atom...") );
					GlobalGetAtomName( lParam, szData, sizeof( szData ) );
					szData[256] = 0;
					//Log1( WIDE("Got the atom: %s"), szData );
					while( p[0] )
					{
                  tmp = ( p[0] - 'A' ) | ((p[1] - 'A') << 4);
						p+=2;
                  pComPort->buffer[pComPort->buflen++] = tmp;
					}
					GlobalDeleteAtom( lParam );
               Log( WIDE("Writing data...") );
               //LogBinary( pComPort->buffer, pComPort->buflen );
					result = StubCommWriteBuffer( pComPort->handle, pComPort->buffer, pComPort->buflen, g.nWriteTimeout );
               //Log( WIDE("Done writing data...") );
               pComPort->buflen = 0;
               return result;
				}
            pComPort = pComPort->next;
			}
			Log( WIDE("Failed to find handle for write data...") );
		}
		return STUBCOMM_ERR_NOTOPEN;
	case WM_COMM_FLUSH:
      //Log1( WIDE("Flushing com port %d"), wParam );
		{
			PCOM_PORT pComPort ;
			for( pComPort = g.pComPorts;
				  pComPort;
				  pComPort = pComPort->next )
			{
            //Log( WIDE("Checking a com port for match...") );
				if( pComPort->handle == wParam )
				{
					pComPort->dwLastMsg = GetTickCount();
					StubCommFlush( pComPort->handle );
               return TRUE;
				}
			}
		}
      return STUBCOMM_ERR_NOTOPEN;
	}
   return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

int MakeProxyWindow( void )
{
   ATOM aClass;
	{
		WNDCLASS wc;
		memset( &wc, 0, sizeof(WNDCLASS) );
		wc.style = CS_OWNDC | CS_GLOBALCLASS;

		wc.lpfnWndProc = (WNDPROC)WndProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hbrBackground = 0;
		wc.lpszClassName = "ComProxyClass";
		aClass = RegisterClass( &wc );
		if( !aClass )
		{
			MessageBox( NULL, WIDE("Failed to register class to handle SQL Proxy messagses."), WIDE("INIT FAILURE"), MB_OK );
			return FALSE;
		}
	}

	g.hWnd = CreateWindowEx( 0,
								 (char*)aClass,
								 "ComProxy",
								 0,
								 0,
								 0,
								 0,
								 0,
								 HWND_MESSAGE, // Parent
								 NULL, // Menu
								 GetModuleHandle(NULL),
								 (void*)1 );
	if( !g.hWnd )
	{
      Log( WIDE("Failed to create window!?!?!?!") );
		MessageBox( NULL, WIDE("Failed to create window to handle SQL Proxy Messages"), WIDE("INIT FAILURE"), MB_OK );
      return FALSE;
	}
   return TRUE;
}


int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow )
{
	g.nWriteTimeout = 150; // at 9600 == 144 characters
	SystemLogTime( SYSLOG_TIME_HIGH|SYSLOG_TIME_DELTA );
   RegisterIcon( NULL );
	//SetSystemLog( SYSLOG_FILENAME, WIDE("comproxy.log") );
#ifdef _DEBUG
	SetSystemLog( SYSLOG_UDPBROADCAST, 0 );
#endif
	if( MakeProxyWindow() )
	{
		MSG msg;
		while( GetMessage( &msg, NULL, 0, 0 ) )
			DispatchMessage( &msg );
      return msg.wParam;
	}
   return 0;
}


