#ifdef AUDIO_LIB
#define LIBTRACE(x)
#else
#include "sol.hpp"
#endif

#include "kerncall.hpp"
#include "kernaud.hpp"

#include "audio.hpp"
#ifdef WINDOWS
#include "audiowin.hpp"
#else
#include "audiodos.hpp"
#endif
#include "custmsys.hpp"

//------------------------------------------------------------------------------------------
//	@doc AUDLIB	
//
// KERNAUD.CPP -- C++ Kernel Audio Functions 
//
//	Date:			 03/18/96
// Created by:  Jay Lee
//
// This file contains the set of functions that implements a C++ interface into the system
// audio functions.  The functions here have dual purposes - the first is to be the set of
// functions used to access the functionality of the system audio support for programmers 
// writing games in C++.  The second role is to be a layer through which the Kernel calls for
// SCI programmers access the system classes.  
//------------------------------------------------------------------------------------------

#ifdef AUDIO_LIB

KernelInterface* kernel = 0;  // So it is allocated and the user of the audio library does
										// not have to think about it. 
/*******************************************************************************
 * @func 
 *                                               
 * Initializes the audio driver for the stand-alone audio library only.  Do not
 *	call this function except when using the stand-alone audio library
 *                                              
 * @rvalue True  | audio driver initilized            
 * @rvalue False | unable to initialize audio driver         	
 ******************************************************************************/
Boolean KernelAudioInitialize()
{ 
	if(kernel != 0) return(0);

	kernel = new KernelInterface;

	if(kernel == 0) return(0);

#ifdef WINDOWS
	audioMgr = new AudioMgrWin;
#else
	audioMgr = new AudioMgrDOS;
#endif

	if(audioMgr == 0) return(0);

	return audioMgr->ImpInitializeAudioDriver();
}

/*******************************************************************************
 * @func 
 *                                               
 * Terminates the audio driver for the stand-alone audio library only.  Do not
 *	call this function except when using the stand-alone audio library
 ******************************************************************************/
void KernelAudioTerminate()
{ 
	if(kernel == 0) return;
	if(audioMgr == 0) return;

	audioMgr->ImpTerminateAudioDriver();

	delete kernel;
}

/*******************************************************************************
 * @func 
 *                                               
 * Services the audio, that is ensures that the buffers are appropriately filled
 * so that there is no audio breakup.  This should be called during idle processing
 * or in between long duration activities such as file reads so that audio can
 * continue to play.  This should only be called for the stand-alone library
 ******************************************************************************/
void KernelAudioService()
{ 
	if(kernel == 0) return;

	int i;
	kernel->AudioDurationAll(&i);
}

#endif // #ifdef AUDIO_LIB

// @doc AUDLIB KERNEL

/*******************************************************************************
 * @mfunc 
 *                                               
 * Returns the value of the audio installed flag	
 *                                              
 * @rvalue True  | audio is installed             
 * @rvalue False | audio not installed          	
 ******************************************************************************/
Boolean KernelInterface::AudioInstalled()
{ 
	LIBTRACE("KernelInterface::AudioInstalled");

	return audioMgr->ImpAudioInstalled();
}

/*******************************************************************************
 * @mfunc 
 *                                                                 
 *	Sets the value of the internal preload variable which           
 *	indicates which audio resources to preload, starting with       
 *	the value specified.  E.g. if 2 is specified, the 2nd, third,   
 *	etc. samples will be preloaded.  If the value is 1, all samples  
 *	will be preloaded.                                              
 *	                                                               
 *	@parm int | iValue | value to set the preLoad indicator (defaults to
 *								0, which does not set the value, but just returns
 *								its current value)
 *	                                                               
 * @rdesc the current value of the preLoad indicator               
 ******************************************************************************/
