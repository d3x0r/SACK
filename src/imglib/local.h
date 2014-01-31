#include <imglib/fontstruct.h>

// can also return a reload of the image?
// returns internal image data; used to create the data if it doesn't exist on reload texture....
// mostly just a event called by draw routines to non IF_FLAG_FINAL_RENDER images
void CPROC MarkImageUpdated( Image child_image );
Image AllocateCharacterSpaceByFont( Image target_image, SFTFont font, PCHARACTER character );
