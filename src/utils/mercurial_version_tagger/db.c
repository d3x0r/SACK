
#include <stdhdrs.h>
#include <pssql.h>
#include <deadstart.h>
#include "global.h"

CTEXTSTR create_current_version_table = "create table current_version ( version_id int )";

CTEXTSTR create_version_table = "create table version ("
	" version_id int auto_increment,"
	" version_string char(42),"
	" version_date timestamp,"
	" PRIMARY KEY(version_id) )";

CTEXTSTR create_project_table = "create table project(project_id int auto_increment,root_path char(256),PRIMARY KEY(project_id))";
TEXTCHAR create_build_table[] = "create table build_%d(build_id int auto_increment,build_timestamp timestamp,PRIMARY KEY(build_id))";

CTEXTSTR create_project_version_table = "create table %s(project_version_id int auto_increment, project_id int, hg_id char(32), build_id int, version_id int, PRIMARY KEY(project_version_id))";


struct another_local_data_tag
{
   LOGICAL inited;
   INDEX version_id;
} local_2;
#define l local_2

static void InitVersion( PODBC odbc )
{
	if( !l.inited )
	{
		PTABLE table;
      TEXTCHAR build_table[sizeof( create_build_table ) + 12];
		CTEXTSTR result;
		
		table = GetFieldsInSQL( create_version_table, FALSE );
		CheckODBCTable( odbc, table, CTO_MERGE );
		DestroySQLTable( table );

		table = GetFieldsInSQL( create_current_version_table, FALSE );
		CheckODBCTable( odbc, table, CTO_MERGE );
		DestroySQLTable( table );

		table = GetFieldsInSQL( create_project_table, FALSE );
		CheckODBCTable( odbc, table, CTO_MERGE );
		DestroySQLTable( table );


		SQLQueryf( odbc, &result, "select version_id from current_version" );

		if( result )
			l.version_id = (INDEX)IntCreateFromText( result );
		else
		{
			SQLCommandf( odbc, "insert into version(version_string)values('1.0')" );
			l.version_id = FetchLastInsertID( odbc, "version", "version_id" );
         SQLCommandf( odbc, "insert into current_version(version_id)values(%d)", l.version_id );
		}

		snprintf( build_table, sizeof(build_table)/sizeof(TEXTCHAR)
				  , create_build_table, l.version_id );

		table = GetFieldsInSQL( build_table, FALSE );
		CheckODBCTable( odbc, table, CTO_MERGE );
		DestroySQLTable( table );


		if( g.build_id == INVALID_INDEX )
		{
			SQLQueryf( odbc, &result, "select build_id from build_%d order by build_id desc limit 1", l.version_id );
			if( result )
				g.build_id = (INDEX)IntCreateFromText( result );
			else
			{
				SQLCommandf( odbc, "insert into build_%d(build_timestamp)values(now())", l.version_id );
				snprintf( build_table, 64, "build_%d", l.version_id );
				g.build_id = FetchLastInsertID( odbc, build_table, "build_id" );
			}
		}

		l.inited = TRUE;
	}
}

void SetCurrentVersion( PODBC odbc, CTEXTSTR version )
{
	CTEXTSTR result;
	TEXTCHAR build_table[sizeof( create_build_table ) + 12];
	PTABLE table;

	InitVersion( odbc );

	SQLQueryf( odbc, &result, "select version_id from version where version_string=%s", EscapeStringOpt( version, TRUE ) );
	if( result )
		l.version_id = (INDEX)IntCreateFromText( result );
	else
	{
		SQLCommandf( odbc, "insert into version(version_string)values(%s)", EscapeStringOpt(version, TRUE) );
		l.version_id = FetchLastInsertID( odbc, "version", "version_id" );
	}

	SQLCommandf( odbc, "delete from current_version" );
	SQLCommandf( odbc, "insert into current_version(version_id)values(%d)", l.version_id );

	snprintf( build_table, sizeof(build_table)/sizeof(TEXTCHAR)
			  , create_build_table, l.version_id );

	table = GetFieldsInSQL( build_table, FALSE );
	CheckODBCTable( odbc, table, CTO_MERGE );
	DestroySQLTable( table );

	SQLQueryf( odbc, &result, "select build_id from build_%d order by build_id desc limit 1", l.version_id );
	if( result )
		g.build_id = (INDEX)IntCreateFromText( result );
	else
	{
      TEXTCHAR build_table[64];
		SQLCommandf( odbc, "insert into build_%d(build_timestamp)values(now()x)", l.version_id );
		snprintf( build_table, 64, "build_%d", l.version_id );
		g.build_id = FetchLastInsertID( odbc, build_table, "build_id" );
	}
}

