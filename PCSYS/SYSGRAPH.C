/* =®RM250¯======================================================================
   Copyright (c) 1990,1995   Synergistic Software
   All Rights Reserved.
   =======================================================================
   Filename: SYSGRAPH.C  -Handles low level graphics routines
   Author: Chris Phillips
   ========================================================================
   Contains the following internal functions:

   Contains the following general functions:
   set_svga_page        -???
   CalcPage             -Calculate the size of a page on the SVGA card
   set_hires            -Sets the system to run in high resolution
   set_lowres           -sets the system to run in Low Resolution
   init_graph           -initializes the graphics system
   init_pal             -initializes the palettes (loads the pal.pal file)
   load_pal             -loads a palette and sets it active
   close_graph          -closes the graphics system
   set_pal              -sets the active palette to another one
   update_screen        -updates the screen (copies the display buffer)
   clear_screen         -makes the screen black

   ======================================================================== */
/* ------------------------------------------------------------------------
   Includes
   ------------------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graph.h>
#include <dos.h>
#include <i86.h>
#include <bios.h>
#include <conio.h>
#include <malloc.h>
#include "system.h"
#include "engine.h"
#include "engint.h"
#include "machine.h"
#include "machint.h"
#include "player.hxx"

/* ------------------------------------------------------------------------
   Defines and Compile Flags
   ------------------------------------------------------------------------ */
#define WAD_PATH                        "graphics\\"
/* ------------------------------------------------------------------------
   Macros
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Prototypes
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Global Variables
   ------------------------------------------------------------------------ */
LONG    svga_granularity = 1;                   // for TRIDENT cards
LONG    graphics_mode = 0;
//GEH PTR       screen;
PTR     disp;
LONG    disp_color;
COLORSPEC CurPal[256];
BOOL fHighRes;  // So First call will change it.
SHORT   iDispBuffer = fERROR;


extern LONG             margin_left;
extern LONG             margin_right;
extern LONG             margin_top;
extern LONG             margin_bottom;



/* ======================================================================
 *   Various stuff for new update_screen and ScreenCopy (ED)
 * ====================================================================== */
USHORT _iPage;

/* Sets the SVGA page without the overhead of the Watcom int386 function */
void _SetSVGAPage(SHORT page);
#pragma aux _SetSVGAPage parm [edx] = \
                "push eax" \
                "push ebx" \
                "xor ebx, ebx" \
                "mov eax,0x4f05" \
                "int 0x10" \
                "pop ebx" \
                "pop eax"

/* Determines SVGA page from granularity and sets it */
void _CalcSetSVGAPage(LONG p, LONG g);
#pragma aux _CalcSetSVGAPage parm [eax] [ebx] = \
					 "push edx" \
                "xor edx, edx" \
                "idiv ebx" \
                "mov edx, eax" \
                "mov eax,0x4f05" \
                "xor ebx, ebx" \
                "int 0x10" \
					 "pop edx"

/* Copy a line to VRAM within one page */
void _CopyLine(PTR Src, PTR Dest, ULONG sOff, ULONG dOff, SHORT n);
#pragma aux _CopyLine parm [esi] [edi] [eax] [ebx] [ecx] = \
                "add esi, eax" \
                "add edi, ebx" \
                "and ecx, 0x0000ffff" \
                "cld" \
                "rep movsb"

ULONG _UpdateOffsetAndPage(ULONG doff, LONG gran, USHORT page);
#pragma aux _UpdateOffsetAndPage parm [eax] [ebx] [dx] value [ecx] modify [edx] = \
                "mov ecx, eax" \
                "xor edx, edx" \
                "idiv ebx" \
                "add dx, ax" \
                "imul ebx" \
                "sub ecx, eax" \
                "xor ebx, ebx" \
                "mov eax, 0x4f05" \
                "int 0x10"

