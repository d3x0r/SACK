:::
:   
:   
:   

@set S=../../..
@set SRCS=


@set SRCS= 
@set SRCS= %SRCS%   %S%/src/psilib/controls.c
@set SRCS= %SRCS%   %S%/src/psilib/borders.c
@set SRCS= %SRCS%   %S%/src/psilib/progress_bar.c
@set SRCS= %SRCS%   %S%/src/psilib/caption_buttons.c
@set SRCS= %SRCS%   %S%/src/psilib/control_physical.c
@set SRCS= %SRCS%   %S%/src/psilib/ctlbutton.c

@set SRCS= %SRCS%   %S%/src/psilib/ctlcombo.c
@set SRCS= %SRCS%   %S%/src/psilib/ctledit.c
@set SRCS= %SRCS%   %S%/src/psilib/ctlimage.c
@set SRCS= %SRCS%   %S%/src/psilib/ctllistbox.c
@set SRCS= %SRCS%   %S%/src/psilib/ctlmisc.c
@set SRCS= %SRCS%   %S%/src/psilib/ctlprop.c
@set SRCS= %SRCS%   %S%/src/psilib/ctlscroll.c
@set SRCS= %SRCS%   %S%/src/psilib/ctlsheet.c
@set SRCS= %SRCS%   %S%/src/psilib/ctlslider.c
@set SRCS= %SRCS%   %S%/src/psilib/ctltext.c
@set SRCS= %SRCS%   %S%/src/psilib/ctltooltip.c
@set SRCS= %SRCS%   %S%/src/psilib/fileopen.c
@set SRCS= %SRCS%   %S%/src/psilib/fntdlg.c
:@set SRCS= %SRCS%   %S%/src/psilib/loadsave.c
@set SRCS= %SRCS%   %S%/src/psilib/mouse.c
@set SRCS= %SRCS%   %S%/src/psilib/palette.c
@set SRCS= %SRCS%   %S%/src/psilib/popups.c
@set SRCS= %SRCS%   %S%/src/psilib/xml_load.c
@set SRCS= %SRCS%   %S%/src/psilib/xml_save.c
@set SRCS= %SRCS%   %S%/src/psilib/option_frame.c
@set SRCS= %SRCS%   %S%/src/psilib/scrollknob.c

: this is statically linked with imglib which has fontcache and sack::image::fontcache
:@set SRCS= %SRCS%   %S%/src/psilib/fntcache.c

@set SRCS= %SRCS%   %S%/src/psilib/console/console_block_writer.c
@set SRCS= %SRCS%   %S%/src/psilib/console/console_keydefs.c
@set SRCS= %SRCS%   %S%/src/psilib/console/history.c
@set SRCS= %SRCS%   %S%/src/psilib/console/paste.c
@set SRCS= %SRCS%   %S%/src/psilib/console/psicon.c
@set SRCS= %SRCS%   %S%/src/psilib/console/regaccess.c
@set SRCS= %SRCS%   %S%/src/psilib/console/WinLogic.c
@set SRCS= %SRCS%   %S%/src/psilib/console/psicon_interface.c

@set SRCS= %SRCS%   %S%/src/psilib/calctl/calender.c
@set SRCS= %SRCS%   %S%/src/psilib/calctl/clock.c
@set SRCS= %SRCS%   %S%/src/psilib/calctl/analog.c
 
@set SRCS= %SRCS%   %S%/src/systraylib/systray.c

@set SRCS= %SRCS%   %S%/src/SQLlib/optlib/editoption/editopt.c

del sack_ucb_psi.h
del sack_ucb_psi.c

set OPTS=
set OPTS=%OPTS% -I%S%/src/contrib/freetype-2.8/include

c:\tools\ppc.exe -c -K -once -ssio -sd -I%S%/include %OPTS% -p -osack_ucb_psi.c -DINCLUDE_LOGGING %SRCS%

mkdir h
copy config.ppc.h h\config.ppc
cd h

@set HDRS=
@set HDRS= %HDRS% %S%/../include/controls.h
@set HDRS= %HDRS% %S%/../include/psi/namespaces.h
@set HDRS= %HDRS% %S%/../include/psi.h
@set HDRS= %HDRS% %S%/../include/psi/buttons.h
@set HDRS= %HDRS% %S%/../include/psi/clock.h
@set HDRS= %HDRS% %S%/../include/psi/console.h
@set HDRS= %HDRS% %S%/../include/psi/edit.h
@set HDRS= %HDRS% %S%/../include/psi/knob.h
@set HDRS= %HDRS% %S%/../include/psi/shadewell.h
@set HDRS= %HDRS% %S%/../include/psi/slider.h
@set HDRS= %HDRS% %S%/../include/systray.h


c:\tools\ppc.exe -c -K -once -ssio -sd -I%S%/../include -p -o../sack_ucb_psi.h %HDRS%
cd ..



