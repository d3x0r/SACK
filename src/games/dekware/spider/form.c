#include <stdhdrs.h>
#include <network.h>
#include <logging.h>

#include <sharemem.h>
#define PLUGIN_MODULE
#include "plugin.h"
#include "form.h"
#include "address.h"

#undef OutputDebugString
#define OutputDebugString(s) (fputs(s,fLog), fflush(fLog))
extern FILE *fLog;

static FORM *pFirstActive, *pLastActive;
static int nPending;

int SetPointer( char **ppc, char *orig )
{
   char *pc;
   pc = orig;
   while( pc[0] != '=' ) pc++;
   pc++;
   while( pc[0] == ' ' ) pc++; // skip leading spaces...

   if( pc[0] == '\"' ) 
   {
      pc++;
      while( pc[0] == ' ' ) pc++;
      if( ppc )
         *ppc = pc;
      while( pc[0] && pc[0] != '\"' ) pc++;
   }
   else if( pc[0] == '\'' )
   {
      pc++;
      while( pc[0] == ' ' ) pc++;
      if( ppc )
         *ppc = pc;
      while( pc[0] && pc[0] != '\'' ) pc++;
   }
   else 
   {
      while( pc[0] == ' ' ) pc++;
      if( ppc )
         *ppc = pc;
      while( pc[0] && pc[0] != ' ' ) pc++;
   }
   if( pc[0] )
   {
      pc[0] = 0;  // if is already termianted = is okay!
      pc++;
   }
   return pc - orig;
}

