
#include <stdhdrs.h>
#include <deadstart.h>
#include <pssql.h>


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


 


