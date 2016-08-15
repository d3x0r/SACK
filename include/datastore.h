
#ifndef DATASTORE_DEFINED
#define DATASTORE_DEFINED
#include <sack_types.h>




EXPORTED_DATA INDEX iTransform;

// define a type data... it's just a amorphous blob at this point...
// later structure member offsets may be defined such that
// this object and it's related members may be continuously related across
// a cluster...

INDEX   DataStore_RegisterNamedDataType( CTEXTSTR name, uint32_t size );
// make an isntance of a data type...
POINTER DataStore_CreateDataType( INDEX iType );

//  indicate that a member of the data type is a set storage device
// the index result may be used to create a member of this set on an object
INDEX   DataStore_CreateDataSetLinkEx( INDEX iType, uint32_t offsetof_set_pointer, uint32_t setsize, uint32_t setunits, uint32_t maxcnt );
// a simple macro which uses ( iCluster, OBJECT, objects ) to know how to create the set of what type of object at which member
#define DataStore_CreateDataSetLink( iType, type, membername )  DataStore_CreateDataSetLinkEx( iType, offsetof( type, objects ), sizeof( type##SET ), sizeof( type ), MAX##type##SPERSET );
POINTER DataStore_GetFromDataSet( INDEX iType, POINTER member, INDEX iSet );

// define a member of the object which is a link (pointer to) a seperate
// type instance... and the expected type of data...
// provided for sanity checks.

// the iTypeLinked may be depricated in a performance model that can be trusted
INDEX   DataStore_CreateLink( INDEX iType, uint32_t offsetof_pointer, INDEX iTypeLinked );
// set link uses iOtherType and pOther member to know which member on the remote sides
// should be linked...
POINTER DataStore_SetLink( INDEX iType, POINTER member, uint32_t iLink, uint32_t iOtherType, POINTER othermember )



#endif