int KernelInterface::AudioPreload(int iValue)   
{
   LIBTRACE("KernelInterface::AudioPreload");
   
	if (iValue < 0) return(0);
	if(!KernelInterface::AudioInstalled()) return (0);

	return audioMgr->ImpAudioPreload(iValue);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Adds the sample, but  waits for Resume to play it.  If the sample exists and 
 *	is paused, just resumes playing it.                                          
 *	                                                                            
 *	@parm int     | iResNum | resource number of the sample                          
 *	@parm Boolean | fLoop   | True to loop sample indefinitely,                    
 *									  False to play sample once (defaults to False)                           
 *	@parm int     | iVolume | volume level (defaults to 127)                                          
 *		@flag 0       | muted                                                              
 *		@flag 1-127   | increasing loudness                                            
 *		@flag 128     | priority sample, mute all other samples                          
 *					       until this one is done playing at full volume                    
 *	@parm int     | iTag	   | identifier to allow use of same sample more               
 *							        than once, any value > 0 (defaults to 0)                                 
 *	                                                                            
 * @rvalue 0     | no selection to play                                            
 *	@rvalue > 0   | time in 1/60 seconds to play selection                         
 ******************************************************************************/
int KernelInterface::AudioWaitToPlay(int iResNum, Boolean fLoop, int iVolume, int iTag)   
{
	LIBTRACE("KernelInterface::AudioWaitToPlay - resource number");

	if(!KernelInterface::AudioInstalled()) return(0);
	if(iResNum < 0) return(0);
	if((fLoop != True) && (fLoop != False) ) fLoop = False;
	if( iVolume < 0) iVolume = 0;
	if(iVolume > 127) iVolume = 127;
	if(iTag < 0) return(0);

	return( audioMgr->ImpAudioWaitToPlay(iResNum, fLoop, iVolume, iTag) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                 
 *	Returns the number of samples currently active
 *	                                                               
 * @rdesc number of active samples            
 ******************************************************************************/
int KernelInterface::AudioActiveSamples()
{
	LIBTRACE("KernelInterface::AudioActiveSamples");

	if(!KernelInterface::AudioInstalled()) return(0);

	return( audioMgr->ImpAudioActiveSamples());
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Adds the sample and starts playing it.  If the sample exists and
 *	is paused, just resumes playing it.
 *	                                                                            
 *	@parm int     | iResNum | resource number of the sample                          
 *	@parm Boolean | fLoop   | True to loop sample indefinitely,                    
 *									  False to play sample once (defaults to False)                           
 *	@parm int     | iVolume | volume level (defaults to 127)                                          
 *		@flag 0       | muted                                                              
 *		@flag 1-127   | increasing loudness                                            
 *		@flag 128     | priority sample, mute all other samples                          
 *						    until this one is done playing at full volume                    
 *	@parm int     | iTag	   | identifier to allow use of same sample more               
 *							        than once, any value > 0 (defaults to 0)                                 
 *	@parm long    | fPause  | True if wait to play, False otherwise.  This is
 *									  a long as opposed to a Boolean to avoid ambiguity.	
 *									  (Note - don't specify, call KernelInterface::AudioWaitToPlay
 *									  instead)
                                                                            
 * @rvalue 0     | no selection to play                                            
 *	@rvalue > 0   | time in 1/60 seconds to play selection                         
 ******************************************************************************/
int KernelInterface::AudioPlay(int iResNum, Boolean fLoop, int iVolume, int iTag, 
						             long fPause)   
{
	LIBTRACE("KernelInterface::AudioPlay - resource number");
	
	if(!KernelInterface::AudioInstalled())
		return(0);
	if( ( iResNum < 1) || (iTag < 1) )
		return(0);

	if( (fLoop != True) && (fLoop != False) )	fLoop = False;
	if( iVolume < 0) 	iVolume = 0;
	if(iVolume > 127)	iVolume = 127;
	if( (fPause != True) && (fPause != False) ) fPause = False;


	return( audioMgr->ImpAudioPlay(iResNum, fLoop, iVolume, iTag, fPause) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                 
 *	Stops all the samples that are playing
 ******************************************************************************/
void KernelInterface::AudioStopAll()   
{
   LIBTRACE("KernelInterface::AudioStopAll");

	if(!KernelInterface::AudioInstalled()) return;

	audioMgr->ImpAudioStopAll();
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Stops playing the sample identified by iResNum and optional 
 *	iTag value combination
 *	                                                                            
 *	@parm int     | iResNum | resource number of the sample                          
 *	@parm int     | iTag	   | tag associated with sample, 0 if none (defaults to 0)
 ******************************************************************************/
void KernelInterface::AudioStop(int iResNum, int iTag)   
{
   LIBTRACE("KernelInterface::AudioStop - with ResNum/Tag");
   
	if(!KernelInterface::AudioInstalled()) return;
	if(iResNum < 0) return;
	if (iTag < 0) return;

	audioMgr->ImpAudioStop(iResNum, iTag);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Loops the sample identified by iResNum and optional 
 *	iTag value combination the specified number of times
 *	                                                                            
 *	@parm int     | iResNum | resource number of the sample                          
 *	@parm Boolean | fLoop   | True to loop sample indefinitely,                    
 *									  False to play sample once                            
 *	@parm int     | iTag	   | tag associated with sample, 0 if none (defaults to 0)
 ******************************************************************************/
void KernelInterface::AudioLoop(int iResNum, Boolean fLoop, int iTag)   
{
   LIBTRACE("KernelInterface::AudioLoop - with ResNum/Tag");

	if(!KernelInterface::AudioInstalled()) return;
	if( (fLoop != True) && (fLoop != False) )return;
	if(iResNum < 0) return;
	if(iTag < 0) return;

	audioMgr->ImpAudioLoop(iResNum, fLoop, iTag);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Pauses all active samples
 *
 *	@rvalue True  | one or more samples were paused
 *	@rvalue False | no samples were paused
 ******************************************************************************/
Boolean KernelInterface::AudioPauseAll()   
{
   LIBTRACE("KernelInterface::AudioPauseAll");

	if(!KernelInterface::AudioInstalled()) return(0);

	return( audioMgr->ImpAudioPauseAll() );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Pauses the sample identified by iResNum and optional 
 *	iTag value combination
 *	                                                                            
 *	@parm int     | iResNum | resource number of the sample                          
 *	@parm int     | iTag	   | tag associated with sample, 0 if none (defaults to 0)
 *
 *	@rvalue True  | sample was paused
 *	@rvalue False | no samples were paused
 ******************************************************************************/
Boolean KernelInterface::AudioPause(int iResNum, int iTag)   
{
   LIBTRACE("KernelInterface::AudioPause - with ResNum/Tag");
   
	if(!KernelInterface::AudioInstalled()) return(0);
	if(iResNum < 0) return (0);
	if(iTag <0) return(0);

	return( audioMgr->ImpAudioPause(iResNum, iTag) );
}

 /*******************************************************************************
 * @mfunc 
 *                                                                              
 * Resumes all paused samples
 *
 *	@rvalue True  | one or more samples were resumed
 *	@rvalue False | no samples were resumed
 ******************************************************************************/
Boolean KernelInterface::AudioResumeAll()   
{
   LIBTRACE("KernelInterface::AudioResumeAll");

	if(!KernelInterface::AudioInstalled()) return(0);

	return( audioMgr->ImpAudioResumeAll() );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Resumes the sample identified by iResNum and optional 
 *	iTag value combination
 *	                                                                            
 *	@parm int     | iResNum | resource number of the sample                          
 *	@parm int     | iTag	   | tag associated with sample, 0 if none (defaults to 0)
 *
 *	@rvalue True  | sample was resumed
 *	@rvalue False | no samples were resumed
 ******************************************************************************/
Boolean KernelInterface::AudioResume(int iResNum, int iTag)   
{
   LIBTRACE("KernelInterface::AudioResume - with ResNum/Tag");
   
	if(!KernelInterface::AudioInstalled()) return(0);
	if(iResNum < 0) return (0);
	if(iTag <0) return(0);

	return( audioMgr->ImpAudioResume(iResNum, iTag) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Gets the duration in 1/60ths of a second since the start of the initial 
 *	sample.  iDuration is updated with this value.
 *
 *	@parm int*    | iDuration | pointer to int to updates with duration value
 *
 *	@rvalue True  | duration successfully determined
 *	@rvalue False | unable to determine duration
 ******************************************************************************/
Boolean KernelInterface::AudioDurationAll(int* iDuration)   
{
   LIBTRACE("KernelInterface::AudioDurationAll");

	if(!KernelInterface::AudioInstalled()) return(0);
	if(iDuration == 0) return (0);

	return( audioMgr->ImpAudioDurationAll(iDuration) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Updates iDuration with the duration in 60ths of a second since
 *	the start of the sample identified by iResNum and optional iTag
 *	value combination
 *	                                                                            
 *	@parm int     | iResNum   | resource number of the sample
 *	@parm int*    | iDuration | pointer to int to updates with duration value
 *	@parm int     | iTag	     | tag associated with sample (0 if none)
 *
 *	@rvalue True  | duration successfully determined
 *	@rvalue False | unable to determine duration
 ******************************************************************************/
Boolean KernelInterface::AudioDuration(int iResNum, int* iDuration, int iTag)   
{
   LIBTRACE("KernelInterface::AudioDuration - with ResNum/Tag");
   
	if(iDuration == 0) return (0);
	if(!KernelInterface::AudioInstalled()) return(0);
	if(iResNum < 0) return (0);
	if(iTag <0) return(0);

	return( audioMgr->ImpAudioDuration(iResNum, iDuration, iTag) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the audio samples to playback at the specified rate, or
 *	the maximum rate of the DAC, whichever is less.
 *
 *	@parm int  | iRate | the rate at which to play back the samples
 ******************************************************************************/
void KernelInterface::AudioSetRate(int iRate)
{
   LIBTRACE("KernelInterface::AudioSetRate");

	if(!KernelInterface::AudioInstalled()) return;
	if(KernelInterface::AudioActiveSamples() != 0) return;
	
	if(iRate == 11025 || iRate == 22050 || iRate == 44100)
		audioMgr->ImpAudioSetRate(iRate);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the audio samples to playback at their recorded rate
 *	or the maximum rate of the DAC, whichever is less.
 ******************************************************************************/
void KernelInterface::AudioDefaultRate()
{
   LIBTRACE("KernelInterface::AudioDefaultRate");

	if(KernelInterface::AudioInstalled())
		audioMgr->ImpAudioDefaultRate();
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Returns the current playback rate
 *
 * @rdesc playback rate
 ******************************************************************************/
int KernelInterface::AudioCurrentRate()
{
   LIBTRACE("KernelInterface::AudioCurrentRate");

	if(!KernelInterface::AudioInstalled()) return(0);

	return( audioMgr->ImpAudioCurrentRate() );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the audio samples to playback at the specified bit rate, or
 *	the maximum bit-rate of the DAC, whichever is less.
 *
 *	@parm int  | iBits | the bit rate at which to play back the samples
 ******************************************************************************/
void KernelInterface::AudioSetBits(int iBits)
{
   LIBTRACE("KernelInterface::AudioSetBits");

	if(!KernelInterface::AudioInstalled()) return;

	if(KernelInterface::AudioActiveSamples() != 0) return;

	if(iBits == 8 || iBits == 16)
		audioMgr->ImpAudioSetBits(iBits);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the audio samples to playback at their recorded bit rate
 *	or the maximum bit rate of the DAC, whichever is less.
 ******************************************************************************/
void KernelInterface::AudioDefaultBits()
{
   LIBTRACE("KernelInterface::AudioDefaultBits");

	if(KernelInterface::AudioInstalled())
		audioMgr->ImpAudioDefaultBits();
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Returns the current bit rate
 *
 * @rdesc bit rate
 ******************************************************************************/
int KernelInterface::AudioCurrentBits()
{
   LIBTRACE("KernelInterface::AudioCurrentBits");

	if(!KernelInterface::AudioInstalled()) return(0);

	return( audioMgr->ImpAudioCurrentBits() );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the audio samples to playback at the specified number of channels, or
 *	the maximum channels of the DAC, whichever is less.
 *
 *	@parm int  | iChannels | the number of channels with which to play back the samples
 ******************************************************************************/
void KernelInterface::AudioSetChannels(int iChannels)
{
   LIBTRACE("KernelInterface::AudioSetChannels");

	if(!KernelInterface::AudioInstalled()) return;
	if(KernelInterface::AudioActiveSamples() != 0) return;
	if(iChannels == 1 || iChannels == 2);
		audioMgr->ImpAudioSetChannels(iChannels);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the audio samples to playback at their recorded number of 
 *	channels or the maximum number of channels of the DAC, whichever
 *	is less.
 ******************************************************************************/
void KernelInterface::AudioDefaultChannels()
{
   LIBTRACE("KernelInterface::AudioDefaultChannels");

	if(KernelInterface::AudioInstalled())
		audioMgr->ImpAudioDefaultChannels();
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Returns the current number of channels
 *
 *	@rdesc number of channels
 ******************************************************************************/
int KernelInterface::AudioCurrentChannels()
{
   LIBTRACE("KernelInterface::AudioCurrentChannels");

	if(!KernelInterface::AudioInstalled()) return(0);

	return( audioMgr->ImpAudioCurrentChannels() );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Gets the volume reduction setting (whether simultaneous samples 
 *	will be reduced in volume when playing to prevent overflow)
 *
 *	@rvalue True  | volume reduction is on
 * @rvalue False | volume reduction is off
 ******************************************************************************/
Boolean KernelInterface::AudioGetVolReduction()
{
   LIBTRACE("KernelInterface::AudioGetVolReduction");

	if(!KernelInterface::AudioInstalled()) return(0);

	return( audioMgr->ImpAudioGetVolReduction() );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the volume reduction setting (whether simultaneous samples 
 *	will be reduced in volume when playing to prevent overflow) to
 *	the specified value
 *
 *	@parm Boolean  | fFlag | whether to use volume reduction on simultaneous
 *									 samples (True for yes, False for no)
 ******************************************************************************/
void KernelInterface::AudioSetVolReduction(Boolean fFlag)
{
   LIBTRACE("KernelInterface::AudioSetVolReduction");

	if(!KernelInterface::AudioInstalled()) return;
	if(KernelInterface::AudioActiveSamples() != 0) return;
	if(fFlag == True || fFlag == False)
		audioMgr->ImpAudioSetVolReduction(fFlag);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Distorts all current samples using the specified mask
 *
 *	@parm int  | iDistortionMask | mask used for distortion
 *		@flag 0       | distortion off
 *		@flag 1 - 255 | minimum to maximum distortion
 ******************************************************************************/
void KernelInterface::AudioDistortAll(int iDistortionMask)   
{
	LIBTRACE("KernelInterface::AudioDistortAll");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iDistortionMask >= 0 && iDistortionMask <= 255);

	audioMgr->ImpAudioDistortAll(iDistortionMask);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Distorts the sample identified by iResNum and optional 
 *	iTag value combination using the specified mask
 *	                                                                            
 *	@parm int  | iResNum         | resource number of the sample
 *	@parm int  | iDistortionMask | mask used for distortion
 *		@flag 0       | distortion off
 *		@flag 1 - 255 | minimum to maximum distortion
 *	@parm int  | iTag				  | tag associated with sample, 0 if none (defaults to 0)
 ******************************************************************************/
void KernelInterface::AudioDistort(int iResNum, int iDistortionMask, int iTag)   
{
	LIBTRACE("KernelInterface::AudioDistort - with ResNum/Tag");
   
	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iDistortionMask >= 0 && iDistortionMask <= 255);
	fatal_test(iResNum >= 0);
//	fatal_test(iTag >= 0);

	audioMgr->ImpAudioDistort(iResNum, iDistortionMask, iTag);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Returns the value of the global volume level across all samples
 *
 *	@rdesc volume level
 ******************************************************************************/
int KernelInterface::AudioGlobalVolume()   
{
	LIBTRACE("KernelInterface::AudioGlobalVolume");

	if(!KernelInterface::AudioInstalled()) return(0);

	return( audioMgr->ImpAudioGlobalVolume() );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Retrieves the volume of the sample identified by iResNum and optional 
 *	iTag value combination
 *	                                                                            
 *	@parm int     | iResNum | resource number of the sample
 *	@parm int*    | iVolume | pointer to int to updates with volume level
 *	@parm int     | iTag	   | tag associated with sample, 0 if none (defaults to 0)
 *
 *	@rvalue True  | volume of sample determined
 *	@rvalue False | unable to determine volume sample
 ******************************************************************************/
Boolean KernelInterface::AudioSampleVolume(int iResNum, int* iVolume, int iTag)   
{
	LIBTRACE("KernelInterface::AudioSampleVolume - with ResNum/Tag");
	
	if(iVolume <= 0) return(0);
	if(!KernelInterface::AudioInstalled()) return(0);
	if(iResNum < 0) return (0);
	if(iTag <0) return(0);

	return( audioMgr->ImpAudioSampleVolume(iResNum, iVolume, iTag) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the volume level of all samples
 *	                                                                            
 *	@parm int  | iVolume   | volume level 
 *		@flag 0       | muted 
 *		@flag 1-127   | increasing loudness
 ******************************************************************************/
void KernelInterface::AudioVolumeAll(int iVolume)   
{
	LIBTRACE("KernelInterface::AudioVolumeAll");

	if(!KernelInterface::AudioInstalled()) return;
	if(iVolume < 0) iVolume = 0;
	if( iVolume > 127) iVolume = 127;

	audioMgr->ImpAudioVolumeAll(iVolume);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the volume of the sample identified by iResNum and optional 
 *	iTag value combination to the specified value
 *	                                                                            
 *	@parm int     | iResNum   | resource number of the sample
 *	@parm int	  | iVolume   | volume level
 *		@flag 0       | muted 
 *		@flag 1-127   | increasing loudness
 *		@flag 128     | priority sample, mute all other samples
 *					       until this one is done playing at full volume
 *	@parm int     | iTag	     | tag associated with sample, 0 if none (defaults to 0)
 ******************************************************************************/
void KernelInterface::AudioVolume(int iResNum, int iVolume, int iTag)   
{
	LIBTRACE("KernelInterface::AudioVolume - with ResNum/Tag");
	
	if(iVolume < 0) iVolume = 0;
	if(iVolume > 127) iVolume = 127;
	if(!KernelInterface::AudioInstalled()) return;
	if(iResNum < 0) return;
	if(iTag <0) return;

	audioMgr->ImpAudioVolume(iResNum, iVolume, iTag);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Fades the sample identified by iResNum and optional iTag
 *	combination to the target volume using the specified characteristics
 *	                                                                            
 *	@parm int     | iResNum       | resource number of the sample
 *	@parm int	  | iTargetVolume | volume level to fade to
 * @parm int	  | iTicks			| number of 1/60 seconds between each
 *											  incremental volume change
 * @parm int	  | iStep			| the number of incremental volume changes
 *											  to make to get to the target volume , if
 *											  0 , target volume will be reached at once
 *	@parm Boolean | fEnd				| whether to end the sample when the
 *											  target volume is reached or
 *											  to continue playing the sample at
 *											  the target volume (defaults to False)
 *	@parm int     | iTag	         | tag associated with sample, 0 if none (defaults to 0)
 *
 * @rvalue True  | fade successfully initiated
 * @rvalue False | unable to initiate fade
 ******************************************************************************/
Boolean KernelInterface::AudioFade(int iResNum, int iTargetVolume, int iTicks, 
											  int iSteps, Boolean fEnd, int iTag)
{
	LIBTRACE("KernelInterface::AudioFade");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iResNum >= 0);
	fatal_test(iTargetVolume >= 0 && iTargetVolume <= MAXVOLUME);
	fatal_test(iTicks >= 0);
	fatal_test(iSteps > 0); 
	fatal_test(fEnd == True || fEnd == False);
//	fatal_test(iTag >= 0);

	return( audioMgr->ImpAudioFade(iResNum, iTargetVolume, iTicks, iSteps, fEnd, iTag) );
}		

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Determines whether the priority sample is currently audible
 *	                                                                            
 * @rvalue True  | priority sample is audible
 * @rvalue False | priority sample not audible or no priority sample is playing
 ******************************************************************************/
Boolean KernelInterface::AudioCheckNoise()
{
	LIBTRACE("KernelInterface::AudioCheckNoise");

	fatal_test(KernelInterface::AudioInstalled());

	return( audioMgr->ImpAudioCheckNoise() );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Pans the sample identified by iResNum and optional 
 *	iTag value combination using the specified pan percentage
 *	                                                                            
 *	@parm int  | iResNum       | resource number of the sample
 *	@parm int  | iPanPercent   | percentage to pan sample on right speaker,
 *				  						  left speaker will get remainder of this
 *				  						  value subtracted from 100
 *	@parm int  | iTag				| tag associated with sample, 0 if none (defaults to 0)
 ******************************************************************************/
void KernelInterface::AudioPan(int iResNum, int iPanPercent, int iTag)   
{
   LIBTRACE("KernelInterface::AudioPan - with ResNum/Tag");

	if(!KernelInterface::AudioInstalled()) return;
   if(iPanPercent < 0) iPanPercent = 0;
   if( iPanPercent > 100) iPanPercent = 100;
	if(iResNum < 0) return;
	if(iTag <0) return;

	if(KernelInterface::AudioCurrentChannels() != 2)
		return;

	audioMgr->ImpAudioPan(iResNum, iPanPercent, iTag);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Turns of panning for the sample identified by iResNum and optional 
 *	iTag value combination
 *	                                                                            
 *	@parm int  | iResNum | resource number of the sample
 *	@parm int  | iTag	   | tag associated with sample, 0 if none (defaults to 0)
 ******************************************************************************/
void KernelInterface::AudioPanOff(int iResNum, int iTag)   
{
   LIBTRACE("KernelInterface::AudioPanOff - with ResNum/Tag");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iResNum >= 0);
//	fatal_test(iTag >= 0);

	audioMgr->ImpAudioPanOff(iResNum, iTag);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Returns the value of the DAC critical flag, which syncs a robot
 *	that plays with existing samples if False, or allows a delay
 *	in the robot audio until it merges in if True
 *
 *	@rvalue True  | DAC critical flag is on
 *	@rvalue False | DAC critical flag is off
 ******************************************************************************/
Boolean KernelInterface::AudioGetDACCritical()
{
   LIBTRACE("KernelInterface::AudioGetDACCritical");

	if(!KernelInterface::AudioInstalled()) return(0);

	return( audioMgr->ImpAudioGetDACCritical() );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the value of the DAC critical flag, which syncs a robot
 *	that plays with existing samples if False, or allows a delay
 *	in the robot audio until it merges in if True
 *	Note: this function takes no action if the DACCritical
 *	entry is specified in the config file, and the value is
 *	set to True
 *
 *	@parm Boolean  | fFlag | True if there will be a delay in playing robot
 *									 audio, False if existing samples will be stopped
 *									 and restarted at the same point when a robot
 *									 with audio is started
 ******************************************************************************/
void KernelInterface::AudioSetDACCritical(Boolean fFlag)
{
	LIBTRACE("KernelInterface::AudioSetDACCritical");

	if(!KernelInterface::AudioInstalled()) return;
	fatal_test(fFlag == True || fFlag == False);

	audioMgr->ImpAudioSetDACCritical(fFlag);
}


// @doc KERNEL
// NOTE: All functions below are not to be exported for the stand-alone audio 
//       library
//

#ifndef AUDIO_LIB

/*******************************************************************************
 *	@mfunc 
 *  
 * Add the sample, but waits for Resume to play it.  If the sample exists 
 *	and is paused, just resumes playing it.
 *
 *	@parm int     | iModule | module number of the sample
 *	@parm	int     | iNoun   | noun value of the sample
 *	@parm	int     | iVerb   | verb value of the sample
 *	@parm	int	  | iCond   | condition value of the sample
 *	@parm	int     | iSequ   | sequence value of the sample
 *	@parm int     | iResNum | resource number of the sample
 *	@parm Boolean | fLoop   | True to loop sample indefinitely,
 *									  False to play sample once  (defaults to False)
 *	@parm int	  | iVolume | volume level	(defaults to 127)
 *		@flag 0       | muted 
 *		@flag 1-127   | increasing loudness
 *		@flag 128     | priority sample, mute all other samples
 *					       until this one is done playing at full volume
 *
 * @rvalue 0     | no selection to play
 *	@rvalue > 0   | time in 1/60 seconds to play selection
 ******************************************************************************/
int KernelInterface::AudioWaitToPlay(int iModule, int iNoun, int iVerb, int iCond,
												 int iSequ, Boolean fLoop, int iVolume)   
{
	LIBTRACE("KernelInterface::AudioWaitToPlay - module, noun, verb, cond, sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);
	fatal_test(fLoop == True || fLoop == False);
	fatal_test(iVolume >= 0 && iVolume <= 128);

	return( audioMgr->ImpAudioWaitToPlay(iModule, iNoun, iVerb, iCond, iSequ, fLoop, iVolume) );
}

/*******************************************************************************
 *	@mfunc 
 *  
 *	Adds the sample and starts playing it.  If the sample exists and
 *	is paused, just resumes playing it.
 *
 *	@parm int     | iModule | module number of the sample
 *	@parm	int     | iNoun   | noun value of the sample
 *	@parm	int     | iVerb   | verb value of the sample
 *	@parm	int	  | iCond   | condition value of the sample
 *	@parm	int     | iSequ   | sequence value of the sample
 *	@parm Boolean | fLoop   | True to loop sample indefinitely,
 *									  False to play sample once (defaults to False)
 *	@parm int	  | iVolume | volume level	(defaults to 127)
 *		@flag 0       | muted 
 *		@flag 1-127   | increasing loudness
 *		@flag 128     | priority sample, mute all other samples
 *					       until this one is done playing at full volume
 * @parm Boolean | fPause  | True if wait to play, False otherwise
 *									  (Note - don't specify, call KernelInterface::AudioWaitToPlay
 *									   instead)
 *
 * @rvalue 0     | no selection to play
 *	@rvalue > 0   | time in 1/60 seconds to play selection
 ******************************************************************************/
int KernelInterface::AudioPlay(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
										 Boolean fLoop, int iVolume, Boolean fPause)  
{
	LIBTRACE("KernelInterface::AudioPlay - module, noun, verb, cond, sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);
	fatal_test(fLoop == True || fLoop == False);
	fatal_test(iVolume >= 0 && iVolume <= 128);
	fatal_test(fPause == True || fPause == False);

	return( audioMgr->ImpAudioPlay(iModule, iNoun, iVerb, iCond, iSequ, fLoop, iVolume, fPause) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Stops playing the sample identified by the module, noun, verb,
 *	condition and sequence combination
 *	                                                                            
 *	@parm int     | iModule | module number of the sample
 *	@parm	int     | iNoun   | noun value of the sample
 *	@parm	int     | iVerb   | verb value of the sample
 *	@parm	int	  | iCond   | condition value of the sample
 *	@parm	int     | iSequ   | sequence value of the sample
 ******************************************************************************/
void KernelInterface::AudioStop(int iModule, int iNoun, int iVerb, int iCond, 
										  int iSequ)   
{
   LIBTRACE("KernelInterface::AudioStop - Module, Noun, Verb, Cond, Sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);

	audioMgr->ImpAudioStop(iModule, iNoun, iVerb, iCond, iSequ);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Loops the sample identified by the module, noun, verb,
 *	condition and sequence combination the specified number of
 *	times.
 *
 *	@parm int     | iModule | module number of the sample
 *	@parm	int     | iNoun   | noun value of the sample
 *	@parm	int     | iVerb   | verb value of the sample
 *	@parm	int	  | iCond   | condition value of the sample
 *	@parm	int     | iSequ   | sequence value of the sample
 *	@parm Boolean | fLoop   | True to loop sample indefinitely,                    
 *									  False to play sample once                            
 ******************************************************************************/
void KernelInterface::AudioLoop(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
										  Boolean fLoop)   
{
   LIBTRACE("KernelInterface::AudioLoop - Module, Noun, Verb, Cond, Sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);
	fatal_test(fLoop == True || fLoop == False);

	audioMgr->ImpAudioLoop(iModule, iNoun, iVerb, iCond, iSequ, fLoop);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Stops playing the sample identified by the module, noun, verb,
 *	condition and sequence combination
 *
 *	@parm int     | iModule | module number of the sample
 *	@parm	int     | iNoun   | noun value of the sample
 *	@parm	int     | iVerb   | verb value of the sample
 *	@parm	int	  | iCond   | condition value of the sample
 *	@parm	int     | iSequ   | sequence value of the sample
 *
 *	@rvalue True  | sample was paused
 *	@rvalue False | no samples were paused
 ******************************************************************************/
Boolean KernelInterface::AudioPause(int iModule, int iNoun, int iVerb, int iCond,
												int iSequ)   
{
   LIBTRACE("KernelInterface::AudioPause - Module, Noun, Verb, Cond, Sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);

	return( audioMgr->ImpAudioPause(iModule, iNoun, iVerb, iCond, iSequ) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Resumes the sample identified by the module, noun, verb,
 *	condition and sequence combination
 *
 *	@parm int     | iModule | module number of the sample
 *	@parm	int     | iNoun   | noun value of the sample
 *	@parm	int     | iVerb   | verb value of the sample
 *	@parm	int	  | iCond   | condition value of the sample
 *	@parm	int     | iSequ   | sequence value of the sample
 *
 *	@rvalue True  | sample was resumed
 *	@rvalue False | no samples were resumed
 ******************************************************************************/
Boolean KernelInterface::AudioResume(int iModule, int iNoun, int iVerb, int iCond,
												 int iSequ)   
{
   LIBTRACE("KernelInterface::AudioResume - Module, Noun, Verb, Cond, Sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);

	return( audioMgr->ImpAudioResume(iModule, iNoun, iVerb, iCond, iSequ) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Updates iDuration with the duration in 60ths of a second since
 *	the start of the sample identified by module, nooun, verb,
 *	condition and sequence combination
 *
 *	@parm int     | iModule   | module number of the sample
 *	@parm	int     | iNoun     | noun value of the sample
 *	@parm	int     | iVerb     | verb value of the sample
 *	@parm	int	  | iCond	  | condition value of the sample
 *	@parm	int     | iSequ     | sequence value of the sample
 *	@parm int*    | iDuration | pointer to int to updates with duration value
 *
 *	@rvalue True  | duration successfully determined
 *	@rvalue False | unable to determine duration
 ******************************************************************************/
Boolean KernelInterface::AudioDuration(int iModule, int iNoun, int iVerb, int iCond,
													int iSequ, int *iDuration)   
{
   LIBTRACE("KernelInterface::AudioDuration - Module, Noun, Verb, Cond, Sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iDuration != 0);
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);

	return( audioMgr->ImpAudioDuration(iModule, iNoun, iVerb, iCond, iSequ, iDuration) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Distorts the sample identified by the module, noun, verb,
 *	condition and sequence combination using the specified mask
 *	                                                                            
 *	@parm int  | iModule         | module number of the sample
 *	@parm	int  | iNoun           | noun value of the sample
 *	@parm	int  | iVerb           | verb value of the sample
 *	@parm	int  | iCond	        | condition value of the sample
 *	@parm	int  | iSequ           | sequence value of the sample
 *	@parm int  | iDistortionMask | mask used for distortion
 *		@flag 0       | distortion off
 *		@flag 1 - 255 | minimum to maximum distortion
 ******************************************************************************/
void KernelInterface::AudioDistort(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
											  int iDistortionMask)   
{
   LIBTRACE("KernelInterface::AudioDistort - Module, Noun, Verb, Cond, Sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iDistortionMask >= 0 && iDistortionMask <= 255);
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);

	audioMgr->ImpAudioDistort(iModule, iNoun, iVerb, iCond, iSequ, iDistortionMask);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Retrieves the volume of the sample identified by the module, noun, verb,
 *	condition and sequence combination to the specified value.
 *	                                                                            
 *	@parm int	  | iModule   | module number of the sample
 *	@parm	int	  | iNoun     | noun value of the sample
 *	@parm	int	  | iVerb     | verb value of the sample
 *	@parm	int	  | iCond	  | condition value of the sample
 *	@parm	int	  | iSequ     | sequence value of the sample
 *	@parm int*	  | iVolume   | pointer to int to updates with volume level   
 *
 *	@rvalue True  | volume of sample determined
 *	@rvalue False | unable to determine volume sample
 ******************************************************************************/
Boolean KernelInterface::AudioSampleVolume(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
														 int* iVolume)   
{
	LIBTRACE("KernelInterface::AudioSampleVolume - Module, Noun, Verb, Cond, Sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iVolume != 0);
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);

	return( audioMgr->ImpAudioSampleVolume(iModule, iNoun, iVerb, iCond, iSequ, iVolume) );
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Sets the volume of the sample identified by the module, noun, verb,
 *	condition and sequence combination to the specified value.
 *	                                                                            
 *	@parm int	  | iModule   | module number of the sample
 *	@parm	int	  | iNoun     | noun value of the sample
 *	@parm	int	  | iVerb     | verb value of the sample
 *	@parm	int	  | iCond	  | condition value of the sample
 *	@parm	int	  | iSequ     | sequence value of the sample
 *	@parm int	  | iVolume   | volume level
 *		@flag 0       | muted 
 *		@flag 1-127   | increasing loudness
 *		@flag 128     | priority sample, mute all other samples
 *					       until this one is done playing at full volume
 ******************************************************************************/
void KernelInterface::AudioVolume(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
											 int iVolume)   
{
	LIBTRACE("KernelInterface::AudioVolume - Module, Noun, Verb, Cond, Sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iVolume >= 0 && iVolume <= 128);
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);

	audioMgr->ImpAudioVolume(iModule, iNoun, iVerb, iCond, iSequ, iVolume);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Fades the sample identified by the module, noun, verb,
 *	condition and sequence combination to the target volume
 * using the specified characteristics
 *	                                                                            
 *	@parm int	  | iModule       | module number of the sample
 *	@parm	int	  | iNoun         | noun value of the sample
 *	@parm	int	  | iVerb         | verb value of the sample
 *	@parm	int	  | iCond	      | condition value of the sample
 *	@parm	int	  | iSequ         | sequence value of the sample
 *	@parm int	  | iTargetVolume | volume level to fade to (defaults to 0)
 * @parm int	  | iTicks			| number of 1/60 seconds between each
 *											  incremental volume change (defaults to 30)
 * @parm int	  | iStep			| the number of incremental volume changes
 *											  to make to get to the target volume , if
 *											  0 , target volume will be reached at once
 *											  (defaults to 10)	
 *	@parm Boolean | fEnd				| whether to end the sample when the
 *											  target volume is reached or
 *											  to continue playing the sample at
 *											  the target volume (defaults to False)
 *
 * @rvalue True  | fade successfully initiated
 * @rvalue False | unable to initiate fade
 ******************************************************************************/
Boolean KernelInterface::AudioFade36(int iModule, int iNoun, int iVerb, int iCond, int iSequ,
												 int iTargetVolume, int iTicks, int iSteps, Boolean fEnd)
{
	LIBTRACE("KernelInterface::AudioFade36");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);
	fatal_test(iTargetVolume >= 0 && iTargetVolume <= MAXVOLUME);
	fatal_test(iTicks >= 0);
	fatal_test(iSteps > 0); 
	fatal_test(fEnd == True || fEnd == False);
			
	return( audioMgr->ImpAudioFade36(iModule, iNoun, iVerb, iCond, iSequ, iTargetVolume, iTicks, iSteps, fEnd) );
}		

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Pans the sample identified by the module, noun, verb,
 *	condition and sequence combination using the specified
 *	pan percentage
 *	                                                                            
 *	@parm int  | iModule       | module number of the sample
 *	@parm	int  | iNoun         | noun value of the sample
 *	@parm	int  | iVerb         | verb value of the sample
 *	@parm	int  | iCond	      | condition value of the sample
 *	@parm	int  | iSequ         | sequence value of the sample
 *	@parm int  | iPanPercent   | percentage to pan sample on right speaker,
 *				  					     left speaker will get remainder of this
 *										  value subtracted from 100
 ******************************************************************************/
void KernelInterface::AudioPan(int iModule, int iNoun, int iVerb, int iCond, int iSequ, 
										 int iPanPercent)   
{
   LIBTRACE("KernelInterface::AudioPan - Module, Noun, Verb, Cond, Sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);
	fatal_test(iPanPercent >= 0 && iPanPercent <= 100);

	audioMgr->ImpAudioPan(iModule, iNoun, iVerb, iCond, iSequ, iPanPercent);
}

/*******************************************************************************
 * @mfunc 
 *                                                                              
 * Turns off panning for the sample identified by the module, noun,
 *	verb, condition and sequence combination
 *	                                                                            
 *	@parm int  | iModule	| module number of the sample
 *	@parm	int  | iNoun	| noun value of the sample
 *	@parm	int  | iVerb	| verb value of the sample
 *	@parm	int  | iCond	| condition value of the sample
 *	@parm	int  | iSequ	| sequence value of the sample  
 ******************************************************************************/
void KernelInterface::AudioPanOff(int iModule, int iNoun, int iVerb, int iCond,
											 int iSequ)
{
   LIBTRACE("KernelInterface::AudioPan - Module, Noun, Verb, Cond, Sequ");

	fatal_test(KernelInterface::AudioInstalled());
	fatal_test(iModule >= 0);
	fatal_test(iNoun >= 0 && iNoun <= 255);
	fatal_test(iVerb >= 0 && iVerb <= 255);
	fatal_test(iCond >= 0 && iCond <= 255);
	fatal_test(iSequ >= 0 && iSequ <= 255);

	audioMgr->ImpAudioPanOff(iModule, iNoun, iVerb, iCond, iSequ);
}

#endif // #ifndef AUDIO_LIB














 	


