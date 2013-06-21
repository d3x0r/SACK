
#include <stdhdrs.h>
#include <pssql.h>

int main( void )
{
	PODBC connections[3];
   LOGICAL init_database = 0;
	if( GetSizeofFile( "test.db", 0 ) == (_32)-1 )
		init_database = 1;

	connections[0] = ConnectToDatabase( "test.db" );
	if( init_database )
	{
      SQLCommandf( connections[0], "create table table1 (ID int)" );
      SQLCommandf( connections[0], "create table table2 (ID int, unique (ID) )" );
      SQLCommandf( connections[0], "create table table3 (ID int, unique (ID) )" );
      SQLCommandf( connections[0], "create table table4 (ID int)" );
      SQLCommandf( connections[0], "create table table5 (ID int)" );
      SQLCommandf( connections[0], "create table table6 (ID int)" );




	}
   connections[1] = ConnectToDatabase( "test.db" );
   SetSQLAutoTransact( connections[1], TRUE );
   connections[2] = ConnectToDatabase( "test.db" );
   SetSQLAutoTransact( connections[2], TRUE );


         SQLCommandf( connections[1], "insert into option4_name (name_id,name) values( '243a022e-dac5-11e2-9d2f-7054d2ab9d3c','Frame border image' )" );      SQLCommandf( connections[1], "Insert into option4_map(`option_id`,`parent_option_id`,`name_id`) values ('243a022f-dac5-11e2-9d2f-7054d2ab9d3c','405b3bdd-d9c2-11e2-9f56-7054d2ab9d3c','243a022e-dac5-11e2-9d2f-7054d2ab9d3c')" );      SQLCommandf( connections[1], "replace into option4_values (`option_id`,`string`,`segment` ) values ('243a022f-dac5-11e2-9d2f-7054d2ab9d3c','frame_border.png',0)" );      SQLCommandf( connections[1], "delete from option4_values where `option_id`='243a022f-dac5-11e2-9d2f-7054d2ab9d3c' and segment > 0" );      SQLCommandf( connections[1], "insert into option4_name (name_id,name) values( '243a0230-dac5-11e2-9d2f-7054d2ab9d3c','file group' )" );      SQLCommandf( connections[1], "Insert into option4_map(`option_id`,`parent_option_id`,`name_id`) values ('243a0231-dac5-11e2-9d2f-7054d2ab9d3c','405b3b62-d9c2-11e2-9f56-7054d2ab9d3c','243a0230-dac5-11e2-9d2f-7054d2ab9d3c')" );      SQLCommandf( connections[1], "insert into option4_name (name_id,name) values( '243a0232-dac5-11e2-9d2f-7054d2ab9d3c','Images' )" );      SQLCommandf( connections[1], "Insert into option4_map(`option_id`,`parent_option_id`,`name_id`) values ('243a0233-dac5-11e2-9d2f-7054d2ab9d3c','243a0231-dac5-11e2-9d2f-7054d2ab9d3c','243a0232-dac5-11e2-9d2f-7054d2ab9d3c')" );      SQLCommandf( connections[1], "replace into option4_values (`option_id`,`string`,`segment` ) values ('243a0233-dac5-11e2-9d2f-7054d2ab9d3c','./Images',0)" );      SQLCommandf( connections[1], "delete from option4_values where `option_id`='243a0233-dac5-11e2-9d2f-7054d2ab9d3c' and segment > 0" );
      SQLCommandf( connections[2], "insert into option4_name (name_id,name) values( '243a0234-dac5-11e2-9d2f-7054d2ab9d3c','Video Render' )" );
#if 0
//BeginTransact( connections[1] );
	//SQLCommandf( connections[1], "begin transaction" );

	SQLCommandf( connections[1], "insert into table1(ID)values(0)" );
	SQLCommandf( connections[1], "insert into table3(ID)values(0)" );
	SQLCommandf( connections[1], "replace into table2 (ID)values(2)" );
	SQLCommandf( connections[1], "delete from table2 where ID > 55" );
	SQLCommandf( connections[1], "insert into table1(ID)values(2)" );
	SQLCommandf( connections[1], "insert into table3(ID)values(2)" );
	SQLCommandf( connections[1], "insert into table1(ID)values(3)" );
	SQLCommandf( connections[1], "insert into table3(ID)values(3)" );
	SQLCommandf( connections[1], "replace into table2 (ID)values(3)" );
	SQLCommandf( connections[1], "delete from table2 where ID > 55" );

	//BeginTransact( connections[2] );
	//SQLCommandf( connections[2], "begin transaction" );
	SQLCommandf( connections[2], "insert into table1(ID)values(1)" );

	//SQLCommandf( connections[1], "COMMIT" );

#endif

   return 0;
}
