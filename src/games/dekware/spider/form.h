
#include "address.h" // included FIELD.H

typedef struct form_tag {
   int TagAccum; // current state variable controlling scan...

   char FormBuffer[512]; // contains FORM tag info...
   int len;   // amount of data collected in FORM header...
   char *pAction; // would be URL of next page
   char *pMethod;
   char *pEncode;
   int nMethod;
   FIELD Fields[512]; // medium sized form field count...
   int nField;  // current field we're using...

   ADDRESS *Parent; // address of THIS page...
   ADDRESS *pAddress; // action address...

   struct form_tag *pNext, *pPrior;   // active/inactive lists
//                 *pParent, *pChild, // keep web relationship...
//                 *pElder, // since parent can only point at one child...
//                 *pLess, *pMore  // alphabetic sort for duplicate reduction
} FORM, *PFORM;

FIELD *GetField( void );
FIELD *NewField( PFORM );

PFORM CreateForm( ADDRESS *pAddress );
void FreeForm(PFORM pf);

PBYTE FindForm( PFORM pf, PBYTE pBuffer, int BufLen );
//void QueueActive( void );
//void DeQueue( void );
//class FORM *GetActive( void );
void DumpForm( PFORM pf );
void PostForm( PFORM pf );


#define ANYFORM ((PFORM)(1))


// $Log: form.h,v $
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
