// audiodir.cpp
//		digital audio sample playback using Direct Sound

/************************
To Do:

  - check brSrcPos assert in MoveRData
  - rework audLoc for ALL_SAMPLES
  - get device caps and set primary format from user input
  - Check prioritySample
  - read file only once in CheckWAVEFormat (?)
  - priority volume 128
  - tag values
  - Rate stuff (AudioRate, DefaultRate, CurrentRate)
  - Bits stuff (AudioBits, DefaultBits, CurrentBits)
  - Chan stuff (AudioChannels, DefaultChannels, CurrentChannels)
  - Vol reduction (mixCheck, Get, Set)
  - DACCritical (Get, Set)
  - AudioDistort
  - problems if dSoundBuffSize is too close to audioFillBuffSize (< 100 MSec)
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys\stat.h>

#include "audio.hpp"
#include "audiodir.hpp"

#ifdef AUDIO_LIB

#include "custmsys.hpp"
AudioMgr	*audioMgr;

#else	 // !AUDIO_LIB

#include "sciwin.hpp"

#include "sol.hpp"

#include "config.hpp"
#include "dos.hpp"
#include "mgraph.hpp"
#include "mgraphw.hpp"
#include "kernel.hpp"
#include "memid.hpp"
#include "memmgr.hpp"
#include "msg.hpp"
#include "newroom.hpp"
#include "resource.hpp"
#include	"sync.hpp"
#include	"timew.hpp"
#include	"mbox.hpp"

#endif // AUDIO_LIB

#include "dsound.hpp"
#include "intrptw.hpp"

class MemID;

extern "C" {
void		CopyEOS(void *, void *, int, char);
void		Decomp(void *, int, int, int, int, int, int);
int		DecompGet(int);
void		DupeChannel(void *, int, int, char);
void		Silence(void *, int, char);
}

#ifdef REG_CALL_BUILD

#pragma aux STACK_BASED "*" parm caller [];

#pragma aux (STACK_BASED) Silence;
#pragma aux (STACK_BASED) Decomp;
#pragma aux (STACK_BASED) DecompGet;
#pragma aux (STACK_BASED) CopyEOS;
#pragma aux (STACK_BASED) DupeChannels;

#endif

void		AudServer(void);

#define	AUDIO_TEMP MOVEABLE+TRANSITORY
#define	AUDIO_PERM MOVEABLE+PERSIST

#define	ROBOT_RATE			22050
#define	ROBOT_BITS			16
#define	ROBOT_CHANNELS		1
#define	ROBOT_COMPRESSED	1

#define	FD_SILENCE		0
#define	FD_EOS			1
#define	FD_DUPE			2

const int MAX_16BIT_VALUE		= 32767;

Boolean	dSoundInitialized	= False;
Boolean dSoundTimersWorking = True;	// GWP Not using this value right now.

extern	DWORD	DSoundMSec;
extern	DWORD	AudioFeedMSec;
extern	DWORD	AudioIOBytes;

AudioMgrDir	*thisAud;

#ifdef DEBUG
// special assert call for thread
extern	BOOL	threadErrorFlag;
#define threadAssert(expr,n)  if (expr) {} else ThreadAssertFail(n, __FILE__, __LINE__, #expr)
#else
#define threadAssert(expr,n)
#endif

static ResNum	panResNum			= 0;
static int		panPercent			= -1;

AudioMgrDir::AudioMgrDir()
{
	convID				= 0;
	critical				= False;
	DACCritical			= False;
	distortion			= 0;
	ffloc					= -1;
	mixCheck				= True;
	preLoad				= 0;
	prioritySample		= -1;
	rate					= 0;
	bits					= 0;
	channels				= 0;
	masterVolume		= MAXVOLUME;
	robotsActive		= False;
	robotsPaused		= False;
	vmdSync				= 0;
	vmdSample			= 0;
	volFactor			= 0;
	serviceSentry		= FALSE;
	dontPollDAC			= True;
	critSectionCount	= 0;
	dontPollDAC			= False;
	audioInstalled		= False;
	audVolFD				= -1;
	sfxVolFD				= -1;
	activeSamples		= 0;

	InitializeCriticalSection(&memCrit);
}

AudioMgrDir::~AudioMgrDir()
{
	EndAudio();

	DeleteCriticalSection(&memCrit);
}

Boolean
AudioMgrDir::InitAudioDriver()
{
	// initialize the direct sound functions
	// NOTE: InitDSound and starting the timer have been moved to
	//       CreateDSoundObject because at the time InitAudioDriver is
	//       called, the graphics manager has not been created.  This
	//       is determined by the order of creation in sci.cpp.
//	InitDSound(((GraphicsMgrWin*)graphMgr)->WindowHandle());

	#ifdef AUDIO_LIB
	timeMgr = &TimeMgr();
	configMgr = &ConfigMgr();
	msgMgr = &MsgMgr();
	#endif

	// get parameters from config file
	vmdSync = configMgr->GetNum("vmdSync", 0, 0);
	volFactor = configMgr->GetNum("volDirectSound", 0, 1);
	if (volFactor < 1)
		volFactor = 1;
	else if (volFactor > 1000)
		volFactor = 1000;

	// set play parameters
	// (terry - should query dsound here, return false if error)
	channels = 2;
	bits = 16;
	rate = 22050;

	// start the timer (10 callbacks per second)
//	timeBeginPeriod (timerPeriod);
//	timerId = timeSetEvent (timerPeriod, timerAccuracy, (LPTIMECALLBACK)TimeFunc,
//								(DWORD)0, TIME_PERIODIC);

	thisAud = this;
	audioInstalled = True;
	return True;
}

void
AudioMgrDir::EndAudio()
{
	if (!audioInstalled)
		return;

	// stop all samples
	AudioStop(ALL_SAMPLES);

	// close any open files
	if (sfxVolFD != -1)
		Close(sfxVolFD);
	if (audVolFD != -1)
		Close(audVolFD);

	// terminate direct sound functions
	if (dSoundInitialized) {

		StopTimer();
		
		TermDSound();
		dSoundInitialized = False;
	}

	audioInstalled = False;
}

int
AudioMgrDir::AudioSelect(int n,ResNum module,int num,uchar noun,uchar verb,
				uchar cond,uchar sequ,int tag,Boolean loop)
{
// This function is called for normal samples only - not for robots
// or VMDs.
// Return:	0 if error
//				else total play time in ticks
//
// Operation:
//		If the sample is uncompressed and pre-loaded, then it is played
//		directly by the dsound module.  Otherwise, a looping feed buffer
//		is set up and file data and/or de-compressed data is fed to it.
//
//		If the sample is on disk, but is smaller than a certain maximum,
//		then it is read in its entirety and treated as being pre-loaded.
//
//		If the sample must be streamed from disk, then an additional
//		file read buffer is set up.

char	pathName[MaxPath + 1];
BOOL	result;
uchar	*tmpPtr;

	// set sample parameters
	memset(&sample[n], 0, sizeof(Sample));
	sample[n].module	= module;
	sample[n].num		= num;
	sample[n].noun		= noun;
	sample[n].verb		= verb;
	sample[n].cond		= cond;
	sample[n].sequ		= sequ;
	sample[n].tag		= tag;
	sample[n].loop		= loop;
	sample[n].pan		= -1;

	// find the sample in memory or on disk
	if (!FindSample(n,pathName))
		return 0;

	// check the sample format and return:
	//		start, length, rate, bits, channels, compressed
	//    fd != 0 if a file
	if (CheckSOLFormat(n))
		sample[n].resType = MemResAudio;
	else if (CheckWAVEFormat(n))
		sample[n].resType = MemResWAVE;
	else {
		if (sample[n].module == SFXMODNUM)
			msgMgr->Fatal("Invalid audio file format: %d",sample[n].num);
		else
			msgMgr->Fatal("Invalid audio file format: %s",pathName);
	}

//	DWORD tmpFeed = sample[n].feedID;
//	sprintf(mbuff,"samp:%d num:%d fd:%d len:%d bits:%d rate:%d feed:%d comp:%d loop:%d\n",
//				n,sample[n].num,sample[n].fd,sample[n].length,sample[n].bits,
//				sample[n].rate,tmpFeed,sample[n].compressed,sample[n].loop);
//	MonoStr(mbuff);

	if (sample[n].length <= 0) {
	#ifdef xDEBUG
      msgMgr->Alert("Invalid sample length on %d of type %s\n",
                     sample[n].num,
						   ::GetMemTypeStr((MemType) sample[n].resType));
   #endif
      return 0;
   }

	// compute uncompressed length
	sample[n].uncompLen = sample[n].length * (sample[n].compressed + 1);

	// establish buffer sizes
	SetBuffSize(n);

	// initialize compression values
	InitCompressionValues(n);

//#ifndef AUDIO_LIB
	// if file-based but marked for pre-load, or if the file is small
	// enough, then load it into memory
	if (sample[n].fd &&
		((preLoad && n+1 >= preLoad) ||
		 (sample[n].length <= (int)sample[n].fileBuffSize) ||
		 (sample[n].uncompLen <= ((int)sample[n].fileBuffSize + (int)sample[n].feedBuffSize)))) {

		// If it is a sound effect, then load it as a resource so that it
		// will stay in memory if we need it again

		// commented out for now - having trouble if sample is a WAV patch
		// file - resMgr->Get does not find the patch, but instead
		// finds the sample in vols  -TM 9/26/96

//		if (sample[n].module == SFXMODNUM) {
//			resMgr->Get(sample[n].resType, sample[n].num);
//			sample[n].memID = resMgr->Find(sample[n].resType,sample[n].num);
//			if (!sample[n].memID)
//				msgMgr->Fatal("Error loading audio resource: %d",sample[n].num);
//			sample[n].attrs = sample[n].memID.Attrs();
//			memMgr->SetNotDiscardable(sample[n].memID);
//			sample[n].memID.Critical(True);
//			if (sample[n].patch) {
//				sample[n].patch = False;
//				Close(sample[n].fd);
//			}
//			sample[n].fd = 0;
//			// Now, re-check the format to get valid start value
//			if (!CheckSOLFormat(n) && !CheckWAVEFormat(n))
//				msgMgr->Fatal("Invalid audio file format: %d",sample[n].num);
//
//		// Not a sound effect, just allocate memory and load it
//		} else {
			sample[n].memID.Get(MemAudioBuffer, sample[n].length, AUDIO_TEMP+DISCARDABLE);
#ifndef AUDIO_LIB
			sample[n].attrs = sample[n].memID.Attrs();
			memMgr->SetNotDiscardable(sample[n].memID);
			sample[n].memID.Critical(True);
			sample[n].memID.Read(sample[n].fd);
#else
			sample[n].memID.Read(sample[n].fd,0,sample[n].length);
#endif
			sample[n].start = 0;
			if (sample[n].patch) {
				sample[n].patch = False;
				Close(sample[n].fd);
			}
			sample[n].fd = 0;
//		}
	}
//#endif

	// if memory-based and compressed, then decompress it if it's small enough
	if (!sample[n].fd &&
		 sample[n].compressed &&
		 (sample[n].uncompLen <= (sample[n].length + (int)sample[n].feedBuffSize))) {

		// wave can't be compressed
		assert(sample[n].resType == MemResAudio);

		sample[n].memID.Realloc(sample[n].start + sample[n].uncompLen);
		tmpPtr = (uchar*)*sample[n].memID + sample[n].start;
		memcpy(tmpPtr + sample[n].uncompLen - sample[n].length,
				 tmpPtr,
				 sample[n].length);
		Decomp(tmpPtr,
				 sample[n].length,
				 sample[n].compressed,
				 sample[n].bits,
				 sample[n].channels,
				 sample[n].compValL,
				 sample[n].compValR);
		sample[n].compressed = 0;
		sample[n].length = sample[n].uncompLen;

		// if it's a sound effect, fix up the resource header
		if (sample[n].module == SFXMODNUM) {
			tmpPtr = (uchar*)*sample[n].memID;
			// reset compressed flag
			*(tmpPtr + 8) &= 0xfc;
			// set new length
			*((int *)(tmpPtr + 9)) = sample[n].uncompLen;
#ifdef xDEBUG
			memMgr->SetChecksum(sample[n].memID);
#endif
		}
	}

	// set source sample start position
	sample[n].readStart = sample[n].start;

	// if still file-based, create a file read buffer
	if (sample[n].fd) {
		sample[n].fileID.Get(MemAudioBuffer, sample[n].fileBuffSize, AUDIO_TEMP);
		sample[n].fileID.Critical(True);
		sample[n].fileOfs = sample[n].start;
		sample[n].fileBuffPos = sample[n].fileBuffSize;
		sample[n].fileReadPos = 0;

		// also, increase the dSound buffer by 50%
		// (and make sure it is not bigger than the feed buff)
		sample[n].dSoundBuffSize += (sample[n].dSoundBuffSize / 2) & 0xfffffffc;
//		sample[n].feedBuffSize = Max(sample[n].feedBuffSize,sample[n].dSoundBuffSize);
	}

	// if compressed and/or file-based, create a feed buffer and prime it
	if (sample[n].compressed || sample[n].fd) {
		sample[n].feedID.Get(MemAudioBuffer, sample[n].feedBuffSize, AUDIO_TEMP);
		sample[n].feedID.Critical(True);
		sample[n].playID = sample[n].feedID;
		sample[n].start = 0;
		PrimeFeedBuff(n);
	} else {
		sample[n].playID = sample[n].memID;
	}

//	tmpFeed = sample[n].feedID;
//	sprintf(mbuff,"samp:%d num:%d fd:%d len:%d bits:%d rate:%d feed:%d comp:%d loop:%d\n",
//				n,sample[n].num,sample[n].fd,sample[n].length,sample[n].bits,
//				sample[n].rate,tmpFeed,sample[n].compressed,sample[n].loop);
//	MonoStr(mbuff);

	// create the direct sound object
	sample[n].dSnd = CreateDSoundObject();

	// initialize the dsound driver for the proper buffer
	if (sample[n].feedID.IsAllocated()) {

		// using feed buffer; loop it
		sample[n].dSnd->SetLoop(TRUE);
		sample[n].streaming = TRUE;
		result = sample[n].dSnd->Init(
						sample[n].dSoundBuffSize,
						(char *)*sample[n].feedID,
						sample[n].feedBuffSize,
						sample[n].bits,
						sample[n].rate,
						sample[n].channels);
	} else {

		// play directly
		// set loop as required
		if (sample[n].loop)
			sample[n].dSnd->SetLoop(TRUE);
		else
			sample[n].dSnd->SetLoop(FALSE);
		// check the size to see if we can play it as a static
		if (sample[n].length > (int)sample[n].dSoundBuffSize) {
			// too big, play as streaming
			sample[n].streaming = TRUE;
			result = sample[n].dSnd->Init(
							sample[n].dSoundBuffSize,
							(char *)*sample[n].memID + sample[n].start,
							sample[n].length,
							sample[n].bits,
							sample[n].rate,
							sample[n].channels);
		} else {

			// small sample, play as static
			sample[n].streaming = FALSE;
			result = sample[n].dSnd->Init(
							0,
							(char *)*sample[n].memID + sample[n].start,
							sample[n].length,
							sample[n].bits,
							sample[n].rate,
							sample[n].channels);
		}
	}

	if (!result) {
		#ifndef AUDIO_LIB
		MBox("dsound init fail","");
		#endif
		return 0;
	}

	// return play time in ticks
	sample[n].playMSec = 
			(sample[n].length * (sample[n].compressed+1) * 1000) /
			(sample[n].bits/8 * sample[n].channels * (int)sample[n].rate);

	return 1 + ((sample[n].playMSec * 3) / 50);
}

void
AudioMgrDir::AudioPlay(int n)
{

//	if (sample[n].nowPlaying)
//		AudioStop(n);

	// prime the feed buffer again (in case this is a restart)
	if (sample[n].feedID.IsAllocated()) {
		PrimeFeedBuff(n);
	}

	// set initial volume
	sample[n].dSnd->SetVolume(ScaleVolume(n),volFactor);

	// set initial pan
	AudioPan(n,sample[n].pan);

	// and start playing if not paused
	if (!sample[n].pausedLoc) {
		sample[n].dSnd->Play((char *)*sample[n].playID + sample[n].start);
		sample[n].startLoc = timeMgr->GetTickCount();
		sample[n].nowPlaying = TRUE;
	}
	sample[n].bufferPrimed = FALSE;
}

void
AudioMgrDir::MarkStopped(int n)

// Mark a sample as having been stopped.  Called from the timer callback
// thread instead of calling AudioStop which causes problems.

{
	sample[n].nowPlaying	= FALSE;
	sample[n].stopped		= TRUE;
	sample[n].dSnd->Stop();
}

int
AudioMgrDir::GetActiveSamples(void)
{

// Called whenever activeSamples is to be referenced.  Scans the list and
// calls AudioStop and decrements activeSamples for any sample which
//	has been stopped.  

BOOL	found;

	while (TRUE) {
		found = FALSE;
		for (int n = 0; n < activeSamples; n++) {
			if (sample[n].stopped) {
				AudioStop(n);
				found = TRUE;
				break;
			}
		}
		if (!found)
			return activeSamples;
	}
}

void
AudioMgrDir::AudioStop(int n)
// Stop a single sample or all samples
{
int	t;

	if (n == NO_SAMPLES || !activeSamples)
		return;

	// protect from timer callback
	EnterCritical();

	if (n == ALL_SAMPLES) {
		// Stop all samples
		for (t = 0; t < activeSamples; t++)
			DropSample(t);
		activeSamples = 0;

	} else {
		// Stop sample n
		DropSample(n);
		--activeSamples;
		// if still have samples, shuffle the table so they are contiguous
		if (activeSamples) {
			for (t = n; t < activeSamples; t++) {
				sample[t] = sample[t+1];
				if (sample[t].vmd)
					vmdSample = t;
			}
		}
	}
	LeaveCritical();
}

void
AudioMgrDir::DropSample(int n)
{
	// free the feed buffer if any
	if (sample[n].feedID.IsAllocated())
		sample[n].feedID.Free();

	// free the decompression buffer if any (robots only)
	if (sample[n].robot)
		convID.Free();
	
	if (sample[n].fd) {
		// file-based sample; close the i/o buffer
		sample[n].fileID.Free();
		// if patch file, close it
		if (sample[n].patch)
			Close(sample[n].fd);

	} else {
		// not file-based; mark buffer as discardable
		int dups;

		for (dups = 0;dups < activeSamples;dups++) 
			if (dups != n && sample[n].memID.IsAllocated() && (*sample[dups].memID == *sample[n].memID) )
				break;

		#ifndef AUDIO_LIB
		if (dups == activeSamples && sample[n].attrs & DISCARDABLE)
			memMgr->SetDiscardable(sample[n].memID);
		#else
		if (dups == activeSamples)
			sample[n].memID.Free();
		#endif
	}

	// stop and delete the dsound object
	sample[n].dSnd->Stop();
	delete sample[n].dSnd;

	sample[n].nowPlaying = FALSE;
}

int
AudioMgrDir::AudioLoc(int n)
//	A return value of -1 indicates that no audio is currently playing.
//	A return value of 0 through 65534 indicates the time in sixtieths of
//	a second that the current audio selection has been playing for.

{
int	ticks, currTicks, i;

	currTicks = timeMgr->GetTickCount();

	if (n == NO_SAMPLES || !activeSamples)
		return -1;

	if (n == ALL_SAMPLES) {
		// find longest-running sample
		int highTicks = 0;
		for (i = 0; i < activeSamples; i++) {
			if (sample[i].pausedLoc)
				ticks = sample[i].pausedLoc - sample[i].startLoc;
			else
				ticks = currTicks - sample[i].startLoc;
			if (ticks > highTicks)
				highTicks = ticks;
		}
		return Max(0,Min(65534,highTicks));
	}

	// Query specific sample
	if (sample[n].pausedLoc)
		ticks = sample[n].pausedLoc - sample[n].startLoc;
	else
		ticks = currTicks - sample[n].startLoc;
	return Max(0,Min(65534,ticks));
}

void
AudioMgrDir::AudioDistort(int request, int n)
// terry - needs work
{
	if (n == ALL_SAMPLES)
		distortion = request;
	else if (n != NO_SAMPLES)
		sample[n].distortion = request;
}

void
AudioMgrDir::AudioVolume(int request, int n)
// Set the volume of sample n to the requested value (0 - MAXVOLUME)
// If ALL_SAMPLES, set the master volume and adjust the individual samples
{
	if (n == NO_SAMPLES)
		return;

	if (n == ALL_SAMPLES) {
		masterVolume = Min(request,(int)MAXVOLUME);
		GetActiveSamples();
		for (int i = 0; i < activeSamples; i++)
				sample[i].dSnd->SetVolume(ScaleVolume(i),volFactor);
		return;
	}

	// single sample
	sample[n].volume = Min(request,(int)MAXVOLUME);
	sample[n].dSnd->SetVolume(ScaleVolume(n),volFactor);
}

void
AudioMgrDir::AudioPan(int n, int panPercent)
// Set the pan value of sample n to panPercent which is the percentage
// value for the right channel.  A value of 50 means centered.  A value of
// -1 turns panning off.
{
	sample[n].pan = Min(panPercent,100);
	if (panPercent == -1)
		sample[n].dSnd->SetPan(50,volFactor);
	else
		sample[n].dSnd->SetPan(panPercent,volFactor);
}

WORD
AudioMgrDir::ScaleVolume(int n)
// Return a volume scaled to 0-100 based on the master volume
// and the sample volume
{
	// (this can be simplified whenever MAXVOLUME becomes 100)
	return (masterVolume * sample[n].volume * 100) / (MAXVOLUME * MAXVOLUME);
}

DSound *
AudioMgrDir::CreateDSoundObject(void)
// Called each time we need to create a new DSound object.  If dSound has
// not been initialized, then do it and start the timer.  See comment in
// InitAudioDriver as to why we do this here.
{
DWORD	setRate;
WORD	setBits;
WORD	setChannels;

	if (!dSoundInitialized) {
		dSoundInitialized = True;
		#ifndef AUDIO_LIB
		InitDSound(((MulGraphMgrWin*)graphMgr)->WindowHandle(DEFAULT_WINDOW));
		#else
		InitDSound(hMyWnd);
		#endif

		setRate		= (DWORD)rate;
		setBits		= (WORD)bits;
		setChannels	= (WORD)channels;
		SetPrimaryFormat(&setRate, &setBits, &setChannels);

	   StartTimer();
	}
	#ifndef AUDIO_LIB
	return New DSound();
	#else
	return new DSound();
	#endif
}

void
AudioMgrDir::StartTimer(void)
{
	dSoundTimersWorking = InstallServer(AudServer,100);
}

void
AudioMgrDir::StopTimer(void)
{
	dSoundTimersWorking = DisposeServer(AudServer);
}

void
AudioMgrDir::SetBuffSize(int n)
{
// Set sample buffer sizes according to parameters specified in the
// RESOURCE.WIN file and based upon the sample's rate, bits, and
// channels as follows:
//		
//		dsound object buffer - number of milliseconds specified
//									  by the "DSoundMSec" parameter
//		
//		feed buffer          - number of milliseconds specified
//									  by the "AudioFeedMSec" parameter
//		
//		file i/o read buffer - number of bytes specified by the
//									  "AudioIOBytes" parameter
DWORD	factor;

	// Compute multiplier factor;
	factor = 1;
	factor *= sample[n].bits / 8;
	factor *= sample[n].channels;
	if (sample[n].rate <= 11025)
		factor *= 1;
	else if (sample[n].rate <= 22050)
		factor *= 2;
	else if (sample[n].rate <= 44100)
		factor *= 4;

	// Compute buff sizes
	sample[n].dSoundBuffSize = ((factor * 11025 * DSoundMSec) / 1000) & 0xfffffffc;
	sample[n].feedBuffSize = ((factor * 11025 * AudioFeedMSec) / 1000) & 0xfffffffc;
	sample[n].fileBuffSize = AudioIOBytes & 0xfffffffc;

	// file i/o buffer can't be smaller than feed buffer
	if (sample[n].fileBuffSize < sample[n].feedBuffSize)
		sample[n].fileBuffSize = sample[n].feedBuffSize;
}

DWORD
AudioMgrDir::GetMSec(int n, DWORD buffSize)
{
// Compute play time in milliseconds for a given buffer size for
// sample n.

DWORD	factor;

	// Compute multiplier factor;
	factor = 1;
	factor *= sample[n].bits / 8;
	factor *= sample[n].channels;
	if (sample[n].rate <= 11025)
		factor *= 1;
	else if (sample[n].rate <= 22050)
		factor *= 2;
	else if (sample[n].rate <= 44100)
		factor *= 4;

	return (1000 * buffSize) / (factor * 11025);
}

void
AudioMgrDir::InitCompressionValues(int n)
{
	if (sample[n].bits == 16) {
		sample[n].compValL = 0;
		sample[n].compValR = 0;
	} else {
		sample[n].compValL = 0x80;
		sample[n].compValR = 0x80;
	}
}

void
AudioMgrDir::PrimeFeedBuff(int n)
// Fill up the feed buffer prior to starting play
{
	if (!sample[n].bufferPrimed) {
		FillBuffer(n, 0, sample[n].feedBuffSize);
		sample[n].bufferPrimed = TRUE;
	}
}

void
AudioMgrDir::FillBuffer(int n, DWORD fillPos, DWORD fillLen)
// Fill the feed buffer for sample n starting at fillPos and
// extending for fillLen bytes.  Wrap to beginning if required.
{
DWORD	bytesToEnd, mod4Len;

	bytesToEnd = sample[n].feedBuffSize - fillPos;
	mod4Len = fillLen & 0xfffffffc;
	if (mod4Len <= bytesToEnd) {
		// enough room - just one move
		MoveData(n,fillPos,mod4Len);
	} else {
		// wrap around - do it with two moves
		MoveData(n,fillPos,bytesToEnd);
		MoveData(n,0,mod4Len - bytesToEnd);
	}
}

void
AudioMgrDir::MoveData(int n, DWORD writePos, DWORD writeLen)
// Move data into the fill buffer starting at writePos and extending
// for writeLen bytes.
{
DWORD	newPos, newLen, bytesLeft;

	// If length is 0, no move
	if (writeLen == 0)
		return;

	// Compute uncompressed bytes remaining in sample
	bytesLeft = (sample[n].length - sample[n].sampReadPos) * (sample[n].compressed + 1);

	// If sample exhausted, fill with silence
	if (bytesLeft == 0) {
		FillSilence(n, writePos, writeLen);
	 	sample[n].feedBuffPos = writePos + writeLen;
		return;
	}

	// Don't get more than what's left
	newLen = Min(writeLen,bytesLeft);
	
	// Decompress or move the data
	GetSomeData(n,writePos,newLen);

	// If not at end of sample, return
	if (writeLen <= bytesLeft)
		return;

	// End of sample.  If looping, start over, else start filling
	// with silence.
	newPos = writePos + bytesLeft;
	newLen = writeLen - bytesLeft;
	if (sample[n].loop) {
		// looping - start over
		sample[n].sampReadPos = 0;
		sample[n].fileReadPos = 0;
		sample[n].fileBuffPos = sample[n].fileBuffSize;
		InitCompressionValues(n);
		GetSomeData(n,newPos,newLen);
	} else {
		// not looping - finish fill with silence
		FillSilence(n, newPos, newLen);
		sample[n].sampReadPos = sample[n].length;			
	}
}

void
AudioMgrDir::GetSomeData(int n, DWORD writePos, DWORD writeLen)
// Get data into the feed buffer at writePos for writeLen bytes
// and decompress it if required.
{
	if (sample[n].compressed)
		Decompress(n,writePos,writeLen);
	else
		GetData(n,writePos,writeLen);

	// update the feed buffer index
	sample[n].feedBuffPos = writePos + writeLen;
}

void
AudioMgrDir::Decompress(int n, DWORD writePos, DWORD writeLen)
// Decompress into the feed buffer at writePos for writeLen bytes
{
DWORD	decompPos, decompLen;

	// Compute position and length for compressed data
	decompLen = writeLen / (sample[n].compressed + 1);
	decompPos = writePos + (decompLen * sample[n].compressed);

	// Get compressed data
	GetData(n,decompPos,decompLen);

	// And decompress it
//	threadAssert((writePos + decompLen) <= sample[n].feedID.Size());
	Decomp((char *)*sample[n].feedID + writePos,
				decompLen,
				sample[n].compressed,
				sample[n].bits,
				sample[n].channels,
				sample[n].compValL,
				sample[n].compValR);
	sample[n].compValL = DecompGet(0);
	sample[n].compValR = DecompGet(1);
}

void
AudioMgrDir::GetData(int n, DWORD writePos, DWORD writeLen)
// Get data from memory or file into feed buffer starting at writePos
// for writeLen bytes
{
DWORD	buffBytesLeft, readLen, moveLen;
#ifndef DEBUG
	threadAssert(memMgr->IsValid(sample[n].feedID),n);
#endif
	// If memory sample, just copy the data
	if (!sample[n].fd) {
		threadAssert((writePos + writeLen) <= sample[n].feedID.Size(),n);
		threadAssert((sample[n].sampReadPos + sample[n].readStart + writeLen) <= sample[n].memID.Size(),n);
		memcpy((char *)*sample[n].feedID + writePos,
				 (char *)*sample[n].memID + sample[n].sampReadPos + sample[n].readStart,
				 writeLen);
		sample[n].sampReadPos += writeLen;
		return;
	}

	// File-based sample. check if we have enough data in the file buffer
#ifndef DEBUG
	threadAssert(memMgr->IsValid(sample[n].fileID),n);
#endif
	buffBytesLeft = sample[n].fileBuffSize - sample[n].fileBuffPos;
	if (buffBytesLeft > writeLen) {
		// We have enough, just copy the data
		threadAssert((writePos + writeLen) <= sample[n].feedID.Size(),n);
		threadAssert((sample[n].fileBuffPos + writeLen) <= sample[n].fileID.Size(),n);
		memcpy((char *)*sample[n].feedID + writePos,
				 (char *)*sample[n].fileID + sample[n].fileBuffPos,
				 writeLen);
		sample[n].sampReadPos += writeLen;
		sample[n].fileBuffPos += writeLen;
		return;
	}

	// Not enough data. Copy what we have.
	if (buffBytesLeft > 0) {
		threadAssert((writePos + buffBytesLeft) <= sample[n].feedID.Size(),n);
		threadAssert((sample[n].fileBuffPos + buffBytesLeft) <= sample[n].fileID.Size(),n);
		memcpy((char *)*sample[n].feedID + writePos,
				 (char *)*sample[n].fileID + sample[n].fileBuffPos,
				 buffBytesLeft);
		sample[n].sampReadPos += buffBytesLeft;
	}

	// Then read data from the file if any left
	readLen = Min(sample[n].length - sample[n].fileReadPos, sample[n].fileBuffSize);
	if (readLen > 0) {
		LSeek(sample[n].fd,sample[n].fileOfs + sample[n].fileReadPos,SEEK_SET);
		threadAssert(readLen <= sample[n].fileID.Size(),n);
		Read(sample[n].fd,(char *)*sample[n].fileID,readLen);
		sample[n].fileReadPos += readLen;
		sample[n].fileBuffPos = 0;

		// If there is still more to copy, then do it
		if (writeLen > buffBytesLeft) {
			moveLen = Min(readLen,writeLen - buffBytesLeft);
			threadAssert((writePos + buffBytesLeft + moveLen) <= sample[n].feedID.Size(),n);
			threadAssert(moveLen <= sample[n].fileID.Size(),n);
			memcpy((char *)*sample[n].feedID + writePos + buffBytesLeft,
					 (char *)*sample[n].fileID,
					 moveLen);
			sample[n].sampReadPos += moveLen;
			sample[n].fileBuffPos += moveLen;
		}
	}
}

void
AudioMgrDir::FillSilence(int n, DWORD fillPos, DWORD fillLen)
// Fill the feed buff with silence starting at fillPos for fillLen bytes
{
int	silenceVal;

	if (sample[n].bits == 8)
		silenceVal = 0x80;
	else
		silenceVal = 0x00;
#ifndef DEBUG
	threadAssert(memMgr->IsValid(sample[n].feedID),n);
#endif
	threadAssert((fillPos + fillLen) <= sample[n].feedID.Size(),n);
	memset((char *)*sample[n].feedID + fillPos, silenceVal, fillLen);
	sample[n].feedBuffPos = fillPos + fillLen;
}

Boolean
AudioMgrDir::FindSample(int n, char *pathName)
// Locate sample n in memory or on disk.  If found, return True.
// If found in memory, set sample[n].memID to its handle, save
// its attributes and mark it NotDiscardable.
// If found on disk, set sample[n].fd to its file handle.
{
	int	fd;

	if (panResNum == sample[n].num)
		sample[n].pan = panPercent;
	panPercent = -1; 

	// Search for AUD/WAV resource:
	pathName[0] = '\0';

#ifndef AUDIO_LIB
	if (sample[n].module == SFXMODNUM) {

		if ((sample[n].memID = resMgr->Find(MemResAudio,sample[n].num)) ||
			 (sample[n].memID = resMgr->Find(MemResWAVE, sample[n].num))) {
			sample[n].attrs = sample[n].memID.Attrs();
			memMgr->SetNotDiscardable(sample[n].memID);
			sample[n].memID.Critical(True);
			return True;

		// Search for .AUD file via 'audio=' dir list
		} else if (
				#ifdef DEBUG
				!configMgr->Get(configMgr->FilesAfterVols) &&
				#endif
				(fd = resMgr->Open(MemResAudio, sample[n].num, pathName)) != -1
				&& fd != sfxVolFD && fd != resVolFD && fd != resVolFDPre && fd != altVolFD) {
			sample[n].fd = fd;
			sample[n].patch = True;
			return True;

		// Search for .WAV file via 'wave=' dir list
		} else if (
				#ifdef DEBUG
				!configMgr->Get(configMgr->FilesAfterVols) &&
				#endif
				(fd = resMgr->Open(MemResWAVE, sample[n].num, pathName)) != -1
				&& fd != sfxVolFD && fd != resVolFD && fd != resVolFDPre && fd != altVolFD) {
			sample[n].fd = fd;
			sample[n].patch = True;
			return True;

		// Search for .AUD resource in Resource Volume
		} else if (resMgr->Check(MemResAudio, (ResNum)sample[n].num) != 0) {
			sample[n].memID = resMgr->Get(MemResAudio, (ResNum)sample[n].num);
			sample[n].attrs = sample[n].memID.Attrs();
			memMgr->SetNotDiscardable(sample[n].memID);
			sample[n].memID.Critical(True);
			return True;

		// Search for .WAV resource in Resource Volume
		} else if (resMgr->Check(MemResWAVE, (ResNum)sample[n].num) != 0) {
			sample[n].memID = resMgr->Get(MemResWAVE, (ResNum)sample[n].num);
			sample[n].attrs = sample[n].memID.Attrs();
			memMgr->SetNotDiscardable(sample[n].memID);
			sample[n].memID.Critical(True);
			return True;

		// Search for AUD/WAV resource in SFX Resource Volume
		} else if ((sample[n].fileOfs = FindAudEntry((ResNum)sample[n].num)) != -1) {
			sample[n].fd = sfxVolFD;
			#ifdef DEBUG   
			sprintf(pathName,"%d from SFX",sample[n].num);
			CheckDiscStreaming(pathName);
			#endif
			return True;

		#ifdef DEBUG
		// Search for .AUD file via 'audio=' dir list
		} else if (configMgr->Get(configMgr->FilesAfterVols)
				&& (fd = resMgr->Open(MemResAudio, sample[n].num, pathName)) != -1
				&& fd != sfxVolFD && fd != resVolFD && fd != resVolFDPre && fd != altVolFD) {
			missingResources++;
			msgMgr->Dump("room#%5u: %u%s not found in vols\n",currentRoom,sample[n].num,resMgr->GetExtension(MemResAudio));
			sample[n].fd = fd;
			sample[n].patch = True;
			return True;

		// Search for .WAV file via 'wave=' dir list
		} else if (configMgr->Get(configMgr->FilesAfterVols)
				&& (fd = resMgr->Open(MemResWAVE, sample[n].num, pathName)) != -1
				&& fd != sfxVolFD && fd != resVolFD && fd != resVolFDPre && fd != altVolFD) {
			missingResources++;
			msgMgr->Dump("room#%5u: %u%s not found in vols\n",currentRoom,sample[n].num,resMgr->GetExtension(MemResWAVE));
			sample[n].fd = fd;
			sample[n].patch = True;
			return True;
		#endif

		// AUD/WAV resource NOT FOUND!
		} else {
			return False;
		}

	} else {

		// Search for @ resource:

		// Search for @ file via 'audio=' dir list
		MakeName36(MemResAudio, pathName, (ResNum)sample[n].module, sample[n].noun,
			sample[n].verb, sample[n].cond, sample[n].sequ);
		if (
				#ifdef DEBUG
				!configMgr->Get(configMgr->FilesAfterVols) &&
				#endif
				(fd = resMgr->Open(MemResAudio36, (ResNum)-1, pathName)) != -1
				&& fd != audVolFD && fd != sfxVolFD) {
			sample[n].fd = fd;
			sample[n].patch = True;
			return True;

		// Search for @ resource in AUD Resource Volume */
		} else if ((sample[n].fileOfs = FindAud36Entry((ResNum)sample[n].module,
						sample[n].noun,sample[n].verb,sample[n].cond,sample[n].sequ))
					!= -1) {
			sample[n].fd = audVolFD;
			#ifdef DEBUG   
			CheckDiscStreaming(pathName);
			#endif
			return True;

		#ifdef DEBUG
		} else if (configMgr->Get(configMgr->FilesAfterVols)
				&& (fd = resMgr->Open(MemResAudio36, (ResNum)-1, pathName)) != -1
				&& fd != audVolFD && fd != sfxVolFD) {
			missingResources++;
			msgMgr->Dump("room#%5u: %s not found in vols\n",currentRoom,pathName);
			sample[n].fd = fd;
			sample[n].patch = True;
			return True;
		#endif

		// @ resource NOT FOUND!
		} else {
			return False;
		}
	}
