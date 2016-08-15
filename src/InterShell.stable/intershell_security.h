

// load security is triggered by starting a sub-configuration
void CPROC InterShell_SaveSecurityInformation( FILE *file, uintptr_t psv );

uintptr_t CPROC CreateSecurityContext( uintptr_t button );
void CPROC CloseSecurityContext( uintptr_t button, uintptr_t psv_context_to_Destroy );

void CPROC EditSecurity( uintptr_t psv, PSI_CONTROL button );
void CPROC EditSecurityNoList( uintptr_t psv, PSI_CONTROL button );
void CPROC SelectEditSecurity( uintptr_t psv, PSI_CONTROL listbox, PLISTITEM pli );

void CPROC SetupSecurityEdit( PSI_CONTROL frame, uintptr_t object_to_secure );
void CPROC InterShell_ReloadSecurityInformation( PCONFIG_HANDLER pch );
void CPROC AddSecurityContextToken( uintptr_t button, CTEXTSTR module, CTEXTSTR token );
void CPROC GetSecurityContextTokens( uintptr_t button, CTEXTSTR module, PLIST *tokens );
void CPROC GetSecurityModules( PLIST *ppList );

