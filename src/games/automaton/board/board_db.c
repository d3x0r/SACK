
#include <stdhdrs.h>
#include <deadstart.h>
#include <pssql.h>

//#define BUILD_FROM_SOURCE

#ifdef BUILD_FROM_SOURCE
#define Check(n) table = GetFieldsInSQL( n, FALSE ); CheckODBCTable( NULL, table, CTO_MERGE );

#define TABLE_TYPE WIDE(" )") //TYPE=MyISAM";  // can't handle parsing this
// and database differences causes this to be invalid anyhow

TEXTCHAR *board_info = WIDE("CREATE TABLE `board_info` (")
WIDE("  `board_info_id` int(11) NOT NULL auto_increment,")
WIDE("  `board_name` varchar(100) NOT NULL default '',")
WIDE("  PRIMARY KEY  (`board_info_id`)")
TABLE_TYPE;

TEXTCHAR *board_input_output = WIDE("CREATE TABLE `board_input_output` (")
WIDE("  `board_input_output_id` int(11) NOT NULL auto_increment,")
WIDE("  `board_id` int(11) NOT NULL default '0',")
WIDE("  `board_layer_id` int(11) NOT NULL default '0',")
WIDE("  `component` varchar(100) NOT NULL default '',")
WIDE("  `connector` varchar(100) NOT NULL default '',")
WIDE("  PRIMARY KEY  (`board_input_output_id`)")
TABLE_TYPE;

TEXTCHAR *board_layer = WIDE("CREATE TABLE `board_layer` (")
WIDE("  `board_layer_id` int(11) NOT NULL auto_increment,")
WIDE("  `board_info_id` int(11) NOT NULL default '0',")
WIDE("  `peice_type` varchar(100) NOT NULL default '',")
WIDE("  `peice_info_id` int(11) NOT NULL default '0',")
WIDE("  `route` tinyint(4) NOT NULL default '0',")
WIDE("  `x` int(11) NOT NULL default '0',")
WIDE("  `y` int(11) NOT NULL default '0',")
WIDE("  `min_x` int(11) NOT NULL default '0',")
WIDE("  `min_y` int(11) NOT NULL default '0',")
WIDE("  `width` int(10) unsigned NOT NULL default '0',")
WIDE("  `linked_to_id` int(11) NOT NULL default '0',")
WIDE("  `linked_to_x` int(11) NOT NULL default '0',")
WIDE("  `linked_to_y` int(11) NOT NULL default '0',")
WIDE("  `linked_from_id` int(11) NOT NULL default '0',")
WIDE("  `linked_from_x` int(11) NOT NULL default '0',")
WIDE("  `linked_from_y` int(11) NOT NULL default '0',")
WIDE("  `height` int(10) unsigned NOT NULL default '0',")
WIDE("  PRIMARY KEY  (`board_layer_id`)")
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'board_layer_link'
//#
TEXTCHAR *board_layer_link = WIDE("CREATE TABLE `board_layer_link` (")
WIDE("  `board_layer_id` int(11) NOT NULL default '0',")
WIDE("  `board_info_id` int(11) NOT NULL default '0',")
WIDE("  PRIMARY KEY  (`board_layer_id`)")
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'board_layer_neuron'
//#
TEXTCHAR *board_layer_neuron = WIDE("CREATE TABLE `board_layer_neuron` (")
WIDE("  `brain_neuron_id` int(11) NOT NULL default '0',")
WIDE("  `board_layer_id` int(11) NOT NULL default '0',")
WIDE("  PRIMARY KEY  (`brain_neuron_id`,`board_layer_id`)")
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'board_layer_path'
//#
TEXTCHAR *board_layer_path = WIDE("CREATE TABLE `board_layer_path` (")
WIDE("  `board_layer_path_id` int(11) NOT NULL auto_increment,")
WIDE("  `board_layer_id` int(11) NOT NULL default '0',")
WIDE("  `x` int(11) NOT NULL default '0',")
WIDE("  `y` int(11) NOT NULL default '0',")
WIDE("  `fore` tinyint(4) NOT NULL default '0',")
WIDE("  `back` tinyint(4) NOT NULL default '0',")
WIDE("  PRIMARY KEY  (`board_layer_path_id`)")
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'board_layer_synapse'
//#
TEXTCHAR *board_layer_synapse = WIDE("CREATE TABLE `board_layer_synapse` (")
WIDE("  `board_layer_id` int(11) NOT NULL default '0',")
WIDE("  `brain_synapse_id` int(11) NOT NULL default '0',")
WIDE("  PRIMARY KEY  (`board_layer_id`)")
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'brain_info'
//#
TEXTCHAR *brain_info = WIDE("CREATE TABLE `brain_info` (")
WIDE("  `brain_info_id` int(11) NOT NULL auto_increment,")
WIDE("  `brain_name` varchar(100) NOT NULL default '',")
WIDE("  `version` int(11) NOT NULL default '0',")
WIDE("  `k` double NOT NULL default '0',")
WIDE("  PRIMARY KEY  (`brain_info_id`)")
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'brain_neuron'
//#
TEXTCHAR *brain_neuron = WIDE("CREATE TABLE `brain_neuron` (")
WIDE("  `brain_neuron_id` int(11) NOT NULL auto_increment,")
WIDE("  `brain_info_id` int(11) NOT NULL default '0',")
WIDE("  `parent_id` int(11) NOT NULL default '0',")
WIDE("  `type` int(11) NOT NULL default '0',")
WIDE("  `threshold` double NOT NULL default '0',")
WIDE("  `min_output` double NOT NULL default '0',")
WIDE("  `max_output` double NOT NULL default '0',")
WIDE("  PRIMARY KEY  (`brain_neuron_id`)")
TABLE_TYPE; 

