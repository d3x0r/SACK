#include <stdhdrs.h>
#include <filesys.h>
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <image.h>
#include <render.h>
#include <vectlib.h>

/* 
rail_pictures_internal = function(elems)
  local keys = {{"straight_rail", "horizontal", 64, 128, 0, 0, true},
                {"straight_rail", "vertical", 128, 64, 0, 0, true},
                {"straight_rail", "diagonal-left-top", 96, 96, 0.5, 0.5, true},
                {"straight_rail", "diagonal-right-top", 96, 96, -0.5, 0.5, true},
                {"straight_rail", "diagonal-right-bottom", 96, 96, -0.5, -0.5, true},
                {"straight_rail", "diagonal-left-bottom", 96, 96, 0.5, -0.5, true},
                {"curved_rail", "vertical-left-top", 192, 288, 0.5, 0.5},
                {"curved_rail", "vertical-right-top", 192, 288, -0.5, 0.5},
                {"curved_rail", "vertical-right-bottom", 192, 288, -0.5, -0.5},
                {"curved_rail", "vertical-left-bottom", 192, 288, 0.5, -0.5},
                {"curved_rail" ,"horizontal-left-top", 288, 192, 0.5, 0.5},
                {"curved_rail" ,"horizontal-right-top", 288, 192, -0.5, 0.5},
                {"curved_rail" ,"horizontal-right-bottom", 288, 192, -0.5, -0.5},
                {"curved_rail" ,"horizontal-left-bottom", 288, 192, 0.5, -0.5}}
*/

struct image_info {
	const char *prefix;
	const char *variant;
	int w, h;
	RCOORD ox, oy;
	LOGICAL _3variations;
	Image low[12];
	Image high[12];
};

#define TOTAL_HALF_UNITS 80
#define CENTER  41

#define X_HALF_UNITS 16
#define Y_HALF_UNITS 16

//#define X_UNITS 32
//#define Y_UNITS 32


struct image_info image_srcs[] = {
	{"straight-rail", "horizontal", 64, 128, 0, 0, TRUE},
	{"straight-rail", "vertical", 128, 64, 0, 0, TRUE},
	{"straight-rail", "diagonal-left-top", 96, 96, 0.5, 0.5, TRUE},
	{"straight-rail", "diagonal-right-top", 96, 96, -0.5, 0.5, TRUE},
	{"straight-rail", "diagonal-right-bottom", 96, 96, -0.5, -0.5, TRUE},
	{"straight-rail", "diagonal-left-bottom", 96, 96, 0.5, -0.5, TRUE},
	{"curved-rail", "vertical-left-top", 192, 288, 0.5, 0.5},
	{"curved-rail", "vertical-right-top", 192, 288, -0.5, 0.5},
	{"curved-rail", "vertical-right-bottom", 192, 288, -0.5, -0.5},
	{"curved-rail", "vertical-left-bottom", 192, 288, 0.5, -0.5},
	{"curved-rail" ,"horizontal-left-top", 288, 192, 0.5, 0.5},
	{"curved-rail" ,"horizontal-right-top", 288, 192, -0.5, 0.5},
	{"curved-rail" ,"horizontal-right-bottom", 288, 192, -0.5, -0.5},
	{"curved-rail" ,"horizontal-left-bottom", 288, 192, 0.5, -0.5}
};

#define hrz  "horizontal"
#define vrt  "vertical"
#define crv_hlb  hrz "-left-bottom"
#define crv_hlt  hrz "-left-top"
#define crv_hrb  hrz "-right-bottom"
#define crv_hrt  hrz "-right-top"
#define crv_vlb  vrt "-left-bottom"
#define crv_vlt  vrt "-left-top"
#define crv_vrb  vrt "-right-bottom"
#define crv_vrt  vrt "-right-top"

#define dia  "diagonal"
#define dia_rb  dia  "-right-bottom"
#define dia_rt  dia  "-right-top"
#define dia_lb  dia  "-left-bottom"
#define dia_lt  dia  "-left-top"

#define remnant  "remnants"
#define bed  "stone-path"
#define bed_back  bed "-background"
#define ties  "ties"
#define metals  "metals"
#define backplates  "backplates"

#define num_parts  (sizeof(parts)/sizeof(parts[0]))
const char *parts[] = { bed, bed_back, ties, metals, backplates };

const char *curve_prefix = "curved-rail";
const char *hr_curve_prefix = "hr-curved-rail";
const char *straight_prefix = "straight-rail";
const char *hr_straight_prefix = "hr-straight-rail";

