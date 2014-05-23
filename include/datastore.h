
#ifndef DATASTORE_DEFINED
#define DATASTORE_DEFINED
#include <sack_types.h>




EXPORTED_DATA INDEX iTransform;

// define a type data... it's just a amorphous blob at this point...
// later structure member offsets may be defined such that
// this object and it's related members may be continuously related across
// a cluster...

INDEX   DataStore_RegisterNamedDataType( CTEXTSTR name, _32 size );
// make an isntance of a data type...
POINTER DataStore_CreateDataType( INDEX iType );

//  indicate that a member of the data type is a set storage device
// the index result may be used to create a member of this set on an object
INDEX   DataStore_CreateDataSetLinkEx( INDEX iType, _32 offsetof_set_pointer, _32 setsize, _32 setunits, _32 maxcnt );
// a simple macro which uses ( iCluster, OBJECT, objects ) to know how to create the set of what type of object at which member
#define DataStore_CreateDataSetLink( iType, type, membername )  DataStore_CreateDataSetLinkEx( iType, offsetof( type, objects ), sizeof( type##SET ), sizeof( type ), MAX##type##SPERSET );
POINTER DataStore_GetFromDataSet( INDEX iType, POINTER member, INDEX iSet );

// define a member of the object which is a link (pointer to) a seperate
// type instance... and the expected type of data...
// provided for sanity checks.

// the iTypeLinked may be depricated in a performance model that can be trusted
INDEX   DataStore_CreateLink( INDEX iType, _32 offsetof_pointer, INDEX iTypeLinked );
// set link uses iOtherType and pOther member to know which member on the remote sides
// should be linked...
POINTER DataStore_SetLink( INDEX iType, POINTER member, _32 iLink, _32 iOtherType, POINTER othermember )



#endif
