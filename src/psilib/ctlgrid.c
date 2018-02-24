#include <stdhdrs.h>
#include <string.h>
#include <sharemem.h>
#include <keybrd.h>
#include "controlstruc.h"
#include "controls.h"


//if one will notice, these are a blatant ripoff of ctllistbox.c

typedef struct griditem_tag
{
  //speaking of this, we should have something to set the cell data up for this cell.
  uintptr_t data;
  char *text;

	struct {
     //set if the data this points to should even be drawn
     unsigned int bAvailable :1;
     unsigned int bFocused  :1;  //should the focus be granted
     unsigned int bSelected :1;  //should the data be visible
	} flags;
   
  //FYI: This is meant as a singly linked list...  the prior and prior_group aren't
  //going to be expected to work
  //think of next as being on the x axis, and 'group's as being on the y-axis.

  //HOWEVER, for columns, only 'next' is used, so the above comments won't be true
  struct griditem_tag *next, *prior, *next_group, *prior_group;

} GRIDITEM, *PGRIDITEM;

typedef struct gridbox_tag
{
  CONTROL common;
  Image GridSurface;
  Image RowbarSurface;
  Image ColumnbarSurface;

  //DLC - the plan for item storage:
  //have a list of row entries, and a linked list per each row
  //I thought this was the best plan for both space efficiency, speed, and/or simplicity
  PGRIDITEM *records;

  //I think it's always good to know where the end of your list is
  //for quick and easy traversal to the end
  PGRIDITEM *records_end;
  PGRIDITEM *records_start;

  //I also think it's good to have your columns where one starts drawing pegged
  //in advance (we know what row is on, so we can skip that...and with this, we can
  //skip traversing until we see that we're in the viewport)
  PGRIDITEM *records_draw_start;

  PGRIDITEM records_first; //the first record of records
  PGRIDITEM records_last; //the last record of records


  PGRIDITEM *columns;
  PGRIDITEM columns_start;
  PGRIDITEM columns_end;
  PGRIDITEM columns_draw_start;

  PGRIDITEM *rows;
  PGRIDITEM rows_start;
  PGRIDITEM rows_end;
  PGRIDITEM rows_draw_start;

  int        options; // defined in controls.h around MakeGridBox

  PGRIDITEM items // first item
  , last // last item
    , current // current (focus cursor on this)
    , firstshown // first item shown
    , lastshown  // last item shown
    ;
  struct {
    unsigned int bSingle : 1; // alternative multiple selections can be made
    unsigned int bDestroying : 1;
  } localflags;
	PSI_CONTROL pcScroll;
	PSI_CONTROL pcScrolly;

  //	int TimeLastClick;
  int x, y, b; // old mouse info;

  //a whole bunch of function pointers and their respective purposes
  //DoubleClicker DoubleClickHandler;
  //uintptr_t psvDoubleClick;

  //thickness of the column and row bars
  int ColThickness;
  int RowThickness;

  void (*RenderCell)(PSI_CONTROL, uintptr_t);
  void (*RenderCellSelected)(PSI_CONTROL, uintptr_t);
  void (*SingleClickCell)(PSI_CONTROL);
  DoubleClicker CellDClickHandler;
  uintptr_t psvCellDoubleClick;
  int CellTimeLastClick;
  int Cellx, Celly, Cellb; // old mouse info;

  void (*RenderRowBarCell)(PSI_CONTROL, uintptr_t);
  void (*RenderRowBarCellSelected)(PSI_CONTROL, uintptr_t);
  void (*SingleClickRowBarCell)(PSI_CONTROL);
  DoubleClicker TopDClickHandler;  
  uintptr_t psvDoubleClick;
  int RowTimeLastClick;
  int Rowx, Rowy, Rowb; // old mouse info;

  void (*RenderColumnBarCell)(PSI_CONTROL, uintptr_t);
  void (*RenderColumnBarCellSelected)(PSI_CONTROL, uintptr_t);
  void (*SingleClickColumnBarCell)(PSI_CONTROL);
  DoubleClicker ColumnDClickHandler;
  uintptr_t psvColumnDoubleClick;
  int ColumnTimeLastClick;
  int Columnx, Columny, Columnb; // old mouse info;

  //cell data.  viewport is how many cells we see, total is how many cells 
  //we can look at with this viewport window
  int viewport_x_cells;
  int viewport_y_cells;

  int total_x_cells;
  int total_y_cells;

  //this is the offset into the top left hand corner
  //of the viewport
  int offset_cell_x;
  int offset_cell_y;

  int viewport_cell_width;
  int viewport_cell_height;

} GRIDBOX, *PGRIDBOX;


