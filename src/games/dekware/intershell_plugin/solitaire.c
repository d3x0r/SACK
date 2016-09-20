#define USE_IMAGE_INTERFACE l.pii
//#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <ctype.h>
#include <psi.h>
#include <sharemem.h>

// this is correct for cmake proper projects.
#include "../cards/cards.h"

#include <InterShell/intershell_registry.h>
#include <procreg.h>
#undef StrDup
#define StrDup(s) SaveNameConcatN( s, NULL )
/*
 * this module should be able to coordinate solitaire type games all in itself.
 * the deck can be created, and the motions between stacks defined such that
 * any solitaire game can be layed out by just adjusting the rules on the
 * card-stack controls.
 *
 *  Attributes
 *    Stack-down, under stack/line of other cards
 *    stack/nostack up
 *    the deck itself
 *    a deal-to pile from the deck to the table...
 *
 *    suite collector  : any card same suit
 *    ace collector : ace first, mst be number +1 next
 *    king collector : king first, countdown.
 *    next-card collector : up/down - same suit, must be number +/- 1
 *    suited next-card collector (guess there's an or'd state here)
 *
 *    select top only
 *    select up to next number
 *    select up to next number same suite
 *    select up to same number alternate color
 *    select always same suite consequetive
 *
 */



#define MakeSetFlag( bName )   \
	uintptr_t CPROC SetDeck##bName( uintptr_t psv, arg_list args )   \
{                                                               \
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv ); \
	PARAM( args, LOGICAL, yesno );                                                  \
	stack->flags.bName = yesno;                                                     \
	return psv;                                                                     \
}


#define MakeAddFlag( string, bName ) \
	AddConfigurationMethod( pch, string WIDE(#bName) WIDE("=%b"), SetDeck##bName );

#define SaveStackFlag( string, bName )    \
	fprintf( file, string WIDE(#bName)  WIDE("=%s\n"), stack->flags.bName?WIDE("yes"):WIDE("no") );


//typedef struct deck_tag *PDECK;

struct card_game {
	CTEXTSTR name;
	PDECK deck;
	uint32_t suits;
	uint32_t faces;
	PLIST controls;
	PSI_CONTROL selected_stack;
} ;

struct card_stack_control {
	struct {
		BIT_FIELD bVertical : 1;
		BIT_FIELD bReversed : 1;
		BIT_FIELD bStacked : 1;
		BIT_FIELD bNotStackedDown : 1;

		BIT_FIELD bOnlyAceWhenEmpty : 1;
		BIT_FIELD bOnlyKingWhenEmpty : 1;
		BIT_FIELD bLastAce : 1;
		BIT_FIELD bLastKing : 1;
		BIT_FIELD bMustPlayWhenEmpty : 1; // uses acitve.nMustPlay as first
		BIT_FIELD bOnlyPlusOne : 1;
		BIT_FIELD bOnlyMinusOne : 1;
		BIT_FIELD bOnlySame : 1; // same number must be played
		BIT_FIELD bAlternateSuit : 1;
		BIT_FIELD bSameSuit : 1;
		BIT_FIELD bNoSelect : 1; // no draw/move from.


		//MAKEFLAG( WIDE("select"), CHECKBOX_SELECT_TOP, bSelectOnlyTop );

		BIT_FIELD bSelectSameSuit : 1;
		BIT_FIELD bSelectAltSuit : 1;
		BIT_FIELD bSelectOnlyTop : 1;
		BIT_FIELD bSelectAny : 1; // any up to valid... if no restrict, up to all.
		BIT_FIELD bSelectPlusOne : 1;
		BIT_FIELD bSelectMinusOne : 1;

		BIT_FIELD bSelectAll : 1;
		BIT_FIELD bCompareTopOnly : 1;
		BIT_FIELD bDrawFrom : 1;
	} flags;

	struct {
		int nDrawAtStart;
		int nDrawDownAtStart;
		int nDrawAtDeal;
	} startup;

	struct {
		int nMustPlay; // specific card must be played here...
		int nCardsSelected; // number of cards selected from the top
	} active;

	PLIST allow_if_has_cards; // can play if any of these stacks have cards
	PLIST allow_move_to; // can move only to these stacks... defined on the source
	uint32_t clone_count; // number of times this was cloned
	//{
	 //  CTEXTSTR hand_stack;
	//};//PLIST;
	struct card_game *game; // game has the deck
	CTEXTSTR deck_stack;


	int32_t step_x;
	int32_t step_y;
	int32_t scaled_step_x;
	int32_t scaled_step_y;

	uint32_t width;
	uint32_t height;
	uint32_t real_width; // actual card_width - this is the width to output.
	uint32_t real_height;
	uint32_t image_width;  // for the mouse to compute
	uint32_t image_height;

	int nCards; // array, may be by dimensional...
	Image *card_image;
	CDATA background;
	CDATA empty_background;
	uintptr_t psv_update_callback; // reference from registering an update callback for this control - important for edit which may change which part of the deck we watch....
	uint32_t _b; // last known button state on this control
};

EasyRegisterControl( WIDE("Games/Cards/Card Stack"), sizeof( struct card_stack_control ) );
// MyControlID is the result of this...
// Or MyValidatedControlData( my_type, my_var_name, psi_control ) removes need for registration_struct.TypeID

static struct {
	struct card_game game;
	PLIST games;
	Image *card_image;
	PIMAGE_INTERFACE pii;
} l;

//-----------------------------------------------------------------------

enum { // edit control resource IDs
	LISTBOX_ACTIVE_GAMES = 5000
	  , LISTBOX_POSSIBLE_GAMES
	  , LISTBOX_DECK_STACKS
	  , LISTBOX_ALLOW_MOVE_TO
	  , LISTBOX_ALLOW_IF_HAS_CARDS
	  , LISTBOX_HAND_STACKS // eventually...
	  , EDIT_DECK_STACK_NAME // add a new name
	  , BTN_ADD_DECK_STACK
	  , EDIT_GAME_NAME
	  , EDIT_MUST_PLAY // specific card ID must be played
	  , EDIT_DRAW_AT_START
	  , EDIT_DRAW_AT_DEAL
	  , EDIT_DRAW_DOWN_AT_START
	  , BTN_ADD_GAME
	  , CHECKBOX_REVERSED_DRAW_ORDER
	  , CHECKBOX_VERTICAL
	  , CHECKBOX_STACKED
	  , CHECKBOX_UNSTACKED_FACEDOWN
	  , CHECKBOX_KING_ONLY
	  , CHECKBOX_ACE_ONLY
	  , CHECKBOX_KING_LAST
	  , CHECKBOX_ACE_LAST
	  , CHECKBOX_SAME_SUIT_ONLY
	  , CHECKBOX_ALT_SUIT_ONLY
	  , CHECKBOX_COMPARE_TOP_ONLY

	  , CHECKBOX_SELECT_ONLY_SUIT
	  , CHECKBOX_SELECT_ONLY_ALT_SUIT
	  , CHECKBOX_SELECT_ONLY_PLUS_ONE
	  , CHECKBOX_SELECT_ONLY_MINUS_ONE
	  , CHECKBOX_SELECT_TOP
	  , CHECKBOX_SELECT_ANY
	  , CHECKBOX_SELECT_ALL

	  /* actually either one of PLUS_ONE or MINUS_ONE then match allow, else disallow */
	  , CHECKBOX_FACE_ONLY
	  , CHECKBOX_MUST_PLAY_FACE
	  , CHECKBOX_FACE_PLUS_ONE_ONLY
	  , CHECKBOX_FACE_MINUS_ONE_ONLY
	  , CHECKBOX_NO_SELECT
};

PRELOAD( InitGame )
{
	l.pii = GetImageInterface();
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), LISTBOX_ALLOW_MOVE_TO, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), LISTBOX_ALLOW_IF_HAS_CARDS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), LISTBOX_ACTIVE_GAMES, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), LISTBOX_POSSIBLE_GAMES, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), LISTBOX_DECK_STACKS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), LISTBOX_HAND_STACKS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), EDIT_DECK_STACK_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), BTN_ADD_DECK_STACK, NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), EDIT_GAME_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), EDIT_MUST_PLAY, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), EDIT_DRAW_AT_START, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), EDIT_DRAW_AT_DEAL, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), EDIT_DRAW_DOWN_AT_START, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), BTN_ADD_GAME, NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_STACKED, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_UNSTACKED_FACEDOWN, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_KING_ONLY, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_ACE_ONLY, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_KING_LAST, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_ACE_LAST, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_SAME_SUIT_ONLY, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_ALT_SUIT_ONLY, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_FACE_PLUS_ONE_ONLY, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_FACE_MINUS_ONE_ONLY, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_NO_SELECT, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_REVERSED_DRAW_ORDER, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_VERTICAL, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_COMPARE_TOP_ONLY                , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards"), CHECKBOX_FACE_ONLY                , RADIO_BUTTON_NAME );
	
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards/select"), CHECKBOX_SELECT_ONLY_SUIT        , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards/select"), CHECKBOX_SELECT_ONLY_ALT_SUIT    , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards/select"), CHECKBOX_SELECT_ONLY_PLUS_ONE    , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards/select"), CHECKBOX_SELECT_ONLY_MINUS_ONE   , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards/select"), CHECKBOX_SELECT_TOP              , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards/select"), CHECKBOX_SELECT_ANY              , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/dekware/games/cards/select"), CHECKBOX_SELECT_ALL              , RADIO_BUTTON_NAME );
	{
		int s, f;
		int faces = 13;
		int suits = 4;
		TEXTCHAR filename[96];
		FILE *file;
		l.card_image = NewArray( Image, faces*suits + 1 );
		snprintf( filename, sizeof( filename ), WIDE("cardset1.config") );
		file = sack_fopen( GetFileGroup( WIDE("playing cards"), WIDE("images/cards") ), filename, WIDE("rt") );
		lprintf( WIDE("file is [%s]%p"), filename, file );
		if( file )
		{
			fgets( filename, sizeof( filename ), file );
			if( filename[strlen(filename)-1] == '\n' )
				filename[strlen(filename)-1] = 0;
			l.card_image[suits*faces] = LoadImageFile( filename );
			lprintf( WIDE("%s is %p"), filename, l.card_image[suits*faces] );
			if( !l.card_image[suits*faces] )
			{
				l.card_image[suits*faces] = MakeImageFile( 100, 100 );
				ClearImageTo( l.card_image[suits*faces], BASE_COLOR_ORANGE );
			}
			for( s = 0; s < suits; s++ )
				for( f = 0; f < faces; f++ )
				{
					fgets( filename, sizeof( filename ), file );
					if( filename[strlen(filename)-1] == '\n' )
						filename[strlen(filename)-1] = 0;
					/* read 2-14 instead of 1-13 */
					//sprintf( filename, WIDE("images/cards/card%d-%02d.bmp"), s+1, ( 12 + f ) % 13 + 2 );
					l.card_image[s*13+f] = LoadImageFile( filename );
				}
			fclose( file );
		}
		else
		{
			Release( l.card_image );
			l.card_image = NULL;
		}
	}

	//GetImageSize( l.card_image[0][0], &l.width, &l.height );


	// arbitrary... could be based off card size...
	//l.step_x = 20;
	//l.step_y = 40;


	l.game.name = WIDE("Stud"); // this is the first deck that is create by dekware.
	l.game.deck = CreateDeck( l.game.name, NULL );
	l.game.controls = NULL;
	AddLink( &l.games, &l.game );
}

