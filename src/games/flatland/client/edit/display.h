#ifndef DISPLAY_DEFINED
#define DISPLAY_DEFINED

#include <render.h>
#include <image.h>
#include <controls.h>

#include <world.h>


typedef struct display_tag {
	struct {
		unsigned int bShiftMode : 1;
        unsigned int bDragging           : 1; // something dragging...
        unsigned int bLocked             : 1; // locked on something.

            // only one of these may be true
            // and these are only valid if bDragging or bLocked is set.
        unsigned int bDisplay           : 1; // (dragging only) move display
        unsigned int bSectorOrigin      : 1; // origin of sector
        unsigned int bOrigin                : 1; // orogin of wall
        unsigned int bSlope             : 1; // using slope of line to update
        unsigned int bEndStart      : 1; // end of wall (start)
        unsigned int bEndEnd          : 1; // end of wall (end)

        unsigned int bSelect                    : 1;
      unsigned int bMarkingMerge        : 1;

        unsigned int bSectorList            : 1;
        unsigned int bWallList              : 1;

        unsigned int bShowSectorText        : 1;
        unsigned int bShowLines             : 1;
        unsigned int bShowSectorTexture : 1;
        unsigned int bUseGrid               : 1;
        unsigned int bShowGrid              : 1;
        unsigned int bGridTop               : 1; 
        unsigned int bDrawNearSector     : 1; // draw a range of sectors from current.
		unsigned int bNormalMode  : 1; // edit normals, not wall position...
    } flags;

    int delxaccum, delyaccum;
    int x, y, b; // last state of mouse

    _POINT origin; // offset of origin to display...
    RCOORD scale; // 1:1 = default 
                  // 1:100 - small changes make big worlds 
                      // 1:0.001 - big changes make small worlds
    int xbias, ybias, zbias;  // bias to make origin be centered on display
                                      // zbias is more of a zoom factor...

    INDEX  pWorld;    // what world we're showing now...

    int     nSectors;  // number of selected sectors
    struct {
        INDEX Sector;
        INDEX *SectorList; // may be &CurSector.Sector ...
    } CurSector;

    int nWalls;
    struct {
        INDEX   Wall;   // current selected wall...
        INDEX *WallList;
    } CurWall;

	struct {
		INDEX Wall; // when this wall gets created, store it in curwall
	} NextWall; 

    _POINT SectorOrigin;
    int CurSecOrigin[2];
    _POINT WallOrigin;
    int CurOrigin[2];
    int CurEnds[2][2];
    int CurSlope[2];
	 ORTHOAREA SelectRect;

    INDEX MarkedWall;
    _POINT lastpoint; // save last point - though mouse moved, point might not

    int GridXUnits;
    int GridYUnits;
    int GridXSets;
    int GridYSets;
    CDATA GridColor;
    CDATA Background;

    PRENDERER hVideo;
    Image  pImage;
} DISPLAY, *PDISPLAY;

#define ONE_SCALE   8
#define DISPLAY_SCALE(pdisplay, c)    ( ( ( (c) / (pdisplay)->scale ) * ONE_SCALE ) / (pdisplay)->zbias )
#define DISPLAY_X(pdisplay, x)     ( DISPLAY_SCALE( (pdisplay), (x) + (pdisplay)->origin[0] ) + (pdisplay)->xbias )
#define DISPLAY_Y(pdisplay, y)     ( DISPLAY_SCALE( (pdisplay), (pdisplay)->origin[1] - (y) ) + (pdisplay)->ybias )

#define REAL_SCALE(pdisplay, c) (RCOORD)( ( (RCOORD)(c) * (RCOORD)(pdisplay)->zbias * (pdisplay)->scale ) / (RCOORD)ONE_SCALE )
#define REAL_X(pdisplay,x)        ( REAL_SCALE( (pdisplay), (x) - (pdisplay)->xbias ) - (pdisplay)->origin[0] )
#define REAL_Y(pdisplay,y)        ( REAL_SCALE( (pdisplay), (pdisplay)->ybias - (y) ) + (pdisplay)->origin[1] )


#endif