/*------12/18/02: Probably not a good idea to use these yet...----*/
void SetTotalCellsX(PSI_CONTROL pc, int i)
{
  PGRIDBOX pgb = (PGRIDBOX)pc;
  pgb->total_x_cells = i;
  //...as if this was all there is to resizing...
}

void SetTotalCellsY(PSI_CONTROL pc, int i)
{
  PGRIDBOX pgb = (PGRIDBOX)pc;
  pgb->total_y_cells = i;
  //...as if this was all there is to resizing...
}

void SetViewportCellsX(PSI_CONTROL pc, int i)
{
  PGRIDBOX pgb = (PGRIDBOX)pc;
  pgb->viewport_x_cells = i;
  //...as if this was all there is to resizing...
}

void SetViewportCellsY(PSI_CONTROL pc, int i)
{
  PGRIDBOX pgb = (PGRIDBOX)pc;
  pgb->viewport_y_cells = i;
  //...as if this was all there is to resizing...
}

//----------------------------------
//PGRIDBOX is the gridbox to initialize data to, x is the number of rows
int InitializeData(PGRIDBOX pc)
{
  int i;
  int j;

  //we need to allocate the number of records
  pc->records_end = Allocate(sizeof(GRIDITEM) *  pc->total_x_cells);
  pc->records_start = Allocate(sizeof(GRIDITEM) *  pc->total_x_cells);
  pc->records_draw_start = Allocate(sizeof(GRIDITEM) *  pc->total_x_cells);

  pc->columns = Allocate(sizeof(GRIDITEM) * pc->total_x_cells);
  pc->rows = Allocate(sizeof(GRIDITEM) * pc->total_y_cells);

  //our last one
  pc->records = Allocate(sizeof(GRIDITEM) * pc->total_x_cells);
  
  if(pc->records)
    {
      pc->columns_start = pc->columns[0];
      pc->columns_draw_start = pc->columns[0];
      pc->rows_start = pc->rows[0];
      pc->rows_draw_start = pc->rows[0];
      pc->columns_end = pc->columns[pc->total_x_cells - 1];
      pc->rows_end = pc->rows[pc->total_y_cells - 1];

      records_first = pc->records[0];
      records_last = pc->records[total_y_cells-1];

      for(i = 0; i < pc->total_x_cells; i++)
        {
          pc->records[i]->flags.bAvailable = 0;
          pc->records[i]->flags.bSelected = 0;
          pc->records[i]->flags.bFocused = 0;

          pc->columns[i]->flags.bAvailable = 0;
          pc->columns[i]->flags.bSelected = 0;
          pc->columns[i]->flags.bFocused = 0;

          //pointing it at itself...
          pc->records_end[i]->next = NULL;
          pc->records_draw_start[i] = pc->records[i];
          pc->records_start[i] = pc->records[i];

          if(i < pc->total_x_cells-1)
            {
              pc->columns[i]->next = pc->columns[i+1];
              pc->records[i]->next_group = pc->records[i+1];
            }
          else
            {
              pc->columns[i]->next = 0;
              pc->records[i]->next_group = 0;
            }
        }

      for(i = 0; i < pc->total_y_cells; i++)
        {
          pc->rows[i]->flags.bAvailable = 0;
          pc->rows[i]->flags.bSelected = 0;
          pc->rows[i]->flags.bFocused = 0;
          
          if(i < pc->total_x_cells-1)
            pc->rows[i]->next = pc->rows[i+1];
          else
            pc->rows[i]->next = 0;          
        }      
    }
  
  //presumably returns false on the failure to allocate
  //so it should return false here as well
  return pc->records && pc->records_end && pc->records_draw_start &&
    pc->columns && pc->rows;
}

