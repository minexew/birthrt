#ifndef KERNCALL_HPP
#define KERNCALL_HPP

//------------------------------------------------------------------------------------------
//
// -- K E R N E L   I N T E R F A C E
//
//	Date:			 05/13/96
// Created by:  Jay Lee
//
// This file contains the definition of the KernelInterface class that implements a C++ 
// interface into the system classes.  The member functions here have a dual purpose - 
// the first is to be the set of functions used to access the functionality of the system 
// classes for programmers writing games in C++.  The second role is to be a layer 
// through which the Kernel calls for SCI programmers access the system classes. 
// This changes the prior achitecture to the one illustrated below:
//
//	BEFORE:
//
//		 |SCI PROGRAMMER| --> K[call] --> System classes
//
//	AFTER:
//
//		 |SCI PROGRAMMER| --> K[call] --> kernel->[call] --> System classes
//
//		 |C++ PROGRAMMER| --> kernel->[call] --> System classes
//
//	A single instance of this class in contructed at program initialization and destructed
// at program exit time.  
//
// VERY IMPORTANT NOTE:  When adding new methods, make sure that the are declared virtual.
//								 While this has no application for the interpreter per se, it 
//								 allows users of the C++ library to access the methods from a
//								 DLL.  Also, be aware of the various defines that are present
//								 and what code gets brought in as a result
//------------------------------------------------------------------------------------------

#ifndef AUDIO_LIB

#include "sol.hpp"

#ifdef WINDOWS
#include <windows.h>
#endif

#include "celobj.hpp"
#include "dos.hpp"
#include "event.hpp"
#include "kernel.hpp"
#include "line.hpp"
#include "palette.hpp"
#include "memid.hpp"
#include "memmgr.hpp"
#include "memtype.hpp"
#ifdef WINDOWS
#include "moview.hpp"  // enums for Avi calls
#endif
#include "palette.hpp"
#include "polygon.hpp"
#include "rect.hpp"
#include "savegame.hpp"
#include "scale.hpp"
#include "shwstyle.hpp"

#endif // #ifndef AUDIO_LIB

#include "audio.hpp"


#ifndef AUDIO_LIB

// You must define a function that matches the following prototype in
// your code.  This function is called once all the appropriate
// system initializations are completed.  The arguments passed into
// the executable are passed through so that you can create your own
// arguments

void KernelMain(int argc, char* argv[]);


// -- Utility / Help Functions

// returns boolean indicating whether is value passed is even.
inline Boolean EvenValue(int iValue)
{
	if ( iValue % 2 )
		return False;
	else
		return True;
}

// Internal library trace
void LogTrace(const char* strOutput);


// -- DEFINES

#if defined (DEBUG) && defined (INT_DEBUG) 
	#define LIBTRACE( x )	LogTrace( x )
#else
	#define LIBTRACE( x )
#endif

#define DEFINE_ERROR assert(!"You are calling an empty function.  Check the #defines you have specified for the build");
#define DEFINE_ERROR_RETURN assert(!"You are calling an empty function.  Check the #defines you have specified for the build"); return 0;


// CONSTANTS

const int DEBUG_CHAR				= 0xA3;
const int DEFAULT_PRIORITY    = -9999;
const int DEFAULT_COLOR       = -1;
const int MAX_XOFFSET		   = 0x7FFF;
const int MAX_YOFFSET		   = MAX_XOFFSET;
const int MAX_RESX			   = 1200;
const int MAX_RESY			   = 800;
const int SAVE_GAME_LIST_SIZE = SaveGame::MaxGames * SaveGame::NameLen + 1;
const int MAX_SAVED_GAMES     = SaveGame::MaxGames;
const int SAVE_GAME_NAME_LEN  = SaveGame::NameLen;
const int BUF_MAX_PATH			= MaxPath;
const int BUF_DEVICE				= 3;
const int MAX_16BIT_VALUE		= 32767;


// ENUMS

// Platform calls
enum PlatformType {
	Macintosh,
	Dos,
	Windows,
	Amiga
};

enum ColorTypes {
	Error,
	Colors16,
	Colors256,
	High16Bit,
	True32Bit
};


// file modes in KernelFileOpen

enum FileModes { 
	F_APPEND, 
	F_READ, 
	F_TRUNC 
};

// subfunctions in Kernel Free Space calls

enum CheckFreeSpaceFuncs { 
	SAVEGAMESIZE, 
	FREESPACE, 
	ENOUGHSPACETOSAVE
};


// -- Forward Declarations

class ScreenItem;
class SOL_Plane;

