#ifndef VIRTUALITY_DEFINITON
#define VIRTUALITY_DEFINITON


#ifdef VIRTUALITY_LIBRARY_SOURCE
#define VIRTUALITY_EXPORT EXPORT_METHOD
#else
#define VIRTUALITY_EXPORT IMPORT_METHOD
#endif


//-----------------------------------------
// standard includes - need these things to do much of anything...

typedef struct object_tag OBJECT, *POBJECT;


#include <virtuality/plane.h>
#include <virtuality/object.h>
#include <virtuality/view.h>

#if !defined( DEFINE_SHAPES )&& !defined( VIRTUALITY_LIBRARY_SOURCE )
// these shapes need to be defined this way cause watcom compiler sucks
// for data export definitions.
#define CUBE_SIDES 6
VIRTUALITY_EXPORT BASIC_PLANE CubeNormals[];

#endif

VIRTUALITY_EXPORT void ExportVRML( POBJECT object );
VIRTUALITY_EXPORT void DeclareObject( CTEXTSTR object_name );


// static PTRSZVAL OnInitObject( "Cube" )( POBJECT object )
// gives Cube a chance to initialize a prototype object
// this object will be used later for making instances of Cube.
// This routine should load the plane definitions and compile in the object given, and
// it may return a custom value which will be returned for all other methods dealling
// with the class of object.
#define OnInitObject(name) \
	__DefineRegistryMethod("sack/game",InitObject,WIDE("virtuality"),name,WIDE("InitObject"),PTRSZVAL,(POBJECT object),__LINE__)

// static PTRSZVAL OnMakeObject( "cube" )( PTRSZVAL psv_init, POBJECT new_object )
// When an instance is created, after the object has been cloned, the new object is given to the class
#define OnMakeObject(name) \
	__DefineRegistryMethod("sack/game",MakeObject,WIDE("virtuality"),name,WIDE("MakeObject"),PTRSZVAL,(PTRSZVAL object, POBJECT new_object),__LINE__)



#endif
