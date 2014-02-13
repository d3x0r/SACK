/*
 * SACK extension to define methods to render to javascript/HTML5 Canvas with WebSocket event interface
 *
 * Crafted by: Jim Buckeyne
 *
 * Purpose: Provide a well defined, concise structure to
 *   provide controls with a way to output a HTML5 Canvas rendering javascipt phrase.
 *   
 *
 *
 * (c)Freedom Collective, Jim Buckeyne 2012+; SACK Collection.
 *
 */

#ifndef PSI_HTML5_CANVAS_STUFF_DEFINED
#define PSI_HTML5_CANVAS_STUFF_DEFINED

#include <stdhdrs.h>

#include <procreg.h>
#include <controls.h>


#ifdef __cplusplus
#define _PSI_HTML5_CANVAS_NAMESPACE namespace Html5Canvas {
#define PSI_HTML5_CANVAS_NAMESPACE SACK_NAMESPACE _PSI_NAMESPACE _PSI_HTML5_CANVAS_NAMESPACE
#define PSI_HTML5_CANVAS_NAMESPACE_END } _PSI_NAMESPACE_END SACK_NAMESPACE_END
#define USE_PSI_HTML5_CANVAS_NAMESPACE using namespace sack::PSI::Html5Canvas;
#else
#define _PSI_HTML5_CANVAS_NAMESPACE 
#define PSI_HTML5_CANVAS_NAMESPACE 
#define PSI_HTML5_CANVAS_NAMESPACE_END 
#define USE_PSI_HTML5_CANVAS_NAMESPACE
#endif

PSI_HTML5_CANVAS_NAMESPACE


#ifdef PSI_HTML5_CANVAS_SOURCE
#define PSI_HTML5_CANVAS_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define PSI_HTML5_CANVAS_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

typedef struct html5_canvas *HTML5Canvas;

// need some sort of other methods to work with an HTML5Canvas...
PSI_HTML5_CANVAS_PROC( HTML5Canvas, CreateCanvas )( PSI_CONTROL root_control );
PSI_HTML5_CANVAS_PROC( LOGICAL, WriteCanvas )( HTML5Canvas, CTEXTSTR filename );
//PSI_HTML5_CANVAS_PROC( void, AddScriptToCanvas )( HTML5Canvas, CTEXTSTR format, ... );
PSI_HTML5_CANVAS_PROC( void, h5printf )( HTML5Canvas, CTEXTSTR format, ... );
PSI_HTML5_CANVAS_PROC( void, AddResourceToCanvas )( HTML5Canvas, CTEXTSTR resource_name );



/* define a callback which uses a HTML5Canvas collector to build javascipt to render the control.
 * example:
 *       static int OnDrawToHTML("Control Name")(PSI_CONTROL, HTML5Canvas ){ }
 */
#define OnDrawToHTML(name)  \
	__DefineRegistryMethodP(PSI_PRELOAD_PRIORITY,PSI_ROOT_REGISTRY,_OnDrawCommon,WIDE("control"),name WIDE("/rtti"),WIDE("draw_to_canvas"),int,(PSI_CONTROL, HTML5Canvas ), __LINE__)

/* define a callback which uses a HTML5Canvas collector to build javascipt to render the control.
 * example:
 *       static int OnMouseFromHTML("Control Name")(PSI_CONTROL, HTML5Canvas ){ }
 */
#define OnMouseFromHTML(name)  \
	__DefineRegistryMethodP(PSI_PRELOAD_PRIORITY,PSI_ROOT_REGISTRY,_OnDrawCommon,WIDE("control"),name WIDE("/rtti"),WIDE("draw"),int,(PSI_CONTROL, HTML5Canvas, int x, int y ), __LINE__)




PSI_HTML5_CANVAS_NAMESPACE_END
USE_PSI_HTML5_CANVAS_NAMESPACE
#endif
