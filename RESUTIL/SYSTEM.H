/* =®RM250¯======================================================================
   Copyright (c) 1990,1995   Synergistic Software
   All Rights Reserved.
   =======================================================================
   Filename: SYSTEM.H   -Header file for all source files
   Author: Chris Phillips & Wes Cumberland
   ========================================================================
   Contains the following internal functions:
   Contains the following general functions:
   ======================================================================== */
#ifndef _SYSTEM_H
#define _SYSTEM_H

/* ------------------------------------------------------------------------
	Type defines
   ------------------------------------------------------------------------ */
#include <stdlib.h>		// for rand.

#ifdef _WINDOWS
//#include <windows.h>
#endif

#include "typedefs.h"

/* ------------------------------------------------------------------------
	Misc Defines
   ------------------------------------------------------------------------ */
/* Scale defines */
#define UNITARY_SCALE				5
#define UNITARY_PEOP_SCALE			3
#define NORMAL_LENS					4
#define FULL_SCALE					(1<<8)

/* ------------------------------------------------------------------------
	General Defines
   ------------------------------------------------------------------------ */
#define fNOERR				((SHORT)0)
#define fERROR				((SHORT)-1)

/* mark file as unavailable for future referance */
#define	fFILE_NOT_FOUND	-2

#define fREPORT_ALL		0xFF
#define fREPORT_MEMMGR	0x01
#define fREPORT_RESMGR	0x02
#define fREPORT_SYSTEM	0x04
#define fREPORT_GRAPH	0x08
#define fREPORT_OTHER	0x10

#define DPMI_INT	0x31

#define FONT_HEIGHT2 (8)

/* menu defines */
#define MAX_MENUS			50
#define MAX_ACTIVE_MENUS	10

/* menu flags */
#define D_NO_PUSH		0x00000001
#define D_HILIGHTED		0x00000002
#define D_KEEP_CR		0x00000004
#define D_TEXT_LEFT		0x00010000
#define D_ZOOM			0x00020000
#define D_UPDATE		0x00040000
#define D_FREEZE		0x00080000
#define D_INVISIBLE		0x00100000

/* ------------------------------------------------------------------------
	Font Defines
   ------------------------------------------------------------------------ */
#define FONT_PATH				"fonts\\"
#define FONT_SANS_5PT		(0)
#define FONT_SANS_6PT		(1)
#define FONT_SANS_8PT		(2)
#define FONT_SANS_12PT		(3)
#define FONT_SANS_16PT		(4)
#define FONT_TITL_8PT		(5)
#define FONT_TITL_10PT		(6)
#define FONT_TITL_16PT		(7)
#define FONT_TITL_20PT		(8)
#define FONT_TIMS_7PT		(9)
#define FONT_TIMS_8PT		(10)
#define FONT_TIMS_10PT		(11)
#define FONT_TIMS_30PT		(12)

/* ------------------------------------------------------------------------
	Defines for memory manager functions
   ------------------------------------------------------------------------ */

/* Equates for memory manager block attributes */
#define UNUSED				0x00
#define FREEMEM			0x01
#define BLKINUSE			0x02
#define BLKTYPE3			0x03
#define BLKTYPE4			0x04
#define STARTBLK			0x05
#define ENDBLK				0x06
#define BLKTYPEMASK		0x07

#define CLASSPERM			0x00
#define CLASS1				0x08
#define CLASS2				0x10
#define CLASSTEMP			0x18
#define CLASSMASK			0x18

#define LOCKED				0x20
#define RESOURCE			0x40
#define PURGABLE			0x80

/* ------------------------------------------------------------------------
	Defines for resource manager functions
   ------------------------------------------------------------------------ */
/* file I/O flag defines */
#define MODE_MAKEFILE	O_CREAT|O_RDWR|O_BINARY,S_IREAD|S_IWRITE
//#define MODE_MAKEFILE	O_CREAT|O_RDWR|O_BINARY
#define MODE_READFILE	O_RDONLY|O_BINARY
#define MODE_NEWFILE		O_CREAT|O_WRONLY|O_BINARY|O_TRUNC,S_IWRITE

/* header type defines */
#define TYPEANIM		0x000A
#define TYPEBITM		0x000B

#define FixWord(w)		(w) = ( (((USHORT)(w)) >> 8) | (((USHORT)(w)) << 8) )
#define FixLong(l)		(l) = ((l)>>24)|(((l)&0xFF0000L)>>8)|(((l)&0xFF00L)<<8)|((l)<<24)

