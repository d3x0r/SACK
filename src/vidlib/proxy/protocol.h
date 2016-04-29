
#ifdef _MSC_VER
#pragma pack (push, 1)
#endif
#ifndef UNALIGNED
#define UNALIGNED
#endif

#define MSGBLOCK(type,...) struct commsg_##type { __VA_ARGS__ } type
PREFIX_PACKED struct opendisplay_data 
{
	S_32 x, y;
	_32 w, h;
	_32 attr;
	PTRSZVAL server_display_id;
	PTRSZVAL over;
	PTRSZVAL under;
} PACKED;

// also usedf for Flush_display command
// only info needed is the server_display_ID
PREFIX_PACKED struct close_display_data 
{
	PTRSZVAL server_display_id;
} PACKED;

PREFIX_PACKED struct transfer_sub_image_data
{
	PTRSZVAL image_to_id;
	PTRSZVAL image_from_id;
} PACKED;

PREFIX_PACKED struct move_size_display_data
{
	PTRSZVAL server_display_id;
	S_32 x, y;
	_32 w, h;
} PACKED;

PREFIX_PACKED struct make_image_data 
{
	// what the server calls this image; for all further draw ops
	PTRSZVAL server_image_id;
	_32 w, h;
	// so the client can know which to output surface attach to
	PTRSZVAL server_display_id;
} PACKED;

PREFIX_PACKED struct image_data_data 
{
	// what the server calls this image; for all further draw ops
	PTRSZVAL server_image_id;
	_8 data[1];
} PACKED;

PREFIX_PACKED struct draw_block_data 
{
	// what the server calls this image; for all further draw ops
	PTRSZVAL server_image_id;
	PTRSZVAL length;
	_8 data[1];
} PACKED;

PREFIX_PACKED struct put_string_data 
{
	// what the server calls this image; for all further draw ops
	PTRSZVAL server_image_id;
	S_32 x, y;
	int orientation; // vertical/invert
	int justification;
	S_32 width;
	PTRSZVAL server_font_id;
	_32 foreground_color;
	_32 background_color;
	TEXTCHAR string[1];
} PACKED;

PREFIX_PACKED struct unmake_image_data
{
	// what the server calls this image; for all further draw ops
	PTRSZVAL server_image_id;
} PACKED;

PREFIX_PACKED struct client_identification_data
{
	// what the server calls this image; for all further draw ops
	CTEXTSTR client_id;
} PACKED;

PREFIX_PACKED struct make_subimage_data 
{
	PTRSZVAL server_image_id;
	S_32 x, y;
	_32 w, h;
	PTRSZVAL server_parent_image_id;
} PACKED;

PREFIX_PACKED struct __tmp
{
	_32 data;
} PACKED;

PREFIX_PACKED struct blatcolor_data
{
	PTRSZVAL server_image_id;
	S_32 x, y;
	_32 w, h;
	CDATA color;
} PACKED;

PREFIX_PACKED struct blot_image_data
{
	PTRSZVAL server_image_id;
	S_32 x, y;
	_32 w, h;
	S_32 xs, ys;
	PTRSZVAL image_id;
} PACKED;

PREFIX_PACKED struct blot_scaled_image_data
{
	PTRSZVAL server_image_id;
	S_32 x, y;
	_32 w, h;
	S_32 xs, ys;
	_32 ws, hs;
	PTRSZVAL image_id;
} PACKED;

PREFIX_PACKED struct line_data
{
	PTRSZVAL server_image_id;
	S_32 x1, y1;
	S_32 x2, y2;
	CDATA color;
} PACKED;


PREFIX_PACKED struct mouse_event_data
{
	PTRSZVAL server_render_id;
	S_32 x, y;
	_32 b;
} PACKED;

PREFIX_PACKED struct flush_event_data
{
	PTRSZVAL server_render_id;
} PACKED;

PREFIX_PACKED struct move_image_data
{
	PTRSZVAL server_render_id;
	S_32 x, y;
} PACKED;

PREFIX_PACKED struct size_image_data
{
	PTRSZVAL server_render_id;
	_32 w, h;
} PACKED;

PREFIX_PACKED struct key_event_data
{
	PTRSZVAL server_render_id;
	_32 key;
	_32 pressed;
} PACKED;

