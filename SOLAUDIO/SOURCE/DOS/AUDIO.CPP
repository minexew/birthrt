// audio.cpp
//		digital audio sample playback
#include <stdio.h>
#include <sys\stat.h>

#include "audio.hpp"
#include "audiodos.hpp"

#ifdef AUDIO_LIB

#include <dos.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <string.h>
#include "custmsys.hpp"

#else	 // !AUDIO_LIB

#include "sol.hpp"
#include "config.hpp"
#include "dos.hpp"
#include "graphmgr.hpp"
#include	"kernel.hpp"
#include "memid.hpp"
#include "memmgr.hpp"
#include "msg.hpp"
#include "newroom.hpp"
#include "pmachine.hpp"
#include "resource.hpp"
#include	"sync.hpp"
#include	"time.hpp"
#ifdef DEBUG
#include "event.hpp"
#include	"kbdmgr.hpp"
#endif // DEBUG

#endif // AUDIO_LIB


//*******************  BEGIN COMMON AUDIO SECTION  **************************
int
GetNum(char *str, int defVal)
{
	#ifdef AUDIO_LIB
	return GetNumEntry(str, defVal);
	#else
	return configMgr->GetNum(str, 0, defVal);
	#endif
}

int
GetTicks()
{
// Return the application's elapsed time in 60th seconds
	#ifdef AUDIO_LIB
	return GetElapsedTicks();
	#else
	return timeMgr->GetTickCount();
	#endif
}
//*******************  END COMMON AUDIO SECTION  ****************************

// Codes to be sent to the audio driver.
enum	audioCodes {
	DAC_RELEASE_MEM = 1,
	DAC_INIT,
	DAC_CONTINUE,
	DAC_TERMINATE,
	DAC_PLAY,
	DAC_STOP,
	DAC_PAUSE,
	DAC_RESUME,
	DAC_LOC,
	DAC_SET_RATE,
	DAC_SET_BITS,
	DAC_SET_CHANNELS,
	DAC_SET_VOLUME,
	DAC_SET_IRQ,
	DAC_SET_DMA,
	DAC_SET_BUFF,
	DAC_GET_ID,
	DAC_GET_VER,
	DAC_GET_REALOC,
	DAC_GET_MAX_CHNLS,
	DAC_GET_MAX_BITS,
	DAC_GET_MAX_RATE,
	DAC_GET_VOL,
	DAC_GET_BUFF_SEL,
	DAC_GET_BUFF_OFS,
};

extern "C" {
void MemCopy(void *, void *, int);
void Merge(void *, void *, int, char, char);
void Distort(void *, int, char, char);
void Volume(void *, int, char, char, char);
void Silence(void *, int, char);
void Decomp(void *, int, int, int, int, int, int);
int  DecompGet(int);
void ConvBits(void *, int, int);
void ConvChannels(void *, int, int, int);
void ConvRate(void *, int, int, int, int);
void CopyEOS(void *, void *, int, char);
void DupeChannel(void *, int, int, char);
int  MonitorNoise(void *, int, char);
void Pan(void *, int, int);
}

// if building for register based calls, do the
// following pragmas so that the link will work
// with our asm module.  Used for stand-alone
// audio alone, for now.

#ifdef REG_CALL_BUILD

#pragma aux STACK_BASED "*" parm caller []; 

#pragma aux (STACK_BASED) MemCopy;
#pragma aux (STACK_BASED) Merge;
#pragma aux (STACK_BASED) Distort;
#pragma aux (STACK_BASED) Volume;
#pragma aux (STACK_BASED) Silence;
#pragma aux (STACK_BASED) Decomp;
#pragma aux (STACK_BASED) DecompGet;
#pragma aux (STACK_BASED) ConvBits;
#pragma aux (STACK_BASED) ConvChannels;
#pragma aux (STACK_BASED) ConvRate;
#pragma aux (STACK_BASED) CopyEOS;
#pragma aux (STACK_BASED) DupeChannel;
#pragma aux (STACK_BASED) MonitorNoise;
#pragma aux (STACK_BASED) Pan;

#endif

typedef int (CODE)(int,void*);

#ifdef REG_CALL_BUILD

#pragma aux (CODE,STACK_BASED);

#endif

union {
	void *data;
	CODE *code;
} audioDriver;

union {
	_Packed struct {
		int	ofs;
		short	sel;
		int	ofsOrig;
	} adr;
	char far *ptr;
} DACBuffer;

#define AUDIO_TEMP MOVEABLE+TRANSITORY
#define AUDIO_PERM MOVEABLE+PERSIST

MemID	audioDrv;

Boolean	audioInstalled		= False;
int		audVolFD				= -1;
int		sfxVolFD				= -1;
int		activeSamples		= 0;


AudioMgr	*audioMgr;

AudioMgrDOS::AudioMgrDOS()
{
	activeSamples = 0;
	convID = 0;
	convIDx = 0;
	critical = False;
	distortion = 0;
	ffloc = -1;
	mbTickCount = 0;
	mixCheck = True;
	pausedLoc = 0;
	preLoad = 0;
	prioritySample = -1;
	rate = 0;
	bits = 0;
	channels = 0;
	volume = MAXVOLUME;
	rateSet = False;
	bitsSet = False;
	channelsSet = False;
	staticFilter = False;

	#ifndef AUDIO_LIB
	audVolFD = -1;
	sfxVolFD = -1;
	robotsPaused = False;
	#endif
}

AudioMgrDOS::~AudioMgrDOS()
{
	EndAudio();
}

unsigned
AudioMgrDOS::GetDACDefaultSize(void)
{
	return DAC_HBUFFSIZE*2;
}

void *
AudioMgrDOS::LoadAudioDrv(void)
{
	#ifndef AUDIO_LIB
	char* fileName = configMgr->Get("audioDrv");
	if (!fileName || !*fileName)
		return 0;

	/* Load the audio driver.
	 */
	if (!(audioDrv.Load(MemDriver, fileName, PERSIST, AudioDriverHandle))) {
		msgMgr->Alert(Msg_NoAudioDrv, fileName);
		return 0;
	}
	#else
	audioDrv.Load(GetAudioDriverName());
	#endif

	return (*audioDrv);
}

void
AudioMgrDOS::FreeAudioDrv(void)
{
	audioDrv.Free();
}

Boolean
AudioMgrDOS::InitAudioDriver()
{
	unsigned	n, dSize, aSize;

	dSize = GetNum("DACSize", GetDACDefaultSize());
	if (!dSize)
		return False;

	aSize = GetNum("audioSize", INT_HBUFFSIZE*2);
	if (!aSize)
		return False;

	AdjustAudBuffSizes(&aSize,&dSize);

	intHbufferSize = aSize/2;
	DACHbufferSize = dSize/2;

	/* If audio is not requested -- vamboose
	 */

	audioDriver.data = LoadAudioDrv();
	if (!audioDriver.data)
		return False;

 	if ((DACArgsIn[1] = GetNum("audioIRQ",0)) != 0)
		CallDAC(DAC_SET_IRQ,1);
	if ((DACArgsIn[1] = GetNum("audioDMA",0)) != 0)
		CallDAC(DAC_SET_DMA,1);

	if ((n = CallDAC(DAC_INIT,0)) != 0) {
		#ifndef AUDIO_LIB
		if (n != NO_DAC)
			msgMgr->Alert(SrcLoc, Msg_NoAudio);
		#endif
		FreeAudioDrv();
		return False;
	}

	channelsMax = CallDAC(DAC_GET_MAX_CHNLS,0);
	bitsMax = CallDAC(DAC_GET_MAX_BITS,0);
	rateMax = CallDAC(DAC_GET_MAX_RATE,0);
	volumeMax = CallDAC(DAC_GET_VOL,0);

	if (bitsMax == 8)
		DACHbufferSize /= 2;

	if (!GetDACBuffer())
		return False;

	#ifndef AUDIO_LIB
	convID.Get(MemAudioBuffer, DACHbufferSize, AudioConvBufHandle);
	convIDx.Get(MemAudioBuffer, DACHbufferSize, AudioSubmitBufHandle);
	#else
	convID.Get(DACHbufferSize);
	convIDx.Get(DACHbufferSize);
	#endif

	audioInstalled = True;
	
//	AudioChannels(2);
//	channelsSet = True;
	AudioRate(22050);
	rateSet = True;
//GEH	AudioBits(16);
	AudioBits(8);
	bitsSet = True;

#ifndef AUDIO_LIB
	char* str = configMgr->Get("staticFilter");
	if (str && (!strnicmp(str,"y",1) || !strnicmp(str,"t",1) || !strncmp(str,"1",1)))
		staticFilter = True;
	else
		staticFilter = False;
	#ifdef DEBUG
	if (configMgr->Get(configMgr->PlayAudio))
		DACPlay();
	#endif
#endif

	return True;
}

int
AudioMgrDOS::FindSampleN(short* arg,int argn,int tag)
{
	// argCount = # kernel args (subfunction takes one)
	// argn = # additional args prior to possible file number(s)
	int	n;

	// return the sample number if found; else return activeSamples

	if (arg[0] - 1 - argn <= 0)
		return ALL_SAMPLES;

	if (!activeSamples)
		return NO_SAMPLES;

	if (arg[0] - 1 - argn < 5) {
		for (n = 0; n < activeSamples; n++) {
			if (sample[n].module != SFXMODNUM)
				continue;
			if (sample[n].num == arg[2+argn]) {
				if (tag && tag == sample[n].tag)
					return n;
			} else if (tag && tag == sample[n].tag)
				AudioStop(n--);
		}
	} else for (n = 0; n < activeSamples; n++) {
		if (
				#ifndef AUDIO_LIB
				!sample[n].robot && !sample[n].vmd &&
				#endif
				sample[n].num == 0 &&
				sample[n].module == (ResNum)arg[2+argn] &&
				sample[n].noun == (uchar)arg[3+argn] &&
				sample[n].verb == (uchar)arg[4+argn] &&
				sample[n].cond == (uchar)arg[5+argn] &&
				sample[n].sequ == (uchar)arg[6+argn])
			return n;
	}
	return NO_SAMPLES;
}

