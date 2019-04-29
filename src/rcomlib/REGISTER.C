#include <windows.h>

#include "rcomlib.h"

#import "M:\regserver\regclient\debug\regclient.dll"  \
            no_namespace

#include "sql_support.h" // also loggin support...

IRegisterPtr pRegister;
IRegister *iRegister;

// interface to register software...

BOOL OpenRegister( void )
{
   CoInitialize( NULL );

   hr = pRegister.CreateInstance( _uuidof( Register ) );
   LOGERROR(hr);
   if( hr ) return FALSE;

   
   hr =( pRegister.QueryInterface( _uuidof( IRegister ), &iRegister ));
   LOGERROR(hr);
   if( hr ) return FALSE;

   iRegister->put_PerUser(FALSE);
   iRegister->raw_SetCompany_C( 1, "ADA Software Development" );

   return TRUE;
}

void ShowRegister( void )
{
   iRegister->put_Visible( TRUE );
}

BOOL IsRegistered( char *pProgramID, char *pProgramName )
{
   int r;
   char pMail[256];
   iRegister->raw_SetProduct_C( pProgramID, pProgramName );
   iRegister->get_IsRegistered( &r );
   if( !r )
   {
      int i;
      iRegister->get_Result( &i );
      if( i == 4 )
      {
         if( SimpleQuery( "Please enter your E-Mail address to register this", pMail, sizeof( pMail ) ) )
         {
            iRegister->raw_SetClientName_C( pMail );
         }
         else
         {
            MessageBox( NULL, "You will not be able to use this product if you do not enter a name.", "Registration Incomplete", MB_OK );
            return FALSE;
         }
      }
   }
   return r;
}

void GetRegisterError( char *pResult )
{
   iRegister->get_ResultText_C( pResult );
}

int DoRegister( void )
{
   int i;
   iRegister->Register();
   iRegister->get_Result( &i );
   if( i == 1400 )
      return TRUE;
   else
      return FALSE;
}

int GetRegisterResult( void )
{
   int i;
   iRegister->get_Result( &i );
   return i;
}

int RegisterProduct( char *pResult, char *pProductID, char *pProductName )
{
   int bOkay;
   bOkay = TRUE;
   if( OpenRegister() )
   {
      if( !IsRegistered( "ADA0001", "Nettool\\Ping Control" ) )
      {
         //ShowRegister();  // set to show the dialog...
         if( !DoRegister() )
         {
            int i;
            i = GetRegisterResult();
            switch( i ) 
            {
            case -1:
               bOkay = FALSE;
               MessageBox( NULL, "Registration server is apparently offline, or you are not connected to the internet.  Continuing in sub-evaluation mode", "Registration", MB_OK  );
               break;
            default:
               {
                  BYTE byMsg[256];
                  BYTE byError[256];
                  bOkay = FALSE;
                  GetRegisterError( (char *)byError );
                  wsprintf( (char*)byMsg, "%d - %s", i, byError );
                  MessageBox( NULL, (char*)byMsg, "Registration Error", MB_OK );
               }
            }
         }
         else
            if( !IsRegistered( "ADA0001", "Nettool\\Ping Control" ) )
            {
               strcpy( pResult, "Impossible to register Software.\n" );
               bOkay = FALSE;
            }
      }
   }
   else
   {
      strcpy( pResult, "Could not register program.\n" );
      bOkay = FALSE;
   }
   return bOkay;

}  