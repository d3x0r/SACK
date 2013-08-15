#define MAKE_KEYMAP
#include "keymap.h"

int main( void )
{
	int n;
   int first = 1;
	for( n = 0; n < (sizeof( keymap )/sizeof(keymap[0])); n++ )
	{
		printf( "%s { %s,\"%s\",\"%s\" } \t//[%s]=%s   (%d=%d)\n"
				, first
				 ?"unsigned struct {char scancode; char *keyname; char *othername; } keymap[] = {"
				 :"\t,"
				, keymap[n].my_name?keymap[n].my_name:"0"
				, keymap[n].my_name?keymap[n].my_name:"0"
				, keymap[n].sdl_name?keymap[n].sdl_name:"0"
				, keymap[n].sdl_name?keymap[n].sdl_name:"0"
				, keymap[n].my_name?keymap[n].my_name:"0"
				, keymap[n].sdl_number
				, keymap[n].my_number
				);
      first = 0;
	}
	printf( "};\n" );
}