/* Copy a line to VRAM that crosses a page boundary */
void _CopySplitLine(PTR Src, PTR Dest, ULONG sOff, ULONG doff, SHORT n);
#pragma aux _CopySplitLine parm [esi] [edi] [eax] [ebx] [ecx] modify [edx]= \
                "push edi" \
                "add esi, eax" \
                "add edi, ebx" \
                "mov edx, 0x00010000" \
                "sub edx, ebx" \
                "add esi, edx" \
                "push esi" \
                "sub esi, edx" \
                "push ecx" \
                "mov ecx, edx" \
                "push edx" \
                "cld" \
                "rep movsb" \
                "mov dx, _iPage" \
                "inc dx" \
                "mov _iPage, dx" \
                "xor ebx, ebx" \
                "mov eax, 0x4f05" \
                "int 0x10" \
                "pop edx" \
                "pop ecx" \
                "sub ecx, edx" \
                "pop esi" \
                "pop edi" \
                "rep movsb"

/* Move some double words */
void _DWordCopy(PTR dest, PTR src, ULONG dwords);
#pragma aux _DWordCopy parm [edi] [esi] [ecx] = \
                "cld" \
                "rep movsd"

static void set_term_hires(LONG unused, LONG arg );
static void set_term_lowres(LONG unused, LONG  arg );
/* ===================================================================== */

/* ========================================================================
   Function    - init_graph
   Description - initializes the graphics system
   Returns     - void
   ======================================================================== */
void init_graph (LONG init_mode)
{
	if (init_mode == 0)
	{
	        fHighRes = TRUE;        // Guarentee the resolution will change.
	        set_term_lowres(0L, 0L);
	}
	else
	{
	        fHighRes = FALSE;       // Guarentee the resolution will change.
	        set_term_hires(0L, 0L);
	}
	
	disp=(PTR)0xA0000;
	
	svga_granularity = CalcPage();
	
	screen=(PTR)D32DosMemAlloc(MAX_VIEW_WIDTH*MAX_VIEW_HEIGHT);
	if(screen == NULL)
	{
		// couldn't get low memory, so use memmanag space
	    screen=(PTR)(BLKPTR(NewLockedBlock(MAX_VIEW_WIDTH*MAX_VIEW_HEIGHT)));
	}
	
	if(screen==NULL)
	        fatal_error("Can't allocate screen buffer");
	
	iDispBuffer = NewExternalBlock(screen);
	
	clear_screen();
	init_pal("default");
	init_shade_table("default");
}

/* ========================================================================
   Function    - set_svga_page
   Description -
   Returns     -
   ======================================================================== */
void set_svga_page (ULONG p)
{
        union REGS regs;

        regs.w.ax = 0x4f05;
        regs.w.bx = 0x0;
        regs.w.dx = (USHORT) p;
        int386(0x10,&regs,&regs);
}

/* ========================================================================
   Function    - CalcPage
   Description - Calculate the size of a page on the SVGA card
   Returns     - Granularity
   ======================================================================== */
LONG CalcPage (void)
{
        ULONG           rv;

        set_svga_page(0);
        disp[4096] = 0;
        disp[4097] = 0;
        disp[4098] = 0;

        set_svga_page(1);
        disp[0] = 0xAB;
        disp[1] = 0xCA;
        disp[2] = 0xBC;

        set_svga_page(0);
        rv = 65536L;                                    /* 64k */
        if (disp[4096]==0xAB && disp[4097]==0xCA && disp[4098]==0xBC)
                rv = 4096L;                                     /* 4k */

        // printf("Granularity: %ld\n",rv);

        disp[4096] = 0;
        disp[4097] = 0;
        disp[4098] = 0;
        return rv;
}

/* ========================================================================
   Function    - set_term_hires
   Description - sets the system to run in High Resolution mode
   Returns     - void
   ======================================================================== */
#pragma unreferenced off
static void set_term_hires(LONG unused, LONG arg )
#pragma unreferenced on
{
	LONG NewMapCX;
	LONG NewMapCY;
        if (fHighRes == FALSE)
        {
                if( _setvideomode(0x101) == 0 )
                        fatal_error("Can't Set Graph mode! VESA not present?");

                graphics_mode = SVGA;
				
                fHighRes = TRUE;
                set_pal((char *)&CurPal[0]);
				
				ZoomMapAbsolute(0,MapZoomFactor()/2);
			
				NewMapCX=MapCenterX()/2;
				NewMapCY=MapCenterY()*2;
				SetMapCenter(NewMapCX,NewMapCY);

                set_screen_size(640,480);

                set_mouse_max();
			 	
				PlayerSpeed = PlayerHiresSpeed;	// GWP
        }
}
/* ========================================================================
   Function    - set_hires
   Description - sets the system to run in High Resolution mode
   Returns     - void
   ======================================================================== */