static void FixupStackDisplayStep( PSI_CONTROL pc )
{
	FRACTION scale;
	MyValidatedControlData( struct card_stack_control *, stack, pc );
	SetFraction( scale, stack->real_width, stack->width );
	stack->scaled_step_x = ScaleValue( &scale, ( stack->step_x * (int32_t)stack->width ) / 100 );

	SetFraction( scale, stack->real_height, stack->height );
	stack->scaled_step_y = ScaleValue( &scale, ( stack->step_y  * (int32_t)stack->height ) / 100 );
}

static void SetStackCards( PSI_CONTROL pc, Image *images )
{
	uint32_t card_w, card_h;
	uint32_t control_w, control_h;
	Image image = GetControlSurface( pc );
	MyValidatedControlData( struct card_stack_control *, stack, pc );
	/* this is the only place this variable is set... */
	// selection of images needs to be added...
	stack->card_image = images;
	GetImageSize( images[0], &card_w, &card_h );
	GetImageSize( image, &control_w, &control_h );
	stack->image_width = control_w;
	stack->image_height = control_h;

	if( card_w > control_w )
	{
		FRACTION aspect;
		FRACTION scale;
		uint32_t scaled_height;
		// use aspec to compute card height if control_w is used instead of card_w
		SetFraction( aspect, card_h, card_w );
		scaled_height = ScaleValue( &aspect, control_w );
		if( scaled_height > control_h )
		{
			stack->real_height = control_h;
			SetFraction( scale, control_h, card_h );
			stack->real_width = ScaleValue( &scale, card_w );
		}
		else
		{
			stack->real_height = scaled_height;
			stack->real_width = control_w;
		}
	}
	else if( card_w < control_w )
	{
		FRACTION aspect;
		FRACTION scale;
		int scaled_height;
		SetFraction( aspect, card_h, card_w );
		// use aspec to compute card height if control_w is used instead of card_w
		scaled_height = ScaleValue( &aspect, control_w );
		if( scaled_height > control_h )
		{
			stack->real_height = control_h;
			// use this scale to compute card_w if card_h is used instead of control_h
			SetFraction( scale, control_h, card_h );
			stack->real_width = ScaleValue( &scale, card_w );
		}
		else
		{
			stack->real_width = control_w;
			SetFraction( scale, control_w, card_w );
			stack->real_height = ScaleValue( &scale, card_h );
		}

	}
	else if( card_h > control_h )
	{
		stack->real_height = card_h;
		stack->real_width = card_w;

	}
	else
	{
		// width matches.
		// height is less than or good, and therefore this fits.
		/* card and control are exactly the same.  output 1:1 */
		stack->real_height = card_h;
		stack->real_width = card_w;
	}
	stack->width = card_w;
	stack->height = card_h;
	FixupStackDisplayStep( pc );
}

static struct card_game *GetGame( CTEXTSTR name )
{
	struct card_game *game;
	INDEX idx;
	if( StrCmp( WIDE("Stud"), name ) == 0 )
		return &l.game;
	LIST_FORALL( l.games, idx, struct card_game *, game )
	{
		if( StrCmp( game->name, name ) == 0 )
			break;
	}
	if( !game )
	{
		game = New( struct card_game );
		game->name = StrDup( name );
		game->suits = 4;
		game->faces = 13;
		game->deck = CreateDeck( name, NULL );
		game->selected_stack = NULL;
		game->controls = NULL;
		AddLink( &l.games, game );
	}
	return game;
}

static void CPROC SmudgeMe( uintptr_t psv )
{
	SmudgeCommon( (PSI_CONTROL)psv );
}



static void SetControlGame( PSI_CONTROL pc, CTEXTSTR name )
{
	MyValidatedControlData( struct card_stack_control *, stack, pc );
	struct card_game *game = GetGame( name );
	if( stack->game != game )
	{
		INDEX idx;
		if( stack->game && stack->deck_stack )
		{
			RemoveCardStackUpdateCallback( GetCardStack( stack->game->deck, stack->deck_stack ), (uintptr_t)pc );
			idx = FindLink( &stack->game->controls, pc );
			if( idx != INVALID_INDEX )
				SetLink( &stack->game->controls, idx, NULL );
		}

		stack->game = game;
		AddLink( &game->controls, pc );
		if( stack->deck_stack )
			AddCardStackUpdateCallback( GetCardStack( stack->game->deck, stack->deck_stack ), SmudgeMe, (uintptr_t)pc );
	}
}

