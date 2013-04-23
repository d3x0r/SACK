#include <stdhdrs.h>

#include "../genx/genx.h"


static genxStatus WriteBuffer( void *UserData, constUtf8 s )
{
	vtprintf( (PVARTEXT)UserData, WIDE("%s"), s );
	return GENX_SUCCESS;
}

static genxStatus WriteBufferBounded( void *UserData, constUtf8 s, constUtf8 end )
{
	vtprintf( (PVARTEXT)UserData, WIDE("%*.*s"), end-s, end-s, s );
	return GENX_SUCCESS;
}

static genxStatus Flush( void *UserData )
{
	return GENX_SUCCESS;
}
genxSender senderprocs = { WriteBuffer
								 , WriteBufferBounded
								 , Flush };

int main ( void )
{
	genxWriter w;
	genxElement header;
	genxElement body;
   genxElement html;
	genxStatus status;
   int n;
	PVARTEXT pvt = VarTextCreate();


	w = genxNew(NULL,NULL,NULL);

	genxSetUserData( w, (void*)pvt );
	html = genxDeclareElement( w, NULL, (constUtf8)"HTML", &status );
	header = genxDeclareElement( w, NULL, (constUtf8)"header", &status );
   body = genxDeclareElement( w, NULL, (constUtf8)"body", &status );

	genxStartDocSender( w, &senderprocs );
	n = 	genxPI( w, "xml", "version=\"1.0\" encoding=\"UTF-8\"" );

	printf( "result:%d\n", n );
	genxStartElement( html );
	{
		genxStartElement( header );
		genxAddText( w, "Header Here" );
		genxEndElement( w );

		genxStartElement( body );
		genxAddText( w, "body Here" );
		genxEndElement( w );
	}
	genxEndElement( w );

	genxEndDocument( w );

	printf( "Result Document:\n\n" );
   printf( "%s", GetText( VarTextPeek( pvt ) ) );
   return 0;
}



