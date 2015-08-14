PSI_NAMESPACE

struct listitem_tag
{
	TEXTCHAR *text;
	PSI_CONTROL within_list;
	PTRSZVAL data;
	// top == -1 if not show, or not shown yet
	// else top == pixel offset of the top of the item
	S_32 top; // top of the item in the listbox...
				// makes for quick rendering of custom items
				// also can use this to push current down
				// when inserting sorted items...
	_32 height; // height of the line...
	int nLevel; // level of the tree item...
	Image icon;
	struct {
		_32 bSelected :1;
		_32 bFocused  :1;
		_32 bOpen : 1; // if open, show any items after this +1 level...
	} flags;
	PMENU pPopup;
	void (CPROC*MenuProc)(PTRSZVAL,struct listitem_tag*,_32);
	PTRSZVAL psvContextMenu;
	PLISTITEM next, prior;
};
typedef struct listitem_tag LISTITEM;

PSI_NAMESPACE_END