LOGICAL CanMoveCards( struct card_stack_control *from, struct card_stack_control *to )
{
	if( from->game == to->game )
	{
		PCARD_STACK from_stack = GetCardStack( from->game->deck, from->deck_stack );
		PCARD_STACK to_stack = GetCardStack( to->game->deck, to->deck_stack );
		if( !from_stack->cards )
			return FALSE;
		{
			PCARD_STACK draw = GetCardStack( from->game->deck, WIDE("Draw") );
			PCARD_STACK table = GetCardStack( from->game->deck, WIDE("Table") );
			if( to_stack == draw )
				return FALSE;
			if( to_stack == table && from_stack != draw )
				return FALSE;
		}
		if( !from->active.nCardsSelected )
			return FALSE; // can't move none selected
		{
			// compare stacks that this is allowed to move to
			INDEX idx;
			int had_one = 0;
			int allow = 0;
			struct card_stack_control *stack;
			LIST_FORALL( from->allow_move_to, idx, struct card_stack_control *, stack )
			{
				had_one = 1;
				if( GetCardStack( stack->game->deck, stack->deck_stack ) == to_stack )
				{
					allow = 1;
					break;
				}
			}
			if( had_one && !allow )
				return FALSE;
		}
		{
			// compare stacks that must not be empty to move this...
			INDEX idx;
			int had_one = 0;
			int allow = 0;
			struct card_stack_control *stack;
			LIST_FORALL( to->allow_if_has_cards, idx, struct card_stack_control *, stack )
			{
				PCARD_STACK test;
				had_one = 1;
				if( ( test = GetCardStack( stack->game->deck, stack->deck_stack) ) && test->cards )
				{
					allow = 1;
					break;
				}
			}
			if( had_one && !allow )
				return FALSE;
		}
		if( !to_stack->cards )
		{
			if( to->flags.bMustPlayWhenEmpty )
			{
				PCARD test = from->flags.bCompareTopOnly
					?from_stack->cards
					:( GetNthCard( from_stack, from->active.nCardsSelected-1 ) );
				if( ( test->id % from->game->faces ) != to->active.nMustPlay )
					return FALSE;
			}
			if( to->flags.bOnlyAceWhenEmpty )
			{
				{
					int n;
					//if( from->active.nCardsSelected > 1 )
					//	return FALSE;
					for( n = 0; n < from->active.nCardsSelected; n++ )
					{
						if( !( GetNthCard( from_stack, n )->id % from->game->faces ) )
						{
							break;
						}
					}
					if( n == from->active.nCardsSelected ) // no ace in stack.
						return FALSE;
				}
			}
			if( to->flags.bOnlyKingWhenEmpty )
			{
				{
					//if( from->active.nCardsSelected > 1 )
					//	return FALSE;
					//if( ( from_stack->cards->id % from->game->faces ) != (from->game->faces-1) )
					//	return FALSE;
					int n;
					for( n = 0; n < from->active.nCardsSelected; n++ )
					{
						if( GetNthCard( from_stack, n )->id % from->game->faces == (from->game->faces-1) )
						{
							break;
						}
					}
					if( n == from->active.nCardsSelected ) // no king in stack.
						return FALSE;
				}
			}
		}
		if( to->flags.bSameSuit )
		{
			PCARD test = from->flags.bCompareTopOnly
				?from_stack->cards
				:( GetNthCard( from_stack, from->active.nCardsSelected-1 ) );
			if( to_stack->cards ) // is not empty
			{
				if( ( to_stack->cards->id / from->game->faces ) != (test->id / from->game->faces) )
					return FALSE;
			}
		}
		if( to->flags.bAlternateSuit ) // is not empty
		{
			PCARD test = from->flags.bCompareTopOnly
				?from_stack->cards
				:( GetNthCard( from_stack, from->active.nCardsSelected-1 ) );
			if( test && to_stack->cards )
			{
				if( ( ( to_stack->cards->id / from->game->faces ) %2 )
					!= ( 1 - ( (test->id / from->game->faces ) % 2)) )
					return FALSE;
			}
		}
		if( to->flags.bOnlyPlusOne || to->flags.bOnlyMinusOne )
		{
			if( to_stack->cards ) // not empty
			{
				int failed = TRUE;
				PCARD test = from->flags.bCompareTopOnly
					?from_stack->cards
					:( GetNthCard( from_stack, from->active.nCardsSelected-1 ) );
				if( test )
				{
					if( to->flags.bOnlyPlusOne )
					{
						if( to->flags.bLastKing )
							if( CARD_NUMBER( to_stack->cards->id ) == (from->game->faces-1) )
								return FALSE;
						if( ( (to_stack->cards->id + 1) % from->game->faces ) ==
							( (test->id ) % from->game->faces ) )
							failed = FALSE;
					}
					if( to->flags.bOnlyMinusOne )
					{
						if( to->flags.bLastAce )
							if( CARD_NUMBER( to_stack->cards->id ) == 0 )
								return FALSE;
						if( ( (to_stack->cards->id) % from->game->faces ) ==
							( (test->id + 1 ) % from->game->faces ) )
							failed = FALSE;
					}
				}
				if( failed )
					return FALSE;
			}
		}

		return TRUE;
	}
	return FALSE;
}

void SelectCards( PSI_CONTROL pc, int card_index_picked )
{
	MyValidatedControlData( struct card_stack_control *, stack, pc );
	if( !stack->flags.bNoSelect )
	{
		if( stack->game->selected_stack )
		{
			MyValidatedControlData( struct card_stack_control *, selected_stack, stack->game->selected_stack );
			selected_stack->active.nCardsSelected = 0;
			SmudgeCommon( stack->game->selected_stack );
		}
		stack->game->selected_stack = pc;
		{
			PCARD_STACK card_stack = GetCardStack( stack->game->deck, stack->deck_stack );
			PCARD card = NULL;
			PCARD _card;
			int count = 0;
			int thinking = 1;
			// ignore count now... compute what we should select...
			do
			{
				if( count >= card_index_picked )
				{
					thinking = 0;
					continue;
				}
				_card = card;
				card = GetNthCard( card_stack, count++ );
				if( !card )
				{
					thinking = 0;
					count--;
					continue;
				}
				if( card->flags.bFaceDown )
				{
					//lprintf( WIDE("Cannot select face down cards (yet) ") );
					count--;
					break;
				}
				if( stack->flags.bSelectOnlyTop )
				{
					thinking = 0;
					continue;
				}


				if( stack->flags.bSelectAltSuit )
				{
					if( _card )
					{
						if( (CARD_SUIT(card->id)%2) != (1 - CARD_SUIT(_card->id)%2) )
						{
							count--;
							break;
						}
					}
				}
				if( stack->flags.bSelectSameSuit )
				{
					if( _card )
					{
						if( CARD_SUIT(card->id) != CARD_SUIT(_card->id) )
						{
							count--;
							break;
						}
					}
				}
				if( stack->flags.bSelectPlusOne )
				{
					if( _card )
					{
						if( CARD_NUMBER(card->id) != (1+CARD_NUMBER(_card->id)) )
						{
							count--;
							break;
						}
					}
				}
				if( stack->flags.bSelectMinusOne )
				{
					if( _card )
					{
						if( (1+CARD_NUMBER(card->id)) != (CARD_NUMBER(_card->id)) )
						{
							count--;
							break;
						}
					}
				}

			}while( thinking );
			stack->active.nCardsSelected = count;
		}
		SmudgeCommon( pc );
	}
}

LOGICAL DoMoveCards( PSI_CONTROL pc_from, PSI_CONTROL pc_to )
{
	MyValidatedControlData( struct card_stack_control *, stack_from, pc_from );
	MyValidatedControlData( struct card_stack_control *, stack_to, pc_to );
	if( stack_from->active.nCardsSelected )
	{
		if( CanMoveCards( stack_from, stack_to ) )
		{
			TransferCards( GetCardStack( stack_from->game->deck, stack_from->deck_stack )
						, GetCardStack( stack_to->game->deck, stack_to->deck_stack )
						, stack_from->active.nCardsSelected );
			stack_from->active.nCardsSelected = 0;
			stack_from->game->selected_stack = NULL;
			SmudgeCommon( pc_from );
			SmudgeCommon( pc_to );
			return TRUE;
		}
	}
	return FALSE;
}

