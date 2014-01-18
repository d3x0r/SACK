#define DEFINES_DEKWARE_INTERFACE
#define DO_LOGGING
#include <stdhdrs.h>
#include "plugin.h"

INDEX iTelnet;

typedef struct mydatapath_tag {
	DATAPATH common;
	struct {
		_32 bDoNAWS : 1;
		_32 bSendNAWS : 1;
		_32 bLastWasIAC : 1; // secondary IAC (255) received
	} flags;
// buffers needed for proc_iac
	TEXTCHAR iac_data[256];
	int iac_count;
	PVARTEXT vt; // collects var text....
} MYDATAPATH, *PMYDATAPATH;

///*
// * Definitions for the TELNET protocol.
// */
// 
#define  IAC   (_8)255    /* 0XFF interpret as command: */
#define  DONT  (_8)254    /* 0XFE you are not to use option */
#define  DO    (_8)253    /* 0XFD please, you use option */
#define  WONT  (_8)252    /* 0XFC I won't use option */
#define  WILL  (_8)251    /* 0XFB I will use option */
#define  SB    (_8)250    /* 0XFA interpret as subnegotiation */
#define  GA    (_8)249    /* 0XF9 you may reverse the line */
#define  EL    (_8)248    /* 0XF8 erase the current line */
#define  EC    (_8)247    /* 0XF7 erase the current character */
#define  AYT   (_8)246    /* 0XF6 are you there */
#define  AO    (_8)245    /* 0XF5 abort output--but let prog finish */
#define  IP    (_8)244    /* 0XF4 interrupt process--permanently */
#define  IACBK (_8)243    /* 0XF3 break */
#define  DM    (_8)242    /* 0XF2 data mark--for connect. cleaning */
#define  NOP   (_8)241    /* 0XF1 nop */
#define  SE    (_8)240    /* 0XF0 end sub negotiation */
#define  EOR   (_8)239    /* 0XEF end of record (transparent mode) */

#define DO_TB                 0  //*856  do transmit binary 
#define DO_ECHO               1  //*857
#define DO_SGA                3  //*858 supress go ahead
#define DO_STATUS             5  //*859
#define DO_TIMINGMARK         6  //*860
#define DO_BM                19  // 735 Telent Byte Macro
#define DO_DET               20  // 1043 Data Entry Terminal
#define DO_TERM_TYPE         24  //*884/930/1091
#define DO_EOR               25  // 885 end of record
#define DO_TUID              26  // 927 TACACS User Identification 
#define DO_OUTMRK            27  // 933 Output Marking 
#define DO_TTYLOC            28  // 946 Telnet Terminal Location
#define DO_3270_REGIME       29  // 1041 Telnet 3270 Regime Option
#define DO_X3_PAD            30  // 1053 
#define DO_NAWS              31  //*1073 Negotiate About Window Size
#define DO_TERMSPEED         32  //*1079 Set terminal line speed
#define DO_FLOW_CONTROL      33  //?1372   - xon/xoff
#define DO_LINEMODE          34  //?1184
#define DO_X_DISPLAY_LOCATION 35 // 1986 send back my name for other xwindows clients
#define DO_AUTHENTICATION    37  // 1409 - IS/SEND/REPLY/NAME
#define DO_SUPDUP            21   // 736
#define DO_NEW_ENVIRON       39   // 1529

#define DO_COMPRESS          85  // mud client zlib compression on stream

#define DO_EXOPL            255  //*861 do extended option list
#define DO_RANDOMLOSE       256  // 748 option to gernerate random data loss
#define DO_SUBLIMINAL       257  // 1097 

#define MAX_DO_S ( sizeof( DoText ) / sizeof( struct do_text_tag ) )

