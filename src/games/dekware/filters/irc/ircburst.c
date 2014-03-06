
#include "termstruc.h" // mydatapath

PTEXT IRCBurst( PMYDATAPATH pmdp, PLINKQUEUE *ppInput, PTEXT pBuffer )
{
   // this function is expecting a single buffer segment....
   INDEX idx;
   int nState;
   char *ptext;
   int start, end, setstart;
   FORMAT attribute;
   int bAttribute;
   PTEXT pReturn = NULL, pNew;
   bAttribute = FALSE;
   attribute.foreground = DEFAULT_COLOR;
   attribute.background = DEFAULT_COLOR;
   nState = 0;
   start = 0;
   end = 0;
   setstart = FALSE;
   while( pBuffer )
   {
      ptext = GetText( pBuffer );
      for( idx = 0; idx < pBuffer->data.size; idx++ )
      {
         if( setstart )
         {
            start = idx;
            setstart = FALSE;
         }
         switch( ptext[idx] )
         {
         case ' ':
         	// ignore multiple spaces - or spaces which 
         	// follow something that has already been collected...
         	if( start == idx )
         		setstart = TRUE;
         	else
         	{
	            pNew = SegCreate( end - start );
      	      MemCpy( pNew->data.data, ptext + start, end - start );
         		pReturn = SegAppend( pReturn, pNew );
         	}
         	break;
         case '!': // only collect the first one to break the line...
         	break;
         case 0:
            // end buffer now... can be bad... but works for now.
            ptext[idx] = ' ';
            //DebugBreak();
            //idx = GetTextSize( pBuffer );
            break;
         case '\r':
            // if we haven't yet started the line, or if we've already
            // updated somewhat - but ... well DAMN how does this work??
            if( ( start == end ) && ( start == idx ) )
               setstart = TRUE;
            else
               end = idx;
            break;
         case '\n':
            {
               if( end == start ) // only a newline...
                  end = idx;
               if( start != end )
               {
                  pNew = SegCreate( end - start );
                  if( bAttribute )
                  {
                     pNew->format = attribute;
                     bAttribute = FALSE;
                  }
                  if( !pmdp->flags.bNewLine )
                     pNew->flags |= TF_NORETURN;
                  MemCpy( pNew->data.data, ptext + start, end - start );
                  pNew->data.data[end-start] = 0;
                  pReturn = SegAppend( pReturn, pNew );
                  EnqueLink( ppInput, pReturn );
                  pReturn = NULL;
               }
               else // newline only no data on line...
               {
                  if( pReturn )
                  {
                     EnqueLink( ppInput, pReturn );
                     pReturn = NULL;
                  }
                  else
                  {
                     if( pmdp->flags.bNewLine )
                        EnqueLink( ppInput, SegCreate(0) );
                  }
               }
               pmdp->flags.bNewLine = TRUE;
               setstart = TRUE;
            }
            break;
         }
      }
      if( setstart )
      {
         start = idx;
         setstart = FALSE;
      }
      // maybe... this should log the end of the buffer...
      // we should figure out a way to step through the buffer
      // and make sure that we're retaining appropriate state
      // informatino - including end of line... cause if the line
      // is recieved and the last thing was not a newline, then
      // the new line should be prefixed with TF_NORETURN
      end = idx;
      if( start >= 0 && start < end )
      {
         end = idx;
         pNew = SegCreate( end - start );
         pNew->flags |= TF_NORETURN;
         if( bAttribute )
         {
            pNew->format = attribute;
            bAttribute = FALSE;
         }
         if( !pmdp->flags.bNewLine )
            pNew->flags |= TF_NORETURN;
         // since this is extra data after a return, and we're adding it
         // to the output queue - we can assume that this did not have a newline...
         pmdp->flags.bNewLine = FALSE;
         MemCpy( pNew->data.data, ptext + start, end - start );
         pNew->data.data[end-start] = 0;
         pReturn = SegAppend( pReturn, pNew );
         EnqueLink( ppInput, pReturn );
         pReturn = NULL;
         start = end;
      }
      if( pReturn )
      {
         EnqueLink( ppInput, pReturn );
         pReturn = NULL;
      }

      pBuffer = NEXTLINE( pBuffer );
   }
   return pReturn;
}

// $Log: ircburst.c,v $
// Revision 1.2  2003/03/25 08:59:02  panther
// Added CVS logging
//
