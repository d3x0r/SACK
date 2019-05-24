@set CFLAGS=-I../../include
@set CFLAGS=%CFLAGS% -I../contrib/freetype-2.8/include

@set CFLAGS=%CFLAGS% -D__NO_OPTIONS__ -D__STATIC__

@set PSI_MORE_CONTROLS= ^
  console/history.c ^
  console/console_keydefs.c ^
  console/paste.c ^
  console/psicon.c ^
  console/psicon_interface.c ^
  console/regaccess.c ^
  console/WinLogic.c ^
  console/console_block_writer.c 



@set PSI_SOURCES=  ^
  borders.c ^
  calctl/calender.c ^
  calctl/clock.c ^
  calctl/analog.c ^
  controls.c ^
  progress_bar.c ^
  caption_buttons.c ^
  control_physical.c ^
  ctlbutton.c ^
  ctlcombo.c ^
  ctledit.c ^
  ctlimage.c ^
  ctllistbox.c ^
  ctlmisc.c ^
  ctlprop.c ^
  ctlscroll.c ^
  ctlsheet.c ^
  ctlslider.c ^
  ctltext.c ^
  ctltooltip.c ^
  fileopen.c ^
  fntdlg.c ^
  mouse.c ^
  palette.c ^
  popups.c ^
  xml_load.c ^
  xml_save.c ^
  option_frame.c ^
  scrollknob.c ^
  %PSI_MORE_CONTROLS%

@set SRCS=%PSI_SOURCES%

@set CFLAGS=%CFLAGS% -DTARGET_LABEL=imglib_puregl2 -D__3D__ -D_OPENGL_DRIVER -DMAKE_RCOORD_SINGLE 
@set CFLAGS=%CFLAGS% -DTARGETNAME=\"sack.wasm\"


call emcc -g -o ./psi.lo -D_DEBUG -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%
call emcc -O3 -o ./psio.lo  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %CFLAGS% %SRCS%

@echo on