static void OnSaveControl( WIDE("Games/Cards/Card Stack") )( FILE *file, uintptr_t psv )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	fprintf( file, WIDE("Card stack game is \'%s\'\n"), stack->game->name );
	fprintf( file, WIDE("Card stack deck stack is \'%s\'\n"), stack->deck_stack );
	fprintf( file, WIDE("Card stack Stacked=%s\n"), stack->flags.bStacked?WIDE("yes"):WIDE("no") );
	fprintf( file, WIDE("Card stack UnStacked Down=%s\n"), stack->flags.bNotStackedDown?WIDE("yes"):WIDE("no") );
	fprintf( file, WIDE("Card stack No Select=%s\n"), stack->flags.bNoSelect?WIDE("yes"):WIDE("no") );
	fprintf( file, WIDE("Card stack Only Ace=%s\n"), stack->flags.bOnlyAceWhenEmpty?WIDE("yes"):WIDE("no") );
	fprintf( file, WIDE("Card stack Only King=%s\n"), stack->flags.bOnlyKingWhenEmpty?WIDE("yes"):WIDE("no") );
	SaveStackFlag( WIDE("Card Stack "), bLastAce );
	SaveStackFlag( WIDE("Card Stack "), bLastKing );
	fprintf( file, WIDE("Card stack Only Same Suit=%s\n"), stack->flags.bSameSuit?WIDE("yes"):WIDE("no") );
	fprintf( file, WIDE("Card stack Only Alternate Suit=%s\n"), stack->flags.bAlternateSuit?WIDE("yes"):WIDE("no") );
	fprintf( file, WIDE("Card stack Only Minus One=%s\n"), stack->flags.bOnlyMinusOne?WIDE("yes"):WIDE("no") );
	fprintf( file, WIDE("Card stack Only Plus One=%s\n"), stack->flags.bOnlyPlusOne?WIDE("yes"):WIDE("no") );
	fprintf( file, WIDE("Card stack Draw %d at start\n"), stack->startup.nDrawAtStart );
	fprintf( file, WIDE("Card stack Draw %d at deal\n"), stack->startup.nDrawAtDeal );
	fprintf( file, WIDE("Card stack Draw %d Down at start\n"), stack->startup.nDrawDownAtStart );
	fprintf( file, WIDE("Card stack must play %d first\n"), stack->active.nMustPlay );
	fprintf( file, WIDE("Card stack background=$%08x\n"), stack->background );
	fprintf( file, WIDE("Card stack empty background=$%08x\n"), stack->empty_background );
	SaveStackFlag( WIDE("Card Stack "), bVertical );
	SaveStackFlag( WIDE("Card Stack "), bOnlySame );
	SaveStackFlag( WIDE("Card Stack "), bMustPlayWhenEmpty );
	SaveStackFlag( WIDE("Card Stack "), bReversed );
	SaveStackFlag( WIDE("Card Stack Select"), bSelectSameSuit );
	SaveStackFlag( WIDE("Card Stack Select"), bSelectAltSuit );
	SaveStackFlag( WIDE("Card Stack Select"), bSelectOnlyTop );
	SaveStackFlag( WIDE("Card Stack Select"), bSelectAny );
	SaveStackFlag( WIDE("Card Stack Select"), bSelectPlusOne );
	SaveStackFlag( WIDE("Card Stack Select"), bSelectMinusOne );

}


static uintptr_t CPROC SetDeckGame( uintptr_t psv, arg_list args )
{
	//MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, CTEXTSTR, name );
	SetControlGame( (PSI_CONTROL)psv, name );
	return psv;
}

static uintptr_t CPROC SetDeckStack( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, CTEXTSTR, name );
	RemoveCardStackUpdateCallback( GetCardStack( stack->game->deck, stack->deck_stack ), psv );
	stack->deck_stack = StrDup( name );
	AddCardStackUpdateCallback( GetCardStack( stack->game->deck, stack->deck_stack ), SmudgeMe, psv );
	return psv;
}

static uintptr_t CPROC SetAceOnly( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, LOGICAL, yesno );
	stack->flags.bOnlyAceWhenEmpty = yesno;
	return psv;
}

static uintptr_t CPROC SetKingOnly( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, LOGICAL, yesno );
	stack->flags.bOnlyKingWhenEmpty = yesno;
	return psv;
}

static uintptr_t CPROC SetSameSuitOnly( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, LOGICAL, yesno );
	stack->flags.bSameSuit = yesno;
	return psv;
}

uintptr_t CPROC SetAltSuitOnly( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, LOGICAL, yesno );
	stack->flags.bAlternateSuit = yesno;
	return psv;
}

uintptr_t CPROC SetPlusOneOnly( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, LOGICAL, yesno );
	stack->flags.bOnlyPlusOne = yesno;
	return psv;
}

uintptr_t CPROC SetMinusOneOnly( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, LOGICAL, yesno );
	stack->flags.bOnlyMinusOne = yesno;
	return psv;
}

uintptr_t CPROC SetDrawAtStart( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, int64_t, count );
	stack->startup.nDrawAtStart = count;
	return psv;
}

uintptr_t CPROC SetDrawAtDeal( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, int64_t, count );
	stack->startup.nDrawAtDeal = count;
	return psv;
}

uintptr_t CPROC SetDrawDownAtStart( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, int64_t, count );
	stack->startup.nDrawDownAtStart = count;
	return psv;
}

uintptr_t CPROC SetMustPlayEmpty( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, int64_t, count );
	stack->active.nMustPlay = count;
	return psv;
}

uintptr_t CPROC SetNoSelect( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, LOGICAL, yesno );
	stack->flags.bNoSelect = yesno;
	return psv;
}

uintptr_t CPROC SetDeckStackStacked( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, LOGICAL, yesno );
	stack->flags.bStacked = yesno;
	return psv;
}

uintptr_t CPROC SetDeckStackNotStackedDown( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, LOGICAL, yesno );
	stack->flags.bNotStackedDown = yesno;
	return psv;
}

static uintptr_t CPROC SetStackBackground( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, CDATA, color );
	stack->background = color;
	return psv;
}

static uintptr_t CPROC SetStackEmptyBackground( uintptr_t psv, arg_list args )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PARAM( args, CDATA, color );
	stack->empty_background = color;
	return psv;
}

MakeSetFlag( bSelectSameSuit );
MakeSetFlag( bSelectAltSuit );
MakeSetFlag( bSelectOnlyTop );
MakeSetFlag( bSelectAny );
MakeSetFlag( bSelectAll );
MakeSetFlag( bCompareTopOnly );
MakeSetFlag( bSelectPlusOne );
MakeSetFlag( bSelectMinusOne );
MakeSetFlag( bVertical );
MakeSetFlag( bReversed );
MakeSetFlag( bLastAce );
MakeSetFlag( bLastKing );
MakeSetFlag( bMustPlayWhenEmpty );
MakeSetFlag( bOnlySame );


static void OnLoadControl( WIDE("Games/Cards/Card Stack") )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	//MyValidatedControlData( struct card_stack_control *, stack, psv );
	MakeAddFlag( WIDE("Card Stack Select"), bSelectSameSuit );
	MakeAddFlag( WIDE("Card Stack Select"), bSelectAltSuit );
	MakeAddFlag( WIDE("Card Stack Select"), bSelectOnlyTop );
	MakeAddFlag( WIDE("Card Stack Select"), bSelectAny );
	MakeAddFlag( WIDE("Card Stack Select"), bSelectAll );
	MakeAddFlag( WIDE("Card Stack Select"), bCompareTopOnly );
	MakeAddFlag( WIDE("Card Stack Select"), bSelectPlusOne );
	MakeAddFlag( WIDE("Card Stack Select"), bSelectMinusOne );
	MakeAddFlag( WIDE("Card Stack "), bVertical );
	MakeAddFlag( WIDE("Card Stack "), bReversed );
	MakeAddFlag( WIDE("Card Stack "), bLastAce );
	MakeAddFlag( WIDE("Card Stack "), bLastKing );

	AddConfigurationMethod( pch, WIDE("Card stack game is \'%m\'"), SetDeckGame );
	AddConfigurationMethod( pch, WIDE("Card stack deck stack is \'%m\'"), SetDeckStack );
	AddConfigurationMethod( pch, WIDE("Card stack Stacked=%b"), SetDeckStackStacked );
	AddConfigurationMethod( pch, WIDE("Card stack UnStacked=%b"), SetDeckStackNotStackedDown );
	//AddConfigurationMethod( pch, WIDE("Card stack card prefix is \'%m\'"), NULL );
	AddConfigurationMethod( pch, WIDE("Card stack Only Ace=%b"), SetAceOnly );
	AddConfigurationMethod( pch, WIDE("Card stack Only King=%b"), SetKingOnly );
	AddConfigurationMethod( pch, WIDE("Card stack Only Same Suit=%b"), SetSameSuitOnly );
	AddConfigurationMethod( pch, WIDE("Card stack Only Alternate Suit=%b"), SetAltSuitOnly );
	AddConfigurationMethod( pch, WIDE("Card stack Only Minus One=%b"), SetMinusOneOnly );
	AddConfigurationMethod( pch, WIDE("Card stack Only Plus One=%b"), SetPlusOneOnly );
	AddConfigurationMethod( pch, WIDE("Card stack No Select=%b"), SetNoSelect );
	AddConfigurationMethod( pch, WIDE("Card stack Draw %i at start"), SetDrawAtStart );
	AddConfigurationMethod( pch, WIDE("Card stack Draw %i at deal"), SetDrawAtDeal );
	AddConfigurationMethod( pch, WIDE("Card stack Draw %i Down at start"), SetDrawDownAtStart );
	AddConfigurationMethod( pch, WIDE("Card stack must play %i first"), SetMustPlayEmpty );
	AddConfigurationMethod( pch, WIDE("Card stack background=%c"), SetStackBackground );
	AddConfigurationMethod( pch, WIDE("Card stack empty background=%c"), SetStackEmptyBackground );

}


