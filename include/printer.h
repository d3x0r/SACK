#ifndef PRINTER_LIBRARY_DEFINED
#define PRINTER_LIBRARY_DEFINED

#ifdef PRINTER_LIBRARY_SOURCE
#define PRINTER_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define PRINTER_PROC(type,name) IMPORT_METHOD type CPROC name
#endif


PRINTER_PROC( struct printer_context *, Printer_Open )( void );

#endif