int
AudioMgrDOS::AudioSelect(int n)
{
	int	fd;
	char	pathName[MaxPath + 1] = {""};

	sample[n].patch = False;
	sample[n].fd = 0;
	sample[n].fadeSteps = 0;
	sample[n].distortion = 0;
	sample[n].fileOfs = 0;
	sample[n].pan = -1;
	#ifndef AUDIO_LIB
	sample[n].attrs = 0;
	if (!sample[n].vmd)
	#endif
		sample[n].memID = 0;

	#ifndef AUDIO_LIB
	if (sample[n].robot) {

		sample[n].rate = ROBOT_RATE;
		sample[n].bits = ROBOT_BITS;
		sample[n].channels = ROBOT_CHANNELS;
		sample[n].compressed = 1;

	} else if (sample[n].vmd) {

		;

	} else if (sample[n].module == SFXMODNUM) {

		// Search for AUD/WAV resource:

		if ((sample[n].memID = resMgr->Find(MemResAudio,sample[n].num)) ||
			 (sample[n].memID = resMgr->Find(MemResWAVE, sample[n].num))) {
			sample[n].attrs = sample[n].memID.Attrs();
			memMgr->SetNotDiscardable(sample[n].memID);
/*
		int	ndx;
		// FIRST: Search for .AUD file via 'patchDir=' dir list
		} else if ((ndx = FindPatchEntry(MemResAudio, sample[n].num)) != -1) {
			sprintf(pathName, "%s%s", patchDir[ndx], resMgr->MakeName(MemResAudio, sample[n].num));
			sample[n].fd = Open(pathName, 0);
			sample[n].patch = True;

		// SECOND: Search for .WAV file via 'patchDir=' dir list
		} else if ((ndx = FindPatchEntry(MemResWAVE, sample[n].num)) != -1) {
			sprintf(pathName, "%s%s", patchDir[ndx], resMgr->MakeName(MemResWAVE, sample[n].num));
			sample[n].fd = Open(pathName, 0);
			sample[n].patch = True;
*/

		// THIRD: Search for .AUD file via 'audio=' dir list
		} else if ((fd = resMgr->Open(MemResAudio, sample[n].num, pathName)) != -1
				&& fd != sfxVolFD && fd != resVolFD && fd != resVolFDPre && fd != altVolFD) {
			sample[n].fd = fd;
			sample[n].patch = True;

		// FOURTH: Searh for .WAV file via 'wave=' dir list
		} else if ((fd = resMgr->Open(MemResWAVE, sample[n].num, pathName)) != -1
				&& fd != sfxVolFD && fd != resVolFD && fd != resVolFDPre && fd != altVolFD) {
			sample[n].fd = fd;
			sample[n].patch = True;

		// FIFTH: Search for .AUD resource in Resource Volume
		} else if (resMgr->Check(MemResAudio, (ResNum)sample[n].num) != 0) {
			sample[n].memID = resMgr->Get(MemResAudio, (ResNum)sample[n].num);
			sample[n].attrs = sample[n].memID.Attrs();
			memMgr->SetNotDiscardable(sample[n].memID);

		// SIXTH: Search for .WAV resource in Resource Volume
		} else if (resMgr->Check(MemResWAVE, (ResNum)sample[n].num) != 0) {
			sample[n].memID = resMgr->Get(MemResWAVE, (ResNum)sample[n].num);
			sample[n].attrs = sample[n].memID.Attrs();
			memMgr->SetNotDiscardable(sample[n].memID);

		// LAST CHANCE: Search for AUD/WAV resource in SFX Resource Volume
		} else if ((sample[n].fileOfs = FindAudEntry((ResNum)sample[n].num)) != -1) {
			sample[n].fd = sfxVolFD;
			#ifdef DEBUG   
			sprintf(pathName,"%d from SFX",sample[n].num);
			CheckDiscStreaming(pathName);
			#endif

		// AUD/WAV resource NOT FOUND!
		} else {
			return 0;
		}

	} else {

		// Search for @ resource:

		// FIRST: Search for @ file via 'audio=' dir list
		MakeName36(MemResAudio, pathName, (ResNum)sample[n].module, sample[n].noun,
			sample[n].verb, sample[n].cond, sample[n].sequ);
		if ((fd = resMgr->Open(MemResAudio36, (ResNum)-1, pathName)) != -1
				&& fd != audVolFD && fd != sfxVolFD) {
			sample[n].fd = fd;
			sample[n].patch = True;

		// LAST CHANCE: Search for @ resource in AUD Resource Volume */
		} else if ((sample[n].fileOfs = FindAud36Entry((ResNum)sample[n].module,
						sample[n].noun,sample[n].verb,sample[n].cond,sample[n].sequ))
					!= -1) {
			sample[n].fd = audVolFD;
			#ifdef DEBUG   
			CheckDiscStreaming(pathName);
			#endif

		// @ resource NOT FOUND!
		} else {
			return 0;
		}
	}
	#else  // AUDIO_LIB

	// Search for memory-resident audio
	if ((sample[n].memID = GetAdr(sample[n].num)) != 0) {
		;
	// Search for file-based audio
	} else if ((fd = Open(sample[n].num)) != -1) {
		sample[n].fd = fd;
		sample[n].patch = True;
	// audio NOT FOUND!
	} else {
		return 0;
	}
	#endif  // AUDIO_LIB

	if (
		#ifndef AUDIO_LIB
		!sample[n].robot && !sample[n].vmd &&
		#endif
		!CheckSOLFormat(n) && !CheckWAVEFormat(n)) {
		#ifndef AUDIO_LIB
		if (sample[n].module == SFXMODNUM)
		{
			msgMgr->Fatal("Invalid audio file format: %d",sample[n].num);
		}
		else
		{
			msgMgr->Fatal("Invalid audio file format: %s",pathName);
		}
		#else
//			Fatal(BAD_FORMAT,sample[n].num);
		#endif
	}

	if (n == 0) {
		if (!rateSet)
			AudioRate(sample[n].rate);
		if (!bitsSet)
			AudioBits(sample[n].bits);
		if (!channelsSet)
			AudioChannels(sample[n].channels);
	}
	if (prioritySample == n) {
		#ifndef AUDIO_LIB
		priorityID.Get(MemAudioBuffer, DACHbufferSize, AudioPriorityHandle);
		#else
		priorityID.Get(DACHbufferSize);
		#endif
		bytesPerTick = rate * (bits/8) / 60;
		if (bits == 16)
			bytesPerTick &= 0xfffe;
	}

	sample[n].div = 1;
	sample[n].mul = 1;
	sample[n].fileOfs = sample[n].start;
	sample[n].bytesToRead = sample[n].length;
	sample[n].bytesRead = 0;
	sample[n].bytesBuffered = 0;
	sample[n].bufferNumber = 0;
	sample[n].fillToggle = 0;

	if (sample[n].fd == 0)
		sample[n].bytesRead = sample[n].start;

	if (sample[n].compressed) {
		if (sample[n].bits == 16) {
			sample[n].compValL = 0;
			sample[n].compValR = 0;
		} else {
			sample[n].compValL = 0x80;
			sample[n].compValR = 0x80;
		}
		sample[n].div *= sample[n].compressed + 1;
	}

	if (sample[n].bits > bits)
		sample[n].mul *= sample[n].bits / bits;
	else if (sample[n].bits < bits)
		sample[n].div *= bits / sample[n].bits;

	if (sample[n].rate > rate)
		sample[n].mul *= sample[n].rate / rate;
	else if (sample[n].rate < rate)
		sample[n].div *= rate / sample[n].rate;

	if (sample[n].channels > channels)
		sample[n].mul *= sample[n].channels / channels;
	else if (sample[n].channels < channels)
		sample[n].div *= channels / sample[n].channels;

	int size = intHbufferSize * sample[n].mul;
	if ((int)convID.Size() < size)
		convID.Realloc(size);

	#ifndef AUDIO_LIB
	if (sample[n].fd && preLoad && n+1 >= preLoad) {
		sample[n].memID.Get(MemAudioBuffer, sample[n].length, AUDIO_TEMP+DISCARDABLE);
		sample[n].attrs = sample[n].memID.Attrs();
		memMgr->SetNotDiscardable(sample[n].memID);
		sample[n].memID.Read(sample[n].fd);
		if (sample[n].patch) {
			sample[n].patch = False;
			Close(sample[n].fd);
		}
		sample[n].fd = 0;
		sample[n].start = 0;
	}

	if (!sample[n].vmd) {
		sample[n].buffID.Get(MemAudioBuffer,
			intHbufferSize * (sample[n].robot? 4:2), AUDIO_TEMP);
		sample[n].buffID.Critical(True);
	}
	#else
	// GWP Only allocate just enough space for the buffer. (2x the sound size)
	if (sample[n].memID.IsAllocated())
	{
		// Buffers must be multiples of DACHbufferSize
		int size = ((sample[n].memID.Size()/DACHbufferSize) + 1 ) * DACHbufferSize;
	    
	    if (size < intHbufferSize)
	    {
			sample[n].buffID.Get(size*2);
		}
		else
		{
			sample[n].buffID.Get(intHbufferSize*2);
		}
	}
	else
	{
		sample[n].buffID.Get(intHbufferSize*2);
	}
	#endif
	FillBuffer(n);
	FillBuffer(n);

	return 1 +
			(sample[n].length * (sample[n].compressed+1) * 60) /
			(sample[n].bits/8 * sample[n].channels * (int)sample[n].rate);
}

void
AudioMgrDOS::AudioPlay()
{
	AudioCheck();
	if (activeSamples == 1) {
		callBacks = 0;
		DACBuffer.adr.ofs = DACBuffer.adr.ofsOrig;
		MergeBuffers();
		DACArgsIn[1] = MergeBuffers();
		CallDAC(DAC_PLAY,1);  // begin playing...
		mbTickCount = GetTicks();
		sample[activeSamples-1].restartLoc = 0;
	} else if (
			#ifndef AUDIO_LIB
			(sample[activeSamples-1].robot || sample[activeSamples-1].vmd) &&
			#endif
			rateSet && bitsSet) {
		CallDAC(DAC_STOP,0);
		callBacks = 0;
		sample[MAXSAMPLES] = sample[activeSamples-1];
		sample[activeSamples-1] = sample[0];
		sample[0] = sample[MAXSAMPLES];
		DACBuffer.adr.ofs = DACBuffer.adr.ofsOrig;
		MergeBuffers();
		
		for (int n = 1; n < activeSamples; n++) {
			// GWP if (sample[n].bytesBuffered <= intHbufferSize && sample[n].bytesToRead)
			if (sample[n].bytesBuffered <= (sample[n].buffID.Size()/2) && sample[n].bytesToRead)
				FillBuffer(n);
		}
		DACArgsIn[1] = MergeBuffers();
		CallDAC(DAC_PLAY,1);  // begin playing...
		mbTickCount = GetTicks();
		sample[activeSamples-1].restartLoc = 0;
	} else if (lastDAC) {
		lastDAC = False;
		// continue playing (i.e. ignore prior "LAST BUFFER" message)
		CallDAC(DAC_CONTINUE,0);
		sample[activeSamples-1].restartLoc = 1;
	} else {
		sample[activeSamples-1].restartLoc = 2;
	}
	sample[activeSamples-1].startLoc = GetTicks();
	if (activeSamples == 1)
		globalLoc = sample[0].startLoc;
}

void
AudioMgrDOS::AudioStop(int n)
{
	int	t;

	if (n == NO_SAMPLES || !activeSamples)
		return;
	if (n == ALL_SAMPLES) {
		// Stop all samples
		for (t = 0; t < activeSamples; t++)
			DropSample(t);
		activeSamples = 0;
	} else {
		DropSample(n);
		--activeSamples;
		for (t = n; t < activeSamples; t++) {
			sample[t] = sample[t+1];
			if (t+1 == prioritySample)
				prioritySample = t;
		}
	}
	if (!activeSamples) {
		// No samples remain for playback...
		CallDAC(DAC_STOP,0);
		convID.Realloc(DACHbufferSize);
	}
}

