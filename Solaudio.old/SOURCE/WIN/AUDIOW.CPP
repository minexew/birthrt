// audiow.cpp
#if !defined(VISUALCPP) && !defined(AUDIO_LIB)
#define WATCOM_HACK
#endif

#define INCLUDE_MMSYSTEM_H
#include "windows.h"  // this is "sciwin.hpp" on our end
#ifdef WATCOM_HACK
UINT __pascal _16waveOutPrepareHeader(HWAVEOUT hWaveOut, DWORD, UINT uSize);
UINT __pascal _16waveOutUnprepareHeader(HWAVEOUT hWaveOut, DWORD, UINT uSize);
UINT __pascal _16waveOutWrite(HWAVEOUT hWaveOut, DWORD, UINT uSize);
#endif

#include "audio.hpp"


#ifdef AUDIO_LIB

#include <stdio.h>
#include <stdlib.h>
#include "custmsys.hpp"
BOOL haveFocus = 1;
BOOL USEAUDIO = 1;
void MBox(char* s,char* t)
{
	MessageBox(0,s,t,MB_OK|MB_ICONEXCLAMATION);
}

#else	 // !AUDIO_LIB 

#include "sol.hpp"
#include "mbox.hpp"

#endif // AUDIO_LIB

#include "audiow.hpp"

static	void	DisplayError (WORD, char *);
static	void	OpenWaveDev (void);
static	int	OpenQuery (int,int,int);
static	void	CloseWaveDev (void);
static	void	ResetWaveDev (void);
static	void	WriteWave (int);
static	void	PrepareHeader (int);
static	void	UnprepareHeader (int);
static	int	CheckDoneFlags (void);
static	int	BumpBuffNum (int);
static	void	AdjustFormat(void);

#define	NUMBUFFERS		60
WAVEHDR			wHdr			[NUMBUFFERS];
#ifdef WATCOM_HACK
DWORD				wHdr_alias	[NUMBUFFERS];
#endif
LPSTR				lpBuff		[NUMBUFFERS];
BOOL				buffInUse	[NUMBUFFERS];
int				buffQueue	[NUMBUFFERS];
int				buffSize		[NUMBUFFERS];

static	WAVEOUTCAPS		woc;
#ifdef WATCOM_HACK
static	PCMWAVEFORMAT	pwf;
#else
static	WAVEFORMATEX 	wf;
#endif
static	HWAVEOUT			hWave;
static	UINT				wError;
static	Boolean				waveDevOpen = False;
static	int				maxBits, maxRate, maxChannels;
static	int				nextIn, nextOut;
static	int				writeCount = 0;
static	int				focusBuffCount = 0;
static	MMTIME			mmTime;
static	int				totSamples;

int DACInit(void)
{
int	bNum;

	if (!waveOutGetNumDevs())		/* any devices? */
		return -1;

	// Because of problems encountered with a Medivision driver, I have 
	// learned not to trust the return from DevCaps.  Instead, I now do
	// an open QUERY for each parameter.

	if (OpenQuery(8,44100,1))
		maxRate = 44100;
	else if (OpenQuery(8,22050,1))
		maxRate = 22050;
	else
		maxRate = 11025;

	if (OpenQuery(16,maxRate,1))
		maxBits = 16;
	else
		maxBits = 8;

	if (OpenQuery(maxBits,maxRate,2))
		maxChannels = 2;
	else
		maxChannels = 1;

	// set default values for the format
#ifdef WATCOM_HACK
	pwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	pwf.wf.nChannels  = 1;
	pwf.wf.nSamplesPerSec = 11025;
	pwf.wf.nAvgBytesPerSec = 11025;
	pwf.wf.nBlockAlign = 1;
	pwf.wBitsPerSample = (WORD)8;
#else
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels  = 1;
	wf.nSamplesPerSec = 11025;
	wf.nAvgBytesPerSec = 11025;
	wf.nBlockAlign = 1;
	wf.wBitsPerSample = (WORD)8;
	wf.cbSize = 0;
#endif


	// set up buffer headers
	for (bNum = 0; bNum < NUMBUFFERS; bNum++) {
		wHdr[bNum].dwBytesRecorded = (DWORD)0;
		wHdr[bNum].dwUser = (DWORD)0;
		wHdr[bNum].dwFlags = (DWORD)0;
		wHdr[bNum].dwLoops = (DWORD)0;
#ifdef WATCOM_HACK
		wHdr_alias[bNum] = AllocAlias16(&wHdr[bNum]);
#endif
		buffInUse[bNum] = FALSE;
	}

	OpenWaveDev();

	return 0;
}

