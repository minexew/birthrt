/* ========================================================================
   Copyright (c) 1990,1995   Synergistic Software
   All Rights Reserved.
   =======================================================================
   Filename: KEYINT.C    -Handles keyboard input and installing the interrupt
   Author: Chris Phillips
   ========================================================================
   Contains the following internal functions: 
   key_rtn               -used as our new keyboard interrupt

   Contains the following general functions:
   install_keyint        -installs the new interrupt
   remove_keyint         -reinstalls the old interrupt, removes the new one
   keys_down             -checks to see if any keys are down
   key_status            -returns the status of a key
   
   ======================================================================== */
/* ------------------------------------------------------------------------
   Includes
   ------------------------------------------------------------------------ */
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include "system.h"
#include "machine.h"
#include "machint.h"

/* ------------------------------------------------------------------------
   Defines and Compile Flags
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Macros
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Prototypes
   ------------------------------------------------------------------------ */
void (__interrupt __far *prev_int_09)();

/* ------------------------------------------------------------------------
   Global Variables
   ------------------------------------------------------------------------ */
volatile unsigned char key_stat[256];
volatile BOOL	fAnyKeyChanged;
int keyint_installed=FALSE;
#ifdef _FRENCHVER
static char asciiTable[58] = {
/* NO_KEY			*/   0,
/* KEY_ESCAPE		*/   0,
/* KEY_1				*/ '1',
/* KEY_2				*/ '2',
/* KEY_3				*/ '3',
/* KEY_4				*/ '4',
/* KEY_5				*/ '5',
/* KEY_6				*/ '6',
/* KEY_7				*/ '7',
/* KEY_8				*/ '8',
/* KEY_9				*/ '9',
/* KEY_0				*/ '0',
/* KEY_MINUS		*/   0,
/* KEY_EQUAL		*/   0,
/* KEY_BACKSPACE	*/   8,
/* KEY_TAB 			*/   0,
/* KEY_Q				*/ 'A',
/* KEY_W				*/ 'Z',
/* KEY_E				*/ 'E',
/* KEY_R				*/ 'R',
/* KEY_T				*/ 'T',
/* KEY_Y				*/ 'Y',
/* KEY_U				*/ 'U',
/* KEY_I				*/ 'I',
/* KEY_O				*/ 'O',
/* KEY_P				*/ 'P',
/* KEY_LEFTSQUARE	*/   0,
/* KEY_RIGHTSQUARE*/   0,
/* KEY_ENTER		*/  13,
/* KEY_CONTROL		*/   0,
/* KEY_A				*/ 'Q',
/* KEY_S				*/ 'S',
/* KEY_D				*/ 'D',
/* KEY_F				*/ 'F',
/* KEY_G				*/ 'G',
/* KEY_H				*/ 'H',
/* KEY_J				*/ 'J',
/* KEY_K				*/ 'K',
/* KEY_L				*/ 'L',
/* KEY_SEMICOLON	*/ 'M',
/* KEY_QUOTE		*/   0,
/* KEY_BACKQUOTE	*/   0,
/* KEY_LEFTSHIFT	*/   0,
/* KEY_BACKSLASH	*/   0,
/* KEY_Z				*/ 'W',
/* KEY_X				*/ 'X',
/* KEY_C				*/ 'C',
/* KEY_V				*/ 'V',
/* KEY_B				*/ 'B',
/* KEY_N				*/ 'N',
/* KEY_M				*/   0,
/* KEY_COMMA		*/   0,
/* KEY_PERIOD		*/   0,
/* KEY_SLASH		*/   0,
/* KEY_RIGHTSHIFT	*/   0,
/* KEY_PRINTSCREEN*/   0,
/* KEY_ALT 			*/   0,
/* KEY_SPACEBAR	*/ ' ',
};
#else
static char asciiTable[58] = {
/* NO_KEY			*/   0,
/* KEY_ESCAPE		*/   0,
/* KEY_1				*/ '1',
/* KEY_2				*/ '2',
/* KEY_3				*/ '3',
/* KEY_4				*/ '4',
/* KEY_5				*/ '5',
/* KEY_6				*/ '6',
/* KEY_7				*/ '7',
/* KEY_8				*/ '8',
/* KEY_9				*/ '9',
/* KEY_0				*/ '0',
/* KEY_MINUS		*/   0,
/* KEY_EQUAL		*/   0,
/* KEY_BACKSPACE	*/   8,
/* KEY_TAB 			*/   0,
/* KEY_Q				*/ 'Q',
/* KEY_W				*/ 'W',
/* KEY_E				*/ 'E',
/* KEY_R				*/ 'R',
/* KEY_T				*/ 'T',
/* KEY_Y				*/ 'Y',
/* KEY_U				*/ 'U',
/* KEY_I				*/ 'I',
/* KEY_O				*/ 'O',
/* KEY_P				*/ 'P',
/* KEY_LEFTSQUARE	*/   0,
/* KEY_RIGHTSQUARE*/   0,
/* KEY_ENTER		*/  13,
/* KEY_CONTROL		*/   0,
/* KEY_A				*/ 'A',
/* KEY_S				*/ 'S',
/* KEY_D				*/ 'D',
/* KEY_F				*/ 'F',
/* KEY_G				*/ 'G',
/* KEY_H				*/ 'H',
/* KEY_J				*/ 'J',
/* KEY_K				*/ 'K',
/* KEY_L				*/ 'L',
/* KEY_SEMICOLON	*/   0,
/* KEY_QUOTE		*/   0,
/* KEY_BACKQUOTE	*/   0,
/* KEY_LEFTSHIFT	*/   0,
/* KEY_BACKSLASH	*/   0,
/* KEY_Z				*/ 'Z',
/* KEY_X				*/ 'X',
/* KEY_C				*/ 'C',
/* KEY_V				*/ 'V',
/* KEY_B				*/ 'B',
/* KEY_N				*/ 'N',
/* KEY_M				*/ 'M',
/* KEY_COMMA		*/   0,
/* KEY_PERIOD		*/   0,
/* KEY_SLASH		*/   0,
/* KEY_RIGHTSHIFT	*/   0,
/* KEY_PRINTSCREEN*/   0,
/* KEY_ALT 			*/   0,
/* KEY_SPACEBAR	*/ ' ',
};
#endif

