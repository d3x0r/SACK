
#include <stdhdrs.h>
#include <deadstart.h>
#include <pssql.h>


#define Check(n) CheckODBCTable( odbc, &n, CTO_MERGE );

 
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
	, { WIDE("height") , WIDE("int(10)") , WIDE("unsigned NOT NULL default '0'") }
	, { WIDE("linked_to_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("linked_to_x") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("linked_to_y") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("linked_from_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("linked_from_x") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("linked_from_y") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
	, { WIDE("stacked_on_id") , WIDE("int(11)") , WIDE("NOT NULL default '0'") }
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
	required_key_def( 0, 1, WIDE("neuronkey"), WIDE("brain_neuron_id") )
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

void CheckTables( PODBC odbc )
{

	Check(board_info          );
	Check(board_input_output  );
	Check(board_layer         );
	Check(board_layer_link    );
	Check(board_layer_neuron  );
	Check(board_layer_path    );
	Check(board_layer_synapse );
}


 