void ParseField( PFIELD field )
{
   int c;
   PSTR f;
   f = field->Field;
   switch( field->nType )
   {
   case FT_INPUT:
      c = 0;
      while( f[c] )
      {
         while( f[c] <= ' ' ) c++; 

         if( !strnicmp( f+c, "value", 5 ) )
         {
            c += 5;
            c += SetPointer( &field->pValue, f + c );
         }
         else if( !strnicmp( f+c, "selected", 8 ) )
         {
            c += 8;
         }
         else if( !strnicmp( f+c, "disabled", 8 ) )
         {
            c += 8;
         }
         else if( !strnicmp( f+c, "label", 5 ) )
         {
            c += 5;
            c += SetPointer( &field->pLabel, f + c );

         }
         else if( !strnicmp( f+c, "maxlength", 9 ) )
         {
            c += 9;
            c += SetPointer( NULL, f + c );
         }
         else if( !strnicmp( f+c, "type", 4 ) )
         {
            c += 4;
            c += SetPointer( &field->pType, f + c );
         }
         else if( !strnicmp( f+c, "text", 4 ) )
         {
            c += 4;
            if( !field->pValue )
               c += SetPointer( &field->pValue, f + c );
         }
         else if( !strnicmp( f+c, "name", 4 ) )
         {
            c += 4;
            c += SetPointer( &field->pName, f + c );
         }
         else if( !strnicmp( f+c, "size", 4 ) )
         {
            c += 4;
            c += SetPointer( &field->pSize, f + c );
         }
         else if( !strnicmp( f+c, "src", 3 ) )
         {
            c += 3;
            c += SetPointer( NULL, f+c );
         }
         else if( !strnicmp( f+c, "align", 5 ) )
         {
            c += 5;
            c += SetPointer( NULL, f+c );
         }
         else if( !strnicmp( f+c, "height", 6 ) )
         {
            c += 6;
            c += SetPointer( NULL, f+c );
         }
         else if( !strnicmp( f+c, "width", 5 ) )
         {
            c += 5;
            c += SetPointer( NULL, f+c );
         }
         else if( !strnicmp( f+c, "border", 6 ) )
         {
            c += 6;
            c += SetPointer( NULL, f+c );
         }
         else if( !strnicmp( f+c, "tabindex", 8 ) )
         {
            c += 8;
            c += SetPointer( NULL, f+c );
         }
         else if( !strnicmp( f+c, "checked", 7 ) )
         {
            c += 7;
//            c += SetPointer( NULL, f+c );
         }
         else if( !strnicmp( f+c, "alt", 3 ) )
         {
            c += 3;
            c += SetPointer( NULL, f+c );
         }
//         while( f[c] && (isalpha(f[c]) || isdigit( f[c] )) ) c++;
      }
      break;
   case FT_SELECT:
      c = 0;
      while( f[c] )
      {
         while( f[c] <= ' ' ) c++; 

         if( !strnicmp( f+c, "name", 4 ) )
         {
            c += 4;
            c += SetPointer( &field->pName, f + c );
         }
         else if( !strnicmp( f+c, "selected", 8 ) )
         {
            c += 8;
         }
         else if( !strnicmp( f+c, "disabled", 8 ) )
         {
            c += 8;
            field->bDisabled = TRUE;
         }
         else if( !strnicmp( f+c, "label", 5 ) )
         {
            c += 5;
            c += SetPointer( &field->pLabel, f + c );

         }
         else if( !strnicmp( f+c, "text", 4 ) )
         {
            c += 4;
            if( !field->pValue )
               c += SetPointer( &field->pValue, f + c );
            else
               while( f[c] ) c++;
         }
         else if( !strnicmp( f+c, "tabindex", 8 ) )
         {
            c += 8;
            c += SetPointer( NULL, f+c );
         }
         else if( !strnicmp( f+c, "size", 4 ) )
         {
            c+= 4;
            c += SetPointer( NULL, f+c );
         }
         //while( isalpha(f[c]) || isdigit( f[c] ) ) c++;
      }
      break;
   case FT_OPTION:
      c = 0;
      while( f[c] )
      {
         while( f[c] <= ' ' ) c++; 

         if( !strnicmp( f+c, "value", 5 ) )
         {
            c += 5;
            c += SetPointer( &field->pValue, f + c );
         }
         else if( !strnicmp( f+c, "selected", 8 ) )
         {
            c += 8;
         }
         else if( !strnicmp( f+c, "disabled", 8 ) )
         {
            c += 8;
         }
         else if( !strnicmp( f+c, "label", 5 ) )
         {
            c += 5;
            c += SetPointer( &field->pLabel, f + c );

         }
         else if( !strnicmp( f+c, "text", 4 ) )
         {
            c += 4;
            if( !field->pValue )
               c += SetPointer( &field->pValue, f + c );
            else
               while( f[c] ) c++;
         }
         //while( isalpha(f[c]) || isdigit( f[c] ) ) c++;
      }
      break;
   case FT_TEXTAREA:
      break;
   case FT_BUTTON:
      break;
   }
}

BOOL bLogForms;

