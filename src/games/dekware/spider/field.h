
#ifndef FIELD_DEFINED
#define FIELD_DEFINED

#define FT_UNUSED    -1  
#define FT_INPUT      0
#define FT_SELECT     1
#define FT_OPTION     11
#define FT_SELECT_END 12
#define FT_TEXTAREA   2
#define FT_BUTTON     3

#define FIT_SUBMIT 0
#define FIT_RESET  1
#define FIT_OTHER  2

typedef struct field_tag
{
   int nType;
   char Field[256]; // only use short fields for now....
   int  len;        // length of buffer used.
   struct {
      unsigned int  ended: 1;      
      unsigned int  used: 1; // last option used to process
      unsigned int bDisabled:1;
   };
   char *pName;
   char *pLabel;
   char *pValue;
   char *pType;
//   int  nInputType;
   char *pSize;
}FIELD, *PFIELD;

#endif

// $Log: field.h,v $
// Revision 1.3  2003/03/25 20:41:39  panther
// Fix what CVS logging addition broke
//
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