void
AudioMgrDOS::DropSample(int n)
{
	if (sample[n].fd && sample[n].patch){
		Close(sample[n].fd);
		#ifndef AUDIO_LIB
		if (sample[n].swapped){
			resMgr->DeleteHandleFile(sample[n].handleFile,"swp");
		}
		#endif
		sample[n].memID.Free();
	}
	// GWP BEGIN FIX TO BAD CODE.
	else
	{
		// not file-based; mark buffer as discardable
		int dups;

		for (dups = 0;dups < activeSamples;dups++) 
			if (dups != n && sample[dups].memID.IsAllocated && (*sample[dups].memID == *sample[n].memID) )
				break;
				
		if (dups == activeSamples)
		{
			sample[n].memID.Free();
		}
	}
	// GWP END FIX TO BAD CODE.
	
	#ifndef AUDIO_LIB
	if ((int)sample[n].memID && (sample[n].attrs & DISCARDABLE))
		memMgr->SetDiscardable(sample[n].memID);
	if (!sample[n].vmd)
	#endif
		sample[n].buffID.Free();
	if (n == prioritySample) {
		prioritySample = -1;
		priorityID.Free();
	}
}

Boolean
AudioMgrDOS::AudioPause(int n)
{
	int	ticks = GetTicks();

	if (n == NO_SAMPLES)
		return False;
	if (n == ALL_SAMPLES) {
		if (!pausedLoc) {
			pausedLoc = ticks;
			return True;
		}
	#ifndef AUDIO_LIB
	} else if (n == ROBOTS_ONLY) {
		robotsPaused = True;
		for (n = 0; n < activeSamples; n++) {
			if (sample[n].robot) {
				sample[n].pausedLoc = ticks;
			}
		}
	#endif
	} else if (!sample[n].pausedLoc) {
		sample[n].pausedLoc = ticks;
		return True;
	}
	return False;
}

Boolean
AudioMgrDOS::AudioResume(int n)
{
	int	t, ticks = GetTicks();

	if (n == NO_SAMPLES)
		return False;
	if (n == ALL_SAMPLES) {
		if (pausedLoc) {
			for (t = 0; t < activeSamples; t++)
				if (!sample[t].pausedLoc)
					sample[t].startLoc += ticks - pausedLoc;
			globalLoc += ticks - pausedLoc;
			pausedLoc = 0;
			return True;
		}
	#ifndef AUDIO_LIB
	} else if (n == ROBOTS_ONLY) {
		robotsPaused = False;
		for (n = 0; n < activeSamples; n++) {
			if (sample[n].robot) {
				sample[n].startLoc += ticks - sample[n].pausedLoc;
				sample[n].pausedLoc = 0;
				AudioPlay();
				ffloc = -1;
				return True;
			}
		}
	#endif
	} else if (sample[n].pausedLoc) {
		sample[n].startLoc += ticks - sample[n].pausedLoc;
		sample[n].pausedLoc = 0;
		return True;
	}
	return False;
}

int
AudioMgrDOS::AudioLoc(int n)
{
/* Notes on LOC:
	A return value of -1 indicates that no audio is currently playing.
	A return value of 0 through 65534 indicates the time in sixtieths of
	a second that the current audio selection has been playing for.
*/

	int	ticks = GetTicks();

	if (n == NO_SAMPLES || (!activeSamples && !CallDAC(DAC_LOC,0)))
		return -1;
	if (n == ALL_SAMPLES)
		if (pausedLoc)
			ticks = pausedLoc - globalLoc;
		else
			ticks -= globalLoc;
	else if (sample[n].pausedLoc)
		ticks = sample[n].pausedLoc - sample[n].startLoc;
	else if (pausedLoc)
		ticks = pausedLoc - sample[n].startLoc;
	else
		if (sample[n].restartLoc)
			ticks = 0;
		else
			ticks -= sample[n].startLoc;
	if (ticks >= 65535)
		return 65534;
	return ticks;
}

void
AudioMgrDOS::AudioRate(int request)
{
	DACArgsIn[1] = Min(request,rateMax);
	if (rate != DACArgsIn[1]) {
		rate = DACArgsIn[1];
		CallDAC(DAC_SET_RATE,1);
	}
}

void
AudioMgrDOS::AudioBits(int request)
{
	DACArgsIn[1] = Min(request,bitsMax);
	if (bits != DACArgsIn[1]) {
		bits = DACArgsIn[1];
		CallDAC(DAC_SET_BITS,1);
	}
}

void
AudioMgrDOS::AudioChannels(int request)
{
	DACArgsIn[1] = Min(request,channelsMax);
	if (channels != DACArgsIn[1]) {
		channels = DACArgsIn[1];
		CallDAC(DAC_SET_CHANNELS,1);	 
	}
}

void
AudioMgrDOS::AudioDistort(int request, int n)
{
	if (n == ALL_SAMPLES)
		distortion = request;
	else if (n != NO_SAMPLES)
		sample[n].distortion = request;
}

void
AudioMgrDOS::AudioVolume(int request, int n)
{
	if (n == ALL_SAMPLES)
		volume = Min(request,MAXVOLUME);
	else if (n != NO_SAMPLES)
		sample[n].volume = Min(request,MAXVOLUME);
}

Boolean
AudioMgrDOS::StaticMergeBuffers()
{
	return ((AudioMgrDOS *)audioMgr)->MergeBuffers();
}

Boolean
AudioMgrDOS::MergeBuffers()
{
	int	n, cnt = 0;

	mbTickCount = GetTicks();

	if (!activeSamples)
		return False;

	callBacks++;
	lastDAC = False;
	if (!pausedLoc) {
		for (n = 0; n < activeSamples; n++) {
			if (sample[n].restartLoc) {
				sample[n].restartLoc--;
				sample[n].startLoc = GetTicks();
			}
			#ifndef AUDIO_LIB
			if (sample[n].vmd){
				if (MergeBuffer(n,cnt))
					cnt++;
				continue;
			}
			#endif

			if (!sample[n].buffID.IsAllocated() || 
				(prioritySample != -1 && prioritySample != n))
				continue;

			if (sample[n].bytesBuffered && !sample[n].pausedLoc)
				if (MergeBuffer(n,cnt))
					cnt++;
		}
	}
	if (cnt) {
		DistortBuffer(ALL_SAMPLES,(char *)*convIDx);
		VolumeBuffer(ALL_SAMPLES,(char *)*convIDx);
	} else
		Silence((char *)*convIDx,DACHbufferSize,bits);
	_fmemcpy(DACBuffer.ptr,(char far *)*convIDx,DACHbufferSize);

	if (DACBuffer.adr.ofs == DACBuffer.adr.ofsOrig)
		DACBuffer.adr.ofs += DACHbufferSize;
	else
		DACBuffer.adr.ofs -= DACHbufferSize;
	if (cnt)
		return True;
	for (n = 0; n < activeSamples; n++)
		if (sample[n].bytesBuffered || sample[n].bytesToRead || sample[n].pausedLoc)
			return True;
	lastDAC = True;
	return False;
}

Boolean
AudioMgrDOS::MergeBuffer(int n, int cnt)
{
	char*	buffer;

	#ifndef AUDIO_LIB
	if (sample[n].robot) {
		buffer = (char*)*sample[n].buffID +
			sample[n].bufferNumber * DACHbufferSize;
		if (robotsPaused)
			return False;
		static int bufferNumber;
		if (rbot.min == 0)
			bufferNumber = 0;
		bufferNumber++;
		if (bufferNumber > 2)
			robotCallBacks++;
		else
			robotCallBacks = 0;
		rbot.min += DACHbufferSize * (16/bits);
		rbot.max += DACHbufferSize * (16/bits);
		if (rbot.min > rbot.hwm[1]) {
			if (rbot.min > rbot.hwm[0]) {
				Silence(buffer,DACHbufferSize,bits);
				rbot.hwm[0] = bufferNumber * DACHbufferSize * (16/bits);
				rbot.hwm[1] = bufferNumber * DACHbufferSize * (16/bits) + 2;
			} else {
				DupeChannel(buffer,DACHbufferSize/(2*(bits/8)),1,bits);
				rbot.hwm[1] = bufferNumber * DACHbufferSize * (16/bits) + 2;
			}
		} else if (rbot.min > rbot.hwm[0]) {
			DupeChannel(buffer,DACHbufferSize/(2*(bits/8)),0,bits);
			rbot.hwm[0] = bufferNumber * DACHbufferSize * (16/bits);
		}
	}

	if (n == 0 && sample[0].vmd)
		FillVMD();
	else
	#endif
	{
		// GWP buffer = (char*)*sample[n].buffID +
		// GWP 	sample[n].bufferNumber * DACHbufferSize;
		buffer = (char *)&sample[n].buffID[sample[n].bufferNumber * DACHbufferSize];
		DistortBuffer(n,buffer);
		VolumeBuffer(n,buffer);
		if (prioritySample == -1)
			if (!cnt)
				MemCopy(*convIDx,buffer,DACHbufferSize);
			else
				Merge(*convIDx,buffer,DACHbufferSize,bits,mixCheck);
		else if (n == prioritySample) {
			MemCopy(*convIDx,buffer,DACHbufferSize);
			MemCopy(*priorityID,buffer,DACHbufferSize);
			noiseOfs = sample[n].bufferNumber * DACHbufferSize;
		}
	
		// GWP if (++sample[n].bufferNumber == intHbufferSize
		if (++sample[n].bufferNumber >= (sample[n].buffID.Size()/2)
			#ifndef AUDIO_LIB
				* (sample[n].robot? 2 :1)
			#endif
				/ DACHbufferSize * 2)
			sample[n].bufferNumber = 0;
	}
	sample[n].bytesBuffered -= Min(DACHbufferSize,sample[n].bytesBuffered);
	return True;
}