//#Host: localhost
//#Database: game_dev
//#Table: 'brain_synapse'
//#
TEXTCHAR *brain_synapse = WIDE("CREATE TABLE `brain_synapse` (")
WIDE("  `brain_synapse_id` int(11) NOT NULL auto_increment,")
WIDE("  `brain_info_id` int(11) NOT NULL default '0',")
WIDE("  `brain_neuron_id_from` int(11) NOT NULL default '0',")
WIDE("  `brain_neuron_id_to` int(11) NOT NULL default '0',")
WIDE("  `synapse_gain` double NOT NULL default '0',")
WIDE("  PRIMARY KEY  (`brain_synapse_id`)")
	TABLE_TYPE;

TEXTCHAR *brain_connectors = WIDE("CREATE TABLE `brain_connectors` (")
WIDE("  `brain_connector_id` int(11) NOT NULL auto_increment,")
WIDE("  `connector_name` varchar(100) NOT NULL default '',")
WIDE("  `parent_id` int(11) NOT NULL default '0',")
WIDE("  `input` int(11) NOT NULL default '0',")
	WIDE("  PRIMARY KEY  (`brain_connector_id`)")
   TABLE_TYPE;

#else
#define Check(n) CheckODBCTable( NULL, &n, CTO_MERGE );

 
//--------------------------------------------------------------------------
// board_info 
// Total number of fields = 2
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_info_fields[] = { { WIDE("board_info_id") , WIDE("int(11)") , WIDE("NOT NULL auto_increment") }
	, { WIDE("board_name") , WIDE("varchar(100)") , WIDE("NOT NULL default ''") }
};

DB_KEY_DEF board_info_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, WIDE("board_info_id") )
#else
	{ {1,0}, NULL, { WIDE("board_info_id") } }
#endif
};

TABLE board_info = { WIDE("board_info") 
	 , FIELDS( board_info_fields )
	 , TABLE_KEYS( board_info_keys ) };
  
