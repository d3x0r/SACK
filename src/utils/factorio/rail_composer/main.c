#include <stdhdrs.h>
#include <filesys.h>
//#define DEFINE_DEFAULT_IMAGE_INTERFACE
#define USE_IMAGE_INTERFACE l.pii
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

// in overlapped areas, this is the alpha that should be used.
uint8_t alpha_translation[] = {
	/* 0*/    1 , 1 , 2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8
	/*16*/ ,  9 , 9 ,10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 17
	/*32*/ , 17 ,18 ,18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25, 25
	/*48*/ , 26 ,26 ,27, 27, 28, 29, 29, 30, 30, 31, 31, 32, 33, 33, 34, 34
	/*64*/ , 35 ,35 ,36, 37, 37, 38, 38, 39, 40, 40, 41, 41, 42, 43, 43, 44
	/*80*/ , 44 ,45 ,46, 46, 47, 47, 48, 49, 49, 50, 50, 51, 52, 52, 53, 54
	/*96*/ , 54 ,55 ,56, 56, 57, 57, 58, 59, 59, 60, 61, 61, 62, 63, 63, 64
	/*112*/, 65 ,65 ,66, 67, 67, 68, 69, 69, 70, 71, 71, 72, 73, 74, 74, 75
	/*128*/, 76 ,76 ,77, 78, 79, 79, 80, 81, 81, 82, 83, 84, 84, 85, 86, 87
	/*144*/, 87 ,88 ,89, 90, 90, 91, 92, 93, 94, 94, 95, 96, 97, 98, 98, 99
	/*160*/,100,101,102,103,103,104,105,106,107,108,108,109,110,111,112,113
	/*176*/,114,115,116,117,117,118,119,120,121,122,123,124,125,126,127,128
	/*192*/,129,130,131,132,133,134,135,136,137,138,140,141,142,143,144,145
	/*208*/,146,147,149,150,151,152,154,155,156,157,159,160,161,163,164,165
	/*224*/,167,168,170,171,173,174,176,178,179,181,183,184,186,188,190,192
	/*240*/,194,196,198,201,203,205,208,211,214,217,220,224,228,233,240,255
	/*256*/
};

struct image_info {
	int type;
	const char *prefix;
	const char *variant;
	int w, h;
	int ox, oy;
	LOGICAL _3variations;
	int *high_x, *high_y;
	int *low_x, *low_y;
	Image low[12];
	Image lowMask;
	Image high[12];
	Image highMask;

};

#define TOTAL_HALF_UNITS 80
#define CENTER  40

#define X_HALF_UNITS 16
#define Y_HALF_UNITS 16

#define STR  1
#define DIA  2
#define CRV  256
#define VER  4
#define HOR  8
#define LFT  16
#define TOP  32
#define RGT  64
#define BOT  128

//#define X_UNITS 32
//#define Y_UNITS 32

int coords[][3] = {
	{1120,1248,1376},{32,32,32},{560,624,688},{16,16,16},
	{32,32,32},{1120,1248,1376},{16,16,16},{560,624,688},
	{608,480,352},{352,480,608},{304,240,176},{176,240,304},
	{1824,1952,2080},{352,480,608},{912,976,1040},{176,240,304},
	{2080,1952,1824},{1824,1952,2080},{1040,976,912},{912,976,1040},
	{608,480,352},{2080,1952,1824},{304,240,176},{1040,976,912},
	{2240,0,0},{1504,0,0},{1120,0,0},{752,0,0},
	{0,0,0},{1504,0,0},{0,0,0},{752,0,0},
	{0,0,0},{544,0,0},{0,0,0},{272,0,0},
	{2240,0,0},{544,0,0},{1120,0,0},{272,0,0},
	{1504,0,0},{2240,0,0},{752,0,0},{1120,0,0},
	{544,0,0},{2240,0,0},{272,0,0},{1120,0,0},
	{544,0,0},{0,0,0},{272,0,0},{0,0,0},
	{1504,0,0},{0,0,0},{752,0,0},{0,0,0},

};


