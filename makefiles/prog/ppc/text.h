#ifndef TEXT_SUPPORT
#define TEXT_SUPPORT

#include <string.h>

#include "types.h"

#define DEFAULT_COLOR 0xF7
#define PRIOR_COLOR 0xF6 // this does not change the color....
typedef struct format_info_tag
{
	int spaces; // for restoration of spaces in strings?
	int tabs;
} FORMAT, *PFORMAT;


#define TF_STATIC    0x00000001   // declared in program data.... do NOT release
#define TF_INDIRECT  0x00000002   // data field extually points at PTEXT
#define TF_DEEP      0x00000004   // on release release indrect also...
#define TF_NOEXPAND  0x00010000  // don't expand - define substitution handling...

// these values used originally for ODBC query construction....
// these values are flags stored on the indirect following a value
// label...



// flag combinatoin which represents actual data is present even with 0 size
#define IS_DATA_FLAGS (0)

#define DECLTEXTSZ( name, size ) struct { \
   uint32_t flags; \
   struct text_segment_tag *Next, *Prior; \
   FORMAT format; \
   DECLDATA(data, size); \
} name


typedef struct text_segment_tag
{
	uint32_t flags;  // then here I could overlap with pEnt .bshadow, bmacro, btext ?
	struct text_segment_tag *Next, *Prior;
	FORMAT format; // valid if TF_FORMAT is set...
	DATA data; // must be last since var character data is included
} TEXT, *PTEXT;

#define DEFTEXT(str) {TF_STATIC,NULL,NULL,{0},{sizeof(str)-1,str}}
#define DECLTEXT(name, str) static DECLTEXTSZ( name, sizeof(str) ) = DEFTEXT(str)
extern TEXT newline;

//#define SETPRIORLINE(line,p) SetPriorLineEx( line, p DBG_SRC )
//#define SETNEXTLINE(line,p)  SetNextLineEx( line, p DBG_SRC )
#define SETPRIORLINE(line,p) ((line)?(((line)->Prior) = (PTEXT)(p)):0)
#define SETNEXTLINE(line,p)  ((line)?(((line)->Next ) = (PTEXT)(p)):0)
#define NEXTLINE(line)   ((PTEXT)(((PTEXT)line)?(((PTEXT)line)->Next):(NULL)))
#define PRIORLINE(line)  ((PTEXT)(((PTEXT)line)?(((PTEXT)line)->Prior):(NULL)))

#define SetStart(line)   do { PTEXT tmp;  for(; line && (tmp=PRIORLINE(line));line=tmp); } while(0)
#define SetEnd(line)    do { PTEXT tmp;  for(; line && (tmp=NEXTLINE(line)); line=tmp); } while (0)
// might also check to see if pseg is an indirect - setting this size would be BAD
#define SetTextSize(pseg, sz ) ((pseg)?((pseg)->data.size = (sz )):0)
PTEXT GetIndirect(PTEXT segment );

int GetTextFlags( PTEXT segment );
INDEX GetTextSize( PTEXT segment );

char *GetTextEx( PTEXT segment );
#define GetText(seg) ((seg)?((seg)->flags&TF_INDIRECT)?GetTextEx(seg):(char*)((PTEXT)(seg))->data.data:(char*)NULL)


#define SetIndirect(Seg,Where)  ( (Seg)->data.size = (int)(Where) )

#define SameText( l1, l2 )  ( strcmp( GetText(l1), GetText(l2) ) )
#define LikeText( l1, l2 )  ( strnicmp( GetText(l1), GetText(l2), min( GetTextSize(l1), \
                                                                        GetTextSize(l2) ) ) )
#define TextIs(text,string) ( !stricmp( GetText(text), string ) )
#define TextLike(text,string) ( !stricmp( GetText(text), string ) )

PTEXT SegCreateEx( size_t nSize DBG_PASS );
#define SegCreate(s) SegCreateEx(s DBG_SRC)
PTEXT SegCreateFromTextEx( char *text DBG_PASS );
#define SegCreateFromText(t) SegCreateFromTextEx(t DBG_SRC)
PTEXT SegCreateIndirectEx( PTEXT pText DBG_PASS );
#define SegCreateIndirect(t) SegCreateIndirectEx(t DBG_SRC)

PTEXT SegDuplicateEx( PTEXT pText DBG_PASS);
#define SegDuplicate(pt) SegDuplicateEx( pt DBG_SRC )
PTEXT LineDuplicateEx( PTEXT pText DBG_PASS );
#define LineDuplicate(pt) LineDuplicateEx(pt DBG_SRC )
PTEXT TextDuplicateEx( PTEXT pText DBG_PASS );
#define TextDuplicate(pt) TextDuplicateEx(pt DBG_SRC )

