

#ifndef DEFINE_GLOBAL
extern
#endif
	struct global_shared_data
{
	struct global_flags_tag
	{
		BIT_FIELD use_common_build : 1;
		BIT_FIELD use_all_build : 1;
		BIT_FIELD skip_push : 1;
	} flags;
	CTEXTSTR dsn;
	INDEX build_id;
} g;

//----------------------------------------------------------------------
// db.c

void SetCurrentVersion( PODBC odbc, CTEXTSTR version );
INDEX CheckProjectTable( PODBC odbc, CTEXTSTR project_root );
INDEX GetBuildVersion( PODBC odbc, INDEX project_id );
void IncrementBuild( PODBC odbc );