#else  // AUDIO_LIB

	// Search for memory-resident audio
	if ((sample[n].memID = GetAdr(sample[n].num)) != 0) {
		sample[n].memID.Critical(True);
		return True;
	// Search for file-based audio
	} else if ((fd = Open(sample[n].num)) != -1) {
		sample[n].fd = fd;
		sample[n].patch = True;
		return True;
	// audio NOT FOUND!
	} else
		return False;
#endif  // #ifndef AUDIO_LIB
}

void
AudioMgrDir::ConvBase36(char *str, int num10, int digits)
{
	int	n, t;

	t = 0;
	if (digits >= 3) {
		str[t++] = GetDigit36(n = num10 / (36*36));
		num10 -= n * (36*36);
	}
	if (digits >= 2) {
		str[t++] = GetDigit36(n = num10 / 36);
		num10 %= 36;
	}
	str[t] = GetDigit36(n = num10);
}

char
AudioMgrDir::GetDigit36(int n)
{
	if (n <=  9)
		return((char)('0' + n));
	else
		return((char)('A' + n - 10));
}

int
AudioMgrDir::CheckNoise()
{
	return 0;
}

Boolean
AudioMgrDir::CheckSOLFormat(int n)
// Check if sample n is in SOL format.  If it is, set the file pointer
// to the first 'playable' byte and return True.
// Also, set the following:
//			sample[n].start		- relative start position
//			sample[n].length		- sample length
//			sample[n].rate			- sample rate
//			sample[n].bits			- bits per sample
//			sample[n].channels	- mono or stero
//			sample[n].compressed	- compression flag
{
uchar	header[256], flag;
uchar*	sptr = 0;

	if (!sample[n].fd) {
		sptr = (uchar*)*sample[n].memID;
		memcpy(header,sptr,6);
		sptr += 6;
	} else {
		LSeek(sample[n].fd,sample[n].fileOfs,SEEK_SET);
		Read(sample[n].fd,header,6);	// 2-byte header + 4-byte signature
	}
	if (((header[0]&0x7f) != (uchar)(MemResAudio)) || strcmp((char*)&header[2],"SOL"))
		return False;
	if (sptr) {
		sample[n].rate = ((uint)*sptr) + ((uint)(*(sptr+1)) << 8);
		flag = *(sptr+2);
		sample[n].length = ((uint)*(sptr+3)) + ((uint)(*(sptr+4)) << 8) +
			((uint)(*(sptr+5)) << 16) + ((uint)(*(sptr+6)) << 24);
		sample[n].start = (int)header[1]+2;
	} else {
		Read(sample[n].fd,&sample[n].rate,2);
		Read(sample[n].fd,&flag,1);
		Read(sample[n].fd,&sample[n].length,4);
		LSeek(sample[n].fd,(int)header[1]-11,SEEK_CUR);
		sample[n].start = LSeek(sample[n].fd,0,SEEK_CUR);
	}
	sample[n].bits = flag & 4 ? 16 : 8;
	sample[n].channels = flag & 16 ? 2 : 1;
	sample[n].compressed = flag & 3;
	sample[n].length &= 0xFFFFFFFC;
	return True;
}