struct image_info image_srcs[] = {
	{STR|HOR               ,"straight-rail", "horizontal"             , 64 , 128,  0,  0, TRUE , coords[0], coords[1], coords[2], coords[3] },
	{STR | VER             ,"straight-rail", "vertical"               , 128, 64 ,  0,  0, TRUE , coords[(4 *  1) + 0], coords[(4 *  1) + 1], coords[(4 *  1) + 2], coords[(4 *  1) + 3] },
	{DIA|LFT|TOP           ,"straight-rail", "diagonal-left-top"      , 96 , 96 ,  1,  1, TRUE , coords[(4 *  2) + 0], coords[(4 *  2) + 1], coords[(4 *  2) + 2], coords[(4 *  2) + 3] },
	{DIA|RGT|TOP           ,"straight-rail", "diagonal-right-top"     , 96 , 96 , -1,  1, TRUE , coords[(4 *  3) + 0], coords[(4 *  3) + 1], coords[(4 *  3) + 2], coords[(4 *  3) + 3] },
	{DIA|RGT|BOT           ,"straight-rail", "diagonal-right-bottom"  , 96 , 96 , -1, -1, TRUE , coords[(4 *  4) + 0], coords[(4 *  4) + 1], coords[(4 *  4) + 2], coords[(4 *  4) + 3] },
	{DIA|LFT|BOT           ,"straight-rail", "diagonal-left-bottom"   , 96 , 96 ,  1, -1, TRUE , coords[(4 *  5) + 0], coords[(4 *  5) + 1], coords[(4 *  5) + 2], coords[(4 *  5) + 3] },
	{ CRV | VER | LFT | TOP,"curved-rail"  , "vertical-left-top"      , 192, 288,  1,  1, FALSE, coords[(4 *  6) + 0], coords[(4 *  6) + 1], coords[(4 *  6) + 2], coords[(4 *  6) + 3] },
	{ CRV | VER | RGT | TOP,"curved-rail"  , "vertical-right-top"     , 192, 288, -1,  1, FALSE, coords[(4 *  7) + 0], coords[(4 *  7) + 1], coords[(4 *  7) + 2], coords[(4 *  7) + 3] },
	{ CRV | VER | RGT | BOT,"curved-rail"  , "vertical-right-bottom"  , 192, 288, -1, -1, FALSE, coords[(4 *  8) + 0], coords[(4 *  8) + 1], coords[(4 *  8) + 2], coords[(4 *  8) + 3] },
	{ CRV | VER | LFT | BOT,"curved-rail"  , "vertical-left-bottom"   , 192, 288,  1, -1, FALSE, coords[(4 *  9) + 0], coords[(4 *  9) + 1], coords[(4 *  9) + 2], coords[(4 *  9) + 3] },
	{ CRV | HOR | LFT | TOP,"curved-rail"  , "horizontal-left-top"    , 288, 192,  1,  1, FALSE, coords[(4 * 10) + 0], coords[(4 * 10) + 1], coords[(4 * 10) + 2], coords[(4 * 10) + 3] },
	{ CRV | HOR | RGT | TOP,"curved-rail"  , "horizontal-right-top"   , 288, 192, -1,  1, FALSE, coords[(4 * 11) + 0], coords[(4 * 11) + 1], coords[(4 * 11) + 2], coords[(4 * 11) + 3] },
	{ CRV | HOR | RGT | BOT,"curved-rail"  , "horizontal-right-bottom", 288, 192, -1, -1, FALSE, coords[(4 * 12) + 0], coords[(4 * 12) + 1], coords[(4 * 12) + 2], coords[(4 * 12) + 3] },
	{ CRV | HOR | LFT | BOT,"curved-rail"  , "horizontal-left-bottom" , 288, 192,  1, -1, FALSE, coords[(4 * 13) + 0], coords[(4 * 13) + 1], coords[(4 * 13) + 2], coords[(4 * 13) + 3] },
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
const char *parts[] = { bed_back, bed, ties, metals, backplates };

const char *curve_prefix = "curved-rail";
const char *hr_curve_prefix = "hr-curved-rail";
const char *straight_prefix = "straight-rail";
const char *hr_straight_prefix = "hr-straight-rail";


struct segment {
	int type;
	int type_index;
	int variant;
	int save_coords;
	struct {
		int x, y;
	} pos;
};

#define segment_count (sizeof(circle)/sizeof(circle[0]))

const struct segment circle[] = {
	{ STR|VER        , 1, 0, 1, { CENTER - CENTER , CENTER - 6 } },
	{ STR|VER        , 1, 1, 1, { CENTER - CENTER, CENTER - 2 } },
	{ STR|VER        , 1, 2, 1, { CENTER - CENTER, CENTER + 2 } },
	{ STR|VER        , 1, 0, 0, { CENTER + CENTER-8, CENTER - 6 } },
	{ STR|VER        , 1, 1, 0, { CENTER + CENTER-8, CENTER - 2 } },
	{ STR|VER        , 1, 2, 0, { CENTER + CENTER-8, CENTER + 2 } },

	{ STR|HOR        , 0, 0, 1, { CENTER - 6 , CENTER - CENTER   } },
	{ STR|HOR        , 0, 1, 1, { CENTER - 2 , CENTER - CENTER   } },
	{ STR|HOR        , 0, 2, 1, { CENTER + 2 , CENTER - CENTER   } },
	{ STR|HOR        , 0, 0, 0, { CENTER - 6 , CENTER + CENTER-8 } },
	{ STR|HOR        , 0, 1, 0, { CENTER - 2 , CENTER + CENTER-8 } },
	{ STR|HOR        , 0, 2, 0, { CENTER + 2 , CENTER + CENTER-8 } },

	{ CRV|VER|RGT|TOP, 7, 0, 1, { CENTER - CENTER, CENTER + (6) -1 } },
	{ CRV|VER|RGT|BOT, 8, 0, 1,{ CENTER - CENTER, CENTER - (18 + 6) +1 } },
	{ CRV|VER|LFT|TOP, 6, 0, 1,{ CENTER + CENTER - 12, CENTER + (6) -1 } },
	{ CRV|VER|LFT|BOT, 9, 0, 1,{ CENTER + CENTER - 12, CENTER - (18 + 6) + 1 } },

	{ CRV|HOR|RGT|BOT, 12, 0, 1,{ CENTER - 16 - 8 +1, CENTER - CENTER + 0  } },
	{ CRV|HOR|RGT|TOP, 11, 0, 1,{ CENTER - 16 - 8 +1, CENTER + CENTER - 12 } },
	{ CRV|HOR|LFT|BOT, 13, 0, 1,{ CENTER + 6 - 1, CENTER - CENTER + 0 } },
	{ CRV|HOR|LFT|TOP, 10, 0, 1,{ CENTER + 6 - 1, CENTER + CENTER - 12 } },

	
	{ STR|DIA|RGT|BOT, 4, 0, 0, { CENTER-CENTER + 12 - 3, CENTER  - (6+18 + 4) + 1 } },
	{ STR|DIA|RGT|BOT, 4, 1, 0, { CENTER-CENTER + 12+4 - 3, CENTER - (6+18 + 4)-4 + 1 } },
	{ STR|DIA|RGT|BOT, 4, 2, 1, { CENTER + (6 + 18) -8 + 1, CENTER + (6+18) - 0 +1 } },
	{ STR|DIA|RGT|BOT, 4, 1, 1, { CENTER + (6 + 18) -4 + 1, CENTER + (6 + 18) - 4 +1 } },
	{ STR|DIA|RGT|BOT, 4, 0, 1, { CENTER + (6 + 18) +0 +1, CENTER + (6 + 18) - 8 +1 } },
	
	{ STR | DIA | LFT | TOP, 2, 2,1,{ CENTER - CENTER + 12 + 0 - 3, CENTER - (6 + 18) + 3 -2 } },
	{ STR | DIA | LFT | TOP, 2, 1,1,{ CENTER - CENTER + 12 + 4 - 3, CENTER - (6 + 18) - 1 -2 } },
	{ STR | DIA | LFT | TOP, 2, 0,1,{ CENTER - CENTER + 12 + 8 - 3, CENTER - (6 + 18) - 5 - 2 } },
	{ STR|DIA|LFT|TOP, 2, 0, 0,{ CENTER + (6+18) - 4 + 1, CENTER + (6+16)+4 -1 } },
	{ STR|DIA|LFT|TOP, 2, 1, 0,{ CENTER + (6+18) + 0 + 1, CENTER + (6+16)-0 -1 } },
	//{ STR|DIA|LFT|TOP, 2, 2, { CENTER+CENTER - (8-1)-12, CENTER + (3+16)+3 + 8 } },

	{ STR|DIA|LFT|BOT, 5, 0, 1,{ CENTER - (6+18- 4) -3, CENTER+(6+18) + 1 } },
	{ STR|DIA|LFT|BOT, 5, 1, 1,{ CENTER - (6 + 18 + 0) -3, CENTER+(6+18)-4 + 1 } },
	{ STR|DIA|LFT|BOT, 5, 2, 1,{ CENTER - (6 + 18 + 4) -3, CENTER+(6+18)-8 +1 } },
	{ STR|DIA|LFT|BOT, 5, 0, 0,{ CENTER + (18 + 6) - 4 +1 , CENTER - (6+18)-4 - 4+1} },
	{ STR|DIA|LFT|BOT, 5, 1, 0,{ CENTER + (18+6)+4 - 4 + 1, CENTER - (6+18)-4  +1} },
	//{ STR|DIA|LFT|BOT, 5, 2, { CENTER+CENTER - (8-1)-12, CENTER + (3+16)-3 - 8 } },

	{ STR|DIA|RGT|TOP, 3, 0, 0,{ CENTER - ( 6+18) - 4 +1, CENTER+(6+18)+2 -1  } },
	{ STR|DIA|RGT|TOP, 3, 1, 0,{ CENTER - (6 + 18) - 8 +1 , CENTER+(6+18)+2-4 -1 } },
	//{ STR|DIA|RGT|TOP, 3, 2, { CENTER + 8-1+8, CENTER-(3+16)+3-8 } },
	{ STR|DIA|RGT|TOP, 3, 0, 1,{ CENTER + (6+18) - 8 +1 , CENTER - (6+18) - 4 - 4 + 1 } },
	{ STR|DIA|RGT|TOP, 3, 1, 1,{ CENTER + (6 + 18) - 4 +1 , CENTER - (6+18) + 0 - 4 + 1 } },
	{ STR|DIA|RGT|TOP, 3, 2, 1,{ CENTER + (6 + 18) + 0 +1, CENTER - (6+18) + 4 - 4 + 1 } },
};

static struct local_data{
	const char *curve_path;
	const char *straight_path;
	const char *base_path;
	const char *output_path;
	PRENDERER render;
	RCOORD scale;
	PLIST images;
	int half_size;
	Image half_temp;
	Image full_temp;
	struct {
		Image hcurve; // 288x192   12x8
		Image vcurve; // 192x288    8x12 
		Image hor;  // 192x128  (  12*X_HALF_UNITS  x 8*(X_HALF_UNITS) )
		Image ver;  // 384x64  (  24*X_HALF_UNITS  x 4*(X_HALF_UNITS) )
		Image small2;  // 288x96  (  18*X_HALF_UNITS  x 6*(X_HALF_UNITS) )
		Image raw_hcurve; // 288x192   12x8
		Image raw_vcurve; // 192x288    8x12 
		Image raw_hor;  // 192x128  (  12*X_HALF_UNITS  x 8*(X_HALF_UNITS) )
		Image raw_ver;  // 384x64  (  24*X_HALF_UNITS  x 4*(X_HALF_UNITS) )
		Image raw_small2;  // 288x96  (  18*X_HALF_UNITS  x 6*(X_HALF_UNITS) )
	} low;
	struct {
		Image hcurve; // 288x192   12x8
		Image vcurve; // 192x288    8x12 
		Image hor;  // 192x128  (  12*X_HALF_UNITS  x 8*(X_HALF_UNITS) )
		Image ver;  // 384x64  (  24*X_HALF_UNITS  x 4*(X_HALF_UNITS) )
		Image small2;  // 288x96  (  18*X_HALF_UNITS  x 6*(X_HALF_UNITS) )
		Image raw_hcurve; // 288x192   12x8
		Image raw_vcurve; // 192x288    8x12 
		Image raw_hor;  // 192x128  (  12*X_HALF_UNITS  x 8*(X_HALF_UNITS) )
		Image raw_ver;  // 384x64  (  24*X_HALF_UNITS  x 4*(X_HALF_UNITS) )
		Image raw_small2;  // 288x96  (  18*X_HALF_UNITS  x 6*(X_HALF_UNITS) )
	} high;

