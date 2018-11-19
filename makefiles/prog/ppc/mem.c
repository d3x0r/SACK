#include <stdio.h>
#include <stdlib.h>
#ifndef __MAC__
#  include <malloc.h>
#endif
#include <string.h>
#include <stddef.h>
#include "./types.h"
#include "./mem.h"
#include "global.h"

#define MEMLOG
//#define VALIDATE

//#pragma asm

typedef struct memblock
{
	short owners;
	short size;
#ifdef _DEBUG
	char *file;
	int   line;
#endif
	struct memblock **me, *next;
#ifdef _DEBUG
	unsigned long start_tag;
#endif
	char data[1];
} MEMBLOCK, *PMEMBLOCK;

PMEMBLOCK root;

int bDisableValidate;
#ifdef VALIDATE

void ValidateMemory( void )
{
	PMEMBLOCK mem = root;
	while( mem )
	{
		if( (mem->start_tag) != 0x12345678 )
		{
#ifdef _DEBUG
			fprintf( stddbg, WIDE("Block %s(%d) underflowed\n"), mem->file, mem->line );
#else
 			fprintf( stddbg, WIDE("Block %p underflowed\n"), mem );
#endif
			g.ErrorCount++;
			exit(g.ErrorCount);
		}
		if( *(long*)(mem->data + mem->size ) != 0x12345678 )
		{
#ifdef _DEBUG
			fprintf( stddbg, WIDE("Block %s(%d) overflowed\n"), mem->file, mem->line );
#else
 			fprintf( stddbg, WIDE("Block %p overflowed\n"), mem );
#endif
			g.ErrorCount++;
			exit(g.ErrorCount);
		}
		mem = mem->next;
	}
}
#endif

void DisableMemoryValidate( int bDisable )
{
	bDisableValidate = bDisable;
}

void *AllocateEx( size_t size DBG_PASS ) {
	PMEMBLOCK mem;
#ifdef VALIDATE
	ValidateMemory();
#endif
	g.nAllocates++;
	g.nAllocSize += size;
#ifdef VALIDATE
//                         long is the type used later
#  define EXTRA_SPACE sizeof( long )
#else
#  define EXTRA_SPACE 0
#endif
	mem = malloc(sizeof(MEMBLOCK) + EXTRA_SPACE + size);
	if( !mem )
	{
#ifdef _DEBUG
		fprintf( stddbg, WIDE("%s(%d) Out of memory.\n"), pFile, nLine );
#else
		fprintf( stddbg, WIDE("Out of memory.\n") );
#endif
		g.ErrorCount++;
		exit(g.ErrorCount);
	}
#ifdef _DEBUG
#ifdef MEMLOG
	if( g.bDebugLog & DEBUG_MEMORY ) {
		fprintf( stddbg, WIDE( "%s(%d): Allocate %zd %p\n" ), pFile, nLine, size, mem->data );
		fflush( stddbg );
	}
#endif
#endif

	if( mem->next = root )
		root->me = &mem->next;
	mem->me = &root;
	root = mem;

	mem->owners = 1;
	mem->size = (short)size;
#ifdef _DEBUG
	mem->file = pFile;
	mem->line = nLine;
#endif
#ifdef VALIDATE
	mem->start_tag = 0x12345678;
	*(long*)(mem->data + size) = 0x12345678;
	if( !bDisableValidate )
	{
		ValidateMemory( );
	}
#endif
	return &mem->data[0];
}

void ReleaseExx( void **pp DBG_PASS ) {
	void *p = *pp;
	PMEMBLOCK mem = (PMEMBLOCK)(((char*)p) - offsetof( MEMBLOCK, data ));
	g.nReleases++;
#ifdef MEMLOG
	if( g.bDebugLog & DEBUG_MEMORY ) {
#ifdef _DEBUG
		fprintf( stddbg, WIDE( "%s(%d): Release %p\n" )
			, pFile, nLine
			, p );
		fprintf( stddbg, WIDE( "%s(%d): %s(%d)Release %p\n" )
			, pFile, nLine
			, mem->file, mem->line
			, p );
		fflush( stddbg );
#else
		fprintf( stddbg, WIDE("Release %lp\n")
			, p );
#endif
	}
#endif
#ifdef VALIDATE
	if( !bDisableValidate )
	{
		ValidateMemory();
	}
#endif
	if( mem->owners != 1 )
	{
		fprintf( stddbg, WIDE("Block %p already free from: %s(%d) - or long ago freed (%d)...")
#ifdef _DEBUG
						" %s(%d)"
#endif
						, p
#ifdef _DEBUG
						, mem->file, mem->line
#endif
						, mem->owners
#ifdef _DEBUG
						, pFile, nLine
#endif
		);
		g.ErrorCount++;
		exit(g.ErrorCount);
	}

#ifdef _DEBUG
#ifdef VALIDATE
	if( ( *(long*)(mem->data + mem->size ) != 0x12345678 ||
		  mem->start_tag != 0x12345678 ) )
	{
		fprintf( stddbg, WIDE("Application overflowed memory.%p(%d) %s(%d)")
				 " %s(%d)"
				, mem->data
				, mem->size
				, mem->file, mem->line
				  DBG_RELAY );
		g.ErrorCount++;
		exit(g.ErrorCount);
	}
#endif
#endif

	if( *mem->me = mem->next )
		mem->next->me = mem->me;

#ifdef _DEBUG
	mem->file = pFile;
	mem->line = nLine;
#endif

	mem->owners = 0;
	free(mem);
	*pp = NULL;
}

void DumpMemory( void )
{
	PMEMBLOCK mem = root;
	while( mem )
	{
		fprintf( stddbg, WIDE("Block: %d %p ")
#ifdef _DEBUG
							"%s(%d)"
#endif
							"\n", mem->size, mem->data
#ifdef _DEBUG
				,mem->file, mem->line
#endif
				 );
		mem = mem->next;
	}
}

void CPROC MemSet( void *p, uint32_t v, size_t n) {memset(p,v,n);}
void CPROC MemCpy( void *p, const void *p2, size_t n) {memcpy(p,p2,n);}

uint32_t LockedExchange( uint32_t *p, uint32_t val )
{
	uint32_t x;
	x = *p;
	*p = val;
	return x;
/*
   long res;
   asm .386;
   asm les di, p;
   asm mov ecx, val;
   asm xchg es:[di], ecx ;
   asm mov res, ecx;
   return res;
*/
}

char *StrDupEx( const char *original DBG_PASS )
{
	size_t len = strlen( original ) + 1;
	char *result = AllocateEx( len DBG_RELAY );
	MemCpy( result, original, len );
	return result;
}

#if defined( __GNUC__ ) && !defined( _WIN32 )
int stricmp( char *one, char *two )
{
	return strcasecmp( one, two );
}

int strnicmp( char *one, char *two, int len )
{
	return strncasecmp( one, two, len );
}
#endif