#define STR  1
#define DIA  2
#define CRV  256
#define VER  4
#define HOR  8
#define LFT  16
#define TOP  32
#define RGT  64
#define BOT  128

struct segment {
	int type;
	int type_index;
	int variant;
	struct {
		int x, y;
	} pos;
};

#define segment_count (sizeof(circle)/sizeof(circle[0]))

const struct segment circle[] = {
	{ STR | VER        , 1, 0,{ CENTER - CENTER + 1, CENTER - 6 } },
	{ STR|VER        , 1, 1, { CENTER - CENTER+1, CENTER - 2 } },
	{ STR|VER        , 1, 2, { CENTER - CENTER+1, CENTER + 2 } },
	{ STR|VER        , 1, 0, {1+ CENTER + CENTER-8-2, CENTER - 6 } },
	{ STR|VER        , 1, 1, {1+ CENTER + CENTER-8-2, CENTER - 2 } },
	{ STR|VER        , 1, 2, { 1 + CENTER + CENTER-8-2, CENTER + 2 } },

	{ STR|HOR        , 0, 0, { CENTER - 6 , CENTER - CENTER + 1  } },
	{ STR|HOR        , 0, 1, { CENTER - 2 , CENTER - CENTER + 1  } },
	{ STR|HOR        , 0, 2, { CENTER + 2 , CENTER - CENTER + 1  } },
	{ STR|HOR        , 0, 0, { CENTER - 6 , CENTER + CENTER-8-1 } },
	{ STR|HOR        , 0, 1, { CENTER - 2 , CENTER + CENTER-8-1 } },
	{ STR|HOR        , 0, 2, { CENTER + 2 , CENTER + CENTER-8-1 } },

	{ CRV|VER|RGT|TOP, 7, 0, { CENTER - CENTER, CENTER + (6) } },
	{ CRV|VER|RGT|BOT, 8, 0, { CENTER - CENTER, CENTER - (18 + 6) } },
	{ CRV|VER|LFT|TOP, 6, 0, { CENTER + CENTER - 12, CENTER + (6) } },
	{ CRV|VER|LFT|BOT, 9, 0, { CENTER + CENTER - 12, CENTER - (18 + 6) } },

	{ CRV|HOR|RGT|BOT, 12, 0, { CENTER - 16 - 8 , CENTER - CENTER + 0  } },
	{ CRV|HOR|RGT|TOP, 11, 0, { CENTER - 16 - 8, CENTER + CENTER - 12 } },
	{ CRV|HOR|LFT|BOT, 13, 0, { CENTER + 6, CENTER - CENTER + 0 } },
	{ CRV|HOR|LFT|TOP, 10, 0, { CENTER + 6, CENTER + CENTER - 12 } },

	
	{ STR|DIA|RGT|BOT, 4, 0, { CENTER-CENTER + 12 - 3, CENTER  - (6+18 + 4) } },
	{ STR|DIA|RGT|BOT, 4, 1, { CENTER-CENTER + 12+4 - 3, CENTER - (6+18 + 4)-4 } },
	//{ STR|DIA|RGT|BOT, 4, 2, { CENTER-CENTER + 12+8 - 1, CENTER - (6+18 + 4)-8 } },
	{ STR|DIA|RGT|BOT, 4, 2, { CENTER + (6 + 18) -8, CENTER + (6+18) - 0   } },
	{ STR | DIA | RGT | BOT, 4, 1,{ CENTER + (6 + 18) -4, CENTER + (6 + 18) - 4  } },
	{ STR | DIA | RGT | BOT, 4, 0,{ CENTER + (6 + 18) +0 , CENTER + (6 + 18) - 8  } },
	
	{ STR | DIA | LFT | TOP, 2, 2,{ CENTER - CENTER + 12 - 1 , CENTER - (6 + 18) + 2 } },
	{ STR | DIA | LFT | TOP, 2, 1,{ CENTER - CENTER + 12 - 1 +4, CENTER - (6 + 18) - 2 } },
	{ STR|DIA|LFT|TOP, 2, 0, { CENTER-CENTER + 12 + 8 -1, CENTER - (6+18)-6 } },
	{ STR|DIA|LFT|TOP, 2, 0, { CENTER + (6+18) - 2, CENTER + (6+16)+4  } },
	{ STR|DIA|LFT|TOP, 2, 1, { CENTER + (6+18) + 2, CENTER + (6+16)-0  } },
	//{ STR|DIA|LFT|TOP, 2, 2, { CENTER+CENTER - (8-1)-12, CENTER + (3+16)+3 + 8 } },

	{ STR|DIA|LFT|BOT, 5, 0, { CENTER - (6+18- 4) -2, CENTER+(6+18) } },
	{ STR|DIA|LFT|BOT, 5, 1, { CENTER - (6 + 18 + 0) -2, CENTER+(6+18)-4 } },
	{ STR|DIA|LFT|BOT, 5, 2, { CENTER - (6 + 18 + 4) -2, CENTER+(6+18)-8 } },
	{ STR|DIA|LFT|BOT, 5, 0, { CENTER + (18 + 6) - 2 , CENTER - (6+18)-4 - 4} },
	{ STR|DIA|LFT|BOT, 5, 1, { CENTER + (18+6)+4 - 2 , CENTER - (6+18)-4  } },
	//{ STR|DIA|LFT|BOT, 5, 2, { CENTER+CENTER - (8-1)-12, CENTER + (3+16)-3 - 8 } },

	{ STR|DIA|RGT|TOP, 3, 0, { CENTER - ( 6+18) - 4 , CENTER+(6+18)+2  } },
	{ STR|DIA|RGT|TOP, 3, 1, { CENTER - (6 + 18) - 8 , CENTER+(6+18)+2-4  } },
	//{ STR|DIA|RGT|TOP, 3, 2, { CENTER + 8-1+8, CENTER-(3+16)+3-8 } },
	{ STR|DIA|RGT|TOP, 3, 0, { CENTER + (6+18) - 8, CENTER - (6+18) - 4 - 2 } },
	{ STR|DIA|RGT|TOP, 3, 1, { CENTER + (6 + 18) - 4, CENTER - (6+18) + 0 - 2 } },
	{ STR|DIA|RGT|TOP, 3, 2, { CENTER + (6 + 18) + 0, CENTER - (6+18) + 4 - 2 } },


};


