#ifdef __cplusplus

#include "netstruc.h"
#include <network.h>



NETWORK_PROC( void, NETWORK::WrapReadComplete )( PTRSZVAL psv, POINTER buffer, int nSize )
	{ ((class network*)psv)->ReadComplete( buffer, nSize ); }
NETWORK_PROC( void, NETWORK::WrapReadComplete )( PTRSZVAL psv, POINTER buffer, int nSize, SOCKADDR *sa )
	{ ((class network*)psv)->ReadComplete( buffer, nSize, sa ); }
NETWORK_PROC( void, NETWORK::WrapWriteComplete )( PTRSZVAL psv )
	{ ((class network*)psv)->WriteComplete( ); }
NETWORK_PROC( void, NETWORK::WrapConnectComplete )( PTRSZVAL psv, int nError )
	{ ((class network*)psv)->ConnectComplete( nError ); }
NETWORK_PROC( void, NETWORK::WrapConnectComplete )( PTRSZVAL psv, PCLIENT pcNew )
{
   NETWORK net(pcNew);
	((PNETWORK)psv)->ConnectComplete( net );
}
NETWORK_PROC( void, NETWORK::WrapCloseCallback )( PTRSZVAL psv )
	{ ((class network*)psv)->CloseCallback( ); }

NETWORK_PROC( void, NETWORK::SetNotify)( PCLIENT pc, cppNotifyCallback proc, PTRSZVAL psv )
{
	pc->connect.CPPClientConnected = proc;
	pc->psvConnect = psv;
   pc->dwFlags |= CF_CPPCONNECT;

}
NETWORK_PROC( void, NETWORK::SetConnect)( PCLIENT pc, cppConnectCallback proc, PTRSZVAL psv )
{
	pc->connect.CPPThisConnected = proc;
	pc->psvConnect = psv;
   pc->dwFlags |= CF_CPPCONNECT;
}
NETWORK_PROC( void, NETWORK::SetRead)( PCLIENT pc, cppReadComplete proc, PTRSZVAL psv )
{
	pc->read.CPPReadComplete = proc;
	pc->psvRead = psv;
   pc->dwFlags |= CF_CPPREAD;
}
NETWORK_PROC( void, NETWORK::SetWrite)( PCLIENT pc, cppWriteComplete proc, PTRSZVAL psv )
{
	pc->write.CPPWriteComplete = proc;
	pc->psvRead = psv;
   pc->dwFlags |= CF_CPPWRITE;
}
NETWORK_PROC( void, NETWORK::SetClose)( PCLIENT pc, cppCloseCallback proc, PTRSZVAL psv )
{
	pc->close.CPPCloseCallback = proc;
	pc->psvClose = psv;
   pc->dwFlags |= CF_CPPCLOSE;
}

#endif