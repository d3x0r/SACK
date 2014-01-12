
enum SHAPE_TYPE{ // SET_VECTOR_MODE
   // not creating anything at the moment...
   WB_SVM_NOTHING
	// freehand... no constraints, unlimited chain
	// of vectors
	  , WB_SVM_FREE
		// a box, begin vector marks one corner
      // end vector marks final corner, add vector is ignored
	  , WB_SVM_BOX
	  // a circle, begin vector marks origin
     // end vector marks radius/tilt (it is retained)
	  , WB_SVM_CIRCLE
	  // a elipse, begin vector marks one corner of a box within which
     // the elipse is inscribed... end is other corner
	  , WB_SVM_ELIPSE
	  // first vector marks origin
	  // first add vector determines direction of first side
     // end vector defines directino of remaining sides.
	  , WB_SVM_PARALLELOGRAM
     //
     , WB_SVM_REGION
};



enum {//whiteboard_messages;
	WB_CONNECT = MSG_ServiceUnload
	  , WB_DISCONNECT = MSG_ServiceLoad
	  , WB_OPEN_BOARD = MSG_EventUser
	  // login takes a text parameter with a user ID
	  // preferably a name.  Result is a unique client ID
     // which maintains their own current layers and other information
	  , WB_LOGIN
	  , WB_CREATE_LAYER
     , WB_SET_LAYER
	  , WB_SET_COLOR
	  , WB_ADD_POINT
	  // this actually resembles Mouse Event
	  // Begin at x, y (left down)
	  // go to x2,y2 (left remains down)
     // end at x3,y3 (left ends being down, segment ends.)
	  , WB_FIRST_VECTOR
	  , WB_BEGIN_DRAW = WB_FIRST_VECTOR
	  , WB_BEGIN_SEGMENT = WB_FIRST_VECTOR

     , WB_ADD_VECTOR
	  , WB_ADD_DRAW = WB_ADD_VECTOR
	  , WB_ADD_SEGMENT = WB_ADD_VECTOR

     , WB_END_VECTOR
	  , WB_END_DRAW = WB_END_VECTOR
	  , WB_END_SEGMENT = WB_END_VECTOR

     , WB_SET_VECTOR_MODE
}; //whiteboard_messaes;

typedef struct board_point_tag
{
	S_32 x, y;
   CDATA c;
} BOARD_POINT, *PBOARD_POINT;
#define MAXBOARD_POINTSPERSET 256
DeclareSet( BOARD_POINT );

typedef struct board_segment_tag
{
// at least 2 points... but may be more...
// freehand vectors are represented with single
// segment strokes...
	PLIST points;
	CDATA color;
} BOARD_SEGMENT, *PBOARD_SEGMENT;
#define MAXBOARD_SEGMENTSPERSET 128
DeclareSet( BOARD_SEGMENT );

typedef struct board_shape_tag
{
	enum SHAPE_TYPE type;
	union {
		struct {
			BOARD_POINT points[2];
			CDATA color_fill;
         CDATA color_border;
		} box;
	} data;
} WHITEBOARD_SHAPE, *PWHITEBOARD_SHAPE;

typedef struct layer_tag
{
	struct board_tag *board; // which board this layer is on.
   struct board_client_tag *client;
	PBOARD_POINTSET points;
	PBOARD_SEGMENTSET segments;
   CDATA current_color;
} BOARD_LAYER, *PBOARD_LAYER;
#define MAXBOARD_LAYERSPERSET 32
DeclareSet( BOARD_LAYER );


typedef struct board_client_tag
{
	struct {
		_32 bCreating : 1;
	} flags;
   struct board_tag *board; // which board this layer is on.
   enum SHAPE_TYPE creating;
	_32 client_id;
	PLIST layers;
   CTEXTSTR name;
	PBOARD_LAYER current_layer;
} BOARD_CLIENT, *PBOARD_CLIENT;
#define MAXBOARD_CLIENTSPERSET 32
DeclareSet( BOARD_CLIENT );


typedef struct board_tag
{
	struct {
      // ready to be used...
		_32 bReady : 1;
		_32 bWaiting : 1;
	} flags;
   TEXTSTR name;
	PLIST clients;
   PLIST layers;
#ifdef WHITEBOARD_CLIENT
   CDATA current_color;
	S_32 org_x, org_y; // origin x, y
// fixed point number.. scale 1 is 256x bigger
//, 0x100 is 1x, 0x200 is 1/2x
	S_32 scale;
#endif
} WHITEBOARD, *PWHITEBOARD;
#define MAXWHITEBOARDSPERSET 8
DeclareSet( WHITEBOARD );