#endif // #ifndef AUDIO_LIB

// @doc KERNEL AUDLIB

/*****************************************************************************
 * @class This is the interface class for access into system functionality.
 *        The instantiation of this class is handled automatically.  The
 *			 pointer "kernel" may be de-referenced to access any of the functions.
 ******************************************************************************/
class KernelInterface
{
//@access Public Members
public:
	// @cmember Returns number of active samples
	virtual int AudioActiveSamples();
	// @cmember Determines is audio is installed
	virtual Boolean AudioInstalled();
	// @cmember Perform audio pre-loading
	virtual int AudioPreload(int iValue=0);
	// @cmember Play sample with pause
	virtual int AudioWaitToPlay(int iResNum, Boolean fLoop=False, int iVolume=MAXVOLUME, int iTag=0);   
	// @cmember Play a sample
	virtual int AudioPlay(int iResNum, Boolean fLoop=False, int iVolume=MAXVOLUME, int iTag=0, 
							    long fPause=False);
	// @cmember Stops all samples
	virtual void AudioStopAll();
	// @cmember Stop a specific sample
	virtual void AudioStop(int iResNum, int iTag=0);
	// @cmember Loop a sample
	virtual void AudioLoop(int iResNum, Boolean fLoop, int iTag=0);   
	// @cmember Pause all samples
	virtual Boolean AudioPauseAll();
   // @cmember Pause a specific sample
	virtual Boolean AudioPause(int iResNum, int iTag=0);
	// @cmember Resume all paused samples
	virtual Boolean AudioResumeAll();   
	// @cmember Resume a specific sample
	virtual Boolean AudioResume(int iResNum, int iTag=0);
	// @cmember Time since the first active sample started
	virtual Boolean AudioDurationAll(int* iDuration);   
	// @cmember Time since specified sample started
	virtual Boolean AudioDuration(int iResNum, int* iDuration, int iTag=0);
	// @cmember Set the playback rate
	virtual void AudioSetRate(int iRate);
	// @cmember Set the playback rate to its default
	virtual void AudioDefaultRate();
	// @cmember Returns the current playback rate
	virtual int AudioCurrentRate();
	// @cmember Set the number of bits
	virtual void AudioSetBits(int iBits);
	// @cmember Sets the bits to its default
	virtual void AudioDefaultBits();
	// @cmember Returns the current bits value
	virtual int AudioCurrentBits();
	// @cmember Sets the number of channels
	virtual void AudioSetChannels(int iChannels);
	// @cmember Set the number of channels to its default
	virtual void AudioDefaultChannels();
	//	@cmember Returns the current number of channels
	virtual int AudioCurrentChannels();
	// @cmember Returns the volume reduction setting
	virtual Boolean AudioGetVolReduction();
	// @cmember Set the volume reduction value
	virtual void AudioSetVolReduction(Boolean fFlag);
	// @cmember Distorts all the samples
	virtual void AudioDistortAll(int iDistortionMask);   
	// @cmember Distort a specific sample
	virtual void AudioDistort(int iResNum, int iDistortionMask, int iTag=0);   
	// @cmember Returns the overall volume
	virtual int AudioGlobalVolume();
	// @cmember Get the volume of a specific sample
	virtual Boolean AudioSampleVolume(int iResNum, int* iVolume, int iTag=0);   
	// @cmember Sets the global volume
	virtual void AudioVolumeAll(int iVolume);   
	// @cmember Sets the volume of a specific sample
	virtual void AudioVolume(int iResNum, int iVolume, int iTag=0);   
	// @cmember Fade a sample
	virtual Boolean AudioFade(int iResNum, int iTargetVolume, int iTicks, int iSteps, Boolean fEnd=False,
									  int iTag=0);
	// @cmember Checks for priority sample
	virtual Boolean AudioCheckNoise();
	// @cmember Pans a sample
	virtual void AudioPan(int iResNum, int iPanPercent, int iTag=0);   
	// @cmember Turns panning off for a sample
	virtual void AudioPanOff(int iResNum, int iTag=0);
	// @cmember Get the value of the DAC critical flag
	virtual Boolean AudioGetDACCritical();
	// @cmember Sets the value of the DAC critical flag
	virtual void AudioSetDACCritical(Boolean fFlag);

// @doc KERNEL

#ifndef AUDIO_LIB
	//  Audio functions - these are within the #ifdef so that they are not accessible
	//							 from the stand-alone audio library
	virtual int AudioWaitToPlay(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
									    Boolean fLoop=False, int iVolume=MAXVOLUME);
	virtual int AudioPlay(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
							    Boolean fLoop=False, int iVolume=MAXVOLUME, Boolean fPause=False); 
	virtual void AudioStop(int iModule, int iNoun, int iVerb, int iCond, int iSequ);
	virtual void AudioLoop(int iModule, int iNoun, int iVerb, int iCond, 
								  int iSequ, Boolean fLoop);   
	virtual Boolean AudioPause(int iModule, int iNoun, int iVerb, int iCond, int iSequ);
	virtual Boolean AudioResume(int iModule, int iNoun, int iVerb, int iCond, int iSequ);
	virtual Boolean AudioDuration(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
										   int* iDuration);
	virtual void AudioDistort(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
									  int iDistortionMask);
	virtual Boolean AudioSampleVolume(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
											    int* iVolume);
	virtual void AudioVolume(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
								    int iVolume);   
	virtual Boolean AudioFade36(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
									    int iTargetVolume=0, int iTicks=30, int iSteps=10, 
									    Boolean fEnd=True);
	virtual void AudioPan(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
							    int iPanPercent);
	virtual void AudioPanOff(int iModule, int iNoun, int iVerb, int iCond, int iSequ);

