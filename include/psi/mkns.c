

// quick and dirty program to build namespace.h.
// was a complex copy and paste...

struct {
	char *kindname;
	char *defname;
} names[] = { { "button", "BUTTON" }
				, { "colorwell", "COLORWELL" }
				, { "popup", "MENU" }
				, { "text", "TEXT" }
				, { "edit", "EDIT" }
				, { "slider", "SLIDER" }
				, { "font", "FONTS" }
				, { "listbox", "LISTBOX" }
				, { "scrollbar", "SCROLLBAR" }
				, { "sheet_control", "SHEETS" }
				, { "_mouse", "MOUSE" }
				, { "xml", "XML" }
				, { "properties", "PROP" }
};

#define count (sizeof(names)/sizeof(names[0] ))
int main( void )
{
	int i;
	printf( "#ifndef __panthers_slick_interface_namespace__\n" );
	printf( "#define __panthers_slick_interface_namespace__\n" );
	printf( "\n" );
	printf( "#ifdef __cplusplus\n" );
	printf( "\n" );
	printf( "#define PSI_NAMESPACE SACK_NAMESPACE namespace psi {\n");
	printf( "#define PSI_NAMESPACE_END } SACK_NAMESPACE_END\n");
	printf( "#define USE_PSI_NAMESPACE using namespace sack::psi;\n");
	printf( "\n" );

	for( i = 0; i < count; i++ )
	{
   	printf( "#   define _%s_NAMESPACE namespace %s {\n", names[i].defname, names[i].kindname );
		printf( "#   define _%s_NAMESPACE_END } \n", names[i].defname);
		printf( "#   define USE_%s_NAMESPACE using namespace %s; \n", names[i].defname, names[i].kindname);
		printf( "#   define USE_PSI_%s_NAMESPACE using namespace sack::psi::%s; \n", names[i].defname, names[i].kindname);
		printf( "\n" );

	}
	printf( "#else\n" );
	printf( "\n" );
	printf( "#define PSI_NAMESPACE SACK_NAMESPACE \n");
	printf( "#define PSI_NAMESPACE_END SACK_NAMESPACE_END\n");
	printf( "#define USE_PSI_NAMESPACE\n");
	printf( "\n" );

	for( i = 0; i < count; i++ )
	{
   	printf( "#   define _%s_NAMESPACE \n", names[i].defname );
		printf( "#   define _%s_NAMESPACE_END \n", names[i].defname);
		printf( "#   define USE_%s_NAMESPACE\n", names[i].defname, names[i].kindname);
		printf( "#   define USE_PSI_%s_NAMESPACE\n", names[i].defname, names[i].kindname);
		printf( "\n" );

	}
	printf( "#endif\n" );
	printf( "\n" );
	for( i = 0; i < count; i++ )
	{
   	printf( "#define PSI_%s_NAMESPACE PSI_NAMESPACE _%s_NAMESPACE\n", names[i].defname, names[i].defname);
		printf( "#define PSI_%s_NAMESPACE_END _%s_NAMESPACE_END PSI_NAMESPACE_END\n", names[i].defname, names[i].defname);
		printf( "\n" );
	}
	printf( "#endif\n" );
}



