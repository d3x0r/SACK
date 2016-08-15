

struct menu_listbox_tag
{
	struct {
		BIT_FIELD bMultiSelect : 1;
	} flags;
	PSI_CONTROL list;

   uint32_t scrollbar_width; // might be nice to override this seperate from the font...

	SFTFont *font;
	CTEXTSTR font_name;

   // for edit to have a temp place to set value...
   SFTFont *new_font; 
   CTEXTSTR new_font_name;
};


