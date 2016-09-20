#include <stdhdrs.h> // string type
#include <sack_types.h>
#include <sharemem.h>
#include <filesys.h>

typedef struct file_entry
{
   char *name;
}FILE_ENTRY, *PFILE_ENTRY;

PTREEROOT tree1, tree2;

int test1, test2;

int bDoLog = 0;
int nRootLen1, nRootLen2;

static int CPROC MyStrCmp( uintptr_t old, uintptr_t new_node )
{
   if( bDoLog )
		lprintf( WIDE("Compare %s and %s"), (char*)old+test1, (char*)new_node+test2 );
   return stricmp( (CTEXTSTR)old + test1, (CTEXTSTR)new_node + test2 );
}

void CPROC AddDir( uintptr_t psv, CTEXTSTR name, int flags )
{
   name = StrDup( name );
   test1 = nRootLen1;
	test2 = nRootLen1;
	//lprintf( WIDE("Add Dir %s"), name );
   AddBinaryNode( tree1, (POINTER)name, (uintptr_t)name );
}

void CPROC FindDir( uintptr_t psv,CTEXTSTR name, int flags )
{
	char *othername;
   name = StrDup( name );

   test1 = nRootLen2;
	test2 = nRootLen1;
   //bDoLog = 1;
   //lprintf( WIDE("Find Dir %s"), name );
	othername = (char*)FindInBinaryTree( tree1, (uintptr_t)name );
   test1 = nRootLen2;
   test2 = nRootLen2;
   //lprintf( WIDE("Add Dir %s"), name );
   AddBinaryNode( tree2, (POINTER)name, (uintptr_t)name );
	if( !othername )
      printf( WIDE("%s\n"), name );
}

void CPROC FindDir2( uintptr_t psv, CTEXTSTR name, int flags )
{
	char *othername;

   test1 = nRootLen1;
   test2 = nRootLen2;
	othername = (char*)FindInBinaryTree( tree2, (uintptr_t)name );
	if( !othername )
      printf( WIDE("%s\n"), name );
}


int main( int argc, char **argv )
{
	//int n;
	if( argc < 3 )
	{
		printf( WIDE("Must specify two directories to compare...\n"));
      return 0;
	}
   tree1 = CreateBinaryTreeExx( 0, MyStrCmp, NULL );
   tree2 = CreateBinaryTreeExx( 0, MyStrCmp, NULL );
	nRootLen1 = strlen( argv[1] );
	nRootLen2 = strlen( argv[2] );
	{
		void *info = NULL;
		while( ScanFiles( argv[1], WIDE("*"), &info, AddDir, SFF_DIRECTORIES|SFF_SUBCURSE, 0 ) );
	}
   BalanceBinaryTree( tree1 );
	printf( WIDE("Files not in first tree...\n") );
	{
		void *info = NULL;
		while( ScanFiles( argv[2], WIDE("*"), &info, FindDir, SFF_DIRECTORIES|SFF_SUBCURSE, 0 ) );
	}
   BalanceBinaryTree( tree2 );
	printf( WIDE("Files not in second tree...\n") );
	{
		void *info = NULL;
		while( ScanFiles( argv[1], WIDE("*"), &info, FindDir2, SFF_DIRECTORIES|SFF_SUBCURSE, 0 ) );
	}


   return 0;
}