	PIMAGE_INTERFACE pii;
} l;

static void drawImages( Image dest, int drawpart, int rem, int hr, int xofs, int yofs, int drawMask )
{
	int xStart = xofs;
	int yStart = yofs;
	int ulX;
	int ulY;
	int part;
	int seg;
	int hrneg = 0;
	if( hr < 0 )
	{
		hr = (-hr) - 1;
		hrneg = 1;
	}
	for( part = (drawpart?drawpart-1:0)+(rem?5:0); part < (drawpart?drawpart:num_parts) + (rem ? 5 : 0); part++ ) {
		for( seg = 0; seg < 40	//segment_count
				; seg++ ) {
			int type = circle[seg].type_index;
			//if( !drawpart )
			//	if( !circle[seg].save_coords )
			//		continue;
			ulX = (xStart + circle[seg].pos.x + image_srcs[circle[seg].type_index].ox) * X_HALF_UNITS * (hr ? 2 : 1);
			ulY = (yStart + circle[seg].pos.y + image_srcs[circle[seg].type_index].oy) * Y_HALF_UNITS * (hr ? 2 : 1);
			if( !hrneg ) {
				if( hr && circle[seg].save_coords ) {
					image_srcs[type].high_x[circle[seg].variant] = ulX;
					image_srcs[type].high_y[circle[seg].variant] = ulY;
				}
				else if( circle[seg].save_coords ) {
					image_srcs[type].low_x[circle[seg].variant] = ulX;
					image_srcs[type].low_y[circle[seg].variant] = ulY;
				}

				//circle[seg].variant * image_srcs[circle[seg].type_index].w*(hr ? 2 : 1)
				BlotScaledImageSizedEx( dest, hr ? image_srcs[circle[seg].type_index].high[part] : image_srcs[circle[seg].type_index].low[part]
					, ulX
					, ulY
					, image_srcs[circle[seg].type_index].w*(hr ? 2 : 1), image_srcs[circle[seg].type_index].h*(hr ? 2 : 1)
					, circle[seg].variant * image_srcs[circle[seg].type_index].w*(hr ? 2 : 1), 0
					, image_srcs[circle[seg].type_index].w*(hr ? 2 : 1), image_srcs[circle[seg].type_index].h*(hr ? 2 : 1)
					, ALPHA_TRANSPARENT
					, BLOT_COPY );
			}
				
			if( drawMask )
				if( drawpart ? part == (drawpart - 1) : !part )
					BlotScaledImageSizedEx( dest, hr ? image_srcs[circle[seg].type_index].highMask : image_srcs[circle[seg].type_index].lowMask
						, ulX
						, ulY
						, image_srcs[circle[seg].type_index].w*(hr ? 2 : 1), image_srcs[circle[seg].type_index].h*(hr ? 2 : 1)
						, circle[seg].variant * image_srcs[circle[seg].type_index].w*(hr ? 2 : 1), 0
						, image_srcs[circle[seg].type_index].w*(hr ? 2 : 1)
						, image_srcs[circle[seg].type_index].h*(hr ? 2 : 1)
						, ALPHA_TRANSPARENT
						, BLOT_COPY );
			if( drawMask && hrneg ) 
				if( drawpart ? part == (drawpart - 1) : !part )
				{
					do_hline( dest, ulY
						, ulX
						, ulX + (image_srcs[circle[seg].type_index].w*(hr ? 2 : 1))
						, BASE_COLOR_GREEN
					);
					do_hline( dest, ulY + (image_srcs[circle[seg].type_index].h*(hr ? 2 : 1)) - 1
						, ulX
						, ulX + (image_srcs[circle[seg].type_index].w*(hr ? 2 : 1))
						, BASE_COLOR_RED
					);


					do_hline( dest, ulY + (image_srcs[circle[seg].type_index].h*(hr ? 2 : 1)) / 2
						, ulX + (image_srcs[circle[seg].type_index].w*(hr ? 2 : 1)) / 2 - 10
						, ulX + (image_srcs[circle[seg].type_index].w*(hr ? 2 : 1)) / 2 + 10
						, BASE_COLOR_WHITE
					);

					do_vline( dest, ulX
						, ulY
						, ulY + (image_srcs[circle[seg].type_index].h*(hr ? 2 : 1))
						, BASE_COLOR_GREEN
					);
					do_vline( dest, ulX + (image_srcs[circle[seg].type_index].w*(hr ? 2 : 1)) - 1
						, ulY
						, ulY + (image_srcs[circle[seg].type_index].h*(hr ? 2 : 1))
						, BASE_COLOR_RED
					);

					do_vline( dest, ulX + (image_srcs[circle[seg].type_index].w*(hr ? 2 : 1)) / 2
						, ulY + (image_srcs[circle[seg].type_index].h*(hr ? 2 : 1)) / 2 - 10
						, ulY + (image_srcs[circle[seg].type_index].h*(hr ? 2 : 1)) / 2 + 10
						, BASE_COLOR_WHITE
					);

					if( circle[seg].type & DIA ) {
						switch( circle[seg].type & (LFT | RGT | TOP | BOT) ) {
						case LFT | TOP:
							do_line( dest, (ulX + X_HALF_UNITS * 4 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 0 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 5 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 1 * (hr ? 2 : 1)), BASE_COLOR_CYAN );
							do_line( dest, (ulX + X_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 1 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 6 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 1 * (hr ? 2 : 1)), BASE_COLOR_CYAN );
							do_line( dest, (ulX + X_HALF_UNITS * 0 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 4 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 1 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 5 * (hr ? 2 : 1)), BASE_COLOR_MAGENTA );
							do_line( dest, (ulX + X_HALF_UNITS * 1 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 1 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 6 * (hr ? 2 : 1)), BASE_COLOR_MAGENTA );
							break;
						case RGT | TOP:
							do_line( dest, (ulX + X_HALF_UNITS * 2 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 0 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 1 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 1 * (hr ? 2 : 1)), BASE_COLOR_CYAN );
							do_line( dest, (ulX + X_HALF_UNITS * 1 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 1 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 0 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 1 * (hr ? 2 : 1)), BASE_COLOR_CYAN );
							do_line( dest, (ulX + X_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 6 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 5 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 5 * (hr ? 2 : 1)), BASE_COLOR_MAGENTA );
							do_line( dest, (ulX + X_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 6 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 4 * (hr ? 2 : 1)), BASE_COLOR_MAGENTA );
							break;
						case LFT | BOT:
							do_line( dest, (ulX + X_HALF_UNITS * 0 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 2 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 1 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 1 * (hr ? 2 : 1)), BASE_COLOR_CYAN );
							do_line( dest, (ulX + X_HALF_UNITS * 1 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 1 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 1 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 0 * (hr ? 2 : 1)), BASE_COLOR_CYAN );
							do_line( dest, (ulX + X_HALF_UNITS * 6 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 5 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 5 * (hr ? 2 : 1)), BASE_COLOR_MAGENTA );
							do_line( dest, (ulX + X_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 4 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 6 * (hr ? 2 : 1)), BASE_COLOR_MAGENTA );
							break;
						case RGT | BOT:
							do_line( dest, (ulX + X_HALF_UNITS * 0 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 1 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 5 * (hr ? 2 : 1)), BASE_COLOR_CYAN );
							do_line( dest, (ulX + X_HALF_UNITS * 1 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 2 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 6 * (hr ? 2 : 1)), BASE_COLOR_CYAN );
							do_line( dest, (ulX + X_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 0 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 5 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 1 * (hr ? 2 : 1)), BASE_COLOR_MAGENTA );
							do_line( dest, (ulX + X_HALF_UNITS * 5 * (hr ? 2 : 1)), (ulY + Y_HALF_UNITS * 1 * (hr ? 2 : 1)), (ulX + X_HALF_UNITS * 6 * (hr ? 2 : 1)), yStart + (ulY + Y_HALF_UNITS * 2 * (hr ? 2 : 1)), BASE_COLOR_MAGENTA );
							break;
						}
					}
				}
		}
	}
}

static void CPROC MyDraw( uintptr_t psv, PRENDERER r ) {
	Image surface = GetDisplayImage( r );
	//ClearImageTo( surface, BASE_COLOR_WHITE );
	ClearImage( surface );
	drawImages( surface, 1, 0, 0, 1, 1, 1 );
	UpdateDisplay(r);
}


static Image createSegmentMask( int type, int hiRes ) {
	int x, y;
	int maxx, maxy;
	int units = X_HALF_UNITS * (hiRes ? 2 : 1);
	Image image = NULL;
	CDATA c;
	CDATA *out;
	if( type & STR ) {
		if( type & HOR ) {
			image = MakeImageFile( (maxx = 4 * units) * 3, maxy = 8 * units );
			ClearImageTo( image, SetAlpha( BASE_COLOR_PURPLE, 64 ) );
		}
		if( type & VER ) {
			image = MakeImageFile( (maxx = 8 * units) * 3, maxy = 16 * units );
			ClearImageTo( image, SetAlpha( BASE_COLOR_PURPLE, 64 ) );
		}
	}
	else if( type & DIA ) {
		switch( type & (TOP | BOT | LFT | RGT) ) {
		case TOP | RGT:
			c = SetAlpha( BASE_COLOR_BLUE, 64 );
			image = MakeImageFile( (maxx = 6 * units) * 3, maxy = 6 * units );
			ClearImage( image );
			out = GetImageSurface( image ) + (maxy-1)*maxx*3;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						while( 1 ) {
							if( x < units )
								break;
							if( x < (2 * units - y) )
								break;
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
							break;
						}
					}
					else if( y < (units * 4) ) {
						out[0] = c;
						out[units * 6] = c;
						out[units * 12] = c;
					}
					else  if( y < (units * 5) ) {
						if( x < (units * 6 - (y-units * 4)) ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					else {
						if( x < units * 5 ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					out++;
				}
				out -= ((maxx * 3) + maxx);
			}
			break;
		case BOT | RGT:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			image = MakeImageFile( (maxx = 6 * units) * 3, maxy = 6 * units );
			ClearImage( image );
			out = GetImageSurface( image ) + (maxy - 1)*maxx * 3;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x < units * 5 ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					else if( y < 2*units ) {
						if( x < (units * 4+y) ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					else if( y < (units * 5) ) {
						out[0] = c;
						out[units * 6] = c;
						out[units * 12] = c;
					}
					else {
						if( x >= units + (y - units * 5) ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					out++;
				}
				out -= ((maxx * 3) + maxx);
			}
			break;
		case TOP | LFT:
			c = SetAlpha( BASE_COLOR_GREEN, 64 );
			image = MakeImageFile( (maxx = 6 * units) * 3, maxy = 6 * units );
			ClearImage( image );
			out = GetImageSurface( image ) + (maxy - 1)*maxx * 3;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x <= (units * 4 + y) ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					else if( y < (units * 4) ) {
						out[0] = c;
						out[units * 6] = c;
						out[units * 12] = c;
					}
					else if( y < (units * 5) ) {
						if( x >= (y - units * 4) ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					else {
						if( x >= units ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					out++;
				}
				out -= ( (maxx *3) + maxx );
			}
			break;
		case BOT | LFT:
			c = SetAlpha( BASE_COLOR_NICE_ORANGE, 64 );
			image = MakeImageFile( (maxx = 6 * units) * 3, maxy = 6 * units );
			ClearImage( image );
			out = GetImageSurface( image ) + (maxy - 1)*maxx * 3;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x >= units ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					else if( y < (units * 2) ) {
						if( x >= (2*units-y)  ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					else if( y < (units * 5) ) {
						out[0] = c;
						out[units * 6] = c;
						out[units * 12] = c;
					}
					else {
						if( x < (10*units-y) ) {
							out[0] = c;
							out[units * 6] = c;
							out[units * 12] = c;
						}
					}
					out++;
				}
				out -= ((maxx * 3) + maxx);
			}
			break;
		}
	}
	else if( type & CRV ){
		switch( type & (VER | HOR | RGT | LFT | TOP | BOT) ) {
		case VER | RGT | TOP:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			image = MakeImageFile( (maxx = 12 * units), maxy = 18 * units );
			ClearImage( image );
			//image->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx ;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units * 11 ) {
						out[0] = c;
					}
					else if( y < units * 12 ) {
						if( x < (units * 12 + ( units*11-y)) )
							out[0] = c;
					}
					else if( y < units * 16 ) {
						if( x < (units * 11) )
							out[0] = c;
					}
					else if( y < units * 17 ) {
						if( x < (units * 10 - (y - units * 17)) )
							out[0] = c;
					}
					else
						if( x < units * 9 )
							out[0] = c;
					out++;
				}
				out -= maxx * 2;
			}
			break;
		case VER | RGT | BOT:
			c = SetAlpha( BASE_COLOR_BLUE, 64 );
			image = MakeImageFile( (maxx = 12 * units), maxy = 18 * units );
			ClearImage( image );
			//image->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x < units * 9 )
							out[0] = c;
					}
					else if( y < units * 2 ) {
						if( x < (units * 9 + y) )
							out[0] = c;
					}
					else if( y < units * 6 ) {
						if( x < (units * 11) )
							out[0] = c;
					}
					else if( y < units * 7 ) {
						if( x < (units * 11 + ( y-units*6) ) )
							out[0] = c;
					}
					else
						out[0] = c;
					out++;
				}
				out -= maxx * 2;
			}
			break;
		case VER | LFT | TOP:
			c = SetAlpha( BASE_COLOR_GREEN, 64 );
			image = MakeImageFile( (maxx = 12 * units), maxy = 18 * units );
			ClearImage( image );
			//image->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units * 11 ) {
						out[0] = c;
					}
					else if( y < units * 12 ) {
						if( x >= ( (y-units * 11)) )
							out[0] = c;
					}
					else if( y < units * 16 ) {
						if( x >= units )
							out[0] = c;
					}
					else if( y < units * 17 ) {
						if( x >= units + (y - units * 16) )
							out[0] = c;
					}
					else
						if( x >= units * 3 )
							out[0] = c;
					out++;
				}
				out -= maxx * 2;
			}
			break;
		case VER | LFT | BOT:
			c = SetAlpha( BASE_COLOR_NICE_ORANGE, 64 );
			image = MakeImageFile( (maxx = 12 * units), maxy = 18 * units );
			ClearImage( image );
			//image->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x >= (units * 3) )
							out[0] = c;
					}
					else if( y < units * 2 ) {
						if( x >= (units * 1 + (2 * units - y)) )
							out[0] = c;
					}
					else if( y < units * 6 ) {
						if( x >= (units) )
							out[0] = c;
					}
					else if( y < units * 7 ) {
						if( x >= ( ( units*7-y)) )
							out[0] = c;
					}
					else
						out[0] = c;
					out++;
				}
				out -= maxx * 2;
			}
			break;
		case HOR | RGT | BOT:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			image = MakeImageFile( (maxx = 18 * units), maxy = 12 * units );
			ClearImage( image );
			//image->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units * 9 ) {
						out[0] = c;
					}
					else if( y < units * 10 ) {
						if( x >= (units) )
							out[0] = c;
					}
					else if( y < units * 11 ) {
						if( x >= (units + (y - units * 10)) )
							out[0] = c;
					}
					else
						if( x >= units * 6 + ( y - units*11) )
							out[0] = c;
					out++;
				}
				out -= maxx * 2;
			}
			break;
		case HOR | RGT | TOP:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			image = MakeImageFile( (maxx = 18 * units), maxy = 12 * units );
			ClearImage( image );
			//image->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x >= (units*7-y) )
							out[0] = c;
					}
					else if( y < units * 2 ) {
						if( x >= (units) + ( units*2-y) )
							out[0] = c;
					}
					else if( y < units * 3 ) {
						if( x >= units )
							out[0] = c;
					}
					else
						out[0] = c;
					out++;
				}
				out -= maxx * 2;
			}
			break;
		case HOR | LFT | BOT:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			image = MakeImageFile( (maxx = 18 * units), maxy = 12 * units );
			ClearImage( image );
			//image->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units * 9 ) {
						out[0] = c;
					}
					else if( y < units * 10 ) {
						if( x < (units*17) )
							out[0] = c;
					}
					else if( y < units * 11 ) {
						if( x < (units*17 - (y - units*10)) )
							out[0] = c;
					}
					else
						if( x < ( units * 11 ) + (units * 12 - y  ) )
							out[0] = c;
					out++;
				}
				out -= maxx * 2;
			}
			break;
		case HOR | LFT | TOP:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			image = MakeImageFile( (maxx = 18 * units), maxy = 12 * units );
			ClearImage( image );
			//image->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x < units * 11+y )
							out[0] = c;
					}
					else if( y < units * 2 ) {
						if( x < (units*16)+(y-units ) )
							out[0] = c;
					}
					else if( y < units * 3 ) {
						if( x < units*17 )
							out[0] = c;
					}
					else
						out[0] = c;
					out++;
				}
				out -= maxx*2;
			}
			break;
		}
	}
	//FlipImage( image );
	return image;
}