	// @cmember Retrieve pointer to object - Don't call from DLL!!
	static KernelInterface* GetKernelPointer();

	// Resource manipulation
	// @cmember Load multiple resources
	virtual Boolean Load(MemType memType, int* resNumArray, int iArraySize);
	// @cmember Unload a resource
	virtual void Unload(MemType memType, int iResNum);
	// @cmember	Make a resource Non-dicardable
   virtual void MakeNonDiscardable(MemType memType, int iResNum);
	virtual void MakeDiscardable(MemType memType, int iResNum);
   virtual Boolean ResCheck36(MemType memType, int iResNum, int iNoun, int iVerb, int iCond, int iSequ);
	virtual Boolean ResCheck(MemType memType, int iResNum);
   virtual SOL_Handle GetMemory(size_t theSize);
   virtual SOL_Handle LoadChunk(int iResNum);
	virtual Boolean SetLanguageDir(const char* strLangDir=0);

	// Animated objects & views
   virtual Boolean GetNowSeenRect(long lPlaneId, long lScreenItemId, SOL_Rect* pSOL_RectWork);
	virtual int NumLoops(int iView);
	virtual int NumCels(int iView, int iLoop);
	virtual int CelWidth(int iView, int iLoop, int iCel);
	virtual int CelHeight(int iView, int iLoop, int iCel);
	virtual int TopPlanePriority();
	virtual int TopScreenItemPriority(long lPlaneId);
	
	virtual Boolean ScreenItemIsOnMe(const SOL_Point& SOL_PointCheck, long lPlaneId, long lScreenItemId,
												Boolean fCheckSkip);
	virtual void AddScreenItem(ScreenItem* pScreenItemNew);
	virtual void DeleteScreenItem(ScreenItem* pScreenItem);
	virtual void UpdateScreenItem(ScreenItem* pScreenItem);
	
	virtual long AddLine(long lPlaneId, const SOL_Point& SOL_PointA, const SOL_Point& SOL_PointB,		
							   int iPriority=1000, int iColor=SOL_WHITE, int iStyle=SOL_Line::tSolid, 
							   int iPattern=0, int iThickness=1);
	virtual void DeleteLine(long lPlaneId, long lLineId);
	virtual void UpdateLine(long lPlaneId, long lLineId, const SOL_Point& SOL_PointA, 
								   const SOL_Point& SOL_PointB,	int iPriority=DEFAULT_PRIORITY,
								   int iColor=DEFAULT_COLOR, int iStyle=SOL_Line::tSolid, 
								   int iPattern=0, int iThickness=1);
	
	virtual long AddPolygon(long lPlaneId, SOL_Point* pointArray, int iArraySize, int iPriority=1000, 
								   int iColor=SOL_WHITE, int iStyle=SOL_Line::tSolid, int iPattern=0, 
								   int iThickness=1, Boolean fClosePoly=True);
	virtual void DeletePolygon(long lPlaneId, long lPolygonId);
	virtual void UpdatePolygon(long lPlaneId, long lPolygonId, SOL_Point* pointArray, int iArraySize,
									   int iPriority=DEFAULT_PRIORITY, int iColor=DEFAULT_COLOR, 
									   int iStyle=SOL_Line::tSolid, int iPattern=0, int iThickness=1, 
									   Boolean fClosePoly=True);
	
