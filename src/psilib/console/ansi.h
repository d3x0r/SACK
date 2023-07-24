
typedef struct mydatapath_tag  * PANSI_DATAPATH;
typedef struct myconsolestruc *PCONSOLE_INFO;
PANSI_DATAPATH OpenAnsi( PCONSOLE_INFO );
PTEXT GetPendingWrite( PANSI_DATAPATH pmdp );

// close this channel;
void CloseAnsi( PANSI_DATAPATH pmdp );

// sometimes ansi writes data back (getting cursor position for example)
void SetWriteCallback( PANSI_DATAPATH pmdp, void (*write)(uintptr_t,PTEXT), uintptr_t );

// this converts pBuffer data into GetPendingWrite() data...
void AnsiBurst( PANSI_DATAPATH pmdp, PTEXT pBuffer );
