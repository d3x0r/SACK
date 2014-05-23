

// these need to be pulled out soon...
extern "C" BOOL CALLBACK SynapsePropProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
extern "C" BOOL CALLBACK NeuronPropProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
extern "C" BOOL CALLBACK SigmoidProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static CDATA colors[18] = 
{  Color( 132, 132, 132),
   Color( 0, 240, 32      ),
   Color( 188, 0, 16      ),
              
   Color( 92, 64, 64      ),
   Color( 40, 209, 240    ),
   Color( 255, 146, 5     ),
              
   Color( 132, 132, 132   ),
   Color( 32, 240, 0      ),
   Color( 188, 0, 16      ),
              
   Color( 0, 0, 0         ),
   Color( 144, 0, 0       ),
   Color( 0, 0, 144       ),
              
   Color(  255, 255, 0    ),
   Color( 255, 0, 0     ),
   Color(   255, 0, 255   ),
              
   Color( 0, 255, 255  ),
   Color( 0, 255, 0    ),
   Color( 255, 255, 0  ) };

#define GAIN_MID     (colors+0)
#define GAIN_HIGH    (colors+1)
#define GAIN_LOW     (colors+2)
                                
#define LEVEL_MID    (colors+3) 
#define LEVEL_LOW    (colors+4) 
#define LEVEL_HIGH   (colors+5) 
                                           
#define INPUT_MID    (colors+6)  
#define INPUT_HIGH   (colors+7)  
#define INPUT_LOW    (colors+8)  
                                           
#define THRESH_MID   (colors+9)  
#define THRESH_LOW   (colors+10) 
#define THRESH_HIGH  (colors+11) 
                                
#define INPUT_LEVEL_LOW   (colors+12)     
#define INPUT_LEVEL_MID   (colors+13)    
#define INPUT_LEVEL_HIGH  (colors+14)    

#define OUTPUT_LEVEL_LOW  (colors+15)      
#define OUTPUT_LEVEL_MID  (colors+16)     
#define OUTPUT_LEVEL_HIGH (colors+17)     


enum {
   BLANK,
	LHD, LHU,
	HORIZONTAL,
	RHU, RHD,
	DIAG1,  DF1, DF2, // diagonal fill corners...
	DVL, DVR,
	VERTICAL,
	UVL, UVR,
	DIAG2,  DF3, DF4, // diagonal fill corners

	_GATE  // no opposite state....
	  , GATE0, GATE1, GATE2
	  , GATE3       , GATE4
	  , GATE5, GATE6, GATE7,

	  _SYN
	  , SYN0, SYN1, SYN2
	  , SYN3      , SYN4
	  , SYN5, SYN6, SYN7,

   _NEURON, // = SYN7 + 1,
   NEURON1,  NEURON2,  NEURON3,
   NEURON4,  NEURON5,  NEURON6,
   NEURON7,  NEURON8,  NEURON9,

   _NEURON_ON, // = SYN7 + 1,
   NEURON_ON1,  NEURON_ON2,  NEURON_ON3,
   NEURON_ON4,  NEURON_ON5,  NEURON_ON6,
   NEURON_ON7,  NEURON_ON8,  NEURON_ON9,

   _NEURON_OFF, // = _NEURON_ON | 0x8000,
   NEURON_OFF1,  NEURON_OFF2,  NEURON_OFF3,
   NEURON_OFF4,  NEURON_OFF5,  NEURON_OFF6,
   NEURON_OFF7,  NEURON_OFF8,  NEURON_OFF9,

   _INPUT, // = NEURON_ON9 + 1,
   INPUT1,INPUT2,INPUT3,
   INPUT4,INPUT5,INPUT6,
   INPUT7,INPUT8,INPUT9,

   _OUTPUT, // = INPUT_ON9 + 1,
   OUTPUT1,OUTPUT2,OUTPUT3,
   OUTPUT4,OUTPUT5,OUTPUT6,
   OUTPUT7,OUTPUT8,OUTPUT9,

   NOTHING, MASTER,
};