	virtual MemID BitmapCreate(int iWidth, int iHeight, int iColorSkip, int iColorBack,
                				   int iXRes=-1, int iYRes=-1, Boolean fRemap=False);
	virtual MemID BitmapCreateWithCel(int iView, int iLoop, int iCel, int iColorSkip=-1, 
											    int iColorBack=-1, Boolean fMirror=False, 
											    uchar* colorLookupArray=0);
	virtual void BitmapDispose(SOL_Handle SOL_HandleBitmap);
	virtual void BitmapAddLine(const MemID& memIDBitmap, int iLineX1, int iLineY1, int iLineX2, 
									   int iLineY2, int iColor, int iStyle=SOL_Line::tSolid, int iPattern=0);
	virtual void BitmapAddCel(const MemID& memIDBitmap, int iView, int iLoop, int iCel,
									  int iCelPosX=0, int iCelPosY=0, int iMatchCelX=-1, int iMatchCelY=-1);
	virtual void BitmapAddText(const MemID& memIDBitmap, const char* strText, int iPointAX,
									   int iPointAY, int iPointBX, int iPointBY, int iColorFore, 
									   int iColorBack, int iColorSkip, int iFont, int iMode,
									   int iColorBorder=-1, Boolean fDim=False);
	virtual void BitmapAddRect(const MemID& memIDBitmap, const SOL_Rect& SOL_RectDraw, 
									   int iColor);
	virtual void BitmapAddBitmap(const MemID& memIDBitmap, const MemID& memIdBitmapMerge, 
										  int iXPos=0, int iYPos=0, Boolean fIgnore=False);
	virtual void BitmapInvertRect(const MemID& memIDBitmap, const SOL_Rect& SOL_RectInvert, 
										   int iColorFore, int iColorBack);
	virtual void BitmapSetOrigin(const MemID& memIDBitmap, int iXPos, int iYPos);
	virtual void RemapBitmap(const MemID& memIDBitmap, uchar* colorLookupArray);
	virtual MemID BitmapDuplicate(const MemID& memIDBitmap, int iXRes=-1, int iYRes=-1);
	virtual int BitmapGetColor(const MemID& memIDBitmap, int iXPos, int iYPos);
	virtual int BitmapGetColor(const MemID& memIDBitmap, int iOffset);
	virtual int BitmapGetWidth(const MemID& memIDBitmap);
	virtual Boolean BitmapCopyToClipboard(const MemID& memIDBitmap);

	virtual void FrameOut(Boolean fShowBits = True);
	
	virtual void AddPlane(SOL_Plane* pPlaneNew);
	virtual void DeletePlane(long lPlaneId);
	virtual void UpdatePlane(SOL_Plane* pPlane);
	virtual void AddPicAt(long lPlaneId, int iPicture, int iXPos, int iYPos,
							    Boolean fMirror=False, Boolean fDeleteCopy=True);
	virtual void DeletePic(long lPlaneId, int iPicture);
	virtual void RepaintPlane(long lPlaneId);
	virtual void SetShowStyle(long lPlaneId, planeShowStyles showStyleType, int iSpeed, 
 						 int iColor, Boolean fAnimateFlag, Boolean fUseReferenceFrame, 
						 int iToFrom=0, int* intArray=0, int iArraySize=0, int iDivisionCount=-1);
	virtual int ShowStylePercentComplete(long lPlaneId);				 
	virtual void SetScroll(long lPlaneId, int iXStepSize, int iYStepSize, int iPicture,
								  Boolean fWithAnimate, int iTicks=-1, Boolean fMirror=False);
	virtual void MovePlaneItems(long lPlaneId, int iDeltaX, int iDeltaY, 
									    Boolean fMovePics=False);

	virtual Boolean IsHiRes();
	virtual long ObjectIntersection(long lScreenItemId1, long lPlaneId1, 
											  long lScreenItemId2, long lPlaneId2);
	virtual void BaseLineSpan(SOL_Rect* pSOL_RectWork, int iView, int iLoop, int iCel,
									  int iXOffset=0, int iYOffset=0);
	void SetVideoMode(short sMode);

	// Robots
	virtual void RobotInit(int iResNum, long lPlaneId, int iPriority, int iXOffset, int iYOffset);
	virtual void RobotDisplayFrame(int iFrame, int iNewXOffset=50000, int iNewYOffset=50000,
										    int iNewPriority=50000);
	virtual int RobotFrameInfo(SOL_Rect* pSOL_RectInfo);
	virtual void RobotResume();
	virtual void RobotPause();
	virtual int RobotFrameNum();
	virtual void RobotSetPriority(int iPriority);
	virtual Boolean RobotHasEnded();
	virtual Boolean RobotInProgress();
	virtual void RobotRemove();
	virtual long RobotGetCue();
	virtual Boolean RobotIsPaused();

