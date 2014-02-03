#define RCOORD_IS_DOUBLE 1

#define DOC_O_DOC 1

#define CPROC __cdecl
#define IMAGE_API __cdecl
#define TYPELIB_CALLTYPE __cdecl

#define SACK_NAMESPACE namespace sack {
#define SACK_NAMESPACE_END }
#define _CONTAINER_NAMESPACE namespace containers {
#define _CONTAINER_NAMESPACE_END }
#define _LINKLIST_NAMESPACE namespace list {
#define _LINKLIST_NAMESPACE_END }
#define _DATALIST_NAMESPACE namespace data_list {
#define _DATALIST_NAMESPACE_END }



#define INTERSHELL_PROC(type,name) IMPORT_METHOD type CPROC name
#define INTERSHELL_PROC_PTR(type,name) type CPROC (*name)
#define _DEFINE_INTERFACE

#define SACK_DEADSTART_NAMESPACE   namespace sack { namespace app { namespace deadstart {
#define SACK_DEADSTART_NAMESPACE_END    } } }

#define SACK_CONTAINER_NAMESPACE namespace sack { namespace containers {
#define SACK_CONTAINER_NAMESPACE_END _CONTAINER_NAMESPACE_END SACK_NAMESPACE_END
#define SACK_CONTAINER_LINKLIST_NAMESPACE SACK_CONTAINER_NAMESPACE _LISTLIST_NAMESPACE
#define SACK_CONTAINER_LINKLIST_NAMESPACE_END _LISTLIST_NAMESPACE_END SACK_CONTAINER_NAMESPACE
#define IMAGE_NAMESPACE namespace sack { namespace image {
#define IMAGE_NAMESPACE_END }}
#define _IMAGE_NAMESPACE namespace image {
#define ASM_IMAGE_NAMESPACE extern "C" {
#define ASM_IMAGE_NAMESPACE_END }
#define _TASK_NAMESPACE namespace task {
#define _CONSTRUCT_NAMESPACE namespace construct {
#define _TASK_NAMESPACE_END }
#define _CONSTRUCT_NAMESPACE_END }
#define CONSTRUCT_NAMESPACE SACK_NAMESPACE _TASK_NAMESPACE _CONSTRUCT_NAMESPACE
#define CONSTRUCT_NAMESPACE_END _CONSTRUCT_NAMESPACE_END _TASK_NAMESPACE_END SACK_NAMESPACE_END
#define TIMER_NAMESPACE namespace sack { namespace timers {
#define _TIMER_NAMESPACE namespace timers {
#define TIMER_NAMESPACE_END } }
#define LOGGING_NAMESPACE namespace sack { namespace logging {
#define LOGGING_NAMESPACE_END }; };

#define _FILESYS_NAMESPACE  namespace filesys {
#define _FILESYS_NAMESPACE_END }
#define FILESYS_NAMESPACE  namespace sack { namespace filesys {
#define FILESYS_NAMESPACE_END } }
#define _FILEMON_NAMESPACE  namespace monitor {
#define _FILEMON_NAMESPACE_END }

#define IMPROT_METHOD __declspec(dllimport)
#define PASTE(a,b)  b
#define RENDER_PROC(type,name)  type name
#define RENDER_NAMESPACE namespace sack { namespace image { namespace render {
#define RENDER_NAMESPACE_END }}}
#define _RENDER_NAMESPACE namespace render {
#define _RENDER_KEYBOARD_NAMESPACE namespace keyboard {
#define _RENDER_KEYBOARD_NAMESPACE_END }

#define _SQL_NAMESPACE   namespace sql {
#define SQL_NAMESPACE   namespace sack { namespace sql {
#define SQL_NAMESPACE_END } }
#define SQLSTUB_PROC(type,name) IMPORT_METHOD type CPROC name
#define _OPTION_NAMESPACE namespace options {
#define _OPTION_NAMESPACE_END };
#define SQLGETOPTION_PROC(type,name) IMPORT_METHOD type CPROC name
#define METHOD_PTR(type,name) type name



