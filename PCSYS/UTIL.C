/* ========================================================================
   Copyright (c) 1990,1995   Synergistic Software
   All Rights Reserved.
   =======================================================================
   Filename: UTIL.C      -Low Level Utitilies
   Author: Chris Phillips
   ========================================================================
   Contains the following internal functions: 

   Contains the following general functions:
   get_free_mem          -gets the largest free block of memory
   safe_malloc           -a "safer" malloc
   fatal_error           -aborts the game, writes and displays error message
   get_time              -returns the time
   pause                 -pauses for a specified length of time

   ======================================================================== */
/* ------------------------------------------------------------------------
   Includes
   ------------------------------------------------------------------------ */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include <i86.h>
#include <bios.h>
#include <conio.h>
#include "system.h"
#include "machine.h"
#include "machint.h"

/* ------------------------------------------------------------------------
   Defines and Compile Flags
   ------------------------------------------------------------------------ */
#if 0
struct meminfo {
    unsigned LargestBlockAvail;
    unsigned MaxUnlockedPage;
    unsigned LargestLockablePage;
    unsigned LinAddrSpace;
    unsigned NumFreePagesAvail;
    unsigned NumPhysicalPagesFree;
    unsigned TotalPhysicalPages;
    unsigned FreeLinAddrSpace;
    unsigned SizeOfPageFile;
    unsigned Reserved[3];
} MemInfo;
#endif

/* ------------------------------------------------------------------------
   Macros
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Prototypes
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Global Variables
   ------------------------------------------------------------------------ */

#if 0
/* ========================================================================
   Function    - get_free_mem
   Description - gets the largest block of free memory
   Returns     - the size of the block
   ======================================================================== */
ULONG get_free_mem()
{
	union REGS regs;
	struct SREGS sregs;

	regs.x.eax = 0x00000500;
	memset( &sregs, 0, sizeof(sregs) );
	sregs.es = FP_SEG( &MemInfo );
	regs.x.edi = FP_OFF( &MemInfo );

	int386x( DPMI_INT, &regs, &regs, &sregs );
	return( MemInfo.LargestBlockAvail );
}
#endif

/* ========================================================================
   Function    - safe_malloc
   Description - mallocs and aborts if malloc returns NULL
   Returns     - the pointer to the new block of memory
   ======================================================================== */
char *safe_malloc(LONG size)
{
	char * x;

	x = (char *)malloc(size);

	if(x==NULL)
		fatal_error("Cant Malloc memory in safe_malloc");
	return(x);
}

/* ========================================================================
   Function    - fatal_error
   Description - aborts the game, displaying and writing to disk an error msg
   Returns     - void
   ======================================================================== */
void fatal_error(const char *format, ...)
{
	char texbuffer[200];
	FILE *f;
	va_list argp;

	f=fopen("Fatal.err","wt");
	va_start(argp, format);
	vsprintf(texbuffer,format,argp);
	close_graph();
	remove_keyint();
	printf("Fatal Error:%s\n",texbuffer);
	fprintf(f,"Fatal Error:%s\n",texbuffer);
	fclose(f);
	
#if defined(_DEBUG)
	// if debuggable, crash out so the debugger will catch us 
	// still live in the code
	{
	volatile SHORT i = 1;
	i /= 0;
	}
#endif
	
	exit(0);
}

#if 0
	// Replaced with a Macro.
/* ========================================================================
   Function    - get_time
   Description - gets the time
   Returns     - the time
   ======================================================================== */
LONG get_time()
{
	LONG time;
	
	//_bios_timeofday(_TIME_GETCLOCK,&time);
	// Read the time directly, "We don't need no d*mm fn() calls!"
	time = * (volatile LONG *)(0x46c);
	return(time);
}
#endif

/* ========================================================================
   Function    - pause
   Description - pauses the game
   Returns     - void
   ======================================================================== */
void pause(LONG t)
{
	LONG end,now;

	//I belive a "tick" is 1/18 of a second

	end=get_time()+t;
	now=0;
	while(get_time()<end);

}

/* ======================================================================== */