static int OnDrawCommon( WIDE("Games/Cards/Card Stack") )( PSI_CONTROL pc )
{
	MyValidatedControlData( struct card_stack_control *, stack, pc );
	Image surface = GetFrameSurface( pc );
	if( ( surface->width != stack->image_width ) ||(  surface->height != stack->image_height)  )
		SetStackCards( pc, stack->card_image );
	ClearImageTo( surface, stack->background );
	{
		PCARD card;
		uint32_t s_width;
		uint32_t s_height;
		int max_id = stack->game->faces * stack->game->suits;
		if( !stack->flags.bStacked )
		{
			INDEX count = 0;
			INDEX row = 0;
			INDEX col = 0;
			uint32_t step_width = (((stack->step_x<0)?-stack->step_x:stack->step_x) * stack->width ) / 100;
			uint32_t step_height = (((stack->step_y<0)?-stack->step_y:stack->step_y) * stack->height ) / 100;
			uint32_t scaled_step_width = (stack->scaled_step_x<0)?-stack->scaled_step_x:stack->scaled_step_x;
			uint32_t scaled_step_height = (stack->scaled_step_y<0)?-stack->scaled_step_y:stack->scaled_step_y;
			int32_t x = (surface->width) - stack->real_width;
			int32_t y = (surface->height) - stack->real_height;//surface->height - stack->height;

			PCARD_STACK cardstack = GetCardStack( stack->game->deck, stack->deck_stack );
			/* should count the cards so we can better position this... */
			for( card = cardstack->cards; card; card = card->next )
			{
				Image card_image = stack->card_image[card->flags.bFaceDown?max_id:card->id];
				if( card_image )
				{
				s_width = stack->width;
				if( s_width > card_image->width )
					s_width = card_image->width;
				s_height = stack->height;
				if( s_height > card_image->height )
					s_height = card_image->height;
				if( card->flags.bFaceDown )
				{
					if( !stack->flags.bNotStackedDown )
					{
						if( count )
							break;
					}
				}
				if( count < stack->active.nCardsSelected )
				{
					if( !col && !row )
						BlotScaledImageSizedMultiShaded( surface, card_image
																 , x + col * stack->scaled_step_x
																 , y + row * stack->scaled_step_y
																 , stack->real_width
																 , stack->real_height
																 , 0, 0
																 , s_width
																 , s_height
																 , Color( 0, 255, 0 )
																 , Color( 128, 0, 0 )
																 , Color( 0, 0, 255 ) );
					else if( col && !row )
					{
						BlotScaledImageSizedMultiShaded( surface, card_image
																 , x + col * stack->scaled_step_x
																 , y + row * stack->scaled_step_y
																 , scaled_step_width
																 , stack->real_height
																 , 0, 0
																 , step_width
																 , s_height
																	, Color( 0, 255, 0 )
																	, Color( 128, 0, 0 )
																	, Color( 0, 0, 255 )
																 );
					}
					else if( !col && row )
						BlotScaledImageSizedMultiShaded( surface, card_image
																 , x + col * stack->scaled_step_x
																 , y + row * stack->scaled_step_y
																 , stack->real_width
																 , scaled_step_height
																 , 0, 0
																 , s_width
																 , step_height
														 , Color( 0, 255, 0 )
														 , Color( 128, 0, 0 )
														 , Color( 0, 0, 255 )
														 );
					else if( col && row )
						BlotScaledImageSizedMultiShaded( surface, card_image
																 , x + col * stack->scaled_step_x
																 , y + row * stack->scaled_step_y
																 , scaled_step_width
																 , scaled_step_height
																 , 0, 0
																 , step_width
																 , step_height
																	, Color( 0, 255, 0 )
																	, Color( 128, 0, 0 )
																	, Color( 0, 0, 255 )
																	);
				}
				else
				{
					if( !col && !row )
						BlotScaledImageSized( surface, card_image
																 , x + col * stack->scaled_step_x
																 , y + row * stack->scaled_step_y
																 , stack->real_width
																 , stack->real_height
																 , 0, 0
																 , s_width
																 , s_height
																 );
					else if( col && !row )
					{
						BlotScaledImageSized( surface, card_image
												  , x + col * stack->scaled_step_x
												  , y + row * stack->scaled_step_y
												  , scaled_step_width
												  , stack->real_height
												  , 0, 0
												  , step_width
												  , s_height
												  );
					}
					else if( !col && row )
						BlotScaledImageSized( surface, card_image
										  , x + col * stack->scaled_step_x
												  , y + row * stack->scaled_step_y
												  , stack->real_width
													, scaled_step_height
										  , 0, 0
										  , s_width
										  , step_height
										  );
					else if( col && row )
						BlotScaledImageSized( surface, card_image
										  , x + col * stack->scaled_step_x
												  , y + row * stack->scaled_step_y
												  , scaled_step_width
													, scaled_step_height
										  , 0, 0
										  , step_width
										  , step_height
										  );
				}
				}
				if( stack->flags.bVertical )
				{
					row++;
					if( ( y + (int32_t)row * stack->scaled_step_x ) < 0 )
					{
						col++;
						row = 0;
					}
				}
				else
				{
					col++;
					if( ( x + (int32_t)col * stack->scaled_step_x ) < 0 )
					{
						row++;
						col = 0;
					}
				}
				count++;
			}
			if( !count )
				ClearImageTo( surface, stack->empty_background );
		}
		else
		{
			PCARD card = GetCardStack( stack->game->deck, stack->deck_stack )->cards;
			if( card )
			{
				Image card_image = stack->card_image[card->flags.bFaceDown?max_id:card->id];
				s_width = stack->width;
				if( card_image )
				{
					if( s_width > card_image->width )
						s_width = card_image->width;
					s_height = stack->height;
					if( s_height > card_image->height )
						s_height = card_image->height;
					if( stack->active.nCardsSelected )
					{
						BlotScaledImageSizedMultiShaded( surface, card_image
																 , 0//x + col * stack->scaled_step_x
																 , 0//y + row * stack->scaled_step_y
																 , stack->real_width
																 , stack->real_height
																 , 0, 0
																 , stack->width
																 , stack->height
																 , Color( 0, 255, 0 )
																 , Color( 128, 0, 0 )
																 , Color( 0, 0, 255 ) );
				}
				else
				{
				BlotScaledImageSized( surface, card_image
										  , 0
										  , 0
										  , stack->real_width
										  , stack->real_height
										  , 0
										  , 0
										  , stack->width
										  , stack->height
										  );
				}
				}
			}
		}
	}
	// and then I can draw like cards or something here?
	return 1;
}