#define _NETWORK_NAMESPACE  namespace network {
#define _NETWORK_NAMESPACE_END }
#define _TCP_NAMESPACE  namespace tcp {
#define _TCP_NAMESPACE_END }
#define USE_TCP_NAMESPACE using namespace tcp;
#define _UDP_NAMESPACE  namespace udp {
#define _UDP_NAMESPACE_END }
#define USE_UDP_NAMESPACE using namespace tcp;
#define SACK_NETWORK_NAMESPACE  namespace sack { namespace network
#define SACK_NETWORK_NAMESPACE_END } }
#define SACK_NETWORK_TCP_NAMESPACE  namespace sack { namespace network { namespace tcp
#define SACK_NETWORK_TCP_NAMESPACE_END }}}
#define SACK_NETWORK_UDP_NAMESPACE  namespace sack { namespace network { namespace udp
#define SACK_NETWORK_UDP_NAMESPACE_END }}}
#define NETWORK_PROC(type,name) IMPORT_METHOD type name


#define PSI_PROC(type,name) type  name

#define IMAGE_PROC_PTR(type,name)  type (*name)
#define RENDER_PROC_PTR(type,name)  type (*name)
#define PSI_PROC_PTR(type,name)  type (*name)

#define TIMER_PROC(type,name) IMPORT_METHOD type name


#define _PSI_NAMESPACE namespace PSI {

#define PSI_NAMESPACE namespace sack { namespace PSI {
#define PSI_NAMESPACE_END } }
#define USE_PSI_NAMESPACE using namespace sack::PSI;

#   define _INTERFACE_NAMESPACE namespace interface {
#   define _INTERFACE_NAMESPACE_END }
#   define _PSI_INTERFACE_NAMESPACE namespace interface {
#   define _PSI_INTERFACE_NAMESPACE_END }
#   define _BUTTON_NAMESPACE namespace button {
#   define _BUTTON_NAMESPACE_END } 
#   define USE_BUTTON_NAMESPACE using namespace button; 
#   define USE_PSI_BUTTON_NAMESPACE using namespace sack::PSI::button; 

#   define _COLORWELL_NAMESPACE namespace colorwell {
#   define _COLORWELL_NAMESPACE_END } 
#   define USE_COLORWELL_NAMESPACE using namespace colorwell; 
#   define USE_PSI_COLORWELL_NAMESPACE using namespace sack::PSI::colorwell; 

#   define _MENU_NAMESPACE namespace popup {
#   define _MENU_NAMESPACE_END } 
#   define USE_MENU_NAMESPACE using namespace popup; 
#   define USE_PSI_MENU_NAMESPACE using namespace sack::PSI::popup; 

#   define _TEXT_NAMESPACE namespace text {
#   define _TEXT_NAMESPACE_END } 
#   define TEXT_NAMESPACE namespace sack { namespace containers { namespace text {
#   define TEXT_NAMESPACE_END }  } }
#   define USE_TEXT_NAMESPACE using namespace text; 
#   define USE_PSI_TEXT_NAMESPACE using namespace sack::PSI::text; 

#   define _EDIT_NAMESPACE namespace edit {
#   define _EDIT_NAMESPACE_END } 
#   define USE_EDIT_NAMESPACE using namespace edit; 
#   define USE_PSI_EDIT_NAMESPACE using namespace sack::PSI::edit; 

#   define _SLIDER_NAMESPACE namespace slider {
#   define _SLIDER_NAMESPACE_END } 
#   define USE_SLIDER_NAMESPACE using namespace slider; 
#   define USE_PSI_SLIDER_NAMESPACE using namespace sack::PSI::slider; 

#   define _FONTS_NAMESPACE namespace font {
#   define _FONTS_NAMESPACE_END } 
#   define USE_FONTS_NAMESPACE using namespace font; 
#   define USE_PSI_FONTS_NAMESPACE using namespace sack::PSI::font; 