#define NOWHERE -1
#define UP 0
#define UP_RIGHT 1
#define RIGHT 2
#define DOWN_RIGHT 3
#define DOWN 4
#define DOWN_LEFT 5
#define LEFT 6
#define UP_LEFT 7

typedef struct delta_tag
{
	int x, y;
} DIR_DELTA;

static DIR_DELTA DirDeltaMap[8] = { { 0, -1 }, 
                             { 1, -1 }, 
								     { 1, 0 }, 
							        { 1, 1 }, 
								     { 0, 1 } , 
							        { -1, 1 },  
								     { -1, 0 },
								     { -1, -1 } 
									};

typedef struct possible_tag
{
	int nDirection;
	int nUsed;
	int nNext[3];  // array of next possible peices - but is not used...
                  // could be used for random building 
                  // rather than picking directions which may unwind 
                  // this guarantees that it is forward moving...
} POSSIBLE;


typedef struct PeiceDef_tag
{
	PSTR pIconName;  
	int nPeice;      // ID of the peice...
	int nValid;
	POSSIBLE valid[2];

   int DrawMethod; // copy to layer tag...
     // base colors - actually has to be [3]...
   PCDATA cPrimary[3];   // [0] = zero value [1] = maxvalue
   PCDATA cSecondary[3]; 
   PCDATA cTertiary[3]; 
   char *ResourceName;
   DLGPROC pDlgProc;

   // this part needs to be initialized from an external directory...
   Image pImage[3];  // reg, med, small
   int rows,cols; // logical coords for position check
   // need to also include

   //    "Name PropSheet"
   //    "method propsheet"
   //    on click method
   //    on unclick method
   //       width, height (grid units)
   // 
 //  int cx, cy;     // center x, center y for hotspot placement...
} PEICE, *PPEICE;

#define NOCOLOR { {0,0,0},{0,0,0},{0,0,0}},{ {0,0,0},{0,0,0},{0,0,0}},{ {0,0,0},{0,0,0},{0,0,0}}

#define NUM_PEICES (sizeof(peices)/sizeof(PEICE))