static int OnMouseCommon( WIDE("Games/Cards/Card Stack") )(PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)pc );
	// what can I do with a mouse?
	// I can drraw on a frame... I don't need to be a control...
	if( ( b & MK_LBUTTON ) && !( stack->_b & MK_LBUTTON ) )
	{
		PCARD_STACK card_stack = GetCardStack( stack->game->deck, stack->deck_stack );
		if( card_stack->cards && card_stack->cards->flags.bFaceDown )
			TurnTopCard( card_stack );
		else
		{
			int card_pos;
			int row = 0, col = 0;
			int cols = 7; // short-rows didn't work?
			uint32_t scaled_step_width = (stack->scaled_step_x<0)?-stack->scaled_step_x:stack->scaled_step_x;
			uint32_t scaled_step_height = (stack->scaled_step_y<0)?-stack->scaled_step_y:stack->scaled_step_y;
			if( !stack->flags.bVertical )
			{
				col = 0;

				if( y > stack->image_height - stack->real_height )
					row = 0;
				else
					row = 1+ (( (int32_t)( stack->image_height - stack->real_height ) - y ) /scaled_step_height);

				if( x > stack->image_width - stack->real_width )
					col = 0;
				else
				{
					//lprintf( WIDE("%d %d %d"),( ( stack->image_width - stack->real_width ) - x )/ scaled_step_width);
					col = 1 + (( (int32_t)( stack->image_width - stack->real_width ) - x ) / scaled_step_width);
				}
				card_pos = row * cols + col;
				if( stack->game->selected_stack &&
					stack->game->selected_stack != pc )
				{
					if( !DoMoveCards( stack->game->selected_stack, pc ) )
						SelectCards( pc, card_pos + 1 ); // pass the index of the card selected... may select up to this...
				}
				else
					SelectCards( pc, card_pos + 1 );
			}
			else
			{
				row = 0;
				if( y > stack->image_height - stack->real_height )
					col = 0;
				else
					row = 1+( ( stack->image_height - stack->real_height ) - y ) / scaled_step_height;
				if( x > stack->width - stack->width )
					col = 0;
				else
					col = 1+( ( stack->image_height - stack->real_height ) - y ) / scaled_step_width;
				card_pos = row * cols + col;
				if( stack->game->selected_stack &&
					stack->game->selected_stack != pc )
				{
					if( !DoMoveCards( stack->game->selected_stack, pc ) )
						SelectCards( pc, card_pos + 1 ); // pass the index of the card selected... may select up to this...
				}
				else
					SelectCards( pc, card_pos + 1 );
			}
		}

	}
	stack->_b = b;
	return 1;
}

void CPROC ButtonAddStack( uintptr_t psv, PSI_CONTROL button )
{
	TEXTCHAR buffer[256];
	//MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	GetControlText( GetNearControl( button, EDIT_DECK_STACK_NAME ), buffer, sizeof( buffer ) );
	//stack->deck_stack = StrDup( buffer );
	AddListItem( GetNearControl( button, LISTBOX_DECK_STACKS ), buffer );
}

void CPROC ButtonAddGame( uintptr_t psv, PSI_CONTROL button )
{
	TEXTCHAR buffer[256];
	//MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	GetControlText( GetNearControl( button, EDIT_GAME_NAME ), buffer, sizeof( buffer ) );
	AddListItem( GetNearControl( button, LISTBOX_POSSIBLE_GAMES ), buffer );
}

void FillDeckStackList( struct card_stack_control *stack, PSI_CONTROL list )
{
	INDEX idx;
	PCARD_STACK card_stack;
	PLISTITEM pli = NULL;
	if( !stack || !stack->game || !stack->game->deck )
		return;
	LIST_FORALL( stack->game->deck->card_stacks, idx, PCARD_STACK, card_stack )
	{
		pli = AddListItem( list, card_stack->name );
		if( StrCmp( card_stack->name, stack->deck_stack ) == 0 )
			SetSelectedItem( list, pli );
	}
	/* new deck doesn't have this stack (yet) */
	/* don't create stacks accidentally. */
	if( !pli )
		stack->deck_stack = NULL;
}


void FillGameList( PSI_CONTROL frame, struct card_game *current )
{
	PSI_CONTROL list = GetControl( frame, LISTBOX_POSSIBLE_GAMES );
	if( list )
	{
		INDEX idx;
		//PCARD_STACK card_stack;
		struct card_game *game;
		LIST_FORALL( l.games, idx, struct card_game *,game )
		{
			PLISTITEM pli = AddListItem( list, game->name );
			SetItemData( pli, (uintptr_t)game );
			if( game == current )
				SetSelectedItem( list, pli );

		}
	}
}

void CPROC GameSelected( uintptr_t psv, PSI_CONTROL pc, PLISTITEM pli )
{
	TEXTCHAR buffer[256];
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PSI_CONTROL list;
	GetListItemText( pli, buffer, sizeof( buffer ) );
	SetControlGame( (PSI_CONTROL)psv, buffer ); // at this point... we have a game with a deck that has 'Draw'
	ResetList( list = GetNearControl( pc, LISTBOX_DECK_STACKS ) );
	FillDeckStackList( stack, list );
}

void FillCanMoveIf( PSI_CONTROL frame, struct card_stack_control *stack )
{
	PSI_CONTROL list = GetControl( frame, LISTBOX_ALLOW_MOVE_TO );
	if( list )
	{
		INDEX idx;
		PCARD_STACK card_stack;
		PLISTITEM pli = NULL;
		LIST_FORALL( stack->game->deck->card_stacks, idx, PCARD_STACK, card_stack )
		{
			pli = AddListItem( list, card_stack->name );
			SetItemData( pli, (uintptr_t)card_stack );
			{
				INDEX idx;
				PCARD_STACK test_stack;
				LIST_FORALL( stack->allow_if_has_cards, idx, PCARD_STACK, test_stack )
				{
					if( test_stack == card_stack )
					{
						SetSelectedItem( list, pli );
						break;
					}
				}
			}
		}
	}
}

void FillCanMoveTo( PSI_CONTROL frame, struct card_stack_control *stack )
{
	PSI_CONTROL list = GetControl( frame, LISTBOX_ALLOW_MOVE_TO );
	if( list )
	{
		INDEX idx;
		PCARD_STACK card_stack;
		PLISTITEM pli = NULL;
		LIST_FORALL( stack->game->deck->card_stacks, idx, PCARD_STACK, card_stack )
		{
			pli = AddListItem( list, card_stack->name );
			SetItemData( pli, (uintptr_t)card_stack );
			{
				INDEX idx;
				PCARD_STACK test_stack;
				LIST_FORALL( stack->allow_move_to, idx, PCARD_STACK, test_stack )
				{
					if( test_stack == card_stack )
					{
						SetSelectedItem( list, pli );
						break;
					}
				}
			}
		}
	}
}

