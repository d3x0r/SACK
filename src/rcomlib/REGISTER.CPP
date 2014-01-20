#include <windows.h>

#include "rcomlib.h"

#import "M:\regserver\regclient\debug\regclient.dll"  \
            no_namespace

#include "sql_support.h" // also loggin support...

IRegisterPtr pRegister;
IRegister *iRegister;

// interface to register software...

extern "C" BOOL OpenRegister( void )
{
   CoInitialize( NULL );

   hr = pRegister.CreateInstance( _uuidof( Register ) );
   LOGERROR(hr);
   if( hr ) return FALSE;

   
   hr =( pRegister.QueryInterface( _uuidof( IRegister ), &iRegister ));
   LOGERROR(hr);
   if( hr ) return FALSE;

   iRegister->put_PerUser(FALSE);
   iRegister->raw_SetCompany_C( 1, WIDE("ADA Software Development") );

   return TRUE;
}

extern "C" void ShowRegister( void )
{
   iRegister->put_Visible( TRUE );
}

extern "C" int IsRegistered( char *pProgramID, char *pProgramName, BOOL bSilent )
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
         if( !bSilent && SimpleQuery( WIDE("Please enter your E-Mail address to register this"), pMail, sizeof( pMail ) ) )
         {
            iRegister->raw_SetClientName_C( pMail );
            return 1;
         }
         else
         {
            if( !bSilent )
               MessageBox( NULL, WIDE("You will not be able to use this product if you do not enter a name."), WIDE("Registration Incomplete"), MB_OK );
            return 3;
         }
      }
      else if( i == 1 )
      {
         if( !bSilent )
            MessageBox( NULL, WIDE("This product's evaluation period has expired.  ")
                           "Please contact it's manufacturers to find out how to get an unlimited version."
                           "Thank you for your continued use of this software", 
                           "Registration Expired", MB_OK );
         return 2;
      }
      else if( i == 2 )
      {
         if( !bSilent )
            MessageBox( NULL, WIDE("This product has found an invalid Registration code."), WIDE("Registration Error"), MB_OK );
         return 4;
      }
      else
         return 1;
   }
   return 0;
}

extern "C" void GetRegisterError( char *pResult )
{
   iRegister->get_ResultText_C( pResult );
}

extern "C" int DoRegister( void )
{
   int i;
   iRegister->Register();
   iRegister->get_Result( &i );
   if( i == 1400 )
      return TRUE;
   else
      return FALSE;
}

extern "C" int GetRegisterResult( void )
{
   int i;
   iRegister->get_Result( &i );
   return i;
}

extern "C" int RegisterProduct( char *pResult, char *pProductID, char *pProductName )
{
   int bOkay;
   int res;
   bOkay = TRUE;
   pResult[0] = 0;
   if( OpenRegister() )
   {
      res = IsRegistered( pProductID, pProductName, FALSE ); // returned an error...
      if( res )
      {
         //ShowRegister();  // set to show the dialog...
         if( res == 1 ) // can register - hasn't been registered..
         {
            if( !DoRegister() )
            {
               int i;
               i = GetRegisterResult();
               switch( i ) 
               {
               case -1:
                  bOkay = FALSE;
                  MessageBox( NULL, WIDE("Registration server is apparently offline, or you are not connected to the internet.  Continuing in sub-evaluation mode"), WIDE("Registration"), MB_OK  );
                  break;
               case 0: // ERROR_SUCCESS (no error)
               case 1404: // result from LIC command 1400 - 'Lic - "
               case 1400: // result from normal command - Okay
                  break;
               default:
                  {
                     BYTE byMsg[256];
                     BYTE byError[256];
                     bOkay = FALSE;
                     GetRegisterError( (char *)byError );
                     wsprintf( (char*)byMsg, WIDE("%d - %s"), i, byError );
                     MessageBox( NULL, (char*)byMsg, WIDE("Registration Error"), MB_OK );
                  }
                  break;
               }
            }
            else
               if( !IsRegistered( pProductID, pProductName, FALSE ) )
               {
                  strcpy( pResult, WIDE("Impossible to register Software.\n") );
                  bOkay = FALSE;
               }
         }
         else
            bOkay = FALSE; 
      }
   }
   else
   {
      strcpy( pResult, WIDE("Could not register program.\n") );
      bOkay = FALSE;
   }
   return bOkay;

}  