
S=../../..


SRCS= 
SRCS+=$S/src/psilib/controls.c
SRCS+=$S/src/psilib/borders.c
SRCS+=$S/src/psilib/progress_bar.c
SRCS+=$S/src/psilib/caption_buttons.c
SRCS+=$S/src/psilib/control_physical.c
SRCS+=$S/src/psilib/ctlbutton.c

SRCS+=$S/src/psilib/ctlcombo.c
SRCS+=$S/src/psilib/ctledit.c
SRCS+=$S/src/psilib/ctlimage.c
SRCS+=$S/src/psilib/ctllistbox.c
SRCS+=$S/src/psilib/ctlmisc.c
SRCS+=$S/src/psilib/ctlprop.c
SRCS+=$S/src/psilib/ctlscroll.c
SRCS+=$S/src/psilib/ctlsheet.c
SRCS+=$S/src/psilib/ctlslider.c
SRCS+=$S/src/psilib/ctltext.c
SRCS+=$S/src/psilib/ctltooltip.c
SRCS+=$S/src/psilib/fileopen.c
SRCS+=$S/src/psilib/fntdlg.c
#SRCS+=$S/src/psilib/loadsave.c
SRCS+=$S/src/psilib/mouse.c
SRCS+=$S/src/psilib/palette.c
SRCS+=$S/src/psilib/popups.c
SRCS+=$S/src/psilib/xml_load.c
SRCS+=$S/src/psilib/xml_save.c
SRCS+=$S/src/psilib/option_frame.c
SRCS+=$S/src/psilib/scrollknob.c

# this is statically linked with imglib which has fontcache and sack::image::fontcache
#:SRCS+=$S/src/psilib/fntcache.c

SRCS+=$S/src/psilib/console/console_block_writer.c
SRCS+=$S/src/psilib/console/console_keydefs.c
SRCS+=$S/src/psilib/console/history.c
SRCS+=$S/src/psilib/console/paste.c
SRCS+=$S/src/psilib/console/psicon.c
SRCS+=$S/src/psilib/console/regaccess.c
SRCS+=$S/src/psilib/console/WinLogic.c
SRCS+=$S/src/psilib/console/psicon_interface.c

SRCS+=$S/src/psilib/calctl/calender.c
SRCS+=$S/src/psilib/calctl/clock.c
SRCS+=$S/src/psilib/calctl/analog.c
 
SRCS+=$S/src/systraylib/systray.c

SRCS+=$S/src/SQLlib/optlib/editoption/editopt.c

rm sack_ucb_psi.h
rm sack_ucb_psi.c

set OPTS=
set OPTS=%OPTS% -I%S%/src/contrib/freetype-2.8/include

ppc -c -K -once -ssio -sd -I%S%/include %OPTS% -p -osack_ucb_psi.c -DINCLUDE_LOGGING %SRCS%

mkdir h
cp config.ppc.h h\config.ppc
cd h

HDRS=
HDRS+=$S/../include/controls.h
HDRS+=$S/../include/psi/namespaces.h
HDRS+=$S/../include/psi.h
HDRS+=$S/../include/psi/buttons.h
HDRS+=$S/../include/psi/clock.h
HDRS+=$S/../include/psi/console.h
HDRS+=$S/../include/psi/edit.h
HDRS+=$S/../include/psi/knob.h
HDRS+=$S/../include/psi/shadewell.h
HDRS+=$S/../include/psi/slider.h
HDRS+=$S/../include/systray.h


ppc -c -K -once -ssio -sd -I%S%/../include -p -o../sack_ucb_psi.h %HDRS%
cd ..



