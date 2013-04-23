#ifdef WIN32
#define _INCLUDE_CLIPBOARD
#endif

#if ( defined( PSICON ) && defined( _WIN32 ) ) || defined( CONSOLECON ) || defined( WINCON )
#include <stdhdrs.h>
#include "regaccess.h"
#include "WinLogic.h"
#include "consolestruc.h"
int KeystrokePaste( PCONSOLE_INFO pdp )
{
    if( OpenClipboard(NULL) )
    {
        _32 format;
        // successful open...
        format = EnumClipboardFormats( 0 );
        while( format )
        {
            //DECLTEXT( msg, "                                     " );
            //msg.data.size = sprintf( msg.data.data, "Format: %d", format );
            //EnqueLink( pdp->ps->Command->ppOutput, SegDuplicate( (PTEXT)&msg ) );
#ifndef CF_TEXT
#define CF_TEXT 1
#endif
            if( format == CF_TEXT )
            {
                HANDLE hData = GetClipboardData( CF_TEXT );
                LPVOID pData = GlobalLock( hData );
                PTEXT pStroke = SegCreateFromText( pData );
                int ofs, n;
                GlobalUnlock( hData );
                n = ofs = 0;
                while( pStroke->data.data[n] )
                {
                    pStroke->data.data[ofs] = pStroke->data.data[n];
                    if( pStroke->data.data[n] == '\r' ) // trash extra returns... keep newlines
                    {
                        n++;
                        continue;
                    }
                    else
                    {
                        ofs++;
                        n++;
                    }           
                }
                pStroke->data.size = ofs;
                pStroke->data.data[ofs] = pStroke->data.data[n];
                if( PSI_DoStroke( pdp, pStroke ) ) 1;
                //   RenderCommandLine( pdp );
                //EnqueLink( pdp->ps->Command->ppOutput, SegDuplicate(pStroke) );
                LineRelease( pStroke );
                break;
            }
            format = EnumClipboardFormats( format );
        }
        CloseClipboard();
    }
    else
	 {
#ifdef __DEKWARE__PLUGIN__
        DECLTEXT( msg, "Clipboard was not available" );
		  EnqueLink( &pdp->common.Owner->Command->Output, &msg );
#endif
    }
    return 0;

}

#endif