static Image applySegmentMask( int type, int hiRes, Image input, Image output ) {
	int x, y;
	int maxx, maxy;
	int units = X_HALF_UNITS * (hiRes ? 2 : 1);
	Image image = output;
	CDATA c;
	CDATA *out;
	CDATA *in;
	ClearImage( output );
	if( type & STR ) {
		if( type & HOR ) {
			(maxx = 4 * units) * 3, maxy = 8 * units;
		}
		if( type & VER ) {
			(maxx = 8 * units) * 3, maxy = 16 * units;
		}
		BlotImage( output, input, 0, 0 );
	}
	else if( type & DIA ) {
		switch( type & (TOP | BOT | LFT | RGT) ) {
		case TOP | RGT:
			c = SetAlpha( BASE_COLOR_BLUE, 64 );
			(maxx = 6 * units), maxy = 6 * units;

			//image->flags |= IF_FLAG_INVERTED;
			//input->flags |= IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx * 3;
			in = GetImageSurface( input ) + (maxy - 1)*maxx * 3;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						while( 1 ) {
							if( x < units )
								break;
							if( x < (2 * units - y) )
								break;
							out[0] = (in[0]);
							out[units * 6] = (in[units * 6]);
							out[units * 12] = (in[units * 12]);
							break;
						}
					}
					else if( y < (units * 4) ) {
						out[0] = in[0];
						out[units * 6] = in[units*6];
						out[units * 12] = in[units * 12];
					}
					else  if( y < (units * 5) ) {
						if( x < (units * 6 - (y - units * 4)) ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					else {
						if( x < units * 5 ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					out++;
					in++;
				}
				out -= (maxx * 3) + maxx;
				in -= (maxx * 3) + maxx;
			}
			break;
		case BOT | RGT:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			(maxx = 6 * units), maxy = 6 * units;
			//image->flags |= IF_FLAG_INVERTED;
			//input->flags |= IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx * 3;
			in = GetImageSurface( input ) + (maxy - 1)*maxx * 3;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x < units * 5 ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					else if( y < 2 * units ) {
						if( x < (units * 4 + y) ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					else if( y < (units * 5) ) {
						out[0] = in[0];
						out[units * 6] = in[units * 6];
						out[units * 12] = in[units * 12];
					}
					else {
						if( x >= units + (y - units * 5) ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					out++;
					in++;
				}
				out -= (maxx * 3) + maxx;
				in -= (maxx * 3) + maxx;
			}
			break;
		case TOP | LFT:
			c = SetAlpha( BASE_COLOR_GREEN, 64 );
			(maxx = 6 * units), maxy = 6 * units;
			//image->flags |= IF_FLAG_INVERTED;
			//input->flags |= IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx * 3;
			in = GetImageSurface( input ) + (maxy - 1)*maxx * 3;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x <= (units * 4 + y) ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					else if( y < (units * 4) ) {
						out[0] = in[0];
						out[units * 6] = in[units * 6];
						out[units * 12] = in[units * 12];
					}
					else if( y < (units * 5) ) {
						if( x >= ( y - units * 4 ) ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					else {
						if( x >= units ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					out++;
					in++;
				}
				out -= (maxx * 3) + maxx;
				in -= (maxx * 3) + maxx;
			}
			break;
		case BOT | LFT:
			c = SetAlpha( BASE_COLOR_NICE_ORANGE, 64 );
			(maxx = 6 * units), maxy = 6 * units;
			//image->flags |= IF_FLAG_INVERTED;
			//input->flags |= IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx * 3;
			in = GetImageSurface( input ) + (maxy - 1)*maxx * 3;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x >= units ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					else if( y < (units * 2) ) {
						if( x >= (2 * units - y) ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					else if( y < (units * 5) ) {
						out[0] = in[0];
						out[units * 6] = in[units * 6];
						out[units * 12] = in[units * 12];
					}
					else {
						if( x < (10 * units - y) ) {
							out[0] = in[0];
							out[units * 6] = in[units * 6];
							out[units * 12] = in[units * 12];
						}
					}
					out++;
					in++;
				}
				out -= (maxx * 3) + maxx;
				in -= (maxx * 3) + maxx;
			}
			break;
		}
	}
	else if( type & CRV ) {
		switch( type & (VER | HOR | RGT | LFT | TOP | BOT) ) {
		case VER | RGT | TOP:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			(maxx = 12 * units), maxy = 18 * units;
			//image->flags &= ~IF_FLAG_INVERTED;
			//input->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			in = GetImageSurface( input ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units * 11 ) {
						out[0] = in[0];
					}
					else if( y < units * 12 ) {
						if( x < (units * 12 + (units * 11 - y)) )
							out[0] = in[0];
					}
					else if( y < units * 16 ) {
						if( x < (units * 11) )
							out[0] = in[0];
					}
					else if( y < units * 17 ) {
						if( x < (units * 10 - (y - units * 17)) )
							out[0] = in[0];
					}
					else
						if( x < units * 9 )
							out[0] = in[0];
					out++;
					in++;
				}
				out -= maxx*2;
				in -= maxx*2;
			}
			break;
		case VER | RGT | BOT:
			c = SetAlpha( BASE_COLOR_BLUE, 64 );
			(maxx = 12 * units), maxy = 18 * units;
			//image->flags &= ~IF_FLAG_INVERTED;
			//input->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			in = GetImageSurface( input ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x < units * 9 )
							out[0] = in[0];
					}
					else if( y < units * 2 ) {
						if( x < (units * 9 + y) )
							out[0] = in[0];
					}
					else if( y < units * 6 ) {
						if( x < (units * 11) )
							out[0] = in[0];
					}
					else if( y < units * 7 ) {
						if( x < (units * 11 + (y - units * 6)) )
							out[0] = in[0];
					}
					else
						out[0] = in[0];
					out++;
					in++;
				}
				out -= maxx * 2;
				in -= maxx * 2;
			}
			break;
		case VER | LFT | TOP:
			c = SetAlpha( BASE_COLOR_GREEN, 64 );
			(maxx = 12 * units), maxy = 18 * units;
			//image->flags &= ~IF_FLAG_INVERTED;
			//input->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			in = GetImageSurface( input ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units * 11 ) {
						out[0] = in[0];
					}
					else if( y < units * 12 ) {
						if( x >= ((y - units * 11)) )
							out[0] = in[0];
					}
					else if( y < units * 16 ) {
						if( x >= units )
							out[0] = in[0];
					}
					else if( y < units * 17 ) {
						if( x >= units + (y - units * 16) )
							out[0] = in[0];
					}
					else
						if( x >= units * 3 )
							out[0] = in[0];
					out++;
					in++;
				}
				out -= maxx * 2;
				in -= maxx * 2;
			}
			break;
		case VER | LFT | BOT:
			c = SetAlpha( BASE_COLOR_NICE_ORANGE, 64 );
			(maxx = 12 * units), maxy = 18 * units;
			//image->flags &= ~IF_FLAG_INVERTED;
			//input->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			in = GetImageSurface( input ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x >= (units * 3) )
							out[0] = in[0];
					}
					else if( y < units * 2 ) {
						if( x >= (units * 1 + (2 * units - y)) )
							out[0] = in[0];
					}
					else if( y < units * 6 ) {
						if( x >= (units) )
							out[0] = in[0];
					}
					else if( y < units * 7 ) {
						if( x >= ((units * 7 - y)) )
							out[0] = in[0];
					}
					else
						out[0] = in[0];
					out++;
					in++;
				}
				out -= maxx * 2;
				in -= maxx * 2;
			}
			break;
		case HOR | RGT | BOT:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			(maxx = 18 * units), maxy = 12 * units;
			//image->flags &= ~IF_FLAG_INVERTED;
			//input->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			in = GetImageSurface( input ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units * 9 ) {
						out[0] = in[0];
					}
					else if( y < units * 10 ) {
						if( x >= (units) )
							out[0] = in[0];
					}
					else if( y < units * 11 ) {
						if( x >= (units + (y - units * 10)) )
							out[0] = in[0];
					}
					else
						if( x >= units * 6 + (y - units * 11) )
							out[0] = in[0];
					out++;
					in++;
				}
				out -= maxx * 2;
				in -= maxx * 2;
			}
			break;
		case HOR | RGT | TOP:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			(maxx = 18 * units), maxy = 12 * units;
			//image->flags &= ~IF_FLAG_INVERTED;
			//input->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			in = GetImageSurface( input ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x >= (units * 7 - y) )
							out[0] = in[0];
					}
					else if( y < units * 2 ) {
						if( x >= (units)+(units * 2 - y) )
							out[0] = in[0];
					}
					else if( y < units * 3 ) {
						if( x >= units )
							out[0] = in[0];
					}
					else
						out[0] = in[0];
					out++;
					in++;
				}
				out -= maxx * 2;
				in -= maxx * 2;
			}
			break;
		case HOR | LFT | BOT:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			(maxx = 18 * units), maxy = 12 * units;
			//image->flags &= ~IF_FLAG_INVERTED;
			//input->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			in = GetImageSurface( input ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units * 9 ) {
						out[0] = in[0];
					}
					else if( y < units * 10 ) {
						if( x < (units * 17) )
							out[0] = in[0];
					}
					else if( y < units * 11 ) {
						if( x < (units * 17 - (y - units * 10)) )
							out[0] = in[0];
					}
					else
						if( x < (units * 11) + (units * 12 - y) )
							out[0] = in[0];
					out++;
					in++;
				}
				out -= maxx * 2;
				in -= maxx * 2;
			}
			break;
		case HOR | LFT | TOP:
			c = SetAlpha( BASE_COLOR_RED, 64 );
			(maxx = 18 * units), maxy = 12 * units;
			//image->flags &= ~IF_FLAG_INVERTED;
			//input->flags &= ~IF_FLAG_INVERTED;
			out = GetImageSurface( image ) + (maxy - 1)*maxx;
			in = GetImageSurface( input ) + (maxy - 1)*maxx;
			for( y = 0; y < maxy; y++ ) {
				for( x = 0; x < maxx; x++ ) {
					if( y < units ) {
						if( x < units * 11 + y )
							out[0] = in[0];
					}
					else if( y < units * 2 ) {
						if( x < (units * 16) + (y - units) )
							out[0] = in[0];
					}
					else if( y < units * 3 ) {
						if( x < units * 17 )
							out[0] = in[0];
					}
					else
						out[0] = in[0];
					out++;
					in++;
				}
				out -= maxx * 2;
				in -= maxx * 2;
			}
			break;
		}
	}
	//FlipImage( image );
	return image;
}



