/* ========================================================================
   Copyright (c) 1990,1995   Synergistic Software
   All Rights Reserved.
   =======================================================================
   Filename: MOUSE.C     -Handles low level mouse stuff
   Author: Chris Phillips
   ========================================================================
   Contains the following internal functions:

   Contains the following general functions:
   init_mouse            -Initializes the mouse
   set_mouse_max         -sets the max screen value for the mouse
   set_mouse_position    -sets the mouse's position
   read_mouse            -sets the position and button state variables
   draw_cursor           -draws a cursor
   cursor_in_box         -checks to see if the cursor is in a bounding box

   ======================================================================== */
/* ------------------------------------------------------------------------
   Includes
   ------------------------------------------------------------------------ */
/* ®RM250¯ */
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <i86.h>
#include <bios.h>
#include <string.h>

#include "system.h"
#include "engine.h"
#include "engint.h"
#include "machine.h"
#include "machint.h"

/* ------------------------------------------------------------------------
   Defines and Comile Flags
   ------------------------------------------------------------------------ */
#define MIN_MOUSE_X	(0)
#define MIN_MOUSE_Y	(0)
#define MAX_MOUSE_X	(MAX_VIEW_WIDTH-8)	// 1/3 cursor size JKE
#define MAX_MOUSE_Y	(MAX_VIEW_HEIGHT-8)	// ditto

/* ------------------------------------------------------------------------
   Macros
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Prototypes
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Global Variables
   ------------------------------------------------------------------------ */
LONG	cursor_x = 320;
LONG	cursor_y = 100;
LONG	hot_x = 0;
LONG	hot_y = 0;
LONG	mouse_button;
LONG	mouse_present = FALSE;
LONG	mouse_click = 0;
//LONG mouse_need_seg = 0;
//LONG mouse_found_seg = -1;
//LONG mouse_found_type;
SHORT	cursor_bitmap = fERROR;
BOOL	fAutoRestoreCursor = FALSE;

static UBYTE	st[1024];
static SHORT	fMouseHidden = 1;

extern SHORT	iDispBuffer;
extern BOOL		fRender;

/* ========================================================================
   Function    - CheckForMouse
   Description - Checks whether a mouse driver is present.
   Returns     - TRUE if mouse driver is present, FALSE if not.
	Notes			- [d4-17-97 JPC] Called from MACHINE.C: main
						so we can inform users that mouse is missing right away.
						Code was copied from init_mouse, below.
   ======================================================================== */
BOOL CheckForMouse ()
{
	union REGS inregs,outregs;

	/* check for mouse driver */
	inregs.w.ax = 0x0000;		/* function #35 return vector  33=int # */
	int386( 0x33, &inregs, &outregs );

	if (outregs.w.ax==0)
	{
		return FALSE;
	}

	return TRUE;
}


/* ========================================================================
   Function    - init_mouse
   Description - initializes the mouse and then sets the maxes and mins for it
   Returns     - void
   ======================================================================== */
void init_mouse (CSTRPTR name, BOOL Centered)
{
	union REGS inregs,outregs;
	PTR	p;
	LONG w,h;
//	char cursor_path[256];
	char path[256];


	/* check for mouse driver */
	inregs.w.ax = 0x0000;		/* function #35 return vector  33=int # */
	int386( 0x33, &inregs, &outregs );

	if (outregs.w.ax==0)
	{
		mouse_present = FALSE;
		return;
	}

	mouse_present = TRUE;

	/* reset mouse driver */
	inregs.w.ax=0x0000;      /*function #0*/
	int386( 0x33, &inregs, &outregs );


	/* set mouse sensitivity */
//JKE	inregs.w.ax=0x001a;
//	inregs.w.bx=64;
//	inregs.w.cx=64;
//	inregs.w.dx=0;
//	int386( 0x33, &inregs, &outregs );
//	set_mouse_position(160,100);		// removed because it causes the cursor to move every
							// time the cursor bit map changed.

	// load the cursor from the graphics dir
#if 0  // chrisb 5/30, this is obsolete
	sprintf(cursor_path, "graphics\\%s.pcx", name);
	if(InstallationType == INSTALL_SMALL)
	{
		sprintf(path,"%s%s",CDDrive,cursor_path);
	}
	else
	{
		sprintf(path,"%s",cursor_path);
	}
	
	if (cursor_bitmap != fERROR)
	{
		SetPurge(cursor_bitmap);
	}
#endif	
	sprintf(path, "graphics\\%s.pcx", name);
	cursor_bitmap = GetResourceStd(path,FALSE);
	if (cursor_bitmap == fERROR)
		fatal_error ("init_mouse: file %s not found", name);

	if(Centered)
	{
		p = ((PTR)BLKPTR(cursor_bitmap)) + sizeof(BITMHDR);
		w = ((BITMPTR)BLKPTR(cursor_bitmap))->w;
		h = ((BITMPTR)BLKPTR(cursor_bitmap))->h;
	
		hot_x = w/2;
		hot_y = h/2;
	}
	else
	{
		hot_x = 0;
		hot_y = 0;
	}
	
//JKE	set_mouse_max();
}