struct do_text_tag 
{
	_8 dovalue;
	TEXTSTR dotext;
} DoText [] = {
	{DO_TB, WIDE("TB")}
	,{DO_ECHO, WIDE("ECHO")}
	,{DO_SGA, WIDE("SGA")}
	,{DO_STATUS, WIDE("STATUS")}
	,{DO_TIMINGMARK, WIDE("TIMINGMARK")}
	,{DO_BM, WIDE("BM")}
	,{DO_DET, WIDE("DET")}     
	,{DO_TERM_TYPE, WIDE("TERM_TYPE")}         
	,{DO_EOR, WIDE("EOR")}
	,{DO_TUID, WIDE("TUID")}
	,{DO_OUTMRK, WIDE("OUTMRK")}
	,{DO_TTYLOC, WIDE("TTYLOC")}  
	,{DO_3270_REGIME, WIDE("3270_REGIME")}      
	,{DO_X3_PAD, WIDE("X3_PAD")}          
	,{DO_NAWS, WIDE("NAWS")}
	,{DO_TERMSPEED, WIDE("TERMSPEED")}
	,{DO_FLOW_CONTROL, WIDE("FLOW_CONTROL")}
	,{DO_LINEMODE, WIDE("LINEMODE")}
	,{DO_X_DISPLAY_LOCATION, WIDE("X_DISPLAY_LOCATION")}
	,{DO_AUTHENTICATION, WIDE("AUTHENTICATION")}
	,{DO_SUPDUP, WIDE("SUPDUP")}
	,{DO_NEW_ENVIRON, WIDE("NEW_ENVIRON")}

	,{DO_COMPRESS, WIDE("COMPRESS")}

	,{DO_EXOPL, WIDE("EXOPL")}
	,{(TEXTCHAR)DO_RANDOMLOSE, WIDE("RANDOMLOSE")}
	,{(TEXTCHAR)DO_SUBLIMINAL, WIDE("SUBLIMINAL")}};
	
#define TWECHO     1
#define BREAKEVNT  2
static _32 status;

// this is used below during processing IAC codes
// it's a relic from obsolete code... but it may
// contain some hidden meaning that was missed....
//static int wwdd_val; // unsure of this value
//---------------------------------------------------------------------------

