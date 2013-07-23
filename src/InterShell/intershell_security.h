

// load security is triggered by starting a sub-configuration
void CPROC InterShell_SaveSecurityInformation( FILE *file, PTRSZVAL psv );

PTRSZVAL CPROC CreateSecurityContext( PTRSZVAL button );
void CPROC CloseSecurityContext( PTRSZVAL button, PTRSZVAL psv_context_to_Destroy );

void CPROC EditSecurity( PTRSZVAL psv, PSI_CONTROL button );
void CPROC EditSecurityNoList( PTRSZVAL psv, PSI_CONTROL button );
void CPROC SelectEditSecurity( PTRSZVAL psv, PSI_CONTROL listbox, PLISTITEM pli );

void CPROC SetupSecurityEdit( PSI_CONTROL frame, PTRSZVAL object_to_secure );
void CPROC InterShell_ReloadSecurityInformation( PCONFIG_HANDLER pch );
void CPROC AddSecurityContextToken( PTRSZVAL button, CTEXTSTR module, CTEXTSTR token );
void CPROC GetSecurityContextTokens( PTRSZVAL button, CTEXTSTR module, PLIST *tokens );

