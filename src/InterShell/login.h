

#define INVALID_INDEX_SESSION     ((INDEX)-2)
#define INVALID_INDEX_LOGING      ((INDEX)-3)
// required program is invalid, login aborted...
#define INVALID_INDEX_PROGRAM_REQ ((INDEX)-4)

INDEX AutoLogin( char *required_program, char *program );
INDEX GetLoginID( INDEX iUser );