#pragma unreferenced off
void set_hires(LONG unused, LONG arg )
#pragma unreferenced on
{
	LONG NewMapCX;
	LONG NewMapCY;
        if (fHighRes == FALSE)
        {
                // GWP if( _setvideomode(0x101) == 0 )
                // GWP         fatal_error("Can't Set Graph mode! VESA not present?");

                // GWP graphics_mode = SVGA;
				
                fHighRes = TRUE;
                set_pal((char *)&CurPal[0]);

				ZoomMapAbsolute(0,MapZoomFactor()/2);
				NewMapCX=MapCenterX()/2;
				NewMapCY=MapCenterY()*2;
				SetMapCenter(NewMapCX,NewMapCY);
								
                set_screen_size(640,480);

                set_mouse_max();
			 	
				PlayerSpeed = PlayerHiresSpeed;	// GWP
        }
}

/* ========================================================================
   Function    - set_term_lowres
   Description - sets the system to run in Low Resolution mode
   Returns     - void
   ======================================================================== */
#pragma unreferenced off
static void set_term_lowres(LONG unused, LONG  arg )
#pragma unreferenced on
{
	LONG NewMapCX;
	LONG NewMapCY;
        if (fHighRes == TRUE)
        {
                union REGS regs;

                regs.w.ax=0x93;                         // mode 0x13 (320x200x256color)
                int386(0x10,&regs,&regs);

                graphics_mode = 0;
				
				fHighRes = FALSE;	// GWP
                set_pal((char *)&CurPal[0]);

				ZoomMapAbsolute(0,MapZoomFactor()*2);
				NewMapCX=MapCenterX()*2;
				NewMapCY=MapCenterY()/2;
				SetMapCenter(NewMapCX,NewMapCY);

                set_screen_size(320,200);

                set_mouse_max();
				
//				ZoomMapAbsolute(0,MapZoomFactor()*2);	//GWP WRC
//				NewMapCX=MapCenterX()*2;				//GWP WRC
//				NewMapCY=MapCenterY()/2;				//GWP WRC
//				SetMapCenter(NewMapCX,NewMapCY);		//GWP WRC
			
				PlayerSpeed = (SHORT)PlayerLoresSpeed;	// GWP
        }
}
/* ========================================================================
   Function    - set_lowres
   Description - sets the system to run in Low Resolution mode
   Returns     - void
   ======================================================================== */
#pragma unreferenced off
void set_lowres(LONG unused, LONG  arg )
#pragma unreferenced on
{
	LONG NewMapCX;
	LONG NewMapCY;
        if (fHighRes == TRUE)
        {
                // GWP union REGS regs;

                // GWP regs.w.ax=0x93;                         // mode 0x13 (320x200x256color)
                // GWP int386(0x10,&regs,&regs);

                // GWP graphics_mode = 0;
				
				fHighRes = FALSE;	// GWP
				set_screen_size(320,240);	// GWP
                set_pal((char *)&CurPal[0]);


				ZoomMapAbsolute(0,MapZoomFactor()*2);
				NewMapCX=MapCenterX()*2;
				NewMapCY=MapCenterY()/2;
				SetMapCenter(NewMapCX,NewMapCY);

                set_screen_size(320,200);

                // GWP fHighRes = FALSE;
                set_mouse_max();
				
//				gMapInfo.zoom_factor *= 2;	// GWP WRC
//				gMapInfo.center_x *= 2;		// GWP WRC
//				gMapInfo.center_y /= 2;		// GWP WRC
			
				PlayerSpeed = (SHORT)PlayerLoresSpeed;	// GWP
        }
}

/* ========================================================================
   Function    - set_pal
   Description - sets the specified palette to be active
   Returns     - void
   ======================================================================== */
