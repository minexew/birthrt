/* SCIWIN.C -- Windows version of SCI */

#define INCLUDE_MMSYSTEM_H
#define INCLUDE_COMMDLG_H
#include "windows.h"
#include "mmsystem.h"
#include "skel.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define	SAMPLE_NUM		100
#define	SAMPLE_NUM2		200
#define	SAMPLE_TAG		0
#define	SAMPLE_LOOP		0
#define	SAMPLE_VOLUME	127
#define	SAMPLE_PAUSE	0

#include "kernaud.hpp"

long	FAR PASCAL WndProc (HWND, unsigned, WORD, LONG) ;
int	PASCAL WinMain (HANDLE, HANDLE, LPSTR, int);
void	mMBox (char *, char *);

HWND			hMyWnd;
HINSTANCE	MyInstance;
HWND			hWndList;
HDC			hMyDC;

#ifndef VISUALCPP
#pragma warning 579  9         //   Shut off cast warning for lpfnWndProc
#endif

int PASCAL
WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
/***********************************************************************
	Standard Windows application main procedure.
************************************************************************/
{
static char szAppName[] = "Skel" ;
HWND        hWnd ;
MSG         msg ;
WNDCLASS    wndclass ;

	lpszCmdLine = lpszCmdLine;
	msg.wParam = 0;                 //ditto

	if (!hPrevInstance) {
		MyInstance = hInstance;
		wndclass.style				= CS_HREDRAW | CS_VREDRAW ;
		wndclass.lpfnWndProc		= (WNDPROC)WndProc ;
		wndclass.cbClsExtra		= 0 ;
		wndclass.cbWndExtra		= 0 ;
		wndclass.hInstance		= hInstance ;
		wndclass.hIcon				= LoadIcon (hInstance, "skelicon") ;
		wndclass.hCursor        = LoadCursor (NULL, IDC_ARROW) ;
		wndclass.hbrBackground  = GetStockObject (WHITE_BRUSH) ;
		wndclass.lpszMenuName   = szAppName ;
		wndclass.lpszClassName  = szAppName ;

		if (!RegisterClass (&wndclass))
			return FALSE ;

	} else {
		mMBox ("Cannot run two copies of test!", "Sierra");
		return msg.wParam ;                                     /* return to Windows */
	}


     hWnd = CreateWindow (szAppName, "Skel",
                         WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                         CW_USEDEFAULT, 0,
                         CW_USEDEFAULT, 0,
                         NULL, NULL, hInstance, NULL) ;
	hMyWnd = hWnd;

	ShowWindow (hWnd, nCmdShow) ;
	UpdateWindow (hWnd);

	hMyDC = GetDC (hMyWnd);

	KernelAudioInitialize();

	do {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		KernelAudioService();
	} while (1);
	KernelAudioTerminate();   
	return msg.wParam;
}

void
mMBox (char* text, char* title)
{
	MessageBox (NULL, (LPSTR)text, (LPSTR)title, MB_OK | MB_TASKMODAL);
}


long FAR PASCAL
WndProc (HWND hWnd, unsigned iMessage, WORD wParam, LONG lParam)
{
	char name[13];
	int  num;

	switch (iMessage)
		{
		case WM_COMMAND:
		// Menu bar selections

			switch (wParam)
				{
				case IDM_EXIT:
					SendMessage (hWnd, WM_CLOSE, 0, 0L);
					break;

				case IDM_TEST:
					assert(kernel->AudioInstalled());
					if (!kernel->AudioActiveSamples())
						sprintf(name,"%u.WAV",num = SAMPLE_NUM);
					else
						sprintf(name,"%u.WAV",num = SAMPLE_NUM2);
					if (!kernel->AudioPlay(num, (Boolean)SAMPLE_LOOP, SAMPLE_VOLUME,
						SAMPLE_TAG, (long)SAMPLE_PAUSE))
						mMBox("Can't play audio",name);
					break;

				default:
					break;
				}
			break;

		case WM_DESTROY:				// terminate
			PostQuitMessage (0) ;
			break ;

		default :
			return DefWindowProc (hWnd, iMessage, wParam, lParam) ;
			}
     return 0L ;
}