#define LEFT_SET        {LEFT, 3, {RHU, HORIZONTAL, RHD}}
#define RIGHT_SET       {RIGHT, 3, { LHU, HORIZONTAL, LHD }}
#define UP_SET          {UP, 3, { UVL, VERTICAL, UVR } }
#define DOWN_SET        { DOWN, 3, { DVR, VERTICAL,  DVL } }
#define DOWN_RIGHT_SET  {DOWN_RIGHT, 3, { RHU, DIAG1, UVL }}
#define DOWN_LEFT_SET   {DOWN_LEFT, 3, { UVR, DIAG2, LHU }}
#define UP_RIGHT_SET    { UP_RIGHT, 3, { DVL, DIAG2, RHD } } 
#define UP_LEFT_SET     {UP_LEFT, 3, { LHD, DIAG1, DVR } } 
PEICE static_peices[] = {
   {"Invalid", -1, 0 },


	{"Master", MASTER }, // gets broken into sub-parts...
   // begin row one...
	{"GATE4", GATE7, 1, { UP_LEFT_SET}
         , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {"DF4", DF4, 0, {{0}}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },

	{"RHD", RHD,			2, { RIGHT_SET, DOWN_LEFT_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },

	{"HORIZONTAL",HORIZONTAL,2, { LEFT_SET, RIGHT_SET } 
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },

	{"LHD", LHD,			2, { LEFT_SET, DOWN_RIGHT_SET  }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },

   {"DF1", DF1, 0, {{0}}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },

	{"GATE5", GATE1, 1, { UP_RIGHT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   // begin row two...
   {NULL, NOTHING }, // ignore filling in this sub-peice...
	{"DIAG2", DIAG2,		 2,{ UP_RIGHT_SET, DOWN_LEFT_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {"DF2", DF2, 0, {{0}}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
	{"GATE2", GATE4, 1, { DOWN_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {"DF3", DF3, 0, {{0}}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
	{"DIAG1", DIAG1,		 2,{ UP_LEFT_SET,DOWN_RIGHT_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {NULL, NOTHING }, // ignore filling in this sub-peice...
   // begin row 3
	{"UVR", UVR,			 2,{ DOWN_SET, UP_RIGHT_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {NULL, NOTHING }, // ignore filling in this sub-peice...
	{"SYN4", SYN7, 1, { UP_LEFT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} }, "SynapseProp",SynapsePropProc },
	{"SYN0", SYN0, 1, { UP_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} }, "SynapseProp",SynapsePropProc },
	{"SYN5", SYN1, 1, { UP_RIGHT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} }, "SynapseProp",SynapsePropProc },
   {NULL, NOTHING }, // ignore filling in this sub-peice...
	{"UVL", UVL,			 2,{ DOWN_SET, UP_LEFT_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   // begin row 4
	{"VERTICAL", VERTICAL,	 2,{ UP_SET, DOWN_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
	{"GATE1", GATE2, 1, { RIGHT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
	{"SYN3", SYN6, 1, { LEFT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} }, "SynapseProp",SynapsePropProc },
   {NULL, NOTHING }, // ignore filling in this sub-peice...
	{"SYN1", SYN2, 1, { RIGHT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} }, "SynapseProp",SynapsePropProc },
	{"GATE3", GATE6, 1, { LEFT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {NULL, NOTHING }, // ignore filling in this sub-peice...

	{"DVR", DVR,			 2,{ UP_SET, DOWN_RIGHT_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {NULL, NOTHING }, // ignore filling in this sub-peice...
	{"SYN7", SYN5, 1, { DOWN_LEFT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} }, "SynapseProp",SynapsePropProc },
	{"SYN2", SYN4, 1, { DOWN_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} }, "SynapseProp",SynapsePropProc },
	{"SYN6", SYN3, 1, { DOWN_RIGHT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} }, "SynapseProp",SynapsePropProc },
   {NULL, NOTHING }, // ignore filling in this sub-peice...
	{"DVL", DVL,			 2,{ UP_SET, DOWN_LEFT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   // begin last row...

   {NULL, NOTHING }, // ignore filling in this sub-peice...
   {NULL, NOTHING }, // ignore filling in this sub-peice...
   {NULL, NOTHING }, // ignore filling in this sub-peice...
 	{"GATE0", GATE0, 1, { UP_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {NULL, NOTHING }, // ignore filling in this sub-peice...
   {NULL, NOTHING }, // ignore filling in this sub-peice...
   {NULL, NOTHING }, // ignore filling in this sub-peice...

	{"GATE7", GATE5, 1, { DOWN_LEFT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {NULL, NOTHING }, // ignore filling in this sub-peice...
	{ "RHU", RHU,			 2,{ RIGHT_SET, UP_LEFT_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {NULL, NOTHING }, // ignore filling in this sub-peice...
	{"LHU", LHU,			 2,{ LEFT_SET,UP_RIGHT_SET }
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },
   {NULL, NOTHING }, // ignore filling in this sub-peice...
   {"GATE6", GATE3, 1, { DOWN_RIGHT_SET}
            , DRAW_MULTI_SHADED, { {LEVEL_LOW}, {LEVEL_MID}, {LEVEL_HIGH} },
                            { {GAIN_LOW}, {GAIN_MID}, {GAIN_HIGH} },
                            { {0} } },

#pragma warning( disable: 4309; disable:4305 )

   {"NEURON", _NEURON, 0, {0}
   , DRAW_MULTI_SHADED, { {INPUT_LOW}, {INPUT_MID}, {INPUT_HIGH} },
                        { {THRESH_LOW}, {THRESH_MID}, {THRESH_HIGH} },
                        { {0} },
                        "NeuronProp",
                        NeuronPropProc }
   ,{"INPUT", _INPUT, 0, {0}
   , DRAW_SHADED, { {INPUT_LEVEL_LOW}, {INPUT_LEVEL_MID}, {INPUT_LEVEL_HIGH} } }
   
   ,{"OUTPUT", _OUTPUT, 0, {0}
   , DRAW_SHADED, { {OUTPUT_LEVEL_LOW}, {OUTPUT_LEVEL_MID}, {OUTPUT_LEVEL_HIGH} } }

   ,{"BLANK", BLANK } };