Boolean
AudioMgrDir::CheckWAVEFormat(int n)
{
// Check if sample n is in WAVE format.  If it is, set the file pointer
// to the first 'playable' byte and return True.
// Also, set the following:
//			sample[n].start		- relative start position
//			sample[n].length		- sample length
//			sample[n].rate			- sample rate
//			sample[n].bits			- bits per sample
//			sample[n].channels	- mono or stero
//			sample[n].compressed	- compression flag

char*	sptr = 0;
int slen;
int extra, br;
char	okRIFF, okWAVE, okFMT;
#pragma pack(push,1)
	struct {
		char	id[4];
		long	len;
	} chunk;
	struct {
		short	fmttag;
		short	channels;
		long	rate;
		long	bytespersec;
		short	blockalign;
		short	bits;	
	} wf;
#pragma pack(pop)

	if (!sample[n].fd) {

		sptr = (char*)*sample[n].memID;
		slen = sample[n].memID.Size();
		okRIFF = okWAVE = okFMT = 0;
		while(1) {
			if (slen < sizeof(chunk))
				break;
			memcpy((char*)&chunk,sptr,sizeof(chunk));
			sptr += sizeof(chunk); 
			slen -= sizeof(chunk);

			if (!strncmp(chunk.id,"RIFF",4)) {
				okRIFF = 1;
				continue;
			}

			if (!strncmp(chunk.id,"WAVE",4)) {
				// WAVE chunk does not have a length dword:
				sptr -= sizeof(chunk.len);
				slen += sizeof(chunk.len);
				okWAVE = 1;
				continue;
			}

			if (!strncmp(chunk.id,"fmt ",4)) {
				if (slen < sizeof(wf))
					break;
				memcpy((char *)&wf,sptr,sizeof(wf));
				sptr += sizeof(wf);
				slen -= sizeof(wf);
				br = sizeof(wf);
				if (wf.fmttag == 2) {   // compressed wave file
					if (slen < sizeof(int))
						break;
					memcpy(&extra,sptr,sizeof(int));
					sptr += sizeof(int) + extra;
					slen -= sizeof(int) + extra;
					br += sizeof(int) + extra;
					sample[n].compressed = 3;  // 4-to-1 compression
				} else
					sample[n].compressed = 0;
				sample[n].rate = (int)wf.rate;
				sample[n].bits = (uchar)wf.bits;
				if (sample[n].compressed  || (sample[n].bits != 8 && sample[n].bits != 16))
					msgMgr->Fatal("Can't play compressed WAVE audio");
				sample[n].channels = (uchar)wf.channels;
				sptr += chunk.len - br + (chunk.len & 1);
				slen -= chunk.len - br + (chunk.len & 1);
				okFMT = 1;
				continue;
			}

			if (!strncmp(chunk.id,"data",4)) {
				if (!okRIFF || !okWAVE || !okFMT)
					break;
				if (!sample[n].fd) {
					sample[n].start = sptr - (char*)*sample[n].memID;
				} else {
					sample[n].start = LSeek(sample[n].fd,0,SEEK_CUR);
				}
				sample[n].length = chunk.len & 0xFFFFFFFC;
				return True;
			}

			// Unknown chunk type -- skip over it:
			if (!okRIFF)
				break;
			sptr += chunk.len + (chunk.len & 1);
			slen -= chunk.len + (chunk.len & 1);
		}

	} else {

		LSeek(sample[n].fd,sample[n].fileOfs,SEEK_SET);
		okRIFF = okWAVE = okFMT = 0;
		while(1) {
			if (Read(sample[n].fd,(char*)&chunk,sizeof(chunk)) != sizeof(chunk))
				break;

			if (!strncmp(chunk.id,"RIFF",4)) {
				okRIFF = 1;
				continue;
			}

			if (!strncmp(chunk.id,"WAVE",4)) {
				// WAVE chunk does not have a length dword:
				LSeek(sample[n].fd,0 - sizeof(chunk.len),SEEK_CUR);
			okWAVE = 1;
			continue;
			}

			if (!strncmp(chunk.id,"fmt ",4)) {
				if (Read(sample[n].fd,(char *)&wf,sizeof(wf)) != sizeof(wf))
					break;
				br = sizeof(wf);
				if (wf.fmttag == 2) {   // compressed wave file
					if (Read(sample[n].fd,&extra,sizeof(int)) != sizeof(int))
						break;
					LSeek(sample[n].fd,extra,SEEK_CUR);
					br += sizeof(int) + extra;
					sample[n].compressed = 3;  // 4-to-1 compression
				} else
					sample[n].compressed = 0;
				sample[n].rate = (int)wf.rate;
				sample[n].bits = (uchar)wf.bits;
				if (sample[n].compressed  || (sample[n].bits != 8 && sample[n].bits != 16))
					msgMgr->Fatal("Can't play compressed WAVE audio");
				sample[n].channels = (uchar)wf.channels;
				LSeek(sample[n].fd,chunk.len - br + (chunk.len & 1),SEEK_CUR);
				okFMT = 1;
				continue;
			}

			if (!strncmp(chunk.id,"data",4)) {
				if (!okRIFF || !okWAVE || !okFMT)
					break;
				sample[n].start = LSeek(sample[n].fd,0,SEEK_CUR);
				sample[n].length = chunk.len & 0xFFFFFFFC;
				return True;
			}

			// Unknown chunk type -- skip over it:
			if (!okRIFF)
				break;
			LSeek(sample[n].fd,chunk.len + (chunk.len & 1),SEEK_CUR);
		}
	}

	return False;
}


