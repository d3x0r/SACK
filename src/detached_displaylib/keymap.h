

#include <sack_types.h>
#if (defined( __LINUX__ ) || defined( GCC )) && !defined( __arm__ )
#include <SDL/SDL.h>
#endif
#include <keybrd.h>

#ifndef _WIN32
#if 1 && !defined( MAKE_KEYMAP )
struct {unsigned char scancode; char *keyname; char *othername; } keymap[] = { { 0,"0","SDLK_UNKNOWN" } 	//[SDLK_UNKNOWN]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { KEY_BACKSPACE,"KEY_BACKSPACE","SDLK_BACKSPACE" } 	//[SDLK_BACKSPACE]=KEY_BACKSPACE   (8=22)
	, { KEY_TAB,"KEY_TAB","SDLK_TAB" } 	//[SDLK_TAB]=KEY_TAB   (9=23)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","SDLK_CLEAR" } 	//[SDLK_CLEAR]=0   (12=0)
	, { KEY_ENTER,"KEY_ENTER","SDLK_RETURN" } 	//[SDLK_RETURN]=KEY_ENTER   (13=237)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","SDLK_PAUSE" } 	//[SDLK_PAUSE]=0   (19=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { KEY_ESCAPE,"KEY_ESCAPE","SDLK_ESCAPE" } 	//[SDLK_ESCAPE]=KEY_ESCAPE   (27=9)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { KEY_SPACE,"KEY_SPACE","SDLK_SPACE" } 	//[SDLK_SPACE]=KEY_SPACE   (32=65)
	, { KEY_1,"KEY_1","SDLK_EXCLAIM" } 	//[SDLK_EXCLAIM]=KEY_1   (33=10)
	, { KEY_QUOTE,"KEY_QUOTE","SDLK_QUOTEDBL" } 	//[SDLK_QUOTEDBL]=KEY_QUOTE   (34=48)
	, { KEY_3,"KEY_3","SDLK_HASH" } 	//[SDLK_HASH]=KEY_3   (35=12)
	, { KEY_4,"KEY_4","SDLK_DOLLAR" } 	//[SDLK_DOLLAR]=KEY_4   (36=13)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { KEY_5,"KEY_5","SDLK_AMPERSAND" } 	//[SDLK_AMPERSAND]=KEY_5   (38=14)
	, { KEY_QUOTE,"KEY_QUOTE","SDLK_QUOTE" } 	//[SDLK_QUOTE]=KEY_QUOTE   (39=48)
	, { KEY_9,"KEY_9","SDLK_LEFTPAREN" } 	//[SDLK_LEFTPAREN]=KEY_9   (40=18)
	, { KEY_0,"KEY_0","SDLK_RIGHTPAREN" } 	//[SDLK_RIGHTPAREN]=KEY_0   (41=19)
	, { KEY_8,"KEY_8","SDLK_ASTERISK" } 	//[SDLK_ASTERISK]=KEY_8   (42=17)
	, { KEY_EQUAL,"KEY_EQUAL","SDLK_PLUS" } 	//[SDLK_PLUS]=KEY_EQUAL   (43=21)
	, { KEY_COMMA,"KEY_COMMA","SDLK_COMMA" } 	//[SDLK_COMMA]=KEY_COMMA   (44=59)
	, { KEY_DASH,"KEY_DASH","SDLK_MINUS" } 	//[SDLK_MINUS]=KEY_DASH   (45=20)
	, { KEY_PERIOD,"KEY_PERIOD","SDLK_PERIOD" } 	//[SDLK_PERIOD]=KEY_PERIOD   (46=60)
	, { KEY_SLASH,"KEY_SLASH","SDLK_SLASH" } 	//[SDLK_SLASH]=KEY_SLASH   (47=61)
	, { KEY_0,"KEY_0","SDLK_0" } 	//[SDLK_0]=KEY_0   (48=19)
	, { KEY_1,"KEY_1","SDLK_1" } 	//[SDLK_1]=KEY_1   (49=10)
	, { KEY_2,"KEY_2","SDLK_2" } 	//[SDLK_2]=KEY_2   (50=11)
	, { KEY_3,"KEY_3","SDLK_3" } 	//[SDLK_3]=KEY_3   (51=12)
	, { KEY_4,"KEY_4","SDLK_4" } 	//[SDLK_4]=KEY_4   (52=13)
	, { KEY_5,"KEY_5","SDLK_5" } 	//[SDLK_5]=KEY_5   (53=14)
	, { KEY_6,"KEY_6","SDLK_6" } 	//[SDLK_6]=KEY_6   (54=15)
	, { KEY_7,"KEY_7","SDLK_7" } 	//[SDLK_7]=KEY_7   (55=16)
	, { KEY_8,"KEY_8","SDLK_8" } 	//[SDLK_8]=KEY_8   (56=17)
	, { KEY_9,"KEY_9","SDLK_9" } 	//[SDLK_9]=KEY_9   (57=18)
	, { KEY_SEMICOLON,"KEY_SEMICOLON","SDLK_COLON" } 	//[SDLK_COLON]=KEY_SEMICOLON   (58=47)
	, { KEY_SEMICOLON,"KEY_SEMICOLON","SDLK_SEMICOLON" } 	//[SDLK_SEMICOLON]=KEY_SEMICOLON   (59=47)
	, { KEY_COMMA,"KEY_COMMA","SDLK_LESS" } 	//[SDLK_LESS]=KEY_COMMA   (60=59)
	, { KEY_EQUAL,"KEY_EQUAL","SDLK_EQUALS" } 	//[SDLK_EQUALS]=KEY_EQUAL   (61=21)
	, { KEY_PERIOD,"KEY_PERIOD","SDLK_GREATER" } 	//[SDLK_GREATER]=KEY_PERIOD   (62=60)
	, { KEY_SLASH,"KEY_SLASH","SDLK_QUESTION" } 	//[SDLK_QUESTION]=KEY_SLASH   (63=61)
	, { KEY_2,"KEY_2","SDLK_AT" } 	//[SDLK_AT]=KEY_2   (64=11)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { KEY_LEFT_BRACKET,"KEY_LEFT_BRACKET","SDLK_LEFTBRACKET" } 	//[SDLK_LEFTBRACKET]=KEY_LEFT_BRACKET   (91=34)
	, { KEY_BACKSLASH,"KEY_BACKSLASH","SDLK_BACKSLASH" } 	//[SDLK_BACKSLASH]=KEY_BACKSLASH   (92=51)
	, { KEY_RIGHT_BRACKET,"KEY_RIGHT_BRACKET","SDLK_RIGHTBRACKET" } 	//[SDLK_RIGHTBRACKET]=KEY_RIGHT_BRACKET   (93=35)
	, { KEY_6,"KEY_6","SDLK_CARET" } 	//[SDLK_CARET]=KEY_6   (94=15)
	, { KEY_MINUS,"KEY_MINUS","SDLK_UNDERSCORE" } 	//[SDLK_UNDERSCORE]=KEY_MINUS   (95=240)
	, { KEY_ACCENT,"KEY_ACCENT","SDLK_BACKQUOTE" } 	//[SDLK_BACKQUOTE]=KEY_ACCENT   (96=49)
	, { KEY_A,"KEY_A","SDLK_a" } 	//[SDLK_a]=KEY_A   (97=38)
	, { KEY_B,"KEY_B","SDLK_b" } 	//[SDLK_b]=KEY_B   (98=56)
	, { KEY_C,"KEY_C","SDLK_c" } 	//[SDLK_c]=KEY_C   (99=54)
	, { KEY_D,"KEY_D","SDLK_d" } 	//[SDLK_d]=KEY_D   (100=40)
	, { KEY_E,"KEY_E","SDLK_e" } 	//[SDLK_e]=KEY_E   (101=26)
	, { KEY_F,"KEY_F","SDLK_f" } 	//[SDLK_f]=KEY_F   (102=41)
	, { KEY_G,"KEY_G","SDLK_g" } 	//[SDLK_g]=KEY_G   (103=42)
	, { KEY_H,"KEY_H","SDLK_h" } 	//[SDLK_h]=KEY_H   (104=43)
	, { KEY_I,"KEY_I","SDLK_i" } 	//[SDLK_i]=KEY_I   (105=31)
	, { KEY_J,"KEY_J","SDLK_j" } 	//[SDLK_j]=KEY_J   (106=44)
	, { KEY_K,"KEY_K","SDLK_k" } 	//[SDLK_k]=KEY_K   (107=45)
	, { KEY_L,"KEY_L","SDLK_l" } 	//[SDLK_l]=KEY_L   (108=46)
	, { KEY_M,"KEY_M","SDLK_m" } 	//[SDLK_m]=KEY_M   (109=58)
	, { KEY_N,"KEY_N","SDLK_n" } 	//[SDLK_n]=KEY_N   (110=57)
	, { KEY_O,"KEY_O","SDLK_o" } 	//[SDLK_o]=KEY_O   (111=32)
	, { KEY_P,"KEY_P","SDLK_p" } 	//[SDLK_p]=KEY_P   (112=33)
	, { KEY_Q,"KEY_Q","SDLK_q" } 	//[SDLK_q]=KEY_Q   (113=24)
	, { KEY_R,"KEY_R","SDLK_r" } 	//[SDLK_r]=KEY_R   (114=27)
	, { KEY_S,"KEY_S","SDLK_s" } 	//[SDLK_s]=KEY_S   (115=39)
	, { KEY_T,"KEY_T","SDLK_t" } 	//[SDLK_t]=KEY_T   (116=28)
	, { KEY_U,"KEY_U","SDLK_u" } 	//[SDLK_u]=KEY_U   (117=30)
	, { KEY_V,"KEY_V","SDLK_v" } 	//[SDLK_v]=KEY_V   (118=55)
	, { KEY_W,"KEY_W","SDLK_w" } 	//[SDLK_w]=KEY_W   (119=25)
	, { KEY_X,"KEY_X","SDLK_x" } 	//[SDLK_x]=KEY_X   (120=53)
	, { KEY_Y,"KEY_Y","SDLK_y" } 	//[SDLK_y]=KEY_Y   (121=29)
	, { KEY_Z,"KEY_Z","SDLK_z" } 	//[SDLK_z]=KEY_Z   (122=52)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { KEY_DELETE,"KEY_DELETE","SDLK_DELETE" } 	//[SDLK_DELETE]=KEY_DELETE   (127=242)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { KEY_PAD_0,"KEY_PAD_0","SDLK_KP0" } 	//[SDLK_KP0]=KEY_PAD_0   (256=90)
	, { KEY_PAD_1,"KEY_PAD_1","SDLK_KP1" } 	//[SDLK_KP1]=KEY_PAD_1   (257=87)
	, { KEY_PAD_2,"KEY_PAD_2","SDLK_KP2" } 	//[SDLK_KP2]=KEY_PAD_2   (258=88)
	, { KEY_PAD_3,"KEY_PAD_3","SDLK_KP3" } 	//[SDLK_KP3]=KEY_PAD_3   (259=89)
	, { KEY_PAD_4,"KEY_PAD_4","SDLK_KP4" } 	//[SDLK_KP4]=KEY_PAD_4   (260=83)
	, { KEY_PAD_5,"KEY_PAD_5","SDLK_KP5" } 	//[SDLK_KP5]=KEY_PAD_5   (261=84)
	, { KEY_PAD_6,"KEY_PAD_6","SDLK_KP6" } 	//[SDLK_KP6]=KEY_PAD_6   (262=85)
	, { KEY_PAD_7,"KEY_PAD_7","SDLK_KP7" } 	//[SDLK_KP7]=KEY_PAD_7   (263=79)
	, { KEY_PAD_8,"KEY_PAD_8","SDLK_KP8" } 	//[SDLK_KP8]=KEY_PAD_8   (264=80)
	, { KEY_PAD_9,"KEY_PAD_9","SDLK_KP9" } 	//[SDLK_KP9]=KEY_PAD_9   (265=81)
	, { KEY_PAD_DOT,"KEY_PAD_DOT","SDLK_KP_PERIOD" } 	//[SDLK_KP_PERIOD]=KEY_PAD_DOT   (266=91)
	, { KEY_PAD_DIV,"KEY_PAD_DIV","SDLK_KP_DIVIDE" } 	//[SDLK_KP_DIVIDE]=KEY_PAD_DIV   (267=112)
	, { KEY_PAD_MULT,"KEY_PAD_MULT","SDLK_KP_MULTIPLY" } 	//[SDLK_KP_MULTIPLY]=KEY_PAD_MULT   (268=63)
	, { KEY_PAD_MINUS,"KEY_PAD_MINUS","SDLK_KP_MINUS" } 	//[SDLK_KP_MINUS]=KEY_PAD_MINUS   (269=82)
	, { KEY_PAD_PLUS,"KEY_PAD_PLUS","SDLK_KP_PLUS" } 	//[SDLK_KP_PLUS]=KEY_PAD_PLUS   (270=86)
	, { KEY_PAD_ENTER,"KEY_PAD_ENTER","SDLK_KP_ENTER" } 	//[SDLK_KP_ENTER]=KEY_PAD_ENTER   (271=108)
	, { KEY_PAD_ENTER,"KEY_PAD_ENTER","SDLK_KP_EQUALS" } 	//[SDLK_KP_EQUALS]=KEY_PAD_ENTER   (272=108)
	, { KEY_UP,"KEY_UP","SDLK_UP" } 	//[SDLK_UP]=KEY_UP   (273=251)
	, { KEY_DOWN,"KEY_DOWN","SDLK_DOWN" } 	//[SDLK_DOWN]=KEY_DOWN   (274=245)
	, { KEY_RIGHT,"KEY_RIGHT","SDLK_RIGHT" } 	//[SDLK_RIGHT]=KEY_RIGHT   (275=247)
	, { KEY_LEFT,"KEY_LEFT","SDLK_LEFT" } 	//[SDLK_LEFT]=KEY_LEFT   (276=249)
	, { KEY_INSERT,"KEY_INSERT","SDLK_INSERT" } 	//[SDLK_INSERT]=KEY_INSERT   (277=243)
	, { KEY_HOME,"KEY_HOME","SDLK_HOME" } 	//[SDLK_HOME]=KEY_HOME   (278=252)
	, { KEY_END,"KEY_END","SDLK_END" } 	//[SDLK_END]=KEY_END   (279=246)
	, { KEY_PGUP,"KEY_PGUP","SDLK_PAGEUP" } 	//[SDLK_PAGEUP]=KEY_PGUP   (280=250)
	, { KEY_PGDN,"KEY_PGDN","SDLK_PAGEDOWN" } 	//[SDLK_PAGEDOWN]=KEY_PGDN   (281=244)
	, { KEY_F1,"KEY_F1","SDLK_F1" } 	//[SDLK_F1]=KEY_F1   (282=67)
	, { KEY_F2,"KEY_F2","SDLK_F2" } 	//[SDLK_F2]=KEY_F2   (283=68)
	, { KEY_F3,"KEY_F3","SDLK_F3" } 	//[SDLK_F3]=KEY_F3   (284=69)
	, { KEY_F4,"KEY_F4","SDLK_F4" } 	//[SDLK_F4]=KEY_F4   (285=70)
	, { KEY_F5,"KEY_F5","SDLK_F5" } 	//[SDLK_F5]=KEY_F5   (286=71)
	, { KEY_F6,"KEY_F6","SDLK_F6" } 	//[SDLK_F6]=KEY_F6   (287=72)
	, { KEY_F7,"KEY_F7","SDLK_F7" } 	//[SDLK_F7]=KEY_F7   (288=73)
	, { KEY_F8,"KEY_F8","SDLK_F8" } 	//[SDLK_F8]=KEY_F8   (289=74)
	, { KEY_F9,"KEY_F9","SDLK_F9" } 	//[SDLK_F9]=KEY_F9   (290=75)
	, { KEY_F10,"KEY_F10","SDLK_F10" } 	//[SDLK_F10]=KEY_F10   (291=76)
	, { KEY_F11,"KEY_F11","SDLK_F11" } 	//[SDLK_F11]=KEY_F11   (292=95)
	, { KEY_F12,"KEY_F12","SDLK_F12" } 	//[SDLK_F12]=KEY_F12   (293=96)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { 0,"0","0" } 	//[0]=0   (0=0)
	, { KEY_NUM_LOCK,"KEY_NUM_LOCK","SDLK_NUMLOCK" } 	//[SDLK_NUMLOCK]=KEY_NUM_LOCK   (300=77)
	, { KEY_CAPS_LOCK,"KEY_CAPS_LOCK","SDLK_CAPSLOCK" } 	//[SDLK_CAPSLOCK]=KEY_CAPS_LOCK   (301=66)
	, { KEY_SCROLL_LOCK,"KEY_SCROLL_LOCK","SDLK_SCROLLOCK" } 	//[SDLK_SCROLLOCK]=KEY_SCROLL_LOCK   (302=78)
	, { KEY_RIGHT_SHIFT,"KEY_RIGHT_SHIFT","SDLK_RSHIFT" } 	//[SDLK_RSHIFT]=KEY_RIGHT_SHIFT   (303=62)
	, { KEY_LEFT_SHIFT,"KEY_LEFT_SHIFT","SDLK_LSHIFT" } 	//[SDLK_LSHIFT]=KEY_LEFT_SHIFT   (304=50)
	, { KEY_RIGHT_CONTROL,"KEY_RIGHT_CONTROL","SDLK_RCTRL" } 	//[SDLK_RCTRL]=KEY_RIGHT_CONTROL   (305=109)
	, { KEY_LEFT_CONTROL,"KEY_LEFT_CONTROL","SDLK_LCTRL" } 	//[SDLK_LCTRL]=KEY_LEFT_CONTROL   (306=37)
	, { KEY_RIGHT_ALT,"KEY_RIGHT_ALT","SDLK_RALT" } 	//[SDLK_RALT]=KEY_RIGHT_ALT   (307=113)
	, { KEY_LEFT_ALT,"KEY_LEFT_ALT","SDLK_LALT" } 	//[SDLK_LALT]=KEY_LEFT_ALT   (308=64)
	, { 0,"0","SDLK_RMETA" } 	//[SDLK_RMETA]=0   (309=0)
	, { 0,"0","SDLK_LMETA" } 	//[SDLK_LMETA]=0   (310=0)
	, { KEY_WINDOW_1,"KEY_WINDOW_1","SDLK_LSUPER" } 	//[SDLK_LSUPER]=KEY_WINDOW_1   (311=115)
	, { KEY_WINDOW_2,"KEY_WINDOW_2","SDLK_RSUPER" } 	//[SDLK_RSUPER]=KEY_WINDOW_2   (312=117)
	, { 0,"0","SDLK_MODE" } 	//[SDLK_MODE]=0   (313=0)
	, { 0,"0","SDLK_COMPOSE" } 	//[SDLK_COMPOSE]=0   (314=0)
};
#if 0
unsigned char keymap[] =
{
	0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, KEY_BACKSPACE
, KEY_TAB
, 0
, 0
, 0 // clear
, KEY_ENTER
, 0
, 0
, 0
, 0
, 0
, 0 // pause
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, SDLK_ESCAPE
, 0
, 0
, 0
, 0
, KEY_SPACE
, KEY_1
, KEY_QUOTE
, KEY_3
, KEY_4
, 0
, KEY_5
, KEY_QUOTE
, KEY_9
, KEY_0
, KEY_8
, KEY_EQUAL
, KEY_COMMA
, KEY_DASH
, KEY_PERIOD
, KEY_SLASH
, KEY_0
, KEY_1
, KEY_2
, KEY_3
, KEY_4
, KEY_5
, KEY_6
, KEY_7
, KEY_8
, KEY_9
, KEY_SEMICOLON
, KEY_SEMICOLON
, KEY_COMMA
, KEY_EQUAL
, KEY_PERIOD
, KEY_SLASH
, KEY_2
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, 0
, KEY_LEFT_BRACKET
, KEY_BACKSLASH
, KEY_RIGHT_BRACKET
, KEY_6
, KEY_MINUS
, KEY_ACCENT
, KEY_A
, KEY_B
, KEY_C
, KEY_D
, KEY_E
, KEY_F
, KEY_G
, KEY_H
, KEY_I
, KEY_J
, KEY_K
, KEY_L
, KEY_M
, KEY_N
, KEY_O
, KEY_P
, KEY_Q
, KEY_R
, KEY_S
, KEY_T
, KEY_U
, KEY_V
, KEY_W
, KEY_X
, KEY_Y
, KEY_Z
, 0
, 0
, 0
, 0
, KEY_DELETE
/* Note skipping entries 128 to 255*/
/* Numeric Keypad */
, KEY_PAD_0  // entry 256
, KEY_PAD_1
, KEY_PAD_2
, KEY_PAD_3
, KEY_PAD_4
, KEY_PAD_5
, KEY_PAD_6
, KEY_PAD_7
, KEY_PAD_8
, KEY_PAD_9
, KEY_PAD_DOT
, KEY_PAD_DIV
, KEY_PAD_MULT
, KEY_PAD_MINUS
, KEY_PAD_PLUS
, KEY_PAD_ENTER
, KEY_PAD_ENTER

/* Arrows + Home/End pad */

, KEY_UP
, KEY_DOWN
, KEY_RIGHT
, KEY_LEFT
, KEY_INSERT
, KEY_HOME
, KEY_END
, KEY_PGUP
, KEY_PGDN

/* Function keys */
, KEY_F1
, KEY_F2
, KEY_F3
, KEY_F4
, KEY_F5
, KEY_F6
, KEY_F7
, KEY_F8
, KEY_F9
, KEY_F10
, KEY_F11
, KEY_F12
, 0
, 0
, 0

/* Key state modifier keys */
, 0
, 0
, 0
, KEY_NUM_LOCK
, KEY_CAPS_LOCK
, KEY_SCROLL_LOCK
, KEY_RIGHT_SHIFT
, KEY_LEFT_SHIFT
, KEY_RIGHT_CONTROL
, KEY_LEFT_CONTROL
, KEY_RIGHT_ALT
, KEY_LEFT_ALT
, 0
, 0
, KEY_WINDOW_1
, KEY_WINDOW_2
, 0
, 0
};
#endif
#else
#ifdef MAKE_KEYMAP
#define MAPTO( sdl_keyname, my_keyname ) [sdl_keyname]={ #sdl_keyname, sdl_keyname, #my_keyname, my_keyname }
struct key_info { char *sdl_name; int sdl_number; char*my_name; int my_number; } keymap[] =
#else
#define MAPTO( sdl_keyname, my_keyname ) [sdl_keyname] = my_keyname
unsigned char keymap[] =
#endif