// THIS WHOLE FUNCTION IS NOT NEEDED --- JKE
/* ========================================================================
   Function    - set_mouse_max
   Description - sets the max coords for the mouse
   Returns     - void
   ======================================================================== */
void set_mouse_max()
{
	union REGS inregs,outregs;

	//This system is not used
	/*	make mouse x range  equal to ... 2 x MAX_MOUSE_X. we do this cause
		the mouse moves too fast in 1 for 1 mode. so when we read the mouse in
		the timer interrupt we divide by 2..... neat huh!*/

	inregs.w.ax=0x0007;      /*function #7*/
	inregs.w.cx=MIN_MOUSE_X;            /*min x*/
	inregs.w.dx=((screen_buffer_width)*2)-16;          /*max x*/
		// doubling the screen buffer width halves the granularity of the mouse
		// on my system in dos I could only move in 8 pixel jumps. JKE
//JKE	inregs.w.dx=screen_buffer_width-8;          /*max x*/
	int386( 0x33, &inregs, &outregs );

	/* make mouse range y equal to  0->200 */
	inregs.w.ax=0x0008;      /*function #8*/
	inregs.w.cx=MIN_MOUSE_Y;            /*min y*/
	inregs.w.dx=screen_buffer_height-8; /* 1/3 of cursor.height max y*/
	int386( 0x33, &inregs, &outregs );
}


// THIS FUNCTION WAS KEPT JUST FOR PRESERVING THE INTERFACE. IF THE CURSOR NEEDS TO BE CHANGED
// JUST CHANGE THE MOUSE VARIABLE
/* ========================================================================
   Function    - set_mouse_position
   Description - sets the position of the cursor
   Returns     - void
   ======================================================================== */
void set_mouse_position (LONG x,LONG y)
{
//	union REGS inregs,outregs;
	
	cursor_x = x;
	cursor_y = y; //we don't need the rest
	printf("SET_MOUSE_POSITION : %d, %d\n", x, y);

//	inregs.w.ax = 0x0004; 	/* mouse set */
//	inregs.w.cx = x<<1;	 	/* row */
		// correct for doubled buffer
//JKE	inregs.w.cx = x;	 	/* row */
//	inregs.w.dx = y;	 		/* col */
//	int386( 0x33, &inregs, &outregs );
}

/* ========================================================================
   Function    - read_mouse
   Description - causes cursor_x,cursor_y and mouse_button to be set.
   Returns     - void
   ======================================================================== */
void read_mouse()
{
	union REGS inregs,outregs;

	 /* read mouse position */
	// This is used only for the buttons now.
	inregs.w.ax = 0x0003;
	int386( 0x33, &inregs, &outregs );
	mouse_button = outregs.w.bx;
//JKE	cursor_x = outregs.w.cx/2;  /*x value is 0 to 640 so divide by 2*/
//	cursor_y = outregs.w.dx;

	// use mickeys to calculate delta x and delta y -JKE
	inregs.w.ax = 11;
	int386(0x33, &inregs, &outregs);
	cursor_x += (signed short)outregs.w.cx;
	cursor_y += (signed short)outregs.w.dx;

//	printf("Mickeys : %d, %d\n", horiz, vert);
	if (cursor_x > MAX_MOUSE_X)
		cursor_x = MAX_MOUSE_X;
	if (cursor_x < MIN_MOUSE_X)
		cursor_x = MIN_MOUSE_Y;
	if (cursor_y > MAX_MOUSE_Y)
		cursor_y = MAX_MOUSE_Y;
	if (cursor_y < MIN_MOUSE_Y)
		cursor_y = MIN_MOUSE_Y;

// This would cause the mouse to wander
//	cursor_x += hot_x ;
//	cursor_y += hot_y;
//	printf("Mouse position: %d, %d Mouse hot %d, %d\n",cursor_x,cursor_y,hot_x,hot_y);

}