#   define _LISTBOX_NAMESPACE namespace listbox {
#   define _LISTBOX_NAMESPACE_END } 
#   define USE_LISTBOX_NAMESPACE using namespace listbox; 
#   define USE_PSI_LISTBOX_NAMESPACE using namespace sack::PSI::listbox; 

#   define _SCROLLBAR_NAMESPACE namespace scrollbar {
#   define _SCROLLBAR_NAMESPACE_END } 
#   define USE_SCROLLBAR_NAMESPACE using namespace scrollbar; 
#   define USE_PSI_SCROLLBAR_NAMESPACE using namespace sack::PSI::scrollbar; 

#   define _SHEETS_NAMESPACE namespace sheet_control {
#   define _SHEETS_NAMESPACE_END } 
#   define USE_SHEETS_NAMESPACE using namespace sheet_control; 
#   define USE_PSI_SHEETS_NAMESPACE using namespace sack::PSI::sheet_control; 

#   define _MOUSE_NAMESPACE namespace _mouse {
#   define _MOUSE_NAMESPACE_END } 
#   define USE_MOUSE_NAMESPACE using namespace _mouse; 
#   define USE_PSI_MOUSE_NAMESPACE using namespace sack::PSI::_mouse; 

#   define _XML_NAMESPACE namespace xml {
#   define _XML_NAMESPACE_END } 
#   define USE_XML_NAMESPACE using namespace xml; 
#   define USE_PSI_XML_NAMESPACE using namespace sack::PSI::xml; 

#   define _PROP_NAMESPACE namespace properties {
#   define _PROP_NAMESPACE_END } 
#   define USE_PROP_NAMESPACE using namespace properties; 
#   define USE_PSI_PROP_NAMESPACE using namespace sack::PSI::properties; 

#   define _CLOCK_NAMESPACE namespace clock {
#   define _CLOCK_NAMESPACE_END } 
#   define USE_CLOCK_NAMESPACE using namespace clock; 
#   define USE_PSI_CLOCK_NAMESPACE using namespace sack::PSI::clock; 

#define PSI_COLORWELL_NAMESPACE namespace sack { namespace PSI {namespace colorwell {
#define PSI_COLORWELL_NAMESPACE_END } } }

#define PSI_BUTTON_NAMESPACE namespace sack { namespace PSI { namespace button {
#define PSI_BUTTON_NAMESPACE_END } } }


#define PSI_MENU_NAMESPACE namespace sack { namespace PSI {  namespace menu {
#define PSI_MENU_NAMESPACE_END } } }

#define PSI_TEXT_NAMESPACE namespace sack { namespace PSI {  namespace text {
#define PSI_TEXT_NAMESPACE_END } } }

#define PSI_EDIT_NAMESPACE namespace sack { namespace PSI {  namespace edit {
#define PSI_EDIT_NAMESPACE_END } } }

#define PSI_SLIDER_NAMESPACE namespace sack { namespace PSI {  namespace slider {
#define PSI_SLIDER_NAMESPACE_END } } }

#define PSI_FONTS_NAMESPACE namespace sack { namespace PSI { namespace font {
#define PSI_FONTS_NAMESPACE_END } } }

#define PSI_LISTBOX_NAMESPACE namespace sack { namespace PSI {  namespace listbox {
#define PSI_LISTBOX_NAMESPACE_END } } }

#define PSI_SCROLLBAR_NAMESPACE namespace sack { namespace PSI {  namespace scrollbar{
#define PSI_SCROLLBAR_NAMESPACE_END } } }

#define PSI_SHEETS_NAMESPACE namespace sack { namespace PSI {  namespace sheet_control{
#define PSI_SHEETS_NAMESPACE_END } } }

#define PSI_MOUSE_NAMESPACE namespace sack { namespace PSI {  namespace _mouse{
#define PSI_MOUSE_NAMESPACE_END } } }