void
AudioMgrDir::PauseWaveDev(void)
{
	if (dSoundInitialized) {
		AudioPause(ALL_SAMPLES);
		StopTimer();
	}
}

void
AudioMgrDir::ResumeWaveDev(void)
{
	if (GetActiveWindow() == NULL)
		return;

	if (dSoundInitialized) {
  		StartTimer();
		AudioResume(ALL_SAMPLES);
	}
}

void
AudioMgrDir::PollWaveBuffs()
// Not needed for direct sound, only to avoid link errors
{
}

void
AudioMgrDir::EnterCritical(void)
{
	EnterCriticalSection(&memCrit);
#ifdef DEBUG
	critSectionCount++;
	assert(critSectionCount < 5);
#endif
}

void
AudioMgrDir::LeaveCritical(void)
{
#ifdef DEBUG
	critSectionCount--;
	assert(critSectionCount >= 0);
#endif
	LeaveCriticalSection(&memCrit);
}

void
AudServer(void)
{
	thisAud->ServiceAud();
}

void
AudioMgrDir::ServiceAud(void)

// Audio service routine.  Must be called frequently enough to keep
// up with audio.
//
// This is a timer callback, which means that it runs as a separate
// thread and can interrupt other code.
//
// For each active sample:
//			- stop the sample if it is finished playing
//			- process fades
//			- call the dsound service routine
//			- back fill the feed buffer if it is used
//			- determine finish time if sample is exhausted

{
int		n;
DWORD		playPos, nextPos, feedLen;
ULONG		currTime;
int		fadeElapsed, newVol;
BOOL		serviceResult;

	// do not re-enter
	if (serviceSentry)
		return;
	serviceSentry = TRUE;
	EnterCritical();

	currTime = timeGetTime();

	for (n = 0; n < activeSamples; n++) {
		if (sample[n].nowPlaying) {

			// Stop if done playing
			if (!sample[n].dSnd->IsPlaying()) {
				MarkStopped(n);
				continue;
			}
			if (sample[n].finishTime && (currTime > sample[n].finishTime)) {
				MarkStopped(n);
				continue;
			}
		
			// Check if currently doing a fade
			if (sample[n].fadeStartTime) {
				// compute elapsed fade time
				fadeElapsed = currTime - sample[n].fadeStartTime;

				// If the fade is finished, reset the fade flag and either
				// stop the sound or set volume to its final value
				if (fadeElapsed > sample[n].fadeTotTime) {
					sample[n].fadeStartTime = 0;
					if (sample[n].fadeStop)
						MarkStopped(n);
					else
						AudioVolume(sample[n].fadeEndVol,n);
					continue;
				}

				// Fade not finished.  Compute the new volume based on the
				// elapsed time and the slope of the fade.
				// Y = X(y2 - y1)/x2 + y2
				//		where Y is volume and X is time
				newVol = ((fadeElapsed * (sample[n].fadeEndVol - sample[n].fadeStartVol))
							/ sample[n].fadeTotTime) + sample[n].fadeStartVol;
				AudioVolume(newVol,n);
			}
		}
	}

	// Call service routine for each playing, streaming sample
	for (n = 0; n < activeSamples; n++) {
		if (sample[n].nowPlaying && sample[n].streaming) {
			// Now call the service routine
#ifndef DEBUG
			threadAssert(memMgr->IsValid(sample[n].playID),n);
#endif
			serviceResult = sample[n].dSnd->Service((char *)*sample[n].playID + sample[n].start);
#ifdef DEBUG
			if (!serviceResult) {
				ThreadAlert(sample[n].dSnd->GetErrorMsg());
				serviceSentry = FALSE;
				LeaveCritical();
			 	return;
			}
#endif
		}
	}

	// Back fill the feed buffer for each sample which uses it
	for (n = 0; n < activeSamples; n++) {
		if (sample[n].feedID.IsAllocated() && sample[n].nowPlaying && !sample[n].robot) {
			// Compute the fill length up to the current read position
			playPos = sample[n].dSnd->GetReadPos();
			nextPos = sample[n].feedBuffPos;
			// check for wrap-around
			if (nextPos <= playPos)
				feedLen = playPos - nextPos;
			else
				feedLen = sample[n].feedBuffSize - nextPos + playPos;
			// if the length is zero, either it's a full buffer fill
			// or else the audio is stuck and we have a problem
			if (feedLen == 0) {
				if ((playPos == 0) && (nextPos == sample[n].feedBuffSize))
					feedLen = sample[n].feedBuffSize;
				else
					continue;
			}

			// Now, fill the buffer
			FillBuffer(n,nextPos,feedLen);

			// If sample finished, compute finish time
			if ((sample[n].sampReadPos == (DWORD)sample[n].length) &&
				 (!sample[n].loop) &&
				 (sample[n].finishTime == 0)) {
				sample[n].finishTime = timeGetTime() + 
											GetMSec(n,sample[n].feedBuffSize) +
											GetMSec(n,sample[n].dSoundBuffSize);
			}
		}
	}
	serviceSentry = FALSE;
	LeaveCritical();
}


// The following two routines are for debugging purposes.  It is meant to
// be activated from a hotkey when a problem is detected.  Characteristics
// of the currently playing samples are dumped to DUMP.TXT, and the contents
// of the current buffers are dumped.  Define DODUMP or comment out the
// ifdef's to activate.
// Hotkey can be defined in eventw.cpp to call ((AudioMgrDir*)audioMgr)->DumpAudio()

//#define	DODUMP	1

void
AudioMgrDir::DumpAudio(void)
{
#ifdef DODUMP
static int dumpCount = 0;

	EnterCritical();

	dumpCount++;

	sprintf(mbuff,"start dump:%d",dumpCount);
	MBox(mbuff,"");

	// output info to the DUMP.TXT file
	sprintf(mbuff,"\ndump:%d **********************************",dumpCount);
	msgMgr->Dump(mbuff);

	for (int n = 0; n < activeSamples; n++) {
		msgMgr->Dump("\n");
		sprintf(mbuff,"samp:%d num:%d rate:%d bits:%d chan:%d fd:%d comp:%d len:%d loop:%d rbt:%d vmd:%d stop:%d playing:%d\n",
			n,
			sample[n].num,
			sample[n].rate,
			sample[n].bits,
			sample[n].channels,
			sample[n].fd,
			sample[n].compressed,
			sample[n].length,
			sample[n].loop,
			sample[n].robot,
			sample[n].vmd,
			sample[n].stopped,
			sample[n].nowPlaying
			);
		msgMgr->Dump(mbuff);

		// dump buffer contents to disk
		DumpMemID(sample[n].memID, "memID", dumpCount, n);
		DumpMemID(sample[n].feedID,"feedID",dumpCount, n);
		DumpMemID(sample[n].fileID,"fileID",dumpCount, n);

		// dump direct sound buffer to disk
		sample[n].dSnd->DumpBuffer(dumpCount,n);
	}

	MBox("end dump","");
	LeaveCritical();
#endif
}

void
AudioMgrDir::DumpMemID(MemID memID, char *name, int dumpCount, int n)
{
#ifdef DODUMP
int	fd;

	if (memID) {
		sprintf(mbuff,"  %s type:%s attrs:%x size:%d\n",
			name,
			memID.GetMemTypeStr(),
			memID.Attrs(),
			memID.Size()
			);
		msgMgr->Dump(mbuff);

		sprintf(mbuff,"%s.%d%d",name,dumpCount,n);
		msgMgr->Dump("  dumped to:%s\n",mbuff);

		fd = open(mbuff, O_CREAT | O_TRUNC | O_BINARY | O_RDWR, S_IWRITE);
		write(fd,(char *)*memID,memID.Size());
		close(fd);
	}
#else
memID=memID;
name=name;
dumpCount=dumpCount;
n=n;
#endif
}

#ifdef DEBUG
void
AudioMgrDir::ThreadAssertFail(int n, char* file, int line, char* expression)

// Special assert routine for calling from within the timer callback thread.
// This is necessary since displays from a thread don't work well.
// We just format a buffer and set a flag so that the next AsyncEventCheck
// in sciwin.cpp will display it and quit.

{
#ifndef DEBUG
	sprintf(mbuff,"line:%d expr:%s\nsamp:%d fd:%d len:%d feed:%d cmp:%d loop:%d",
		line,expression,
		sample[n].num,
		sample[n].fd,
		sample[n].length,
		sample[n].feedID,
		sample[n].compressed,
		sample[n].loop
		);
	threadErrorFlag = TRUE;
#endif
//	codeSentry++;
}

void
AudioMgrDir::ThreadAlert(char *message)
{
#ifndef DEBUG
	strcpy(mbuff,message);
	threadErrorFlag = TRUE;
#endif
}
#endif

/*******************************************************************************
 *  FUNCTION: 		GetSampleNumber
 *
 *  PARAMETERS:	int iResNum			 - resource number of the sample
 *						int* iSampleNumber -	int set to sample number, if found
 *						int iTag				 - tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Retrieves the sample number of the specified resource number
 *						and optional tag combination
 *
 *  RETURNS:		Boolean - True if sample found, False otherwise 
 ******************************************************************************/
Boolean
AudioMgrDir::GetSampleNumber(int iResNum, int* iSampleNumber, int iTag)   
{
	GetActiveSamples();
	for (int i = 0; i < activeSamples; i++) 
		if (sample[i].num == iResNum && sample[i].module == SFXMODNUM) 
			if (!iTag || (iTag == sample[i].tag)) {
				*iSampleNumber = i;
				return True;
			}
	return False;
}
/*******************************************************************************
 *  FUNCTION: 		GetSampleNumber
 *
 *  PARAMETERS:	int iModule			 - module number of the sample
 *						int iNoun			 - noun value of the sample
 *						int iVerb			 - verb value of the sample
 *						int iCond			 - condition value of the sample
 *						int iSequ			 - sequence value of the sample
 *						int* iSampleNumber -	int set to sample number, if found
 *
 *  DESCRIPTION: 	Retrieves the sample number of the specified module, noun
 *						verb, condition, sequence combination
 *
 *  RETURNS:		Boolean - True if sample found, False otherwise 
 ******************************************************************************/
Boolean
AudioMgrDir::GetSampleNumber(int iModule, int iNoun, int iVerb, int iCond, 
										int iSequ, int* iSampleNumber)   
{
	GetActiveSamples();
	for (int i = 0; i < activeSamples; i++) 
		if (!sample[i].robot && 
			 !sample[i].vmd &&
			 sample[i].num == 0 &&
			 sample[i].module == (ResNum)iModule &&
			 sample[i].noun == (uchar)iNoun &&
			 sample[i].verb == (uchar)iVerb &&
			 sample[i].cond == (uchar)iCond &&
			 sample[i].sequ == (uchar)iSequ)
		{

			*iSampleNumber = i;
			return True;
		}
	return False;
}

int
AudioMgrDir::WAudioPlay(Boolean licensePlate, int iResNum, int iTag, int iModule, int iNoun,
			int iVerb, int iCond, int iSequ, Boolean fLoop, int iVolume,
			Boolean fPause)