extern BOOL bIntroMode;	// for intro stuff
extern BOOL bShowBanner;
extern int iChatKey;


/* =======================================================================
   Function    - key_rtn
   Description - the new keyboard interrupt
   Returns     - void
   ======================================================================== */
void __interrupt __far key_rtn (void)
{
	unsigned a,b;
	//char a,b,c;
	char c;

#ifdef _DEBUG
	static char CtrlDown = 0;
	union REGS regs;
#endif

	a=inp(0x60);		// get the scan code
	b=inp(0x61);		// get the control port

	outp(0x61,b|0x80);	// clear the key
	outp(0x61,b);
	outp(0x20,0x20);	// clear the interrupt
	
	c = a & 0x7f;		// mask off key up bit to get pure scan code
	
	if(a&0x80)			// if key up bit is on
	{
		key_stat[c] &= 0x3F;					/* key up, clear prohibit */
		fAnyKeyChanged = TRUE;
	}
	else
	{
		if (!(key_stat[c] & 0x80))			/* if not prohibit */
		{
			fAnyKeyChanged = TRUE;
			key_stat[c] = 0x81;				/* set key press and prohibit */
			iChatKey = 0;

			if(c < 58)
				iChatKey = (int)asciiTable[c];
		}
		key_stat[c] |= 0x40;					/* key down */
	}
	
#ifdef _DEBUG	
	// debug break into the debugger code
	if(a == 0x01D)		// control key pressed
	{
		CtrlDown = 1;
	}
	else
	if(a == 0x09D)		// control key released
	{
		CtrlDown = 0;
	}
	else
	if(CtrlDown && a == 0x02e)	// pass on control-c
	{
		//int386(0x1B, &regs, &regs);	// generate ctrl-break
		int386(0x03, &regs, &regs);	// generate debug interrupt
		return;
	}
#endif
}

/* =======================================================================
   Function    - install_keyint 
   Description - installs the keyboard interrupt
   Returns     - void
   ======================================================================== */
void install_keyint()
{
	LONG i;

	for(i=0;i<256;i++)
		key_stat[i]=0;
	prev_int_09 = _dos_getvect( 0x09 );
	_dos_setvect( 0x09, key_rtn );
	keyint_installed=TRUE;
}

/* =======================================================================
   Function    - remove_keyint
   Description - removes the installed keyboard interrupt
   Returns     - void
   ======================================================================== */
void remove_keyint()
{
	if(keyint_installed)
		_dos_setvect( 0x09, prev_int_09 );
	keyint_installed=FALSE;
}

/* =======================================================================
   Function    - keys_down
   Description - scans the key_stat array to see if any keys are being pressed
   Returns     - TRUE if there are any keys down, false otherwise
   ======================================================================== */
LONG keys_down()
{
	LONG i;

	for(i=0;i<256;i++)
		{
		if(key_stat[i])
			return(TRUE);
		}
	return(FALSE);
}

/* =======================================================================
   Function    - key_status
   Description - returns the status of a specified key
   Returns     - the status of the specified key
   ======================================================================== */
LONG key_status(LONG k)
{
	LONG	rv;

	rv = key_stat[k] & 0x3F;
	key_stat[k] &= 0xC0;			/* clear key press */

	return rv;
}


LONG async_key_status(LONG k)
{
	return(key_stat[k] & 0x40);
}

#pragma off (unreferenced)
void clear_key_status(LONG unused)
{
	memset((void *)&key_stat,0,256);
}
#pragma on (unreferenced)

/*	======================================================================== */