void set_pal(char * pal)
{
        UBYTE           i;
#if 0
        char far *ptr;
        struct SREGS sregs;
        union REGS inregs, outregs;

        /* set pal */
        segread(&sregs);
        inregs.w.ax = 0x1012;
        inregs.w.bx = 0x0;
        inregs.w.cx = 0x100;
        ptr = pal;
        inregs.x.edx = FP_OFF( ptr );
        sregs.es     = FP_SEG( ptr );
        int386x( 0x10, &inregs, &outregs, &sregs );
#endif

        outp(0x3c8, 0);                                                         /* set starting index */
        i = 0;
        do
        {
                outp(0x3c9, *pal++);            /* red */
                outp(0x3c9, *pal++);            /* green */
                outp(0x3c9, *pal++);            /* blue */
        } while (++i);
}

/* ========================================================================
   Function    - load_pal
   Description - loads a palette and sets to the active one
   Returns     - void
   ======================================================================== */
void load_pal(char *n)
{
        FILE    *f;
        LONG    i;
        struct SREGS sregs;
        union REGS inregs, outregs;

        f=FileOpen(n,"rb");
        if(f==NULL)
                fatal_error("Palette not found: %s",n);
        fread(&CurPal[0],sizeof(char),8,f);             /* waste first 8 bytes */
        fread(&CurPal[0],sizeof(char),768,f);
        FileClose(f);

        for (i=0; i<256; i++)
        {
                CurPal[i].bRed = CurPal[i].bRed / 4;                    /* scale DAC range from 0-255 to 0-63 */
                CurPal[i].bGreen = CurPal[i].bGreen / 4;
                CurPal[i].bBlue = CurPal[i].bBlue / 4;
        }

        /* clear border */
        segread(&sregs);
        inregs.w.ax = 0x1001;
        inregs.w.bx = 0x0;
        int386x( 0x10, &inregs, &outregs, &sregs );

        /* set palette */
        set_pal((char *)&CurPal[0]);
}

/* ========================================================================
   Function    - init_pal
   Description - initializes the palette
   Returns     - void
   ======================================================================== */
void init_pal(char *PalName)
{
        char    path_buffer[80];

		sprintf(path_buffer, "%s%s.col", WAD_PATH, PalName);
        load_pal(path_buffer);
}

/* ====================================================================
   FadeOut                 - Fade to black
	==================================================================== */
void FadeOut (USHORT steps)
{
	UBYTE	CardPal[768];
	UBYTE	TempPal[768];
	SHORT	i, j;
	
	if (steps) /* if steps is not zero */
	{
		/* get current palette from card */
		outp(0x3c7, 0); /* set starting index */
		for (i=0; i<768; i++)
		CardPal[i] = inp(0x3c9);

		/* step through intensity levels */
		for (j=steps; j>=0; j--)
		{
			run_timers();
			for (i=0; i<768; i++)
				TempPal[i] = (CardPal[i] * j) / steps;
			run_timers();
			set_pal((char *)TempPal);
		}
	}
	else
	{
		memset((PTR)TempPal, 0, 768);
		set_pal((char *)TempPal);
	}

	// set state of flag
	fIsFadedOut = 4;		// was true
}

/* ====================================================================
   FadeIn                  - Fade from black to CurPal
	==================================================================== */
void FadeIn (USHORT steps)
{
	COLORSPEC	TempPal[256];
	USHORT		i, j;
	
	if (steps) /* if steps is not zero */
	{
		for (i=0; i<=steps; i++)
		{
			run_timers();
			for (j=0; j<256; j++)
			{
				TempPal[j].bRed		= (CurPal[j].bRed * i) / steps;
				TempPal[j].bGreen	= (CurPal[j].bGreen * i) / steps;
				TempPal[j].bBlue	= (CurPal[j].bBlue * i) / steps;
			}
			run_timers();
			set_pal((char *)TempPal);
		}
	}
	else
		set_pal((char *)CurPal);

	// set state of flag
	fIsFadedOut = FALSE;	
}

/* ========================================================================
   Function    - close_graph
   Description - closes the graphics system
   Returns     - void
   ======================================================================== */