void IncrementBuild( PODBC odbc )
{
	TEXTCHAR build_table[64];
	InitVersion( odbc );

	SQLCommandf( odbc, "insert into build_%d(build_timestamp)values(now())", l.version_id );
	snprintf( build_table, 64, "build_%d", l.version_id );
	g.build_id = FetchLastInsertID( odbc, build_table, "build_id" );

}


INDEX CheckProjectTable( PODBC odbc, CTEXTSTR project_root )
{
   InitVersion( odbc );
	{
		TEXTCHAR tmp[256];
		TEXTCHAR tmp_create[256];
		INDEX project_id = SQLReadNameTableExx( odbc, project_root, "project", "project_id", "root_path", TRUE );
		PTABLE table;

		snprintf( tmp, 256, "project_version_%d_%d", l.version_id, project_id );

		snprintf( tmp_create, 256, create_project_version_table, tmp );

		table = GetFieldsInSQL( tmp_create, FALSE );
		CheckODBCTable( odbc, table, CTO_MERGE );
		DestroySQLTable( table );

		return project_id;
	}
}


static void CPROC CaptureMercurialVersion( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	TEXTSTR *result = (TEXTSTR*)psv;
	(*result) = StrDup( buffer );
	if( (*result)[size-1] == '\n' )
		(*result)[size-1] = 0;
}


INDEX GetBuildVersion( PODBC odbc, INDEX project_id )
{
	INDEX project_build_id = INVALID_INDEX;
	CTEXTSTR result;
	CTEXTSTR *result_value;
	TEXTCHAR tmp[256];

	InitVersion( odbc );

	snprintf( tmp, 256, "project_version_%d_%d", l.version_id, project_id );
	if( System( "hg id -i", CaptureMercurialVersion, (uintptr_t)(&result) ) )
	{
		SQLCommandf( odbc, "insert into %s(project_id,hg_id,build_id,version_id)values(%d,'%s',%d,%d)"
					  , tmp
					  , project_id
					  , result
					  , g.build_id
					  , l.version_id );
		project_build_id = FetchLastInsertID( odbc, tmp, "project_version_id" );
		SQLRecordQueryf( odbc, NULL, &result_value, NULL
							, "select version_string from %s join version using(version_id) where project_version_id=%d"
							, tmp
							, project_build_id );

		if( g.flags.use_common_build )
		{
			snprintf( tmp, 256, "hg tag -m \"Tag Version %s.%d\" %s.%d"
					  , result_value[0]
					  , g.build_id
					  , result_value[0]
					  , g.build_id );
		}
		else if( g.flags.use_all_build )
		{
			snprintf( tmp, 256, "hg tag -m \"Tag Version %s.%d.%d\" %s.%d.%d"
					  , result_value[0]
					  , g.build_id
					  , project_build_id
					  , result_value[0]
					  , g.build_id
					  , project_build_id );
		}
		else
		{
			snprintf( tmp, 256, "hg tag -m \"Tag Version %s.%d\" %s.%d"
					  , result_value[0]
					  , project_build_id
					  , result_value[0]
					  , project_build_id );
		}
		System( tmp, NULL, 0 );
	}

	
	return project_build_id;
}