//--------------------------------------------------------------------------
// board_input_output 
// Total number of fields = 5
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_input_output_fields[] = { { WIDE("board_input_output_id") , WIDE("int(11)") , WIDE("NOT NULL auto_increment") }
	, { WIDE("board_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("board_layer_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("component") , WIDE("varchar(100)") , WIDE("NOT NULL default ''") }
	, { WIDE("connector") , WIDE("varchar(100)") , WIDE("NOT NULL default ''") }
};

DB_KEY_DEF board_input_output_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, WIDE("board_input_output_id") )
#else
	{ {1,0}, NULL, { WIDE("board_input_output_id") } }
#endif
};


TABLE board_input_output = { WIDE("board_input_output") 
	 , FIELDS( board_input_output_fields )
	 , TABLE_KEYS( board_input_output_keys ) };
  
//--------------------------------------------------------------------------
// board_layer 
// Total number of fields = 17
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_layer_fields[] = { { WIDE("board_layer_id") , WIDE("int(11)") , WIDE("NOT NULL auto_increment") }
	, { WIDE("board_info_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("peice_type") , WIDE("varchar(100)") , WIDE("NOT NULL default ''") }
	, { WIDE("peice_info_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("route") , WIDE("tinyint(4)") , WIDE("NOT NULL default '0'") }
	, { WIDE("x") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("y") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("min_x") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("min_y") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("width") , WIDE("int(10)") , WIDE("unsigned NOT NULL default '0'") }
	, { WIDE("linked_to_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("linked_to_x") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("linked_to_y") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("linked_from_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("linked_from_x") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("linked_from_y") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("height") , WIDE("int(10)") , WIDE("unsigned NOT NULL default '0'") }
};

DB_KEY_DEF board_layer_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, WIDE("board_layer_id") )
#else
	{ {1,0}, NULL, { WIDE("board_layer_id") } }
#endif
};


TABLE board_layer = { WIDE("board_layer") 
	 , FIELDS( board_layer_fields )
	 , TABLE_KEYS( board_layer_keys ) };
  
//--------------------------------------------------------------------------
// board_layer_link 
// Total number of fields = 2
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_layer_link_fields[] = { { WIDE("board_layer_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("board_info_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
};

DB_KEY_DEF board_layer_link_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, WIDE("board_layer_id") )
#else
	{ {1,0}, NULL, { WIDE("board_layer_id") } }
#endif
};


TABLE board_layer_link = { WIDE("board_layer_link") 
	 , FIELDS( board_layer_link_fields )
	 , TABLE_KEYS( board_layer_link_keys ) };
  
//--------------------------------------------------------------------------
// board_layer_neuron 
// Total number of fields = 2
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_layer_neuron_fields[] = { { WIDE("brain_neuron_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("board_layer_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
};

DB_KEY_DEF board_layer_neuron_keys[] = { 
#ifdef __cplusplus
	required_key_def( 0, 1, NULL, WIDE("brain_neuron_id") )
//, ... columns are short this is an error.
#else
	{ {1,0}, NULL, { WIDE("brain_neuron_id"), WIDE("board_layer_id") } }
#endif
};


TABLE board_layer_neuron = { WIDE("board_layer_neuron") 
	 , FIELDS( board_layer_neuron_fields )
	 , TABLE_KEYS( board_layer_neuron_keys ) };
  
//--------------------------------------------------------------------------
// board_layer_path 
// Total number of fields = 6
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_layer_path_fields[] = { { WIDE("board_layer_path_id") , WIDE("int(11)") , WIDE("NOT NULL auto_increment") }
	, { WIDE("board_layer_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("x") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("y") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("fore") , WIDE("tinyint(4)") , WIDE("NOT NULL default '0'") }
	, { WIDE("back") , WIDE("tinyint(4)") , WIDE("NOT NULL default '0'") }
};

DB_KEY_DEF board_layer_path_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, WIDE("board_layer_path_id") )
#else
	{ {1,0}, NULL, { WIDE("board_layer_path_id") } }
#endif
};

TABLE board_layer_path = { WIDE("board_layer_path") 
	 , FIELDS( board_layer_path_fields )
	 , TABLE_KEYS( board_layer_path_keys ) };
  
//--------------------------------------------------------------------------
// board_layer_synapse 
// Total number of fields = 2
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD board_layer_synapse_fields[] = { { WIDE("board_layer_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("brain_synapse_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
};

DB_KEY_DEF board_layer_synapse_keys[] = { 
#ifdef __cplusplus
	required_key_def( 1, 0, NULL, WIDE("board_layer_id") )
#else
	{ {1,0}, NULL, { WIDE("board_layer_id") } }
#endif
};


TABLE board_layer_synapse = { WIDE("board_layer_synapse") 
	 , FIELDS( board_layer_synapse_fields )
	 , TABLE_KEYS( board_layer_synapse_keys ) };

#if 0
//--------------------------------------------------------------------------
// brain_info 
// Total number of fields = 4
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD brain_info_fields[] = { { WIDE("brain_info_id") , WIDE("int(11)") , WIDE("NOT NULL auto_increment") }
	, { WIDE("brain_name") , WIDE("varchar(100)") , WIDE("NOT NULL default ''") }
	, { WIDE("version") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("k") , WIDE("double ") , WIDE("NOT NULL default '0'") }
};

DB_KEY_DEF brain_info_keys[] = { { { 1 }, NULL , { WIDE("brain_info_id") } }
};


TABLE brain_info = { WIDE("brain_info") 
	 , FIELDS( brain_info_fields )
	 , TABLE_KEYS( brain_info_keys ) };
  
//--------------------------------------------------------------------------
// brain_neuron 
// Total number of fields = 7
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD brain_neuron_fields[] = { { WIDE("brain_neuron_id") , WIDE("int(11)") , WIDE("NOT NULL auto_increment") }
	, { WIDE("brain_info_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("parent_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("type") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("threshold") , WIDE("double ") , WIDE("NOT NULL default '0'") }
	, { WIDE("min_output") , WIDE("double ") , WIDE("NOT NULL default '0'") }
	, { WIDE("max_output") , WIDE("double ") , WIDE("NOT NULL default '0'") }
};

DB_KEY_DEF brain_neuron_keys[] = { { { 1 }, NULL , { WIDE("brain_neuron_id") } }
};


TABLE brain_neuron = { WIDE("brain_neuron") 
	 , FIELDS( brain_neuron_fields )
	 , TABLE_KEYS( brain_neuron_keys ) };
  
//--------------------------------------------------------------------------
// brain_synapse 
// Total number of fields = 5
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD brain_synapse_fields[] = { { WIDE("brain_synapse_id") , WIDE("int(11)") , WIDE("NOT NULL auto_increment") }
	, { WIDE("brain_info_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("brain_neuron_id_from") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("brain_neuron_id_to") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("synapse_gain") , WIDE("double ") , WIDE("NOT NULL default '0'") }
};

DB_KEY_DEF brain_synapse_keys[] = { { { 1 }, NULL , { WIDE("brain_synapse_id") } }
};


TABLE brain_synapse = { WIDE("brain_synapse") 
	 , FIELDS( brain_synapse_fields )
	 , TABLE_KEYS( brain_synapse_keys ) };

//--------------------------------------------------------------------------
// brain_connectors 
// Total number of fields = 4
// Total number of keys = 1
//--------------------------------------------------------------------------

FIELD brain_connectors_fields[] = { { WIDE("brain_connector_id") , WIDE("int(11)") , WIDE("NOT NULL auto_increment") }
											 , { WIDE("connector_name") , WIDE("varchar(100)") , WIDE("NOT NULL default ''") }
											 , { WIDE("parent_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
											 , { WIDE("input") , WIDE("int(11)") , WIDE("NOT NULL default '0'") } };
	 DB_KEY_DEF brain_connectors_keys[] = { { { 1 }, NULL , { WIDE("brain_connector_id") } } };
TABLE brain_connectors = { WIDE("brain_connectors")
	 , FIELDS( brain_connectors_fields )
	 , TABLE_KEYS( brain_connectors_keys ) };
#endif
#endif
PRELOAD( CheckTables )
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

 


