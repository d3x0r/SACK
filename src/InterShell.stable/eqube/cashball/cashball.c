#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#define USE_IMAGE_INTERFACE cashball_local.pii
#include <stdhdrs.h>
#include <psi.h>
#include <pssql.h>
#include <sqlgetoption.h>
#include "../../intershell_registry.h"
#include "../../intershell_export.h"

static struct countdown_timer_local
{
	PIMAGE_INTERFACE pii;
	CDATA background;
	CDATA text;
	SFTFont *font;
	CTEXTSTR value;
	PLIST timers;
	TEXTCHAR database[256];
	TEXTCHAR DSN[256];
	TEXTCHAR user[256];
	TEXTCHAR password[256];
} cashball_local;

EasyRegisterControlWithBorder( "Cashball", 0, BORDER_NONE );

static void InitView()
{
	CTEXTSTR *results;
	PODBC odbc = SQLGetODBCEx( cashball_local.DSN, cashball_local.user, cashball_local.password );
	SQLCommandf( odbc, "use %s", cashball_local.database );

	if( SQLRecordQueryf( odbc, NULL, &results, NULL, "select count(*) from INFORMATION_SCHEMA.VIEWS where table_name = 'v_SessionCashball'" ) )
	{
		if( results && results[0][0] == '0' )
		{
			SQLEndQuery( odbc );
			if( !SQLCommandf( odbc, "CREATE VIEW v_SessionCashball as "
				"SELECT top 1 itemprog.pri_sunball Cashball"
				" FROM   Itemprog  inner join"
				"	  (SELECT top 1  dbo.session.ses_id, dbo.session.hallid, dbo.session.ses_date, ISNULL(RTRIM(dbo.program.prg_desc) + ' - ' + RTRIM(dbo.sesslot.slt_desc)"
				"             + CASE WHEN [Session].Ses_Test = 1 THEN ' (Test)' ELSE '' END, '') AS SessionName"
				"        FROM  dbo.session\n"
				"        INNER JOIN dbo.program ON dbo.session.prg_id = dbo.program.prg_id \n"
				"         INNER JOIN dbo.sesslot ON dbo.session.slt_id = dbo.sesslot.slt_id \n"
				"        WHERE ses_status=2"
				"        ORDER BY dbo.session.ses_id desc )\n"
				"          AS SessionTemp ON itemprog.ses_id=sessionTemp.ses_id"
				) )
			{
				CTEXTSTR error;
				FetchSQLError( odbc, &error );
				lprintf( "Error : %s", error );
			}
		}
		SQLEndQuery( odbc );
	}
	else
	{
		CTEXTSTR error;
		FetchSQLError( odbc, &error );
		lprintf( "Error : %s", error );
	}
	SQLDropODBC( odbc );
}

static void CPROC RefreshProc( uintptr_t psvTimer )
{
	CTEXTSTR *results;
	PODBC odbc = SQLGetODBCEx( cashball_local.DSN, cashball_local.user, cashball_local.password );

	SQLRecordQueryf( odbc, NULL, &results, NULL, "select Cashball from v_SessionCashball" );
	if( results && results[0] )
	{
		if( StrCmp( cashball_local.value, results[0] ) )
		{
			if( cashball_local.value )
			{
				Deallocate( TEXTSTR, cashball_local.value );
			}
			cashball_local.value = StrDup( results[0] );
			{
				INDEX idx;
				PSI_CONTROL pc;
				LIST_FORALL( cashball_local.timers, idx, PSI_CONTROL, pc )
					SmudgeCommon( pc );
			}
		}
	}
	else
	{
		if( cashball_local.value )
		{
			Deallocate( TEXTSTR, cashball_local.value );
			cashball_local.value = NULL;
			{
				INDEX idx;
				PSI_CONTROL pc;
				LIST_FORALL( cashball_local.timers, idx, PSI_CONTROL, pc )
					SmudgeCommon( pc );
			}
		}
	}
	SQLEndQuery( odbc );
	SQLDropODBC( odbc );
}

PRELOAD( InitCountdownTimer )
{
	cashball_local.pii = GetImageInterface();
	SACK_GetProfileString( GetProgramName(), "Database/DSN", "eQube", cashball_local.DSN, 256 );
	SACK_GetProfileString( GetProgramName(), "Database/user", "boss", cashball_local.user, 256 );
	SACK_GetProfileString( GetProgramName(), "Database/password", "789456", cashball_local.password, 256 );
	SACK_GetProfileString( GetProgramName(), "Database/Database", "hall", cashball_local.database, 256 );

	InitView();
	{
		DECLTEXTSZ( tmp, 32 );
		PTEXT updateable;
		SetTextSize( (PTEXT)&tmp, SACK_GetProfileString( GetProgramName(), "Cashball/Background Color", "0", GetText( (PTEXT)&tmp ), 32 ) );
		updateable = (PTEXT)&tmp;
		if( !GetColorVar( &updateable, &cashball_local.background ) )
			cashball_local.background = 0xFFFFFFFF;
		SetTextSize( (PTEXT)&tmp, SACK_GetProfileString( GetProgramName(), "Cashball/Text Color", "$FF000000", GetText( (PTEXT)&tmp ), 32 ) );
		updateable = (PTEXT)&tmp;
		if( !GetColorVar( &updateable, &cashball_local.text ) )
			cashball_local.background = 0xFF000000;
	}
	AddTimer( 2000, RefreshProc, 0 );
}

static int OnDrawCommon( "Cashball" )( PSI_CONTROL pc )
{
	Image surface = GetControlSurface( pc );
	{
		uint32_t now = GetTickCount();
		BlatColorAlpha( surface, 0, 0, surface->width, surface->height, cashball_local.background );

		{
			uint32_t w, h;
			GetStringSizeFontEx( cashball_local.value, StrLen( cashball_local.value ), &w, &h, *cashball_local.font );
			PutStringFontEx( surface, ( surface->width - w ) / 2, (surface->height - h ) / 2, cashball_local.text, 0, cashball_local.value, StrLen( cashball_local.value ), *cashball_local.font );
		}

	}
	return 1;
}

static int OnCreateCommon( "Cashball" )( PSI_CONTROL pc )
{
   SetCommonTransparent( pc, TRUE );
	AddLink( &cashball_local.timers, pc );
	return 1;
}

static uintptr_t OnCreateControl( "Cashball" )(PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h)
{
	if( !cashball_local.font )
		cashball_local.font = UseACanvasFont( parent, "Cashball Font" );
	return (uintptr_t)MakeNamedControl( parent, "Cashball", x, y, w, h, 0 );
}

static PSI_CONTROL OnGetControl( "Cashball" )(uintptr_t psv)
{
	return (PSI_CONTROL)psv;
}