int DACTerminate(void)
{
	CloseWaveDev();

#ifdef WATCOM_HACK
	for (int bNum = 0; bNum < NUMBUFFERS; bNum++)
			FreeAlias16((DWORD)wHdr_alias[bNum]);
#endif

	return 0;
}

int DACSubmitBuffer(char *addr, int size)
{
int	bNum;

	// find a free header
	for (bNum = 0; bNum < NUMBUFFERS;bNum++) {
		if (!buffInUse[bNum])
			break;
	}
	if (bNum >= NUMBUFFERS) {
		MBox("audio buffs exhausted","");
		return 1;
	}

	if (!waveDevOpen)
		OpenWaveDev();

	buffInUse[bNum] = TRUE;

	UnprepareHeader(bNum);

	lpBuff[bNum] = (LPSTR)addr;
#ifdef WATCOM_HACK
	wHdr[bNum].lpData = (LPSTR)AllocAlias16(lpBuff[bNum]);
#else
	wHdr[bNum].lpData = lpBuff[bNum];
#endif
	wHdr[bNum].dwBufferLength = (DWORD)size;

#ifdef WATCOM_HACK
	buffSize[bNum] = size / pwf.wf.nBlockAlign;
#else
	buffSize[bNum] = size / wf.nBlockAlign;
#endif

	PrepareHeader(bNum);
	WriteWave(bNum);

	return 0;
}

int DACStop(void)
{
	ResetWaveDev();
	focusBuffCount = 0;
	return 0;
}

int DACLoc(void)
{
	return writeCount + focusBuffCount;
}

int DACSetVolume(int)
{
	return 0;
}

int DACGetVolume(void)
{
	return (woc.dwSupport & WAVECAPS_VOLUME);
}

int DACGetMaxChannels(void)
{
	return maxChannels;
}

int DACGetMaxBits(void)
{
	return maxBits;
}

int DACGetMaxRate(void)
{
	return maxRate;
}

int DACSetRate(int rate)
{
	if (writeCount) {
		MBox("set rate: device busy","");
		return 1;
	}

	CloseWaveDev();

#ifdef WATCOM_HACK
	pwf.wf.nSamplesPerSec = Min(rate,maxRate);
#else
	wf.nSamplesPerSec = Min(rate,maxRate);
#endif
	AdjustFormat();

	OpenWaveDev();

	return 0;
}

int DACSetBits(int bits)
{
	if (writeCount) {
		MBox("set bits: device busy","");
		return 1;
	}

	CloseWaveDev();

#ifdef WATCOM_HACK
	pwf.wBitsPerSample = (WORD)Min(bits,maxBits);
#else
	wf.wBitsPerSample = Min((int)bits,maxBits);
#endif
	AdjustFormat();

	OpenWaveDev();

	return 0;
}

int DACSetChannels(int channels)
{
	if (writeCount) {
		MBox("set channels: device busy","");
		return 1;
	}

	CloseWaveDev();

#ifdef WATCOM_HACK
	pwf.wf.nChannels = (BYTE)Max(1,Min(channels,maxChannels));
#else
	wf.nChannels = Max(1,Min((int)channels,maxChannels));
#endif
	AdjustFormat();

	OpenWaveDev();

	return 0;
}

