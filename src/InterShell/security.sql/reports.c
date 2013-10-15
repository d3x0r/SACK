//
//   login_monitor.c
//   (C) Copyright 2009 
//   Crafted by d3x0r
//                   
////////////////////////////////////////////////////////////////////////////

#define DEFINE_DEFAULT_IMAGE_INTERFACE
#ifndef __LINUX__
//#include <futcal.h>
//#include <comnprnt.h>
#include <pssql.h>
//#include <futgetpr.h>
#define USES_INTERSHELL_INTERFACE
#include "../intershell_export.h"
#include "../intershell_registry.h"

#include "global.h"

//--------------------------------------------------------------------

OnKeyPressEvent( "SQL Password/Users/User Report" )( PTRSZVAL psv )
{
	HDC printer = GetPrinterDC(1);
	int n;
	char szString[256];
	struct {
		_16 wYr, wMo, wDy, wHr, wMn, wSc;
	} time;

	PUSER user;
   _32 now = CAL_GET_FDATETIME();
	ReloadUserCache( NULL );
	FontFromColumns( printer, NULL, NULL, 100, NULL );
	//ReadPasswordFile();
   ClearReportHeaders();
	CAL_P_YMDHMS_OF_FDATETIME( now, &time.wYr, &time.wMo, &time.wDy, &time.wHr, &time.wMn, &time.wSc );
	snprintf( szString, sizeof( szString ), WIDE("Active User Report")
			  , time.wMo, time.wDy, time.wYr);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), WIDE("Printed at %d:%02d:%02d on %02d/%02d/%02d")
			  , time.wHr, time.wMn, time.wSc
			  , time.wMo, time.wDy, time.wYr%100);
	AddReportHeader( szString );
	AddReportHeader( WIDE("") );

	if( g.flags.bPrintAccountCreated )
	{
		AddReportHeader( "User Name            Group             Updated     Expires     Created" );
		//               "20characternamegoesh Supervisor Group  03/03/1009  02/01/1008
		AddReportHeader( "____________________ _________________ ___________ ___________ ___________\n" );
	}
	else
	{
		AddReportHeader( "User Name            Group             Updated     Expires    " );
		//               "20characternamegoesh Supervisor Group  03/03/1009  02/01/1008
		AddReportHeader( "____________________ _________________ ___________ ___________\n" );
	}
	PrintReportHeader( printer, -1, -1 ); // current position, write haeder...
   LIST_FORALL( g.users, n, PUSER, user )
	//for( n = 0; n < ; n++ )
	{
		int m;
		struct {
			_16 wYr, wMo, wDy, wHr, wMn, wSc;
		} expire;
		struct { 
			_16 wYr, wMo, wDy, wHr, wMn, wSc;
		} created;
		struct { 
			_16 wYr, wMo, wDy, wHr, wMn, wSc;
		} password_updated;
		char buf[256];

		{
			char *date;
			char datebuf[13];
			char *exp_date;
			char exp_datebuf[13];
			char *upd_date;
			char upd_datebuf[13];
			_32 now = CAL_GET_FDATETIME();
			if( user->dwFutTime > now )
			{
				CAL_P_YMDHMS_OF_FDATETIME( user->dwFutTime, &expire.wYr, &expire.wMo, &expire.wDy, &expire.wHr, &expire.wMn, &expire.wSc );
				snprintf( exp_datebuf, sizeof( exp_datebuf ), "%02d/%02d/%04d" 
						, expire.wMo, expire.wDy, expire.wYr );
				exp_date = exp_datebuf;
			}
			else
				exp_date = "Expired   ";
			if( user->dwFutTime_Created )
			{
				CAL_P_YMDHMS_OF_FDATETIME( user->dwFutTime_Created, &created.wYr, &created.wMo, &created.wDy, &created.wHr, &created.wMn, &created.wSc );
				snprintf( datebuf, sizeof( datebuf ), "%02d/%02d/%04d" 
						, created.wMo, created.wDy, created.wYr );
				date = datebuf;
			}
			else
				date = "Before Now";
			if( user->dwFutTime_Updated_Password )
			{
				CAL_P_YMDHMS_OF_FDATETIME( user->dwFutTime_Updated_Password, &password_updated.wYr, &password_updated.wMo, &password_updated.wDy, &password_updated.wHr, &password_updated.wMn, &password_updated.wSc );
				snprintf( upd_datebuf, sizeof( upd_datebuf ), "%02d/%02d/%04d"
						, password_updated.wMo, password_updated.wDy, password_updated.wYr );
				upd_date = upd_datebuf;
			}
			else
				upd_date = "Before Now";

			{
				INDEX idx;
            PGROUP group;
				LIST_FORALL( user->groups, idx, PGROUP, group )
				{
					//for( m = 0; m < _PASSWORD_ACCESS_LEVELS; m++ )
			//if( g.file[0].users[n].fAcc[m] )
					{
						snprintf( buf, sizeof( buf ), "%-20.20s %-16.16s  %s  %s  %s\n"
								  , user->name
								  , group->name
								  , upd_date
								  , exp_date
								  , g.flags.bPrintAccountCreated?date:""
								  );
						PrintString( buf );
						break;
					}
				}
				if( !group )//m == _PASSWORD_ACCESS_LEVELS )
				{

					snprintf( buf, sizeof( buf ), "%-20.20s %-16.16s  %s  %s  %s\n"
							  , user->name
							  , "No Permission"
							  , upd_date
							  , exp_date
							  , g.flags.bPrintAccountCreated?date:""
							  );
					PrintString( buf );
				}
			}
		}
	}
	ClosePrinterDC( printer );
}

