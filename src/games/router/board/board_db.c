
#include <stdhdrs.h>
#include <deadstart.h>
#include <pssql.h>

//#define BUILD_FROM_SOURCE

#ifdef BUILD_FROM_SOURCE
#define Check(n) table = GetFieldsInSQL( n, FALSE ); CheckODBCTable( odbc, table, CTO_MERGE );

#define TABLE_TYPE " )" //TYPE=MyISAM";  // can't handle parsing this
// and database differences causes this to be invalid anyhow

char *board_info = "CREATE TABLE `board_info` ("
"  `board_info_id` int(11) NOT NULL auto_increment,"
"  `board_name` varchar(100) NOT NULL default '',"
"  PRIMARY KEY  (`board_info_id`)"
TABLE_TYPE;

char *board_input_output = "CREATE TABLE `board_input_output` ("
"  `board_input_output_id` int(11) NOT NULL auto_increment,"
"  `board_id` int(11) NOT NULL default '0',"
"  `board_layer_id` int(11) NOT NULL default '0',"
"  `component` varchar(100) NOT NULL default '',"
"  `connector` varchar(100) NOT NULL default '',"
"  PRIMARY KEY  (`board_input_output_id`)"
TABLE_TYPE;

char *board_layer = "CREATE TABLE `board_layer` ("
"  `board_layer_id` int(11) NOT NULL auto_increment,"
"  `board_info_id` int(11) NOT NULL default '0',"
"  `peice_type` varchar(100) NOT NULL default '',"
"  `peice_info_id` int(11) NOT NULL default '0',"
"  `route` tinyint(4) NOT NULL default '0',"
"  `x` int(11) NOT NULL default '0',"
"  `y` int(11) NOT NULL default '0',"
"  `min_x` int(11) NOT NULL default '0',"
"  `min_y` int(11) NOT NULL default '0',"
"  `width` int(10) unsigned NOT NULL default '0',"
"  `height` int(10) unsigned NOT NULL default '0',"
"  `linked_to_id` int(11) NOT NULL default '0',"
"  `linked_to_x` int(11) NOT NULL default '0',"
"  `linked_to_y` int(11) NOT NULL default '0',"
"  `linked_from_id` int(11) NOT NULL default '0',"
"  `linked_from_x` int(11) NOT NULL default '0',"
"  `linked_from_y` int(11) NOT NULL default '0',"
"  `stacked_on_id` int(11) NOT NULL default '0',"
"  PRIMARY KEY  (`board_layer_id`)"
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'board_layer_link'
//#
char *board_layer_link = "CREATE TABLE `board_layer_link` ("
"  `board_layer_id` int(11) NOT NULL default '0',"
"  `board_info_id` int(11) NOT NULL default '0',"
"  PRIMARY KEY  (`board_layer_id`)"
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'board_layer_neuron'
//#
char *board_layer_neuron = "CREATE TABLE `board_layer_neuron` ("
"  `brain_neuron_id` int(11) NOT NULL default '0',"
"  `board_layer_id` int(11) NOT NULL default '0',"
"  PRIMARY KEY  (`brain_neuron_id`,`board_layer_id`)"
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'board_layer_path'
//#
char *board_layer_path = "CREATE TABLE `board_layer_path` ("
"  `board_layer_path_id` int(11) NOT NULL auto_increment,"
"  `board_layer_id` int(11) NOT NULL default '0',"
"  `x` int(11) NOT NULL default '0',"
"  `y` int(11) NOT NULL default '0',"
"  `fore` tinyint(4) NOT NULL default '0',"
"  `back` tinyint(4) NOT NULL default '0',"
"  PRIMARY KEY  (`board_layer_path_id`)"
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'board_layer_synapse'
//#
char *board_layer_synapse = "CREATE TABLE `board_layer_synapse` ("
"  `board_layer_id` int(11) NOT NULL default '0',"
"  `brain_synapse_id` int(11) NOT NULL default '0',"
"  PRIMARY KEY  (`board_layer_id`)"
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'brain_info'
//#
char *brain_info = "CREATE TABLE `brain_info` ("
"  `brain_info_id` int(11) NOT NULL auto_increment,"
"  `brain_name` varchar(100) NOT NULL default '',"
"  `version` int(11) NOT NULL default '0',"
"  `k` double NOT NULL default '0',"
"  PRIMARY KEY  (`brain_info_id`)"
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'brain_neuron'
//#
char *brain_neuron = "CREATE TABLE `brain_neuron` ("
"  `brain_neuron_id` int(11) NOT NULL auto_increment,"
"  `brain_info_id` int(11) NOT NULL default '0',"
"  `parent_id` int(11) NOT NULL default '0',"
"  `type` int(11) NOT NULL default '0',"
"  `threshold` double NOT NULL default '0',"
"  `min_output` double NOT NULL default '0',"
"  `max_output` double NOT NULL default '0',"
"  PRIMARY KEY  (`brain_neuron_id`)"
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'brain_synapse'
//#
char *brain_synapse = "CREATE TABLE `brain_synapse` ("
"  `brain_synapse_id` int(11) NOT NULL auto_increment,"
"  `brain_info_id` int(11) NOT NULL default '0',"
"  `brain_neuron_id_from` int(11) NOT NULL default '0',"
"  `brain_neuron_id_to` int(11) NOT NULL default '0',"
"  `synapse_gain` double NOT NULL default '0',"
"  PRIMARY KEY  (`brain_synapse_id`)"
	TABLE_TYPE;

