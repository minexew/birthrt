// audiodos.hpp

#ifndef AUDIODOS_HPP
#define AUDIODOS_HPP

const	unsigned DAC_ARGS_IN	= 5;

class AudioMgrDOS : public AudioMgr {

public:
	AudioMgrDOS();
	~AudioMgrDOS();

#ifndef AUDIO_LIB
	Boolean	AudioVMDStart(MemID,int,int,int);
	uint	 	AudioVMDQuery();
	Boolean	AudioVMDStop();
	Boolean	AudRobot(RobotAudInfo);
	Boolean	QueryAudRobot(RobotAudStatus*);
	Boolean	AudioFree(int);
	Boolean	AudioLock(ResNum,Boolean);
	Boolean	AudioPurge(ResNum);
	Boolean	AudioQueryDiscardable(ResNum);
	int	 	FindAudEntry(ResNum);
	int	 	FindAud36Entry(ResNum,uchar,uchar,uchar,uchar);
	void	 	InitAudioVols();
	void	 	MakeName36(MemType,char*,ResNum,uchar,uchar,uchar,uchar);
#ifdef DEBUG
	void	 	CheckDiscStreaming(char*);
#endif
#endif
	void	 	SubmitMaxBuffers() {}
	void		DACCallBack(int,int) {}

	Boolean	AudioPause(int);
	Boolean	AudioResume(int);
	void	 	AudioStop(int);
	void	 	EndAudio();
	Boolean	InitAudioDriver();
	void		AudioServer();

	Boolean	ImpInitializeAudioDriver();
	void		ImpTerminateAudioDriver();
	Boolean	ImpAudioInstalled();
	int	 	ImpAudioPreload(int iValue=-1);
	int	 	ImpAudioActiveSamples();
	int	 	ImpAudioWaitToPlay(int iResNum, Boolean fLoop=False,
										int iVolume=MAXVOLUME, int iTag=0);   
	int	 	ImpAudioWaitToPlay(int iModule, int iNoun, int iVerb, int iCond,
										int iSequ, Boolean fLoop=False, int iVolume=MAXVOLUME);
	int	 	ImpAudioPlay(int iResNum, Boolean fLoop=False, int iVolume=MAXVOLUME, int iTag=0, 
								  long fPause=False);
	int	 	ImpAudioPlay(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
						  Boolean fLoop=False, int iVolume=MAXVOLUME, Boolean fPause=False); 
	void	 	ImpAudioStopAll();
	void	 	ImpAudioStop(int iResNum, int iTag=0);
	void	 	ImpAudioStop(int iModule, int iNoun, int iVerb, int iCond, int iSequ);
	void	 	ImpAudioLoop(int iResNum, Boolean fLoop, int iTag=0);   
	void	 	ImpAudioLoop(int iModule, int iNoun, int iVerb, int iCond, 
							int iSequ, Boolean fLoop);   
	Boolean	ImpAudioPauseAll();   
	Boolean	ImpAudioPause(int iResNum, int iTag=0);
	Boolean	ImpAudioPause(int iModule, int iNoun, int iVerb, int iCond, int iSequ);
	Boolean	ImpAudioResumeAll();   
	Boolean	ImpAudioResume(int iResNum, int iTag=0);
	Boolean	ImpAudioResume(int iModule, int iNoun, int iVerb, int iCond, int iSequ);
	Boolean	ImpAudioDurationAll(int* iDuration);   
	Boolean	ImpAudioDuration(int iResNum, int* iDuration, int iTag=0);
	Boolean	ImpAudioDuration(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
									 int* iDuration);
	void	 	ImpAudioSetRate(int iRate);
	void	 	ImpAudioDefaultRate();
	int	 	ImpAudioCurrentRate();
	void	 	ImpAudioSetBits(int iBits);
	void	 	ImpAudioDefaultBits();
	int	 	ImpAudioCurrentBits();
	void	 	ImpAudioSetChannels(int iChannels);
	void	 	ImpAudioDefaultChannels();
	int	 	ImpAudioCurrentChannels();
	Boolean	ImpAudioGetVolReduction();
	void	 	ImpAudioSetVolReduction(Boolean fFlag);
	Boolean	ImpAudioGetDACCritical();
	void	 	ImpAudioSetDACCritical(Boolean fFlag);
	void	 	ImpAudioDistortAll(int iDistortionMask);   
	void	 	ImpAudioDistort(int iResNum, int iDistortionMask, int iTag=0);   
	void	 	ImpAudioDistort(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
								int iDistortionMask);
	int	 	ImpAudioGlobalVolume();
	Boolean	ImpAudioSampleVolume(int iResNum, int* iVolume, int iTag=0);   
	Boolean	ImpAudioSampleVolume(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
										  int* iVolume);
	void	 	ImpAudioVolumeAll(int iVolume);   
	void	 	ImpAudioVolume(int iResNum, int iVolume, int iTag=0);   
	void	 	ImpAudioVolume(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
							  int iVolume);   
	Boolean	ImpAudioFade(int iResNum, int iTargetVolume, int iTicks, int iSteps,
								Boolean fEnd=False, int iTag=0);
	Boolean	ImpAudioFade36(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
								  int iTargetVolume=0, int iTicks=30, int iSteps=10, 
								  Boolean fEnd=True);
	Boolean	ImpAudioCheckNoise();
	void	 	ImpAudioPan(int iResNum, int iPanPercent, int iTag=0);   
	void	 	ImpAudioPan(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
						  int iPanPercent);
	void	 	ImpAudioPanOff(int iResNum, int iTag=0);   
	void	 	ImpAudioPanOff(int iModule, int iNoun, int iVerb, int iCond, int iSequ);
	int	 	ImpAudioSampNotInMem(void);
	uint		AudioVMDPosLoc();
	void		PollWaveBuffs();
	void		PauseWaveDev(void);
	void		ResumeWaveDev(void);


private:
	Boolean	ImpGetSampleNumber(int iResNum, int* iSampleNumber, int iTag);
	Boolean	ImpGetSampleNumber(int iModule, int iNoun, int iVerb, int iCond, 
										int iSequ, int* iSampleNumber); 
	void		ImpAudioStopNoEx(int iSample);
	Boolean	ImpAudioPauseNoEx(int iSample);
	Boolean	ImpAudioResumeNoEx(int iSample);
	Boolean	ImpAudioDurationNoEx(int iSample, int* iDuration);
	void		ImpAudioDistortNoEx(int iDistortionMask, int iSample); 
	void		ImpAudioVolumeNoEx(int iVolume, int iSample);   
	void		AudioServer();
	int		CallDAC(int,int);
	void		AdjustAudBuffSizes(unsigned *,unsigned *);
	Boolean	GetDACBuffer(void);
	int		FindSampleN(short*,int,int tag = 0);
	void		AudioPlay();
	void		AudioRate(int);
	void		AudioBits(int);
	void		AudioChannels(int);
	void		AudioVolume(int,int);
	int		CheckNoise(void);
	int		AudioLoc(int);
	void		AudioDistort(int,int);
	int		AudioSelect(int);
	void		AudioCheck();
	Boolean	AudioFade(int);
	void		AudioUnLoad();
	Boolean	CallDAC(int,int);
	void		CheckRawFormat(int);
	Boolean	CheckSOLFormat(int);
	Boolean	CheckWAVEFormat(int);
	void		ConvBase36(char*,int,int);
	void		DistortBuffer(int,char *);
	void		DropSample(int);
	void		FillBuffer(int);
	void		FreeAudioDrv(void);
	char		GetDigit36(int);
	unsigned GetDACDefaultSize(void);
	void*		LoadAudioDrv(void);
	Boolean	MergeBuffer(int,int);
	Boolean	MergeBuffers(void);
	static	Boolean	StaticMergeBuffers(void);
	void		VolumeBuffer(int,char *);
	#ifndef AUDIO_LIB
	void		FillRbotBuffer8(int);
	void		FillRbotBuffer16(int);
	void		FillVMD(void);
	#endif
	#ifdef DEBUG
	void		DACPlay();
	int		GetBase36(char*,int);
	#endif
	void		AudioSwap(int which);