static Image createMask( Image fromImage ) {
	CDATA *surface = GetImageSurface( fromImage );
	uint32_t x, y;
	uint32_t width = fromImage->width;
	uint32_t height = fromImage->height;
	Image output = MakeImageFile( width, height );
	CDATA *outSurface = GetImageSurface( output );
	for( x = 0; x < width; x++ )
		for( y = 0; y < height; y++ ) {
			if( ((uint8_t*)surface)[3] ) {
				outSurface[0] = AColor( 255, 255, 255, 255 );
			}
			surface++;
			outSurface++;
		}
	return output;
}

static void Slice( INDEX i, int p, int res, Image *slice, Image *sliceMask ) {
	Image sliceTarget;
	if( image_srcs[i].type & STR ) {
		if( image_srcs[i].type & VER ) {
			(*slice) = sliceTarget = res ? l.high.raw_ver : l.low.raw_ver;
			(*sliceMask) = res ? l.high.ver : l.low.ver;
		}
		else {
			(*slice) = sliceTarget = res ? l.high.raw_hor : l.low.raw_hor;
			(*sliceMask) = res ? l.high.hor : l.low.hor;
		}

	} 
	else if( image_srcs[i].type &  DIA ) {
		(*slice) = sliceTarget = res ? l.high.raw_small2 : l.low.raw_small2;
		(*sliceMask) = res ? l.high.small2 : l.low.small2;
	}
	else
		if( image_srcs[i].type & VER ) {
			(*slice) = sliceTarget = res ? l.high.raw_vcurve : l.low.raw_vcurve;
			(*sliceMask) = res ? l.high.vcurve : l.low.vcurve;
		}
		else {
			(*slice) = sliceTarget = res ? l.high.raw_hcurve : l.low.raw_hcurve;
			(*sliceMask) = res ? l.high.hcurve : l.low.hcurve;
		}

	{
		Image src = (Image)GetLink( &l.images, p + (res?10:0) );
		int v;
		ClearImage( sliceTarget );
		for( v = 0; v < (image_srcs[i]._3variations ? 3 : 1); v++ ) {
			BlotImageSizedTo( sliceTarget, src
				, v*image_srcs[i].w*(res ? 2 : 1), 0
				, res ? image_srcs[i].high_x[v]:image_srcs[i].low_x[v], (res ? image_srcs[i].high_y[v] : image_srcs[i].low_y[v])
				, image_srcs[i].w*(res ? 2 : 1), image_srcs[i].h*(res ? 2 : 1) );
		}
	}
	//return sliceTarget;
}