char *brain_connectors = "CREATE TABLE `brain_connectors` ("
"  `brain_connector_id` int(11) NOT NULL auto_increment,"
"  `connector_name` varchar(100) NOT NULL default '',"
"  `parent_id` int(11) NOT NULL default '0',"
"  `input` int(11) NOT NULL default '0',"
	"  PRIMARY KEY  (`brain_connector_id`)"
   TABLE_TYPE;

#else
#define Check(n) CheckODBCTable( odbc, &n, CTO_MERGE );

 
//--------------------------------------------------------------------------
// board_info 
// Total number of fields = 2
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_info_fields[] = { { "board_info_id" , "int(11)" , "NOT NULL auto_increment" }
	, { "board_name" , "varchar(100)" , "NOT NULL default ''" }
};

DB_KEY_DEF board_info_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, "board_info_id" )
#else
	{ {1,0}, NULL, { "board_info_id" } }
#endif
};

TABLE board_info = { "board_info" 
	 , FIELDS( board_info_fields )
	 , TABLE_KEYS( board_info_keys ) };
  
//--------------------------------------------------------------------------
// board_input_output 
// Total number of fields = 5
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_input_output_fields[] = { { "board_input_output_id" , "int(11)" , "NOT NULL auto_increment" }
	, { "board_id" , "int(11)" , "NOT NULL default '0'" }
	, { "board_layer_id" , "int(11)" , "NOT NULL default '0'" }
	, { "component" , "varchar(100)" , "NOT NULL default ''" }
	, { "connector" , "varchar(100)" , "NOT NULL default ''" }
};

DB_KEY_DEF board_input_output_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, "board_input_output_id" )
#else
	{ {1,0}, NULL, { "board_input_output_id" } }
#endif
};


TABLE board_input_output = { "board_input_output" 
	 , FIELDS( board_input_output_fields )
	 , TABLE_KEYS( board_input_output_keys ) };
  