	// Text
	virtual void TextSizeWrap(SOL_Rect* pSOL_RectWork, const char* strText, int iFontNum, 
									  int iMaxWidth=192, Boolean fScale=True);
	virtual void TextSizeNoWrap(SOL_Rect* pSOL_RectWork, const char* strText, int iFontNum, 
									    Boolean fScale=True);
	virtual int TextWidth(const char* strText, int iFontNum);
	virtual Boolean TextCopyToClipboard(const char* strText);


	// Font
	virtual int FontPointSize(int iFontNum, Boolean fScale=True);
	virtual void SetFontResolution(int iXRes, int iYRes);

	// Message
	virtual int MessageGet(int iResNum, int iNoun, int iVerb, int iCond, int iSequ, char* pBuf=0, 
								  int iBufSize=0);
	virtual int MessageGetNext(char* pBuf=0, int iBufSize=0);
	virtual Boolean MessageGetSize(int iResNum, int iNoun, int iVerb, int iCond, int iSequ, int* iMsgSize);
	virtual Boolean MessageGetNextSize(int* iMsgSize);
	virtual Boolean MessageGetRefValues(int iResNum, int iNoun, int iVerb, int iCond, int iSequ,
												   uchar* pRefNoun, uchar* pRefVerb, uchar* pRefCond);
	virtual void MessagePush();
	virtual void MessagePop();
	virtual void MessageGetKey(int* intArray);
	virtual void SetQuitString(const char* strQuit);

	// Event
	virtual Boolean GetEvent(SOL_Event::Type eventTypeMask, SOL_Event* pSOL_Event);
	virtual Boolean GlobalToLocal(long lPlaneId, SOL_Point* pSOL_PointGlobal);
	virtual Boolean LocalToGlobal(long lPlaneId, SOL_Point* pSOL_PointLocal);
	virtual void MapKeysToDirections(SOL_Event* pSOL_Event);

	// Mouse/Cursor
	virtual Boolean MousePresent();
	virtual void CursorClearRestrict();
	virtual void CursorShow();
	virtual void CursorHide();
	virtual void CursorForcePos(const SOL_Point& SOL_PointPos);
	virtual void CursorSet(int iView, int iLoop, int iCel);
	virtual void CursorSetRestrict(const SOL_Rect& SOL_RectRestrict);

	// Save/Restore
	virtual Boolean SaveGame(const char* strGameName, int iSaveGameNumber, 
								    const char* strSaveDesc, const char* strGameVersion);
	virtual Boolean RestoreGame(const char* strGameName, int iSaveGameNumber, 
									    const char* strGameVersion);
	virtual const char* GetSaveGameDir();
	virtual void SetSaveGameDir(const char* strDirectory);
	virtual Boolean CheckSaveGame(const char* strGameName, int iSaveGameNumber, 
										   const char* strGameVersion);
	virtual Boolean GetSaveFiles(const char* strGameName, char* strNameList, 
										  short* shortArray, int* iSaveGameCount);
	virtual void MakeCatalogName(const char* strGameName, char* strCatalogName);
	virtual void MakeSaveGameName(const char* strGameName, int iSaveNum,
										   char* strSaveGameName);

	// Syncs
	virtual void SyncStart(int iResNum, short* sCue);
	virtual void Sync36Start(int iResNum, int iNoun, int iVerb, int iCond, int iSequ,
								    short* sCue);
	virtual void SyncNext(short* sTime, short* sCue);
	virtual void SyncStop();

	// Math functions
	virtual unsigned long GetRandomNumber(unsigned int uiLow, unsigned int uiHigh);
	virtual unsigned long GetSeed();
	virtual void SetSeed(unsigned long ulSeed);
	virtual unsigned int Sqrt(unsigned long ulValue);
	virtual int GetAngle(const SOL_Point& SOL_PointA, const SOL_Point& SOL_PointB);
	virtual unsigned int GetDistance(const SOL_Point& SOL_PointA, const SOL_Point& SOL_PointB,
											   int iPerspective=0);
	virtual int ATan(int iPointAX, int iPointAY, int iPointBX, int iPointBY);
	virtual int SinMult(int iAngle, long lMultiplier);
	virtual int CosMult(int iAngle, long lMultiplier);
	virtual int SinDiv(int iAngle, long lDividend);
	virtual int CosDiv(int iAngle, long lDividend);