void close_graph()
{
        _setvideomode(_TEXTC80);
}

/* ========================================================================
   Function    - update_screen
   Description - copies the display buffer into video ram
   Returns     - void
   ======================================================================== */
void __update_screen()
{
        if (graphics_mode == SVGA)
        {
//              if(render_height != MAX_VIEW_HEIGHT)
//              {
//                      ScreenCopy(0, margin_left, margin_top, render_width, render_height);
//              }
//              else
                {
                        set_svga_page(0);
                        memcpy(disp,screen,0x10000);
                        set_svga_page(0x10000L/svga_granularity);
                        memcpy(disp,screen+0x10000,0x10000);
                        set_svga_page(0x20000L/svga_granularity);
                        memcpy(disp,screen+0x20000,0x10000);
                        set_svga_page(0x30000L/svga_granularity);
                        memcpy(disp,screen+0x30000,0x10000);
                        set_svga_page(0x40000L/svga_granularity);
                        memcpy(disp,screen+0x40000,45056);
                }
        }
        else
                memcpy(disp,screen,64000);
}

/* =======================================================================
 *  Note: the following procedure utilizes an optimization technique often
 *  called "strip-mining", which is effective on any Harvard-architecture
 *  CPU.  It simply consists of processing large arrays in chunks small
 *  enough to fit into the Pentium L2 cache.  And since the 640 x 480
 *  screen buffer is too big to fit even in a 256k secondary cache, we
 *  not only minimize L2 misses, we eliminate double misses.  (ED)
 * ====================================================================== */
/* ========================================================================
   Function    - update_screen
   Description - copies the display buffer into video ram
   Returns     - void
   ======================================================================== */
void update_screen()
{
        if (graphics_mode == SVGA)
        {
              _SetSVGAPage(0);
              _DWordCopy(disp, screen, 0x800);
              _DWordCopy(disp + 0x2000, screen + 0x2000, 0x800);
              _DWordCopy(disp + 0x4000, screen + 0x4000, 0x800);
              _DWordCopy(disp + 0x6000, screen + 0x6000, 0x800);
              _DWordCopy(disp + 0x8000, screen + 0x8000, 0x800);
              _DWordCopy(disp + 0xa000, screen + 0xa000, 0x800);
              _DWordCopy(disp + 0xc000, screen + 0xc000, 0x800);
              _DWordCopy(disp + 0xe000, screen + 0xe000, 0x800);
              _CalcSetSVGAPage(0x10000L, svga_granularity);
              _DWordCopy(disp, screen + 0x10000, 0x800);
              _DWordCopy(disp + 0x2000, screen + 0x12000, 0x800);
              _DWordCopy(disp + 0x4000, screen + 0x14000, 0x800);
              _DWordCopy(disp + 0x6000, screen + 0x16000, 0x800);
              _DWordCopy(disp + 0x8000, screen + 0x18000, 0x800);
              _DWordCopy(disp + 0xa000, screen + 0x1a000, 0x800);
              _DWordCopy(disp + 0xc000, screen + 0x1c000, 0x800);
              _DWordCopy(disp + 0xe000, screen + 0x1e000, 0x800);
              _CalcSetSVGAPage(0x20000L, svga_granularity);
              _DWordCopy(disp, screen + 0x20000, 0x800);
              _DWordCopy(disp + 0x2000, screen + 0x22000, 0x800);
              _DWordCopy(disp + 0x4000, screen + 0x24000, 0x800);
              _DWordCopy(disp + 0x6000, screen + 0x26000, 0x800);
              _DWordCopy(disp + 0x8000, screen + 0x28000, 0x800);
              _DWordCopy(disp + 0xa000, screen + 0x2a000, 0x800);
              _DWordCopy(disp + 0xc000, screen + 0x2c000, 0x800);
              _DWordCopy(disp + 0xe000, screen + 0x2e000, 0x800);
              _CalcSetSVGAPage(0x30000L, svga_granularity);
              _DWordCopy(disp, screen + 0x30000, 0x800);
              _DWordCopy(disp + 0x2000, screen + 0x32000, 0x800);
              _DWordCopy(disp + 0x4000, screen + 0x34000, 0x800);
              _DWordCopy(disp + 0x6000, screen + 0x36000, 0x800);
              _DWordCopy(disp + 0x8000, screen + 0x38000, 0x800);
              _DWordCopy(disp + 0xa000, screen + 0x3a000, 0x800);
              _DWordCopy(disp + 0xc000, screen + 0x3c000, 0x800);
              _DWordCopy(disp + 0xe000, screen + 0x3e000, 0x800);
              _CalcSetSVGAPage(0x40000L, svga_granularity);
              _DWordCopy(disp, screen + 0x40000, 0x800);
              _DWordCopy(disp + 0x2000, screen + 0x42000, 0x800);
              _DWordCopy(disp + 0x4000, screen + 0x44000, 0x800);
              _DWordCopy(disp + 0x6000, screen + 0x46000, 0x800);
              _DWordCopy(disp + 0x8000, screen + 0x48000, 0x800);
              _DWordCopy(disp + 0xa000, screen + 0x4a000, 0x400);
        }
        else
              _DWordCopy(disp, screen, 16000);
}
/* ========================================================================
   Function    - clear_screen
   Description - clears the screen
   Returns     - void
   ======================================================================== */