static uintptr_t OnEditControl( WIDE("Games/Cards/Card Stack") )( uintptr_t psv, PSI_CONTROL parent )
{
	MyValidatedControlData( struct card_stack_control *, stack, (PSI_CONTROL)psv );
	PSI_CONTROL frame = LoadXMLFrameOver( parent, WIDE("ConfigureGameCardStack.isFrame") );
	TEXTCHAR buffer[256];
	if( stack )
	{
		int done = 0;
		int okay = 0;
		// chose a new game for this control...
		PSI_CONTROL list;
		SetCommonButtons( frame, &done, &okay );
		SetButtonPushMethod( GetControl( frame, BTN_ADD_GAME ), ButtonAddGame, psv );
		SetButtonPushMethod( GetControl( frame, BTN_ADD_DECK_STACK ), ButtonAddStack, psv );
		SetSelChangeHandler( GetControl( frame, LISTBOX_POSSIBLE_GAMES ), GameSelected, psv );
		snprintf( buffer, sizeof( buffer ), WIDE("%d"), stack->active.nMustPlay );
		SetControlText( GetControl( frame, EDIT_MUST_PLAY ), buffer );
		snprintf( buffer, sizeof( buffer ), WIDE("%d"), stack->startup.nDrawAtStart );
		SetControlText( GetControl( frame, EDIT_DRAW_AT_START ), buffer );
		snprintf( buffer, sizeof( buffer ), WIDE("%d"), stack->startup.nDrawAtDeal );
		SetControlText( GetControl( frame, EDIT_DRAW_AT_DEAL ), buffer );
		snprintf( buffer, sizeof( buffer ), WIDE("%d"), stack->startup.nDrawDownAtStart );
		SetControlText( GetControl( frame, EDIT_DRAW_DOWN_AT_START ), buffer );
#define SetStackFlagCheck( flag, id )  \
		SetCheckState( GetControl( frame, id )			 , stack->flags.flag );
		SetStackFlagCheck( bSelectAny, CHECKBOX_SELECT_ANY );
		SetStackFlagCheck( bSelectAll, CHECKBOX_SELECT_ALL );
		SetStackFlagCheck( bOnlySame, CHECKBOX_FACE_ONLY );
		SetStackFlagCheck( bMustPlayWhenEmpty, CHECKBOX_MUST_PLAY_FACE );
		SetStackFlagCheck( bCompareTopOnly, CHECKBOX_COMPARE_TOP_ONLY );
		SetStackFlagCheck( bSelectOnlyTop, CHECKBOX_SELECT_TOP );
		SetStackFlagCheck( bSelectMinusOne, CHECKBOX_SELECT_ONLY_MINUS_ONE );
		SetStackFlagCheck( bSelectPlusOne, CHECKBOX_SELECT_ONLY_PLUS_ONE );
		SetStackFlagCheck( bSelectAltSuit, CHECKBOX_SELECT_ONLY_ALT_SUIT );
		SetStackFlagCheck( bSelectSameSuit, CHECKBOX_SELECT_ONLY_SUIT );
		SetCheckState( GetControl( frame, CHECKBOX_STACKED )			 , stack->flags.bStacked );
		SetCheckState( GetControl( frame, CHECKBOX_VERTICAL )			 , stack->flags.bVertical );
		SetCheckState( GetControl( frame, CHECKBOX_REVERSED_DRAW_ORDER )			 , stack->flags.bReversed );
		SetCheckState( GetControl( frame, CHECKBOX_UNSTACKED_FACEDOWN )			 , stack->flags.bNotStackedDown );
		SetCheckState( GetControl( frame, CHECKBOX_NO_SELECT )			 , stack->flags.bNoSelect );
		SetStackFlagCheck( bOnlyAceWhenEmpty, CHECKBOX_ACE_ONLY	 );
		SetStackFlagCheck( bOnlyKingWhenEmpty, CHECKBOX_KING_ONLY  );
		SetStackFlagCheck( bLastAce, CHECKBOX_ACE_LAST				 );
		SetStackFlagCheck( bLastKing, CHECKBOX_KING_LAST			  );
		SetCheckState( GetControl( frame, CHECKBOX_ALT_SUIT_ONLY )		, stack->flags.bAlternateSuit	 );
		SetCheckState( GetControl( frame, CHECKBOX_SAME_SUIT_ONLY )	  , stack->flags.bSameSuit			);
		SetCheckState( GetControl( frame, CHECKBOX_FACE_PLUS_ONE_ONLY ) , stack->flags.bOnlyPlusOne		);
		SetCheckState( GetControl( frame, CHECKBOX_FACE_MINUS_ONE_ONLY ), stack->flags.bOnlyMinusOne	  );
		list = GetControl( frame, LISTBOX_DECK_STACKS );
		FillDeckStackList( stack, list );

		FillGameList( frame, stack->game );
		FillCanMoveIf( frame, stack );
		FillCanMoveTo( frame, stack );
#if 0
		list = GetControl( frame, LISTBOX_POSSIBLE_GAMES );
		if( list )
		{
			PLIST dekware_decks;
			INDEX idx;
			//PCARD_STACK card_stack;
			struct card_game *game;
			LIST_FORALL( l.games, idx, struct card_game *,game )
			{
				AddListItem( list, game->name );
			}
		}
#endif
		list = GetControl( frame, LISTBOX_ACTIVE_GAMES );
		if( list )
		{
			AddListItem( list, WIDE("Uhmm yeah what games?") );
		}
		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
			PLISTITEM pli;
			stack->flags.bStacked = GetCheckState( GetControl( frame, CHECKBOX_STACKED ) );
			stack->flags.bOnlyAceWhenEmpty = GetCheckState( GetControl( frame, CHECKBOX_ACE_ONLY ) );
			stack->flags.bOnlyKingWhenEmpty = GetCheckState( GetControl( frame, CHECKBOX_KING_ONLY ) );
			stack->flags.bAlternateSuit = GetCheckState( GetControl( frame, CHECKBOX_ALT_SUIT_ONLY ) );
			stack->flags.bSameSuit = GetCheckState( GetControl( frame, CHECKBOX_SAME_SUIT_ONLY ) );
			stack->flags.bOnlyPlusOne = GetCheckState( GetControl( frame, CHECKBOX_FACE_PLUS_ONE_ONLY ) );
			stack->flags.bOnlyMinusOne = GetCheckState( GetControl( frame, CHECKBOX_FACE_MINUS_ONE_ONLY ) );
			stack->flags.bNoSelect = GetCheckState( GetControl( frame, CHECKBOX_NO_SELECT ) );
			stack->flags.bVertical		 = GetCheckState( GetControl( frame, CHECKBOX_VERTICAL )				 );
			stack->flags.bReversed		 = GetCheckState( GetControl( frame, CHECKBOX_REVERSED_DRAW_ORDER )  );
			stack->flags.bNotStackedDown = GetCheckState( GetControl( frame, CHECKBOX_UNSTACKED_FACEDOWN )	);
#define GetStackFlagCheck( flag, id )  \
			stack->flags.flag = GetCheckState( GetControl( frame, id ) );

			GetStackFlagCheck( bOnlyAceWhenEmpty, CHECKBOX_ACE_ONLY	);
			GetStackFlagCheck( bOnlyKingWhenEmpty, CHECKBOX_KING_ONLY );
			GetStackFlagCheck( bLastAce, CHECKBOX_ACE_LAST				);
			GetStackFlagCheck( bLastKing, CHECKBOX_KING_LAST			 );
			GetStackFlagCheck( bSelectAny, CHECKBOX_SELECT_ANY		  );
			GetStackFlagCheck( bSelectOnlyTop, CHECKBOX_SELECT_TOP	 );
			GetStackFlagCheck( bSelectMinusOne, CHECKBOX_SELECT_ONLY_MINUS_ONE );
			GetStackFlagCheck( bSelectPlusOne, CHECKBOX_SELECT_ONLY_PLUS_ONE );
			GetStackFlagCheck( bSelectAltSuit, CHECKBOX_SELECT_ONLY_ALT_SUIT );
			GetStackFlagCheck( bSelectSameSuit, CHECKBOX_SELECT_ONLY_SUIT );
			GetStackFlagCheck( bSelectAll, CHECKBOX_SELECT_ALL );
			GetStackFlagCheck( bCompareTopOnly, CHECKBOX_COMPARE_TOP_ONLY );
			GetStackFlagCheck( bOnlySame, CHECKBOX_FACE_ONLY );
			GetStackFlagCheck( bMustPlayWhenEmpty, CHECKBOX_MUST_PLAY_FACE );

			pli = GetSelectedItem( GetControl( frame, LISTBOX_DECK_STACKS ) );
			GetListItemText( pli, buffer, sizeof( buffer ) );
			RemoveCardStackUpdateCallback( GetCardStack( stack->game->deck, stack->deck_stack ), psv );
			stack->deck_stack = StrDup( buffer );
 			pli = GetSelectedItem( GetControl( frame, LISTBOX_POSSIBLE_GAMES ) );
			GetListItemText( pli, buffer, sizeof( buffer ) );
			SetControlGame( (PSI_CONTROL)psv, buffer );
			//GetCardStack( stack->game->deck, stack->deck_stack );
			AddCardStackUpdateCallback( GetCardStack( stack->game->deck, stack->deck_stack ), SmudgeMe, psv );

			GetControlText( GetControl( frame, EDIT_DRAW_AT_START ), buffer, sizeof( buffer ) );
			stack->startup.nDrawAtStart = atoi( buffer );

			GetControlText( GetControl( frame, EDIT_DRAW_AT_DEAL ), buffer, sizeof( buffer ) );
			stack->startup.nDrawAtDeal = atoi( buffer );

			GetControlText( GetControl( frame, EDIT_DRAW_DOWN_AT_START ), buffer, sizeof( buffer ) );
			stack->startup.nDrawDownAtStart = atoi( buffer );

			GetControlText( GetControl( frame, EDIT_MUST_PLAY ), buffer, sizeof( buffer ) );
			stack->active.nMustPlay = atoi( buffer );
}
		DestroyFrame( &frame );
	}
	return psv;
}

static int OnCreateCommon( WIDE("Games/Cards/Card Stack") )( PSI_CONTROL pc )
{
	MyValidatedControlData( struct card_stack_control *, stack, pc );
	//stack->deck = l.game.deck; // use the default deck for starters....
	//GetImageSize( image, &width, &height );
	//Image image = GetControlSurface( pc );
	//GetImageSize( stack->card_image[0], &stack->width, &stack->height );
	//GetImageSize( image, &width, &height );
	stack->step_x = -20;
	stack->step_y = -20;
	stack->background = AColor( 0, 0x12, 0x32, 0x10 );
	stack->empty_background = AColor( 0x32, 0x12, 0x0, 0x10 );
	SetStackCards( pc, l.card_image );
	//stack->flags.bStacked = 1;

	/* defaults... */
	stack->deck_stack = StrDup( WIDE("Table") ); // I want this to change by release/redup
	SetControlGame( pc, WIDE("Stud") );

	return TRUE;
}

