
#include <stdhdrs.h>
#include <excpt.h>
#include <filedotnet.h>

#ifdef __cplusplus_cli 

using namespace System::IO;
using namespace System;

struct MyFile {
	gcroot<System::IO::StreamReader^> sr;
	gcroot<System::IO::FileStream^> fs;

};
typedef struct MyFile MYFILE;

int Rename( CTEXTSTR from, CTEXTSTR to )
{
	String^ tmp_from = gcnew System::String(from);
	String^ tmp_to = gcnew System::String(to);
	File::Move( tmp_from, tmp_to );
	return 0;
}

int Unlink( CTEXTSTR filename )
{
	String^ tmp = gcnew System::String(filename);
// sharemem defined Delete to compliement New macros...
#undef Delete
	File::Delete( tmp );
	return 1;
}

#if 0
MYFILE *Fopen( CTEXTSTR filename, CTEXTSTR mode )
{
	String^ tmp = gcnew System::String(filename);
	MYFILE *file = New( MYFILE );
	if( File::Exists(tmp) )
	{
		__try
		{
			file->fs = File::OpenRead( tmp ); 
		} 
		__except(EXCEPTION_EXECUTE_HANDLER )
		{
			delete file;
			file = NULL;
		}
	}
	return file;
}
#endif
int Fwrite( POINTER data, int count, int size, MYFILE *file )
{
	return 0;
}


int Fread( POINTER data, int count, int size, MYFILE *file )
{
	return 0;
}

int Fclose( MYFILE *file )
{
	if( file )
	{
		file->fs->Close();
		Release( file );
	}
	return 1;
}

int Fprintf( FILE *file, CTEXTSTR fmt, ... )
{
	PVARTEXT pvt = VarTextCreate();
	va_list args;
	va_start( args, fmt );
	vvtprintf( pvt, fmt, args );
	PTEXT out = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	fputs( GetText( out ), file );
	LineRelease( out );
	return 0;
}

#if 0
int Fprintf( MYFILE *file, CTEXTSTR fmt, ... )
{
	return 0;
}
#endif

int Fseek( MYFILE *file, S_64 pos, int whence )
{
	return 0;
}

_64 Ftell( MYFILE *file )
{
	return 0;
}

int Fflush( MYFILE *file )
{
	return 0;
}

int Fputs( CTEXTSTR buf, MYFILE *file )
{
	return 0;
}

int Fgets( CTEXTSTR buf, int bufsize, MYFILE *file )
{
	return 0;
}

#if 0


using namespace System;
using namespace System::Runtime::InteropServices;
//typedef void* HWND;
[DllImport("user32", CharSet=CharSet::Ansi)]
extern "C" int MessageBox(HWND hWnd, String ^ pText, String ^ pCaption, unsigned int uType);

#endif

#endif
