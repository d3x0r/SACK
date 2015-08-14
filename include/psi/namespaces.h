#ifndef __panthers_slick_interface_namespace__
#define __panthers_slick_interface_namespace__

#ifdef __cplusplus

#define PSI_NAMESPACE SACK_NAMESPACE namespace PSI {
#define _PSI_NAMESPACE namespace PSI {
#define _PSI_NAMESPACE_END }
#define PSI_NAMESPACE_END _PSI_NAMESPACE_END SACK_NAMESPACE_END
#define USE_PSI_NAMESPACE using namespace sack::PSI;
	
#   define _PSI_INTERFACE_NAMESPACE namespace Interface {
#   define _PSI_INTERFACE_NAMESPACE_END }

#   define _BUTTON_NAMESPACE namespace button {
#   define _BUTTON_NAMESPACE_END } 
#   define USE_BUTTON_NAMESPACE using namespace button; 
#   define USE_PSI_BUTTON_NAMESPACE using namespace sack::PSI::button; 

#   define _COLORWELL_NAMESPACE namespace colorwell {
#   define _COLORWELL_NAMESPACE_END } 
#   define USE_COLORWELL_NAMESPACE using namespace colorwell; 
#   define USE_PSI_COLORWELL_NAMESPACE using namespace sack::PSI::colorwell; 

#   define _MENU_NAMESPACE namespace popup {
#   define _MENU_NAMESPACE_END } 
#   define USE_MENU_NAMESPACE using namespace popup; 
#   define USE_PSI_MENU_NAMESPACE using namespace sack::PSI::popup; 

#   define _TEXT_NAMESPACE namespace text {
#   define _TEXT_NAMESPACE_END } 
#   define USE_TEXT_NAMESPACE using namespace text; 
#   define USE_PSI_TEXT_NAMESPACE using namespace sack::PSI::text; 

#   define _EDIT_NAMESPACE namespace edit {
#   define _EDIT_NAMESPACE_END } 
#   define USE_EDIT_NAMESPACE using namespace edit; 
#   define USE_PSI_EDIT_NAMESPACE using namespace sack::PSI::edit; 

#   define _SLIDER_NAMESPACE namespace slider {
#   define _SLIDER_NAMESPACE_END } 
#   define USE_SLIDER_NAMESPACE using namespace slider; 
#   define USE_PSI_SLIDER_NAMESPACE using namespace sack::PSI::slider; 

#   define _FONTS_NAMESPACE namespace font {
#   define _FONTS_NAMESPACE_END } 
#   define USE_FONTS_NAMESPACE using namespace font; 
#   define USE_PSI_FONTS_NAMESPACE using namespace sack::PSI::font; 

#   define _COMBOBOX_NAMESPACE namespace listbox {
#   define _COMBOBOX_NAMESPACE_END } 
#   define USE_COMBOBOX_NAMESPACE using namespace listbox; 
#   define USE_PSI_COMBOBOX_NAMESPACE using namespace sack::PSI::listbox; 

#   define _LISTBOX_NAMESPACE namespace listbox {
#   define _LISTBOX_NAMESPACE_END } 
#   define USE_LISTBOX_NAMESPACE using namespace listbox; 
#   define USE_PSI_LISTBOX_NAMESPACE using namespace sack::PSI::listbox; 

#   define _SCROLLBAR_NAMESPACE namespace scrollbar {
#   define _SCROLLBAR_NAMESPACE_END } 
#   define USE_SCROLLBAR_NAMESPACE using namespace scrollbar; 
#   define USE_PSI_SCROLLBAR_NAMESPACE using namespace sack::PSI::scrollbar; 

#   define _SHEETS_NAMESPACE namespace sheet_control {
#   define _SHEETS_NAMESPACE_END } 
#   define USE_SHEETS_NAMESPACE using namespace sheet_control; 
#   define USE_PSI_SHEETS_NAMESPACE using namespace sack::PSI::sheet_control; 

#   define _MOUSE_NAMESPACE namespace _mouse {
#   define _MOUSE_NAMESPACE_END } 
#   define USE_MOUSE_NAMESPACE using namespace _mouse; 
#   define USE_PSI_MOUSE_NAMESPACE using namespace sack::PSI::_mouse; 

#   define _XML_NAMESPACE namespace xml {
#   define _XML_NAMESPACE_END } 
#   define USE_XML_NAMESPACE using namespace xml; 
#   define USE_PSI_XML_NAMESPACE using namespace sack::PSI::xml; 

#   define _PROP_NAMESPACE namespace properties {
#   define _PROP_NAMESPACE_END } 
#   define USE_PROP_NAMESPACE using namespace properties; 
#   define USE_PSI_PROP_NAMESPACE using namespace sack::PSI::properties; 

#   define _CLOCK_NAMESPACE namespace clock {
#   define _CLOCK_NAMESPACE_END } 
#   define USE_CLOCK_NAMESPACE using namespace clock; 
#   define USE_PSI_CLOCK_NAMESPACE using namespace sack::PSI::clock; 

#else

#define PSI_NAMESPACE SACK_NAMESPACE 
#define _PSI_NAMESPACE
#define PSI_NAMESPACE_END SACK_NAMESPACE_END
#define USE_PSI_NAMESPACE

#   define _PSI_INTERFACE_NAMESPACE
#   define _PSI_INTERFACE_NAMESPACE_END

#   define _BUTTON_NAMESPACE 
#   define _BUTTON_NAMESPACE_END 
#   define USE_BUTTON_NAMESPACE
#   define USE_PSI_BUTTON_NAMESPACE