//--------------------------------------------------------------------------
// board_layer 
// Total number of fields = 17
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_layer_fields[] = { { "board_layer_id" , "int(11)" , "NOT NULL auto_increment" }
	, { "board_info_id" , "int(11)" , "NOT NULL default '0'" }
	, { "peice_type" , "varchar(100)" , "NOT NULL default ''" }
	, { "peice_info_id" , "int(11)" , "NOT NULL default '0'" }
	, { "route" , "tinyint(4)" , "NOT NULL default '0'" }
	, { "x" , "int(11)" , "NOT NULL default '0'" }
	, { "y" , "int(11)" , "NOT NULL default '0'" }
	, { "min_x" , "int(11)" , "NOT NULL default '0'" }
	, { "min_y" , "int(11)" , "NOT NULL default '0'" }
	, { "width" , "int(10)" , "unsigned NOT NULL default '0'" }
	, { "height" , "int(10)" , "unsigned NOT NULL default '0'" }
	, { "linked_to_id" , "int(11)" , "NOT NULL default '0'" }
	, { "linked_to_x" , "int(11)" , "NOT NULL default '0'" }
	, { "linked_to_y" , "int(11)" , "NOT NULL default '0'" }
	, { "linked_from_id" , "int(11)" , "NOT NULL default '0'" }
	, { "linked_from_x" , "int(11)" , "NOT NULL default '0'" }
	, { "linked_from_y" , "int(11)" , "NOT NULL default '0'" }
	, { "stacked_on_id" , "int(11)" , "NOT NULL default '0'" }
};

DB_KEY_DEF board_layer_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, "board_layer_id" )
#else
	{ {1,0}, NULL, { "board_layer_id" } }
#endif
};


TABLE board_layer = { "board_layer" 
	 , FIELDS( board_layer_fields )
	 , TABLE_KEYS( board_layer_keys ) };
  
//--------------------------------------------------------------------------
// board_layer_link 
// Total number of fields = 2
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_layer_link_fields[] = { { "board_layer_id" , "int(11)" , "NOT NULL default '0'" }
	, { "board_info_id" , "int(11)" , "NOT NULL default '0'" }
};

DB_KEY_DEF board_layer_link_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, "board_layer_id" )
#else
	{ {1,0}, NULL, { "board_layer_id" } }
#endif
};


TABLE board_layer_link = { "board_layer_link" 
	 , FIELDS( board_layer_link_fields )
	 , TABLE_KEYS( board_layer_link_keys ) };
  
//--------------------------------------------------------------------------
// board_layer_neuron 
// Total number of fields = 2
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_layer_neuron_fields[] = { { "brain_neuron_id" , "int(11)" , "NOT NULL default '0'" }
	, { "board_layer_id" , "int(11)" , "NOT NULL default '0'" }
};

DB_KEY_DEF board_layer_neuron_keys[] = { 
#ifdef __cplusplus
	required_key_def( 0, 1, "neuronkey", "brain_neuron_id" )
//, ... columns are short this is an error.
#else
	{ {1,0}, NULL, { "brain_neuron_id", "board_layer_id" } }
#endif
};


TABLE board_layer_neuron = { "board_layer_neuron" 
	 , FIELDS( board_layer_neuron_fields )
	 , TABLE_KEYS( board_layer_neuron_keys ) };
  
//--------------------------------------------------------------------------
// board_layer_path 
// Total number of fields = 6
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_layer_path_fields[] = { { "board_layer_path_id" , "int(11)" , "NOT NULL auto_increment" }
	, { "board_layer_id" , "int(11)" , "NOT NULL default '0'" }
	, { "x" , "int(11)" , "NOT NULL default '0'" }
	, { "y" , "int(11)" , "NOT NULL default '0'" }
	, { "fore" , "tinyint(4)" , "NOT NULL default '0'" }
	, { "back" , "tinyint(4)" , "NOT NULL default '0'" }
};

DB_KEY_DEF board_layer_path_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, "board_layer_path_id" )
#else
	{ {1,0}, NULL, { "board_layer_path_id" } }
#endif
};

TABLE board_layer_path = { "board_layer_path" 
	 , FIELDS( board_layer_path_fields )
	 , TABLE_KEYS( board_layer_path_keys ) };
  
//--------------------------------------------------------------------------
// board_layer_synapse 
// Total number of fields = 2
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_layer_synapse_fields[] = { { "board_layer_id" , "int(11)" , "NOT NULL default '0'" }
	, { "brain_synapse_id" , "int(11)" , "NOT NULL default '0'" }
};

DB_KEY_DEF board_layer_synapse_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, "board_layer_id" )
#else
	{ {1,0}, NULL, { "board_layer_id" } }
