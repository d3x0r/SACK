
#include <sharemem.h>

typedef struct resource_header_tag
{
	struct {
		_32 bUsed : 1; // this has been referenced.
		_32 bFree : 1; // this slot is actually empty now.
	} flags;
	_32 dwNextEntry;
	_32 dwEntrySize;
   _32 dwDirectory;
	// Version is encoded date/time of the file...
	// plus some bits of sub version for quick reference.
	// Low bytes count XX.YY where the first version is
	// 1.0
   // this leave 48 bytes for the time information.
	_64 Version;
	TEXTCHAR name[];
	// name is a nul terminated sequence of characters...
   // following name is the data of this resource.
} RESOURCE_HEADER, *PRESOURCE_HEADER;

typedef struct resource_directory_tag
{
   _32 parent;
	_32 next;
   TEXTCHAR name[];
} RESOURCE_DIRECTORY, *PRESOURCE_DIRECTORY;



typedef struct resource_usage_tag
{

}



// So of course this library needs...

int LoadResourceEx( char *address
						 , char *path
						 , char *name
						 , POINTER *buffer
						, _32 *size
						, _32 options )
{
	// address may specify a remote host including a mime prefix
	// http:, ftp:, etc?
	// path is of course the path to the resource
	// and the name is the filename(typically) of the resource.

	// Address may be omitted, path may be omitted, but then name
	// MUST be already in the resource library, else failure results.

	// path can be used to reference the resource in the local resource
   // library...

	// the pointer buffer is filled in with a reference of the data
	// and size is set to the size of the object.

	// options may indicate - duplicate the resource into its own
   // which therefore does not set used in the library.

	// result code is 0 on failure else true on success - perhaps a
   // redundant result of size?
}

void StoreResource( PRESOURCE_SET prs
						, POINTER data, _32 size
                  , char *path
						, char *name )
{
	// need to figure out if
	//   a> this resource exists
	//   b> there is a hole large enough to store the resource
	//   c> If the resource exists, see if the existing resource
	//     is large enough to contain the new one overwriting it...
   //   d> append the resource at the end of the library, remap it.
}


PRESOURCE_SET OpenResourceLibrary( char *name )
{
   // opens a local library of resources for read/write...
}

void CloseResourceLibrary( PRESOURCE_SET prs )
{
	// on close, collapse any empty space between resource records.
	// this is easy enough to do, since each record defines exactly it's size.

	// while traversing the list of resources, flags such as used are checked
   // and or cleared...
}

