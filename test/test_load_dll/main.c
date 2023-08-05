
#include <windows.h>
#include <stdio.h>

int main( void ) {
	static char here[4096];
	static wchar_t here_w[4096];
	static char tmp[4096];
	static wchar_t tmp_w[4096];
	GetCurrentDirectoryA( 4096, here );
	GetCurrentDirectoryW( 4096, here_w );
	//HMODULE mod_dep1 = LoadLibraryA( "plugin/dll_dep.dll" );
	//HMODULE mod_dep2 = LoadLibraryW( L"plugin/dll_dep.dll" );

	HMODULE mod1 = LoadLibraryA( "plugin\\dll.dll" );
	DWORD dwError1 = GetLastError();
	HMODULE mod2 = LoadLibraryW( L"plugin\\dll.dll" );
	DWORD dwError2 = GetLastError();

	snprintf( tmp, 4096, "%s\\plugin\\dll.dll", here );
	HMODULE mod_f1 = LoadLibraryA( tmp );
	DWORD dwError_f1 = GetLastError();
	printf( "Tried full ascii:%s\n", tmp );
	_snwprintf( tmp_w, 4096, L"%ls\\plugin\\dll.dll", here_w );
	HMODULE mod_f2 = LoadLibraryW( tmp_w );
	DWORD dwError_f2 = GetLastError();
	printf( "Tried full wide :%ls\n", tmp_w );

#if 1
	HMODULE mod3 = LoadLibraryExA( tmp, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
	DWORD dwError3 = GetLastError();
#else
	HMODULE mod3 = NULL;
	DWORD dwError3 = 0;
#endif
	HMODULE mod4 = LoadLibraryExW( tmp_w, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
	DWORD dwError4 = GetLastError();

	HMODULE mod5 = LoadLibraryExA( "plugin\\dll.dll", NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
	DWORD dwError5 = GetLastError();
	HMODULE mod6 = LoadLibraryExW( L"plugin\\dll.dll", NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
	DWORD dwError6 = GetLastError();

	printf( "modules: mod1&mod2 %p(%d) %p(%d)\n", mod1, dwError1, mod2, dwError2  );
	printf( "modules: mod3&mod4 %p(%d) %p(%d)\n", mod3, dwError3, mod4, dwError4 );
	printf( "modules: mod5&mod6 %p(%d) %p(%d)\n", mod5, dwError5, mod5, dwError5 );
	printf( "modules: %p(%d) %p(%d)", mod_f1, dwError_f1, mod_f2, dwError_f2 );

	return 0;
}
