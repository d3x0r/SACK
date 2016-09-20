#ifndef BANNER_WIDGET_DEFINED
#define BANNER_WIDGET_DEFINED

#include <controls.h>
#include <render.h>

#ifdef __cplusplus
#define BANNER_NAMESPACE SACK_NAMESPACE namespace widgets { namespace banner {
#define BANNER_NAMESPACE_END } } SACK_NAMESPACE_END
SACK_NAMESPACE
	namespace widgets {
		namespace banner {
#else
#define BANNER_NAMESPACE
#define BANNER_NAMESPACE_END
#endif

#ifdef BANNER_SOURCE
#define BANNER_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define BANNER_PROC(type,name) IMPORT_METHOD type CPROC name
#endif



typedef struct banner_tag *PBANNER;

#define BANNER_CLICK    0x0001
#define BANNER_TIMEOUT  0x0002
#define BANNER_CLOSED   0x0004
#define BANNER_WAITING  0x0008
#define BANNER_OKAY     0x0010
#define BANNER_DEAD     0x0020 // banner does not click, banner does not timeout...
#define BANNER_TOP      0x0040 // banner does not click, banner does not timeout...
#define BANNER_NOWAIT   0x0080 // don't wait in banner create...
#define BANNER_EXPLORER  0x0100 // don't wait in banner create...
#define BANNER_ABSOLUTE_TOP      (0x0200|BANNER_TOP) // banner does not click, banner does not timeout...
#define BANNER_EXTENDED_RESULT     0x0400  //OKAY/CANCEL on YesNo dialog
#define BANNER_ALPHA    0x0800
#define BANNER_ABORT    0x1000
#define BANNER_KEEP     0x2000

#define BANNER_OPTION_YESNO          0x010000
#define BANNER_OPTION_OKAYCANCEL     0x020000

#define BANNER_OPTION_LEFT_JUSTIFY   0x100000

	// banner will have a keyboard to enter a value...
   // banner text will show above the keyboard?

#define BANNER_OPTION_KEYBOARD       0x01000000

BANNER_PROC( int, CreateBanner2Extended )( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text, int options, int timeout, CDATA textcolor, CDATA basecolor, int lines, int cols, int display );
BANNER_PROC( int, CreateBanner2Exx )( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text, int options, int timeout, CDATA textcolor, CDATA basecolor );
BANNER_PROC( int, CreateBanner2Ex )( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text, int options, int timeout );
BANNER_PROC( int, CreateBanner2 )( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text );
BANNER_PROC( void, SetBanner2Options )( PBANNER, uint32_t flags, uint32_t extra );
BANNER_PROC( void, SetBanner2OptionsEx )( PBANNER*, uint32_t flags, uint32_t extra );
#define SetBanner2Options( banner,flags,extra ) SetBanner2OptionsEx( &(banner), flags,extra )

// click, closed, etc...
// if it's in wait, it will not be destroyed until after
// wait results...
// if wait is not active, .. then the resulting kill banner
// will clear the ppBanner originally passed... so that becomes
// the wait state variable...
// results false if no banner to wait for.
BANNER_PROC( void, SetBanner2Text )( PBANNER banner, TEXTCHAR *text );
BANNER_PROC( int, WaitForBanner2 )( PBANNER banner );
BANNER_PROC( int, WaitForBanner2Ex )( PBANNER *banner );
#define WaitForBanner2(b) WaitForBanner2Ex( &(b) )
BANNER_PROC( void, RemoveBanner2Ex )( PBANNER *banner DBG_PASS );
BANNER_PROC( void, RemoveBanner2 )( PBANNER banner );
#define RemoveBanner2(b) RemoveBanner2Ex( &(b) DBG_SRC )
BANNER_PROC( SFTFont, GetBanner2Font )( void );
BANNER_PROC( uint32_t, GetBanner2FontHeight )( void );

BANNER_PROC( PRENDERER, GetBanner2Renderer )( PBANNER banner );
BANNER_PROC( PSI_CONTROL, GetBanner2Control )( PBANNER banner );

typedef void (CPROC *DoConfirmProc)( void );
BANNER_PROC( int, Banner2ThreadConfirm )( CTEXTSTR type, CTEXTSTR msg, DoConfirmProc dokey );
BANNER_PROC( int, Banner2ThreadConfirmEx )( CTEXTSTR type, CTEXTSTR msg, DoConfirmProc dokey, DoConfirmProc doNoKey );
BANNER_PROC( void, Banner2AnswerYes )( CTEXTSTR type );
BANNER_PROC( void, Banner2AnswerNo )( CTEXTSTR type );


#define Banner2NoWait( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_NOWAIT|BANNER_DEAD, 0 )
#define Banner2NoWaitAlpha( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_NOWAIT|BANNER_DEAD|BANNER_ALPHA, 0 )
#define Banner2TopNoWait( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_NOWAIT|BANNER_DEAD|BANNER_TOP, 0 )
#define Banner2Top( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_DEAD|BANNER_TOP|BANNER_TIMEOUT, 5000 )
#define Banner2TopClickable( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_CLICK|BANNER_TOP|BANNER_TIMEOUT, 5000 )
#define Banner2TopClickContinue( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_NOWAIT|BANNER_CLICK|BANNER_TOP, 0 )
#define Banner2YesNo( renderparent, text ) CreateBanner2Ex( renderparent, NULL, text, BANNER_OPTION_YESNO, 0 )
#define Banner2TopYesNo( renderparent, text ) CreateBanner2Ex( renderparent, NULL, text, BANNER_TOP|BANNER_OPTION_YESNO, 0 )
#define Banner2YesNoEx( renderparent, pb, text ) CreateBanner2Ex( renderparent, pb, text, BANNER_OPTION_YESNO, 0 )
#define Banner2Message( text ) CreateBanner2( NULL, NULL, text )
#define Banner2MessageEx( renderparent,text ) CreateBanner2( renderparent, NULL, text )
#define RemoveBanner2Message() RemoveBanner2Ex( NULL DBG_SRC )

#ifndef NO_SUPPORT_VERSION_1
#define BannerThreadConfirm(type,msg,dokey)  Banner2ThreadConfirm(type,msg,dokey)
#define BannerThreadConfirmEx(type,msg,dokey,nokey) Banner2ThreadConfirmEx(type,msg,dokey,nokey)
#define BannerNoWait( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_NOWAIT|BANNER_DEAD, 0 )
#define BannerNoWaitAlpha( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_NOWAIT|BANNER_DEAD|BANNER_ALPHA, 0 )
#define BannerTopNoWait( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_NOWAIT|BANNER_DEAD|BANNER_TOP, 0 )
#define BannerTopNoWaitEx( b, text ) CreateBanner2Ex( NULL, &b, text, BANNER_NOWAIT|BANNER_DEAD|BANNER_TOP, 0 )
#define BannerTop( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_DEAD|BANNER_TOP|BANNER_TIMEOUT, 5000 )
#define BannerYesNo( renderparent, text ) CreateBanner2Ex( renderparent, NULL, text, BANNER_OPTION_YESNO, 0 )
#define BannerTopYesNo( renderparent, text ) CreateBanner2Ex( renderparent, NULL, text, BANNER_TOP|BANNER_OPTION_YESNO, 0 )
#define BannerYesNoEx( renderparent, pb, text ) CreateBanner2Ex( renderparent, pb, text, BANNER_OPTION_YESNO, 0 )
#define BannerMessage( text ) CreateBanner2( NULL, NULL, text )
#define BannerMessageEx( renderparent,text ) CreateBanner2( renderparent, NULL, text )
#define RemoveBannerMessage() RemoveBanner2Ex( NULL DBG_SRC )
#define RemoveBannerEx( a )  RemoveBanner2Ex( a )
#define RemoveBanner(b) RemoveBanner2( b )
#define CreateBannerEx(p,b,text,o,time) CreateBanner2Ex(p,b,text,o,time)
#define BannerTopClickable( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_CLICK|BANNER_TOP|BANNER_TIMEOUT, 5000 )
#define BannerTopClickContinue( text ) CreateBanner2Ex( NULL, NULL, text, BANNER_NOWAIT|BANNER_CLICK|BANNER_TOP, 0 )
#define WaitForBannerEx(b)             WaitForBanner2Ex( b )

#endif



BANNER_PROC( int, CreateBanner3Extended )( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text, int options, int timeout, CDATA textcolor, CDATA basecolor, int lines, int cols, int display );
BANNER_PROC( int, CreateBanner3Exx )( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text, int options, int timeout, CDATA textcolor, CDATA basecolor );
BANNER_PROC( int, CreateBanner3Ex )( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text, int options, int timeout );
BANNER_PROC( int, CreateBanner3 )( PRENDERER parent, PBANNER *ppBanner, CTEXTSTR text );
BANNER_PROC( void, SetBanner3Options )( PBANNER, uint32_t flags, uint32_t extra );
BANNER_PROC( void, SetBanner3OptionsEx )( PBANNER*, uint32_t flags, uint32_t extra );
#define SetBanner3Options( banner,flags,extra ) SetBanner3OptionsEx( &(banner), flags,extra )

// click, closed, etc...
// if it's in wait, it will not be destroyed until after
// wait results...
// if wait is not active, .. then the resulting kill banner
// will clear the ppBanner originally passed... so that becomes
// the wait state variable...
// results false if no banner to wait for.
BANNER_PROC( void, SetBanner3Text )( PBANNER banner, TEXTCHAR *text );
BANNER_PROC( int, WaitForBanner3 )( PBANNER banner );
BANNER_PROC( int, WaitForBanner3Ex )( PBANNER *banner );
#define WaitForBanner3(b) WaitForBanner3Ex( &(b) )
BANNER_PROC( void, RemoveBanner3Ex )( PBANNER *banner DBG_PASS );
BANNER_PROC( void, RemoveBanner3 )( PBANNER banner );
#define RemoveBanner3(b) RemoveBanner3Ex( &(b) DBG_SRC )
BANNER_PROC( SFTFont, GetBanner3Font )( void );
BANNER_PROC( uint32_t, GetBanner3FontHeight )( void );

BANNER_PROC( PRENDERER, GetBanner3Renderer )( PBANNER banner );
BANNER_PROC( PSI_CONTROL, GetBanner3Control )( PBANNER banner );

BANNER_PROC( int, Banner3ThreadConfirm )( CTEXTSTR type, CTEXTSTR msg, DoConfirmProc dokey );
BANNER_PROC( void, Banner3AnswerYes )( CTEXTSTR type );
BANNER_PROC( void, Banner3AnswerNo )( CTEXTSTR type );


#define Banner3NoWait( text ) CreateBanner3Ex( NULL, NULL, text, BANNER_NOWAIT|BANNER_DEAD, 0 )
#define Banner3TopNoWait( text ) CreateBanner3Ex( NULL, NULL, text, BANNER_NOWAIT|BANNER_DEAD|BANNER_TOP, 0 )
#define Banner3Top( text ) CreateBanner3Ex( NULL, NULL, text, BANNER_DEAD|BANNER_TOP|BANNER_TIMEOUT, 5000 )
#define Banner3YesNo( renderparent, text ) CreateBanner3Ex( renderparent, NULL, text, BANNER_OPTION_YESNO, 0 )
#define Banner3TopYesNo( renderparent, text ) CreateBanner3Ex( renderparent, NULL, text, BANNER_TOP|BANNER_OPTION_YESNO, 0 )
#define Banner3YesNoEx( renderparent, pb, text ) CreateBanner3Ex( renderparent, pb, text, BANNER_OPTION_YESNO, 0 )
#define Banner3Message( text ) CreateBanner3( NULL, NULL, text )
#define Banner3MessageEx( renderparent,text ) CreateBanner3( renderparent, NULL, text )
#define RemoveBanner3Message() RemoveBanner3( NULL )

BANNER_NAMESPACE_END
#ifdef __cplusplus
	using namespace sack::widgets::banner;
#endif
#endif
