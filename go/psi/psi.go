package psi
//package psi

/*
#cgo LDFLAGS: -lbag.psi -lbag
//#cgo LDFLAGS: -Lc:/general/build/mingw/sack/debug_out/core/lib
//#cgo CFLAGS:-D_DEBUG
//#cgo CFLAGS: -IM:/sack/include
#include <psi.h>
#include "psi_methods.h"

int RegisterControl_cgo( const char *name );
void SetUserData_cgo( PSI_CONTROL pc, void *p );
void *GetUserData_cgo( PSI_CONTROL pc );
void *GetCustomData_cgo( PSI_CONTROL pc );
void SetCustomData_cgo( PSI_CONTROL pc, void *p );

int ControlMethodInit_cgo( PSI_CONTROL pc );
int ControlMethodMouse_cgo( PSI_CONTROL pc, S_32 x, S_32 y, _32 b );
int ControlMethodDraw_cgo( PSI_CONTROL pc );

int MouseCallback_cgo(PSI_CONTROL pc, S_32 x, S_32 y, _32 b); // Forward declaration.

*/
import "C"
import "unsafe"
import "fmt"
//import "strings"
import "sync"
import "sack/image"

const (
	BORDER_RESIZABLE = C.BORDER_RESIZABLE
)

var create_mutex = &sync.Mutex{};
var creating *Control

type ControlMethods interface {
	OnInit() int
	OnMouse( x int32, y int32, b uint32 ) int
	OnDraw() int
}

type ControlManualMethods interface {
	OnInit() int
	OnMouse( x int32, y int32, b uint32 ) int
}

type ControlType struct {
	//name string
	//c_name *C.char
	id int
}


type Control struct {
	methods ControlMethods
	control C.PSI_CONTROL
	surface image.Image
}

// ------------ Globals -----------------
var control_name *C.char = C.CString( "Go Control" )
var control_type ControlType = ControlType{};

// ------------ Registration -------------------

func init() {
	var strings []*C.char// = [0]*C.char{}
        
	fmt.Println( "PSI Init called? ");
	//ct.c_name = C.CString( name );
	control_type.id = int(C.RegisterControl_cgo( control_name ))
	
	root := "psi/control/Go Control/rtti/";
        
        cRoot := C.CString( root )
	strings = append( strings, cRoot )
	
        cTypeInt := C.CString( "int" )
	strings = append( strings, cTypeInt )
        
        cMouse := C.CString( "mouse" )
	strings = append( strings, cMouse )
        cDraw := C.CString( "draw" )
	strings = append( strings, cDraw )
        cInit := C.CString( "init" )
	strings = append( strings, cInit )
        cInitArgs := C.CString( "(PSI_CONTROL)" )
	strings = append( strings, cInitArgs )
        cMouseArgs := C.CString( "(PSI_CONTROL,S_32,S_32,_32)" )
	strings = append( strings, cMouseArgs )

	
	C.RegisterFunctionExx( nil, cRoot, cInit, cTypeInt, (C.PROCEDURE)(unsafe.Pointer(C.ControlMethodInit_cgo)),  cInitArgs, nil, nil );
	C.RegisterFunctionExx( nil, cRoot, cMouse, cTypeInt, (C.PROCEDURE)(unsafe.Pointer(C.ControlMethodMouse_cgo)),  cMouseArgs, nil, nil );
	C.RegisterFunctionExx( nil, cRoot, cDraw, cTypeInt, (C.PROCEDURE)(unsafe.Pointer(C.ControlMethodDraw_cgo)),  cInitArgs, nil, nil );
	
        for i := range strings {
        	C.free( unsafe.Pointer( strings[i] ) );
        }
}

// ---------- Default Methods ------------

func (ct *Control)OnInit() int {
	fmt.Println( "default init; fail." );
	return 0;
}

func (ct *Control)OnMouse( x int32, y int32, b uint32 ) int {
       fmt.Println("default mouse callback")
	return 0;
}

func (ct *Control)OnDraw() int {
	return 0;
}

//------------------------------------------

func CreateFrame( caption string, x int, y int, w uint, h uint, border_flags uint ) *Control {
	c_caption := C.CString( caption )
	c := Control{ control: C.CreateFrame( c_caption, C.int(x), C.int(y), C.int(w), C.int(h), C._32(border_flags), nil ) }
	C.free( unsafe.Pointer( c_caption ) );
	return &c
}


func (parent *Control)AddControl( control_type string, x int, y int, w uint, h uint ) {
	var c *Control = &Control{}
	
	c_type := C.CString( control_type )
	
	fmt.Println( "c.control is", c.control )
	
	fmt.Println( "Creating legacy control...", c.control );
     	c.control = C.MakeNamedControl( parent.control, c_type, C.int(x), C.int(y), C.int(w), C.int(h), 0 );
	fmt.Println( "Created legacy control...", c.control );
}

func (parent *Control)Add( c *Control, x int, y int, w uint, h uint ) {
	if c.methods == nil {
        	fmt.Println( "control has not been defined" );
        	return;
        }
	fmt.Println( "c.control is", c.control );
	if c.control == nil {
		create_mutex.Lock();
		creating = c;
		fmt.Println( "Creating custom control...", c.control );
		c.control = C.MakeNamedControl( parent.control, control_name, C.int(x), C.int(y), C.int(w), C.int(h), 0 );
		fmt.Println( "Created custom control...", c.control );
		create_mutex.Unlock();
		fmt.Println( "result of add is ", c.control );
	} else {
		fmt.Println( "wasn't nil?", c.control, " or ", unsafe.Pointer(uintptr(0)) );
	}
}

//export ControlInitMethod
func ControlInitMethod( pc C.PSI_CONTROL ) int {
	C.SetCustomData_cgo( pc, unsafe.Pointer( creating ) );
	return creating.methods.OnInit();
}

//export ControlDrawMethod
func ControlDrawMethod( p unsafe.Pointer ) int {
	var c *Control = (*Control)(p )
	fmt.Println( "customdrawmethod", c.methods );
	//C.SetCustomData_cgo( pc, unsafe.Pointer( creating ) );
	return c.methods.OnDraw();
}

func DefineControl( inst *Control, methods ControlMethods ) {
	inst.methods = methods
}


func (ctype *ControlType) Make( parent *Control, x int, y int, w int, h int ) *Control {
	//c := Control{}
	return nil
}

func (ctype *ControlType) MakeWithCaption( parent *Control, x int, y int, w int, h int, caption string ) *Control {
	return nil
}

//export ControlMouseMethod
func ControlMouseMethod( p unsafe.Pointer, x int32, y int32, b uint32 ) int {
	var c *Control = (*Control)(p )
	return c.methods.OnMouse( x, y, b );
}

//export ControlSetMouseMethod
func ControlSetMouseMethod( pc C.PSI_CONTROL, x C.S_32, y C.S_32, b C._32 ) int {
	// no information to get back original structure
	var p unsafe.Pointer = C.GetUserData_cgo( pc );
	return (*Control)(p).methods.OnMouse( int32(x), int32(y), uint32(b) );
}

func (c *Control)Display( ) {
	C.DisplayFrame( c.control )
}
func (c *Control)Wait() {
	C.CommonWait( c.control )
}

func (c *Control)GetSurface() image.Image {
        if( c.surface == nil ){
		c.surface = image.MakeImageFromPointer( unsafe.Pointer( C.GetControlSurface( c.control ) ) );
        }
	return c.surface;
}