static void sliceComposites( void ) {
	INDEX i;
	int p;
	char buf[256];

	for( i = 0; i < 14; i++ ) {
		int res;
		for( res = 0; res < 2; res++ ) {
			for( p = 0; p < num_parts * 2; p++ ) {
				Image slice;
				Image slice_mask;
				const char *base;
				if( image_srcs[i].type & (STR | DIA) )
					base = l.straight_path;
				else
					base = l.curve_path;

				Slice( i, p, res, &slice, &slice_mask );
				applySegmentMask( image_srcs[i].type, res, slice, slice_mask );

				snprintf( buf, 256, "%s/%s%s-%s-%s%s%s.png"
					, base, res ? "hr-" : "", image_srcs[i].prefix, image_srcs[i].variant
					, parts[p%num_parts]
					, p >= num_parts ? "-" : ""
					, p >= num_parts ? remnant : "" );

				{
					uint8_t *imgbuf = NULL;
					size_t size = 0;
					PngImageFile( slice_mask, &imgbuf, &size );
					{
						FILE *out;
						//printf( "write:%s\n", buf );
						out = sack_fopen( 0, buf, "wb" );
						sack_fwrite( imgbuf, size, 1, out );
						sack_fclose( out );
					}
					Release( imgbuf );
				}
			}
		}
	}

}