void
AudioMgrDOS::FillBuffer(int n)
{
	static int	loopOfs = 0;
	int	i;

	if (
		#ifndef AUDIO_LIB
		sample[n].vmd || sample[n].robot ||
		#endif
		sample[n].bytesToRead == 0) {
		sample[n].fillToggle = (uchar)(1 - sample[n].fillToggle);
		return;
	}
		
	// GWP int fillOfs = sample[n].fillToggle * intHbufferSize;
	int ThisBufferSize = sample[n].buffID.Size()/2;
	int fillOfs = sample[n].fillToggle * ThisBufferSize;
	// GWP int size = intHbufferSize - loopOfs;
	int size = ThisBufferSize - loopOfs;
	int len = size * sample[n].mul / sample[n].div;
	if (len > sample[n].bytesToRead)
		len = sample[n].bytesToRead;

	sample[n].bytesToRead -= len;

	if (sample[n].fd == 0) {
		// GWP memcpy((char *)*convID + len*sample[n].compressed,
		// GWP 		&sample[n].memID[sample[n].bytesRead], len);
		memcpy((char *)&convID[len*sample[n].compressed],
				&sample[n].memID[sample[n].bytesRead], len);
		sample[n].bytesRead += len;
	} else {
		LSeek(sample[n].fd,sample[n].fileOfs,SEEK_SET);
		convID.Read(sample[n].fd,len*sample[n].compressed,len);
		sample[n].fileOfs += len;
	}

	if (sample[n].compressed) {
		Decomp(*convID, len, sample[n].compressed, sample[n].bits,
				sample[n].channels, sample[n].compValL, sample[n].compValR);
		sample[n].compValL = DecompGet(0);
		sample[n].compValR = DecompGet(1);
		len *= sample[n].compressed + 1;
	}

	if (sample[n].rate > rate) {
		i = sample[n].rate / rate;
		len /= i;
		ConvRate(*convID, len, sample[n].bits, sample[n].channels, -i);
	}

	if (sample[n].channels > channels) {
		i = sample[n].channels / channels;
		len /= i;
		ConvChannels(*convID, len, sample[n].bits, -i);
	}

	if (sample[n].bits > bits) {
		i = sample[n].bits / bits;
		len /= i;
		ConvBits(*convID, len, -i);
	}

	char *dp = &sample[n].buffID[fillOfs+loopOfs];
	memcpy(dp, *convID, len);
#if defined (DEBUG)
	// GWP Test code.
	dp = &sample[n].buffID[fillOfs+loopOfs];
#endif

	if (sample[n].bits < bits) {
		i = bits / sample[n].bits;
		ConvBits(dp, len, i);
		len *= i;
#if defined (DEBUG)
		// GWP Test code.
		dp = &sample[n].buffID[fillOfs+loopOfs];
#endif
	}

	if (sample[n].channels < channels) {
		i = channels / sample[n].channels;
		ConvChannels(dp, len, bits, i);
		len *= i;
#if defined (DEBUG)
		// GWP Test code.
		dp = &sample[n].buffID[fillOfs+loopOfs];
#endif
	}

	if (sample[n].rate < rate) {
		i = rate / sample[n].rate;
		ConvRate(dp, len, bits, channels, i);
		len *= i;
#if defined (DEBUG)
		// GWP Test code.
		dp = &sample[n].buffID[fillOfs+loopOfs];
#endif
	}

	if (sample[n].pan >= 0 && channels == 2 && bits == 16) {
		Pan(dp, len/2, sample[n].pan);
#if defined (DEBUG)
		// GWP Test code.
		dp = &sample[n].buffID[fillOfs+loopOfs];
#endif
	}

	if (!sample[n].bytesToRead && sample[n].loop) {
		if (sample[n].bits == 16) {
			sample[n].compValL = 0;
			sample[n].compValR = 0;
		} else {
			sample[n].compValL = 0x80;
			sample[n].compValR = 0x80;
		}
		sample[n].bytesToRead = sample[n].length;
		sample[n].fileOfs = sample[n].start;
		if (sample[n].fd == 0)
			sample[n].bytesRead = sample[n].start;
		else
			sample[n].bytesRead = 0;
		sample[n].bytesBuffered += len;
		// GWP if (intHbufferSize != (loopOfs += len))
		if (ThisBufferSize != (loopOfs += len))
			FillBuffer(n);
		else
			sample[n].fillToggle = (uchar)(1 - sample[n].fillToggle);
		loopOfs = 0;
		return;
	}
	
	// GWP if (len < intHbufferSize && !loopOfs) {
	if (len < ThisBufferSize && !loopOfs) {
		// pad this partial buffer to next DAC-buffer boundary and adjust len
		int newlen = (len + DACHbufferSize - 1) / DACHbufferSize * DACHbufferSize;
		Silence(&sample[n].buffID[fillOfs+loopOfs+len],newlen-len,bits);
		len = newlen;
#if defined (DEBUG)
		// GWP Test code.
		dp = &sample[n].buffID[fillOfs+loopOfs];
#endif
	}
	sample[n].bytesBuffered += len;
	sample[n].fillToggle = (uchar)(1 - sample[n].fillToggle);
	return;
}

int
AudioMgrDOS::CheckNoise()
{
	static int check;
	int t, len, bytesPlayed, ofs, n;

	n = prioritySample;
	if (prioritySample == -1
		#ifndef AUDIO_LIB
		 || sample[n].vmd
		#endif
		)
		return 0;
	if (!sample[n].bytesBuffered)
		return 0;
	bytesPlayed = (GetTicks()-mbTickCount +1) * bytesPerTick;
	len = bytesPerTick;
//	if (len > sample[n].bytesBuffered)
//		len = sample[n].bytesBuffered;

	if (bytesPlayed < DACHbufferSize) {
		ofs = bytesPlayed;
		if (bytesPlayed + len <= DACHbufferSize) {
			check = MonitorNoise(&priorityID[ofs],len,bits);
			return check;
		}
		t = bytesPlayed + len - DACHbufferSize;
		check = MonitorNoise(&priorityID[ofs],len - t,bits);
		if (check)
			return check;
		bytesPlayed += len - t;
		len = t;
	}

	ofs = noiseOfs + bytesPlayed;
	if (noiseOfs + bytesPlayed + len > sample[n].buffID.Size()) {
		t = noiseOfs + bytesPlayed + len - sample[n].buffID.Size();
		if (t < len) {
			check = MonitorNoise(&sample[n].buffID[ofs],len - t,bits);
			if (check) {
				return check;
			}
			len = t;
			ofs = 0;
		} else {
			ofs = t - len;
		}
	}
	check = MonitorNoise(&sample[n].buffID[ofs],len,bits);

	return check;
}

void
AudioMgrDOS::DistortBuffer(int n, char *buffer)
{
	int	distortMask = ((n == ALL_SAMPLES)?
				~distortion : ~sample[n].distortion);

	if (distortMask != -1)
		Distort(buffer,DACHbufferSize,bits,distortMask);
}

void
AudioMgrDOS::VolumeBuffer(int n, char *buffer)
{
	int	vol;

	if (n == ALL_SAMPLES) {
		vol = volume;
		if (volumeMax > 1) {
			DACArgsIn[1] = vol * (volumeMax-1) / MAXVOLUME;
			CallDAC(DAC_SET_VOLUME,1);
			return;
		}
	} else
		vol = sample[n].volume;
	if (vol == MAXVOLUME)
		return;
	if (!vol) {
		Silence(buffer,DACHbufferSize,bits);
		return;
	}
	Volume(buffer,DACHbufferSize,bits,vol/8,staticFilter?0x7f:0x80);
}

