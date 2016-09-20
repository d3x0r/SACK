

#define PLUGIN_MODULE
#include "plugin.h"

#include "plane.h"
 
typedef struct link_tag
{
   PTEXT pName;
   VECTOR origin;
   VECTOR forward;
   VECTOR right;
   VECTOR minrot;
   VECTOR maxrot;
} LINK, *PLINK;

typedef struct bone_tag
{
   PTEXT pName;
   VECTOR color;
   RCOORD mass;
   PLIST pPlanes;
   int nDefined; // number of planes valid for initial computation
   int nRefines; // number of planes valid for final computation

   PLIST pLinks;
} BONE, *PBONE;


typedef struct bone_instance_tag
{
   PTEXT pName;
   PBONE pOriginal;
   VECTOR scale;
   PTRANSFORM pt;
   // VECTOR color; // possibly override color with instantiation
} BONEINST, *PBONEINST;

typedef struct attachment_tag
{
   PBONEINST *pBoneFrom;
   PTEXT pLinkFrom;
   PBONEINST *pBoneTo;
   PTEXT pLinkTo;

} ATTACHMENT, *PATTACHMENT;

typedef struct form_tag
{
   PTEXT pName;
   PLIST pBones;  // PBONEINST array
   PLIST pAttachments; // PATTACHMENT array
} FORM, *PFORM;

typedef struct library_tag
{
   PLIST pForms;
   PLIST pBones;
} LIBRARY, *PLIBRARY;

typedef struct body_tag
{
   // actual instantiation of a form...
   uint8_t unused;
} BODY, *PBODY;
// $Log: bone.h,v $
// Revision 1.3  2003/03/25 08:59:03  panther
// Added CVS logging
//
