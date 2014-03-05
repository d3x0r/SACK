
//#include "stdhdrs.h"

#include <comdef.h>  // bstr_t


class STRING_CONVERSION {
public:
   STRING_CONVERSION(){}
   ~STRING_CONVERSION(){}
   bool Reduce( char *out, WCHAR *in );
   bool Reduce( char *out, _bstr_t in );
   bool Reduce( char *out, CHAR *in );
};// $Log: $