OnCreateMenuButton( "SQL Password/Users/User Report" )( PMENU_BUTTON button )
{
	MILK_SetButtonStyle( button, "bicolor square" );
	MILK_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_PURPLE, BASE_COLOR_BLACK, 0 );
	MILK_SetButtonText( button, WIDE("User_Report") );
	return 1;
}

//--------------------------------------------------------------------

OnKeyPressEvent( "SQL Password/Users/History Report" )( PTRSZVAL psv )
{
	HDC printer = GetPrinterDC(0);
	//int n;
	char szString[256];
	struct {
		_16 wYr, wMo, wDy, wHr, wMn, wSc;
	} time;
	struct {
		_16 wYr, wMo, wDy, wHr, wMn, wSc;
	} report_from;
	//struct {
	//	_16 wYr, wMo, wDy, wHr, wMn, wSc;
	//} report_to;
	_32 now = CAL_GET_FDATETIME();
   ReloadUserCache( NULL );
	FontFromColumns( printer, NULL, NULL, 140, NULL );
   ClearReportHeaders();

	CAL_P_YMDHMS_OF_FDATETIME( now, &time.wYr, &time.wMo, &time.wDy, &time.wHr, &time.wMn, &time.wSc );
	CAL_P_YMDHMS_OF_FDATETIME( now - (100*(24*60*60))
									 , &report_from.wYr, &report_from.wMo, &report_from.wDy, NULL, NULL, NULL );
	snprintf( szString, sizeof( szString ), WIDE("User History Report")
			  , time.wMo, time.wDy, time.wYr);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), WIDE("Printed at %d:%02d:%02d on %02d/%02d/%02d")
			  , time.wHr, time.wMn, time.wSc
			  , time.wMo, time.wDy, time.wYr%100);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), WIDE("Report from %02d/%02d/%02d to %02d/%02d/%02d")
			  , report_from.wMo, report_from.wDy, report_from.wYr%100
			  , time.wMo, time.wDy, time.wYr%100
			  );
	AddReportHeader( szString );
	AddReportHeader( WIDE("") );

	AddReportHeader( "Time                UserName             Event              Message" );
	//               "00/00/0000 00:00:00 
	//               "20characternamegoesh Supervisor Group  03/03/1009  02/01/1008
	AddReportHeader( "___________________ ____________________ __________________ ______________________________________\n" );
	PrintReportHeader( printer, -1, -1 ); // current position, write haeder...
	if( g.flags.bUseSQL )
	{
		static char query[512];
		CTEXTSTR *result = NULL;
		snprintf( query, sizeof( query )
				 , "select user_event_log_timestamp,description,event_type,user_name"
				  " from user_event_log"
				  " where user_event_log_timestamp>=%04d%02d%02d and user_event_log_timestamp<=%04d%02d%02d235959"
				" and ( event_type='Password Update'"
				" or event_type='Delete User'"
				" or event_type='Create User'"
				" or event_type='Expire Password'"
				  " or event_type='') order by user_event_log_timestamp"
              , report_from.wYr
              , report_from.wMo
              , report_from.wDy
              , time.wYr
              , time.wMo
              , time.wDy );
		for( DoSQLRecordQuery( query, NULL, &result, NULL )
			; result
			; GetSQLRecord( &result ) )
		{
			char buf[120];
			sprintf( buf, "%s %-20.20s %-18.18s %s\n", result[0], result[3], result[2], result[1] );
			PrintString( buf );
		}
	}
	else
	{
      PrintString( "SQL Not enabled, user history not tracked." );
	}
	ClosePrinterDC( printer );

}

