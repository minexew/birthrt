/* ==================================================================== */
/*                                                                      */
/* Sounds.c                                                             */
/*                                                                      */
/* ==================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <ctype.h>

#include "syslevel.h"
#include "system.h"
#include "midlevel.h"
#include "utils.h"
#include "scrgame.h"


#define SND_COUNT 15


HSAMPLE	sndSound[20];
VM_HDL	hdlSnd[SND_COUNT];

//JC	for unattached sample playing.
#define TEMP_SOUND_MAX 1
HSAMPLE	TempSound[TEMP_SOUND_MAX];
int		TempSoundCount = 0;
//JC


char	*sndNames[SND_COUNT] = 
	{
		"sounds\\1crwdL12.voc",
		"sounds\\1drib11.8",
		"sounds\\1sqeak11.8",
		"sounds\\1swish11.8",
		"sounds\\2drib11.8",
		"sounds\\2sqeak11.8",
		"sounds\\2swish11.8",
		"sounds\\3drib11.8",
		"sounds\\3sqeak11.8",
		"sounds\\rimoff11.8",
		"sounds\\whistl11.8",
		"sounds\\1charg16.8",
		"sounds\\1charg22.8",
		"sounds\\3defnc16.8",
		"sounds\\3defnc22.8",
	};

int		sndLens[SND_COUNT] = 
	{
		53307,
		 2613,
		 1868,
		 2348,
		 4244,
		 2908,
		 2681,
		 3883,
		 2709,
		 6813,
		14523,
		29981,
		41751,
		18991,
		26860,
	};

LONG	sndRates[SND_COUNT] = 
	{
		11000L,
		16000L,
		11000L,
		11000L,
		16000L,
		11000L,
		11000L,
		16000L,
		11000L,
		11000L,
		22000L,
		16000L,
		22000L,
		16000L,
		22000L,
	};


void CloseSounds (void)
{
	int	i;

	for( i=0; i < SND_COUNT; i++ )
	{
		SoundEnd( sndSound[i] );
		VMUnLock( hdlSnd[i] );
	}
}


void InitSounds (void)
{
	void	*ptr;
	int		i;

	//  Load the main array.
	for( i=0; i < SND_COUNT; i++ )
	{
		hdlSnd[i] = ResourceLoad( sndNames[i], FALSE, NULL );
		ptr = VMLockMove( hdlSnd[i] );
		sndSound[i] = SoundGetHandle();
		SoundSetHandle( sndSound[i], ptr, sndLens[i], sndRates[i] );
	}

	//  Tweak sounds to fit typical usage.
	SoundSetLoopCount( sndSound[0], 0L );	//	Crowd noise loops.
	SoundSetVolume( sndSound[0], 16L );		//	Crowd noise is quiet.
	SoundSetVolume( sndSound[1], 40L );		//	Dribbles are quiet.
	SoundSetVolume( sndSound[4], 40L );		//	Dribbles are quiet.
	SoundSetVolume( sndSound[7], 40L );		//	Dribbles are quiet.

}


void InitTempSounds( void )
{
	int	i;

	for( i = 0; i < TEMP_SOUND_MAX; i++ )
	{
		TempSound[i] = SoundGetHandle();

		if( NULL == TempSound[i] )
		{
			break;
		}
	}

	TempSoundCount = i;
}


HSAMPLE SetTempSound(
	void	*pData,	//	Sound data's location in memory.
	USHORT	usLen,	//	Length of sound.
	LONG	lRate	//	Playback rate (samps/sec).
)
{
	HSAMPLE	hS = NULL;
	int		i;

	if( _snd_Installed )
	{
		//	Find a now free temp sound slot.
		for( i = 0; i < TempSoundCount; i++ )
		{
			if( SMP_DONE == SoundGetStatus( TempSound[i] ) )
			{
				break;
			}
		}

		if( i == TempSoundCount )
		{
			mprintf( "No temp sounds available now.\n" );
			return 0;
		}

		hS = TempSound[i];

		AIL_init_sample( hS );
		AIL_set_sample_type( hS, DIG_F_MONO_8, 0 );
		AIL_set_sample_address( hS, pData, usLen );
		AIL_set_sample_playback_rate( hS, lRate );
	}

	return hS;
}


/*JC
 *	If a sound is in the arrays above, PlayTempSound can be called with 
 *	only its index number.
 *		Advantage:  The sample can be playing multiply simultaneously.
 *		Disadvantages:  The sample won't play if all temp samples are going.
 *			
 */
void PlayTempSound(
	int	i
)
{
	void	*ptrSound;
	HSAMPLE	hS;

	ptrSound = VMLoad( hdlSnd[i] );
	hS = SetTempSound( ptrSound, sndLens[i], sndRates[i] );
	if( NULL != hS )
		SoundStart( hS );
}