/*----------------------------
int TackOnARow(PGRID pc, int nrows)
{


}
------------------------------*/

//---------------------------------------------------------------------------
static void RenderGridBox( PSI_CONTROL pc );

void DeleteGridItem( PSI_CONTROL pc, HGRIDITEM hgi)
{
  Log("Destroying a grid object");

  /*
	PGRIDITEM pgi = (PGRIDITEM)hgi, cur;
	PGRIDBOX pgb = (PGRIDBOX)pc;
	cur = pgb->items;
	while( cur )
	{
		if( cur == pgi )
			break;
		cur = cur->next;
	}
	if( cur )
	{
		if( pgi->prior )
			pgi->prior->next = pgi->next;
		else
			pgb->items = cur->next;
		if( pgi->next )
			pgi->next->prior = pgi->prior;
		else
			pgb->last = pgi->prior;

		if( pgb->firstshown == pgi )
		{
			if( pgi->next )
				pgb->firstshown = pgi->next;
			else if( pgi->prior )
				pgb->firstshown = pgi->prior;
			else
				pgb->firstshown = NULL;
		}
		if( pgb->lastshown == pgi )
			pgb->lastshown = NULL;
		if( pgb->current == pgi )
			pgb->current = NULL;
		Release( pgi->text );
		Release( pgi );
	}
	if( !pgb->localflags.bDestroying )
		RenderGridBox( pc );
      
  */
}

//---------------------------------------------------------------------------

PSI_PROC( void, ResetGrid )( PSI_CONTROL pc )
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
      PGRIDBOX pgb = (PGRIDBOX)pc;
      pgb->localflags.bDestroying = 1; // fake it...
      while( pgb->items )
         DeleteGridItem( pc, (HGRIDITEM)pgb->items );
      pgb->localflags.bDestroying = 0; // okay we're not really ...
		RenderGridBox( pc );
   }
}

//---------------------------------------------------------------------------

static void DestroyGridBox( PSI_CONTROL pc )
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
		PGRIDBOX pgb = (PGRIDBOX)pc;
      PGRIDITEM pcgi;
      PGRIDITEM prgi;
      
      //clear all the data
      
		pgb->localflags.bDestroying = TRUE;
		UnmakeImageFile( pgb->GridSurface );
		UnmakeImageFile( pgb->RowbarSurface );
		UnmakeImageFile( pgb->ColumnbarSurface );

      //dump the columns
      while(pcgi)
        {          
          pcgi = pgb->columns_start->next;
          //kill columns_start;
          pgb->columns_start = pcgi;
        }
      
      //dump the rows
      while(prgi)
        {
          prgi = pgb->rows_start->next;
          //kill rows_start;
          pgb->rows_start = prgi;
        }
      
      //and now to nuke the grid
      //set both to the top left hand corner
      pcgi = pgb->records_first;
      prgi = pgb->records_first;
          
      //go row by row, deleting all the columns, then delete that row and move on
      while(prgi)
        {
          while(pcgi)
            {
              pcgi = pgb->records_first->next;
              //kill records_first;
              pgb->records_first = pcgi;              
            }

          //records_first reset to the top-left
          pgb->records_first = prgi;

          //reset these to the top left, next row down
          prgi = pgb->records_first->next_group;
          pcgi = pgb->records_first->next_group;

          //kill records_first
                    
          //move this to the newly formed topmost left
          pgb->records_first = pcgi;
        }
	}
}