static int proc_iac(PMYDATAPATH pmdp, TEXTCHAR tchar)
{
	// 0 data is just incomming use it
	// 1 we used the data don't do any thing with it
	// 3 send data....
	TEXTCHAR i;
	TEXTCHAR retval=0;   /* assume we will use the character */

	/* don't start if not telnet and not an IAC character string */
	/* also need to filter out double IAC into a single IAC */
	if( tchar == IAC )
	{
		if( !pmdp->iac_count )
		{
			// start command...
			pmdp->iac_data[0]=tchar;
			pmdp->iac_count = 1;
			pmdp->flags.bLastWasIAC = 0;
			return 1;
		}
		if( !pmdp->flags.bLastWasIAC )
		{
			// receive 1, delete it....
			pmdp->flags.bLastWasIAC = 1;
			return 1;
		}
		// second IAC - continue on and process it.
		// and is in collection of IAC code...
		pmdp->flags.bLastWasIAC = 0;
	}

	if(pmdp->iac_count > 0)
	{
		retval = 1;
		if(pmdp->iac_count>255)
			pmdp->iac_count=255;
		//Log1( WIDE("Added byte: %02x"), tchar );
		pmdp->iac_data[pmdp->iac_count++]=tchar;

      switch(pmdp->iac_data[1])
		{
		case IAC:
			// displayln(traceout,WIDE("double iac on %hd\n"),Channel);
			pmdp->iac_count=0;
			retval=0; /* return the character is yours */
			goto done;
		case DO:
			if(pmdp->iac_count<3)break; /* get entire option request */
			// displayln(traceout,WIDE("DO   %n "),pmdp->iac_data[2]);
			if(pmdp->iac_data[2]== DO_ECHO )      /* ECHO */
			{
				if(status&TWECHO)
				{
					// displayln(traceout,WIDE("no response %n \n"),pmdp->iac_data[2]);
					goto processed;/* ECHO IS CORRECT DONT RESPOND */
				}
				status|= TWECHO;
			reply_will:
				pmdp->iac_data[1]=WILL;
				goto send_iac;
			}
			else if(pmdp->iac_data[2]== DO_SGA ) /* SGA  */
			{
				goto reply_will;
			}
			else if( pmdp->iac_data[2] == DO_NAWS )
			{
				pmdp->flags.bDoNAWS = 1;
				pmdp->flags.bSendNAWS = 1;
				goto reply_will;
			}
			else
			{
				int n;
				for( n = 0; n < MAX_DO_S; n++ )
				{
					if( pmdp->iac_data[2] == DoText[n].dovalue )
					{
						Log1( WIDE("Server request for DO %s"), DoText[n].dotext );
						break;
					}
				}
				if( n == MAX_DO_S )
					Log1( WIDE("Server request for unknown DO: %02X"), pmdp->iac_data[2] );
			}
			goto sendwont;

		case DONT:
			if(pmdp->iac_count<3)break; /* get entire option request */
			// displayln(traceout,WIDE("DONT %n "),pmdp->iac_data[2]);
			if(pmdp->iac_data[2]==DO_ECHO)           /* ECHO */
			{
				if( (status&TWECHO) ==0)
				{
					// displayln(traceout,WIDE("no response %n \n"),pmdp->iac_data[2]);
					goto processed;/* ECHO IS CORRECT DONT RESPOND */
				}
				status&=~TWECHO;
			}
			else if(pmdp->iac_data[2]==DO_SGA)           /* SGA  */
			{
				goto reply_will;
			}
			else
			{
				int n;
				for( n = 0; n < MAX_DO_S; n++ )
				{
					if( pmdp->iac_data[2] == DoText[n].dovalue )
					{
						Log1( WIDE("Server request for DONT %s"), DoText[n].dotext );
						break;
					}
				}
				if( n == MAX_DO_S )
					Log1( WIDE("Server request for unknown DONT: %02X"), pmdp->iac_data[2] );
			}
		sendwont:
			pmdp->iac_data[1]=WONT;
			goto send_iac;

		case WILL:
			if(pmdp->iac_count<3)break; /* get entire option request */
			// displayln(traceout,WIDE("WILL %n "),pmdp->iac_data[2]);
			if( pmdp->iac_data[2] == DO_SGA )
			{
				pmdp->iac_data[1] = DO;
				goto send_iac;
			}
			goto senddont;
 
		case WONT:
			if(pmdp->iac_count<3)break; /* get entire option request */
			// displayln(traceout,WIDE("WONT %n "),pmdp->iac_data[2]);

		senddont:
			pmdp->iac_data[1]=DONT;

		send_iac:

			//if(*((short*)(&pmdp->iac_data[1])) == wwdd_val)
			//{
			//	goto processed;  /* don't resend anything*/
			//}

			//wwdd_val=*((short*)(&pmdp->iac_data[1]));

			i=pmdp->iac_data[1];

			switch(i)
			{
			case DO:
				Log( WIDE("response DO   "));
				break;
			case DONT:
				Log( WIDE("response DONT "));
				break;
			case WILL:
				Log( WIDE("response WILL "));
				break;
			case WONT:
				Log( WIDE("response WONT "));
				break;
			}
			//Log1(WIDE("command data byte... %02d"),pmdp->iac_data[2]);

			retval=3; /* we used the character and we need a write */
			goto processed;

		case SB:
			if(tchar==SE)goto unknown;
			break;

		case IACBK:
			status |= BREAKEVNT;
			// displayln(traceout,WIDE("iac break\n"));
			goto processed;

		case NOP:
			// displayln(traceout,WIDE("iac nop\n"));
			goto processed;
 
		case GA:
		case EL:
		case EC:
		case AYT:
		case AO:
		case IP:
		case DM:
		case SE:
		case EOR:
		default:
		unknown:

			// displayln(traceout,WIDE("iac on %hd ="),Channel);
			// for (i=0;i<pmdp->iac_count;i++)
			//   displayln(traceout,WIDE(" %hd"),pmdp->iac_data[i]);
			// displayln(traceout,WIDE("\n"));

		processed:
			pmdp->iac_count=0;
		}
		goto done;
	}

done:
	return(retval);
}

//---------------------------------------------------------------------------

