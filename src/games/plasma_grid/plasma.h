

struct plasma_state *PlasmaCreate( RCOORD seed[4], RCOORD roughness, int width, int height );
RCOORD *PlasmaGetSurface( struct plasma_state *plasma );
void PlasmaRender( struct plasma_state *plasma, RCOORD seed[4] );
void PlasmaSetRoughness( struct plasma_state *plasma, RCOORD roughness, RCOORD horiz_rough );