	// Miscellaneous functions
	virtual unsigned int SysTime(int iFormat=1);
	virtual unsigned int SysDate();
	virtual unsigned long GetTickCount();
	virtual int PlatformWhat();
	virtual int CDSpeed();
	virtual int PlatformColorDepth();
	virtual const char* GetCDDrive();
	virtual int GetCDisc(int iDiscNo=0);
	virtual int GetSaveCD();

	// OS functions
	virtual int FileOpen(const char* strFileName, int iMode);
	virtual int FileClose(int iFileHandle);
	virtual int FileWrite(int iFileHandle, void* buffer, int iBufferSize);
	virtual int FileDelete(const char* strFileName);
	virtual char* FileFGets(char* strBuffer, int iMaxReadChars, int iFileHandle);
	virtual long FileSeek(int iFileHandle, long iOffset, int iLocation);
	virtual Boolean FileFindFirst(const char* strSearchName, unsigned int uiAttrs, char* fileNameBuffer,
											int iBufferSize);
	virtual Boolean FileFindNext(char* fileNameBuffer, int iBufferSize);
	virtual Boolean FileExists(const char* strFileName);
	virtual Boolean FileRename(const char* strOldName, const char* strNewName);
	virtual Boolean FileCopy(const char* strCopyFromName, const char* strCopyToName);
	virtual int FileRead(int iFileHandle, void* buffer, int iBufferCount);
	virtual short FileSaveGameSize();
	virtual short FileFreeSpace(const char* strDrive);
	virtual Boolean FileEnoughSpaceToSave(const char* strDrive);
	virtual char* FileGetCWD(char* dirBuffer);
	virtual Boolean FileValidPath(const char* strPath);

	// Memory functions
	virtual size_t AvailableMemInK();
	virtual int MemAllocCount(MemType memType);
	virtual MemType TypeOfMemory(SOL_Handle handle);

	// System reporting
	virtual void GetDevice(const char* strPath, char* strDevice);
	virtual void GetCurrentDevice(char* strDevice);
	virtual Boolean SameDevice(const char* strDeviceA, const char* strDeviceB);
	virtual Boolean IsDriveAOrB(const char* strPath);

	// Palette Functions
	virtual void PaletteGetInfo(int iPaletteNum, SOLPalette* pSOLPalette);
	virtual void PaletteLoad(int iPaletteNum);
	virtual void PaletteIntensityRange(int iPercentage, int iStartColor=0, int iEndColor=255);
	virtual int PaletteColorIndex(int iRed, int iGreen, int iBlue);
	virtual void PaletteSetGamma(int iGammaValue=0);

	virtual void PalVaryStart(int iResNum, int iPercent=100, int iShiftTime=0, int iPalStart=-1, int iPalEnd=-1);
	virtual void PalVaryReverse(int iPercent=0, int iShiftTime=0);
	virtual int PalVaryGetPercent();
	virtual void PalVaryNewTime(int iShiftTimeNew);
	virtual void PalVaryKill();
	virtual int PalVaryMergeTarget(int iResNum);
	virtual void PalVaryPause();
	virtual void PalVaryResume();
	virtual int PalVaryNewTarget(int iResNum);
	virtual int PalVaryNewSource(int iResNum);
	virtual int PalVaryMergeSource(int iResNum);
	
	virtual void PalCycleDefine(int iStartIndex, int iEndIndex, int iDirection, int iTicks=0);
	virtual void PalCycleExecute(int iCycleStartIndex, int iCycleCount=1);
	virtual void PalCyclePauseAll();
	virtual void PalCyclePause(int iCycleStartIndex);
	virtual void PalCycleResumeAll();
	virtual void PalCycleResume(int iCycleStartIndex);
	virtual void PalCycleDisposeAll();
	virtual void PalCycleDispose(int iCycleStartIndex);
	
	virtual void SetPalStyleRange(int iLowerBound, int iUpperBound);

	// Remap
	virtual void RemapColorOffAll();
	virtual void RemapColorOff(int iColor);
	virtual void RemapColorByRange(int iColorRemap, int iColorStart, int iColorEnd, int iDelta);
	virtual void RemapColorByPercent(int iColorRemap, int iPercent);
	virtual void RemapColorToGray(int iColorRemap, int iPercentGray);
	virtual void RemapColorToPercentGray(int iColorRemap, int iGray, int iPercent);
	virtual void RemapColorDontMapRange(int iColorStart, int iCount);

