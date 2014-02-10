

typedef struct split_input_tag {
	PDATAPATH pSplitter; // track this so we can close it when we close
	struct split_input_tag *next, **me;
} SPLITTER, *PSPLITTER;

typedef struct mergedatapath_tag {
	DATAPATH common;
   void (*AddSplitter)( struct mergedatapath_tag *pMerger, PDATAPATH pSplitter );
   void (*RemoveSplitter)( struct mergedatapath_tag *pMerger, PDATAPATH pSplitter );
   PSPLITTER connections;
} MERGEDATAPATH, *PMERGEDATAPATH;

// $Log: mergedatapath.h,v $
// Revision 1.2  2003/03/25 08:59:02  panther
// Added CVS logging
//