PTEXT SegCreateFromIntEx( int value DBG_PASS );
#define SegCreateFromInt(v) SegCreateFromIntEx( v DBG_SRC )
PTEXT SegCreateFromFloatEx( float value DBG_PASS );
#define SegCreateFromFloat(v) SegCreateFromFloatEx( v DBG_SRC )


PTEXT SegAppend   ( PTEXT source, PTEXT other );
PTEXT SegAdd      (PTEXT source,PTEXT other); // returns other instead of source
PTEXT SegInsert   ( PTEXT what, PTEXT before );

PTEXT SegExpandEx (PTEXT source, int nSize DBG_PASS );  // add last node... blank space.
#define SegExpand(s,n) SegExpandEx( s,n DBG_SRC );

void  LineReleaseEx (PTEXT *line DBG_PASS );
#define LineRelease(l) LineReleaseEx( &l DBG_SRC )

void SegReleaseEx( PTEXT seg DBG_PASS );
#define SegRelease(l) SegReleaseEx(l DBG_SRC )

PTEXT SegConcatEx   (PTEXT output,PTEXT input,int32_t offset,int32_t length DBG_PASS);
#define SegConcat(out,in,ofs,len) SegConcatEx(out,in,ofs,len DBG_SRC)

PTEXT SegUnlink   (PTEXT segment);
PTEXT SegBreak    (PTEXT segment);
PTEXT SegDeleteEx   (PTEXT *segment DBG_PASS); // removes seg from list, deletes seg.
#define SegDelete(s) SegDeleteEx(s DBG_SRC)
PTEXT SegGrab     (PTEXT segment); // removes seg from list, returns seg.
PTEXT SegSubst    ( PTEXT _this, PTEXT that );
PTEXT SegSubstRangeEx( PTEXT *_this, PTEXT end, PTEXT that DBG_PASS);
#define SegSubstRange(this,end,that) SegSubstRangeEx(this,end,that DBG_SRC)

PTEXT SegSplitEx( PTEXT *pLine, size_t nPos DBG_PASS);
#define SegSplit(line,pos) SegSplitEx( line, pos DBG_SRC )

size_t LineLength( PTEXT pt, int bSingle );
PTEXT BuildLineEx( PTEXT pt, int bSingle DBG_PASS );
#define BuildLine(from) BuildLineEx( from, FALSE DBG_SRC )

int CompareStrings( PTEXT pt1, int single1
                  , PTEXT pt2, int single2
                  , int bExact );

#define FORALLTEXT(start,var)  for(var=start;var; var=NEXTLINE(var))

//-----------------------------------------------------------------------

#define TYPELIB_PROC(t,n) t n

typedef struct vartext_tag {
	char *collect_text;
	INDEX collect_used;
	PTEXT collect;
	PTEXT commit;  
} VARTEXT, *PVARTEXT;

TYPELIB_PROC( void, VarTextInitEx)( PVARTEXT pvt DBG_PASS);
#define VarTextInit(pvt) VarTextInitEx( (pvt) DBG_SRC )
TYPELIB_PROC( void, VarTextEmptyEx)( PVARTEXT pvt DBG_PASS);
#define VarTextEmpty(pvt) VarTextEmptyEx( (pvt) DBG_SRC )
TYPELIB_PROC( void, VarTextAddCharacterEx)( PVARTEXT pvt, char c DBG_PASS );
#define VarTextAddCharacter(pvt,c) VarTextAddCharacterEx( (pvt),(c) DBG_SRC )
// returns true if any data was added...
TYPELIB_PROC( PTEXT, VarTextEndEx)( PVARTEXT pvt DBG_PASS ); // move any collected text to commit...
#define VarTextEnd(pvt) VarTextEndEx( (pvt) DBG_SRC )
TYPELIB_PROC( size_t, VarTextLength)( PVARTEXT pvt );
TYPELIB_PROC( PTEXT, VarTextGetEx)( PVARTEXT pvt DBG_PASS );
#define VarTextGet(pvt) VarTextGetEx( (pvt) DBG_SRC )
TYPELIB_PROC( void, VarTextExpandEx)( PVARTEXT pvt, int size DBG_PASS );
#define VarTextExpand(pvt, sz) VarTextExpandEx( (pvt), (sz) DBG_SRC )

//TYPELIB_PROC( int vtprintfEx( PVARTEXT pvt DBG_PASS, char *format, ... );
// note - don't include format - MUST have at least one parameter passed to ...
//#define vtprintf(pvt, ...) vtprintfEx( (pvt) DBG_SRC, __VA_ARGS__ )

TYPELIB_PROC( int, vtprintfEx)( PVARTEXT pvt, char *format, ... );
// note - don't include format - MUST have at least one parameter passed to ...
#define vtprintf vtprintfEx



#endif