static PTEXT CPROC TelnetHandle( PDATAPATH pdpCommon, PTEXT pText )
{
   PMYDATAPATH pdp = (PMYDATAPATH)pdpCommon;
   INDEX idx;
   int nState;
   TEXTCHAR *ptext;
   PTEXT save;
   save = pText;
   nState = 0;
	//Log1( WIDE("Telnet input... %s"), GetText( pText ) );
   for( save = pText; pText; pText = NEXTLINE(pText) )
   {
      ptext = GetText( pText );
      {
         for( idx = 0; idx < pText->data.size && ptext[idx]; idx++ )
         {
            switch( proc_iac( pdp, ptext[idx] ) )
            {
            case 0:
					VarTextAddCharacter( pdp->vt, ptext[idx] );
               break;
            case 3:
            	{
            		PTEXT responce = SegCreate( 3 );
            		MemCpy( responce->data.data, pdp->iac_data, 3 );
						responce->flags |= TF_BINARY;
						//Log3( WIDE("Enquing a responce... %02x %02x %02x")
						//	 , pdp->iac_data[0]
						//	 , pdp->iac_data[1]
						//	 , pdp->iac_data[2]  );
            		EnqueLink( &pdp->common.Output, responce );
						pdp->iac_count = 0;

						// also send initial packets....
						if( pdp->flags.bSendNAWS )
						{
							PTEXT send = SegCreate( 9 );
							TEXTCHAR *out = GetText( send );
							PTEXT val;
							int tmp;
							send->flags |= TF_BINARY;
							out[0] = IAC;
							out[1] = SB;
							out[2] = DO_NAWS;
							val = GetVolatileVariable( pdp->common.Owner->Current, WIDE("cols") );
							if( val )
								tmp = atoi( GetText( val ) );
							else
								tmp = 80;
							out[3] = ( tmp & 0xFF00 ) >> 8;
							out[4] = tmp & 0xFF;

							val = GetVolatileVariable( pdp->common.Owner->Current, WIDE("rows") );
							if( val )
								tmp = atoi( GetText( val ) );
							else
								tmp = 25;
							out[5] = ( tmp & 0xFF00 ) >> 8;
							out[6] = tmp & 0xFF;

							out[7] = IAC;
							out[8] = SE;
							EnqueLink( &pdp->common.Output, send );
							pdp->flags.bSendNAWS = 0;
						}
	            }
					break;
            case 1:
            	// should totally snag the character...
               // ptext[idx] = ' ';
               break;
            }
         }
      }
   }
	LineRelease( save );
   return VarTextGet( pdp->vt );
}

//---------------------------------------------------------------------------

static int CPROC Read( PDATAPATH pdp )
{
	return RelayInput( pdp, TelnetHandle );
}

//---------------------------------------------------------------------------

static PTEXT CPROC ValidateEOL( PDATAPATH pdpCommon, PTEXT line )
{
   //PMYDATAPATH pdp = (PMYDATAPATH)pdpCommon;
	PTEXT end, check;
	// outbound return is added at the end of the line.
	// so if there is no return, don't add one...
	if( !line || ( line->flags & TF_NORETURN ) )
		return line;
	end = line;
	SetEnd( end );
	check = end;
	while( check && ( check->flags & TF_INDIRECT ) )
	{
		check = GetIndirect( check );
	}
	// if check == NULL the last segment is a indirect with 0 content...

	if( !check ||
	    GetTextSize( end ) && !(end->flags&TF_BINARY) )
	{
		//PTEXT junk = BuildLine( line );
		//Log1( WIDE("Adding a end of line before shipping to network... %s"), GetText( junk ) );
		//LineRelease( junk );
		SegAppend( end, SegCreate( 0 ) );
	}	
	return line;
}

//---------------------------------------------------------------------------

static int CPROC Write( PDATAPATH pdp )
{
	return RelayOutput( pdp, ValidateEOL );
}

//---------------------------------------------------------------------------

static int CPROC Close( PDATAPATH pdpCommon )
{
   PMYDATAPATH pdp = (PMYDATAPATH)pdpCommon;
	VarTextDestroy( &pdp->vt );
   pdp->common.Type = 0;
   return 0;
}

//---------------------------------------------------------------------------

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pdp = CreateDataPath( pChannel, MYDATAPATH );
	pdp->common.Type = iTelnet;
	pdp->common.Read = Read;
	pdp->common.Write = Write;
	pdp->common.Close = Close;
	pdp->vt = VarTextCreate();
	return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	iTelnet = RegisterDevice( WIDE("telnet"), WIDE("Processes telnet IAC sequences"), Open );
   return DekVersion;
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void )
{
	UnregisterDevice( WIDE("telnet") );
}

// $Log: telnet.c,v $
// Revision 1.18  2005/02/21 12:08:40  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.17  2005/01/28 16:02:19  d3x0r
// Clean memory allocs a bit, add debug/logging option to /memory command,...
//
// Revision 1.16  2005/01/18 02:47:01  d3x0r
// Mods to protect symbols from overwrites.... (declare much things static, rename others)
//
// Revision 1.15  2003/11/08 00:09:41  panther
// fixes for VarText abstraction
//
// Revision 1.14  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.13  2003/04/20 01:20:13  panther
// Updates and fixes to window-like console.  History, window logic
//
// Revision 1.12  2003/04/02 06:43:27  panther
// Continued development on ANSI position handling.  PSICON receptor.
//
// Revision 1.11  2003/03/31 14:47:05  panther
// Minor mods to filters
//
// Revision 1.10  2003/03/25 08:59:02  panther
// Added CVS logging
//