// Generalized play function for both sound effects and speech.  The
// first parameter determines which type.
{
Boolean	sampleFound;
int		n;

	// protect from timer callback
	EnterCritical();

	startTime = timeGetTime();
	int iTimeToPlay = 0;
	int iSampleNumber;

	// See if we are already playing the sample
	if (licensePlate)
		sampleFound = GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber);
	else
		sampleFound = GetSampleNumber(iResNum, &iSampleNumber, iTag);

	// If so, then this is just a resume
	if (sampleFound && sample[iSampleNumber].pausedLoc) {
			AudioResume(iSampleNumber);
			LeaveCritical();
			return sample[iSampleNumber].ticks;
	}

	// are we max'ed out?
	if (activeSamples == MAXSAMPLES) {
		#ifndef AUDIO_LIB
		msgMgr->Alert("Cannot play more than %d samples",MAXSAMPLES);
//		#else
//		msgMgr->Fatal("Cannot play more than %d samples",MAXSAMPLES);
		#endif
		LeaveCritical();
		return 0;
	}

	n = GetActiveSamples();

	// Set up the sample (unprotect since there may be a read)
	LeaveCritical();
	if (licensePlate)
		iTimeToPlay = AudioSelect(n,
											iModule,
											0,
											(uchar)iNoun,
											(uchar)iVerb,
											(uchar)iCond,
											(uchar)iSequ,
											iTag,
											fLoop);
	else
		iTimeToPlay = AudioSelect(n,
											SFXMODNUM,
											iResNum,
											(uchar)0,
											(uchar)0,
											(uchar)0,
											(uchar)0,
											iTag,
											fLoop);

	// Not successful, return
	if (iTimeToPlay == 0) {
		return 0;
	}

	EnterCritical();

	// Everything OK, add new sample to the table
	activeSamples++;

	// Save the play time
	sample[n].ticks = iTimeToPlay;
			
	// Set volume
	if (iVolume > MAXVOLUME) {
		sample[n].volume = MAXVOLUME;
		prioritySample   = n;				
	} else
		sample[n].volume = iVolume;

	if (fPause)
		sample[n].pausedLoc = 1;
	else
		sample[n].pausedLoc = 0;

	// Start playing the sample
	AudioPlay(n);

	if (fPause)
		sample[n].pausedLoc = sample[n].startLoc;					

//	sprintf(mbuff,"end play1:%d time:%d\n",iResNum,timeGetTime() - startTime);
//	OutputDebugString(mbuff);
	LeaveCritical();
	return iTimeToPlay;
}



//***************** Kernel call interface routines ****************************


/*******************************************************************************
 *  FUNCTION: 		ImpInitializeAudioDriver
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION:	One-time call to initialize audio support.
 *                No other audio functions should be called before this function.
 *
 *  RETURNS:		Boolean - True if audio is installed, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpInitializeAudioDriver()
{
	return InitAudioDriver();
}

/*******************************************************************************
 *  FUNCTION: 		ImpTerminateAudioDriver
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION:	One-time call to terminate audio support.
 *                No other audio functions should be called after this function.
 *
 *  RETURNS:		Nothing
 ******************************************************************************/
void
AudioMgrDir::ImpTerminateAudioDriver()
{
	EndAudio();
}

/******************************************************************************
 *  FUNCTION: 		ImpAudioInstalled
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION:	Returns the value of the audio installed flag
 *
 *  RETURNS:		Boolean - True if audio is installed, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioInstalled()
{
	return audioInstalled;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioPreload
 *
 *  PARAMETERS:	int iValue - value to set the preLoad indicator
 *
 *  DESCRIPTION: 	Sets the value of the internal preload variable which 
 *						indicates which audio resources to preload, starting with 
 *						the value specified.  E.g. if 2 is specified, the 2nd, third, 
 *						etc. samples will be preloaded.  If the value is 1, all samples 
 *						will be preloaded.
 *
 *  RETURNS:		int - the current preLoad setting
 ******************************************************************************/