//-------------------------Data functions-----------------------------------

//shyly assigns data in the x, y position.  IE, if there isn't room, it will fail.
int AssignDataXY(PSI_CONTROL pc, int x, int y)
{
  PGRIDBOX pgb = (PGRIDBOX)pc;
  //first, do we have room for this co-ordinate
  
  //
}






//---------------------------------------------------------------------------
void ClearSelectedItemsXY( PGRIDBOX pgb )
{
	PGRIDITEM pgi;
	pgi = pgb->items;
	while( pgi )
	{
     //		pgi->flags.bSelected = FALSE;
		pgi = pgi->next;
	}
}

//---------------------------------------------------------------------------

static void RenderGridBox( PSI_CONTROL pc )
{

  Log("RENDERING...");

	int w, h, x, y, maxchars;
	PGRIDITEM pgi; //pointer to the top-left grid cell to draw
   PGRIDITEM pgc; //pointer to the first column cell to draw
   PGRIDITEM pgr; //pointer to the first

	PGRIDBOX pgb = (PGRIDBOX)pc;
   ImageFile *pSurface = pgb->GridSurface;
	ClearImageTo( pSurface, basecolors[EDIT_BACKGROUND] );
	
   //initialize these to their appropriate locations
   pgc = pgb->columns_draw_start;
   pgi = pgb->records_draw_start;
   pgr = pgb->rows_draw_start;

   

   //call the render for the column-bar
   //   y = 0;
   // while(pgc && (y < pgb->common.common.surface_rect.height))
   //   {
   //    if(pgc.flags
   //  }
   //call the render for the top
   

   //call the render for the grid cells
   

   /*------------------------------------
	while( pgi && y < pgb->common.common.surface_rect.height )
	{
		if( pgi->flags.bSelected )
		{
         BlatColor( pSurface, x-2, y, w-4, h, basecolors[SELECT_BACK] );
			PutStringEx( pSurface, x, y, basecolors[SELECT_TEXT], 0,	pgi->text, maxchars );
		}
		else
		{
			PutStringEx( pSurface, x, y, basecolors[EDIT_TEXT], 0,	pgi->text, maxchars );
		}

		if( pc->flags.bFocused && 
		    pgb->current == pgi )
			do_line( pSurface, x + 1, y + h-2, w - 6, y + h - 2, basecolors[SHADE] );
		y += h;
		if( y < pgb->common.common.surface_rect.height ) // probably is only partially shown...
			pgb->lastshown = pgi;
		pgi = pgi->next;
	}
   -------------------------------------*/

}

//---------------------------------------------------------------------------

PSI_PROC( HGRIDITEM, GetXYthItem )( PSI_CONTROL pc, int idx, int idy )
{
	PGRIDBOX pgb = (PGRIDBOX)pc;
	PGRIDITEM pgi, _pgi;
	_pgi = pgi = pgb->items;
	while( idx && pgi )
	{
		idx--;
		_pgi = pgi;
		pgi = pgi->next;
	}
	if( !pgi )
		return (HGRIDITEM)_pgi;
	return (HGRIDITEM)pgi;
}

//---------------------------------------------------------------------------

int GetItemCountXY( PGRIDBOX pgb )
{
	PGRIDITEM pgi;
	int cnt = 0;
	pgi = pgb->items;
	while( pgi )
	{
		cnt++;
		pgi = pgi->next;
	}
	return cnt;
}

//---------------------------------------------------------------------------

int GetItemIndexXY( PGRIDBOX pgb, PGRIDITEM pgi )
{
	PGRIDITEM cur;
	int cnt = 0;
	cur = pgb->items;
	while( cur )
	{
		if( pgi == cur )
			break;
		cnt++;
		cur = cur->next;
	}
	if( cur )
		return cnt;
	return -1;
}