int
OpenQuery (int bits, int rate, int chan)
{
#ifdef WATCOM_HACK
	pwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	pwf.wf.nChannels  = (BYTE)chan;
	pwf.wf.nSamplesPerSec = rate;
	pwf.wBitsPerSample = (WORD)bits;

	AdjustFormat();

	wError = waveOutOpen ((LPHWAVEOUT)NULL,
									0,
									(LPWAVEFORMAT)&pwf.wf,
									(DWORD)NULL,
									(DWORD)0,
									WAVE_FORMAT_QUERY);
#else
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels  = chan;
	wf.nSamplesPerSec = rate;
	wf.wBitsPerSample = (WORD)bits;
	wf.cbSize = 0;

	AdjustFormat();

	wError = waveOutOpen ((LPHWAVEOUT)NULL,
									0,
									(LPWAVEFORMATEX)&wf,
									(DWORD)NULL,
									(DWORD)0,
									WAVE_FORMAT_QUERY);
#endif
	return (!wError);
}

void
AdjustFormat(void)
{
WORD	nBytesPerSample;

#ifdef WATCOM_HACK
	fatal_test (pwf.wBitsPerSample == 8 || pwf.wBitsPerSample == 16);
	fatal_test (pwf.wf.nChannels == 1 || pwf.wf.nChannels == 2);
	nBytesPerSample = (WORD)(pwf.wBitsPerSample / 8);
	pwf.wf.nAvgBytesPerSec = pwf.wf.nSamplesPerSec *
									 nBytesPerSample *
									 pwf.wf.nChannels;
	pwf.wf.nBlockAlign	  = (WORD)(1 * nBytesPerSample * pwf.wf.nChannels);
#else
	fatal_test (wf.wBitsPerSample == 8 || wf.wBitsPerSample == 16);
	fatal_test (wf.nChannels == 1 || wf.nChannels == 2);
	nBytesPerSample = wf.wBitsPerSample / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * nBytesPerSample * wf.nChannels;
	wf.nBlockAlign	  = 1 * nBytesPerSample * wf.nChannels;
#endif
}


void
OpenWaveDev (void)
{
	if (waveDevOpen)
		return;
	wError = waveOutOpen ((LPHWAVEOUT)&hWave,
									WAVE_MAPPER,
#ifdef WATCOM_HACK
									(LPWAVEFORMAT)&pwf.wf,
#else
									(LPWAVEFORMATEX)&wf,
#endif
									NULL,
									(DWORD)0,
									0);
	if (wError)
		DisplayError(wError, "waveOutOpen");
	waveDevOpen = True;
	writeCount = 0;
	totSamples = 0;
	nextIn = nextOut = 0;
}


void
CloseWaveDev (void)
{
	if (!waveDevOpen)
		return;

	ResetWaveDev();

	wError = waveOutClose(hWave);
	if (wError)
		DisplayError(wError, "waveOutClose");
	waveDevOpen = False;
	return;
}

void
ResetWaveDev(void)
{
	// If buffers are outstanding, then do a Reset
	// and wait for buffers to clear.

	if (USEAUDIO) {
		waveOutPause(hWave);
		wError = waveOutReset (hWave);
		if (writeCount) {
			totSamples = 0;
			if (wError)
				DisplayError(wError, "waveOutReset");
			while (writeCount)
				CheckDoneFlags();
		}
	}
}

void
PrepareHeader(int bNum)
{
	UnprepareHeader(bNum);
#ifdef WATCOM_HACK
	wError = _16waveOutPrepareHeader (hWave, wHdr_alias[bNum], sizeof(WAVEHDR));
#else
	wError = waveOutPrepareHeader (hWave, (LPWAVEHDR)&wHdr[bNum], sizeof(WAVEHDR));
#endif
	if (wError)
		DisplayError(wError, "waveOutPrepareHeader");
}