#endif
};


TABLE board_layer_synapse = { "board_layer_synapse" 
	 , FIELDS( board_layer_synapse_fields )
	 , TABLE_KEYS( board_layer_synapse_keys ) };

#if 0
//--------------------------------------------------------------------------
// brain_info 
// Total number of fields = 4
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD brain_info_fields[] = { { "brain_info_id" , "int(11)" , "NOT NULL auto_increment" }
	, { "brain_name" , "varchar(100)" , "NOT NULL default ''" }
	, { "version" , "int(11)" , "NOT NULL default '0'" }
	, { "k" , "double " , "NOT NULL default '0'" }
};

DB_KEY_DEF brain_info_keys[] = { { { 1 }, NULL , { "brain_info_id" } }
};


TABLE brain_info = { "brain_info" 
	 , FIELDS( brain_info_fields )
	 , TABLE_KEYS( brain_info_keys ) };
  
//--------------------------------------------------------------------------
// brain_neuron 
// Total number of fields = 7
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD brain_neuron_fields[] = { { "brain_neuron_id" , "int(11)" , "NOT NULL auto_increment" }
	, { "brain_info_id" , "int(11)" , "NOT NULL default '0'" }
	, { "parent_id" , "int(11)" , "NOT NULL default '0'" }
	, { "type" , "int(11)" , "NOT NULL default '0'" }
	, { "threshold" , "double " , "NOT NULL default '0'" }
	, { "min_output" , "double " , "NOT NULL default '0'" }
	, { "max_output" , "double " , "NOT NULL default '0'" }
};

DB_KEY_DEF brain_neuron_keys[] = { { { 1 }, NULL , { "brain_neuron_id" } }
};


TABLE brain_neuron = { "brain_neuron" 
	 , FIELDS( brain_neuron_fields )
	 , TABLE_KEYS( brain_neuron_keys ) };
  
//--------------------------------------------------------------------------
// brain_synapse 
// Total number of fields = 5
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD brain_synapse_fields[] = { { "brain_synapse_id" , "int(11)" , "NOT NULL auto_increment" }
	, { "brain_info_id" , "int(11)" , "NOT NULL default '0'" }
	, { "brain_neuron_id_from" , "int(11)" , "NOT NULL default '0'" }
	, { "brain_neuron_id_to" , "int(11)" , "NOT NULL default '0'" }
	, { "synapse_gain" , "double " , "NOT NULL default '0'" }
};

DB_KEY_DEF brain_synapse_keys[] = { { { 1 }, NULL , { "brain_synapse_id" } }
};


TABLE brain_synapse = { "brain_synapse" 
	 , FIELDS( brain_synapse_fields )
	 , TABLE_KEYS( brain_synapse_keys ) };

//--------------------------------------------------------------------------
// brain_connectors 
// Total number of fields = 4
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD brain_connectors_fields[] = { { "brain_connector_id" , "int(11)" , "NOT NULL auto_increment" }
											 , { "connector_name" , "varchar(100)" , "NOT NULL default ''" }
											 , { "parent_id" , "int(11)" , "NOT NULL default '0'" }
											 , { "input" , "int(11)" , "NOT NULL default '0'" } };
	 DB_KEY_DEF brain_connectors_keys[] = { { { 1 }, NULL , { "brain_connector_id" } } };
TABLE brain_connectors = { "brain_connectors" 
	 , FIELDS( brain_connectors_fields )
	 , TABLE_KEYS( brain_connectors_keys ) };
#endif
#endif

void CheckTables( PODBC odbc )
{
#ifdef BUILD_FROM_SOURCE
	PTABLE table;
#endif
	Check(board_info          );
	Check(board_input_output  );
	Check(board_layer         );
	Check(board_layer_link    );
	Check(board_layer_neuron  );
	Check(board_layer_path    );
	Check(board_layer_synapse );
#if 0
	Check(brain_info          );
	Check(brain_neuron        );
	Check(brain_synapse       );
	Check(brain_connectors );
#endif
}


 