#define SWAPINT(a,b)	{LONG x;  x=(a); (a)=(b); (b)=x;}

#define BUILD_LONG(s1, s2)		((0x00000000 | (s1)<<16) | (s2))
#define COMBINE_SHORTS(s1,s2,l)	{((l) = (0x00000000 | (s1)<<16) | (s2));}
#define SPLIT_LONG(l,s1,s2)		{((s1) = (short)((0xFFFF0000 & (l))>>16));\
								 ((s2) = (short)(0x0000FFFF & (l)));}
#define HIGH_LONG(l)			((short)((0xFFFF0000 & (l))>>16))								
#define LOW_LONG(l)				((short)(0x0000FFFF & (l)))

/* ------------------------------------------------------------------------
	Defines for graphic functions
   ------------------------------------------------------------------------ */
/* non-standard mode defines */
#define _WRES256COLOR	0x14

/* draw routine function constants */
#define COPY				0x101
#define DRAW				0x102
#define REVD				0x103
#define COMP				0x104
#define RMAP				0x105
#define RVRM				0x106

/* defines for Print */
#define fLEFTKERN			0x40
#define fRIGHTKERN		0x20

/* ------------------------------------------------------------------------
	Structure defines
   ------------------------------------------------------------------------ */
typedef struct colorspec
{
	UBYTE		bRed;
	UBYTE		bGreen;
	UBYTE		bBlue;
} COLORSPEC, *PALETTE;

/* BITM header structure */
typedef struct bitmhdr
{
	SHORT		w;				/* raster width & height in pixels */
	USHORT	h;
	SHORT		scale;		/* current scale discovered */
	SHORT		x_ctr_pt;	/* center point discovered */
	SHORT		type;			/* set to TYPEBITM */
} BITMHDR, *BITMPTR, ARECT, *RECTPTR;

typedef enum {
	REGION_EVENT_NOT_HANDLED,
	REGION_HANDLED_EVENT
} REGION_EVENT_TYPE;

// Structure to return the previous key state in add_key & del_key.
typedef struct _KeyStruct {
	LONG	val;
	LONG	val2;
	PFVLL	func;
} KEYSTRUCT, *PTR_KEYSTRUCT;

/* Flic Chunk Header */
typedef struct
{
	LONG		size;			/* Size of chunk including header. */
	USHORT	type;			/* Value from ChunkTypes above. */
} CHUNKHEAD, *CHUNKHEADPTR;

/* Flic Frame Header */
typedef struct
{
	LONG		size;				/* Size of frame including header. */
	USHORT	type;				/* Always FRAME_TYPE */
	SHORT		chunks;			/* Number of chunks in frame. */
	UBYTE		reserved[8];	/* Always 0. */
} FRAMEHEAD, *FRAMEHEADPTR;

/* menu button types */
typedef enum {
	BUTTON_REGION = 0,
	BUTTON_SHADEBEVEL,
	BUTTON_COLORBEVEL,
	BUTTON_ENTRY,
	BUTTON_LISTBOX,
	BUTTON_BITMAP
} BUTTON_TYPE;

/* menu buttons */
typedef struct _BUTTON {
	LONG			Id;				// button id
	BUTTON_TYPE		btType;			// button type
	LONG			Flags;			// button control flags
	LONG			Color;			// color of button
	int			iLabel;			// text label for button
	LONG			LabelColor;		// color of label text
	CSTRPTR			pHilight;		// name of art for hilight state
	LONG			X;				// x,y,w,y of button region
	LONG			Y;
	LONG			W;
	LONG			H;
	LONG			Key;			// hot key short cut
	PFVLL			pfFunction;		// function if region activated
	LONG			Arg;			// user arg passed to function
	int		idToolTip;		// Tooltip for the region.
} BUTTON;

typedef struct _MENU {
	BUTTON	*Buttons;
	LONG	MenuButtonCount;
} MENU, *PMENU;

typedef SHORT (*PFRESLOAD)		(CSTRPTR szResName, BOOL fSetPal, BOOL fLockRes, BOOL fRotated);
typedef SHORT (*PFRESDISPOSE)	(SHORT sResHandle, SHORT sMemHandle);
typedef SHORT (*PFRESSETPURGE)(SHORT sResHandle, SHORT sMemHandle);
typedef SHORT (*PFRESCLRPURGE)(SHORT sResHandle, SHORT sMemHandle);

/* ------------------------------------------------------------------------
   Macros
   ------------------------------------------------------------------------ */