	struct Sample {
		uchar		bits;
		MemID		buffID;
		uchar		bufferNumber;
		int		bytesBuffered;
		int		bytesRead;
		int		bytesToRead;
		uchar		channels;
		int		compressed;
		int		compValL;
		int		compValR;
		uchar		cond;
		int		distortion;
		int		div;
		int		fadeLoc;
		Boolean	fadeEnd;
		int		fadeSteps;
		int		fadeTicks;
		int		fadeVol;
		int		fd;
		int		fileOfs;
		uchar		fillToggle;
		int		length;
		Boolean	loop;
		MemID		memID;
		ResNum	module;
		int		mul;
		uchar		noun;
		int		num;
		int		pan;
		int		patch;
		int		pausedLoc;
		int		rate;
		int		restartLoc;
		uchar		sequ;
		int		start;
		int		startLoc;
		int		tag;
		int		ticks;
		uchar		verb;
		int		volume;
		#ifndef AUDIO_LIB
		int		attrs;
		Boolean	robot;
		Boolean	vmd;
		Boolean	swapped;		 	  
		int		handleFile;	 	  
		#endif
	};

	Sample sample[MAXSAMPLES+1];

	int		activeSamples;
	int		bytesPerTick;
	uint		callBacks;
	MemID		convID;
	MemID		convIDx;
	Boolean	critical;
	int		distortion;
	int		DACArgsIn[DAC_ARGS_IN];
	int		DACHbufferSize;
	int		ffloc;
	int		intHbufferSize;
	int		globalLoc;
	Boolean	lastDAC;
	int		mbTickCount;
	Boolean	mixCheck;
	int		noiseOfs;
	int		pausedLoc;
	int		preLoad;
	MemID		priorityID;
	int		prioritySample;
	int		rate;
	int		bits;
	int		channels;
	int		volume;
	int		rateMax;
	int		bitsMax;
	int		channelsMax;
	int		volumeMax;
	Boolean	rateSet;
	Boolean	bitsSet;
	Boolean	channelsSet;
	Boolean	staticFilter;

	#ifndef AUDIO_LIB
	#define	ROBOT_RATE		22050
	#define	ROBOT_BITS		16
	#define	ROBOT_CHANNELS	1
	RobotAudInfo rbot;
	int		robotCallBacks;
	Boolean	robotsPaused;
	uint		startVMD;
	uint		vmdQuery;
	#endif
};


#endif