//---------------------------------------------------------------------------

void ScrollBarUpdateX( uintptr_t psvGrid, int type, int current )
{
	PGRIDBOX pgb = (PGRIDBOX)psvGrid;
	if( pgb->common.common.nType == GRIDBOX_CONTROL )
	{
     //pgb->firstshown = (PGRIDITEM)GetNthItem( pgb, current );
		RenderGridBox( (PSI_CONTROL)pgb );
	}	
}

//---------------------------------------------------------------------------

void ScrollBarUpdateY( uintptr_t psvGrid, int type, int current )
{
	PGRIDBOX pgb = (PGRIDBOX)psvGrid;
	if( pgb->common.common.nType == GRIDBOX_CONTROL )
	{
     //pgb->firstshown = (PGRIDITEM)GetNthItem( pgb, current );
		RenderGridBox( (PSI_CONTROL)pgb );
	}	
}

//---------------------------------------------------------------------------
void UpdateScrollForGrid( PGRIDBOX pgb )
{
	int cur = GetItemIndex( pgb, pgb->firstshown );
	int range = ( GetItemIndex( pgb, pgb->lastshown ) - cur ) + 1;
   SetScrollParams( pgb->pcScroll, 0, cur, range, GetItemCount( pgb ) );
}
//---------------------------------------------------------------------------

void MouseGridBox( PSI_CONTROL pc, int x, int y, int b )
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
		PGRIDBOX pgb = (PGRIDBOX)pc;
		if( b & MK_LBUTTON )
		{
			if( !(pgb->b & MK_LBUTTON ) )
			{
            PGRIDITEM pgi;
            
            printf("x = %d, y = %d, b = %d\n",x,y,b);
            if((y > pgb->RowThickness) && (x > pgb->ColThickness))
            {
              int clicked_cellx;
              int clicked_celly;
              int clicked_cellx_last;
              int clicked_celly_last;

              printf("Grid shot\n");
              //what cell did we just click on?
              //divide our position by the cell dimensions and add the offset
              
              clicked_cellx = x/pgb->viewport_cell_width + pgb->offset_cell_x;
              clicked_celly = y/pgb->viewport_cell_height + pgb->offset_cell_y;
              
              if( ( GetTickCount() - pgb->CellTimeLastClick ) < 250 )
                {
                  //is it the same cell?
                  clicked_cellx_last = pgb->Cellx/pgb->viewport_cell_width + pgb->offset_cell_x;
                  clicked_celly_last = pgb->Celly/pgb->viewport_cell_height + pgb->offset_cell_y;

                  if(( clicked_cellx == clicked_cellx_last ) && (clicked_celly == clicked_celly_last))
                    //pgb->DoubleClickHandler( pgb->psvDoubleClick, pc, (uintptr_t)pgb->current );
                    printf("Double click handler invoked\n");
                }

              pgb->CellTimeLastClick = GetTickCount();

              //h = GetFontHeight( NULL );
              //line = ( y - 2 ) / h;
              //pgi = pgb->firstshown;
              pgb->Cellx = x;
              pgb->Celly = y;
              pgb->Cellb = b;
              printf("Grid shot");


              //do something with what we have
              //pgi = getourcellanddosomethingwithit

              if( pgi )
                {
                  if( pgb->localflags.bSingle )
                    ClearSelectedItems( pgb );
                  pgi->flags.bSelected = !pgi->flags.bSelected;
                  pgb->current = pgi;
                  RenderGridBox( pc );
                }              
            }
            //it's on the column
            else if(x < pgb->ColThickness)
            {
              printf("Column");
              pgb->Columnx = x;
              pgb->Columny = y;
              pgb->Columnb = b;
            }
            else if(y < pgb->RowThickness)
            {
              printf("Row");
              pgb->Rowx = x;
              pgb->Rowy = y;
              pgb->Rowb = b;
            }
         }
      }
		pgb->x = x;
		pgb->y = y;
		pgb->b = b;

   }
}
//---------------------------------------------------------------------------