#ifndef ABS
#define ABS(x)			(((x)<0) ? -(x) : (x))
#endif
#ifndef SGN
#define SGN(x)			(((x)<0) ? -1 : (((x)==0) ? 0 : 1))
#endif

#define SAME_SIGNS( a, b ) (((LONG) ((ULONG) a ^ (ULONG) b)) >= 0 )

#define random(num) (SHORT)(((LONG)rand()*(num))/(0x8000U))

#define MAX(a,b)			( ((a) > (b)) ? (a) : (b) )
#define MIN(a,b)			( ((a) < (b)) ? (a) : (b) )

// adjusts a value for screen resolution
//GEH #define X_RES_ADJ(x)	((x) * window_width / MAX_VIEW_WIDTH)
//GEH #define Y_RES_ADJ(y)	((y) * window_height / MAX_VIEW_HEIGHT)
// I fixed the screen to always be MAX_VIEW_WIDTH/HEIGHT
#define X_RES_ADJ(x)	((x))
#define Y_RES_ADJ(y)	((y))

/* These macros set or clear attribute bits in the memory manager */
#define ClrLock(i)		SetBlockAttr(i,LOCKED,0)
#define SetPurge(i)		_SetPurge(i)
#define ClrPurge(i)		_ClrPurge(i)
#define SetResource(i)	SetBlockAttr(i,RESOURCE,RESOURCE)
#define ClrResource(i)	SetBlockAttr(i,RESOURCE,0)
#define SetClassPerm(i) SetBlockAttr(i,CLASSMASK,CLASSPERM)
#define SetClass1(i)		SetBlockAttr(i,CLASSMASK,CLASS1)
#define SetClass2(i)		SetBlockAttr(i,CLASSMASK,CLASS2)
#define SetClassTemp(i) SetBlockAttr(i,CLASSMASK,CLASSTEMP)
#define BLKPTR(i)			(PTR)(apBlocks[i])
//GEH #define BLKPTR(i)			_GetBlockPtr(i)

#define ResetSplitScreen()	SetSplitScreen(0x3FF,0)
#define ASetPalette()		SetPalRegs((PTR)CurPal); MakeCompPal()
#define CloseImag(i)			DisposBlock(i)
#define CloseBitm(i)			DisposBlock(i)
#define ReleaseResBlock(i)	DisposBlock(i)
#define OpenImag(s)			_GetResource_((s),FALSE,FALSE,FALSE)
#define GetResourceStd(s,f)	_GetResource_((s),(f),FALSE,FALSE)
#define GetLockedResource(s,f) _GetResource_((s),(f),TRUE,FALSE)
#define GetResourceRot(s)	_GetResource_((s),FALSE,FALSE,TRUE)
#define OpenBitm(w,h)		_OpenBitm((w),(h),FALSE)
#define OpenLockedBitm(w,h) _OpenBitm((w),(h),TRUE)

#define NewLockedBlock(i)	SetBlockAttr(NewBlockHigh(i),LOCKED,LOCKED)
#define zalloc(i)				(void *)(apBlocks[SetBlockAttr(ValidateBlockID(NewBlockHigh(i)),LOCKED|CLASSMASK,LOCKED|CLASS1)])
#define zone_alloc(i)		(void *)(apBlocks[SetBlockAttr(ValidateBlockID(NewBlockHigh(i)),LOCKED|CLASSMASK,LOCKED|CLASS1)])

#define TextHeight(f,s)		((Print(NULL, 0, 0, (f), 0, (s)) >> 10) & 0x3FF)
#define TextWidth(f,s)		(Print(NULL, 0, 0, (f), 0, (s)) & 0x3FF)

#define SetLight(a)							shade=&shade_table[(a)*256]

/* ------------------------------------------------------------------------
   Prototypes
   ------------------------------------------------------------------------ */