	// Config file function
	virtual Boolean GetConfigValue(const char* strEntry, char* strValue, int iBufSize);

	// Cel functions
	virtual void GetCelRect(int iView, int iLoop, int iCel, int iXPos, int iYPos, int iZPos, 
								   SOL_Rect* pSOL_RectWork);
	virtual void GetCelRectScaled(int iView, int iLoop, int iCel, int iXPos, int iYPos, int iZPos, 
									      int iScaleX, int iScaleY, SOL_Rect* pSOL_RectWork);
	virtual int CelXOrigin(int iView, int iLoop, int iCel);
	virtual int CelYOrigin(int iView, int iLoop, int iCel);
	virtual int CelGetPixel(int iView, int iLoop, int iCel, int iXPos, int iYPos);
	virtual void MorphOn();
	virtual void SetHotRectangleStatus(Boolean fStatus);
	virtual void SetupHotRectangles(SOL_Rect* rectArray, int iArraySize);

	// VMDs
	virtual unsigned int VMDOpen(const char* strFileName, int iCacheSize=-1, int iOpenFlags=0);
	virtual void VMDPut(int iXPos, int iYPos, int iPutFlags=0, int iRGB=-1, int iRGBStart=-1, int iRGBEnd=-1);
	virtual unsigned int VMDWaitEvent(int iEvent, int iLastFrame=-1, int iReturnInterval=-1);
	virtual unsigned int VMDStop();
	virtual unsigned int VMDResume();
	virtual unsigned int VMDClose();
	virtual unsigned int VMDGetLength();
	virtual unsigned int VMDGetPosition();
	virtual unsigned int VMDStatus();
	virtual unsigned int VMDFramesSkipped();
	virtual void VMDSetPalette();
	virtual void VMDRestrictPalette(int iPaletteStart, int iPaletteEnd);
	virtual void VMDShowCursor(Boolean fShowFlag);
	virtual void VMDInitBlobs();
	virtual void VMDStopBlobs();
	virtual int VMDAddBlob(int iSquareSize, int iLeft, int iTop, int iRight, int iBottom);
	virtual void VMDDeleteBlob(int iBlob);
	virtual void VMDBlackoutArea(int iLeft, int iTop, int iRight, int iBottom);
	virtual void VMDSkipOn(Boolean fSkipFlag=True);
	virtual unsigned int VMDGetWidth();
	virtual unsigned int VMDGetHeight();
	virtual SOL_Handle VMDGetBitmap();
	virtual void VMDUseGraphicsSystem(int iPriority,long plane=0);
	virtual void VMDSetPreload(int iPreloadPercent);
	virtual void VMDSetInset(int left,int top,int right,int bottom);
	virtual void VMDClearInset();
	virtual void VMDSetFrameRate(int fps);

	// Polygon
	virtual Boolean InPolygon(int iXPos, int iYPos, SOL_Point* pointArray, int iArraySize);

	// Output to mono monitor
#ifdef VISUALCPP
	virtual void Mono(char*,...);
#else
	virtual void Mono(char*) { DEFINE_ERROR }
#endif

	// DLL Functions - The following functions have been created with the express
	//						 goal of allowing DLLs that have been written in conjunction
	//						 with the C++ Library to NOT have to LINK with with the 
	//						 library when they are built.  These DLL are then still able
	//						 to use the system functionality (including memory management) 
	//						 when they run thru a pointer to this class

#ifdef WINDOWS	
	virtual void* DLLNew(size_t,char*,int);
	virtual void DLLDelete(void*);
	virtual SOL_Plane* DLLConstructPlane(int, int, const SOL_Rect&, int,
											       const SOL_Point&, int, Boolean);
	virtual void DLLConvGameRectToPlaneRect(SOL_Plane*);
	virtual ScreenItem* DLLConstructScreenItem(long, const Bitmap&, SOL_Point, ScaleInfo);
	virtual void DLLDestructScreenItem(ScreenItem*);
	virtual void* DLLGetAddr(SOL_Handle);
	virtual Boolean DLLCheckValid(SOL_Handle);
#else
	virtual void* DLLNew(size_t,char*,int) { DEFINE_ERROR_RETURN }
	virtual void DLLDelete(void*) { DEFINE_ERROR }
	virtual SOL_Plane* DLLConstructPlane(int, int, const SOL_Rect&, int,
											       const SOL_Point&, int, Boolean) { DEFINE_ERROR_RETURN }
	virtual void DLLConvGameRectToPlaneRect(SOL_Plane*) { DEFINE_ERROR }
	virtual ScreenItem* DLLConstructScreenItem(long, const Bitmap&, SOL_Point, ScaleInfo) { DEFINE_ERROR_RETURN }
	virtual void DLLDestructScreenItem(ScreenItem*) { DEFINE_ERROR }
	virtual void* DLLGetAddr(SOL_Handle) { DEFINE_ERROR_RETURN }
	virtual Boolean DLLCheckValid(SOL_Handle) { DEFINE_ERROR_RETURN }
#endif


