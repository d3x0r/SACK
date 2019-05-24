

typedef TEXTCHAR* (CPROC* RegisterRoutinesProc)(void);
typedef void (CPROC* UnloadPluginProc)(void);

typedef struct plugin_tag {
	//	HANDLE hModule;
	RegisterRoutinesProc RegisterRoutines;
	//UnloadPluginProc Unload;
	// maybe track all device_tags and routine_tags registered...
	struct plugin_tag* pNext, * pPrior;
	char* pVersion;
	TEXTCHAR pName[];
} PLUGIN, * PPLUGIN;