int
AudioMgrDir::ImpAudioPreload(int iValue)   
{
	if	(iValue > 0)
		preLoad = iValue;			
				
	return preLoad;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioWaitToPlay
 *
 *  PARAMETERS:	int iResNum    - resource number of the sample
 *						Boolean fLoop	- True to loop sample indefinitely,
 *											  False to play sample once
 *						int iVolume    - volume level 
 *										     0     - muted
 *										     1-127 - increasing loudness
 *										     128   - priority sample, mute all other samples
 *													    until this one is done playing at full
 *													    volume
 *						int iTag			- identifier to allow use of same sample more
 *										     than once, any value > 0
 *					
 *  DESCRIPTION: 	Add the sample waits for Resume to play it.  If the sample exists and
 *						is paused, just resumes playing it.
 *
 *  RETURNS:		int - time in 1/60 seconds to play selection, 0 means no
 *							   selection to play 
 ******************************************************************************/
int
AudioMgrDir::ImpAudioWaitToPlay(int iResNum, Boolean fLoop, int iVolume, int iTag)   
{
	return WAudioPlay(True,iResNum,iTag,0,0,0,0,0,fLoop,iVolume,True);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioWaitToPlay
 *
 *  PARAMETERS:	int iModule		- module number of the sample
 *						int iNoun		- noun value of the sample
 *						int iVerb		- verb value of the sample
 *						int iCond		- condition value of the sample
 *						int iSequ		- sequence value of the sample
 *						Boolean fLoop	- True to loop sample indefinitely,
 *											  False to play sample once
 *						int iVolume    - volume level 
 *										    0     - muted
 *										    1-127 - increasing loudness
 *										    128   - priority sample, mute all other samples
 *													   until this one is done playing at full
 *													   volume
 *					
 *  DESCRIPTION: 	Add the sample, but waits for Resume to play it.  If the 
 *						sample exists and	is paused, just resumes playing it.
 *
 *  RETURNS:		int - time in 1/60 seconds to play selection, 0 means no
 *							   selection to play 
 ******************************************************************************/
int
AudioMgrDir::ImpAudioWaitToPlay(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
								  Boolean fLoop, int iVolume)   
{
	return WAudioPlay(False,0,0,iModule,iNoun,iVerb,iCond,iSequ,fLoop,iVolume,True);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioActiveSamples
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION:	Returns the number of samples currently active
 *
 *  RETURNS:		int - number of active samples
 ******************************************************************************/
int
AudioMgrDir::ImpAudioActiveSamples()
{
	// GWP Incase we're not being called by our timer routine which happens
	// even if it registers. Call the Time service routine.
	TimeFunc(0, 0, 0, 0, 0);
	
	return GetActiveSamples();
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioPlay
 *
 *  PARAMETERS:	int iResNum    - resource number of the sample
 *						Boolean fLoop	- True to loop sample indefinitely,
 *											  False to play sample once
 *						int iVolume    - volume level 
 *										     0     - muted
 *										     1-127 - increasing loudness
 *										     128   - priority sample, mute all other samples
 *													    until this one is done playing at full
 *													    volume
 *						int iTag			- identifier to allow use of same sample more
 *										     than once, any value > 0
 *						long fPause    - True if wait to play, False otherwise.  This is
 *											  a long as opposed to a Boolean to avoid ambiguity.	
 *											  (Note - don't specify, call ImpAudioWaitToPlay
 *												instead)
 *					
 *  DESCRIPTION: 	Add the sample and starts playing it.  If the sample exists and
 *						is paused, just resumes playing it.
 *
 *  RETURNS:		int - time in 1/60 seconds to play selection, 0 means no
 *							   selection to play 
 ******************************************************************************/
int
AudioMgrDir::ImpAudioPlay(int iResNum, Boolean fLoop, int iVolume, int iTag, 
						  long fPause)   
{
	// Stop any samples using the same soundObject (tag)
	if (iTag) {
		GetActiveSamples();
		for (int n = 0; n < activeSamples; n++) {
			if (sample[n].tag == iTag && sample[n].num != iResNum) {
				ImpAudioStop(sample[n].num, iTag);
				break;
			}
		}
	}

	return WAudioPlay(False,iResNum,iTag,0,0,0,0,0,fLoop,iVolume,fPause);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioPlay
 *
 *  PARAMETERS:	int iModule		- module number of the sample
 *						int iNoun		- noun value of the sample
 *						int iVerb		- verb value of the sample
 *						int iCond		- condition value of the sample
 *						int iSequ		- sequence value of the sample
 *						Boolean fLoop	- True to loop sample indefinitely,
 *											  False to play sample once
 *						int iVolume    - volume level 
 *										    0     - muted
 *										    1-127 - increasing loudness
 *										    128   - priority sample, mute all other samples
 *													   until this one is done playing at full
 *													   volume
 *						Boolean fPause - True if wait to play, False otherwise
 *											  (Note - don't specify, call ImpAudioWaitToPlay
 *												instead)
 *					
 *  DESCRIPTION: 	Add the sample and starts playing it.  If the sample exists and
 *						is paused, just resumes playing it.
 *
 *  RETURNS:		int - time in 1/60 seconds to play selection, 0 means no
 *							   selection to play 
 ******************************************************************************/
int
AudioMgrDir::ImpAudioPlay(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
						  Boolean fLoop, int iVolume, Boolean fPause)  
{
	return WAudioPlay(True,0,0,iModule,iNoun,iVerb,iCond,iSequ,fLoop,iVolume,fPause);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioStopAll
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Stops all the playing samples
 *
 *  RETURNS:		int - number of active samples - should be 0
 ******************************************************************************/
void
AudioMgrDir::ImpAudioStopAll()   
{
	// protect from timer callback
	EnterCritical();
	AudioStop(ALL_SAMPLES);
	LeaveCritical();
}


/*******************************************************************************
 *  FUNCTION: 		ImpAudioStop
 *
 *  PARAMETERS:	int iResNum			 - resource number of the sample
 *						int iTag				 - tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Stops playing the sample identified by iResNum and optional 
 *						iTag value combination
 *
 *  RETURNS:	   None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioStop(int iResNum, int iTag)   
{
	int iSampleNumber;

	// protect from timer callback
	EnterCritical();

	if (GetSampleNumber(iResNum, &iSampleNumber, iTag))
		AudioStop(iSampleNumber);

	LeaveCritical();
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioStop
 *
 *  PARAMETERS:	int iModule			 - module number of the sample
 *						int iNoun			 - noun value of the sample
 *						int iVerb			 - verb value of the sample
 *						int iCond			 - condition value of the sample
 *						int iSequ			 - sequence value of the sample
 *
 *  DESCRIPTION: 	Stops playing the sample identified by the module, noun, verb,
 *					   condition and sequence combination
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioStop(int iModule, int iNoun, int iVerb, int iCond, int iSequ)   
{
	int iSampleNumber;

	// protect from timer callback
	EnterCritical();

	if (GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber))
		AudioStop(iSampleNumber);

	LeaveCritical();
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioLoop
 *
 *  PARAMETERS:	int iResNum			 - resource number of the sample
 *						Boolean fLoop		 - True to loop sample indefinitely,
 *													False to play sample once
 *						int iTag				 - tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Loops the sample identified by iResNum and optional 
 *						iTag value combination the specified number of times
 *
 *  RETURNS:		void - True if one or more samples affected, False otherwise
 ******************************************************************************/
void
AudioMgrDir::ImpAudioLoop(int iResNum, Boolean fLoop, int iTag)   
{
	int iSampleNumber;

	if (GetSampleNumber(iResNum, &iSampleNumber, iTag))
		sample[iSampleNumber].loop = fLoop;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioLoop
 *
 *  PARAMETERS:	int iModule			 - module number of the sample
 *						int iNoun			 - noun value of the sample
 *						int iVerb			 - verb value of the sample
 *						int iCond			 - condition value of the sample
 *						int iSequ			 - sequence value of the sample
 *						Boolean fLoop		 - True to loop sample indefinitely,
 *													False to play sample once
 *						
 *  DESCRIPTION: 	Loops the sample identified by the module, noun, verb,
 *					   condition and sequence combination the specified number of
 *						times.
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioLoop(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
							Boolean fLoop)   
{
	int iSampleNumber;

	if (GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber))
		sample[iSampleNumber].loop = fLoop;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioPauseAll
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Pauses all the samples
 *
 *  RETURNS:		Boolean - True if one or more samples affected, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioPauseAll()   
{
	return AudioPause(ALL_SAMPLES);
}


/*******************************************************************************
 *  FUNCTION: 		ImpAudioPause
 *
 *  PARAMETERS:	int iResNum			 - resource number of the sample
 *						int iTag				 - tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Pauses the sample identified by iResNum and optional 
 *						iTag value combination
 *
 *  RETURNS:		Boolean - True if one or more samples affected, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioPause(int iResNum, int iTag)   
{
	int iSampleNumber;

	if (GetSampleNumber(iResNum, &iSampleNumber, iTag))
		return AudioPause(iSampleNumber);
	else
		return FALSE;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioPause
 *
 *  PARAMETERS:	int iModule			 - module number of the sample
 *						int iNoun			 - noun value of the sample
 *						int iVerb			 - verb value of the sample
 *						int iCond			 - condition value of the sample
 *						int iSequ			 - sequence value of the sample
 *
 *  DESCRIPTION: 	Stops playing the sample identified by the module, noun, verb,
 *					   condition and sequence combination
 *
 *  RETURNS:		Boolean - True if one or more samples affected, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioPause(int iModule, int iNoun, int iVerb, int iCond, int iSequ)   
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if (GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber))
		return AudioPause(iSampleNumber);
	else
		return False;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioResumeAll
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Resumes all the samples
 *
 *  RETURNS:		Boolean - True if one or more samples affected, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioResumeAll()   
{
	return AudioResume(ALL_SAMPLES);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioResume
 *
 *  PARAMETERS:	int iResNum			 - resource number of the sample
 *						int iTag				 - tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Resumes the sample identified by iResNum and optional 
 *						iTag value combination
 *
 *  RETURNS:		Boolean - True if one or more samples affected, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioResume(int iResNum, int iTag)   
{
	int iSampleNumber;

	if (GetSampleNumber(iResNum, &iSampleNumber, iTag))
		return AudioResume(iSampleNumber);
	else
		return False;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioResume
 *
 *  PARAMETERS:	int iModule			 - module number of the sample
 *						int iNoun			 - noun value of the sample
 *						int iVerb			 - verb value of the sample
 *						int iCond			 - condition value of the sample
 *						int iSequ			 - sequence value of the sample
 *
 *  DESCRIPTION: 	Resumes the sample identified by the module, noun, verb,
 *					   condition and sequence combination
 *
 *  RETURNS:		Boolean - True if one or more samples affected, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioResume(int iModule, int iNoun, int iVerb, int iCond, int iSequ)   
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if (GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber))
		return AudioResume(iSampleNumber);
	else
		return False;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDurationAll
 *
 *  PARAMETERS:	int* iDuration - pointer to int to update with duration value
 *
 *  DESCRIPTION: 	Updates iDuration with the duration in 60ths of a second since 
 *						the start of the initial sample
 *
 *  RETURNS:		Boolean - True if duration successfully determined, False
 *									 otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioDurationAll(int* iDuration)   
{
	*iDuration = AudioLoc(ALL_SAMPLES);
	if (*iDuration < 0)
		return False;
	else
		return True;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDuration
 *
 *  PARAMETERS:	int iResNum			 - resource number of the sample
 *						int* iDuration     - pointer to int to update with duration value
*						int iTag				 - tag associated with sample (0 if none)
 *						
 *  DESCRIPTION: 	Updates iDuration with the duration in 60ths of a second since
 *						the start of the sample identified by iResNum and optional iTag
 *						value combination
 *
 *  RETURNS:		Boolean - True if duration successfully determined, False
 *									 otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioDuration(int iResNum, int* iDuration, int iTag)   
{
	int iSampleNumber;

	if (GetSampleNumber(iResNum, &iSampleNumber, iTag)) {
		*iDuration = AudioLoc(iSampleNumber);
		if (*iDuration < 0)
			return False;
		else
			return True;
	}
	return False;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDuration
 *
 *  PARAMETERS:	int iModule			 - module number of the sample
 *						int iNoun			 - noun value of the sample
 *						int iVerb			 - verb value of the sample
 *						int iCond			 - condition value of the sample
 *						int iSequ			 - sequence value of the sample
 *						int* iDuration     -	pointer to int to update with duration value
 *						
 *   DESCRIPTION: Updates iDuration with the duration in 60ths of a second since
 *						the start of the sample identified by module, nooun, verb,
 *						condition and sequence combination
 *
 *  RETURNS:		Boolean - True if duration successfully determined, False
 *									 otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioDuration(int iModule, int iNoun, int iVerb, int iCond,
									 int iSequ, int *iDuration)   
{
	int iSampleNumber;

	if (GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber)) {
		*iDuration = AudioLoc(iSampleNumber);
		if (*iDuration < 0)
			return False;
		else
			return True;
	}
	return False;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioSetRate
 *
 *  PARAMETERS:	int iRate - the rate at which to play back the samples
 *
 *  DESCRIPTION: 	Sets the audio samples to playback at the specified rate, or
 *						the maximum rate of the DAC, whichever is less.
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioSetRate(int iRate)
{
	rate = iRate;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDefaultRate
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Sets the audio samples to playback at their recorded rate
 *					   or the maximum rate of the DAC, whichever is less.
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioDefaultRate()
{
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioCurrentRate
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Returns the current playback rate
 *
 *  RETURNS:		int - playback rate
 ******************************************************************************/
int
AudioMgrDir::ImpAudioCurrentRate()
{
	return rate;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioSetBits
 *
 *  PARAMETERS:	int iBits - the bit rate at which to play back the samples
 *
 *  DESCRIPTION: 	Sets the audio samples to playback at the specified bit rate, or
 *						the maximum bit-rate of the DAC, whichever is less.
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioSetBits(int iBits)
{
	bits = iBits;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDefaultBits
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Sets the audio samples to playback at their recorded bit rate
 *					   or the maximum bit rate of the DAC, whichever is less.
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioDefaultBits()
{
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioCurrentBits
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Returns the current bit rate
 *
 *  RETURNS:		int - bit rate
 ******************************************************************************/
int
AudioMgrDir::ImpAudioCurrentBits()
{
	return bits;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioSetChannels
 *
 *  PARAMETERS:	int iChannels - the number of channels which to play back the samples
 *
 *  DESCRIPTION: 	Sets the audio samples to playback at the specified number of channels, or
 *						the maximum channels of the DAC, whichever is less.
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioSetChannels(int iChannels)
{
	channels = iChannels;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDefaultChannels                       
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Sets the audio samples to playback at their recorded number of 
 *					   channels or the maximum number of channels of the DAC, whichever
 *						is less.
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioDefaultChannels()
{
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioCurrentChannels
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Returns the current number of channels
 *
 *  RETURNS:		int - number of channels
 ******************************************************************************/
int
AudioMgrDir::ImpAudioCurrentChannels()
{
	return channels;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioGetVolReduction
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Gets the volume reduction setting (whether simultaneous samples 
 *						will be reduced in volume when playing to prevent overflow)
 *
 *  RETURNS:		Boolean - True if volume reduction is on, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioGetVolReduction()
{
	return((Boolean)mixCheck);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioSetVolReduction
 *
 *  PARAMETERS:	Boolean fFlag - whether to use volume reduction on simultaneous
 *											 samples (True for yes, False for no)
 *
 *  DESCRIPTION: 	Sets the volume reduction setting (whether simultaneous samples 
 *						will be reduced in volume when playing to prevent overflow) to
 *						the specified value
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioSetVolReduction(Boolean fFlag)
{
	int iSave = mixCheck;

	mixCheck  = fFlag;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioGetDACCritical
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Returns the value of the DAC critical flag, which syncs a robot
 *						that plays with existing samples if False, or allows a delay
 *						in the robot audio until it merges in if True
 *
 *
 *  RETURNS:		Boolean - True if DAC critical flag is on, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioGetDACCritical()
{
	return critical;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioSetDACCritical
 *
 *  PARAMETERS:	Boolean fFlag - True if there will be a delay in playing robot
 *											 audio, False if existing samples will be stopped
 *											 and restarted at the same point when a robot
 *											 with audio is started
 *
 *  DESCRIPTION: 	Sets the value of the DAC critical flag, which syncs a robot
 *						that plays with existing samples if False, or allows a delay
 *						in the robot audio until it merges in if True
 *						Note: this function takes no action if the DACCritical
 *						entry is specified in the config file, and the value is
 *						set to True
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioSetDACCritical(Boolean fFlag)
{
	if (!DACCritical)
	{
		critical = fFlag;
	}
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDistortAll
 *
 *  PARAMETERS:	int iDistortionMask - mask used for distortion
 *													 0 - distortion off
 *													 1 - 255 - minimum to maximum distortion
 *
 *  DESCRIPTION: 	Distorts all current samples using the specified mask
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioDistortAll(int iDistortionMask)   
{
	AudioDistort(iDistortionMask, ALL_SAMPLES);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDistort
 *
 *  PARAMETERS:	int iResNum			  - resource number of the sample
 *						int iDistortionMask - mask used for distortion
 *													 0 - distortion off
 *													 1 - 255 - minimum to maximum distortion
 *						int iTag				  - tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Distorts the sample identified by iResNum and optional 
 *						iTag value combination using the specified mask
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioDistort(int iResNum, int iDistortionMask, int iTag)   
{
	int iSampleNumber;

	if ( GetSampleNumber(iResNum, &iSampleNumber, iTag) )
		AudioDistort(iDistortionMask, iSampleNumber);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDistort
 *
 *  PARAMETERS:	int iModule			 - module number of the sample
 *						int iNoun			 - noun value of the sample
 *						int iVerb			 - verb value of the sample
 *						int iCond			 - condition value of the sample
 *						int iSequ			 - sequence value of the sample
 *						int iDistortionMask - mask used for distortion
 *													 0 - distortion off
 *													 1 - 255 - minimum to maximum distortion
 *						
 *  DESCRIPTION: 	Distortts the sample identified by the module, noun, verb,
 *					   condition and sequence combination using the specified mask
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioDistort(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
								int iDistortionMask)   
{
	int iSampleNumber;

	if ( GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
		AudioDistort(iDistortionMask, iSampleNumber);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioGlobalVolume
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Returns the value of the global volume level across all samples
 *
 *  RETURNS:		int - volume level
 ******************************************************************************/
int
AudioMgrDir::ImpAudioGlobalVolume()   
{
	return masterVolume;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioSampleVolume
 *
 *  PARAMETERS:	int iResNum	- resource number of the sample
 *						int* iVolume - pointer to int to update with volume level
						int iTag		- tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Retrieves the volume of the sample identified by iResNum and optional 
 *						iTag value combination 
 *
 *  RETURNS:		Boolean - True if sample found, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioSampleVolume(int iResNum, int* iVolume, int iTag)   
{
	int iSampleNumber;

	if (GetSampleNumber(iResNum, &iSampleNumber, iTag)) {
		*iVolume = sample[iSampleNumber].volume;			
		return True;
	} else
		return False;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioSampleVolume
 *
 *  PARAMETERS:	int iModule	  - module number of the sample
 *						int iNoun	  - noun value of the sample
 *						int iVerb	  - verb value of the sample
 *						int iCond	  - condition value of the sample
 *						int iSequ	  - sequence value of the sample
 *						int* iVolume - pointer to int to update with volume level
 *						
 *  DESCRIPTION: 	Retrieves the volume of the sample identified by the module, noun, verb,
 *					   condition and sequence combination to the specified value.
 *
 *  RETURNS:		True if sample found, False othewise
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioSampleVolume(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
										  int* iVolume)   
{
	int iSampleNumber;

	if (GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber)) {
		*iVolume = sample[iSampleNumber].volume;			
		return True;
	} else
		return False;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioVolumeAll
 *
 *  PARAMETERS:	int iVolume - volume level 
 *										  0     - muted
 *										  1-127 - increasing loudness
 *
 *  DESCRIPTION: 	Sets the volume level of all samples
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioVolumeAll(int iVolume)   
{
	AudioVolume(iVolume, ALL_SAMPLES);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioVolume
 *
 *  PARAMETERS:	int iResNum	- resource number of the sample
 *						int iVolume - volume level 
 *										  0     - muted
 *										  1-127 - increasing loudness
 *										  128   - priority sample, mute all other samples
 *													 until this one is done playing at full
 *													 volume
 *						int iTag		- tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Sets the volume of the sample identified by iResNum and optional 
 *						iTag value combination to the specified value
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioVolume(int iResNum, int iVolume, int iTag)   
{
	int iSampleNumber;

	if (GetSampleNumber(iResNum, &iSampleNumber, iTag))
		AudioVolume(iVolume, iSampleNumber);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioVolume
 *
 *  PARAMETERS:	int iModule	  - module number of the sample
 *						int iNoun	  - noun value of the sample
 *						int iVerb	  - verb value of the sample
 *						int iCond	  - condition value of the sample
 *						int iSequ	  - sequence value of the sample
 *						int iVolume   - volume level 
 *										    0     - muted
 *										    1-127 - increasing loudness
 *										    128   - priority sample, mute all other samples
 *													   until this one is done playing at full
 *													   volume
 *						
 *  DESCRIPTION: 	Sets the volume of the sample identified by the module, noun, verb,
 *					   condition and sequence combination to the specified value.
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioVolume(int iModule, int iNoun, int iVerb, int iCond, int iSequ, int iVolume)   
{
	int iSampleNumber;

	if (GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber))
		AudioVolume(iVolume, iSampleNumber);
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioFade	- note 2 funcs, 1 for Win, 1 for DOS
 *
 *  PARAMETERS:	int iResNum	      - resource number of the sample
 *						int iTargetVolume - volume level to fade to 
 *						int iTicks	  		- number of 1/60 seconds between each
 *												  incremental volume change
 *						int iStep         - the number of incremental volume changes
 *												  to make to get to the target volume , if
 *												  0 , target volume will be reached at once
 *						Boolean fEnd 		- whether to end the sample when the
 *												  target volume is reached or
 *												  to continue playing the sample at
 *												  the target volume
 *						int iTag          - tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Fades the sample identified by iResNum and optional iTag
 *						combination to the target volume
 *
 *  RETURNS:		Boolean - True if fade successfully initiated, False
 *									 otherwise
 ******************************************************************************/
// Windows Version
Boolean
AudioMgrDir::ImpAudioFade(int iResNum, int iTargetVolume, int iTicks, int iSteps,
								Boolean fEnd, int iTag)
{
	int n;

	if (GetSampleNumber(iResNum, &n, iTag)) {
		sample[n].fadeStartVol	= sample[n].volume;
		sample[n].fadeEndVol		= iTargetVolume;
		sample[n].fadeTotTime 	= (iSteps * iTicks * 50) / 3;
		sample[n].fadeStop	 	= fEnd;
		sample[n].fadeStartTime	= timeGetTime();
		return True;
	} else
		return False;
}		


/*******************************************************************************
 *  FUNCTION: 		ImpAudioFade36
 *
 *  PARAMETERS:	int iModule			- module number of the sample
 *						int iNoun			- noun value of the sample
 *						int iVerb			- verb value of the sample
 *						int iCond			- condition value of the sample
 *						int iSequ			- sequence value of the sample
 *						int iTargetVolume - volume level to fade to 
 *						int iTicks	  		- number of 1/60 seconds between each
 *												  incremental volume change
 *						int iStep         - the number of incremental volume changes
 *												  to make to get to the target volume , if
 *												  0 , target volume will be reached at once
 *						Boolean fEnd 		- whether to end the sample when the
 *												  target volume is reached or
 *												  to continue playing the sample at
 *												  the target volume (note the default
 *												  is True unlike ImpAudioFade)
 *
 *  DESCRIPTION: 	Fades the sample identified by the module, noun, verb,
 *					   condition and sequence combination to the target volume
 *
 *  RETURNS:		Boolean - True if fade successfully initiated, False
 *									 otherwise
 ******************************************************************************/
 // Windows Version
 Boolean
AudioMgrDir::ImpAudioFade36(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
								   int iTargetVolume, int iTicks, int iSteps, Boolean fEnd)
{
	int n;

	if (GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &n)) {
		sample[n].fadeStartVol	= sample[n].volume;
		sample[n].fadeEndVol		= iTargetVolume;
		sample[n].fadeTotTime 	= (iSteps * iTicks * 50) / 3;
		sample[n].fadeStop	 	= fEnd;
		sample[n].fadeStartTime	= timeGetTime();
		return True;
	} else
		return False;
}		

/*******************************************************************************
 *  FUNCTION: 		ImpAudioCheckNoise
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION: 	Determines whether the priority sample is currently audible
 *
 *  RETURNS:		Boolean - True if priority sample is audible, False if not 
 *									 audible or no priority sample is playing
 ******************************************************************************/
Boolean
AudioMgrDir::ImpAudioCheckNoise()
{
	return( (Boolean)CheckNoise() );
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioPan
 *
 *  PARAMETERS:	int iResNum			 - resource number of the sample
 *						int iPanPercent	 - percentage to pan sample on right speaker,
 *												   left speaker will get remainder of this
 *													value subtracted from 100
 *						int iTag				 - tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Pans the sample identified by iResNum and optional 
 *						iTag value combination using the specified pan percentage 
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioPan(int iResNum, int iPanPercent, int iTag)   
{
	int iSampleNumber;

	if ( GetSampleNumber(iResNum, &iSampleNumber, iTag) ) {
		AudioPan(iSampleNumber, iPanPercent);
	} else {
		panResNum = iResNum;
		panPercent = iPanPercent;
	}
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioPan
 *
 *  PARAMETERS:	int iModule			 - module number of the sample
 *						int iNoun			 - noun value of the sample
 *						int iVerb			 - verb value of the sample
 *						int iCond			 - condition value of the sample
 *						int iSequ			 - sequence value of the sample
 *						int iPanPercent	 - percentage to pan sample on right speaker,
 *												   left speaker will get remainder of this
 *													value subtracted from 100
 *						
 *  DESCRIPTION: 	Pans the sample identified by the module, noun, verb,
 *					   condition and sequence combination using the specified
 *						pan percentage
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioPan(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
						  int iPanPercent)   
{
	int iSampleNumber;

	if ( GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
		AudioPan(iSampleNumber, iPanPercent);
//		sample[iSampleNumber].pan = iPanPercent;
}


/*******************************************************************************
 *  FUNCTION: 		ImpAudioPanOff
 *
 *  PARAMETERS:	int iResNum			 - resource number of the sample
 *						int iTag				 - tag associated with sample (0 if none)
 *
 *  DESCRIPTION: 	Turns of panning for the sample identified by iResNum and optional 
 *						iTag value combination
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioPanOff(int iResNum, int iTag)   
{
	int iSampleNumber;

	if ( GetSampleNumber(iResNum, &iSampleNumber, iTag) )
		AudioPan(iSampleNumber, -1);
//		sample[iSampleNumber].pan = -1;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioPanOff
 *
 *  PARAMETERS:	int iModule			 - module number of the sample
 *						int iNoun			 - noun value of the sample
 *						int iVerb			 - verb value of the sample
 *						int iCond			 - condition value of the sample
 *						int iSequ			 - sequence value of the sample
 *						
 *  DESCRIPTION: 	Turns off panning for the sample identified by the module, noun,
 *					   verb, condition and sequence combination 
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDir::ImpAudioPanOff(int iModule, int iNoun, int iVerb, int iCond, int iSequ)
{
	int iSampleNumber;

	if ( GetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
		AudioPan(iSampleNumber, -1);
//		sample[iSampleNumber].pan = -1;
}


// Special case of WPLAY kernel call - probably obsolete
int
AudioMgrDir::ImpAudioSampNotInMem()
{
	int saveCount = 0;
	for (int n = 0; n < activeSamples; n++)
		if (!sample[n].memID.IsAllocated())
			saveCount++;
	return saveCount;      
}

Boolean
AudioMgrDir::AudioPause(int n)
// Pause the specified samples
{
int t;

	int	ticks = timeMgr->GetTickCount();

	if (n == NO_SAMPLES)
		return False;

	if (n == ALL_SAMPLES) {
		GetActiveSamples();
		for (t = 0; t < activeSamples; t++) {
			if (!sample[t].pausedLoc) {
				sample[t].pausedLoc = ticks;
				sample[t].dSnd->Pause();
			}
		}
		return True;
	}

	if (n == ROBOTS_ONLY) {
		robotsPaused = True;
		GetActiveSamples();
		for (t = 0; t < activeSamples; t++) {
			if (sample[t].robot) {
				sample[t].pausedLoc = ticks;
			}
		}
		return False;
	}

	// pause a single sample
	if (!sample[n].pausedLoc) {
		sample[n].pausedLoc = ticks;
		sample[n].dSnd->Pause();
		return True;
	}

	return False;
}

Boolean
AudioMgrDir::AudioResume(int n)
// Resume the specified samples
{
int	t;

	int	ticks = timeMgr->GetTickCount();

	if (n == NO_SAMPLES)
		return False;

	if (n == ALL_SAMPLES) {
		GetActiveSamples();
		for (t = 0; t < activeSamples; t++) {
			if (sample[t].pausedLoc) {
				sample[t].startLoc += ticks - sample[t].pausedLoc;
				sample[t].pausedLoc = 0;
				sample[t].dSnd->Resume();
			}
		}
		return True;
	}

	if (n == ROBOTS_ONLY) {
		#ifndef AUDIO_LIB
		robotsPaused = False;
		GetActiveSamples();
		for (t = 0; t < activeSamples; t++) {
			if (sample[t].robot) {
				sample[t].startLoc += ticks - sample[t].pausedLoc;
				sample[t].pausedLoc = 0;
				RobotPlay(t);
				ffloc = -1;
				return True;
			}
		}
		#endif
		return False;
	}

	// resume a single sample
	if (sample[n].pausedLoc) {
		sample[n].startLoc += ticks - sample[n].pausedLoc;
		sample[n].pausedLoc = 0;
		sample[n].dSnd->Resume();
		return True;
	}

	return False;
}

void
AudioMgrDir::SubmitMaxBuffers()
{
}

void
AudioMgrDir::DACCallBack(int cbs,int ticksPlayed)
{
cbs=cbs;
ticksPlayed=ticksPlayed;
}

void
AudioMgrDir::AudioServer()
{
// (This is a DOS-only function) 
}

#ifndef AUDIO_LIB

//****************************************************************************
//***************** Non- Kernel call interface routines **********************
//****************************************************************************

Boolean
AudioMgrDir::AudioVMDStart(MemID vmdID,int vmdRate,int vmdBits, int vmdChannels,int vmdVolume)
// Start a VMD looping buffer at the specified address, rate, and bits.
// The VMD player will keep it filled.
{
int	n;
Boolean	result;

	// protect from timer callback
	EnterCritical();

	// get sample number
	n = vmdSample = GetActiveSamples();

	// set up parameters
	memset(&sample[n], 0, sizeof(Sample));
	sample[n].length	= vmdID.Size();
	sample[n].loop		= True;
	sample[n].volume	= vmdVolume;
	sample[n].vmd		= True;
	sample[n].rate		= vmdRate;
	sample[n].bits		= vmdBits;
	sample[n].channels = vmdChannels;
	sample[n].memID	= vmdID;
	sample[n].memID.Critical(True);
	sample[n].pan		= -1;
	sample[n].playID	= sample[n].memID;
	sample[n].streaming = TRUE;

	// establish buffer sizes
	SetBuffSize(n);

	// create the direct sound object
//	sample[n].dSnd = New DSound();
	sample[n].dSnd = CreateDSoundObject();

	// initialize dsound with looping buffer
	sample[n].dSnd->SetLoop(TRUE);
	result = sample[n].dSnd->Init(
					sample[n].dSoundBuffSize,
					(char *)*sample[n].memID + sample[n].start,
					sample[n].length,
					sample[n].bits,
					sample[n].rate,
					sample[n].channels);
	if (!result) {
		LeaveCritical();
		return 0;
	}
	sample[n].dSnd->SetVolume(ScaleVolume(n),volFactor);

	// Everything OK, add new sample to the table
	++activeSamples;

	// start playing
	AudioPlay(n);

	LeaveCritical();
	return True;
}

Boolean
AudioMgrDir::AudioVMDStop()
// Stop the VMD
{
	GetActiveSamples();
	for (int n = 0; n < activeSamples; n++) {
		if (sample[n].vmd) {
			AudioStop(n);
			return True;
		}
	}
	return False;
}

uint
AudioMgrDir::AudioVMDPosLoc()
// Return adjustment for resuming after pause
{
	return 0;
}

uint
AudioMgrDir::AudioVMDQuery()
// Return total number of samples played so far
{
DWORD	playPos;
uint	samples;

	int n = vmdSample;

	// protect from timer callback
	EnterCritical();

	// get total bytes played
	playPos = sample[n].dSnd->GetBytesPlayed();

	// convert to samples
	// (NOTE: Does not work correctly with channels factored in.  I
	//  don't know why)
	samples = (uint)playPos / (sample[n].bits/8 * sample[n].channels);
//	samples = (uint)playPos / (sample[n].bits/8);

	LeaveCritical();

	return samples;
}

Boolean
AudioMgrDir::AudioFree(int which)
// Stop all sound effects or all samples
// (called from memmgr.cpp)
{
	int	n;

	GetActiveSamples();
	switch (which) {
		case SFX_ONLY:
			for (n = 0; n < activeSamples; n++) {
				if (sample[n].dSnd && sample[n].memID && sample[n].module == SFXMODNUM) {
					AudioStop(n);
					return True;
				}
			}
			break;
		case ALL_SAMPLES:
			for (n = 0; n < activeSamples; n++) {
				if (sample[n].dSnd && sample[n].memID) {
					AudioStop(n);
					return True;
				}
			}
			break;
	}
	return False;
}

Boolean
AudioMgrDir::AudioLock(ResNum num, Boolean lock)
// If the named sfx is active and memory-resident, set its
// locked status as requested and return True; else return False
// (Note: there may be multiple copies of the same sample active)
// (called from kcall.cpp)
{
Boolean rc = False;

	GetActiveSamples();
	for (int n = 0; n < activeSamples; n++)
		if (sample[n].module == SFXMODNUM && sample[n].num == num && sample[n].memID) {
			if (lock)
				sample[n].attrs &= ~DISCARDABLE;
			else
				sample[n].attrs |= DISCARDABLE;
			rc = True;
		}
	return rc;
}

Boolean
AudioMgrDir::AudioPurge(ResNum num)
//
// If the named sfx is active and memory-resident, stop it
// and return True; else return False
// (Note: there may be multiple copies of the same sample active)
// (called from kcall.cpp)
{
	Boolean rc = False;
	GetActiveSamples();
	for (int n = 0; n < activeSamples; n++)
		if (sample[n].module == SFXMODNUM && sample[n].num == num && sample[n].memID) {
			AudioStop(n);
			rc = True;
		}
	return False;
}

Boolean
AudioMgrDir::AudioQueryDiscardable(ResNum num)
//
// If the named sfx is active and memory-resident, return the
// locked status it had prior to when it started playing;
// else return False
// (called from memmgr.cpp)
{

	GetActiveSamples();
	for (int n = 0; n < activeSamples; n++)
		if (sample[n].dSnd && sample[n].module == SFXMODNUM && sample[n].num == num)
			if (sample[n].memID)
				return (Boolean) (sample[n].attrs & DISCARDABLE);
			else
				return False;
	return False;
}

int
AudioMgrDir::FindAudEntry(ResNum id)
//
// Called from restype.cp; I don't know what it does
{
	int				offset;
	MemID				mapID;
	ResAudEntry*	entry;

	if (sfxVolFD == -1)
		return -1;

	if (!resMgr->Check(MemResMap, SFXMODNUM))
		return -1;
	mapID = resMgr->Get(MemResMap, SFXMODNUM);
	mapID.SetNotDiscardable();

	offset = 0;
	for (entry = (ResAudEntry *)*mapID; entry->id != (ushort)-1; ++entry) {
		offset += ((ulong)entry->offsetMSB << 16) + (ulong)entry->offsetLSW;
		if	(entry->id == id)
			return offset;
	}
	return -1;
}

int
AudioMgrDir::FindAud36Entry(ResNum module, uchar noun, uchar verb, uchar cond, uchar sequ)
{
	int				offset;
	MemID				mapID;
	char*				ptr36;
	ResAud36Entry*	entry36;

	if (audVolFD == -1)
		return(-1);

	if (!resMgr->Check(MemResMap, module))
		return(-1);
	mapID = resMgr->Get(MemResMap, module);

	ptr36 = (char *)*mapID;
	offset = *(int *)ptr36;
	ptr36 += 4;
	for (entry36 = (ResAud36Entry*)ptr36; entry36->flag.sequ != 255;
			entry36 = (ResAud36Entry*)ptr36) {
		offset += ((ulong)entry36->offsetMSB << 16) + (ulong)entry36->offsetLSW;
		if	(entry36->noun == noun && entry36->verb == verb &&
			 entry36->cond == cond && (entry36->flag.sequ & SEQUMASK) == sequ) {
			if (entry36->flag.sync & SYNCMASK) {
				offset += entry36->syncLen;
				if (entry36->flag.rave & RAVEMASK)
					offset += entry36->raveLen;
			}
			PreloadSync36(module,noun,verb,cond,sequ);
			return(offset);
		}
		ptr36 += sizeof(ResAud36Entry);
		if (!(entry36->flag.sync & SYNCMASK))
			ptr36 -= sizeof(entry36->syncLen);
		if (!(entry36->flag.rave & RAVEMASK))
			ptr36 -= sizeof(entry36->raveLen);
	}
	return -1;
}

void
AudioMgrDir::InitAudioVols()
{
	char		pathName[MaxPath + 1];
	char*		cp;

	/* Open optional Audio sound effects Volume */
	if (sfxVolFD != -1) {
		Close(sfxVolFD);
		resMgr->Release(MemResMap,SFXMODNUM);
	}
	strcpy(pathName, configMgr->Get("ressfx",0));
	if (strlen(pathName)) {
		cp = &pathName[strlen(pathName)-1];
		if (*cp != ':' && *cp != '\\')
			strcat(pathName,"\\");
	}
	strcat(pathName,SFXVOLNAME);
	sfxVolFD = Open(pathName, O_RDONLY);

	/* Open optional Base-36 Speech/Sync/Rave Volume */
	if (audVolFD != -1) {
		Close(audVolFD);
		ResNum num;
		while ((num = resMgr->FindType(MemResMap)) != (ResNum)-1)
			resMgr->Release(MemResMap,num);
	}
	strcpy(pathName, configMgr->Get("resaud",0));
	if (strlen(pathName)) {
		cp = &pathName[strlen(pathName)-1];
		if (*cp != ':' && *cp != '\\')
			strcat(pathName,"\\");
	}
	strcat(pathName,AUDVOLNAME);
	audVolFD = Open(pathName, O_RDONLY);
}

void
AudioMgrDir::MakeName36(MemType type, char* fname, ResNum module, uchar noun,
			  uchar verb, uchar cond, uchar sequ)
{
	if (type == MemResSync)
		fname[0] = 'S';
	else
		fname[0] = 'A';
	if (module >= 36*36*36) {
		fname[0]++;
		module %= 36*36*36;
	}
	ConvBase36(&fname[1],module,3);
	ConvBase36(&fname[4],(int)noun,2);
	ConvBase36(&fname[6],(int)verb,2);
	fname[8] = '.';
	ConvBase36(&fname[9],(int)cond,2);
	ConvBase36(&fname[11],(int)sequ,1);
	fname[12] = '\0';
}

Boolean
AudioMgrDir::QueryAudRobot(RobotAudStatus *buff)
// Query current robot status
// Input:	pointer to status structure
// Return:	True along with filled-in status structure if a robot
//				is playing.
{
	GetActiveSamples();
	for (int n = 0; n < activeSamples; n++) {
		if (sample[n].robot) {
			buff->bytesPlayed		= sample[n].dSnd->GetBytesPlayed();
			buff->bytesPlaying	= buff->bytesPlayed;
			buff->bytesSubmitted	= sample[n].dSnd->GetBytesRead();
			buff->rate = sample[n].rate;
			buff->bits = sample[n].bits;
			return True;
		}
	}
	buff->bytesPlayed = buff->bytesPlaying = buff->bytesSubmitted = 0;
	return False;
}

Boolean
AudioMgrDir::AudRobot(RobotAudInfo buff)
// Submit robot audio data for playback.
//
// Input:	RobotAudInfo structure
//			where:
//				buff.addrID		is the data address
//				buff.len			is the compressed length of the data
//									in bytes
//				buff.floc		is the byte start location of this data
//									relative to the start of the sample.
{
int	n;

	if (!audioInstalled)
		return True;

	// get the relevant data
	rbot.adrID	= buff.adrID;
	if (rbot.adrID)
		rbot.adrID.Critical(True);
	rbot.len		= buff.len;
	rbot.floc	= buff.floc;

	// ext is the byte sample-relative location this data extends to
	rbot.ext = rbot.floc +
				(rbot.len * (ROBOT_COMPRESSED + 1) * (ROBOT_BITS / 8));
	// channel is 0 or 1 depending on the start boundary
	rbot.channel = rbot.floc % 4 ? 1 : 0;

//	sprintf(mbuff,"Robot%d start:%d end:%d len:%d\n",
//				rbot.channel, rbot.floc, rbot.ext, rbot.len*4);
//	SciDisplay(mbuff);

	// Get current robot number if any
	GetActiveSamples();
	for (n = 0; n < activeSamples; n++)
		assert (!sample[n].vmd);
	for (n = 0; n < activeSamples; n++)
		if (sample[n].robot)
			break;

	// len of 0 means stop robot audio immediately
	if (rbot.len == 0) {
		if (n == activeSamples)
			return False;
		AudioStop(n);
		ffloc = -1;
		return True;
	}

	// len of -1 means stop when current data is finished
	if (rbot.len == -1) {
		if (n == activeSamples)
			return False;
		// compute finish time
		sample[n].finishTime = timeGetTime() +
						((rbot.blen * 8000) /
						(sample[n].rate * sample[n].bits * sample[n].channels));
		ffloc = -1;
		return True;
	}

	// protect from timer callback
	EnterCritical();

	// check if this is a new robot sample
	// if so, perform initialization
	if (rbot.floc <= 2 && ffloc == -1) {
		robotsActive = False;

		// Stop any current robot
		if (n < activeSamples)
			AudioStop(n);

		// Get new sample number
		n = GetActiveSamples();

		if (n == MAXSAMPLES) {
			LeaveCritical();
			return False;
		}

		// everything ok; add new sample
		++activeSamples;

		// set initial channel high water marks
		rbot.hwm[0] = 0;
		rbot.hwm[1] = 2;
		rbotSilenceHWM = 0;

		// set sample parameters
		memset(&sample[n], 0, sizeof(Sample));
		sample[n].length		= rbot.len;
		sample[n].volume		= MAXVOLUME;
		sample[n].robot		= True;
		sample[n].pan			= -1;
		sample[n].rate			= ROBOT_RATE;
		sample[n].bits			= ROBOT_BITS;
		sample[n].channels	= ROBOT_CHANNELS;
		sample[n].compressed = ROBOT_COMPRESSED;

		// establish buffer sizes
		SetBuffSize(n);

		// allocate feed buffer for direct sound
		sample[n].feedID.Get(MemAudioBuffer, sample[n].feedBuffSize, AUDIO_TEMP);
		sample[n].playID = sample[n].feedID;
		sample[n].playID.Critical(True);

		// create the direct sound object
		sample[n].dSnd = CreateDSoundObject();
		sample[n].dSnd->SetLoop(TRUE);

		// allocate buffer for decompression
		GetConvBuffer(convID,rbot.len * 2);
		convID.Critical(True);

		// location and length of destination buffer for all operations
		rbot.memID = sample[n].feedID;
		rbot.blen = rbot.memID.Size();

		// min & max are sample-relative byte positions of the feed
		// buffer; i.e. we cannot write before the min or after the
		// max.  min is always the current dsound bytes read position.
		rbot.min = 0;
		rbot.max = rbot.blen;

		// rbotReadPos is the current dsound buffer-relative read position
		rbotReadPos = 0;
		ffloc = rbot.floc;

		// prime the feed buffer with data from the first robot buffer
		FillRData(n, 0, rbot.blen, FD_SILENCE);
	  	FillRbotBuffer(n);

		LeaveCritical();
		return True;
	}

	// Normal case - continuing robot audio
	if (n == activeSamples) {
		LeaveCritical();
		return False;
	}

	// Get sample-relative min and max write positions
	rbot.min = sample[n].dSnd->GetBytesRead();
	rbot.max = rbot.min + rbot.blen;
	rbotReadPos = sample[n].dSnd->GetReadPos();

	// if we've already read past this point, then we don't need this piece
	if (rbot.ext <= Max(rbot.min,rbot.hwm[rbot.channel])) {
		LeaveCritical();
  		return True;
	}

	// if we're already at the max, then we can't use this piece
	if (rbot.max <= rbot.hwm[rbot.channel]) {
		LeaveCritical();
  		return False;
	}

	// now, do all the magic
  	FillRbotBuffer(n);

	// if this is the second buffer, then start the play
	if (ffloc != -1 && ffloc != rbot.floc && !robotsPaused) {
		if (!robotsActive) {
			RobotPlay(n);
		}
		ffloc = -1;
	}

	LeaveCritical();

	// if we couldn't use all of this piece, return False
  	if (rbot.ext > rbot.max)
  		return False;
	else
		return True;
}

void
AudioMgrDir::GetConvBuffer(MemID &id , int size)
{
	if (id){
		if ((int)id.Size() < size)
			id.Realloc(size);
	}
	else
		#ifndef AUDIO_LIB
		id.Get(MemAudioBuffer,size, AudioConvBufHandle);
		#else
		id.Get(size);
		#endif

}

void
AudioMgrDir::RobotPlay(int n)
// Start the robot playing
{
BOOL	result;

	robotsActive = True;
	// initialize the dsound object (it will prime its internal buffers)
	sample[n].dSoundBuffSize = Min(sample[n].dSoundBuffSize,rbot.blen);
	result = sample[n].dSnd->Init(
				sample[n].dSoundBuffSize,
				(char *)*rbot.memID,
				rbot.blen,
				sample[n].bits,
				sample[n].rate,
				sample[n].channels);
	// set the volume
	sample[n].dSnd->SetVolume(ScaleVolume(n),volFactor);
	// start playing
	sample[n].dSnd->Play((char *)*sample[n].playID + sample[n].start);

	sample[n].startLoc = timeMgr->GetTickCount();
	sample[n].nowPlaying = TRUE;
	sample[n].streaming = TRUE;
}

void
AudioMgrDir::FillRbotBuffer(int n)
//
// Fill the feed buffer with data from the current input buffer
// Note:
//		All position indexes are in bytes relative to the beginning
//		of the sample.
{
int srStart, srEnd, srThisHWM, srThatHWM;

	// adjust the size of the decompression buffer if necessary
	GetConvBuffer(convID,rbot.len * (sample[n].compressed + 1));

	// decompress the data
	memcpy((char *)*convID+rbot.len,*rbot.adrID,rbot.len);
	Decomp(*convID,rbot.len,1,16,1,0,0);

	// use local variables for high water marks
	srThisHWM	= rbot.hwm[rbot.channel];
	srThatHWM	= rbot.hwm[1-rbot.channel];

	// fill with silence up to the beginning of this piece if
	// nothing has been written there
	srStart = Max(rbot.min,Max(srThisHWM,srThatHWM));
	if (srStart < rbot.floc)
		FillRData(n, srStart, rbot.floc, FD_SILENCE);

	// copy every other sample	starting with the HWM for this channel
	srStart = Max(rbot.floc,srThisHWM);
	srEnd   = Min(rbot.ext,rbot.max);
	if (srStart < srEnd) {
		FillRData(n, srStart, srEnd, FD_EOS);
		rbot.hwm[rbot.channel] = srEnd;
	}

	// now dupe what's left after the HWM for the other channel
	srStart = Max(rbot.floc,Max(srThisHWM,srThatHWM - 2));
	if (srStart < srEnd) {
		FillRData(n, srStart, srEnd, FD_DUPE);
		rbot.hwm[rbot.channel] = srEnd;
	}

	// finlly, fill any remainder with silence
	srStart = Max(rbotSilenceHWM,Max(rbot.hwm[rbot.channel],srThatHWM));
	if (srStart < rbot.max) {
		FillRData(n, srStart, rbot.max, FD_SILENCE);
		rbotSilenceHWM = rbot.max;
	}
}

void
AudioMgrDir::FillRData(int n, int srStart, int srEnd, int type)
// Fill the feed buffer with data of type "type" from srStart to
// srEnd (sample-relative byte positions)
{
int brSrcPos, brDestPos, length, bytesToEnd;

	// compute buffer-relative source and destination positions
	// (source position used only for EOS (every other sample)  it is
	// divided by 2 since EOS is a 1 to 2 expansion)
	brSrcPos  = (srStart - rbot.floc) / 2;
	brDestPos = rbotReadPos + (srStart - rbot.min);
	// if at end of dest buff, reset to beginning
	if (brDestPos >= rbot.blen)
		brDestPos -= rbot.blen;

	// compute bytes to end of destination buffer
	bytesToEnd = rbot.blen - brDestPos;
	// compute byte length for move
	length = srEnd - srStart;

	// test for destination buffer wrap-around
	if (length <= bytesToEnd) {
		MoveRData(n, brSrcPos, brDestPos, length, type);
	} else {
		if (rbot.channel == 0) {
			MoveRData(n, brSrcPos, brDestPos, bytesToEnd, type);
			MoveRData(n, brSrcPos + (bytesToEnd / 2), 0, length - bytesToEnd, type);
		} else {
			if (type == FD_EOS)
				bytesToEnd += 2;
			MoveRData(n, brSrcPos, brDestPos, bytesToEnd, type);
			MoveRData(n, brSrcPos + (bytesToEnd / 2), 2, length - bytesToEnd, type);
		}
	}
}

void
AudioMgrDir::MoveRData(int n, int brSrcPos, int brDestPos, int lenBytes, int type)
//
// Fill feed buffer with lenBytes bytes of data of type "type".
// Start at buffer-relative position brDestPos in destination buffer.
// For EOS transfer, start at buffer-relative position brSrcPos in
// source buffer.
{
int	lenSamples;

	// get length in samples
	lenSamples = (lenBytes * 8) / (sample[n].bits * 2);

	// now, do the move based on type
	switch (type) {

		case FD_SILENCE:
			assert((brDestPos & 0x01) == 0);
			assert((brDestPos + lenBytes) <= (rbot.blen + 1));
			Silence(&rbot.memID[brDestPos],	// destination addr
						lenBytes,					// length in bytes
						sample[n].bits);			// sample size
			break;

		case FD_EOS:
			assert((brDestPos & 0x01) == 0);
			assert((brDestPos + lenBytes) <= (rbot.blen + 2));
//			assert((brSrcPos & 0x01) == 0);
			assert((brSrcPos/2 + lenSamples) < (rbot.len + 1));
			CopyEOS(&rbot.memID[brDestPos],	// destination addr
						&convID[brSrcPos],		//	source addr
						lenSamples,					// sample length
						sample[n].bits);			// sample size
			break;

		case FD_DUPE:
			assert((brDestPos & 0x01) == 0);
			assert((brDestPos + lenBytes) <= (rbot.blen + 1));
			DupeChannel(&rbot.memID[brDestPos],	// destination addr
							lenSamples,					// sample length
							1,
							sample[n].bits);			// sample size
			break;
	}
} 

#ifdef DEBUG   
void
AudioMgrDir::CheckDiscStreaming(char* str)
{
	char	pathName[MaxPath + 1];
	Boolean	dump = False;

	if (configMgr->Get(configMgr->AudioMonitor)) {
		if (!configMgr->Arg(configMgr->AudioMonitor) ||
				configMgr->Val(configMgr->AudioMonitor) == 0)
			return;
		if (configMgr->Val(configMgr->AudioMonitor) == 1)
			dump = True;
	}
	#ifdef ROBOT
	if (graphMgr &&
		 graphMgr->RobotStatus() != ROBOT_UNUSED &&
		 graphMgr->GRobot().IsRobotRealTime()) {
		if (dump) {
			msgMgr->Dump("Loading %s while playing %d.RBT in room #%d\n",
				str,graphMgr->GRobot().GetResNum(),currentRoom);
		} else if (!msgMgr->Alert
				("Loading %s while playing %d.RBT in room #%d\r(Press ESC/No to quit)",
					str,graphMgr->GRobot().GetResNum(),currentRoom)) {
			msgMgr->Fatal("User abort");
		}
	}
	#endif
	for (int n = 0; n < activeSamples; n++) {
		if (!sample[n].dSnd || sample[n].robot)
			continue;
		if (sample[n].fd == sfxVolFD) {
			if (dump) {
				msgMgr->Dump("Loading %s while playing audio #%d in room #%d\n",
					str,sample[n].num,currentRoom);
			} else if (!msgMgr->Alert
				("Loading %s while playing audio #%d in room #%d\r(Press ESC/No to quit)",
					str,sample[n].num,currentRoom)) {
				msgMgr->Fatal("User abort");
			}
		} else if (sample[n].fd == audVolFD) {
			MakeName36(MemResAudio, pathName, (ResNum)sample[n].module,
			 sample[n].noun, sample[n].verb, sample[n].cond, sample[n].sequ);
			if (dump) {
				msgMgr->Dump("Loading %s while playing %s in room #%d\n",
					str,pathName,currentRoom);
			} else if (!msgMgr->Alert
					("Loading %s while playing %s in room #%d\r(Press ESC/No to quit)",
						str,pathName,currentRoom)) {
				msgMgr->Fatal("User abort");
			}
		}
	}
	return;
}
#endif  // DEBUG
#endif  // AUDIO_LIB