	// Dialog
#ifndef NETWORK
	virtual Boolean EditText(char* strText, int iTextForeColor, int iTextBackColor,
									 int iTextColorSkip, int iTextFont, int iMaxLength, 
									 const MemID& memIDBitmap, int iBorderColor, int iXOffset,
									 int iYOffset, long lPlaneId, int iMode, Boolean fFrameOut,
									 Boolean fDim, const char* strTitle=0, int iTitleColorFore=-1, 
									 int iTitleColorBack=-1, int iTitleFont=-1);    
 	virtual Boolean InputText(char* strInput, int iBufSize, const char* strTitle, int iBoxWidth);
#else
	virtual Boolean EditText(char*, int, int, int, int, int, const MemID&, int, int, int, long, int,
									 Boolean, Boolean, const char*=0, int=-1, int=-1, int=-1) { DEFINE_ERROR_RETURN }    
 	virtual Boolean InputText(char*, int, const char*, int) { DEFINE_ERROR_RETURN }
#endif 

	// Avi - Windows Only
#if defined(VFW) && defined(WINDOWS)
	virtual unsigned int AviOpen(int iMovieNum, const char* strMovieFileName);
	virtual unsigned int AviClose(int iMovieNum);
	virtual unsigned int AviPut(int iMovieNum, int iXPos, int iYPos, int iWidth=0, int iHeight=0);   
	virtual unsigned int AviPutDouble(int iMovieNum, int iXPos, int iYPos);
	virtual unsigned int AviPlay(int iMovieNum, int iStartFrame=0, int iEndFrame=0, 
										  avi_play_type aviPlayType=SCI_WINDOW, Boolean fCue=False);
	virtual unsigned int AviStop(int iMovieNum);
	virtual unsigned int AviPause(int iMovieNum);
	virtual unsigned int AviCue(int iMovieNum, int iStartFrame);
	virtual unsigned int AviSeek(int iMovieNum, int iStartFrame);
	virtual unsigned int AviResume(int iMovieNum);
	virtual unsigned int AviSetPalette(int iMovieNum);
	virtual unsigned int AviGetLength(int iMovieNum);
	virtual unsigned int AviGetPosition(int iMovieNum);
	virtual unsigned int AviGetStatus(int iMovieNum);
	virtual unsigned int AviGetFramesSkipped(int iMovieNum);
	virtual unsigned int AviWaitEvent(int iEvent=AVI_COMPLETED+AVI_ESC_PRESSED);
#else
	virtual unsigned int AviOpen(int, const char*) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviClose(int) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviPut(int, int, int, int=0, int=0) { DEFINE_ERROR_RETURN }   
	virtual unsigned int AviPutDouble(int, int, int) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviPlay(int, int=0, int=0, int=0, Boolean=False) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviStop(int) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviPause(int) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviCue(int, int) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviSeek(int, int) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviResume(int) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviSetPalette(int)  { DEFINE_ERROR_RETURN }
	virtual unsigned int AviGetLength(int)  { DEFINE_ERROR_RETURN }
	virtual unsigned int AviGetPosition(int) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviGetStatus(int)  { DEFINE_ERROR_RETURN }
	virtual unsigned int AviGetFramesSkipped(int) { DEFINE_ERROR_RETURN }
	virtual unsigned int AviWaitEvent(int=0) { DEFINE_ERROR_RETURN }
#endif

	
#if defined(DEBUG) && !defined(WINDOWS)
	virtual void SaveScreen(const char* strFileName);
#else
	virtual void SaveScreen(const char*) { DEFINE_ERROR }
#endif


#ifdef DEBUG
	//Debug functions
	// @cmember Check internal integrity
	virtual void CheckIntegrity(); 
	// @cmember Implements some of the interpreter debug facilities for library
   virtual void LibraryDebug();
#else
	virtual void CheckIntegrity() { DEFINE_ERROR } 
   virtual void LibraryDebug() { DEFINE_ERROR }
#endif

#endif // #ifndef AUDIO_LIB
};

extern KernelInterface* kernel;

#endif
