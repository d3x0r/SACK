

struct plasma_patch *PlasmaCreate( RCOORD seed[4], RCOORD roughness, int width, int height );
struct plasma_patch *PlasmaExtend( struct plasma_patch *plasma, int in_direction, RCOORD seed[2], RCOORD roughness );

RCOORD *PlasmaGetSurface( struct plasma_patch *plasma );
RCOORD *PlasmaReadSurface( struct plasma_patch *plasma, int x_ofs, int y_ofs, int smoothing );

void PlasmaRender( struct plasma_patch *plasma, RCOORD seed[4] );
void PlasmaSetRoughness( struct plasma_patch *plasma, RCOORD roughness, RCOORD horiz_rough );