static struct local_data{
	const char *curve_path;
	const char *straight_path;
	const char *base_path;
	PRENDERER render;
	RCOORD scale;
	PLIST images;
	int half_size;
	Image half_temp;
	Image full_temp;
	Image composites[10];
} l;

static void drawImages( Image dest, int drawpart, int rem, int hr, int xofs, int yofs )
{
#define SCALE 1
	int part;
	int seg;
	for( part = (drawpart?drawpart-1:0)+(rem?5:0); part < (drawpart?drawpart:num_parts) + (rem ? 5 : 0); part++ ) {
		for( seg = 0; seg < 40	//segment_count
				; seg++ ) {
			BlotScaledImageSizedEx( dest, hr? image_srcs[circle[seg].type_index].high[part] :image_srcs[circle[seg].type_index].low[part]
										, circle[seg].pos.x * X_HALF_UNITS *(hr?2:1) / SCALE
										, (circle[seg].pos.y) * Y_HALF_UNITS*(hr ? 2 : 1) / SCALE
										, image_srcs[circle[seg].type_index].w*(hr ? 2 : 1) /SCALE, image_srcs[circle[seg].type_index].h*(hr ? 2 : 1) / SCALE
										, circle[seg].variant * image_srcs[circle[seg].type_index].w*(hr ? 2 : 1), 0
										, image_srcs[circle[seg].type_index].w*(hr ? 2 : 1)
										, image_srcs[circle[seg].type_index].h*(hr ? 2 : 1)
										, ALPHA_TRANSPARENT
										, BLOT_COPY );
		}
	}
}

static void CPROC MyDraw( uintptr_t psv, PRENDERER r ) {
	Image surface = GetDisplayImage( r );
	drawImages( surface, 0, 1, 0, 0, 0 );
	UpdateDisplay(r);
}



static void loadImages( void ){
	int p;
	char buf[256];
	{
		int res;
		for( res = 0; res < 2; res++ ) {
			for( p = 0; p < 10; p++ ) {
				snprintf( buf, 256, "%s/%slayer-%s%s.png"
					, l.base_path
					, res?"hr-":""
					, parts[p%num_parts]
					, p >= num_parts ? "-" remnant :"");
				l.composites[p + res*5] = LoadImageFile(  buf );
			}
		}
	}
}


SaneWinMain( argc, argv ) {
	char buf[256];
	if( argc > 1 ) {
		l.base_path = StrDup( argv[1] );
	}
	loadImages();

	l.render = OpenDisplaySizedAt( 0, l.half_size, l.half_size, 0, 0 );
	SetRedrawHandler( l.render, MyDraw, 0 );
	UpdateDisplay( l.render );
	while( 1 )
		WakeableSleep( 10000 );
	return 0;
}
EndSaneWinMain()