static void loadAndSlice( void ) {

	{
		int p;
		int r;
		char outname[256];
		for( r = 0; r < 2; r++ )
			for( p = 0; p < 5; p++ ) {
				snprintf( outname, 256, "%s/layer-%s%s.png", l.base_path, parts[p%5], r ? "-remnant" : "" );
				SetLink( &l.images, r*5+p, LoadImageFile( outname ) );

				snprintf( outname, 256, "%s/hr-layer-%s%s.png", l.base_path, parts[p%5], r ? "-remnant" : "" );
				SetLink( &l.images, 10 + (r * 5) + p, LoadImageFile( outname ) );
			}
	}

	{
		l.low.raw_hcurve = MakeImageFile( 18 * X_HALF_UNITS, 12 * X_HALF_UNITS );
		l.low.raw_vcurve = MakeImageFile( 12 * X_HALF_UNITS, 18 * X_HALF_UNITS );
		l.low.raw_hor = MakeImageFile( 12 * X_HALF_UNITS, 8 * X_HALF_UNITS );
		l.low.raw_ver = MakeImageFile( 24 * X_HALF_UNITS, 4 * X_HALF_UNITS );
		l.low.raw_small2 = MakeImageFile( 18 * X_HALF_UNITS, 6 * X_HALF_UNITS );

		l.high.raw_hcurve = MakeImageFile( 2 * 18 * X_HALF_UNITS, 2 * 12 * X_HALF_UNITS );
		l.high.raw_vcurve = MakeImageFile( 2 * 12 * X_HALF_UNITS, 2 * 18 * X_HALF_UNITS );
		l.high.raw_hor = MakeImageFile( 2 * 12 * X_HALF_UNITS, 2 * 8 * X_HALF_UNITS );
		l.high.raw_ver = MakeImageFile( 2 * 24 * X_HALF_UNITS, 2 * 4 * X_HALF_UNITS );
		l.high.raw_small2 = MakeImageFile( 2 * 18 * X_HALF_UNITS, 2 * 6 * X_HALF_UNITS );

		l.low.hcurve = MakeImageFile( 18 * X_HALF_UNITS, 12 * X_HALF_UNITS );
		l.low.vcurve = MakeImageFile( 12 * X_HALF_UNITS, 18 * X_HALF_UNITS );
		l.low.hor = MakeImageFile( 12 * X_HALF_UNITS, 8 * X_HALF_UNITS );
		l.low.ver = MakeImageFile( 24 * X_HALF_UNITS, 4 * X_HALF_UNITS );
		l.low.small2 = MakeImageFile( 18 * X_HALF_UNITS, 6 * X_HALF_UNITS );

		l.high.hcurve = MakeImageFile( 2* 18 * X_HALF_UNITS, 2 * 12 * X_HALF_UNITS );
		l.high.vcurve = MakeImageFile( 2 * 12 * X_HALF_UNITS, 2 * 18 * X_HALF_UNITS );
		l.high.hor = MakeImageFile( 2 * 12 * X_HALF_UNITS, 2 * 8 * X_HALF_UNITS );
		l.high.ver = MakeImageFile( 2 * 24 * X_HALF_UNITS, 2 * 4 * X_HALF_UNITS );
		l.high.small2 = MakeImageFile( 2 * 18 * X_HALF_UNITS, 2 * 6 * X_HALF_UNITS );

		sliceComposites();
		
	}

}