static void KeyGridControl( PSI_CONTROL pc, int key )
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
		PGRIDBOX pgb = (PGRIDBOX)pc;
		//printf( WIDE("%08x\n"), key );
		if( key & 0x80000000 )
		{
			switch( key & 0xFF )
			{
			case KEY_UP:
			   if( GetItemIndex( pgb, pgb->current ) < GetItemIndex( pgb, pgb->firstshown ) )
			   	pgb->firstshown = pgb->current;
			   if( GetItemIndex( pgb, pgb->current ) > GetItemIndex( pgb, pgb->lastshown ) )
				   pgb->firstshown = pgb->current;
			   if( pgb->current && pgb->current->prior )
			   {
			   	if( pgb->current == pgb->firstshown )
			   	{
			   		pgb->firstshown = pgb->firstshown->prior;
					}
			   	pgb->current = pgb->current->prior;
			   }
				else
					if( !pgb->current )
						pgb->current = pgb->items;
				RenderGridBox( pc );
            UpdateScrollForGrid( pgb );
				break;
			case KEY_DOWN:
			   if( GetItemIndex( pgb, pgb->current ) < GetItemIndex( pgb, pgb->firstshown ) )
			   	pgb->firstshown = pgb->current;
			   if( GetItemIndex( pgb, pgb->current ) > GetItemIndex( pgb, pgb->lastshown ) )
				   pgb->firstshown = pgb->current;
			   if( pgb->current && pgb->current->next )
			   {
			   	if( pgb->current->next == pgb->lastshown )
			   	{
			   		pgb->firstshown = pgb->firstshown->next;
					}
				   pgb->current = pgb->current->next;
			   }
				else
					if( !pgb->current )
						pgb->current = pgb->items;
				RenderGridBox( pc );
            UpdateScrollForGrid( pgb );
				break;
			case KEY_PGUP:
				break;
			case KEY_PGDN:
				break;
			case KEY_SPACE:
           if( pgb->current )
           	{
              if( pgb->localflags.bSingle )
                ClearSelectedItems( pgb );
              //pgb->current->flags.bSelected = !pgb->current->flags.bSelected;
            }
				RenderGridBox( pc );
				break;
			case KEY_ESCAPE:
				InvokeDefault( pc, INV_CANCEL );
				break;
			case KEY_ENTER:
				InvokeDefault( pc, INV_OKAY );
				break;
			}
		}
	}
}	

//---------------------------------------------------------------------------