void clear_screen()
{
        memset(screen,0,MAX_VIEW_WIDTH*MAX_VIEW_HEIGHT);
}

/* ========================================================================
   Function    - clear_screen_to
   Description - like clear screen, but lets you specify the clear color.
						Purpose: solve the "mouse droppings" problem at the
						edge of the advisors screen and the adventure screen
						by clearing screen to 1 instead of 0.
						[d3-14-97 JPC]
   Returns     - void
   ======================================================================== */
void clear_screen_to(int c)
{
        memset(screen,c,MAX_VIEW_WIDTH*MAX_VIEW_HEIGHT);
}

/* ========================================================================
   Function    - clear_display
   Description - clears the video screen
   Returns     - void
   ======================================================================== */
void clear_display()
{
        if (graphics_mode == SVGA)
        {
                set_svga_page(0);
                memset(disp,BLACK,0x10000);
                set_svga_page(0x10000L/svga_granularity);
                memset(disp,BLACK,0x10000);
                set_svga_page(0x20000L/svga_granularity);
                memset(disp,BLACK,0x10000);
                set_svga_page(0x30000L/svga_granularity);
                memset(disp,BLACK,0x10000);
                set_svga_page(0x40000L/svga_granularity);
                memset(disp,BLACK,45056);
        }
        else
                memcpy(disp,screen,64000);

        memset(screen,0,MAX_VIEW_WIDTH*MAX_VIEW_HEIGHT);
}

/* ========================================================================
   Function    - ScreenCopy
   Description - copy bitmap data directly to the video card
   Returns     -
   ======================================================================== */