PREFIX_PACKED struct font_character_data
{
	_32 character;
	S_32 x, y;
	_32 w, h;
	S_32 ascent;
} PACKED;

PREFIX_PACKED struct font_color_image_data
{
	PTRSZVAL server_image_id;
	_32 color;
} PACKED;

PREFIX_PACKED struct font_data_data
{
	PTRSZVAL server_font_id;
	S_32 baseline;
	_32 height;
	S_8 bias_x;
	S_8 bias_y;
	/* these translate to JSON easily... but bad for a network structure. */
	PLIST characters; // list of font_character_data
	PLIST colors;   // list of font_color_image_data
	PTRSZVAL image_id;
} PACKED;



PREFIX_PACKED struct common_message {
	_8 message_id;
	union
	{
		TEXTCHAR UNALIGNED text[1];  // actually is more than one
		MSGBLOCK( version,
					_8 bits;
					 _8 unicode;
					 _8 number; );
		struct opendisplay_data opendisplay_data;
		struct blatcolor_data blatcolor;
		struct make_image_data make_image;
		struct make_subimage_data make_subimage;
		struct image_data_data image_data;
		struct blot_image_data blot_image;
		struct blot_scaled_image_data blot_scaled_image;
		struct line_data line;
		struct mouse_event_data mouse_event;
		struct flush_event_data flush_event;
		struct key_event_data key_event;
		struct unmake_image_data unmake_image;
		struct close_display_data close_display;
		struct move_size_display_data  move_size_display;
		struct client_identification_data client_ident;
		struct move_image_data move_image;
		struct size_image_data size_image;
		struct transfer_sub_image_data transfer_sub_image;
		struct draw_block_data draw_block;
		struct put_string_data put_string;
		struct font_data_data font_data;
		MSGBLOCK( open_display_reply,  PTRSZVAL server_display_id; PTRSZVAL client_display_id; );
	} data;
} PACKED;

PREFIX_PACKED struct event_msg
{
	PCLIENT pc;
	_32 sendlen;
	struct common_message msg;
} PACKED;



#ifdef _MSC_VER
#pragma pack (pop)
#endif

enum proxy_message_id{
	     PMID_Version   // 0
							, PMID_SetApplicationTitle   //1
							, PMID_SetApplicationIcon  // 2

							, PMID_OpenDisplayAboveUnderSizedAt  // 3
							, PMID_CloseDisplay  // 4
							, PMID_Reply_OpenDisplayAboveUnderSizedAt  // 5

							, PMID_MakeImage // 6
							, PMID_MakeSubImage // 7
							
							, PMID_BlatColor // 8
							, PMID_BlatColorAlpha // 9 
							, PMID_ImageData // 10 - transfer local image data to client
							, PMID_BlotImageSizedTo  // 11 
							, PMID_BlotScaledImageSizedTo // 12
							, PMID_DrawLine // 13

							, PMID_UnmakeImage // 14
							, PMID_Event_Mouse // 15 (from client to server)
							, PMID_Event_Key // 16 (from client to server)
							, PMID_Flush_Draw // 17 swap buffers... update display... commit changes....
							, PMID_MoveSizeDisplay // 18 change display position/size
							, PMID_Event_Redraw // 19 Client side has lost the screen, and needs a draw
							, PMID_Move_Image // 20 
							, PMID_Size_Image // 21 
							, PMID_TransferSubImages // 22 just allow the client to do the full moves instead of peices and parts.
							, PMID_ImageDataFrag // 23 - transfer local image data to client
							, PMID_ImageDataFragMore // 24 - transfer local image data to client
							, PMID_DrawBlock   // 25 - this is a compressed block of an array of draw commands
							, PMID_Event_Flush_Finished // 26 - response from server when flush is handled (flush mouse)
							, PMID_FontData // 27 - font data
							, PMID_PutString // 28 - put out a string; with given font data
							, PMID_

							, PMID_LAST_PROXY_MESSAGE

							//};
							//enum proxy_message_id_3d{
							, PMID_GetRenderTransform = PMID_LAST_PROXY_MESSAGE
							, PMID_ClipPoints
							, PMID_GetViewVolume
							

							, PMID_ClientSessionId = 100 // multi-instance serve, get unique client ID
							,
};
