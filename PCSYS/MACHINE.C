/* ========================================================================
   Copyright (c) 1990,1995   Synergistic Software
   All Rights Reserved.
   =======================================================================
   Filename: MACHINE.C   -Handles machine specific things
   Author: Chris Phillips
   ========================================================================
   Contains the following internal functions:

   Contains the following general functions:
   main                  -initializes the system and runs GameMain
   quit_sys              -de-initializes the system, and exits the program

   ======================================================================== */
/* ------------------------------------------------------------------------
   Includes
   ------------------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <time.h>
#include <conio.h>
#include <i86.h>
#include <bios.h>
#include "string.h"
#include "system.h"
#include "engine.h"
#include "engint.h"
#include "machine.h"
#include "machint.h"
#include "fileutil.h"
#include "main.hxx"
#include "sound.hxx"
/* ------------------------------------------------------------------------
   Defines and Compile Flags
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Macros
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Prototypes
   ------------------------------------------------------------------------ */
static void print_usage();
void (__interrupt __far *prev_int_08)();
void __interrupt __far timer_routine (void);

/* ------------------------------------------------------------------------
   Global Variables
   ------------------------------------------------------------------------ */
extern char pInitScene[];
extern char pwad[];
LONG	fps;
BOOL fQuitting = FALSE;

int iChatKey;

BOOL JustLoadWad=FALSE;

#if !defined(_RELEASE)
BOOL gDontFight = FALSE;
BOOL gfMakeResScript=FALSE;
#endif

static PFV NewTimerProc;
int InitRedBook(void);
void CheckCD(void);
void SetInt24(void);
void ClearInt24(void);

BOOL CheckForMouse ();						// [d4-17-97 JPC] defined in MOUSE.C

/* =======================================================================
   Function    - main
   Description - initializes the system and calls GameMain
   Returns     - void
   ======================================================================== */
int main(int argc,char * argv[])
{
	FILE *fp;
	char cpBuffer[256];
	LONG fileResult;
	// UNUSED char path[256];


		// Initialize any memory, time, etc statistics
		// These will be calced and printed/dumped in
		// close_statistics <see statistics.c>
	// moved to winsys\machine.c

	if (!CheckForMouse ())
	{
		puts ("You must have a mouse to run Birthright.  No mouse was detected.");
		return 1;								// return to DOS, errorlevel 1
	}

	fp=fopen("brsetup.cfg","r");
	if(fp==NULL)
	{
			fatal_error("Unable to open brsetup.cfg file.  Game not properly installed.");
			return FALSE;
	}
	InstallationType = 2;  // set to English as default 
	do
	{
		fileResult = GetNextLine(fp,cpBuffer,sizeof(cpBuffer));
		if(fileResult != EOF)
		{
			if(strcmp(cpBuffer,"[INSTALL_TYPE]") == 0)
			{
				fileResult = GetNextLine(fp,cpBuffer,sizeof(cpBuffer));
				InstallationType = atoi(cpBuffer);
			}
			else if(strcmp(cpBuffer,"[CD]") == 0)
			{
				fileResult = GetNextLine(fp,cpBuffer,sizeof(cpBuffer));
				strcpy(CDDrive,cpBuffer);
				strcat(CDDrive,":\\");
			}
//			else if(strcmp(cpBuffer,"[INSTALL_PATH]") == 0)
//				sprintf(InstallPath,".\\");
		}
	}while(fileResult != EOF);

// FOREIGN LANGUAGES ARE NOT IN SEPERATE SUBDIRECTORIES [ABC] 9/23/97
//	switch(InstallationType)
//	{
//		case 2:
			strcpy(InstallPath,".\\");
//			break;
//		case 1:
//			strcpy(InstallPath,"french\\");
//			break;
//		case 0:
//			strcpy(InstallPath,"german\\");
//			break;
//		default:
//			strcpy(InstallPath,".\\");
//			break;
//	}

	if(fp != NULL)
		fclose(fp);

	//Parse the command line
	parse_cmdline(argc,argv);
	init_statistics();

#if defined (_RELEASE)
	SetInt24();
	CheckCD();
	ClearInt24();
#endif

	// Install a kbrd handler.. check_regions will call key_status
	// on PC this uses key_stat array. On PC keyint will fill in array
	// automaticly so it doesnt hafta be done durring machine_pre_frame
	install_keyint();

	CheckHandles ();							// in FOPEN.C

	// start up the redbook player - [MDB] 11/04/96
	// this call will set the make sure you can talk to the CD player
	InitRedBook();

	//InitializeSoundSystem();

	GameMain();
	
	return EXIT_SUCCESS;
}

/* =======================================================================
   Function    - quit_sys
   Description - prints stats and removes the keyboard int, and shuts down
   Returns     - void
   ======================================================================== */
void quit_sys(LONG dummy)
{
	close_graph();
	close_statistics();
	// Be Sure to shut down sound system before distroy'ing the heap.
	TurnOffAllSounds();
	ShutDownSoundSystem();
	QuitMemManag();
	remove_keyint();
	StopRedBook();
	CloseAllFiles();
	exit(0);
}