void ScreenCopy (
        SHORT   iSrcBitm,
        SHORT   xSrc,
        SHORT   ySrc,
        SHORT   width,
        SHORT   height,
		  SHORT 	 resolution					// [d10-26-96 JPC] new
        )
{
        SHORT   xDest = xSrc;
        SHORT   yDest = ySrc;
        USHORT  i;
        //UBYTE j;
        PTR             pSrcBitm;
        PTR             pDestBitm;
        USHORT  iPage;
        SHORT   wSrc;
        SHORT   hSrc;
        SHORT   wDest;
        SHORT   hDest;
        SHORT   cbSrc = 0;
        ULONG   cbSrcOffset = 0;
        SHORT   cbDest;
        ULONG   cbDestOffset;
        //ULONG l;

/* -----------------------------------------------------------*/

        if (iSrcBitm == fERROR )
                return;

        if(iSrcBitm == 0)
                pSrcBitm = screen;
        else
                pSrcBitm = BLKPTR(iSrcBitm);

        pDestBitm = disp;

/* -----------------------------------------------------------*/
/* clip source */
/* -----------------------------------------------------------*/
        wSrc = MAX_VIEW_WIDTH;
        hSrc = MAX_VIEW_HEIGHT;
        cbSrc = wSrc;

        if (xSrc < 0)                                           /* clip source */
        {
                xDest -= xSrc;
                width += xSrc;
                xSrc = 0;
        }
        if (xSrc > wSrc)
                goto Exit;
        if (xSrc+width > wSrc)
                width = wSrc - xSrc;
        if (width <= 0)
                goto Exit;

        if (ySrc < 0)
        {
                yDest -= ySrc;
                height += ySrc;
                ySrc = 0;
        }
        if (ySrc > hSrc)
                goto Exit;
        if (ySrc+height > hSrc)
                height = hSrc - ySrc;
        if (height <= 0)
                goto Exit;

/* -----------------------------------------------------------*/
/* clip destination */
/* -----------------------------------------------------------*/
        wDest = MAX_VIEW_WIDTH;
        hDest = MAX_VIEW_HEIGHT;
        cbDest = wDest;

/* -----------------------------------------------------------*/
/* set initial offsets into source and dest bitmaps */
/* -----------------------------------------------------------*/
        cbSrcOffset = (ySrc * cbSrc) + xSrc;
        cbDestOffset = (yDest * cbDest) + xDest;

        iPage = (USHORT)(cbDestOffset / svga_granularity);
        cbDestOffset -= ((ULONG)iPage * svga_granularity);
        set_svga_page(iPage);

/* -----------------------------------------------------------*/
/* Low granularity video cards */
/* -----------------------------------------------------------*/
        if (svga_granularity < 0x10000)
        {
                /* COPY memory to display, copy does not consider transparency */
                do
                {
                        memcpy(pDestBitm+cbDestOffset,pSrcBitm+cbSrcOffset,width);
                        cbSrcOffset += cbSrc;
                        cbDestOffset += cbDest;
                        if (cbDestOffset > 64000)
                        {
                                i = (USHORT)(cbDestOffset / svga_granularity);
                                iPage += i;
                                cbDestOffset -= ((ULONG)i * svga_granularity);
                                set_svga_page(iPage);
                        }
                } while (--height);
        }
/* -----------------------------------------------------------*/
/* High granularity video cards */
/* -----------------------------------------------------------*/
        else
        {
                /* COPY does not consider transparency */
                do
                {
                        if (cbDestOffset+width >= 0x10000)
                        {
                                ULONG l = 0x10000 - cbDestOffset;
                                memcpy(pDestBitm+cbDestOffset,pSrcBitm+cbSrcOffset,(USHORT)l);
                                set_svga_page(++iPage);
                                memcpy(pDestBitm,pSrcBitm+cbSrcOffset+l,width-(USHORT)l);
                                cbDestOffset = (cbDestOffset + cbDest) - 0x10000;
                                cbSrcOffset += cbSrc;
                        }
                        else
                        {
                                memcpy(pDestBitm+cbDestOffset,pSrcBitm+cbSrcOffset,width);
                                cbSrcOffset += cbSrc;
                                cbDestOffset += cbDest;
                                if (cbDestOffset >= 0x10000)
                                {
                                        cbDestOffset -= 0x10000;
                                        set_svga_page(++iPage);
                                }
                        }
                } while (--height);
        }

/* -----------------------------------------------------------*/
Exit:
        i++;
}


/* -----------------------------------------------------------*/
/* ========================================================================
   Function    - NewScreenCopy
   Description - copy bitmap data directly to the video card
   Returns     -
   ======================================================================== */