#   define _COLORWELL_NAMESPACE 
#   define _COLORWELL_NAMESPACE_END 
#   define USE_COLORWELL_NAMESPACE
#   define USE_PSI_COLORWELL_NAMESPACE

#   define _MENU_NAMESPACE 
#   define _MENU_NAMESPACE_END 
#   define USE_MENU_NAMESPACE
#   define USE_PSI_MENU_NAMESPACE

#   define _TEXT_NAMESPACE 
#   define _TEXT_NAMESPACE_END 
#   define USE_TEXT_NAMESPACE
#   define USE_PSI_TEXT_NAMESPACE

#   define _EDIT_NAMESPACE 
#   define _EDIT_NAMESPACE_END 
#   define USE_EDIT_NAMESPACE
#   define USE_PSI_EDIT_NAMESPACE

#   define _SLIDER_NAMESPACE 
#   define _SLIDER_NAMESPACE_END 
#   define USE_SLIDER_NAMESPACE
#   define USE_PSI_SLIDER_NAMESPACE

#   define _FONTS_NAMESPACE 
#   define _FONTS_NAMESPACE_END 
#   define USE_FONTS_NAMESPACE
#   define USE_PSI_FONTS_NAMESPACE

#   define _COMBOBOX_NAMESPACE 
#   define _COMBOBOX_NAMESPACE_END 
#   define USE_COMBOBOX_NAMESPACE
#   define USE_PSI_COMBOBOX_NAMESPACE

#   define _LISTBOX_NAMESPACE 
#   define _LISTBOX_NAMESPACE_END 
#   define USE_LISTBOX_NAMESPACE
#   define USE_PSI_LISTBOX_NAMESPACE

#   define _SCROLLBAR_NAMESPACE 
#   define _SCROLLBAR_NAMESPACE_END 
#   define USE_SCROLLBAR_NAMESPACE
#   define USE_PSI_SCROLLBAR_NAMESPACE

#   define _SHEETS_NAMESPACE 
#   define _SHEETS_NAMESPACE_END 
#   define USE_SHEETS_NAMESPACE
#   define USE_PSI_SHEETS_NAMESPACE

#   define _MOUSE_NAMESPACE 
#   define _MOUSE_NAMESPACE_END 
#   define USE_MOUSE_NAMESPACE
#   define USE_PSI_MOUSE_NAMESPACE

#   define _XML_NAMESPACE 
#   define _XML_NAMESPACE_END 
#   define USE_XML_NAMESPACE
#   define USE_PSI_XML_NAMESPACE

#   define _PROP_NAMESPACE 
#   define _PROP_NAMESPACE_END 
#   define USE_PROP_NAMESPACE
#   define USE_PSI_PROP_NAMESPACE

#   define _CLOCK_NAMESPACE 
#   define _CLOCK_NAMESPACE_END  
#   define USE_CLOCK_NAMESPACE 
#   define USE_PSI_CLOCK_NAMESPACE 

#endif

#define PSI_BUTTON_NAMESPACE PSI_NAMESPACE _BUTTON_NAMESPACE
#define PSI_BUTTON_NAMESPACE_END _BUTTON_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_COLORWELL_NAMESPACE PSI_NAMESPACE _COLORWELL_NAMESPACE
#define PSI_COLORWELL_NAMESPACE_END _COLORWELL_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_MENU_NAMESPACE PSI_NAMESPACE _MENU_NAMESPACE
#define PSI_MENU_NAMESPACE_END _MENU_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_TEXT_NAMESPACE PSI_NAMESPACE _TEXT_NAMESPACE
#define PSI_TEXT_NAMESPACE_END _TEXT_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_EDIT_NAMESPACE PSI_NAMESPACE _EDIT_NAMESPACE
#define PSI_EDIT_NAMESPACE_END _EDIT_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_SLIDER_NAMESPACE PSI_NAMESPACE _SLIDER_NAMESPACE
#define PSI_SLIDER_NAMESPACE_END _SLIDER_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_FONTS_NAMESPACE PSI_NAMESPACE _FONTS_NAMESPACE
#define PSI_FONTS_NAMESPACE_END _FONTS_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_LISTBOX_NAMESPACE PSI_NAMESPACE _LISTBOX_NAMESPACE
#define PSI_LISTBOX_NAMESPACE_END _LISTBOX_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_SCROLLBAR_NAMESPACE PSI_NAMESPACE _SCROLLBAR_NAMESPACE
#define PSI_SCROLLBAR_NAMESPACE_END _SCROLLBAR_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_SHEETS_NAMESPACE PSI_NAMESPACE _SHEETS_NAMESPACE
#define PSI_SHEETS_NAMESPACE_END _SHEETS_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_MOUSE_NAMESPACE PSI_NAMESPACE _MOUSE_NAMESPACE
#define PSI_MOUSE_NAMESPACE_END _MOUSE_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_XML_NAMESPACE PSI_NAMESPACE _XML_NAMESPACE
#define PSI_XML_NAMESPACE_END _XML_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_PROP_NAMESPACE PSI_NAMESPACE _PROP_NAMESPACE
#define PSI_PROP_NAMESPACE_END _PROP_NAMESPACE_END PSI_NAMESPACE_END

#define PSI_CLOCK_NAMESPACE PSI_NAMESPACE _CLOCK_NAMESPACE
#define PSI_CLOCK_NAMESPACE_END _CLOCK_NAMESPACE_END PSI_NAMESPACE_END


#endif
