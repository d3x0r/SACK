#include <stdio.h>
#include <../../psilib/controlstruc.h">
#include <psi.h>

SaveAControl( FILE *output, PSI_CONTROL pc )
{
	if( pc->child )
		SaveAControl( output, pc->child );
	while( pc )
	{
		switch( pc->nType )
		{
  // case STATIC_TEXT:
  // 	fprintf( output, "LTEXT           "The value below has not been found.  Please enter the correct value.",-1,40,2,120,16
  //  //CTEXT           "in",-1,4,56,248,8
  //  //CTEXT           "should be",-1,4,84,248,8
  //    break;
  // case EDIT_FIELD:
  //  EDITTEXT        IDC_SECTION,4,24,248,12,ES_READONLY
  //    break;
  // 	EDITTEXT        IDC_DEFAULT,4,96,248,12,ES_AUTOHSCROLL
  //    break;
	//case NORMAL_BUTTON:
	//	fprintf( output, "PUSHBUTTON      "&Ok",IDOK,4,112,248,14
	//			  break;
	//case LISTBOX_CONTROL:
	//	break;

		default:
			fprintf( output, "CONTROL         \"%s\",%s,\"%s\",WS_TABSTOP,%d,%d,%d,%d\n"
					 , GetText( pc->caption->text ) // get common text? maybe that has some translations for newlines?
					 , pc->nID
					 , pc->pIDName
					 , pc->nTypeName
					 , pc->original_rect.x
					 , pc->original_rect.y
					 , pc->original_rect.width
					 , pc->original_rect.height
					 );
		}
		pc = pc->elder;
	}
}


SaveRC( FILE *output, PSI_CONTROL frame )
{
fprintf( output, "// Microsoft Visual C++ generated resource script.\n" );
fprintf( output, "//\n" );
fprintf( output, "#include \"resource.h\"\n" );
fprintf( output, "\n" );
fprintf( output, "#define APSTUDIO_READONLY_SYMBOLS\n" );
fprintf( output, "/////////////////////////////////////////////////////////////////////////////\n" );
fprintf( output, "//\n" );
fprintf( output, "// Generated from the TEXTINCLUDE 2 resource.\n" );
fprintf( output, "//\n" );
fprintf( output, "#include \"afxres.h\"\n" );
fprintf( output, "\n" );
fprintf( output, "/////////////////////////////////////////////////////////////////////////////\n" );
fprintf( output, "#undef APSTUDIO_READONLY_SYMBOLS\n" );
fprintf( output, "\n" );
fprintf( output, "/////////////////////////////////////////////////////////////////////////////\n" );
fprintf( output, "// English (U.S.) resources\n" );
fprintf( output, "\n" );
fprintf( output, "#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_ENU)\n" );
fprintf( output, "#ifdef _WIN32\n" );
fprintf( output, "LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US\n" );
fprintf( output, "#pragma code_page(1252)\n" );
fprintf( output, "#endif //_WIN32\n" );
fprintf( output, "\n" );
fprintf( output, "/////////////////////////////////////////////////////////////////////////////\n" );
fprintf( output, "//\n" );
fprintf( output, "// Dialog\n" );
fprintf( output, "//\n" );
fprintf( output, "\n" );


fprintf( output, "DEFAULTINIDIALOG DIALOGEX 59, 34, 275, 165\n" );
fprintf( output, "STYLE DS_SYSMODAL | DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION\n" );
fprintf( output, "CAPTION \"INI Entry Error\"\n" );
fprintf( output, "FONT 8, \"MS Sans Serif\", 0, 0, 0x0\n" );
fprintf( output, "BEGIN\n" );
/* for each control... */
SaveAControl( file, pc->child );

fprintf( output, "END\n" );
fprintf( output, "\n" );
fprintf( output, "\n" );
fprintf( output, "#ifdef APSTUDIO_INVOKED\n" );
fprintf( output, "/////////////////////////////////////////////////////////////////////////////\n" );
fprintf( output, "//\n" );
fprintf( output, "// TEXTINCLUDE\n" );
fprintf( output, "//\n" );
fprintf( output, "\n" );
fprintf( output, "1 TEXTINCLUDE \n" );
fprintf( output, "BEGIN\n" );
fprintf( output, "    \"resource.\0\"\n" );
fprintf( output, "END\n" );
fprintf( output, "\n" );
fprintf( output, "\n" );
fprintf( output, "3 TEXTINCLUDE \n" );
fprintf( output, "BEGIN\n" );
fprintf( output, "    \"\r\0\"\n" );
fprintf( output, "END\n" );
fprintf( output, "\n" );
fprintf( output, "#endif    // APSTUDIO_INVOKED\n" );
fprintf( output, "\n" );
fprintf( output, "#endif    // English (U.S.) resources\n" );
fprintf( output, "/////////////////////////////////////////////////////////////////////////////\n" );
fprintf( output, "\n" );
fprintf( output, "\n" );
fprintf( output, "\n" );
fprintf( output, "#ifndef APSTUDIO_INVOKED\n" );
fprintf( output, "/////////////////////////////////////////////////////////////////////////////\n" );
fprintf( output, "//\n" );
fprintf( output, "// Generated from the TEXTINCLUDE 3 resource.\n" );
fprintf( output, "//\n" );
fprintf( output, "\n" );
fprintf( output, "\n" );
fprintf( output, "/////////////////////////////////////////////////////////////////////////////\n" );
fprintf( output, "#endif    // not APSTUDIO_INVOKED\n" );
fprintf( output, "\n" );

}