void NewScreenCopy (
        SHORT   iSrcBitm,
        SHORT   xSrc,
        SHORT   ySrc,
        SHORT   width,
        SHORT   height
        )
{
        SHORT   xDest = xSrc;
        SHORT   yDest = ySrc;
        //USHORT  i;
        PTR     pSrcBitm;
        PTR     pDestBitm;
        //USHORT  iPage;
        SHORT   wSrc;
        SHORT   hSrc;
        SHORT   wDest;
        SHORT   hDest;
        SHORT   cbSrc = 0;
        ULONG   cbSrcOffset = 0;
        SHORT   cbDest;
        ULONG   cbDestOffset;

/* -----------------------------------------------------------*/

        if (iSrcBitm == fERROR )
                return;

        if(iSrcBitm == 0)
                pSrcBitm = screen;
        else
                pSrcBitm = BLKPTR(iSrcBitm);

        pDestBitm = disp;

/* -----------------------------------------------------------*/
/* clip source */
/* -----------------------------------------------------------*/
        wSrc = MAX_VIEW_WIDTH;
        hSrc = MAX_VIEW_HEIGHT;
        cbSrc = wSrc;

        if (xSrc < 0)                                           /* clip source */
        {
                xDest -= xSrc;
                width += xSrc;
                xSrc = 0;
        }
        if (xSrc > wSrc)
                return;
        if (xSrc + width > wSrc)
                width = wSrc - xSrc;
        if (width <= 0)
                return;

        if (ySrc < 0)
        {
                yDest -= ySrc;
                height += ySrc;
                ySrc = 0;
        }
        if (ySrc > hSrc)
                return;
        if (ySrc + height > hSrc)
                height = hSrc - ySrc;
        if (height <= 0)
                return;

/* -----------------------------------------------------------*/
/* clip source */
/* -----------------------------------------------------------*/
        wSrc = MAX_VIEW_WIDTH;
        hSrc = MAX_VIEW_HEIGHT;
        cbSrc = wSrc;

        if (xSrc < 0)                                           /* clip source */
        {
                xDest -= xSrc;
                width += xSrc;
                xSrc = 0;
        }
        if (xSrc > wSrc)
                return;
        if (xSrc + width > wSrc)
                width = wSrc - xSrc;
        if (width <= 0)
                return;

        if (ySrc < 0)
        {
                yDest -= ySrc;
                height += ySrc;
                ySrc = 0;
        }
        if (ySrc > hSrc)
                return;
        if (ySrc + height > hSrc)
                height = hSrc - ySrc;
        if (height <= 0)
                return;

/* -----------------------------------------------------------*/
/* clip destination */
/* -----------------------------------------------------------*/
        wDest = MAX_VIEW_WIDTH;
        hDest = MAX_VIEW_HEIGHT;
        cbDest = wDest;

/* -----------------------------------------------------------*/
/* set initial offsets into source and dest bitmaps */
/* -----------------------------------------------------------*/
        cbSrcOffset = (ySrc * cbSrc) + xSrc;
        cbDestOffset = (yDest * cbDest) + xDest;

        _iPage = (USHORT)(cbDestOffset / svga_granularity);
        cbDestOffset -= ((ULONG)_iPage * svga_granularity);

        _SetSVGAPage(_iPage);

/* -----------------------------------------------------------*/
/* Low granularity video cards */
/* -----------------------------------------------------------*/
        if (svga_granularity < 0x10000)
        {
                /* COPY memory to display, copy does not consider transparency */
                do
                {
                        _CopyLine(pSrcBitm, pDestBitm, cbSrcOffset, cbDestOffset, width);
                        cbSrcOffset += cbSrc;
                        cbDestOffset += cbDest;
                        if (cbDestOffset > 64000)
                        {
                                cbDestOffset = _UpdateOffsetAndPage(cbDestOffset, svga_granularity, _iPage);
						}
                } while (--height);
        }
/* -----------------------------------------------------------*/
/* High granularity video cards */
/* -----------------------------------------------------------*/
        else
        {
                /* COPY does not consider transparency */
                do
                {
                        if (cbDestOffset + width >= 0x10000)
                        {
                                _CopySplitLine(pSrcBitm, pDestBitm, cbSrcOffset, cbDestOffset, width);
                                cbDestOffset = (cbDestOffset + cbDest) - 0x10000;
                                cbSrcOffset += cbSrc;
                        }
                        else
                        {
                                _CopyLine(pSrcBitm, pDestBitm, cbSrcOffset, cbDestOffset, width);
                                cbSrcOffset += cbSrc;
                                cbDestOffset += cbDest;
                                if (cbDestOffset >= 0x10000)
                                {
                                        _SetSVGAPage(++_iPage);
                                }
						}
                } while (--height);
        }
}

/* ======================================================================== */

