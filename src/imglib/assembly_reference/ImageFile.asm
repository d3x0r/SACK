

;typedef struct ImageFile_tag
;{
;  int realx, real_y;
;  int real_width;   // desired height and width may not be actual cause of 
;  int real_height;  // resizing of parent image....
;	int x; // need this for sub images - otherwise is irrelavent
;	int y;
;  int width;  /// Width of image.
;  int height; /// Height of image.
;  int pwidth; // width of real physical layer
;  
;  PCOLOR image;   /// The image data.
;  PCOLOR real_image;
;
;  struct {
;   	int bFree:1;  
;   	int bHidden:1; // parent resized under my x/y
;  }flags;
;  struct ImageFile_tag *pParent, *pChild, *pElder, *pYounger;
;} ImageFile;

%define ALPHA_TRANSPARENT 256

%ifndef __LINUX__
%define INVERT_IMAGE ; if windows frames are used... 0 is first pixel last line
%endif

%define IMGFILE_realx      0 
%define IMGFILE_realy      (IMGFILE_realx + 4)
%define IMGFILE_realw      (IMGFILE_realy + 4)
%define IMGFILE_realh      (IMGFILE_realw + 4)

%define IMGFILE_x 	   (IMGFILE_realh + 4 )
%define IMGFILE_y 	   (IMGFILE_x + 4)
%define IMGFILE_w 	   (IMGFILE_y + 4)
%define IMGFILE_h	   (IMGFILE_w + 4)

%define IMGFILE_pwidth     (IMGFILE_h + 4)

%define IMGFILE_image      (IMGFILE_pwidth + 4)

%define IMGFILE_flags      (IMGFILE_image + 4)
%define IMGFILE_FLAG_free   01h
%define IMGFILE_FLAG_hidden 02h
%define IMGFILE_parent     (IMGFILE_flags + 4 )
%define IMGFILE_child      (IMGFILE_parent + 4 )
%define IMGFILE_elder      (IMGFILE_child + 4)
%define IMGFILE_younger    (IMGFILE_elder + 4)


%define IMGFILE_effx 	   (IMGFILE_younger + 4 )
%define IMGFILE_effy 	   (IMGFILE_effx + 4)
%define IMGFILE_eff_maxx 	   (IMGFILE_effy + 4)
%define IMGFILE_eff_maxy	   (IMGFILE_eff_maxx + 4)

