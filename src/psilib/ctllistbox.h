PSI_NAMESPACE

struct listitem_tag
{
	TEXTCHAR *text;
	PSI_CONTROL within_list;
	uintptr_t data;
	// top == -1 if not show, or not shown yet
	// else top == pixel offset of the top of the item
	int32_t top; // top of the item in the listbox...
				// makes for quick rendering of custom items
				// also can use this to push current down
				// when inserting sorted items...
	uint32_t height; // height of the line...
	int nLevel; // level of the tree item...
	Image icon;
	struct {
		uint32_t bSelected :1;
		uint32_t bFocused  :1;
		uint32_t bOpen : 1; // if open, show any items after this +1 level...
	} flags;
	PMENU pPopup;
	void (CPROC*MenuProc)(uintptr_t,struct listitem_tag*,uint32_t);
	uintptr_t psvContextMenu;
	PLISTITEM next, prior;
};
typedef struct listitem_tag LISTITEM;

PSI_NAMESPACE_END