{
/* The keyboard syms have been cleverly chosen to map to ASCII */
	MAPTO(SDLK_UNKNOWN		, 0 ),
	MAPTO(SDLK_BACKSPACE		, KEY_BACKSPACE ),
	MAPTO(SDLK_TAB		, KEY_TAB ),
	MAPTO(SDLK_CLEAR		, 0 ),
	MAPTO(SDLK_RETURN		, KEY_ENTER ),
	MAPTO(SDLK_PAUSE		, 0 ),
	MAPTO(SDLK_ESCAPE		, KEY_ESCAPE ),
	MAPTO(SDLK_SPACE		, KEY_SPACE ),
	MAPTO(SDLK_EXCLAIM		, KEY_1 ),
	MAPTO(SDLK_QUOTEDBL		, KEY_QUOTE ),
	MAPTO(SDLK_HASH		, KEY_3 ),
	MAPTO(SDLK_DOLLAR		, KEY_4 ),
	MAPTO(SDLK_AMPERSAND		, KEY_5 ),
	MAPTO(SDLK_QUOTE		, KEY_QUOTE ),
	MAPTO(SDLK_LEFTPAREN		, KEY_9 ),
	MAPTO(SDLK_RIGHTPAREN		, KEY_0 ),
	MAPTO(SDLK_ASTERISK		, KEY_8 ),
	MAPTO(SDLK_PLUS		, KEY_EQUAL ),
	MAPTO(SDLK_COMMA		, KEY_COMMA ),
	MAPTO(SDLK_MINUS		, KEY_DASH ),
	MAPTO(SDLK_PERIOD		, KEY_PERIOD ),
	MAPTO(SDLK_SLASH		, KEY_SLASH ),
	MAPTO(SDLK_0			, KEY_0 ),
	MAPTO(SDLK_1			, KEY_1 ),
	MAPTO(SDLK_2			, KEY_2 ),
	MAPTO(SDLK_3			, KEY_3 ),
	MAPTO(SDLK_4			, KEY_4 ),
	MAPTO(SDLK_5			, KEY_5 ),
	MAPTO(SDLK_6			, KEY_6 ),
	MAPTO(SDLK_7			, KEY_7 ),
	MAPTO(SDLK_8			, KEY_8 ),
	MAPTO(SDLK_9			, KEY_9 ),
	MAPTO(SDLK_COLON		, KEY_SEMICOLON ),
	MAPTO(SDLK_SEMICOLON		, KEY_SEMICOLON ),
	MAPTO(SDLK_LESS		, KEY_COMMA ),
	MAPTO(SDLK_EQUALS		, KEY_EQUAL ),
	MAPTO(SDLK_GREATER		, KEY_PERIOD ),
	MAPTO(SDLK_QUESTION		, KEY_SLASH ),
	MAPTO(SDLK_AT			, KEY_2 ),
	/* 
	   Skip uppercase letters
	 */
	MAPTO(SDLK_LEFTBRACKET	, KEY_LEFT_BRACKET ),
	MAPTO(SDLK_BACKSLASH		, KEY_BACKSLASH ),
	MAPTO(SDLK_RIGHTBRACKET	, KEY_RIGHT_BRACKET ),
	MAPTO(SDLK_CARET		, KEY_6 ),
	MAPTO(SDLK_UNDERSCORE		, KEY_MINUS ),
	MAPTO(SDLK_BACKQUOTE		, KEY_ACCENT ),
	MAPTO(SDLK_a			, KEY_A ),
	MAPTO(SDLK_b			, KEY_B ),
	MAPTO(SDLK_c			, KEY_C ),
	MAPTO(SDLK_d			, KEY_D ),
	MAPTO(SDLK_e			, KEY_E ),
	MAPTO(SDLK_f			, KEY_F ),
	MAPTO(SDLK_g			, KEY_G ),
	MAPTO(SDLK_h			, KEY_H ),
	MAPTO(SDLK_i			, KEY_I ),
	MAPTO(SDLK_j			, KEY_J ),
	MAPTO(SDLK_k			, KEY_K ),
	MAPTO(SDLK_l			, KEY_L ),
	MAPTO(SDLK_m			, KEY_M ),
	MAPTO(SDLK_n			, KEY_N ),
	MAPTO(SDLK_o			, KEY_O ),
	MAPTO(SDLK_p			, KEY_P ),
	MAPTO(SDLK_q			, KEY_Q ),
	MAPTO(SDLK_r			, KEY_R ),
	MAPTO(SDLK_s			, KEY_S ),
	MAPTO(SDLK_t			, KEY_T ),
	MAPTO(SDLK_u			, KEY_U ),
	MAPTO(SDLK_v			, KEY_V ),
	MAPTO(SDLK_w			, KEY_W ),
	MAPTO(SDLK_x			, KEY_X ),
	MAPTO(SDLK_y			, KEY_Y ),
	MAPTO(SDLK_z			, KEY_Z ),
	MAPTO(SDLK_DELETE		, KEY_DELETE ),
	/* End of ASCII mapped keysyms */

	/* Numeric keypad */
	MAPTO(SDLK_KP0		, KEY_PAD_0 ),
	MAPTO(SDLK_KP1		, KEY_PAD_1 ),
	MAPTO(SDLK_KP2		, KEY_PAD_2 ),
	MAPTO(SDLK_KP3		, KEY_PAD_3 ),
	MAPTO(SDLK_KP4		, KEY_PAD_4 ),
	MAPTO(SDLK_KP5		, KEY_PAD_5 ),
	MAPTO(SDLK_KP6		, KEY_PAD_6 ),
	MAPTO(SDLK_KP7		, KEY_PAD_7 ),
	MAPTO(SDLK_KP8		, KEY_PAD_8 ),
	MAPTO(SDLK_KP9		, KEY_PAD_9 ),
	MAPTO(SDLK_KP_PERIOD		, KEY_PAD_DOT ),
	MAPTO(SDLK_KP_DIVIDE		, KEY_PAD_DIV ),
	MAPTO(SDLK_KP_MULTIPLY	, KEY_PAD_MULT ),
	MAPTO(SDLK_KP_MINUS		, KEY_PAD_MINUS ),
	MAPTO(SDLK_KP_PLUS		, KEY_PAD_PLUS ),
	MAPTO(SDLK_KP_ENTER		, KEY_PAD_ENTER ),
	MAPTO(SDLK_KP_EQUALS		, KEY_PAD_ENTER ),

	/* Arrows + Home/End pad */
	MAPTO(SDLK_UP			, KEY_UP ),
	MAPTO(SDLK_DOWN		, KEY_DOWN ),
	MAPTO(SDLK_RIGHT		, KEY_RIGHT ),
	MAPTO(SDLK_LEFT		, KEY_LEFT ),
	MAPTO(SDLK_INSERT		, KEY_INSERT ),
	MAPTO(SDLK_HOME		, KEY_HOME ),
	MAPTO(SDLK_END		, KEY_END ),
	MAPTO(SDLK_PAGEUP		, KEY_PGUP ),
	MAPTO(SDLK_PAGEDOWN		, KEY_PGDN ),

	/* Function keys */
	MAPTO(SDLK_F1			, KEY_F1 ),
	MAPTO(SDLK_F2			,  KEY_F2 ),
	MAPTO(SDLK_F3			,  KEY_F3 ),
	MAPTO(SDLK_F4			,  KEY_F4 ),
	MAPTO(SDLK_F5			,  KEY_F5 ),
	MAPTO(SDLK_F6			,  KEY_F6 ),
	MAPTO(SDLK_F7			,  KEY_F7 ),
	MAPTO(SDLK_F8			,  KEY_F8 ),
	MAPTO(SDLK_F9			,  KEY_F9 ),
	MAPTO(SDLK_F10		,  KEY_F10 ),
	MAPTO(SDLK_F11		,  KEY_F11 ),
	MAPTO(SDLK_F12		,  KEY_F12 ),
//	MAPTO(SDLK_F13		, 294 ),
//	MAPTO(SDLK_F14		, 295 ),
//	MAPTO(SDLK_F15		, 296 ),

	/* Key state modifier keys */
	MAPTO(SDLK_NUMLOCK		, KEY_NUM_LOCK ),
	MAPTO(SDLK_CAPSLOCK		, KEY_CAPS_LOCK ),
	MAPTO(SDLK_SCROLLOCK		, KEY_SCROLL_LOCK ),
	MAPTO(SDLK_RSHIFT		, KEY_RIGHT_SHIFT ),
	MAPTO(SDLK_LSHIFT		, KEY_LEFT_SHIFT ),
	MAPTO(SDLK_RCTRL		, KEY_RIGHT_CONTROL ),
	MAPTO(SDLK_LCTRL		, KEY_LEFT_CONTROL ),
	MAPTO(SDLK_RALT		, KEY_RIGHT_ALT ),
	MAPTO(SDLK_LALT		, KEY_LEFT_ALT ),
	MAPTO(SDLK_RMETA		, 0 ),
	MAPTO(SDLK_LMETA		, 0 ),
	MAPTO(SDLK_LSUPER		, KEY_WINDOW_1 ),		/* Left "Windows" key */
	MAPTO(SDLK_RSUPER		, KEY_WINDOW_2 ),		/* Right "Windows" key */
	MAPTO(SDLK_MODE		, 0 ),		/* "Alt Gr" key */
	MAPTO(SDLK_COMPOSE		, 0 ),		/* Multi-key compose key */

	/* Miscellaneous function keys */

  // MAPTO(SDLK_HELP		, 315 ),
 //  MAPTO(SDLK_PRINT		, 316 ),
 //  MAPTO(SDLK_SYSREQ		, 317 ),
 //  MAPTO(SDLK_BREAK		, 318 ),
 //  MAPTO(SDLK_MENU		, 319 ),
 //  MAPTO(SDLK_POWER		, 320 ),		/* Power Macintosh power key */
 //  MAPTO(SDLK_EURO		, 321 ),		/* Some european keyboards */
//	MAPTO(SDLK_UNDO		, 322 ),		/* Atari keyboard has Undo */

	/* Add any other keys here */

//	MAPTO(SDLK_LAST

};
#endif
#else
unsigned char keymap[256];
#endif


// $Log: keymap.h,v $
// Revision 1.4  2003/03/29 22:52:00  panther
// New render/image layering ability.  Added support to Display for WIN32 usage (native not SDL)
//
// Revision 1.3  2003/03/26 01:12:52  panther
// Fix mapping of quote key
//
// Revision 1.2  2003/03/25 08:45:50  panther
// Added CVS logging tag
//