#if 01
#if defined (_WADBUILDERS)
/* =======================================================================
   Function    - ProcessResponseFile
   Description - reads and parses the specified response file
   Returns     - void
	Note			- required by new version of DCK.
   ======================================================================== */
void ProcessResponseFile (char * szArgument)
{

	FILE			*fp;
	char 			cpBuffer[256];
	char *		szWadname;
	LONG 			fileResult;

	if (szArgument[0] == '@')
		szArgument++;
	fp = fopen (szArgument, "r");

	if (fp == NULL)
	{
#if !defined(_RELEASE)
		printf ("Unable to open response file\n");
#endif
		return;
	}

#if !defined(_RELEASE)
		printf ("Opened response file %s\n", szArgument);
#endif

	// Ignore all but the "-file" line.
	while (1)
	{
		fileResult = GetNextLine(fp,cpBuffer,sizeof(cpBuffer));
		if (fileResult == EOF)
			break;
		if (strnicmp (cpBuffer, "-file", 5) == 0)
		{
			szWadname = cpBuffer + 5;
			while (*szWadname == ' ' || *szWadname == '\t')
				szWadname++;
			sprintf (pwad, szWadname);
			JustLoadWad = TRUE;
#if !defined(_RELEASE)
			printf ("Found -file line, WAD = %s\n", pwad);
#endif

		}
	}
}
#endif

/* =======================================================================
   Function    - parse_cmdline
   Description - reads and parses the command line
   Returns     - void
   ======================================================================== */
void parse_cmdline (int argc,char * argv[])
{
	//int e,l;
	int num = 1;

	while (num < argc)
	{
		switch(argv[num++][0])
		{
			case 's':
			case 'S':
				sprintf(pInitScene, argv[num++]);
				JustLoadWad=FALSE;
				break;
			case 'w':
			case 'W':
				sprintf(pwad,argv[num++]);
				JustLoadWad=TRUE;
				break;
			case 'l':
			case 'L':
				fLowMemory=TRUE;
				break;

#if defined (_WADBUILDERS)
			// [d4-07-97 JPC] Added this case for DCK 3.62.
			case '@':
				if (strlen (argv[num-1]) == 1)
					ProcessResponseFile (argv[num]);
				else
					ProcessResponseFile (argv[num-1]);
				break;
#endif

#if !defined (_RELEASE)
			case 'i':
			case 'I':
				gDontFight = TRUE;
				break;
			case 'g':
			case 'G':
				sprintf(pwad, argv[num++]);
				gfMakeResScript = TRUE;
				break;
#endif

			default:
#if !defined (_RELEASE)
				print_usage();
#endif
				exit(0);
		}
	}
}

/* ========================================================================
   Function    - AppIdle
   Description - calls MainLoop, used by dialogs
   Returns     -
   ======================================================================== */
void AppIdle(void)
{
	MainLoop();
}

/* =======================================================================
   Function    - print_usage
   Description - displays command line options
   Returns     - void
   ======================================================================== */
static void print_usage()
{
	printf("Usage:NOVA <switches>\n");
	printf("   Switches:\n");
	printf("      w WADFILE.WAD    Wad file to use, i.e. castle86.wad\n");
	printf("      s SCENEFILE      Scene file to use, i.e. castle\n");
	printf("      l                Assume low memory condition\n");
	printf("      i                Don't attack the adventurers\n");
	printf("      g WADFILE.WAD    Generate a resource script for WAD file\n");
#if defined (_WADBUILDERS)
	printf("      @respfile.rsp    Read switches from DCK response file\n");
#endif
}
#endif
void UpdateFPS(void)
{
	static LONG lastTime = 0;
	static LONG curTime = 0;
	static LONG frames = 0;

	curTime = clock();
	frames++;

	if(curTime - CLOCKS_PER_SEC > lastTime)
	{
		fps = frames;
		lastTime = curTime;
		frames = 0;
	}

} // UpdateFPS

/* ========================================================================
   Function    - StartTimerThread
   Description - start the int8 timer calling the routine passed in
   Returns     - void
   ======================================================================== */
void StartTimerThread(PFV func)
{
	NewTimerProc = func;
	prev_int_08 = _dos_getvect( 0x08 );
	_dos_setvect( 0x08, timer_routine );
}

/* ========================================================================
   Function    - StopTimerThread
   Description - halt and restore the int8
   Returns     - void
   ======================================================================== */
void StopTimerThread(void)
{
	_dos_setvect( 0x08, prev_int_08 );
}

/* =======================================================================
   Function    - timer_routine
   Description - the new timer interrupt
   Returns     - void
   ======================================================================== */
void __interrupt __far timer_routine (void)
{
	static BOOL bInProcess = FALSE;
	
	if(bInProcess)
		return;
	
	bInProcess = TRUE;	
	
	(*prev_int_08)();
	(*NewTimerProc)();
	
	bInProcess = FALSE;	
	
}

/* ======================================================================= */


