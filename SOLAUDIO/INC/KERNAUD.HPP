#if !defined KERNAUD_HPP
#define KERNAUD_HPP

//------------------------------------------------------------------------------------------
//	@doc AUDLIB KERNEL
//
// KERNAUD.HPP -- Function Prototypes for stand-alone audio support functions
//
//	Date:			 03/18/96
// Created by:  Jay Lee
//
// These functions are required for use in the stand-alone audio library.  They implement
// functionality unique to this implementation of audio.  They should ONLY be used from
// the stand-alone library.
//------------------------------------------------------------------------------------------

#include "kerncall.hpp"

Boolean KernelAudioInitialize();
void KernelAudioTerminate();
void KernelAudioService();

#endif // #if !defined KERNAUD_HPP
