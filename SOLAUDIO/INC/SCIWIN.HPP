#if !defined(SCIWIN_HPP)
#define SCIWIN_HPP

#ifndef ___WIN386_INCLUDED__
#include "winodef.hpp"
#include <windows.h>
#endif

#define NUMMIDIBUFFS		10

extern	HWND		hMyWnd;
extern	HDC		hMyDC;
extern	HINSTANCE	MyInstance;
extern	HPALETTE	hPal, hOldPal;
extern	HCURSOR	hMyCur;
extern	HCURSOR	hAltCur;
extern	HCURSOR	hCurCur;
extern	HCURSOR	hNullCur;
extern	int		gameWidth, gameHeight;
extern	int		gameBorderX;
extern	int		gameBorderY;
extern	int		vmapWidth;
extern	int		vmapHeight;
extern	BOOL		LOCKMAIN;
extern	BOOL		LOCKAUDIO;
extern	BOOL		SHOWMEM;
extern	BOOL		SHOWNEWS;
extern	BOOL		USEAUDIO;
extern	BOOL		USEMIDI;
extern	BOOL		DOPPCTEST;
extern	BOOL		TRUECOLOR;
extern	BOOL		COLORCURSOR;
extern	BOOL		enableMidi;
extern	BOOL		haveFocus;
extern	int		forcePPC;
extern	int		reserveK;
extern	int		sciMinK;
extern	int		sciMaxK;
extern	int		DACSize;
extern	int		cursorViewAdd;
extern	int		maxCursorSize;
extern	int		asyncCheckFreq;
extern	int		nColors;
extern	char		szWinPatchDir[];
extern	char		szIniFile[];
extern	char		szInfo[];
extern	char		szResPath[];
extern	DWORD		midiBuffSize[];

BOOL	GetTFOption(LPSTR optionName, BOOL defaultVal);

#endif