OnCreateMenuButton( "SQL Password/Users/History Report" )( PMENU_BUTTON button )
{
	MILK_SetButtonStyle( button, "bicolor square" );
	MILK_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_PURPLE, BASE_COLOR_BLACK, 0 );
	MILK_SetButtonText( button, WIDE("User_History") );
	return 1;
}

//--------------------------------------------------------------------

OnKeyPressEvent( "SQL Password/Users/Permission Report" )( PTRSZVAL psv )
{
	HDC printer = GetPrinterDC(1);
	//int n;
	char szString[256];
	struct {
		_16 wYr, wMo, wDy, wHr, wMn, wSc;
	} time;
	struct {
		_16 wYr, wMo, wDy, wHr, wMn, wSc;
	} report_from;
	//struct {
	//	_16 wYr, wMo, wDy, wHr, wMn, wSc;
	//} report_to;
	_32 now = CAL_GET_FDATETIME();
   ReloadUserCache( NULL );
	FontFromColumns( printer, NULL, NULL, 112, NULL );
   ClearReportHeaders();

	CAL_P_YMDHMS_OF_FDATETIME( now, &time.wYr, &time.wMo, &time.wDy, &time.wHr, &time.wMn, &time.wSc );
	CAL_P_YMDHMS_OF_FDATETIME( now - (60*(24*60*60))
									 , &report_from.wYr, &report_from.wMo, &report_from.wDy, NULL, NULL, NULL );
	snprintf( szString, sizeof( szString ), WIDE("User Permission Report")
			  , time.wMo, time.wDy, time.wYr);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), WIDE("Printed at %d:%02d:%02d on %02d/%02d/%02d")
			  , time.wHr, time.wMn, time.wSc
			  , time.wMo, time.wDy, time.wYr%100);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), WIDE("Report from %02d/%02d/%02d to %02d/%02d/%02d")
			  , report_from.wMo, report_from.wDy, report_from.wYr%100
			  , time.wMo, time.wDy, time.wYr%100
			  );
	AddReportHeader( szString );
	AddReportHeader( WIDE("") );


   ClearReportHeaders();

	CAL_P_YMDHMS_OF_FDATETIME( now, &time.wYr, &time.wMo, &time.wDy, &time.wHr, &time.wMn, &time.wSc );
	CAL_P_YMDHMS_OF_FDATETIME( now - (60*(24*60*60))
									 , &report_from.wYr, &report_from.wMo, &report_from.wDy, NULL, NULL, NULL );
	snprintf( szString, sizeof( szString ), WIDE("User Permission Report")
			  , time.wMo, time.wDy, time.wYr);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), WIDE("Printed at %d:%02d:%02d on %02d/%02d/%02d")
			  , time.wHr, time.wMn, time.wSc
			  , time.wMo, time.wDy, time.wYr%100);
	AddReportHeader( szString );
	AddReportHeader( WIDE("") );

	AddReportHeader( "Group                Permission      " );
	//               "00/00/0000 00:00:00 
	//               "20characternamegoesh Supervisor Group  03/03/1009  02/01/1008
	AddReportHeader( "____________________ __________________\n" );
	PrintReportHeader( printer, -1, -1 ); // current position, write haeder...


	{
		int first;
		int group, n;
      PGROUP sql_group;
		int report_charities = FutGetProfileInt( "PASSWORDS", "Report Charity Permissions", 0 );
      LIST_FORALL( g.groups, n, PGROUP, sql_group )
		//for( group = 0; group < _PASSWORD_ACCESS_LEVELS; group++ )
		{
			int invalid = 0;
         INDEX idx;
         PTOKEN token;
			first = 1;
         LIST_FORALL( sql_group->tokens, idx, PTOKEN, token )
			//if( g.file[0].AccessDesc[group][0] )
				//r( n = 0; permission_names[n]; n++ )
				{
					//( g.file[0].access[group][n].byte )
					{
						char line[80];
						invalid = 0;
						if( token->id >= 16 )
							continue;
                  // don't report charity permissions (meaningless)
						if( !report_charities )
							if( token->id >= 10 && token->id <= 16 )
                        continue;
						if( n == 7 )
						{
							if( !FutGetProfileInt( "PASSWORDS", "Allow Edit User", 0 ) )
                        invalid = 1;
						}
                  // filter out ROOT and AIMS permissions
						if( token->id >= 17 )
                     continue;
						if( group && first )
						{
                     PrintString( "\n" );
							PrintString( "\n" );
						}
						snprintf( line, sizeof( line ), "%-20.20s %s %s\n"
								  , first?sql_group->name:""
								  , token->name
								  , invalid?"[N/A]":"" );
						PrintString( line );
						first = 0;
					}
				}
		}
      PrintString( "\n" );
      PrintString( "\n" );
		PrintString( "Meanings of Permissions above" );
      PrintString( "___________________________________\n" );
		PrintString( "Cashier           - General access to the POS for sales.\n" );
		PrintString( "Manager Options   - Allows access to the Manager Options Screen for reportintime.\n" );
		PrintString( "Z-out             - Allows user to close out/Finalize user on the POS.\n" );
		PrintString( "Configure Buttons - Allows the user to configure buttons available to sell.  Add/Delete/Modify.\n" );
		PrintString( "Edit Taxes        - The POS has support for computing taxes on items.  This option allows the modification\n" );
		PrintString( "                    of these tax rates, if applicable\n" );
		PrintString( "Void              - User is able to void a transaction.\n" );
		PrintString( "Configure Options - Allows the user to change opens in the Manger Option screen.\n" );
		PrintString( "Edit Groups       - If group/user editing is enabled, this user may change the permissions for a group.\n" );
		PrintString( "                    (Modify the above report data)\n" );
		PrintString( "Remove Winners    - Under certain configurations, the payouts are presented in a different format, this\n" );
		PrintString( "                    permission allows the user to no-pay a winner.\n" );
		PrintString( "\n" );
      PrintString( "[N/A] Marks a permission as meaningless based on system configuration.\n" );
	}
	ClosePrinterDC( printer );

}

OnCreateMenuButton( "SQL Password/Users/Permission Report" )( PMENU_BUTTON button )
{	
	MILK_SetButtonStyle( button, "bicolor square" );
	MILK_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_PURPLE, BASE_COLOR_BLACK, 0 );
	MILK_SetButtonText( button, WIDE("User_History") );
	return 1;
}
#endif