PSI_CONTROL MakeGridBox( PFRAME pf, int options, int x, int y, int w, int h, 
                      int viewport_x, int viewport_y, int total_x, int total_y, 
                      int row_thickness, int column_thickness, uintptr_t nID )
{
	PGRIDBOX pgb = (PGRIDBOX)CreateControl( pf, nID, x, y, w, h, BORDER_THIN|BORDER_INVERT
			 									     , (sizeof( GRIDBOX ) - sizeof( CONTROL )) );

	pgb->pcScroll = MakeScrollBar( (PFRAME)pgb, 0
                                  , pgb->common.common.surface_rect.width-15, 0
                                  , 15, pgb->common.common.surface_rect.height-15, nID );

   Log4("Making scroll bar from %d, %d size of w=%d h=%d", pgb->common.common.surface_rect.width-15, 0, 15, pgb->common.common.surface_rect.height-15);

   SetScrollUpdateMethod( pgb->pcScroll, ScrollBarUpdateX, (uintptr_t)pgb );
   //SetScrollParams(pgb->pcScroll, 0, 0, total_x, total_x-viewport_x );

   
   Log4("Making scroll bar from %d, %d size of w=%d h=%d", 0, pgb->common.common.surface_rect.height-15, pgb->common.common.surface_rect.width, 15);
	pgb->pcScrolly = MakeScrollBar( (PFRAME)pgb, 1
                                  , 0, pgb->common.common.surface_rect.height-15
                                  , pgb->common.common.surface_rect.width, 15, nID );

   //just a note for something to clear up at a later time.  We're probably going
   //to have to decide if we should iterate as the scrollbar is moved, or wait
   //to iterate to it's present position on the ScrollBarUpdate*
   SetScrollUpdateMethod( pgb->pcScrolly, ScrollBarUpdateY, (uintptr_t)pgb );
   //SetScrollParams(pgb->pcScrolly, 0, 0, total_y, total_y-viewport_y );
   
   pgb->ColThickness = row_thickness;
   pgb->RowThickness = column_thickness;

   pgb->viewport_cell_width = (w-column_thickness) / viewport_x;
   pgb->viewport_cell_height = (h-row_thickness) / viewport_y;
   

   //rowbar - bar going left to right
   pgb->RowbarSurface = MakeSubImage( pgb->common.common.Surface
                                      , 0, 0
                                      , pgb->common.common.surface_rect.width
                                      , pgb->RowThickness);

   //columnbar - bar going up and down
   pgb->ColumnbarSurface = MakeSubImage( pgb->common.common.Surface
                                         , 0, pgb->RowThickness
                                         , pgb->ColThickness
                                         , pgb->common.common.surface_rect.height - pgb->RowThickness);

   //grid surface
	pgb->GridSurface = MakeSubImage( pgb->common.common.Surface
												, pgb->RowThickness, pgb->ColThickness
												, pgb->common.common.surface_rect.width - 15
												, pgb->common.common.surface_rect.height - 15);



   //initialize data
   InitializeData(pgb);

   

   pgb->localflags.bSingle = TRUE;
	pgb->pcScroll->flags.bNoFocus = TRUE;
	pgb->common.common.nType = GRIDBOX_CONTROL;
	pgb->common.DrawThySelf = RenderGridBox;
	pgb->common.DrawThySelf = NULL;
	pgb->common.KeyProc = KeyGridControl;
	pgb->common.Destroy = DestroyGridBox;
	pgb->options = options;
	SetControlMouse( (PSI_CONTROL)pgb, MouseGridBox );
	pgb->items 	 = NULL;
	pgb->last 	 = NULL;
   pgb->current = NULL;
   pgb->firstshown = NULL;
   pgb->lastshown  = NULL;
	return (PSI_CONTROL)pgb;
}

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------

HGRIDITEM AddGridItem( PSI_CONTROL pc, char *text, int x, int y, uintptr_t data)
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
		PGRIDBOX pgb = (PGRIDBOX)pc;
		PGRIDITEM pgi = Allocate( sizeof( GRIDITEM ) );


      //check to see if we even have this in range
      //if(x > whatever);
      //if(y > whatever);
      
      
      //traverse our list
      

      /*
		pgi->text = Allocate( strlen( text ) + 1 );
		pgi->flags.bSelected = FALSE;
		pgi->flags.bFocused = FALSE;
		strcpy( pgi->text, text );
		pgi->data = 0;
		if( pgb->options & GRIDOPT_SORT )
		{
			PGRIDITEM find;
			find = pgb->items;
			while( find )
			{
				if( strcmp( find->text, pgi->text ) > 0 )
				{
					pgi->next = find;
					if( ( pgi->prior = find->prior ) )
						pgi->prior->next = pgi;
					find->prior = pgi;
					break;
				}
				find = find->next;
			}
			if( !find )
			{
				goto add_at_end;
			}
			else if( find == pgb->items )
				pgb->items = pgi;
		}
		else
		{
add_at_end:
			if( pgb->last )
				pgb->last->next = pgi;
			else
			{
				pgb->firstshown =  
				pgb->current = pgb->items = pgi;
				pgi->flags.bSelected = TRUE;
				pgi->flags.bFocused = TRUE;
			}
			pgi->next = NULL;
			pgi->prior = pgb->last;
			pgb->last = pgi;
		}
      */
		RenderGridBox( pc );
      UpdateScrollForGrid( pgb );
		return (HGRIDITEM)pgi;
      
	}
	return 0;
}