static uintptr_t OnCreateControl( WIDE("Games/Cards/Card Stack") )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PSI_CONTROL pc = MakeControl( parent, MyControlID, x, y, w, h, -1 );
	//MyValidatedControlData( struct card_stack_control *, stack, pc );
	//stack->deck = l.game.deck; // use the default deck for starters....
	return (uintptr_t)pc;
}

static PSI_CONTROL OnGetControl( WIDE("Games/Cards/Card Stack") )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}

static void OnGlobalPropertyEdit( WIDE("Card Game") )( PSI_CONTROL parent_frame )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent_frame, WIDE("ConfigureCardGame.isFrame") );
	if( frame )
	{
		int done = 0;
		int okay = 0;
		SetCommonButtons( frame, &done, &okay );
		DisplayFrame( frame );
		CommonWait( frame );
		DestroyFrame( &frame );
	}

	return;
}

static void OnCloneControl( WIDE("Games/Cards/Card Stack") )( uintptr_t psvNew, uintptr_t psvOriginal )
{
	MyValidatedControlData( struct card_stack_control *, new_stack, (PSI_CONTROL)psvNew );
	MyValidatedControlData( struct card_stack_control *, original_stack, (PSI_CONTROL)psvOriginal );
	new_stack->flags = original_stack->flags;
	SetControlGame( (PSI_CONTROL)psvNew, original_stack->game->name );
	original_stack->clone_count++;
	{
		int lastchar = strlen( original_stack->deck_stack ) - 1;
		if( lastchar >= 0 )
			while( isdigit( original_stack->deck_stack[lastchar] ) )
				lastchar--;
		lastchar++;
		if( original_stack->deck_stack[lastchar] )
		{
			TEXTCHAR newbuf[256];
			snprintf( newbuf, sizeof( newbuf ), WIDE("%*.*s%d"), lastchar, lastchar
					  , original_stack->deck_stack
					  , atoi( original_stack->deck_stack + lastchar ) + original_stack->clone_count );
			new_stack->deck_stack = StrDup( newbuf );
		}
		else
			new_stack->deck_stack = original_stack->deck_stack;
	}

	new_stack->active = original_stack->active;
	new_stack->startup = original_stack->startup;

	// other paramters are computed... (during draw time)
}

static void OnKeyPressEvent( WIDE("Games/Cards/Start Game") )( uintptr_t psv )
{
	struct card_game *game = (struct card_game *)psv;
	Shuffle( game->deck );
	{
		PSI_CONTROL pc_draw = NULL;
		PSI_CONTROL pc;
		INDEX idx;
		LIST_FORALL( game->controls, idx, PSI_CONTROL, pc )
		{
			MyValidatedControlData( struct card_stack_control *, stack, pc );
			PCARD_STACK card_stack;
			int n;
			int moved = 0;
			if( StrCmp( stack->deck_stack, WIDE("Draw") ) == 0 )
				pc_draw = pc;
			else
			{
				// heh - draw down cards first, then draw up cards...
				// simplifies transition between start and play
				for( n = 0; n < stack->startup.nDrawDownAtStart; n++ )
				{
					moved++;
					TransferCards( GetCardStack( stack->game->deck, WIDE("Draw") )
									 , card_stack =GetCardStack( stack->game->deck, stack->deck_stack )
									 , 1 );

				}
				for( n = 0; n < stack->startup.nDrawAtStart; n++ )
				{
					moved++;
					TransferCards( GetCardStack( stack->game->deck, WIDE("Draw") )
									 , card_stack =GetCardStack( stack->game->deck, stack->deck_stack )
									 , 1 );
					if( card_stack && card_stack->cards )
						card_stack->cards->flags.bFaceDown = 0;
				}
			if( moved )
					SmudgeCommon( pc );
			}
		}
		if( pc_draw )
		SmudgeCommon( pc_draw );
	}
}


static uintptr_t OnCreateMenuButton( WIDE("Games/Cards/Start Game") )( PMENU_BUTTON button )
{
	return (uintptr_t)GetGame( WIDE("Stud") ); // default button
}

static uintptr_t OnEditControl( WIDE("Games/Cards/Start Game") )( uintptr_t psv, PSI_CONTROL parent )
{
	//TEXTCHAR buffer[256];
	PSI_CONTROL frame = LoadXMLFrameOver( parent, WIDE("ConfigureCardGameStartGame.isFrame") );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );
		{
			FillGameList( frame, (struct card_game *)psv );
		}
		DisplayFrame( frame );
		CommonWait( frame );
		if( okay )
		{
			PLISTITEM pli = GetSelectedItem( GetControl( frame, LISTBOX_POSSIBLE_GAMES ) );
			if( pli )
			{
				// okay change the game we're referencing...
				psv = GetItemData( pli );
			}
		}
		DestroyFrame( &frame );
	}
	return psv;
}


static void OnSaveControl( WIDE("Games/Cards/Start Game") )( FILE *file, uintptr_t psv )
{
	struct card_game *game = (struct card_game *)psv;
	fprintf( file, WIDE("Start card game button game=\'%s\'\n"), game->name );
}

static uintptr_t CPROC SetStartButtonGame( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	return (uintptr_t)GetGame( name ); // hrm I wonder if this actually gets set to the right place?
}

static void OnLoadControl( WIDE("Games/Cards/Start Game") )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	//struct card_game *game = (struct card_game *)psv;
	AddConfigurationMethod( pch, WIDE("Start card game button game=\'%m\'"), SetStartButtonGame );
}






////-------------------------------------------------------------------------

static void OnKeyPressEvent( WIDE("Games/Cards/Deal Game") )( uintptr_t psv )
{
	struct card_game *game = (struct card_game *)psv;
	//Shuffle( game->deck );
	{
		PSI_CONTROL pc_draw = NULL;
		PSI_CONTROL pc;
		INDEX idx;
		LIST_FORALL( game->controls, idx, PSI_CONTROL, pc )
		{
			MyValidatedControlData( struct card_stack_control *, stack, pc );
			PCARD_STACK card_stack;
			int n;
			int moved = 0;
			if( StrCmp( stack->deck_stack, WIDE("Draw") ) == 0 )
				pc_draw = pc;
			else
			{
				for( n = 0; n < stack->startup.nDrawAtDeal; n++ )
				{
					moved++;
					TransferCards( GetCardStack( stack->game->deck, WIDE("Draw") )
									 , card_stack =GetCardStack( stack->game->deck, stack->deck_stack )
									 , 1 );
					if( card_stack && card_stack->cards )
						card_stack->cards->flags.bFaceDown = 0;
				}
				if( moved )
					SmudgeCommon( pc );
			}
		}
		if( pc_draw )
			SmudgeCommon( pc_draw );
	}
}


static uintptr_t OnCreateMenuButton( WIDE("Games/Cards/Deal Game") )( PMENU_BUTTON button )
{
	return (uintptr_t)GetGame( WIDE("Stud") ); // default button
}

static uintptr_t OnEditControl( WIDE("Games/Cards/Deal Game") )( uintptr_t psv, PSI_CONTROL parent )
{
	//TEXTCHAR buffer[256];
	PSI_CONTROL frame = LoadXMLFrame( WIDE("ConfigureCardGameStartGame.isFrame") );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );
		{
			FillGameList( frame, (struct card_game *)psv );
		}
		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
			PLISTITEM pli = GetSelectedItem( GetControl( frame, LISTBOX_POSSIBLE_GAMES ) );
			if( pli )
			{
				// okay change the game we're referencing...
				psv = GetItemData( pli );
			}
		}
		DestroyFrame( &frame );
	}
	return psv;
}


static void OnSaveControl( WIDE("Games/Cards/Deal Game") )( FILE *file, uintptr_t psv )
{
	struct card_game *game = (struct card_game *)psv;
	fprintf( file, WIDE("Start card game button game=\'%s\'\n"), game->name );
}

static void OnLoadControl( WIDE("Games/Cards/Deal Game") )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	//struct card_game *game = (struct card_game *)psv;
	AddConfigurationMethod( pch, WIDE("Start card game button game=\'%m\'"), SetStartButtonGame );
}