static void loadImages( void ){
	INDEX i;
	int p;
	char buf[256];
	for( i = 0; i < 6; i++ ) {
		int res;
		for( res = 0; res < 2; res++ ) {
			for( p = 0; p < num_parts*2; p++ ) {
				snprintf( buf, 256, "%s/%s%s-%s-%s%s%s.png"
								, l.straight_path, res?"hr-":"", image_srcs[i].prefix, image_srcs[i].variant
							, parts[p%num_parts]
							, p >= num_parts?"-":""
							, p >= num_parts ? remnant :"");
				if( res ) {
					if( !p )
						image_srcs[i].highMask = createSegmentMask( image_srcs[i].type, res );
					//printf( "Load:%s\n", buf );
					image_srcs[i].high[p] = LoadImageFile( buf );
				}
				else {
					if( !p )
						image_srcs[i].lowMask = createSegmentMask( image_srcs[i].type, res );
					//printf( "Load:%s\n", buf );
					image_srcs[i].low[p] = LoadImageFile( buf );
				}
			}
		}
	}
	printf( "Loaded all fragments....\n" );
	for( i = 6+0; i < 6+8; i++ ) {
		int res;
		for( res = 0; res < 2; res++ ) {
			for( p = 0; p < num_parts*2; p++ ){
				snprintf( buf, 256, "%s/%s%s-%s-%s%s.png", l.curve_path, res?"hr-":"", image_srcs[i].prefix, image_srcs[i].variant
							, parts[p%num_parts]
							, p >= num_parts ? "-"remnant:"" );
				if( res ) {
					if( !p )
						image_srcs[i].highMask = createSegmentMask( image_srcs[i].type, res );
					image_srcs[i].high[p] = LoadImageFile( buf );
				}
				else {
					if( !p )
						image_srcs[i].lowMask = createSegmentMask( image_srcs[i].type, res );
					image_srcs[i].low[p] = LoadImageFile( buf );
				}
			}
		}
	}
	l.half_size = 84 * X_HALF_UNITS;// (circle[15].pos.x + image_srcs[circle[15].type_index].ox + 1) * X_HALF_UNITS + image_srcs[9].low[0]->width;
	l.half_temp = MakeImageFile( l.half_size, l.half_size );
	l.full_temp = MakeImageFile( l.half_size*2, l.half_size * 2 );


	{
		char outname[256];
		Image mask = createSegmentMask( DIA | TOP | LFT, 0 );
		uint8_t *buf = NULL;
		size_t size = 0;
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/dia-top-lft-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );
		mask = createSegmentMask( DIA | TOP | RGT, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/dia-top-rgt-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );
		mask = createSegmentMask( DIA | BOT | LFT, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/dia-bot-lft-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );
		mask = createSegmentMask( DIA | BOT | RGT, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/dia-bot-rgt-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );

		mask = createSegmentMask( CRV| VER|RGT|TOP, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/crv-ver-rgt-top-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );

		mask = createSegmentMask( CRV | VER | RGT | BOT, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/crv-ver-rgt-bot-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );

		mask = createSegmentMask( CRV | VER | LFT | TOP, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/crv-ver-lft-top-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );

		mask = createSegmentMask( CRV | VER | LFT | BOT, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/crv-ver-lft-bot-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );

		mask = createSegmentMask( CRV | HOR | RGT | TOP, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/crv-hor-rgt-top-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );

		mask = createSegmentMask( CRV | HOR | RGT | BOT, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/crv-hor-rgt-bot-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );

		mask = createSegmentMask( CRV | HOR | LFT | TOP, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/crv-hor-lft-top-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );

		mask = createSegmentMask( CRV | HOR | LFT | BOT, 0 );
		PngImageFile( mask, &buf, &size );
		UnmakeImageFile( mask );
		{
			FILE *out;
			snprintf( outname, 256, "%s/crv-hor-lft-bot-mask.png", l.output_path );
			out = sack_fopen( 0, outname, "wb" );
			sack_fwrite( buf, size, 1, out );
			sack_fclose( out );
		}
		Release( buf );

	}
	printf( "wrote masks....\n" );
	{
		int p;
		int r;
		char outname[256];
		for( r = 0; r < 2; r++ )
			for( p = 0; p <= 5; p++ ) {
				ClearImageTo( l.half_temp, SetAlpha( BASE_COLOR_WHITE, 0 ) );
				ClearImageTo( l.full_temp, SetAlpha( BASE_COLOR_WHITE, 0 ) );
				{
					uint8_t *buf;
					size_t size;
					if( p == 0 ) {
						drawImages( l.half_temp, p, r, -1, 1, 1, p ? 0 : 1 );
						drawImages( l.full_temp, p, r, -2, 1, 1, p ? 0 : 1 );
						PngImageFile( l.half_temp, &buf, &size );
						{
							FILE *out;
							snprintf( outname, 256, "%s/layer-%s%s.png", l.output_path, p ? parts[p - 1] : "mask", r ? "-remnant" : "" );
							out = sack_fopen( 0, outname, "wb" );
							sack_fwrite( buf, size, 1, out );
							sack_fclose( out );
						}
						Release( buf );
						PngImageFile( l.full_temp, &buf, &size );
						{
							FILE *out;
							snprintf( outname, 256, "%s/hr-layer-%s%s.png", l.output_path, p ? parts[p - 1] : "mask", r ? "-remnant" : "" );
							out = sack_fopen( 0, outname, "wb" );
							sack_fwrite( buf, size, 1, out );
							sack_fclose( out );
						}
						Release( buf );
						ClearImageTo( l.half_temp, SetAlpha( BASE_COLOR_WHITE, 0) );
						ClearImageTo( l.full_temp, SetAlpha( BASE_COLOR_WHITE, 0 ) );
					}
					drawImages( l.half_temp, p, r, 0, 1, 1, p ? 0 : 1 *0 );
					drawImages( l.full_temp, p, r, 1, 1, 1, p ? 0 : 1 * 0 );
					PngImageFile( l.half_temp, &buf, &size );
					{
						FILE *out;
						snprintf( outname, 256, "%s/layer-%s%s.png", l.output_path, p ? parts[p - 1] : "composite", r?"-remnant":"" );
						out = sack_fopen( 0, outname, "wb" );
						sack_fwrite( buf, size, 1, out );
						sack_fclose( out );
					}
					Release( buf );
					PngImageFile( l.full_temp, &buf, &size );
					{
						FILE *out;
						snprintf( outname, 256, "%s/hr-layer-%s%s.png", l.output_path, p?parts[p-1]:"composite", r ? "-remnant" : "" );
						out = sack_fopen( 0, outname, "wb" );
						sack_fwrite( buf, size, 1, out );
						sack_fclose( out );
					}
					Release( buf );
				}
			}
	}
	printf( "wrote composites....\n" );
}


SaneWinMain( argc, argv ) {
	char buf[256];
	int arg;
	int slice = 0;
	int composite = 0;
	l.output_path = ".";
	for( arg = 1; arg < argc; arg++ ) {
		if( argv[arg][0] == '-' ) {
			switch( argv[arg][1] ) {
			case 'o':
				if( argv[arg][2] ) {
					l.output_path = StrDup( argv[arg] + 2 );
					printf( "Setting output path to: %s\n", l.output_path );
				}
				else
				{
					l.output_path = StrDup( argv[arg+1] );
					printf( "Setting output path to: %s\n", l.output_path );
					arg++;
				}
				break;
			case 's':
				slice = 1;
				break;
			case 'c':
				composite = 1;
				break;
			}
		}
		else {
			l.base_path = argv[arg];
		}
	}
	if( !l.base_path ) {
		printf( "base input path not specified.\n" );
		exit( 0 );
	}
	if( slice ) {
		snprintf( buf, 256, "%s/straight-rail", l.output_path );
		l.straight_path = StrDup( buf );
		MakePath( l.straight_path );
		snprintf( buf, 256, "%s/curved-rail", l.output_path );
		l.curve_path = StrDup( buf );
		MakePath( l.curve_path );
	}
	else {
		snprintf( buf, 256, "%s/straight-rail", l.base_path );
		l.straight_path = StrDup( buf );
		lprintf( "set stright:%s", l.straight_path );
		snprintf( buf, 256, "%s/curved-rail", l.base_path );
		l.curve_path = StrDup( buf );
		lprintf( "set stright:%s", l.curve_path );
	}
	MakePath( l.output_path );

	l.pii = GetImageInterface();
	if( slice ) {
		loadAndSlice();
	}
	if( composite )
	{
		loadImages();

#ifdef _DEBUG
		{
			int n;
			for( n = 0; n < 14; n++ ) {
				printf( "{%d,%d,%d},{%d,%d,%d},{%d,%d,%d},{%d,%d,%d},\n"
					, image_srcs[n].high_x[0]
					, image_srcs[n].high_x[1]
					, image_srcs[n].high_x[2]
					, image_srcs[n].high_y[0]
					, image_srcs[n].high_y[1]
					, image_srcs[n].high_y[2]
					, image_srcs[n].low_x[0]
					, image_srcs[n].low_x[1]
					, image_srcs[n].low_x[2]
					, image_srcs[n].low_y[0]
					, image_srcs[n].low_y[1]
					, image_srcs[n].low_y[2]
				);
			}
		}
#endif
		/*
		l.render = OpenDisplaySizedAt( 0, l.half_size + X_HALF_UNITS * 4, l.half_size + Y_HALF_UNITS * 4, 0, 0 );
		{
			//Image  surface = MakeSubImage( GetDisplayImage( l.render ), + X_HALF_UNITS*2, Y_HALF_UNITS*2, l.half_size - X_HALF_UNITS)
			SetRedrawHandler( l.render, MyDraw, 0 );
		}
		UpdateDisplay( l.render );
		while( 1 )
			WakeableSleep( 10000 );
		return 0;
		*/
	}
}
EndSaneWinMain()