//---------------------------------------------------------------------------

void SetSelectedItemXY( PSI_CONTROL pc, HGRIDITEM hgi )
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
		PGRIDBOX pgb = (PGRIDBOX)pc;
		PGRIDITEM pgi = (PGRIDITEM)hgi;
      if( pgb->localflags.bSingle )
        ClearSelectedItems( pgb );
      //		pgi->flags.bSelected = TRUE;
		RenderGridBox( pc );		
	}	
}

//---------------------------------------------------------------------------

void SetCurrentItemXY( PSI_CONTROL pc, HGRIDITEM hgi )
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
		PGRIDBOX pgb = (PGRIDBOX)pc;
		PGRIDITEM pgi = (PGRIDITEM)hgi;
		pgb->current = pgi;
		RenderGridBox( pc );
	}
}

//---------------------------------------------------------------------------

void SetItemDataXY( HGRIDITEM hgi, uintptr_t psv )
{
	PGRIDITEM pgi = (PGRIDITEM)hgi;
	if( hgi )
		pgi->data = psv;
}

//---------------------------------------------------------------------------

uintptr_t GetItemDataXY( HGRIDITEM hgi )
{
	PGRIDITEM pgi = (PGRIDITEM)hgi;
	if( pgi )
		return pgi->data;
	return 0;
}

//---------------------------------------------------------------------------

PSI_PROC( HGRIDITEM, GetSelectedItemXY )( PSI_CONTROL pc )
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
		PGRIDBOX pgb = (PGRIDBOX)pc;
		PGRIDITEM pgi;
		pgi = pgb->items;
      //DLC:commented out
      //		while( pgi )
		//{
		//	if( pgi->flags.bSelected )
		//		return (HGRIDITEM)pgi;
		//	pgi = pgi->next;
		//}
	}
	return 0;
}

//---------------------------------------------------------------------------

PSI_PROC( void, GetItemTextXY )( HGRIDITEM hgi, char *buffer, int bufsize )
{
	strncpy( buffer, ((PGRIDITEM)hgi)->text, bufsize-1 );
	buffer[bufsize] = 0;
}

//---------------------------------------------------------------------------

PSI_PROC( int, GetSelectedItemsXY )( PSI_CONTROL pc, HGRIDITEM *pGrid, int *nSize )
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
		PGRIDBOX pgb = (PGRIDBOX)pc;

	}
	return 0;
}

//---------------------------------------------------------------------------

PSI_PROC( HGRIDITEM, FindGridItemXY )( PSI_CONTROL pc, char *text )
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
		PGRIDBOX pgb = (PGRIDBOX)pc;
		PGRIDITEM pgi = pgb->items;

		while( pgi )
		{
			if( !strcmp( pgi->text, text ) )
				break;
			pgi = pgi->next;
		}
		return (HGRIDITEM)pgi;
	}
	return 0;
}

//---------------------------------------------------------------------------

PSI_PROC( void, SetGridColumnClickHandler )( PSI_CONTROL pc, DoubleClicker proc, uintptr_t psvUser )
{
	if( pc->common.nType == GRIDBOX_CONTROL )
	{
		PGRIDBOX pgb = (PGRIDBOX)pc;
		pgb->ColumnDClickHandler = proc;
		pgb->psvDoubleClick = psvUser;
	}
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

//	if( pc->nType == GRIDBOX_CONTROL )
//	{
//		PGRIDBOX pgb = (PGRIDBOX)pc;
//	}
// $Log: ctlgrid.c,v $
// Revision 1.2  2004/10/08 13:07:42  d3x0r
// Okay beginning to look a lot like PRO-GRESS
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.8  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