#if defined (__cplusplus)
extern "C" {
#endif

#if defined(_RELEASE)
// over-ride the prototype for the release
int	printf( const char *__format, ... );
#endif

/* system.c */
void SaveVideoSet (void);
void RestoreVideoSet (void);
void set_margin_size (LONG l,LONG r,LONG t,LONG b);
void get_margin_size (LONG *l,LONG *r,LONG *t,LONG *b);
void set_screen_size (LONG vw,LONG vh);
void set_view_size (LONG vx,LONG vy,LONG vw,LONG vh);
void plot (LONG x,LONG y,LONG c);
LONG get_pixel (LONG x,LONG y);
void line (LONG x0, LONG y0, LONG x1, LONG y1,LONG color);
ULONG detect_scale (SHORT t, SHORT h, SHORT w);
LONG gplot_char(LONG c, LONG x, LONG y, LONG color);
LONG gtext_width(char *text);
LONG gtext_height(char *text);
SHORT gprint_text(LONG x,LONG y,char *text,LONG color);
void print_text_centered(LONG x,LONG y,char * text,LONG color);
SHORT print_textf(LONG x, LONG y, LONG color, const char *format, ...);
void init_gfont(SHORT type_and_size);
void ClearRemapTable(void);
void SetRemapTable(USHORT);
void DrawBitmap (SHORT x, SHORT y, SHORT iBitm, SHORT xb, SHORT yb, SHORT w, SHORT h);
SHORT SaveBitmap (SHORT x, SHORT y, SHORT w, SHORT h);
void ZoomBitmap(SHORT x, SHORT y, SHORT iBitm, SHORT xb, SHORT yb,
					 SHORT w, SHORT h, SHORT hDest, POINT start,
					 POINT bck, SHORT bckw, SHORT bckh);
void ZoomOutBitmap(SHORT x, SHORT y, SHORT iBitm, SHORT xb, SHORT yb,
						 SHORT w, SHORT h, SHORT hDest, POINT start,
						 POINT bck, SHORT bckw, SHORT bckh);
void ScaleBitmap (SHORT, SHORT, SHORT, SHORT, SHORT, SHORT, SHORT, SHORT);
SHORT _OpenBitm (SHORT w, USHORT h, BOOL fLocked);
BOOL Exists( char *filename);
void dec_view_size(LONG arg1, LONG arg2);
void inc_view_size(LONG arg1, LONG arg2);
void inc_lens (LONG, LONG);
void dec_lens (LONG, LONG);

/* memmanag.c */
ULONG InitMemManag (ULONG cbMinMemoryNeeded, USHORT cNumBlkHeads_, BOOL fReport_);
void QuitMemManag (void);
ULONG get_free_mem(void);
SHORT CheckValidBlock (SHORT i);
SHORT ValidateBlockID (SHORT i);
SHORT FindFreeHeader (void);
SHORT NewExternalBlock (PTR p);
SHORT SetBlockAttr (SHORT i, UBYTE mask, UBYTE value);
ULONG GetBlockSize (LONG i);
SHORT GetBlockIndex (PTR p);
SHORT NewBlock (ULONG cBytes);
SHORT FindBlock (ULONG cBytes);
SHORT NewBlockHigh (ULONG cBytes);
SHORT _SetPurge (SHORT );
SHORT _ClrPurge (SHORT );
SHORT DisposBlock (SHORT i);
SHORT DisposClass (UBYTE );
SHORT SetPurgeClass (UBYTE );
SHORT CleanUpBlock (SHORT i, BOOL fCleanDirection, ULONG cBytes);
SHORT MergeBlocks (SHORT i);
ULONG ReportFreeMem (ULONG fPurgable);
ULONG CompactMem_ (ULONG cBytes);
ULONG PurgeMem_ (ULONG cBytes);
UBYTE MoveFreeBlock (SHORT i, UBYTE dir);
SHORT SetBlockSize (SHORT i, ULONG cBytes);
SHORT SetLock (SHORT i);
SHORT HandleOutOfMemory (void);
void CheckLinks (CSTRPTR sz);
USHORT CheckSum (SHORT i);
void PrintMemList (void);
PTR  _GetBlockPtr(SHORT hdl);

/* regions.c */
void add_key(LONG /* key */,
				  PFVLL /* func */,
				  LONG /* val */,
				  LONG /* val2 */);
#define replace_key(key,func,val, pStruct) replace_key_vals((key), (func), (val), 0, (pStruct))
void replace_key_vals(LONG /* key */,
                 PFVLL /* pNewFunc */,
                 LONG /* NewVal */,
                 LONG /* NewVal2 */,
                 PTR_KEYSTRUCT /* pOldKeyStruct */);
void del_key(LONG /* key */,
             PTR_KEYSTRUCT /* pKeyStruct */);

LONG add_region(LONG x,LONG y,LONG w,LONG h,LONG key,PFVLL func,LONG val, LONG val2,LONG id, int idToolTip);
void del_region(PFVLL func,LONG key);
void del_all_regions(void);
REGION_EVENT_TYPE check_regions();
void init_regions(void);
void push_regions(void);
void pop_regions(void);
void activate_region(LONG id, BOOL state_on);
void OutlineAllRegions(void);
/* menu.c */
void InitMenuSys(PMENU UserMenus, LONG Count);
void ReleaseMenu(LONG Index);
void ShowSubMenu(LONG, LONG index);
void QuitSystem(LONG, LONG);
void ShowMenu(LONG Index);
void HideSubMenu(LONG, LONG index);
void HideMenu(LONG Index);
void RunMenus(void);
void IncrementVal( LONG unused, LONG pVal );
void DecrementVal( LONG unused, LONG pVal );
void ToggleVal(LONG unused,  LONG pVal );
LONG SetButtonValue ( LONG MenuIndex, LONG ButtonId, LONG Val);
LONG GetButtonValue ( LONG MenuIndex, LONG ButtonId);
LONG SetButtonProc ( LONG MenuIndex, LONG ButtonId, PFVLL pFunc, LONG Val, LONG Key );
LONG SetButtonLabel ( LONG MenuIndex, LONG ButtonId, int iLabel, LONG Color );
LONG SetButtonLabelColor ( LONG MenuIndex, LONG ButtonId, LONG Color );
LONG SetButtonPosition ( LONG MenuIndex, LONG ButtonId, LONG X, LONG Y );
LONG GetButtonPosition ( LONG MenuIndex, LONG ButtonId, LONG *X, LONG *Y );
LONG SetButtonSize ( LONG MenuIndex, LONG ButtonId, LONG W, LONG H );
LONG GetButtonSize ( LONG MenuIndex, LONG ButtonId, LONG *W, LONG *H );
LONG GetButtonFlags ( LONG MenuIndex, LONG ButtonId );
LONG SetButtonFlag ( LONG MenuIndex, LONG ButtonId, LONG Flag );
LONG ClearButtonFlag ( LONG MenuIndex, LONG ButtonId, LONG Flag );
LONG GetButtonLabel(LONG MenuIndex, LONG ButtonID, CSTRPTR *pLabel);
LONG SetButtonHilightArt ( LONG MenuIndex, LONG ButtonId, CSTRPTR pArtPath);
LONG SetButtonHilight ( LONG MenuIndex, LONG ButtonId, BOOL Hilight );
LONG SetUpdate(LONG MenuIndex, LONG ButtonId);
LONG CleanUpdate(LONG MenuIndex, LONG ButtonId);

/* resmanag.c */
SHORT OpenResFile_ (CSTRPTR szFileName);
SHORT _GetResource_ (CSTRPTR szResName, BOOL fSetPal, BOOL fLockRes, BOOL fRotated);
SHORT LastResBlockIndex (void);
SHORT Query_iResBlock (SHORT i);
SHORT PutPCX (SHORT iS, USHORT x, USHORT y, USHORT width, USHORT height, CSTRPTR szFileName);
SHORT PutScreen (SHORT iS, USHORT x, USHORT y, USHORT width, USHORT height, CSTRPTR szFileName);
SHORT	RegisterResExtention ( CSTRPTR sExtention, PFRESLOAD LoadProc,
	PFRESDISPOSE DisposeProc, PFRESSETPURGE SetPurgeProc, PFRESCLRPURGE ClrPurgeProc );
SHORT DisposRes (SHORT iResBlk, SHORT iMemBlk);
SHORT SetPurgeRes (SHORT iResBlk, SHORT iMemBlk);
SHORT ClrPurgeRes (SHORT iResBlk, SHORT iMemBlk);

/* redbook.c */
int PlayRedBook (ULONG start, ULONG length);
int StopRedBook (void);
int PlayTrack (unsigned char track);
unsigned long FindTrackStart (unsigned char track);
int CheckCDBusy (void);
int PlayCDMusic (unsigned short tune);
/* game.cpp */
void GAMEToggleMainMenu(LONG fState, LONG);

/* ------------------------------------------------------------------------
   Global Variables
   ------------------------------------------------------------------------ */
extern ULONG *			apBlocks;				/* memmanag.c */
extern COLORSPEC		CurPal[256];
extern BOOL				fLowMemory;				/* resmanag.c */
extern LONG  			screen_buffer_width;	/* system.c */
extern LONG  			screen_buffer_height;
extern LONG				graphics_mode;
extern PTR				screen;
extern LONG				lens_factor;
extern int				fMusic;					/* redbook.c */
extern SHORT			UserCDVolume;			/* redbook.c */
extern unsigned char	ucWhichTrack;			// redbook.c
extern POINT         zoomStart;           //menu.c

/*	======================================================================= */


#if defined (__cplusplus)
}
#endif
#endif // _SYSTEM_H