void
UnprepareHeader(int bNum)
{
	if (wHdr[bNum].dwFlags & WHDR_PREPARED) {
#ifdef WATCOM_HACK
		wError = _16waveOutUnprepareHeader (hWave, wHdr_alias[bNum], sizeof(WAVEHDR));
#else
		wError = waveOutUnprepareHeader (hWave, (LPWAVEHDR)&wHdr[bNum], sizeof(WAVEHDR));
#endif
		if (wError)
			DisplayError(wError, "waveOutUnprepareHeader");	
	}
}

void
WriteWave (int bNum)
{
	if (USEAUDIO) {
#ifdef WATCOM_HACK
		wError = _16waveOutWrite (hWave, wHdr_alias[bNum], sizeof(WAVEHDR));
#else
		wError = waveOutWrite (hWave, (LPWAVEHDR)&wHdr[bNum], sizeof(WAVEHDR));
#endif
		if (wError)
			DisplayError(wError, "waveOutWrite");
		buffInUse[bNum] = TRUE;
	}
	buffQueue[nextOut] = bNum;
	nextOut = BumpBuffNum(nextOut);
	writeCount++;
}

void
DisplayError (WORD wError, char *title)
{
char	szErrorText[150];

if (!waveOutGetErrorText(wError, (LPSTR)szErrorText, sizeof(szErrorText)))
	MessageBox (NULL, szErrorText, title, MB_OK);
}

void
DACPollWaveBuffs()
{
int	doneCount;
int	samplesAhead;
int	ticksAhead;

	if (dontPollDAC)
		return;
	if (!writeCount)
		return;
	dontPollDAC = True;

	while ((doneCount = CheckDoneFlags()) != 0) {
		mmTime.wType = TIME_SAMPLES;
		waveOutGetPosition(hWave,(LPMMTIME)&mmTime,sizeof(MMTIME));
		totSamples = Min((int)mmTime.u.sample,totSamples);
		samplesAhead = mmTime.u.sample - totSamples;
#ifdef WATCOM_HACK
		ticksAhead = (samplesAhead * 60) / pwf.wf.nSamplesPerSec;
#else
		ticksAhead = (samplesAhead * 60) / wf.nSamplesPerSec;
#endif
//	sprintf(mbuff,"t:%d ",ticksAhead);
//	sprintf(mbuff,"t:%d %d ",samplesAhead,writeCount);
//	OutputDebugString(mbuff);
		audioMgr->DACCallBack(doneCount,ticksAhead);
	}

	dontPollDAC = False;

}

uint
ElapsedBytes()
{
	mmTime.wType = TIME_BYTES;
	waveOutGetPosition(hWave,(LPMMTIME)&mmTime,sizeof(MMTIME));
	return mmTime.u.cb;
}

int
CheckDoneFlags()
{
int	bNum, doneCount;

	if (!writeCount)
		return 0;

	doneCount = 0;
	while (TRUE) {
		if (nextIn == nextOut)
			break;
		bNum = buffQueue[nextIn];
		if (!(wHdr[bNum].dwFlags & WHDR_DONE))
			break;
		buffInUse[bNum] = FALSE;
		UnprepareHeader(bNum);
#ifdef WATCOM_HACK
		FreeAlias16((DWORD)wHdr[bNum].lpData);
#endif
		nextIn = BumpBuffNum(nextIn);
		writeCount--;
		doneCount++;
		totSamples += buffSize[bNum];
	}

	return doneCount;
}

int
BumpBuffNum(int buffNum)
{
	buffNum++;
	if (buffNum < NUMBUFFERS)
		return buffNum;
	else
		return 0;
}

void
DACPauseWaveDev(void)
{
	if (audioInstalled) {
		focusBuffCount = writeCount;
		audioMgr->AudioPause(ALL_SAMPLES);
		CloseWaveDev();
	}
}

void
DACResumeWaveDev(void)
{
	if (!haveFocus)
		return;

	if (audioInstalled) {
  		OpenWaveDev();
		audioMgr->AudioResume(ALL_SAMPLES);
		if (focusBuffCount) {
			audioMgr->DACCallBack(focusBuffCount,0);
			focusBuffCount = 0;
		}
	}
}