#define PSI_XML_NAMESPACE namespace sack { namespace PSI {  namespace xml{
#define PSI_XML_NAMESPACE_END } } }

#define PSI_PROP_NAMESPACE namespace sack { namespace PSI {  namespace properties {
#define PSI_PROP_NAMESPACE_END } } }

#define PSI_CLOCK_NAMESPACE namespace sack { namespace PSI {  namespace clock {
#define PSI_CLOCK_NAMESPACE_END } } }

#define PROCREG_PROC(type,name) IMPORT_METHOD type CPROC name
#define _SYSTEM_NAMESPACE namespace system {
#define SYSTEM_PROC(type,name) IMPORT_METHOD type CPROC name

#define PROCREG_NAMESPACE namespace sack { namespace app { namespace registry {
#define _PROCREG_NAMESPACE namespace registry {
#define _APP_NAMESPACE namespace app { 
#define PROCREG_NAMESPACE_END }}}

#define _MATH_VECTOR_NAMESPACE namespace math { namespace vector {
#define _MATH_NAMESPACE namespace math {
#define _VECTOR_NAMESPACE namespace vector {
#define _FRACTION_NAMESPACE namespace fraction {
#define _FRACTION_NAMESPACE_END }
#define SACK_MATH_FRACTION_NAMESPACE_END } } }


#define _MSG_NAMESPACE namespace msg {
#define _PROTOCOL_NAMESPACE namespace protocol {
#define _CLIENT_NAMESPACE namespace client {
#define _SERVER_NAMESPACE namespace server {
#define MSGCLIENT_NAMESPACE namespace sack { namespace msg { namespace client {
#define MSGCLIENT_NAMESPACE_END }} }

#define MSGSERVER_NAMESPACE namespace sack { namespace msg { namespace server {
#define MSGSERVER_NAMESPACE_END }} }

#define MSGPROTOCOL_NAMESPACE namespace sack { namespace msg { namespace protocol {
#define MSGPROTOCOL_NAMESPACE_END }} }

#define SERVERMSG_PROC(type,name) type  name
#define CLIENTMSG_PROC(type,name) type  name
#define SERVERMSG_PROC(type,name) type  name

#define CONTROL_PROC( name,_args )  \
	PSI_PROC( PSI_CONTROL, Get##name##PropertyPage )( PSI_CONTROL pc ); \
	PSI_PROC( void, Apply##name##PropertyPage )( PSI_CONTROL pc, PSI_CONTROL page ); \
   PSI_PROC( int, Config##name)( PSI_CONTROL );  \
	/*PSI_PROC( PSI_CONTROL, Make##name )( PSI_CONTROL pFrame, int attr */ \
	/*, int x, int y, int w, int h*/ \
	/*, PTRSZVAL nID, ... ); */
#define CAPTIONED_CONTROL_PROC( name,_args )  \
	PSI_PROC( PSI_CONTROL, Get##name##PropertyPage )( PSI_CONTROL pc ); \
	PSI_PROC( void, Apply##name##PropertyPage )( PSI_CONTROL pc, PSI_CONTROL page ); \
   PSI_PROC( int, Config##name)( PSI_CONTROL );  \


#define DeclMethod( name ) int n##name; _##name *name
#define DeclSingleMethod( name ) _##name name

#define KEYPAD_NAMESPACE namespace sack { namespace widgets { namespace buttons {
#define KEYPAD_NAMESPACE_END } } SACK_NAMESPACE_END
#define BANNER_NAMESPACE namespace sack { namespace widgets { namespace banner {
#define BANNER_NAMESPACE_END } } SACK_NAMESPACE_END

#define SACK_OPTION_NAMESPACE namespace sack { namespace sql { namespace options {
#define PSSQL_PROC(type,name)   type name

#define _HTTP_NAMESPACE namespace http {
#define _HTTP_NAMESPACE_END }
#define HTTP_EXPORT
#define HTTPAPI CPROC
#define _SETS_NAMESPACE namespace sets {
#define _SETS_NAMESPACE_END }
