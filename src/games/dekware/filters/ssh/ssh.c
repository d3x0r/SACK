#define DEFINES_DEKWARE_INTERFACE
#define DO_LOGGING
#include <stdhdrs.h>
#include "plugin.h"
#include <sack_ssh.h>

INDEX iSSH;

typedef struct mydatapath_tag {
	DATAPATH common;
	struct {
		uint32_t bUnused : 1;
	} flags;
	PVARTEXT vt; // collects var text....

	struct ssh_session *session;
	struct ssh_chanel *channel;

} MYDATAPATH, *PMYDATAPATH;

//---------------------------------------------------------------------------


static PTEXT CPROC handleSSH( PDATAPATH pdPSI_CONTROL, PTEXT pText )
{
   return NULL;
}

//---------------------------------------------------------------------------

static int CPROC Read( PDATAPATH pdp )
{
	return RelayInput( pdp, handleSSH );
}

//---------------------------------------------------------------------------

static PTEXT CPROC sendSSH( PDATAPATH pndp, PTEXT line )
{
	sack_ssh_channel_write( pmdp->channel, 0, GetText( line ), GetTextSize( line ) );
	return NULL;
}

//---------------------------------------------------------------------------

static int CPROC Write( PDATAPATH pdp )
{
	return RelayOutput( pdp, sendSSH );
}

//---------------------------------------------------------------------------

static int CPROC Close( PDATAPATH pdPSI_CONTROL )
{
   PMYDATAPATH pdp = (PMYDATAPATH)pdPSI_CONTROL;
	VarTextDestroy( &pdp->vt );
	sack_ssh_channel_close( pdp->channel );
	sack_ssh_session_close( pdp->session );
   pdp->common.Type = 0;
   return 0;
}

//---------------------------------------------------------------------------

static void handshake_cb( uintptr_t psv, const uint8_t* fingerprint ){
	PMYDATAPATH pdp = (PMYDATAPATH)psv;
	// check, save, compare fingerprint (12 bytes?)
}

static void ssh_shell_opened( uintptr_t psv, LOGICAL success ){
	PMYDATAPATH pdp = (PMYDATAPATH)psv;
}

static void ssh_pty_opened( uintptr_t psv, LOGICAL success ){
	PMYDATAPATH pdp = (PMYDATAPATH)psv;
	sack_ssh_set_shell_open( pdp->channel, ssh_shell_opened );
}


static uintptr_t channel_open_cb( uintptr_t psv, struct ssh_channel* channel ){
	PMYDATAPATH pdp = (PMYDATAPATH)psv;
	sack_ssh_channel_request_pty( pdp->channel, "vt-100", ssh_pty_opened );
}

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pdp = CreateDataPath( pChannel, MYDATAPATH );
	pdp->common.Type = iSSH;
	pdp->common.Read = Read;
	pdp->common.Write = Write;
	pdp->common.Close = Close;
	pdp->session = sack_ssh_session_init( (uintptr_t)pdp );
	sack_ssh_session_connect( pdp->session, parameters[0], parameters[1], handshake_cb );
	sack_ssh_set_channel_open( pdp->session, channel_open_cb );

	pdp->vt = VarTextCreate();
	return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PRELOAD( RegisterRoutines ) // PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	if( DekwareGetCoreInterface( DekVersion ) ) {
		iSSH = RegisterDevice( "ssh", "Processes ssh client protocol", Open );
	}
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void )
{
	UnregisterDevice( "ssh" );
}