/* ========================================================================
   Function    - update_buttons
   Description - causes mouse_button to be set.
   Returns     - void
   ======================================================================== */
void update_buttons()
{
	union REGS inregs,outregs;

	 /* read mouse position */
	inregs.w.ax = 0x0003;
	int386( 0x33, &inregs, &outregs );
	mouse_button = outregs.w.bx;
}

/* ========================================================================
   Function    - SysHideCursor
   Description - hide the cursor
   Returns     - void
   ======================================================================== */
void SysHideCursor (void)
{
	LONG	xy, x, y, yy, w, h, pix;

	if (!mouse_present)
		return;

	if (!fMouseHidden)				/* only hide if not already hidden */
	{
		w = ((BITMPTR)BLKPTR(cursor_bitmap))->w;
		h = ((BITMPTR)BLKPTR(cursor_bitmap))->h;

		// plot to the display buffer what is in the erase buffer
		for (yy=y=0; y<h; y++, yy+=w)
			for (xy=yy,x=0; x<w; x++,xy++)
				if ((pix = st[xy]))
					plot(cursor_x-hot_x+x, cursor_y-hot_y+y, pix);

		// clear the erase buffer
		memset(&st[0], 0, 1024);
	
		// force this new data to the display
		if(fRender)
			ScreenCopy(iDispBuffer,cursor_x-hot_x,cursor_y-hot_y,w,h, SC_DEFAULT_RES);
	}

	fMouseHidden++;					/* increase hidden level by one */
}

/* ========================================================================
   Function    - SysDrawCursor
   Description - make the cursor paint wiether it likes it or not
   Returns     -
   ======================================================================== */
void SysDrawCursor (void)
{
	PTR	p;
	LONG	xy, x, y, yy, w, h, pix;

	if (!mouse_present) return;

	if (!fMouseHidden)				/* only if not already hidden */
	{
		p = ((PTR)BLKPTR(cursor_bitmap)) + sizeof(BITMHDR);
		w = ((BITMPTR)BLKPTR(cursor_bitmap))->w;
		h = ((BITMPTR)BLKPTR(cursor_bitmap))->h;
	
		read_mouse();
		
		//plot to the display buffer the cursor bitmap
		for (yy=y=0; y<h; y++, yy+=w)
			for (xy=yy,x=0; x<w; x++,xy++)
				if ((pix = p[xy]))
				{
					//GEH plot(cursor_x-hot_x+x, cursor_y-hot_y+y, antia_table[(pix*256)+(st[xy]=get_pixel(cursor_x-hot_x+x, cursor_y-hot_y+y))] );
					st[xy]=get_pixel(cursor_x-hot_x+x, cursor_y-hot_y+y);	
					plot(cursor_x-hot_x+x, cursor_y-hot_y+y, pix);
				}
				
		// force this new data to the display
		if(fRender)
			ScreenCopy(iDispBuffer,cursor_x-hot_x,cursor_y-hot_y,w,h, SC_DEFAULT_RES);
	}
}

/* ========================================================================
   Function    - SysShowCursor
   Description - show the cursor
   Returns     -
   ======================================================================== */
void SysShowCursor (void)
{
	if (fMouseHidden > 0)
		fMouseHidden--;

	if(!fMouseHidden)
		SysDrawCursor();
}

/* ========================================================================
   Function    - SysForceCursor
   Description - force show the cursor
   Returns     -
   ======================================================================== */
void SysForceCursor (void)
{
	fMouseHidden = 0;
	SysDrawCursor();
}

/* ========================================================================
   Function    - draw_cursor
   Description - basic cursor draw cover function
   Returns     -
   ======================================================================== */
void draw_cursor()
{
	if (fAutoRestoreCursor)
	{
		SysHideCursor();
		SysShowCursor();
	}
	else
	{
		fMouseHidden = 0;
		SysDrawCursor();
	}

}

/* ========================================================================
   Function    - cursor_in_box
   Description - checks to see if the cursor is inside a bounding box
   Returns     - returns TRUE if cursor is inside the bounding box specified
   ======================================================================== */
LONG cursor_in_box (LONG x1,LONG y1,LONG w,LONG h)
{
	return (cursor_x>x1 && cursor_y>y1 && cursor_x<x1+w && cursor_y<y1+h);
}

/* ======================================================================== */


