
#ifdef _MSC_VER
#pragma pack (push, 1)
#endif
#ifndef UNALIGNED
#define UNALIGNED
#endif

#define MSGBLOCK(type,...) struct commsg_##type { __VA_ARGS__ } type
PREFIX_PACKED struct opendisplay_data 
{
	int32_t x, y;
	uint32_t w, h;
	uint32_t attr;
	uintptr_t server_display_id;
	uintptr_t over;
	uintptr_t under;
} PACKED;

// also usedf for Flush_display command
// only info needed is the server_display_ID
PREFIX_PACKED struct close_display_data 
{
	uintptr_t server_display_id;
} PACKED;

PREFIX_PACKED struct transfer_sub_image_data
{
	uintptr_t image_to_id;
	uintptr_t image_from_id;
} PACKED;

PREFIX_PACKED struct move_size_display_data
{
	uintptr_t server_display_id;
	int32_t x, y;
	uint32_t w, h;
} PACKED;

PREFIX_PACKED struct make_image_data 
{
	// what the server calls this image; for all further draw ops
	uintptr_t server_image_id;
	uint32_t w, h;
	// so the client can know which to output surface attach to
	uintptr_t server_display_id;
} PACKED;

PREFIX_PACKED struct image_data_data 
{
	// what the server calls this image; for all further draw ops
	uintptr_t server_image_id;
	uint8_t data[1];
} PACKED;

PREFIX_PACKED struct draw_block_data 
{
	// what the server calls this image; for all further draw ops
	uintptr_t server_image_id;
	uintptr_t length;
	uint8_t data[1];
} PACKED;

PREFIX_PACKED struct put_string_data 
{
	// what the server calls this image; for all further draw ops
	uintptr_t server_image_id;
	int32_t x, y;
	int orientation; // vertical/invert
	int justification;
	int32_t width;
	uintptr_t server_font_id;
	uint32_t foreground_color;
	uint32_t background_color;
	TEXTCHAR string[1];
} PACKED;

PREFIX_PACKED struct unmake_image_data
{
	// what the server calls this image; for all further draw ops
	uintptr_t server_image_id;
} PACKED;

PREFIX_PACKED struct client_identification_data
{
	// what the server calls this image; for all further draw ops
	CTEXTSTR client_id;
} PACKED;

PREFIX_PACKED struct make_subimage_data 
{
	uintptr_t server_image_id;
	int32_t x, y;
	uint32_t w, h;
	uintptr_t server_parent_image_id;
} PACKED;

PREFIX_PACKED struct __tmp
{
	uint32_t data;
} PACKED;

PREFIX_PACKED struct blatcolor_data
{
	uintptr_t server_image_id;
	int32_t x, y;
	uint32_t w, h;
	CDATA color;
} PACKED;

PREFIX_PACKED struct blot_image_data
{
	uintptr_t server_image_id;
	int32_t x, y;
	uint32_t w, h;
	int32_t xs, ys;
	uintptr_t image_id;
} PACKED;

PREFIX_PACKED struct blot_scaled_image_data
{
	uintptr_t server_image_id;
	int32_t x, y;
	uint32_t w, h;
	int32_t xs, ys;
	uint32_t ws, hs;
	uintptr_t image_id;
} PACKED;

PREFIX_PACKED struct line_data
{
	uintptr_t server_image_id;
	int32_t x1, y1;
	int32_t x2, y2;
	CDATA color;
} PACKED;


PREFIX_PACKED struct mouse_event_data
{
	uintptr_t server_render_id;
	int32_t x, y;
	uint32_t b;
} PACKED;

PREFIX_PACKED struct flush_event_data
{
	uintptr_t server_render_id;
} PACKED;

PREFIX_PACKED struct move_image_data
{
	uintptr_t server_render_id;
	int32_t x, y;
} PACKED;

PREFIX_PACKED struct size_image_data
{
	uintptr_t server_render_id;
	uint32_t w, h;
} PACKED;

PREFIX_PACKED struct key_event_data
{
	uintptr_t server_render_id;
	uint32_t key;
	uint32_t pressed;
} PACKED;

PREFIX_PACKED struct font_character_data
{
	uint32_t character;
	int32_t x, y;
	uint32_t w, h;
	int32_t ascent;
} PACKED;

PREFIX_PACKED struct font_color_image_data
{
	uintptr_t server_image_id;
	uint32_t color;
} PACKED;

PREFIX_PACKED struct font_data_data
{
	uintptr_t server_font_id;
	int32_t baseline;
	uint32_t height;
	int8_t bias_x;
	int8_t bias_y;
	/* these translate to JSON easily... but bad for a network structure. */
	PLIST characters; // list of font_character_data
	PLIST colors;   // list of font_color_image_data
	uintptr_t image_id;
} PACKED;



PREFIX_PACKED struct common_message {
	uint8_t message_id;
	union
	{
		TEXTCHAR UNALIGNED text[1];  // actually is more than one
		MSGBLOCK( version,
					uint8_t bits;
					 uint8_t unicode;
					 uint8_t number; );
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
		MSGBLOCK( open_display_reply,  uintptr_t server_display_id; uintptr_t client_display_id; );
	} data;
} PACKED;

PREFIX_PACKED struct event_msg
{
	PCLIENT pc;
	uint32_t sendlen;
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