void DumpForm( PFORM pf )
{
   int i;
   pf->FormBuffer[pf->len] = 0;
   if( bLogForms )
   {
      Log1( "Found FORM: \n%s\n", pf->FormBuffer );
   }

   {
      int c = 0;
      pf->pAction = NULL;
      while( pf->FormBuffer[c] )
      {
         while( pf->FormBuffer[c] == ' ' ) c++; 
         if( !strnicmp( pf->FormBuffer+c, "action", 6 ) ) {
            c += 6;
            c += SetPointer( &pf->pAction, pf->FormBuffer+c );
         } else if( !strnicmp( pf->FormBuffer+c, "method", 6 ) ) {
            c += 6;
            c += SetPointer( &pf->pMethod, pf->FormBuffer+c );
         } else if( !strnicmp( pf->FormBuffer+c, "enctype", 7 ) ) {
            c += 7;
            c += SetPointer( &pf->pEncode, pf->FormBuffer+c );
         } 
      }

      if( !pf->pAction )
      {
         Log( "Unknown action for form!\n" );
         return;
      }

      //pAddress = MakeAddress( Parent, pAction );
      //if( !pAddress )
      //   DebugBreak();  // why is this an invalid address?

      if( !strnicmp( pf->pMethod, "GET", 3 ) )
      {
         pf->nMethod = METH_GET;
      }
      else if( !strnicmp( pf->pMethod, "POST", 4 ) )
      {
         pf->nMethod = METH_POST;
      }
      else
      {
         pf->nMethod = METH_NONE;
      }
   }

   for( i = 0; i < pf->nField; i++ )
   {
      ParseField( pf->Fields + i );
      if( bLogForms )
      {
         switch( pf->Fields[i].nType )
         {
         case FT_INPUT:    OutputDebugString( "INPUT    : "); 
            if( pf->Fields[i].pType )
               Log1( "Type-%s : ", pf->Fields[i].pType );
            if( pf->Fields[i].pName )
               Log1( "Name-%s : ", pf->Fields[i].pName );
            if( pf->Fields[i].pValue )
               Log1( "Value-%s : ", pf->Fields[i].pValue );
            if( pf->Fields[i].pSize )
               Log1( "Size-%s : ", pf->Fields[i].pSize );
            break;
         case FT_SELECT:   OutputDebugString( "SELECT   : "); 
            if( pf->Fields[i].pType )
               Log1( "Type-%s : ", pf->Fields[i].pType );
            if( pf->Fields[i].pName )
               Log1( "Name-%s : ", pf->Fields[i].pName );
            if( pf->Fields[i].pValue )
               Log1( "Value-%s : ", pf->Fields[i].pValue );
            break;
         case FT_SELECT_END: OutputDebugString( "SELECTEND: "); 
            OutputDebugString( pf->Fields[i].Field );
            break;
         case FT_OPTION:   OutputDebugString( "OPTION   : "); 
            if( pf->Fields[i].pType )
               Log1( "Type-%s : ", pf->Fields[i].pType );
            if( pf->Fields[i].pName )
               Log1( "Name-%s : ", pf->Fields[i].pName );
            if( pf->Fields[i].pValue )
               Log1( "Value-%s : ", pf->Fields[i].pValue );
            break;
         case FT_BUTTON:   OutputDebugString( "BUTTON   : "); 
            OutputDebugString( pf->Fields[i].Field );
            break;
         case FT_TEXTAREA: OutputDebugString( "TEXTAREA : "); 
            OutputDebugString( pf->Fields[i].Field );
            break;
         default:          OutputDebugString( "UNKNOWN  : "); 
            OutputDebugString( pf->Fields[i].Field );
            break;
         }
         OutputDebugString( "\n" );
         Sleep(5);
      }
   }
}

PFORM CreateForm( ADDRESS *pAddress )
{
   PFORM pf;
   pf = (PFORM)Allocate( sizeof( FORM ) );
   pf->Parent = pAddress;
   pf->pAction = NULL;
   pf->FormBuffer[0] = 0;
   pf->TagAccum = 0;   
   pf->len    = 0;
   pf->nField = 0;
   pf->pNext  = NULL;
   pf->pPrior = NULL;

   NewField(pf);
   return pf;
}

void FreeForm( PFORM pf )
{
   Release( (PBYTE)pf );
}

/*
void QueueActive( PFORM pf )
{
   if( pf )
   {
      nPending++;
      pPrior = pLastActive;
      if( pLastActive )
      {
         pLastActive->pNext = pf;
      }
      else
         pFirstActive = pf;
      pLastActive = pf;
   }
}
*/

FIELD *NewField( PFORM pf )
{
   int nField = pf->nField;
   pf->Fields[nField].len       = 0; 
   pf->Fields[nField].pName     = NULL;
   pf->Fields[nField].pLabel    = NULL;
   pf->Fields[nField].pValue    = NULL;
   pf->Fields[nField].pSize     = NULL;
   pf->Fields[nField].pType     = NULL;
   pf->Fields[nField].ended     = FALSE;
   pf->Fields[nField].used      = FALSE;
   pf->Fields[nField].bDisabled = FALSE;
   return pf->Fields + nField;
}

