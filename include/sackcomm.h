#ifndef SACKCOMM_PROTECT_ME_AGAINST_DOBULE_INCLUSION
#define SACKCOMM_PROTECT_ME_AGAINST_DOBULE_INCLUSION
#include <sack_types.h>


#ifdef SACKCOMM_SOURCE
#define SACKCOMM_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define SACKCOMM_PROC(type,name) IMPORT_METHOD type CPROC name
#endif



#define SACKCOMM_ERR_NONE_MORE (   2)
#define SACKCOMM_ERR_NONE_DONE (   1)
#define SACKCOMM_ERR_NONE      (   0)
#define SACKCOMM_ERR_ALLOC     (  -1)
#define SACKCOMM_ERR_COMM      (  -2)
#define SACKCOMM_ERR_TIMEOUT   (  -3)
#define SACKCOMM_ERR_PARTIAL   (  -4)
#define SACKCOMM_ERR_BUFSIZE   (  -5)
#define SACKCOMM_ERR_MORE      (  -6)
#define SACKCOMM_ERR_POINTER   (  -7)
#define SACKCOMM_ERR_UNDERFLOW ( -11)
#define SACKCOMM_ERR_BUSY      ( -12) 
#define SACKCOMM_ERR_NOTOPEN   ( -13)
#define SACKCOMM_ERR_MIN -20

#ifdef __LINUX__
typedef void DCB;
typedef void COMSTAT;
#define STDPROC
#endif

SACKCOMM_PROC( void, SetCommRTS )( int nCommID, int iRTS );

SACKCOMM_PROC( int, SackFlushComm )(int iCommId, int iInOut);
SACKCOMM_PROC( int, SackGetCommState)(int iCommId, DCB FAR *lpDcb);
SACKCOMM_PROC( int, SackGetCommError)(int iCommId, COMSTAT FAR *lpStat);
SACKCOMM_PROC( int, SackSetCommState)(DCB FAR *lpDcb);
SACKCOMM_PROC( int, SackWriteComm)(int iCommId, void far *pBuf, int iChars);
SACKCOMM_PROC( int, SackReadComm)(int iCommId, void far *pBuf, int iChars);
SACKCOMM_PROC( int, SackCloseComm)(int iCommId);

typedef void (CPROC* CommReadCallback)( PTRSZVAL psv, int nCommId, POINTER buffer, int len );

SACKCOMM_PROC( int, SackOpenCommEx)(CTEXTSTR szPort, _32 uiRcvQ, _32 uiSendQ
											, CommReadCallback ReadCallback
                  					, PTRSZVAL psv );
#define SackOpenComm( szport, rq, sq ) SackOpenCommEx( szport, rq, sq, NULL, 0 )

SACKCOMM_PROC( void, SackSetReadCallback )( int nCommId
                                          , CommReadCallback Callback
                                          , PTRSZVAL psvRead );
SACKCOMM_PROC( int, SackClearReadCallback )( int iCommId
                                          , CommReadCallback );

SACKCOMM_PROC( int, SackCommReadBufferEx)( int iCommId, char *buffer, int len
						 , _32 timeout, int *pnCharsRead
									 DBG_PASS );
#define SackCommReadBuffer(c,b,l,t,pl) SackCommReadBufferEx( c,b,l,t,pl DBG_SRC )

SACKCOMM_PROC( int, SackCommReadDataEx)( int iCommId
						 , _32 timeout
						 , char **pBuffer
						 , int *pnCharsRead 
						 DBG_PASS
					  );
#define SackCommReadData(c,t,pb,pn) SackCommReadDataEx( c,t,pb,pn DBG_SRC )
SACKCOMM_PROC( int,  SackCommWriteBufferEx)( int iCommId, char *buffer, int len
							  , _32 timeout DBG_PASS );
#define SackCommWriteBuffer(c,b,l,t) SackCommWriteBufferEx(c,b,l,t DBG_SRC)
SACKCOMM_PROC( void, SackCommFlush )( int nCommID );


// changes the read buffer size from 1024 to (bytes?)
SACKCOMM_PROC( void, SackSetBufferSize )( int iCommId
													 , int readlen );

#define WM_COMM_OPEN      WM_USER + 100
#define WM_COMM_CLOSE     WM_USER + 101
#define WM_COMM_WRITE     WM_USER + 102
#define WM_COMM_DATA      WM_USER + 103
#define WM_COMM_GETERROR  WM_USER + 104
#define WM_COMM_FLUSH     WM_USER + 105
#define WM_COMM_CLOSE_ALL WM_USER + 106
#define WM_COMM_PING      WM_USER + 107

// normal mode - everyone gets same data notifications
#define COM_PORT_OWN_SHARE 0
// exclusive - only this channel callback will get notification
#define COM_PORT_OWN_EXCLUSIVE 1
// normal mode but all channels except this one will get notification
#define COM_PORT_IGNORE 2
SACKCOMM_PROC( void, SackCommOwnPort )( int nCommID, CommReadCallback func, int own_flags );


#endif