Boolean
AudioMgrDOS::CheckSOLFormat(int n)
{
	// If the sample is in SOL format, the file pointer will be left
	// pointing at the first 'playable' byte:

	uchar	header[256], flag;
	uchar*	sptr = 0;

	if (sample[n].fd == 0) {
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
	sample[n].bits = (flag & 4) ? 16 : 8;
	sample[n].channels = (flag & 16) ? 2 : 1;
	sample[n].compressed = flag & 3;
	sample[n].length &= 0xFFFFFFFC;
	return True;
}

Boolean
AudioMgrDOS::CheckWAVEFormat(int n)
{
	// If the sample is in WAVE format, the file pointer will be left
	// pointing at the first 'playable' byte:

	char*	sptr = 0;
	int slen;
	int extra, br;
	char	okRIFF, okWAVE, okFMT;
	_Packed struct {
		char	id[4];
		long	len;
	} chunk;
	_Packed struct {
		short	fmttag;
		short	channels;
		long	rate;
		long	bytespersec;
		short	blockalign;
		short	bits;	
	} wf;

	if (sample[n].fd == 0) {
		sptr = (char*)*sample[n].memID;
		slen = sample[n].memID.Size();
	} else {
		LSeek(sample[n].fd,sample[n].fileOfs,SEEK_SET);
	}
	okRIFF = okWAVE = okFMT = 0;
	while(1) {
		if (sptr) {
			if (slen < sizeof(chunk))
				break;
			memcpy((char*)&chunk,sptr,sizeof(chunk));
			sptr += sizeof(chunk); 
			slen -= sizeof(chunk);
		} else {
			if (Read(sample[n].fd,(char*)&chunk,sizeof(chunk)) != sizeof(chunk))
				break;
		}

		if (!strncmp(chunk.id,"RIFF",4)) {
			okRIFF = 1;
			continue;
		}

		if (!strncmp(chunk.id,"WAVE",4)) {
			// WAVE chunk does not have a length dword:
			if (sptr) {
				sptr -= sizeof(chunk.len);
				slen += sizeof(chunk.len);
			} else {
				LSeek(sample[n].fd,-sizeof(chunk.len),SEEK_CUR);
			}
			okWAVE = 1;
			continue;
		}

		if (!strncmp(chunk.id,"fmt ",4)) {
			if (sptr) {
				if (slen < sizeof(wf))
					break;
				memcpy((char *)&wf,sptr,sizeof(wf));
				sptr += sizeof(wf);
				slen -= sizeof(wf);
			} else {
				if (Read(sample[n].fd,(char *)&wf,sizeof(wf)) != sizeof(wf))
					break;
			}
			br = sizeof(wf);
			if (wf.fmttag == 2) {   // compressed wave file
				if (sptr) {
					if (slen < sizeof(int))
						break;
					memcpy(&extra,sptr,sizeof(int));
					sptr += sizeof(int) + extra;
					slen -= sizeof(int) + extra;
				} else {
					if (Read(sample[n].fd,&extra,sizeof(int)) != sizeof(int))
						break;
					LSeek(sample[n].fd,extra,SEEK_CUR);
				}
				br += sizeof(int) + extra;
				sample[n].compressed = 3;  // 4-to-1 compression
			} else
				sample[n].compressed = 0;
			sample[n].rate = (int)wf.rate;
			sample[n].bits = (uchar)wf.bits;
			if (sample[n].compressed  || (sample[n].bits != 8 && sample[n].bits != 16))
			{
				#ifndef AUDIO_LIB
				msgMgr->Fatal(SrcLoc,"Can't play compressed WAVE audio");
				#else
//				Fatal(COMPRESSED_WAVE,sample[n].num);
				#endif
			}
			sample[n].channels = (uchar)wf.channels;
			if (sptr) {
				sptr += chunk.len - br + (chunk.len & 1);
				slen -= chunk.len - br + (chunk.len & 1);
			} else {
				LSeek(sample[n].fd,chunk.len - br + (chunk.len & 1),SEEK_CUR);
			}
			okFMT = 1;
			continue;
		}

		if (!strncmp(chunk.id,"data",4)) {
			if (!okRIFF || !okWAVE || !okFMT)
				break;
			if (sample[n].fd == 0) {
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
		if (sptr) {
			sptr += chunk.len + (chunk.len & 1);
			slen -= chunk.len + (chunk.len & 1);
		} else {
			LSeek(sample[n].fd,chunk.len + (chunk.len & 1),SEEK_CUR);
		}
	}
	return False;
}

void
AudioMgrDOS::CheckRawFormat(int n)
{
	if (sample[n].fd == 0) {
		sample[n].length = sample[n].memID.Size();
	} else {
		LSeek(sample[n].fd,sample[n].fileOfs,SEEK_SET);
		sample[n].length = FileLength(sample[n].fd);
	}
	sample[n].start = 0;
	sample[n].rate = rate;
	sample[n].bits = (uchar)bits;
	sample[n].channels = (uchar)channels;
	sample[n].compressed = 0;
	return;
}

void
AudioMgrDOS::AdjustAudBuffSizes(unsigned *audioSize, unsigned *DACSize)
{
#ifdef WINDOWS
	if (*audioSize != 0 && (*audioSize % *DACSize) == 0)
		if (((*audioSize / *DACSize) % 2) == 0)
			return;
#endif
//	*audioSize = 65536;
	*audioSize = *audioSize;
	
	//GEHif (*DACSize >= 24*1024)
	//GEH	*DACSize = 32*1024;
	//GEHelse if (*DACSize >= 12*1024)
	
	if (*DACSize >= 96*1024)
		*DACSize = 128*1024;
	else if (*DACSize >= 48*1024)
		*DACSize = 64*1024;
	else if (*DACSize >= 24*1024)
		*DACSize = 32*1024;
	else if (*DACSize >= 12*1024)
		*DACSize = 16*1024;
	else if (*DACSize >= 6*1024)
		*DACSize = 8*1024;
	else if (*DACSize >= 3*1024)
		*DACSize = 4*1024;
	else if (*DACSize >= 1024+512)
		*DACSize = 2*1024;
	else
		*DACSize = 1*1024;
}

Boolean
AudioMgrDOS::GetDACBuffer()
{
	DACArgsIn[1] = DACHbufferSize * 2;
	DACArgsIn[2] = (int)StaticMergeBuffers;
	if (CallDAC(DAC_SET_BUFF,2)) {
		#ifndef AUDIO_LIB
		msgMgr->Alert(SrcLoc, Msg_NoAudio);
		#endif
		FreeAudioDrv();
		return False;
	}
	DACBuffer.adr.sel = (short)CallDAC(DAC_GET_BUFF_SEL,0);
	DACBuffer.adr.ofsOrig = CallDAC(DAC_GET_BUFF_OFS,0);
	return True;
}

void
AudioMgrDOS::EndAudio()
{
	if (!audioInstalled)
		return;
	AudioStop(ALL_SAMPLES);
	CallDAC(DAC_TERMINATE,0);
	FreeAudioDrv();
	#ifndef AUDIO_LIB
	if (sfxVolFD != -1)
		Close(sfxVolFD);
	if (audVolFD != -1)
		Close(audVolFD);
	#endif
	audioInstalled = False;
	convID.Free();
	convIDx.Free();
	priorityID.Free();
}

int
AudioMgrDOS::CallDAC(int func, int cnt)
{
	DACArgsIn[0] = cnt;
	return audioDriver.code(func,DACArgsIn);
}

void
AudioMgrDOS::AudioServer()
{
	int	n;

	AudioCheck();
	for (n = 0; n < activeSamples; n++) {
		// GWP if (sample[n].bytesBuffered <= intHbufferSize && sample[n].bytesToRead)
		if (sample[n].bytesBuffered <= (sample[n].buffID.Size()/2) && sample[n].bytesToRead)
			FillBuffer(n);
		if (sample[n].fadeSteps)
			if (AudioFade(n))
				AudioStop(n--);
	}
}

Boolean
AudioMgrDOS::AudioFade(int n)
{
	int	volume;

	if (sample[n].fadeLoc + sample[n].fadeTicks > GetTicks())
		return False;
	if (sample[n].volume > sample[n].fadeVol) {
		volume = sample[n].volume - sample[n].fadeSteps;
		if (volume <= sample[n].fadeVol) {
			if (sample[n].fadeEnd)
				return True;
			volume = sample[n].fadeVol;
			sample[n].fadeSteps = 0;
		}
	} else {
		volume = sample[n].volume += sample[n].fadeSteps;
		if (volume >= sample[n].fadeVol) {
			if (sample[n].fadeEnd)
				return True;
			volume = sample[n].fadeVol;
			sample[n].fadeSteps = 0;
		}
	}
	AudioVolume(volume,n);
	sample[n].fadeLoc = GetTicks();
	return False;
}

void
AudioMgrDOS::AudioCheck()
{
	int	n;

	for (n = 0; n < activeSamples; n++)
		if (!(sample[n].bytesToRead || sample[n].bytesBuffered))
			if (activeSamples > 1)
				AudioStop(n--);
			else
				if (!CallDAC(DAC_LOC,0))
					AudioStop(n);
}

//***************************************************************************
//********************** SCI interpreter functions **************************
//***************************************************************************

#ifndef AUDIO_LIB

Boolean
AudioMgrDOS::AudioPurge(ResNum num)
{
	// If the named sfx is active and memory-resident, stop it
	// and return True; else return False
	// (Note: there may be multiple copies of the same sample active)

	Boolean rc = False;
	for (int n = 0; n < activeSamples; n++)
		if (sample[n].module == SFXMODNUM && sample[n].num == num && sample[n].memID) {
			AudioStop(n);
			rc = True;
		}
	return False;
}

Boolean
AudioMgrDOS::AudioLock(ResNum num, Boolean lock)
{
	// If the named sfx is active and memory-resident, set its
	// locked status as requested and return True; else return False
	// (Note: there may be multiple copies of the same sample active)

	Boolean rc = False;
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
AudioMgrDOS::AudioQueryDiscardable(ResNum num)
{
	// If the named sfx is active and memory-resident, return the
	// locked status it had prior to when it started playing;
	// else return False

	for (int n = 0; n < activeSamples; n++)
		if (sample[n].module == SFXMODNUM && sample[n].num == num)
			if (sample[n].memID)
				return (Boolean) (sample[n].attrs & DISCARDABLE);
			else
				return False;
	return False;
}

void
AudioMgrDOS::AudioSwap(int which)
{
	if ((int)sample[which].memID && (sample[which].attrs & DISCARDABLE))
		memMgr->SetDiscardable(sample[which].memID);

	sample[which].handleFile = resMgr->WriteHandleFile(sample[which].memID,"swp");
	if (sample[which].handleFile){
	  	sample[which].fd = resMgr->OpenHandleFile(sample[which].handleFile,"swp");
		if (sample[which].fd != -1){
			sample[which].swapped = True;
	  		sample[which].patch=True;
			//check to make sure sample[which] is the only one using this memid
			for (int n = which; n < activeSamples; n++){
				if (sample[n].memID == sample[which].memID)
					break;
			}
			if (n == activeSamples)
				sample[which].memID.Free();
			else
				sample[which].memID=0;
			sample[which].start = HANDLE_FILE_HEADER_SIZE;
		  	sample[which].fileOfs = HANDLE_FILE_HEADER_SIZE + sample[which].bytesRead;
			return;
		}
	}
	//Swap failed so dump the sample
	AudioStop(which);
}

Boolean
AudioMgrDOS::AudioFree(int which)
{
	int	n,n1;

	switch (which) {
		case SFX_ONLY:
			for (n = 0; n < activeSamples; n++) {
				if (sample[n].memID && sample[n].module == SFXMODNUM) {
					for (n1 = n+1; n1 < activeSamples; n1++) {
						fatal_test(sample[n].memID != sample[n1].memID);
					}
					AudioSwap(n);
					return True;
				}
			}
			break;
		case ALL_SAMPLES:
			for (n = 0; n < activeSamples; n++) {
				if (sample[n].memID && !sample[n].vmd) {
					for (n1 = n+1; n1 < activeSamples; n1++) {
						fatal_test(sample[n].memID != sample[n1].memID);
					}
					AudioSwap(n);
					return True;
				}
			}
			break;
	}
	return False;
}

void
AudioMgrDOS::InitAudioVols()
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

int
AudioMgrDOS::FindAudEntry(ResNum id)
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
AudioMgrDOS::FindAud36Entry(ResNum module, uchar noun, uchar verb, uchar cond, uchar sequ)
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
AudioMgrDOS::FillVMD()
{
	static int	loopOfs = 0;
	int	len, i, n = 0;
	len = (DACHbufferSize - loopOfs) * sample[n].mul / sample[n].div;
	if (sample[n].bytesToRead < len)
		len = sample[n].bytesToRead;
	sample[n].bytesToRead -= len;
	MemCopy(*convID,(char *)*sample[n].memID+sample[n].bytesRead, len);
	sample[n].bytesRead += len;

	if (sample[n].rate > rate) {
		i = sample[n].rate / rate;
		len /= i;
		ConvRate(*convID, len, sample[n].bits, sample[n].channels, -i);
	}

	if (sample[n].channels > channels) {
		i = sample[n].channels / channels;
		len /= i;
		ConvChannels(*convID, len, sample[n].bits, -i);
	}

	if (sample[n].bits > bits) {
		i = sample[n].bits / bits;
		len /= i;
		ConvBits(*convID, len, -i);
	}

	if (sample[n].bits < bits) {
		i = bits / sample[n].bits;
		ConvBits(*convID, len, i);
		len *= i;
	}

	if (sample[n].channels < channels) {
		i = channels / sample[n].channels;
		ConvChannels(*convID, len, bits, i);
		len *= i;
	}

	if (sample[n].rate < rate) {
		i = rate / sample[n].rate;
		ConvRate(*convID, len, bits, channels, i);
		len *= i;
	}

	if (sample[n].pan >= 0 && channels == 2 && bits == 16) {
		Pan(*convID, len/2, sample[n].pan);
	}

	MemCopy((char *)*convIDx + loopOfs,*convID,len);

	if (!sample[n].bytesToRead) {
		sample[n].bytesToRead = sample[n].length;
		sample[n].fileOfs = sample[n].bytesRead = sample[n].start;
		sample[n].bytesBuffered += len;
		if (DACHbufferSize > (loopOfs += len))
			FillVMD();
		loopOfs = 0;
		return;
	}
	
	sample[n].bytesBuffered += len;
	loopOfs = 0;
	return;
}

Boolean
AudioMgrDOS::AudioVMDStart(MemID vmdID,int vmdRate,int vmdBits, int vmdChannels)
{
	int n;
	for (n = 0; n < activeSamples; n++) {
		if (sample[n].robot) {
			if (!sample[n].bytesToRead) {
				AudioStop(n);
				break;
			}
			msgMgr->Fatal("VMD started while ROBOT still playing");
		}
	}
	n = activeSamples;
	memset(&sample[n], 0, sizeof(Sample));
	sample[n].module = sample[n].num = 0;
	sample[n].bytesBuffered = sample[n].length = vmdID.Size();
	sample[n].start = 0;
	sample[n].loop = True;
	sample[n].volume = MAXVOLUME;
	sample[n].compressed = False;
	sample[n].robot = False;
	sample[n].vmd = True;
	sample[n].rate = vmdRate;
	sample[n].bits = vmdBits;
	sample[n].channels = vmdChannels;
	sample[n].memID = vmdID;
	sample[n].attrs = sample[n].memID.Attrs();

	if (!AudioSelect(n)) {
		return False;
	}
	++activeSamples;
 	AudioPlay();
	startVMD = callBacks;
	mbTickCount = GetTicks();
	vmdQuery = 0;
	return True;
}

Boolean
AudioMgrDOS::AudioVMDStop()
{
	for (int n = 0; n < activeSamples; n++)
		if (sample[n].vmd) {
			AudioStop(n);
			return True;
		}
	return False;
}

uint
AudioMgrDOS::AudioVMDQuery()
{
	int	n;
	uint	rc;

	for (n = 0; n < activeSamples; n++)
		if (sample[n].vmd)
			break;
	if (n == activeSamples)
		return 0;
	int ticks = GetTicks() - mbTickCount;
	int samps = rate * ticks / 60;
	if (rate < sample[n].rate)
		rc = (callBacks - startVMD) * DACHbufferSize * 2 /	(bits/8) + samps;
	else if (rate > sample[n].rate)
		rc = (callBacks - startVMD) * DACHbufferSize / ((bits/8) * 2) + samps;
	else
		rc = (callBacks - startVMD) * DACHbufferSize / (bits/8) + samps;

	if (sample[n].channels > channels)
		rc <<= 1;
	else
		if (sample[n].channels < channels)
			rc >>= 1;

	if (rc < vmdQuery)
		return vmdQuery;

	vmdQuery = rc;
	return rc;
}

Boolean
AudioMgrDOS::QueryAudRobot(RobotAudStatus *buff)
{
	for (int n = 0; n < activeSamples; n++) {
		if (sample[n].robot) {
			_disable();
			buff->bytesPlayed = robotCallBacks * DACHbufferSize;
			buff->bytesPlaying = buff->bytesPlayed +
				(GetTicks() - mbTickCount) * rate / 60;
			_enable();
			buff->bytesSubmitted = buff->bytesPlayed + DACHbufferSize * 2;
			if (buff->bytesPlaying > buff->bytesSubmitted)
				buff->bytesPlaying = buff->bytesSubmitted;
			buff->rate = rate;
			buff->bits = bits;
			return True;
		}
	}
	buff->bytesPlayed = buff->bytesPlaying = buff->bytesSubmitted = 0;
	return False;
}

Boolean
AudioMgrDOS::AudRobot(RobotAudInfo buff)
{
	int	n;

	if (!audioInstalled)
		return True;
	rbot.adrID = buff.adrID;
	rbot.len = buff.len;
	rbot.floc = buff.floc;
	rbot.ext = rbot.floc + rbot.len*4;
	rbot.channel = rbot.floc % 4 ? 1 : 0;
	if (rbot.floc <= 2 && ffloc == -1 && rbot.len != 0 && rbot.len != -1) {
		for (n = 0; n < activeSamples; n++) {
			fatal_test (!sample[n].vmd);
			if (sample[n].robot)
				AudioStop(n);
		}
		rbot.hwm[0] = 0;
		rbot.hwm[1] = 2;
		if ((n=activeSamples) == MAXSAMPLES)
			return False;
		memset(&sample[n], 0, sizeof(Sample));
		sample[n].module = sample[n].num = 0;
		sample[n].length = rbot.len;
		sample[n].start = 0;
		sample[n].loop = False;
		sample[n].volume = MAXVOLUME;
		sample[n].compressed = 1;
		sample[n].vmd = False;
		sample[n].robot = True;
		if (!AudioSelect(n))
			return False;
		rbot.memID = sample[n].buffID;
		rbot.blen = rbot.memID.Size();
		rbot.min = 0;
		rbot.max = rbot.blen * (16/bits);
		++activeSamples;
		ffloc = rbot.floc;
	} else {
		for (n = 0; n < activeSamples; n++)
			if (sample[n].robot)
				break;
		if (n == activeSamples)
			return False;
	}
	if (rbot.len == 0) {
		AudioStop(n);
		ffloc = -1;
	} else if (rbot.len == -1) {
		sample[n].bytesToRead = 0;
		ffloc = -1;
	} else {
		if (rbot.ext <= Max(rbot.min,rbot.hwm[rbot.channel]) ||
				rbot.max <= rbot.hwm[rbot.channel])
			return False;
		sample[n].bytesToRead = rbot.len;
		int ext = rbot.ext;
		int rmax = rbot.max;
		if ((int)convID.Size() < rbot.len * 2)
			convID.Realloc(rbot.len * 2);
		if (bits == 8)
			FillRbotBuffer8(n);
		else
			FillRbotBuffer16(n);
		if (ext > rmax)
			return False;
		if (ffloc != -1 && ffloc != rbot.floc && !robotsPaused) {
			AudioPlay();
			ffloc = -1;
		}
	}
	return True;
}

void
AudioMgrDOS::FillRbotBuffer8(int n)
{
	int	ofs, ext, len;

	rbot.cofs = 0;
//	Decomp8To16(*convID,*rbot.adrID,rbot.len,0);
	memcpy((char *)*convID+rbot.len,*rbot.adrID,rbot.len);
	Decomp(*convID,rbot.len,1,16,1,0,0);
	rbot.len *= 2;
	int loc = Max(rbot.min+rbot.channel,rbot.hwm[rbot.channel]);
	int mx = rbot.max+rbot.channel;
	if (rbot.floc < loc) {
		rbot.cofs = (loc - rbot.floc) / 2;
		rbot.len -= rbot.cofs;
		rbot.floc = loc;
	}
	if (rbot.floc > mx) {
		rbot.len += (rbot.floc - mx) / 2;
		rbot.floc = mx;
	}
	if (rbot.ext > mx) {
		rbot.len -= (rbot.ext - mx) / 2;
		rbot.ext = mx;
	}

	rbot.len /= 2;
//	Conv16To8(&convID[rbot.cofs],&convID[rbot.cofs],rbot.len);
	ConvBits(&convID[rbot.cofs],rbot.len,-2);

	int maxhwm = Max(rbot.hwm[0],rbot.hwm[1]);
	if (rbot.ext > maxhwm)
		sample[n].bytesBuffered += (rbot.ext - maxhwm) / 2;

	if (rbot.floc > rbot.hwm[rbot.channel]) {
		ext = rbot.floc % (rbot.blen * 2);
		if ((rbot.floc&0xfffffffc) > (rbot.hwm[1-rbot.channel]&0xfffffffc)) {
			ofs = rbot.hwm[1-rbot.channel] % (rbot.blen * 2);
			if (ofs >= ext) {
				len = rbot.blen * 2 - ofs;
				Silence(&rbot.memID[ofs/2],len/2,bits);
				ofs = (1-rbot.channel)?2:0;
			}
			len = ext - ofs;
			if (len > 0)
				Silence(&rbot.memID[ofs/2],len/2,bits);
		}
		ofs = rbot.hwm[rbot.channel] % (rbot.blen * 2);
		if (ofs >= ext) {
			len = rbot.blen * 2 - ofs;
			DupeChannel(&rbot.memID[ofs/2],len/4,0,bits);
			ofs = rbot.channel?2:0;
		}
		len = ext - ofs;
		if (len > 0)
			DupeChannel(&rbot.memID[ofs/2],len/4,0,bits);
	}
	if (rbot.len > 0) {
		ofs = rbot.floc % (rbot.blen * 2);
		ext = rbot.ext % (rbot.blen * 2);
		len = 0;
		if (ofs >= ext) {
			len = (rbot.blen * 2 - (ofs & 0xfffffffc))/2;
			CopyEOS(&rbot.memID[ofs/2],&convID[rbot.cofs],len/2,bits);
			ofs = rbot.channel?2:0;
		}
		CopyEOS(&rbot.memID[ofs/2],&convID[rbot.cofs+len/2],(ext-ofs)/4,bits);
	}
	rbot.hwm[rbot.channel] = rbot.ext;
}

void
AudioMgrDOS::FillRbotBuffer16(int n)
{
	int	ofs, ext, len;

	rbot.cofs = 0;
//	Decomp8To16(*convID,*rbot.adrID,rbot.len,0);
	memcpy((char *)*convID+rbot.len,*rbot.adrID,rbot.len);
	Decomp(*convID,rbot.len,1,16,1,0,0);
	rbot.len *= 2;
	int loc = Max(rbot.min+rbot.channel*2,rbot.hwm[rbot.channel]);
	int mx = rbot.max+rbot.channel*2;
	if (rbot.floc < loc) {
		rbot.cofs = (loc - rbot.floc) / 2;
		rbot.len -= rbot.cofs;
		rbot.floc = loc;
	}
	if (rbot.floc > mx) {
		rbot.len += (rbot.floc - mx) / 2;
		rbot.floc = mx;
	}
	if (rbot.ext > mx) {
		rbot.len -= (rbot.ext - mx) / 2;
		rbot.ext = mx;
	}

	int maxhwm = Max(rbot.hwm[0],rbot.hwm[1]);
	if (rbot.ext > maxhwm)
		sample[n].bytesBuffered += rbot.ext - maxhwm;

	if (rbot.floc > rbot.hwm[rbot.channel]) {
		ext = rbot.floc % rbot.blen;
		if ((rbot.floc&0xfffffffc) > (rbot.hwm[1-rbot.channel]&0xfffffffc)) {
			ofs = rbot.hwm[1-rbot.channel] % rbot.blen;
			if (ofs >= ext) {
				len = rbot.blen - ofs;
				Silence(&rbot.memID[ofs],len,bits);
				ofs = (1-rbot.channel)?2:0;
			}
			len = ext - ofs;
			if (len > 0)
				Silence(&rbot.memID[ofs],len,bits);
		}
		ofs = rbot.hwm[rbot.channel] % rbot.blen;
		if (ofs >= ext) {
			len = rbot.blen - ofs;
			DupeChannel(&rbot.memID[ofs],len/4,0,bits);
			ofs = rbot.channel?2:0;
		}
		len = ext - ofs;
		if (len > 0)
			DupeChannel(&rbot.memID[ofs],len/4,0,bits);
	}
	if (rbot.len > 0) {
		ofs = rbot.floc % rbot.blen;
		ext = rbot.ext % rbot.blen;
		len = 0;
		if (ofs >= ext) {
			len = (rbot.blen - (ofs & 0xfffffffc))/2;
			CopyEOS(&rbot.memID[ofs],&convID[rbot.cofs],len/2,bits);
			ofs = rbot.channel?2:0;
		}
		CopyEOS(&rbot.memID[ofs],&convID[rbot.cofs+len],(ext-ofs)/4,bits);
	}
	rbot.hwm[rbot.channel] = rbot.ext;
}

#ifdef DEBUG

void
AudioMgrDOS::CheckDiscStreaming(char* str)
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
		if (sample[n].robot)
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

void
AudioMgrDOS::DACPlay()
{
	SOL_Event event;
	
	memset(&sample[0], 0, sizeof(Sample));
	char* str = configMgr->Str(configMgr->PlayAudio);
	if (strlen(str) == 12 &&
		(str[0] == 'A' || str[0] == 'a' || str[0] == 'B' || str[0] == 'b')) {
		sample[0].num = 0;
		sample[0].module = GetBase36(&str[1],3);
		if (str[0] == 'B' || str[0] == 'b')
			sample[0].module += 36*36*36;
		sample[0].noun = GetBase36(&str[4],2);
		sample[0].verb = GetBase36(&str[6],2);
		sample[0].cond = GetBase36(&str[9],2);
		sample[0].sequ = GetBase36(&str[11],1);
	} else if (strlen(str)) {
		sample[0].num = atoi(str);
		sample[0].module = SFXMODNUM;
	}
	sample[0].distortion = 0;
	sample[0].volume = MAXVOLUME;
	sample[0].loop = True;
	if (!AudioSelect(0)) {
		Printf("%s not found.  Press any key...\n",str);
		while (!kbdMgr->Get(&event))
			;
		exit(1);
	}
	Printf("(%s)Press any key to stop audio playback...\n",str);
	++activeSamples;
	AudioPlay();
	while (activeSamples) {
		if (kbdMgr->Get(&event)) {
			EndAudio();
			exit(1);
		}
		AudioServer();
	}
	exit(1);
}

int
AudioMgrDOS::GetBase36(char* str36,int digits)
{
	uint	num36, n;

	num36 = 0;
	for (n = 0; n < digits; n++) {
		num36 *= 36;
		if (str36[n] < '0' || str36[n] > '9' && str36[n] < 'A' ||
				str36[n] > 'Z' && str36[n] < 'a' || str36[n] > 'z') {
			return 0;
		}
		if (str36[n] <= '9')
			num36 += str36[n] - '0';
		else if (str36[n] <= 'Z')
			num36 += str36[n] - 'A' + 10;
		else
			num36 += str36[n] - 'a' + 10;
	}
	return num36;
}
#endif  // DEBUG

void
AudioMgrDOS::MakeName36(MemType type, char* fname, ResNum module, uchar noun,
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

void
AudioMgrDOS::ConvBase36(char *str, int num10, int digits)
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
AudioMgrDOS::GetDigit36(int n)
{
	if (n <=  9)
		return((char)('0' + n));
	else
		return((char)('A' + n - 10));
}

#endif  // AUDIO_LIB

//***************************************************************************
//********************** kernel interface functions *************************
//***************************************************************************


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
AudioMgrDOS::ImpInitializeAudioDriver()
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
AudioMgrDOS::ImpTerminateAudioDriver()
{
	EndAudio();
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioInstalled
 *
 *  PARAMETERS:	None
 *
 *  DESCRIPTION:	Returns the value of the audio installed flag
 *
 *  RETURNS:		Boolean - True if audio is installed, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDOS::ImpAudioInstalled()
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
AudioMgrDOS::ImpAudioPreload(int iValue)   
{
	if	(iValue > 0)
	{
		preLoad = iValue;			
	}
				
	return preLoad;
}

/*******************************************************************************
 *  FUNCTION: 		ImpGetSampleNumber - Not exported in library
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
AudioMgrDOS::ImpGetSampleNumber(int iResNum, int* iSampleNumber, int iTag)   
{
	Boolean fRetVal = False;

	for (int i = 0; i < activeSamples; i++) 
	{
		if (sample[i].num == iResNum && sample[i].module == SFXMODNUM) 
		{
			if (iTag && iTag != sample[i].tag)
			{
				// do nothing
			}
			else
			{
				*iSampleNumber = i;
				fatal_test(*iSampleNumber >= 0 && *iSampleNumber < activeSamples);
				fatal_test(sample[*iSampleNumber].module == SFXMODNUM);
				fRetVal = True;
				break;
			}
		}
	}

	fatal_test(fRetVal == True || fRetVal == False);

	return fRetVal;
}

/*******************************************************************************
 *  FUNCTION: 		ImpGetSampleNumber - Not exported in library
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
AudioMgrDOS::ImpGetSampleNumber(int iModule, int iNoun, int iVerb, int iCond, 
										int iSequ, int* iSampleNumber)   
{
	Boolean fRetVal = False;
	*iSampleNumber  = -1;  //set it to a bogus value

	for (int i = 0; i < activeSamples; i++) 
	{
		if (
			#ifndef AUDIO_LIB
			 !sample[i].robot && 
			 !sample[i].vmd &&
			#endif
			 sample[i].num == 0 &&
			 sample[i].module == (ResNum)iModule &&
			 sample[i].noun == (uchar)iNoun &&
			 sample[i].verb == (uchar)iVerb &&
			 sample[i].cond == (uchar)iCond &&
			 sample[i].sequ == (uchar)iSequ)
		{

			*iSampleNumber = i;
			fatal_test(*iSampleNumber >= 0 && *iSampleNumber < activeSamples);
			fatal_test(sample[*iSampleNumber].module != SFXMODNUM);
			fRetVal = True;
			break;
		}
	}

  	fatal_test(fRetVal == True || fRetVal == False);

	return fRetVal;
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
AudioMgrDOS::ImpAudioWaitToPlay(int iResNum, Boolean fLoop, int iVolume, int iTag)   
{
	fatal_test(ImpAudioInstalled());
	fatal_test(iResNum >= 0);
	fatal_test(fLoop == True || fLoop == False);
	fatal_test(iVolume >= 0 && iVolume <= 128);
	fatal_test(iTag >= 0);

   return( ImpAudioPlay(iResNum, fLoop, iVolume, iTag, (long)True) );
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
AudioMgrDOS::ImpAudioWaitToPlay(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
								  Boolean fLoop, int iVolume)   
{
	return( ImpAudioPlay(iModule, iNoun, iVerb, iCond, iSequ, 
									fLoop, iVolume, True) );
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
AudioMgrDOS::ImpAudioActiveSamples()
{
	return activeSamples;
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
AudioMgrDOS::ImpAudioPlay(int iResNum, Boolean fLoop, int iVolume, int iTag, 
						  long fPause)   
{
	int iTimeToPlay = 0;
	int iSampleNumber;

	// Stop any samples using the same soundObject (tag)
	if (iTag) {
		for (int n = 0; n < activeSamples; n++) {
			if (sample[n].tag == iTag) {
				ImpAudioStop(sample[n].num, iTag);
				break;
			}
		}
	}

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		if (sample[iSampleNumber].pausedLoc)
		{
			AudioResume(iSampleNumber);
			iTimeToPlay = sample[iSampleNumber].ticks;
		}
	}
	else
	{		
		if (activeSamples == MAXSAMPLES) 
		{
			fatal_test(!"Cannot add any more samples, number active are at maximum!");
		}
		else
		{
			int n 			  = activeSamples;
			memset(&sample[n], 0, sizeof(Sample));
			sample[n].module = SFXMODNUM;
			sample[n].num    = iResNum;
			sample[n].loop   = fLoop;
			
			if (iVolume > MAXVOLUME)
			{
				sample[n].volume = MAXVOLUME;
				prioritySample   = n;				
			}
			else
			{
				sample[n].volume = iVolume;
			}

			sample[n].tag   = iTag;
			#ifndef AUDIO_LIB
			sample[n].robot = False;
			sample[n].vmd   = False;
			#endif

			if ((iTimeToPlay = sample[n].ticks = AudioSelect(n)) != 0) 
			{
				if (fPause)
				{
					sample[n].pausedLoc = 1;
				}
				else
				{
					sample[n].pausedLoc = 0;
				}

				activeSamples++;
				AudioPlay();  // activeSamples may change in windows !!!
		 
				if (fPause)
				{
					sample[n].pausedLoc = sample[n].startLoc;					
				}
			} 
			else 
			{
				// reset, no sample to play
				if (prioritySample == n)
				{
					prioritySample = -1;
				}
			}
		}
	}

	return iTimeToPlay;
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
AudioMgrDOS::ImpAudioPlay(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
						  Boolean fLoop, int iVolume, Boolean fPause)  
{
	int iTimeToPlay = 0;
	int iSampleNumber;
	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		if (sample[iSampleNumber].pausedLoc)
		{
			AudioResume(iSampleNumber);
			iTimeToPlay = sample[iSampleNumber].ticks;
		}
	}
	else
	{		
		if (activeSamples == MAXSAMPLES) 
		{
			//fatal_test(!"Cannot add any more samples, number active are at maximum!");
		}
		else
		{
			int n 			  = activeSamples;
			memset(&sample[n], 0, sizeof(Sample));
			sample[n].num 	  = 0;
			sample[n].module = (ResNum)iModule;
			sample[n].noun   = (uchar)iNoun;
			sample[n].verb   = (uchar)iVerb;
			sample[n].cond   = (uchar)iCond;
			sample[n].sequ   = (uchar)iSequ;
			sample[n].loop   = fLoop;
			
			if (iVolume > MAXVOLUME)
			{
				sample[n].volume = MAXVOLUME;
				prioritySample   = n;				
			}
			else
			{
				sample[n].volume = iVolume;
			}

			#ifndef AUDIO_LIB
			sample[n].robot = False;
			sample[n].vmd   = False;
			#endif
			
			if ((iTimeToPlay = sample[n].ticks = AudioSelect(n)) != 0) 
			{
				if (fPause)
				{
					sample[n].pausedLoc = 1;
				}
				else
				{
					sample[n].pausedLoc = 0;
				}

				activeSamples++;
				AudioPlay();  // activeSamples may change under windows!!!
		 
				if (fPause)
				{
					sample[n].pausedLoc = sample[n].startLoc;					
				}
			} 
			else 
			{
				// reset, no sample to play
				if (prioritySample == n)
				{
					prioritySample = -1;
				}
			}
		}
	}

	return iTimeToPlay;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioStopNoEx - Not exported in library
 *
 *  PARAMETERS:	int iSample - the sample number to stop
 *
 *  DESCRIPTION: 	Stops playing the specified sample 
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDOS::ImpAudioStopNoEx(int iSample)   
{
	AudioStop(iSample);
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
AudioMgrDOS::ImpAudioStopAll()   
{
	ImpAudioStopNoEx(ALL_SAMPLES);
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
AudioMgrDOS::ImpAudioStop(int iResNum, int iTag)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		ImpAudioStopNoEx(iSampleNumber);
	}
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
AudioMgrDOS::ImpAudioStop(int iModule, int iNoun, int iVerb, int iCond, int iSequ)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		ImpAudioStopNoEx(iSampleNumber);
	}
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
AudioMgrDOS::ImpAudioLoop(int iResNum, Boolean fLoop, int iTag)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		sample[iSampleNumber].loop = fLoop;
	}
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
AudioMgrDOS::ImpAudioLoop(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
							Boolean fLoop)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		sample[iSampleNumber].loop = fLoop;
	}
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioPauseNoEx - Not exported in library
 *
 *  PARAMETERS:	int iSample - the sample number to pause
 *
 *  DESCRIPTION: 	Pauses the specified sample 
 *
 *  RETURNS:		Boolean - True if one or more samples affected, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDOS::ImpAudioPauseNoEx(int iSample)   
{
	Boolean fRetVal = AudioPause(iSample);

	return fRetVal;
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
AudioMgrDOS::ImpAudioPauseAll()   
{
	return( ImpAudioPauseNoEx(ALL_SAMPLES) );
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
AudioMgrDOS::ImpAudioPause(int iResNum, int iTag)   
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		fRetVal = ImpAudioPauseNoEx(iSampleNumber);
	}

	fatal_test(fRetVal == True || fRetVal == False);

	return fRetVal;
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
AudioMgrDOS::ImpAudioPause(int iModule, int iNoun, int iVerb, int iCond, int iSequ)   
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		fRetVal = ImpAudioPauseNoEx(iSampleNumber);
	}

	fatal_test(fRetVal == True || fRetVal == False);

	return fRetVal;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioResumeNoEx - Not exported in library
 *
 *  PARAMETERS:	int iSample - the sample number to pause
 *
 *  DESCRIPTION: 	Resumes the specified sample 
 *
 *  RETURNS:		Boolean - True if one or more samples affected, False otherwise
 ******************************************************************************/
Boolean
AudioMgrDOS::ImpAudioResumeNoEx(int iSample)   
{
	Boolean fRetVal = AudioResume(iSample);

	return fRetVal;
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
AudioMgrDOS::ImpAudioResumeAll()   
{
	return( ImpAudioResumeNoEx(ALL_SAMPLES) );
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
AudioMgrDOS::ImpAudioResume(int iResNum, int iTag)   
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		fRetVal = ImpAudioResumeNoEx(iSampleNumber);
	}

	fatal_test(fRetVal == True || fRetVal == False);

	return fRetVal;
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
AudioMgrDOS::ImpAudioResume(int iModule, int iNoun, int iVerb, int iCond, int iSequ)   
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		fRetVal = ImpAudioResumeNoEx(iSampleNumber);
	}

	fatal_test(fRetVal == True || fRetVal == False);

	return fRetVal;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDurationNoEx	 - not exported in library
 *
 *  PARAMETERS:	int iSample    - sample number to determine duration of play
 *						int* iDuration - pointer to int to update with duration value
 *
 *  DESCRIPTION: 	Updates iDuration with the duration in 60ths of a second since 
 *						the start of the specified
 *
 *  RETURNS:		Boolean - True if duration successfully determined, False
 *									 otherwise
 ******************************************************************************/
Boolean
AudioMgrDOS::ImpAudioDurationNoEx(int iSample, int* iDuration)   
{
	AudioServer();
	Boolean fRetVal = False;

	*iDuration = AudioLoc(iSample);

	if (*iDuration >= 0)
	{
		fRetVal = True;
	}

	return fRetVal;
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
AudioMgrDOS::ImpAudioDurationAll(int* iDuration)   
{
	Boolean fRetVal = ImpAudioDurationNoEx(ALL_SAMPLES,iDuration);

	return fRetVal;
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
AudioMgrDOS::ImpAudioDuration(int iResNum, int* iDuration, int iTag)   
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		fRetVal = ImpAudioDurationNoEx(iSampleNumber, iDuration);
	}

	fatal_test(fRetVal == True || fRetVal == False);

	return fRetVal;
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
AudioMgrDOS::ImpAudioDuration(int iModule, int iNoun, int iVerb, int iCond,
									 int iSequ, int *iDuration)   
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		fRetVal = ImpAudioDurationNoEx(iSampleNumber, iDuration);
	}

	fatal_test(fRetVal == True || fRetVal == False);

	return fRetVal;
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
AudioMgrDOS::ImpAudioSetRate(int iRate)
{
	rateSet = True;
	AudioRate(iRate);
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
AudioMgrDOS::ImpAudioDefaultRate()
{
	rateSet = False;
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
AudioMgrDOS::ImpAudioCurrentRate()
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
AudioMgrDOS::ImpAudioSetBits(int iBits)
{
	bitsSet = True;
	AudioBits(iBits);
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
AudioMgrDOS::ImpAudioDefaultBits()
{
	bitsSet = False;
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
AudioMgrDOS::ImpAudioCurrentBits()
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
AudioMgrDOS::ImpAudioSetChannels(int iChannels)
{
	channelsSet = True;
	AudioChannels(iChannels);
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
AudioMgrDOS::ImpAudioDefaultChannels()
{
	channelsSet = False;
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
AudioMgrDOS::ImpAudioCurrentChannels()
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
AudioMgrDOS::ImpAudioGetVolReduction()
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
AudioMgrDOS::ImpAudioSetVolReduction(Boolean fFlag)
{
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
AudioMgrDOS::ImpAudioGetDACCritical()
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
AudioMgrDOS::ImpAudioSetDACCritical(Boolean fFlag)
{
	critical = fFlag;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioDistortNoEx - Not exported in library
 *
 *  PARAMETERS:	int iDistortionMask - mask used for distortion
 *						int iSample 		  - the sample number to pause
 *
 *  DESCRIPTION: 	Distorts the specified sample 
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDOS::ImpAudioDistortNoEx(int iDistortionMask, int iSample)   
{
	AudioDistort(iDistortionMask, iSample);
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
AudioMgrDOS::ImpAudioDistortAll(int iDistortionMask)   
{
	ImpAudioDistortNoEx(iDistortionMask, ALL_SAMPLES);
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
AudioMgrDOS::ImpAudioDistort(int iResNum, int iDistortionMask, int iTag)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		ImpAudioDistortNoEx(iDistortionMask, iSampleNumber);
	}
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
AudioMgrDOS::ImpAudioDistort(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
								int iDistortionMask)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		ImpAudioDistortNoEx(iDistortionMask, iSampleNumber);
	}
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
AudioMgrDOS::ImpAudioGlobalVolume()   
{
	fatal_test(volume >= 0 && volume <= MAXVOLUME);
	
	return volume;
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
AudioMgrDOS::ImpAudioSampleVolume(int iResNum, int* iVolume, int iTag)   
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		fRetVal = True;
		*iVolume = sample[iSampleNumber].volume;			
	}

	return fRetVal;
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
AudioMgrDOS::ImpAudioSampleVolume(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
										  int* iVolume)   
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		fRetVal = True;
		*iVolume = sample[iSampleNumber].volume;			
	}

	return fRetVal;
}

/*******************************************************************************
 *  FUNCTION: 		ImpAudioVolumeNoEx - Not exported in library
 *
 *  PARAMETERS:	int iVolume - volume level 
 *										  0     - muted
 *										  1-127 - increasing loudness
 *										  128   - priority sample, mute all other samples
 *													 until this one is done playing at full
 *													 volume
 *						int iSample  - the sample number to pause
 *
 *  DESCRIPTION: 	Sets the volume of the specified sample 
 *
 *  RETURNS:		None
 ******************************************************************************/
void
AudioMgrDOS::ImpAudioVolumeNoEx(int iVolume, int iSample)   
{
	AudioVolume(iVolume, iSample);
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
AudioMgrDOS::ImpAudioVolumeAll(int iVolume)   
{
	ImpAudioVolumeNoEx(iVolume, ALL_SAMPLES);
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
AudioMgrDOS::ImpAudioVolume(int iResNum, int iVolume, int iTag)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		ImpAudioVolumeNoEx(iVolume, iSampleNumber);
	}
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
AudioMgrDOS::ImpAudioVolume(int iModule, int iNoun, int iVerb, int iCond, int iSequ, int iVolume)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		ImpAudioVolumeNoEx(iVolume, iSampleNumber);
	}
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
Boolean
AudioMgrDOS::ImpAudioFade(int iResNum, int iTargetVolume, int iTicks, int iSteps,
								Boolean fEnd, int iTag)
{
	Boolean fRetVal = True;
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		if (sample[iSampleNumber].volume == iTargetVolume) 
		{
			if (fEnd != False)
			{
				AudioStop(iSampleNumber);
				fRetVal = False;
			}
		}

		if (fRetVal)
		{
			if (iSteps == 0)
			{
				AudioVolume(iTargetVolume, iSampleNumber);
			}
			else 
			{
				#ifndef AUDIO_LIB
				fatal_test(timeMgr != 0);
				#endif
				sample[iSampleNumber].fadeVol   = iTargetVolume;
				sample[iSampleNumber].fadeTicks = iTicks;
				sample[iSampleNumber].fadeSteps = iSteps;
				sample[iSampleNumber].fadeEnd   = fEnd;
				sample[iSampleNumber].fadeLoc   = GetTicks();
			}
		}
	}

	return fRetVal;
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
Boolean
AudioMgrDOS::ImpAudioFade36(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
								  int iTargetVolume, int iTicks, int iSteps, Boolean fEnd)
{
	Boolean fRetVal = False;
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		if (sample[iSampleNumber].volume != iTargetVolume)
		{
			if (iSteps == 0)
			{
				AudioVolume(iTargetVolume,iSampleNumber);	
			}
			else
			{
				#ifndef AUDIO_LIB
				fatal_test(timeMgr != 0);
				#endif
				sample[iSampleNumber].fadeVol   = iTargetVolume;
				sample[iSampleNumber].fadeTicks = iTicks;
				sample[iSampleNumber].fadeSteps = iSteps;
				sample[iSampleNumber].fadeEnd   = fEnd;
				sample[iSampleNumber].fadeLoc   = GetTicks();
				fRetVal = True;
			}
		}
	}

	return fRetVal;
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
AudioMgrDOS::ImpAudioCheckNoise()
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
AudioMgrDOS::ImpAudioPan(int iResNum, int iPanPercent, int iTag)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		sample[iSampleNumber].pan = iPanPercent;
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
AudioMgrDOS::ImpAudioPan(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
						  int iPanPercent)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		sample[iSampleNumber].pan = iPanPercent;
	}
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
AudioMgrDOS::ImpAudioPanOff(int iResNum, int iTag)   
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iResNum, &iSampleNumber, iTag) )
	{
		sample[iSampleNumber].pan = -1;
	}
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
AudioMgrDOS::ImpAudioPanOff(int iModule, int iNoun, int iVerb, int iCond, int iSequ)
{
	int iSampleNumber;

	if ( ImpGetSampleNumber(iModule, iNoun, iVerb, iCond, iSequ, &iSampleNumber) )
	{
		sample[iSampleNumber].pan = -1;
	}
}

// Special case of WPLAY kernel call - probably obsolete
int
AudioMgrDOS::ImpAudioSampNotInMem()
{
	int saveCount = 0;
	for (int n = 0; n < activeSamples; n++)
		if (sample[n].fd != 0)
			saveCount++;
	return saveCount;      
}