/*
void DeQueue( PFORM pf )
{
   nPending--;
   if( pFirstActive )
      pFirstActive = pFirstActive->pNext;
   if( !pFirstActive )
      pLastActive = NULL;
   pFirstActive = pf->pNext;
}
*/

#define TAG_END            70
#define BEGIN_END_TEXTAREA 62
#define BEGIN_END_SELECT 56
#define BEGIN_END_BUTTON 50
#define BEGIN_OPTION     43
#define BEGIN_BUTTON     36
#define BEGIN_TEXTAREA   27
#define BEGIN_SELECT     20
#define BEGIN_INPUT      14 // Collectiong <INPUT...
#define BEGIN_END         7  // waiting for </FORM>

PBYTE FindForm( PFORM pf, PBYTE pBuffer, int BufLen )
{
   int c, t;
   for( c = 0; c < BufLen; c++ )
   {
      t = pf->TagAccum;
      if( t != 5 && 
          t != 6 && 
          t != 18 &&
          t != 25 &&
          t != 34 &&
          t != 41 && 
          t != 48 &&
          t != 49 &&
          t != 42 &&
          t != 26 &&
          t != 19 &&
          t != 6 &&
          pBuffer[c] <= 32 )  // skip spaces unless needed by state to step...
      {
         if( pf->nField )
            if( pf->Fields[pf->nField-1].nType == FT_OPTION )
            {
               if( !pf->Fields[pf->nField-1].ended )
                  if( pBuffer[c] == '\n' || pBuffer[c] == '\r' )
                     pf->Fields[pf->nField-1].ended = TRUE; // don't collect any more.
                  else
                  {
                     pf->Fields[pf->nField-1].Field[pf->Fields[pf->nField-1].len++] = pBuffer[c];
                     pf->Fields[pf->nField-1].Field[pf->Fields[pf->nField-1].len] = 0;                  
                  }
            }

         continue;
      }
      switch( t )
      {
      case 0:
         if( pBuffer[c] == '<' )
            pf->TagAccum++;
         break;
      case 1:
         if( pBuffer[c] == 'F' || pBuffer[c] == 'f' ) 
            pf->TagAccum++;
         else
            pf->TagAccum = 0;
         break;
      case 2:
         if( pBuffer[c] == 'O' || pBuffer[c] == 'o' ) 
            pf->TagAccum++;
         else
            pf->TagAccum = 0;
         break;
      case 3:
         if( pBuffer[c] == 'R' || pBuffer[c] == 'r' ) 
            pf->TagAccum++;
         else
            pf->TagAccum = 0;
         break;
      case 4:
         if( pBuffer[c] == 'M' || pBuffer[c] == 'm' ) 
            pf->TagAccum++;
         else
            pf->TagAccum = 0;
         break;
      case 5:
         if( pBuffer[c] == ' ' ) 
            pf->TagAccum++;
         else
            pf->TagAccum = 0;
         break;
      case 6:
         if( pBuffer[c] == '>' ) 
         {
            pf->TagAccum++; // next step should enable FIELD checking...
         }
         else
         {
            pf->FormBuffer[pf->len] = pBuffer[c];
            pf->len++;
         }
         break;
      case TAG_END:
         if( pBuffer[c] == '>' ) // toss out a tag we don't like...
         {
            pf->TagAccum = BEGIN_END;
         }
         break;
      case BEGIN_END:
         if( pBuffer[c] == '<' )
         {
            if( pf->nField )
               if( pf->Fields[pf->nField-1].nType == FT_OPTION )
                  pf->Fields[pf->nField-1].ended = TRUE;
            pf->TagAccum++;
         }
         else
         {
            if( pf->nField )
            {
               if( !pf->Fields[pf->nField-1].ended )
                  if( pf->Fields[pf->nField-1].nType == FT_OPTION )
                  {
                     pf->Fields[pf->nField-1].Field[pf->Fields[pf->nField-1].len++] = pBuffer[c];
                     pf->Fields[pf->nField-1].Field[pf->Fields[pf->nField-1].len] = 0;                  
                  }
            }
         }
         break;
      case 8:
         if( pBuffer[c] == '/' )
            pf->TagAccum++;
         else if( pBuffer[c] == 'I' || pBuffer[c] == 'i' )
            pf->TagAccum = BEGIN_INPUT;
         else if( pBuffer[c] == 'S' || pBuffer[c] == 's' ) 
            pf->TagAccum = BEGIN_SELECT;
         else if( pBuffer[c] == 'T' || pBuffer[c] == 't' ) 
            pf->TagAccum = BEGIN_TEXTAREA;
         else if( pBuffer[c] == 'B' || pBuffer[c] == 'b' ) 
            pf->TagAccum = BEGIN_TEXTAREA;
         else if( pBuffer[c] == 'O' || pBuffer[c] == 'o' ) 
            pf->TagAccum = BEGIN_OPTION;
         else
            pf->TagAccum = TAG_END;
         break;
      case 9:
         if( pBuffer[c] == 'F' || pBuffer[c] == 'f' )
         {
            pf->TagAccum++;
         }
         else if( pBuffer[c] == 'B' || pBuffer[c] == 'b' ) // </BUTTON>
            pf->TagAccum = BEGIN_END_BUTTON;
         else if( pBuffer[c] == 'S' || pBuffer[c] == 's' ) // </SELECT>
            pf->TagAccum = BEGIN_END_SELECT;
         else if( pBuffer[c] == 'T' || pBuffer[c] == 't' ) // </TEXTAREA>
            pf->TagAccum = BEGIN_END_TEXTAREA;
         else
            pf->TagAccum = TAG_END;
         break;
      case 10:
         if( pBuffer[c] == 'O' || pBuffer[c] == 'o' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = TAG_END;
         break;
      case 11:
         if( pBuffer[c] == 'R' || pBuffer[c] == 'r' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = TAG_END;
         break;
      case 12:
         if( pBuffer[c] == 'M' || pBuffer[c] == 'm' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = TAG_END;
         break;
      case 13:
         if( pBuffer[c] == '>' )
         {
            DumpForm(pf); // AKA parse form...
            // now - what to do with a completed form?!?!?!?!
            {
               char pAddr[4096];

               pf->pAddress = CreatePageAddress();
               strcpy( pf->pAddress->URL, pf->pAction );
               ParseAddress( pf->pAddress, pf->Parent, FALSE );
               SetMethod( pf->pAddress, METH_POST, pf->nField, pf->Fields );
               while( SLogAddress( pf->pAddress, pAddr ) ) 
               {
                  PADDRESS pa;
                  pa = MakeAddress( pf->pAddress, pAddr );
                  if( pa )
                  {
                     Log1( "URL : %s\n", pAddr );
                     SetMethod( pa, pf->nMethod, 0, NULL ); // don't need actual form content...
                     QueueActive(pa);
                  }
               }
               FreeAddress( pf->pAddress );
               pf->pAddress = NULL;
            }
            OutputDebugString( "------------ FORM PROCESSING GOES HERE --------------\n ");

//********************************************
            // delete the form, it's addresses, and all relatant data?!

            // pAddress->QueueActive();
//********************************************
            FreeForm( pf );
            return pBuffer + c + 1; // next character after last form character
         }
         else
         {
            OutputDebugString( "Extra Data assocated with </FORM> ?? \n" );
            DebugBreak();
            pf->TagAccum = BEGIN_END;
         }
         break;
      case BEGIN_INPUT:
         if( pBuffer[c] == 'N' || pBuffer[c] == 'n' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 15:
         if( pBuffer[c] == 'P' || pBuffer[c] == 'p' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 16:
         if( pBuffer[c] == 'U' || pBuffer[c] == 'u' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 17:
         if( pBuffer[c] == 'T' || pBuffer[c] == 't' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 18:
         if( pBuffer[c] == ' ' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 19:
         if( pBuffer[c] == '>' )
         {
            pf->Fields[pf->nField].Field[pf->Fields[pf->nField].len] = 0; // termiante string.
            pf->Fields[pf->nField].nType = FT_INPUT;
            pf->nField++; // next field please...
            NewField(pf);
            pf->TagAccum = BEGIN_END;
         }
         else 
         {
            pf->Fields[pf->nField].Field[pf->Fields[pf->nField].len++] = pBuffer[c];
         }
         break;
      case BEGIN_SELECT:
         if( pBuffer[c] == 'E' || pBuffer[c] == 'e' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 21:
         if( pBuffer[c] == 'L' || pBuffer[c] == 'l' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 22:
         if( pBuffer[c] == 'E' || pBuffer[c] == 'e' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 23:
         if( pBuffer[c] == 'C' || pBuffer[c] == 'c' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 24:
         if( pBuffer[c] == 'T' || pBuffer[c] == 't' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 25:
         if( pBuffer[c] == ' ' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 26:
         if( pBuffer[c] == '>' )
         {
            pf->Fields[pf->nField].Field[pf->Fields[pf->nField].len] = 0; // termiante string.
            pf->Fields[pf->nField].nType = FT_SELECT;
            pf->nField++; // next field please...
            NewField(pf);
            pf->TagAccum = BEGIN_END;
         }
         else
         {
            pf->Fields[pf->nField].Field[pf->Fields[pf->nField].len++] = pBuffer[c];
         }
         break;
      case BEGIN_TEXTAREA:
         if( pBuffer[c] == 'E' || pBuffer[c] == 'e' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 28:
         if( pBuffer[c] == 'X' || pBuffer[c] == 'x' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 29:
         if( pBuffer[c] == 'T' || pBuffer[c] == 't' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 30:
         if( pBuffer[c] == 'A' || pBuffer[c] == 'a' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 31:
         if( pBuffer[c] == 'R' || pBuffer[c] == 'r' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 32:
         if( pBuffer[c] == 'E' || pBuffer[c] == 'e' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 33:
         if( pBuffer[c] == 'A' || pBuffer[c] == 'a' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 34:
         if( pBuffer[c] == ' ' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 35:
         if( pBuffer[c] == '>' )
         {
            pf->TagAccum = BEGIN_END;
         }
         else
         {
            // accumulate into curent field....
         }
         break;
      case BEGIN_BUTTON:
         if( pBuffer[c] == 'U' || pBuffer[c] == 'u' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 37:
         if( pBuffer[c] == 'T' || pBuffer[c] == 't' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 38:
         if( pBuffer[c] == 'T' || pBuffer[c] == 't' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 39:
         if( pBuffer[c] == 'O' || pBuffer[c] == 'o' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 40:
         if( pBuffer[c] == 'N' || pBuffer[c] == 'n' )
            pf->TagAccum++;
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 41:
         if( pBuffer[c] == ' ' )
         {
            pf->TagAccum++;
         }
         else if( pBuffer[c] == '>' )
            goto ButtonEnd;
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 42:
         if( pBuffer[c] == '>' )
         {
ButtonEnd:
            pf->Fields[pf->nField].Field[pf->Fields[pf->nField].len] = 0; // termiante string.
            pf->Fields[pf->nField].nType = FT_BUTTON;
            pf->nField++; // next field please...
            NewField(pf);
            pf->TagAccum = BEGIN_END;
         }
         else // add input field data to currently collected input field...
         {
            pf->Fields[pf->nField].Field[pf->Fields[pf->nField].len++] = pBuffer[c];
         }
         break;
      case BEGIN_OPTION:
         if( pBuffer[c] == 'P' || pBuffer[c] == 'p' )
            pf->TagAccum++;
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 44:
         if( pBuffer[c] == 'T' || pBuffer[c] == 't' )
            pf->TagAccum++;
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 45:
         if( pBuffer[c] == 'I' || pBuffer[c] == 'i' )
            pf->TagAccum++;
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 46:
         if( pBuffer[c] == 'O' || pBuffer[c] == 'o' )
            pf->TagAccum++;
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 47:
         if( pBuffer[c] == 'N' || pBuffer[c] == 'n' )
            pf->TagAccum++;
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 48:
         if( pBuffer[c] == ' ' )
            pf->TagAccum++;
         else if( pBuffer[c] == '>' ) 
            goto OptionEnd;
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 49:
         if( pBuffer[c] == '>' )
         {
OptionEnd:
            if( pf->nField )
               pf->Fields[pf->nField-1].ended = TRUE;
            pf->Fields[pf->nField].Field[pf->Fields[pf->nField].len] = 0; // termiante string.
            strcat( pf->Fields[pf->nField].Field, " text=\"" );
            pf->Fields[pf->nField].len += 7;
            pf->Fields[pf->nField].Field[pf->Fields[pf->nField].len] = 0; // terminate new string...
            pf->Fields[pf->nField].nType = FT_OPTION;
            pf->nField++; // next field please...
            NewField(pf);
            pf->TagAccum = BEGIN_END;
         }
         else // add input field data to currently collected input field...
         {
            pf->Fields[pf->nField].Field[pf->Fields[pf->nField].len++] = pBuffer[c];
         }
         break;
      case BEGIN_END_BUTTON:
         if( pBuffer[c] == 'U' || pBuffer[c] == 'u' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 51:
         if( pBuffer[c] == 'T' || pBuffer[c] == 't' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 52:
         if( pBuffer[c] == 'T' || pBuffer[c] == 't' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 53:
         if( pBuffer[c] == 'O' || pBuffer[c] == 'o' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 54:
         if( pBuffer[c] == 'N' || pBuffer[c] == 'n' )
            pf->TagAccum++;
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 55:
         if( pBuffer[c] == '>' )
         {
            pf->TagAccum = BEGIN_END;
         }
         break;

      case BEGIN_END_SELECT:
         if( pBuffer[c] == 'E' || pBuffer[c] == 'e' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 57:
         if( pBuffer[c] == 'L' || pBuffer[c] == 'l' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 58:
         if( pBuffer[c] == 'E' || pBuffer[c] == 'e' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 59:
         if( pBuffer[c] == 'C' || pBuffer[c] == 'c' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 60:
         if( pBuffer[c] == 'T' || pBuffer[c] == 't' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 61:
         if( pBuffer[c] == '>' )
         {
            pf->Fields[pf->nField].Field[pf->Fields[pf->nField].len] = 0; // termiante string.
            pf->Fields[pf->nField].nType = FT_SELECT_END;
            pf->nField++; // next field please...
            NewField(pf);
            pf->TagAccum = BEGIN_END;
         }
         break;
      case BEGIN_END_TEXTAREA:
         if( pBuffer[c] == 'E' || pBuffer[c] == 'e' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 63:
         if( pBuffer[c] == 'X' || pBuffer[c] == 'x' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 64:
         if( pBuffer[c] == 'T' || pBuffer[c] == 't' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 65:
         if( pBuffer[c] == 'A' || pBuffer[c] == 'a' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 66:
         if( pBuffer[c] == 'R' || pBuffer[c] == 'r' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 67:
         if( pBuffer[c] == 'E' || pBuffer[c] == 'e' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 68:
         if( pBuffer[c] == 'A' || pBuffer[c] == 'a' )
         {
            pf->TagAccum++;
         }
         else
            pf->TagAccum = BEGIN_END;
         break;
      case 69:
         if( pBuffer[c] == '>' )
         {
            pf->TagAccum = BEGIN_END;
         }
         break;
      default:
         break;
      }
   }
   return NULL; // never update to completed form...
}

/*
void PostForm(void)
{
   // this posts the address to be handled... can't be handled just yet...
}

  */


// $Log: form.c,v $
// Revision 1.3  2003/03/25 08:59:03  panther
// Added CVS logging
//
