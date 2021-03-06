
/* ========================================================================
   Copyright (c) 1990,1996	Synergistic Software
   All Rights Reserved
   ======================================================================== */
#ifndef _MAIN_H
#define _MAIN_H

/* ------------------------------------------------------------------------
   Sub Includes
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Defines and Compile Flags
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Enums
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Typedefs
   ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------
   Macros   
   ------------------------------------------------------------------------ */
/* -----------------------------------------------------------------
   Globals
   ----------------------------------------------------------------- */

extern SHORT	gUnitStackSize;

/* ------------------------------------------------------------------------
   Prototypes
   ------------------------------------------------------------------------ */
void toggleminiview(LONG);
void AddGameKeys(void);

#if defined (__cplusplus)
extern "C" {
#endif

/* from main.cpp */
void ShowMainMenu(LONG, LONG);
void HideMainMenu(LONG, LONG);
void GameMain(void);
void MainLoop(void);
void DirectDrawPreFrame		( void );
void DirectDrawSaveScreen	( void );
void AddGlobalKeys(void);

#if defined (__cplusplus)
}
#endif

#endif
