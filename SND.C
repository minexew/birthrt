
/*#######################################################################\
#
#	Synergistic Software
#	(c) Copyright 1993-1995  All Rights Reserved.
#	Author:  Steve Coleman		sound stuff
#			 Greg Hightower		timer stuff
#
\#######################################################################*/

#define MIDI 0		//no Midi in Bball

//	microscopic change
//-------------------------	snd.c	-------------------

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <ctype.h>

#include "syslevel.h"
#include "system.h"
#include "snd.h"	

#define MERWARS 0
extern SHORT sndVolumeEffects;

#define VOICECH 1	//if VOICECH is true, channel 0 is dedicated to voice.
					//All sounds scanned to channel 0 (only) will be
					//placed in a fifo que, and played one after another.
					//All other sounds will be directed to the first available
					//channel (excluding ch 0) and played immediately. If no
					//channel is available, the sound is tossed.
					//Note that doqueue() must be called once every loop
					//to process the que.


//----------	globals	-------------

LONG		_snd_DriverType = 0;
DIG_DRIVER	*_snd_pDriver = 	0;
BOOLEAN		_snd_Installed = TRUE;
   
MDI_DRIVER	*mdriver = 0;

HTIMER	sys_ctr1hdl;

HSAMPLE		channel[CHANNELS];		//Ail handles for each channel
HSEQUENCE	sequence[SEQUENCES];	//Ail handles for each seq

CHAR		*baskball;

long fadech = -1;						//for fades
long fadevol,fadedest;
long fadedelta,fadestick,fadeetick,fadeticks;

long loadnum = 0;		//if true, load numbers as needed


UBYTE master = 255;
//to convert volume to actual: 			actual =  (vol*master)/256
//to get unscaled volume from actual:  vol = (actual*256)/master
//note that the compiler always interprets *256 as <<8

/* system tick counters */
volatile TCNT _sys_aTimerCounters[_SYS_TIMERS] = { 0L };

/* system stop watch timers controls */
volatile SHORT _sys_StopWatch[_SYS_WATCHES] = { 0 };

/* timer handle for system tick counters update */
HTIMER _sys_thdlCounters = -1;

HTIMER	_mouse_hdlTimer = _TIMER_NULLHDL;		// for use with AIL library


long	tot=0;			//used in loadh
long	mtot=0;
char	name[30];
VM_HDL	hdlFile;

//---------------------	prororypes	----------------------

static void loadh(long i);
static void load(long i);

//----------------	Channel assignments	---------

#define VOICE	1		//only voice in ch 0
#define CRWD	2		// only crowd in ch1
#define POOL	0xfc	// everything else Pooled in ch 2-7


//------ list of all sounds to be used in the game -------

#define ETC 8, NULL, _VM_NULLHDL

SOUND snds[] =
{	//                       rate, ch scan,loop,vol,	type
{	"sounds\\1crwdL12.8",	12000,	CRWD,	0,	 16,	8},	//	CROWD
{	"sounds\\1cheer6a.8",	 6000,	POOL,	1,	 35,	8},	//	CHEER
{	"sounds\\1drib11.8"	,	 8000,	POOL,	1,	 35,	8},	//	DRIB1
{	"sounds\\1swish11.8",	 8000,	POOL,	1,	 80,	8},	//	SWISH1
{	"sounds\\3sqeak11.8",	11000,	POOL,	1,	 40,	8},	//	SQUEAK1
{	"sounds\\rimoff11.8",	 8000,	POOL,	1,	100,	8},	//	RIMOFF
{	"sounds\\whistl11.8",	20000,	POOL,	1,	100,	8},	//	WHISTLE 
{	"sounds\\tm01n1.8"	,	11000,	VOICE,	1,	120,	8},	//	BLAZERS
{	"sounds\\t1pl1b.8"	,	11000,	VOICE,	1,	100,	8},	//	DREXLER
{	"sounds\\t1pl2b.8"	,	11000,	VOICE,	1,	100,	8},	//	ROBINSON
{	"sounds\\t1pl3b.8"	,	11000,	VOICE,	1,	100,	8},	//	STRICKLAND
{	"sounds\\tm02n1.8"	,	11400,	VOICE,	1,	120,	8},	//	SONICS
{	"sounds\\t2pl1b.8"	,	11000,	VOICE,	1,	100,	8},	//	KEMP
{	"sounds\\t2pl2b.8"	,	11000,	VOICE,	1,	100,	8},	//	PAYTON
{	"sounds\\t2pl3b.8"	,	11400,	VOICE,	1,	100,	8},	//	SCHREMPF
{	"sounds\\ncue13.8"	,	11000,	VOICE,	1,	100,	8},	//	SHOOTS
{	"sounds\\ncue14.8"	,	11000,	VOICE,	1,	100,	8},	//	SCORES
{	"sounds\\ncue50.8"	,	11000,	VOICE,	1,	100,	8},	//	MISSEDit
{	"sounds\\ncue30.8"	,	11000,	VOICE,	1,	127,	8},	//	ITSGOOD
{	"sounds\\ncue43.8"	,	11000,	VOICE,	1,	127,	8},	//	ITSIN
{	"sounds\\ncue68.8"	,	11000,	VOICE,	1,	100,	8},	//	NOWAY
{	"sounds\\ncue06.8"	,	11000,	VOICE,	1,	100,	8},	//	NOGOOD
{	"sounds\\ncue09.8"	,	11000,	VOICE,	1,	100,	8},	//	BLOCKED shot
{	"sounds\\ncue23.8"	,	11000,	VOICE,	1,	100,	8},	//	STEALSit
{	"sounds\\ncue24.8"	,	11000,	VOICE,	1,	100,	8},	//	NOBASKet
{	"sounds\\ncue40.8"	,	11000,	VOICE,	1,	127,	8},	//	NOTHINbut net
{	"sounds\\ncue48.8"	,	11000,	VOICE,	1,	127,	8},	//	ALLTHEway to the basket
{	"sounds\\ncue03.8"	,	11000,	VOICE,	1,	127,	8},	//	_2POINT s
{	"sounds\\ncue04.8"	,	11000,	VOICE,	1,	127,	8},	//	_3POINT s
{	"sounds\\ncue05.8"	,	11000,	VOICE,	1,	127,	8},	//	ITSA3  pointer
{	"sounds\\ncue18.8"	,	11000,	VOICE,	1,	127,	8},	//	GRABS the rebound
{	"sounds\\ncue35.8"	,	11000,	VOICE,	1,	100,	8},	//	TIMESRUNning out
{	"sounds\\ncue42.8"	,	11000,	VOICE,	1,	100,	8},	//	GOES for the 3
{	"sounds\\ncue47.8"	,	11000,	VOICE,	1,	100,	8},	//	_24SEC violaion
{	"sounds\\ncue49.8"	,	11000,	VOICE,	1,	127,	8},	//	TIED ,and the game is
{	"sounds\\ncue56.8"	,	11000,	VOICE,	1,	100,	8},	//	THROWS it away
{	"sounds\\shotat5.8"	,	11000,	VOICE,	1,	100,	8},	//	SHOTAT5
{	"sounds\\bsktrk1l.8",	11200,	POOL,	0,	 50,	8},	//	tune1
{	"sounds\\bsktrk2l.8",	11200,	POOL,	0,	 50,	8},	//	tune2
{	"sounds\\ncue07.8"	,	11000,	VOICE,	1,	127,	8},	//	SLAMS one home
{	"sounds\\ncue37.8"	,	11000,	VOICE,	1,	100,	8},	//	GOESHOOP, to the 
{	"sounds\\ncue62.8"	,	11000,	VOICE,	1,	100,	8},	//	GETSOpen
{	"sounds\\ncue15.8"	,	11000,	VOICE,	1,	100,	8},	//	WIDEOpen
{	"sounds\\bsktrk1i.8",	11200,	POOL,	1,	 50,	8},	//	tune1 intro1
{	"sounds\\bsktrk2i.8",	11200,	POOL,	1,	 50,	8},	//	tune2 intro2
{	"sounds\\ncue27.8"	,	11000,	VOICE,	1,	100,	8},	//	TIMEOUT 
{	"sounds\\ohmy.8"	,	11000,	VOICE,	1,	127,	8},	//	OHYEAH(ohhohomy)
{	"sounds\\1ahorn11.8",	11000,	POOL,	1,	100,	8},	//	AHORN
{	"sounds\\t1pl1f.8"	,	11000,	VOICE,	1,	100,	8},	//	DREXU
{	"sounds\\t1pl2f.8"	,	11000,	VOICE,	1,	100,	8},	//	ROBIU
{	"sounds\\t1pl3f.8"	,	11000,	VOICE,	1,	100,	8},	//	STRIU
{	"sounds\\t2pl1f.8"	,	11000,	VOICE,	1,	100,	8},	//	KEMPU
{	"sounds\\t2pl2f.8"	,	11000,	VOICE,	1,	100,	8},	//	PAYTU
{	"sounds\\t2pl3f.8"	,	11400,	VOICE,	1,	100,	8},	//	SCHRU
{	"sounds\\he4n.8"	,	11400,	VOICE,	1,	 70,	8},	//	HE
{	"sounds\\ncu021.8"	,	11000,	VOICE,	1,	127,	8},	//	TOMAHAWK
{	"sounds\\ncu022.8"	,	11000,	VOICE,	1,	127,	8},	//	MONSTER
{	"sounds\\ncu023.8"	,	11000,	VOICE,	1,	100,	8},	//	OFFREB
{	"sounds\\ncu024.8"	,	11000,	VOICE,	1,	100,	8},	//	PICKS
{	"sounds\\ripped.8"	,	11000,	VOICE,	1,	100,	8},	//	RIPPED (was pulls
{	"sounds\\ncu026.8"	,	11000,	VOICE,	1,	100,	8},	//	TOSSES in
{	"sounds\\throwsin.8",	11000,	VOICE,	1,	100,	8},	//	THROWSIN, it
{	"sounds\\dishes.8"	,	11000,	VOICE,	1,	100,	8},	//	DISHES off
{	"sounds\\passoff.8"	,	11000,	VOICE,	1,	100,	8},	//	PASSES
{	"sounds\\hefires.8"	,	11000,	VOICE,	1,	 80,	8},	//	FIRESTO he
{	"sounds\\goesto.8"	,	11000,	VOICE,	1,	100,	8},	//	GOESTO
{	"sounds\\wthball.8"	,	11000,	VOICE,	1,	 80,	8},	//	WITHBALL
{	"sounds\\herecome.8",	11000,	VOICE,	1,	100,	8},	//	HERECOME
{	"sounds\\takeup.8"	,	11000,	VOICE,	1,	100,	8},	//	TAKEUP
{	"sounds\\ncu040.8"	,	11000,	VOICE,	1,	100,	8},	//	WRAPS
{	"sounds\\4defnc11.8",	11000,	POOL,	1,	100,	8},	//	DEFEN
{	"sounds\\1perc11.8"	,	11025,	POOL,	1,	100,	8},	//	LODRUM
{	"sounds\\1perc11.8"	,	13025,	POOL,	1,	100,	8},	//	HIDRUM
{	"sounds\\organ3s2.8",	13025,	POOL,	1,	 30,	8},	//	ORGAN
{	"sounds\\1charg16.8",	13025,	POOL,	1,	100,	8},	//	CHARGE
{	"sounds\\ncue46.8"	,	11000,	VOICE,	1,	 80,	8},	//	SETS it up
{	"sounds\\ftstmp3.8"	,	20000,	POOL,	1,	 70,	8},	//	STOMP
{	"sounds\\hndclp1c.8",	22000,	POOL,	1,	 70,	8},	//	CLAP
{	"sounds\\brass2s2.8",	40000,	POOL,	0,	 30,	8},	//	BRASS

{	"sounds\\pregame1.8",	11000,	VOICE,	0,	120,	8},	//	Hi,this is Dan
{	"sounds\\pregame2.8",	11000,	VOICE,	0,	120,	8},	//	Good evening
{	"sounds\\tonight.8"	,	11000,	VOICE,	0,	120,	8},	//	And, tonight we
{	"sounds\\intro2.8"	,	11000,	VOICE,	0,	120,	8},	//	And for tonight we
{	"sounds\\intro5.8"	,	11000,	VOICE,	0,	120,	8},	//	This is NBA on ESPN
{	"sounds\\vsthe1.8"	,	11000,	VOICE,	0,	120,	8},	//	versus the
{	"sounds\\andthe2.8"	,	11000,	VOICE,	0,	120,	8},	//	and the
{	"sounds\\togo.8"	,	11000,	VOICE,	0,	120,	8},	//	TOGO (tobo)

{	"sounds\\minute1.8"	,	11000,	VOICE,	0,	100,	8},	//
{	"sounds\\minutes1.8",	11000,	VOICE,	0,	100,	8},	//
{	"sounds\\second1.8"	,	11000,	VOICE,	0,	100,	8},	//
{	"sounds\\seconds1.8",	11000,	VOICE,	0,	100,	8},	//

#include "numbers.c"
#include "players.c"
#include "teams.c"

{	"sounds\\lsball1.8",	11000,	VOICE,	0,	100,	ETC},//

//	Misc:		91 samples run from     0 - 90
//	numbers.c:	32 samples run from    91 - 122
//	players.c: 162 samples run from   123 - 284
//	teams.c:	54 samples run from	  285 - 338
//	LOOSE:		 1 sample			@ 339

};
USHORT	sndCount = sizeof(snds) / sizeof(SOUND);

#if MIDI
//note: volume of 0 gets default of 100
//------ list of all midi files to be used in the game -------
XMIDI xmidi[] =
	{ //   name            loop vol                                         
	{ "midi\\bsk04.xmi",    1,  100 },			//DRUMS
	{ "midi\\bsk06.xmi",    1,  100 },			//CHARGE
	{ "midi\\bsk05.xmi",    1,  100 },			//
	{ "midi\\bsktnw3b.xmi", 1,  100 } 			//
};
USHORT	midiCount = sizeof(xmidi) / sizeof(XMIDI);
#endif

//====================================================================

//								Module definitions

//====================================================================


#define midiMARK 0x8000		//mark the end of snd effects & start of midi songs
#define endMARK  0xffff
   
//				module 0 is the actual Merwars module

USHORT gamemod[] = {		//-----List of all sounds & midi files in	module 0
	CROWD,
	CHEER,
	DRIB1,
	SWISH1,
	SQUEAK1,
	RIMOFF,
	WHISTLE,
	AHORN,
	ORGAN,
	CHARGE,
   LODRUM, STOMP,CLAP, DEFEN,
	BRASS,
endMARK       //-----mark end of module

};

USHORT buttonmod[]= {		
	DRIB1,
	SWISH1,
	WHISTLE,
	endMARK       //-----mark end of module
};

USHORT tune1mod[] = {		//-----List of all sounds & midi files in	module 2
	TUNE1,INTRO1,
//midiMARK,		//----mark end of snds		!!!!don't need this if no midi
endMARK       //-----mark end of module

};

USHORT tune2mod[] = {		//-----List of all sounds & midi files in	module 0
	TUNE2, INTRO2,
	endMARK       //-----mark end of module

};


USHORT announcermod[] = {		//-----Announcer
	SHOOTS,
	SCORES,
	MISSED,
	ITSGOOD,
	ITSIN,
	NOWAY,
	NOGOOD,
	//BLOCKED,
	STEALS,
	NOBASK,
	//NOTHIN,
	ALLTHE,		//?used?
	_2POINT,
	_3POINT,
	ITSA3,
	GRABS,
	TIMESRUN,
	SHOTAT5,
	_24SEC,
	TIED,
	THROWS,
	SLAMS,
   GOESHOOP,	
   GETSO, WIDEO, TIMEOUT, OHYEAH,
   FIRESTO, GOESTO, MONSTER, TOMAHAWK, HERECOME, TAKEUP,
	WITHBALL, PASSES, DISHES, TOSSES, THROWSIN, HE, RIPPED, WRAPS, SETS,
endMARK       //-----mark end of module
};

USHORT playersmod[] = {		//-----6 players
	PAYTON,PAYTU,
	KEMP,KEMPU,
	SCHREMPF,SCHRU,
	DREXLER,DREXU,
	ROBINSON,ROBIU,
	STRICKLAND,STRIU,

endMARK       //-----mark end of module
};


USHORT pregamemod[] = {					//-----Pre game stuff
   PREGAME1,PREGAME2,
   AINTRO1,AINTRO2,AINTRO5,
   VSTHE1,ANDTHE1,

endMARK       //-----mark end of module
};

USHORT teammod[] = {					//-----team names
	SONICS,BLAZERS,
	TOGO,
endMARK       //-----mark end of module
};

USHORT numbermod[] = {					//-----numbers
	OH,NOTHING,SONE,ONE,TWO,THREE,FOUR,FIVE,SIX,SEVEN,EIGHT,NINE,TEN,
   TEEN1,TEEN2,TEEN3,TEEN4,TEEN5,TEEN6,TEEN7,TEEN8,TEEN9,
   TY2,TY3,TY4,TY5,TY6,TY7,TY8,TY9,TY10,HUND,
   MINUTE,MINUTES,SECOND,SECONDS,
endMARK       //-----mark end of module
};


USHORT* modules[] = { gamemod, buttonmod, tune1mod, tune2mod,announcermod,
							 playersmod,pregamemod,teammod,numbermod};	//------module list-----

SHORT loaded[8] = {-1,-1,-1,-1,-1,-1,-1,-1};	//list of modules loaded into memory


//------------	for internal use only	--------------
QENTRY que[QSIZE];				//fifo sound queue
LONG qput,qplay = 0;			//pointers (indexes) into que
LONG getchannel(LONG what);		//prototype
LONG getsequence();				//prototype


#if MERWARS
LONG bubblevol,bubblech;
#endif



long delcheer;
ULONG t60;


//===============================================================================
//===============================================================================
/*
	say() searches all of the channels set in the scan bits for the sound and
   when it finds an available channel, the sound is immediately played there.
   Note that continuously looped sounds (.loop = 0) will play until they are
   stopped with stopsnd();

   If VOICECH is being used, and the sound is scanned for channel 0 (.scan = 1),
   the sound is pushed into the que, and doqueue() will play it later.

   say() returns the channel assigned to the sound just played. You can later
   alter the playing of tha channel with: stopsnd(), ratesnd(), and volumesnd().

	Parameter vol: If 1-127, volume is set to that value. If 0, sound is played
   at vol set in the snds array, or if that value is 0, that sound will play
   at the default volume of 100.

   Parameter pan: Valid range is 0-127 (hard left to hard right). Center is
   at 64.

*/


//---------------------------------------------------------------------

//			say()

//---------------------------------------------------------------------


LONG SndSay (LONG what)						//---------say effect using all defaults
{
	LONG ch;   
	ch = SndSayVPR(what, 0, 64, 0);
	return(ch);
}

LONG SndSayV (LONG what, UBYTE vol)					//------- say effect at vol
{
	LONG ch;   
//	snds[what].volume * vol / 100;			// fine  adjust requested volume
	ch = SndSayVPR(what, (snds[what].volume * vol) / 100, 64, 0);
	return(ch);
}

LONG SndSayVP (LONG what, UBYTE vol, UBYTE pan)	//------ say effect at vol, pan
{
	LONG ch;   
	ch = SndSayVPR(what, vol, pan, 0);
	return(ch);
}

LONG SndSayR (LONG what, LONG rate)    			//------ say effect at rate
{
	LONG ch;   
	ch = SndSayVPR(what, 0, 64, rate);
	return(ch);
}

LONG SndSayVR (LONG what, UBYTE vol, LONG rate) 	//------ say effect at volume and rate
{
	LONG ch;   
	ch = SndSayVPR(what, (snds[what].volume * vol) / 100, 64, rate);
//	ch = SndSayVPR(what, vol, 64, rate);
	return(ch);
}

// say effect at vol, pan, rate
LONG SndSayVPR (LONG what, UBYTE vol, UBYTE pan, LONG rate)	
{
	LONG tqput;
	HSAMPLE handle;
	void *ptr;
	LONG ch;

//mprintf("sayvpring: what:%d  \n",what);   

   if(_snd_Installed == FALSE)
		return -1L;
		
   if(what <0x8000)								//if not a rest
  		if(what >= sndCount) return(-1);   //Not a valid sound
	if(pan>127) pan = 64;					//if pan out of range, set center
	if(vol>127) vol = 0;	    				//if vol out of range, use default

#if VOICECH
   if(snds[what].scan == 1 || what >= 0x8000)
      									//if channel 0, is voice. Put snd in queue
                              	// rest is always ch 0
	{
		tqput = qput;					//1st, inc put & see if it wraps
		tqput++;
		if(tqput >= QSIZE) tqput = 0;
		
		if(tqput == qplay)				// if q full,toss this entry,
		return(-1);
		
   	if(what < 0x8000)             //if !rest
      {
//mprintf("Qing: what:%d  \n",what);   
         que[qput].what = what;		//Que this sound
   		que[qput].vol = vol;
   		que[qput].pan = pan;
      }
		else									//is a rest
      {
//mprintf("Qing: rest:%x  \n",what&0xfff);   
         que[qput].what = -1;		//Q rest
         que[qput].t60 =  what&0xfff;	//count to rest
      }
		   
		qput = tqput;	//advance to next empty spot
		return(0);
	}
	else		//---------else ch 1-7 just play the sound
#endif
	{
		ch = getchannel(what);
		if(ch >= 0)
		{
			if(snds[what].size)	//if size !=0, play the sound
			{
//mprintf("saying: %s in ch:%d  at:%d\n",snds[what].name,ch,snds[what].rate);   
				handle = channel[ch];
				AIL_init_sample(handle);  //---------init the channel set default values
				ptr = VMLoad( snds[what].hdl );
				
				if(snds[what].type == 8)
					AIL_set_sample_type(handle, DIG_F_MONO_8, DIG_PCM_ORDER);
				else
					AIL_set_sample_type(handle, DIG_F_MONO_16, DIG_PCM_SIGN);
				
				AIL_set_sample_address(handle, ptr, snds[what].size);
				AIL_start_sample(handle);  //--------------------actually play the sample


				AIL_set_sample_loop_count(handle, snds[what].loop); //set loop count 
				
				AIL_set_sample_pan(handle, pan);   
            if(rate)
					AIL_set_sample_playback_rate(handle, rate);
				else
					AIL_set_sample_playback_rate(handle, snds[what].rate);
				if(vol)
					AIL_set_sample_volume(handle, (vol*master)/256);//if call has a vol, use it,
				else
				if(snds[what].volume)				  //else if snds has a volume,
					AIL_set_sample_volume(handle, (snds[what].volume*master)/256);  //use It.
      		else
               AIL_set_sample_volume(handle, (100*master)/256);  //default
			}			
			else
				ch = -1;     //if size = 0, sound does not exist (in memory)
			
			return(ch);
			
		}  //if ch
		else
			return(-1);		//no channel was available
	}
}


//====================	misc functions	===================

//	fade sound in [ch] from current volume to [vol] over a period
// of [ticks] 1/60ths of a second.

void SndFade(long ch, long vol, long ticks)
{
	if( _snd_Installed == FALSE )
		return;
		
	fadech = ch;				//--------	start a fade	--------
	if (vol<0) vol = 0;
	if (vol>127) vol = 127; //force valid values
	
	fadedest = vol;
   if(master)
  		fadevol = (AIL_sample_volume(channel[fadech])*256)/master;	//current volume
	else
      fadevol = 0;
   
	fadevol = AIL_sample_volume(channel[fadech]);	//current volume
	
	if(fadedest == fadevol)
	{              				//already at requested volume
		fadech = -1;
		return;
	}
	
	fadedelta = (fadedest-fadevol)*256;		//total volume change
	
	fadestick = SysCounter(1);			//start tick
	fadeetick = SysCounter(1)+ticks;	//end tick
	fadeticks = ticks;					//total time for change
}

void rest (long ticks)    					//silent rest for n 1/60ths sec
{
   SndSay(0x8000+ticks);
}





#if VOICECH
/*-------------------------------------------------------------------------
  doqueue() is only used with VOICECH. Once every loop, it checks the que
  to see if say() has queued a sound. If a sound is there, it is played,
  and removed from the que.
  
  Note that queued sounds are always in
  channel 0.

*/

void clearqueue(void)        //----toss everything currently in the Q
{
   qput = qplay = 0;
}


void doqueue()
{
	HSAMPLE handle;
	void *ptr;
	LONG what;
	LONG ch;
	LONG vol,pan;
  
  	if(loadnum) return;			//suspend unqueueing while loadsayingnumbers

	if( _snd_Installed == FALSE )
		return;
		
	if(qput != qplay)                //if !=, sometings in the queue
	{
		
		what = que[qplay].what;
		vol  = que[qplay].vol;
		pan  = que[qplay].pan;
		      
		if(what < 0)
      {                       		//------------------rest--------
      	if(what == -1)									//set rest wait count
         {
//mprintf("Qrest  what:%d\n",what&0xfff);
         	que[qplay].t60 += SysCounter(1); 	//count at end of rest
      	   que[qplay].what = -2;						//nxt time,rest
            
         }
         else
         {
//mprintf("waiting till %d what:%d\n",SysCounter(1)+what&0xfff,what);
         	if(SysCounter(1) >= que[qplay].t60)	//if end of rest count was reached
				{
         		qplay++;								//toss rest entry from the que
      			if(qplay >= QSIZE) qplay = 0;
         	}      
      	}
      }
		else  								//play snd from Q    
      {
			if(AIL_sample_status(channel[0]) == SMP_DONE) //If channel is ready
			{
			
				ch = 0;		//voice channel is Always 0
			
				if(snds[what].size)	//if size = 0, sound does not exist
				{
				         
//mprintf("Qplaying: what:%d \n",what);   
					handle = channel[ch];
				
					AIL_init_sample(handle);  //---------init the channel set default values
				
					ptr = VMLoad( snds[what].hdl );
				   
					AIL_set_sample_address(handle, ptr, snds[what].size);
					AIL_set_sample_playback_rate(handle, snds[what].rate);
					if(vol)
						AIL_set_sample_volume(handle, vol);//if que has a vol, use it,
					else
					if(snds[what].volume)				  //else if snds has a volume,
						AIL_set_sample_volume(handle, snds[what].volume);  //use It.
					else
                  AIL_set_sample_volume(handle, (100*master)/256);  //default
				
					AIL_set_sample_loop_count(handle, 1);	//set loop count
				
					AIL_set_sample_pan(handle, pan);   
				
					AIL_start_sample(handle);  //--------------------actually play the sample
				}
			
			qplay++;								//toss this entry from the que
			if(qplay >= QSIZE) qplay = 0;
		
      	}
		}
   }


	if(delcheer)
   {
      if (SysCounter(1) >= delcheer)
      {
      	delcheer = 0;
			SndSayVPR(CHEER, 25, 64, 6000);	//quieter
      }
   }


   
}
#endif


//==================================================================

update_fade()
{
	long tvol,eticks;
	
	if( _snd_Installed == FALSE )
		return;
		
	//----------	PROCESS FADE ------------------
	
	if(SysCounter(1) >= fadeetick)	//-----------if at end of time, force dest volume
	{
		AIL_set_sample_volume(channel[fadech],fadedest);
		if(fadedest == 0)		//if was fade to 0, stop sample
			stopsnd(fadech);	
		fadech = -1;			//---------fade done
	}
	eticks = SysCounter(1)-fadestick;	//elapsed ticks
	tvol = fadevol+(((fadedelta/fadeticks)*eticks)/256);
	
	//printf("tvol:%d  \n",tvol);
	if(tvol<0)
		tvol = 0;
	AIL_set_sample_volume(channel[fadech],tvol);
}






//---------------	reset the volume of a channel on the fly	-----------

void SndVolume(LONG ch, LONG vol)
{
	if( _snd_Installed == FALSE )
		return;
		
	if(ch >= 0 && ch < CHANNELS &&                  	//if valid ch
		AIL_sample_status(channel[ch]) == SMP_PLAYING)  // & ch is active
	{
		AIL_set_sample_volume(channel[ch], vol);
	}
}

//---------------	reset the sample rate of a channel on the fly	-----------

void SndRate(LONG ch, LONG rate)
{
	if( _snd_Installed == FALSE )
		return;
		
	if(ch >= 0 && ch < CHANNELS &&                  	//if valid ch
		AIL_sample_status(channel[ch]) == SMP_PLAYING)  // & ch is active
	{
		AIL_set_sample_playback_rate(channel[ch], rate);
	}
}

//------------------------	stop a looped sample	-------------
void stopsnd(LONG ch)
{
	if( _snd_Installed == FALSE )
		return;
		
	if(ch >= 0 && ch < CHANNELS &&                  	//if valid ch
		AIL_sample_status(channel[ch]) == SMP_PLAYING)  // & ch is active
	{
		AIL_end_sample(channel[ch]);
	}
}
    
//-----------------------	get the status of a channel	-----------
// returns:
//	SMP_DONE		channel is open
// 	SMP_PLAYING		channel is busy


LONG SndStatus(LONG ch)
{
	if( _snd_Installed == FALSE )
		return SMP_DONE;
		
	if(ch >= 0 && ch < CHANNELS)                   	//if valid ch
		return((LONG)  AIL_sample_status(channel[ch])  );
	else
		return((LONG) SMP_PLAYING);  //if was invalid ch
}

//------------------------	quiet all channels	-------------
//							also, stop all midi
void SndQuiet()
{
	LONG ch,seq;

	for(ch=0;ch<CHANNELS; ch++)
		stopsnd(ch);
	#if MIDI
   for(seq=0;seq<SEQUENCES; seq++)
		SndEndSeq(seq);
	#endif

}


//-------------------	find an available channel	----------

LONG getchannel(LONG what)
{
	LONG ch;
	UBYTE bitmsk[8] = {1,2,4,8,0x10,0x20,0x40,0x80};
	
	if( _snd_Installed == FALSE )
		return 0L;
		
	#if VOICECH
	for (ch=1; ch<CHANNELS; ch++ )
	#else
	for (ch=0; ch<CHANNELS; ch++ )
	#endif
	{
		if(snds[what].scan & bitmsk[ch])		//if this channel included in our scan
		{
			if(AIL_sample_status(channel[ch]) == SMP_DONE) //& it's open,
			return(ch);								//Got a Channel!
		}
	}
	return(-1);  	//no channel avail
}


//---------------------------------------------------------------------
	#if MIDI
//			play()

//---------------------------------------------------------------------

LONG SndPlay (LONG what)	
{
	HSEQUENCE handle;
	void *ptr;
	LONG seq;
	
	if(what >= midiCount) return(-1);   //Not a valid seq
	
	seq = getsequence();
	if(seq >= 0)
	{
		if(xmidi[what].size)	//if size = 0, seq does not exist
		{
			mprintf("playing: %s \n",xmidi[what].name);   
			handle = sequence[seq];
			ptr = VMLoad( xmidi[what].hdl );
			AIL_init_sequence(handle,ptr,0);  
			
			if(xmidi[what].volume)				  //else if has a volume,
				AIL_set_sequence_volume(handle, xmidi[what].volume,1);  //use It.
			
			AIL_set_sequence_loop_count(handle, xmidi[what].loop); //set loop count 
			
			AIL_start_sequence(handle);  //------actually play the xmidi
		}
		return(seq);
	}  //if ch
	else
		return(-1);		//no seq was available
	
}

//---------------------	end a seq -------------

void SndEndSeq(LONG seq)
{
	if(seq >= 0 && seq < SEQUENCES &&                  	//if valid ch
		AIL_sequence_status(sequence[seq]) == SEQ_PLAYING)  // & ch is active
	{
		AIL_end_sequence(sequence[seq]);
	}
}

//---------------------	 -------------

LONG SndStatusSeq(LONG seq)
{
	LONG stat;
	if(seq >= 0 && seq < SEQUENCES)                  	//if valid ch
	{
		stat = AIL_sequence_status(sequence[seq]) ;
		return(stat);
	}
	else
		return(SEQ_FREE);
}


//-------------------	find an available sequence	----------

LONG getsequence()
{
	LONG seq;
	
	for (seq=0; seq<SEQUENCES; seq++ )
	{
		if(AIL_sequence_status(sequence[seq]) == SEQ_DONE) //if it's open,
			return(seq);								//Got a Channel!
	}
	return(-1);  	//no seq avail
}
#endif

//==========================================================================

//			startup stuff

//==========================================================================
   


//--------------	load driver and init channels	------
//---					also load midi driver

void SndStartAIL(LONG driver)
{
	LONG i;
	HWAVE hwave;
	IO_PARMS	*io_params;
	
	_snd_DriverType = driver;
	
	
	AIL_startup();
	
	AIL_set_preference( DIG_USE_STEREO, NO );
	AIL_set_preference( DIG_USE_16_BITS, NO );
	
	// AIL_set_preference( DIG_HARDWARE_SAMPLE_RATE, MIN_VAL );
	AIL_set_preference( DIG_HARDWARE_SAMPLE_RATE, NOM_VAL );
	// AIL_set_preference( DIG_HARDWARE_SAMPLE_RATE, MAX_VAL );
	
	// AIL_set_preference( DIG_LATENCY, 90 );
	AIL_set_preference( DIG_LATENCY, 60 );
	// AIL_set_preference( DIG_LATENCY, 30 );
		
	//Huh? 8000 is invalid!	AIL_set_preference( DIG_HARDWARE_SAMPLE_RATE, 8000 ); // MIN_VAL, MAX_VAL
	//default anyway	AIL_set_preference( DIG_HARDWARE_SAMPLE_RATE, NOM_VAL );
		
	// AIL_set_preference( AIL_ALLOW_VDM_EXECUTION, NO );
	// AIL_set_preference( DIG_SERVICE_RATE, 60 );
	
	if( driver != NO_SOUND )
	{
		// old stuff, we now load a single pre-installed driver
		// switch( _snd_DriverType )
		// {
		// case 1:		//	SoundBlaster generic
		// 	_snd_pDriver = AIL_install_DIG_driver_file( "sblaster.dig", NULL );
		// 	break;
		// case 2:		//	Gravis UltraSound
		// 	_snd_pDriver = AIL_install_DIG_driver_file( "umid.dig", NULL );
		// 	break;
		// default:
		// 	break;
		// }
		
		// check to see if sound driver exists
		if( ( access( "sound.dig", ( F_OK ) ) ) == -1 )
		{
			_snd_pDriver = NULL;
		}
		else
		{
			_snd_pDriver = AIL_install_DIG_driver_file( "sound.dig", NULL );
		}
		
		if(!_snd_pDriver )   //-------------- couldn't load driver
		{
			mprintf("Sound driver load not loaded: " );
			mprintf( AIL_error );
			// AIL_shutdown();
			_snd_Installed = FALSE;
			_snd_DriverType = 0;
		}
		else						//-----------------have a driver
		{
			io_params = AIL_get_IO_environment(_snd_pDriver->drvr);
			mprintf("SND INIT\n");
			mprintf("IO    = %x\n", io_params->IO );
			mprintf("IRQ   = %d\n", io_params->IRQ );
			mprintf("DMA 8 = %d\n", io_params->DMA_8_bit );
			mprintf("DMA 16= %d\n", io_params->DMA_16_bit );
			
			for( i=0; i < CHANNELS; i++ )		//-------get AIL handles for each channel
			{
				channel[i] = AIL_allocate_sample_handle( _snd_pDriver );
				if(!channel[i])
				{
					mprintf("Error allocating sound channel: " );
					mprintf( AIL_error );
					// AIL_shutdown();
					_snd_Installed = FALSE;
					_snd_DriverType = 0;
					break;
				}   
				else
				{
					AIL_init_sample(channel[i]);  //---------init the channel
				}
			}
		}   
		
		#if MIDI	
		//=============================	midi startup	==================
		switch( _snd_DriverType )
		{
		case 1:		//	SoundBlaster generic
			//mdriver = AIL_install_MDI_driver_file("sblaster.mdi",0);
			mdriver = AIL_install_MDI_driver_file("sbpro1.mdi",0);
			break;
		case 2:		//	Gravis UltraSound !!!WHich one????????
			mdriver = AIL_install_MDI_driver_file("null.mdi",0);
			break;
		default:
			mdriver = AIL_install_MDI_driver_file("null.mdi",0);
			break;
		}
		
		if(!mdriver)   //-------------- couldn't load driver
		{
			mprintf("Midi driver load error: " );
			mprintf( AIL_error );
			AIL_shutdown();
			_snd_Installed = FALSE;
			_snd_DriverType = 0;
		}
		else				//	have a driver
		{
			
			for( i=0; i < SEQUENCES; i++ ) //-------get AIL handles for each SEQUENCE
			{
				sequence[i] = AIL_allocate_sequence_handle( mdriver );
				if(!sequence[i])
				{
					mprintf("Error allocating midi seq handle: " );
					mprintf( AIL_error );
					AIL_shutdown();
					mdriver = 0;
					break;
				}   
			}
		}
		#endif
	}
	else
	{
		_snd_Installed = FALSE;
		_snd_DriverType = 0;
	}
	
	//	Initialize the timer manager.
	TimerSetup();
	//	Clear timer counter array.
	for(i = 0; i < _SYS_TIMERS; i++)
	{
		_sys_aTimerCounters[i] = 0L;
	}										
}

void SndStopAIL()
{
	AIL_shutdown();
	TimerResetChip();
}

//-----------------	load sounds from disk to virtual memory -------------

void SndLoadModule (LONG module, BOOLEAN lock)
{
	struct	find_t fileinfo;
	LONG	o,i,done;
	ULONG	rc;
	USHORT	*ptr;
	LONG	defined;
	   
	if( _snd_Installed == FALSE )
		return;
		
	mprintf("load module %d \n",module);
	tot = 0;
	defined = sizeof(modules)/4 ;	//# of 4byte pointers in modules[] =
	
	if(module > defined-1)
	{
		mprintf("Attempt to load invalid module:%d",module);
		return;
	}
	
	for (i=0; i<8; i++)		//----------check to see if already loaded
	{
		if(loaded[i] == module)
		{
			mprintf("Module is already loaded!\n");
			return;
		}
	}
	
	for (i=0; i<8; i++)		//-----------	mark the module as loaded ---
	{
		if(loaded[i] < 0)
		{
			loaded[i] = module;
			break;
		}
	}
	ptr = modules[module];
	
	o=0;
	done=0;
	
	do
	{
		//   i= module0[o];
		i= ptr[o];
		if(i >= 0x8000)
		{
			done = 1;
			break;
		}
		// strcpy(name,path);
		if(lock)
			loadh(i);		//load and lock it
		else
			load(i);
		o++;
	} while (!done);
	
	mprintf("Total size of snds in module %d: %d \n",module,tot);
	
#if MIDI
	//===================	now, load midi files	===========
	if(i == midiMARK)
		o++;	//skip midiMARK
	done = 0;
	do
	{
		//i= module0[o];
		i= ptr[o];
		if(i >= 0x8000)	
		{                 //should be endMARK
			done = 1;
			break;
		}
		
		// strcpy(name,mpath);
		strcpy(name,xmidi[i].name);
		
		if(hdlFile = ResourceLock( name, FALSE ) )
		{
			xmidi[i].hdl = hdlFile;
			xmidi[i].size = VMSize( hdlFile );		//get the filesize from dos
			mtot += xmidi[i].size;
		}
		else
		{
			xmidi[i].size = 0;		//this midi file does not exist
		}
		mprintf("%s  \n",name);
		
		o++;
	} while(!done);
	
	if(mtot) mprintf("Total size of midi files in memory %d \n",mtot);
	
	mprintf("modules loaded: ");
	for (i=0; i<8; i++)		//----------show what's loaded
	{
		if(loaded[i] >=0) mprintf("%d ",loaded[i]);
	}
#endif
	   
	mprintf("\n");
}

void loadh(long i)		//-------------load and lock a sample--------------------
{
	strcpy(name,snds[i].name);
		
	if( _VM_NULLHDL != ( hdlFile = ResourceLock( name, FALSE ) ) )
	{
		snds[i].hdl = hdlFile;
		snds[i].size = VMSize( hdlFile );
		tot += snds[i].size;
	}
	else
	{
		snds[i].size = 0;		//this sound does not exist
	}
	// mprintf("%s rate:%d size:%d \n",name,snds[i].rate,snds[i].size);
}

void load(long i)		//-------------load a sample--------------------
{
	strcpy(name,snds[i].name);
		
	if( _VM_NULLHDL != ( hdlFile = ResourceLoad( name, FALSE, NULL ) ) )
	{
		snds[i].hdl = hdlFile;
		snds[i].size = VMSize( hdlFile );
		tot += snds[i].size;
	}
	else
	{
	 	snds[i].size = 0;		//this sound does not exist
	}
	// mprintf("%s rate:%d size:%d \n",name,snds[i].rate,snds[i].size);
}

void SndUnloadModule (LONG module)
{
	LONG	o,i,done;
	LONG	midi = 0;
	USHORT	*ptr;
	LONG	defined;
	   
	if( _snd_Installed == FALSE )
		return;
		
	mprintf("unload module %d \n",module);
		
	defined = sizeof(modules)/4 ;	//# of 4byte pointers in modules[] =
	//# of valid modules
	if(module > defined-1)
	{
		mprintf("Attempt to unload invalid module:%d",module);
		return;
	}
	for (i=0;i<8 ;i++)		//----------check to see if IS loaded
	{
		if(loaded[i] == module)
		{
			loaded[i] = -1; //--	mark the module as unloaded ---
			break;
		}
	}
	
	if( i == 8 )
	{
		mprintf("Can't unload module. Wasn't loaded.\n");
		return;
	}
	
	ptr = modules[module];
	
	o=0;
	done=0;
	while(1)		//----- unload everything in module -----
	{
		i= ptr[o];
      if(i == midiMARK)
			midi = 1;
		else
      if(i == endMARK)
			return;
		else
		{
         if(!midi)
         {
				VMUnLock(snds[i].hdl);
				ResourceReleaseHandle(snds[i].hdl);
		}
			#if MIDI
         else
				ResourceReleaseHandle(xmidi[i].hdl);
			#endif
      }
		o++;
	} 
	
}

/*#######################################################################\
#    System counter interupt routines
\#######################################################################*/

CALLBACK SysCounterFunc( void )
{
	/* counter 2 is a 120 Hz timer */
	_sys_aTimerCounters[2]++;
	
	/* counter 1 is a 60 Hz timer */
	if( _sys_aTimerCounters[2] & 0x1 )
	{
		_sys_aTimerCounters[1]++;
		t60++;
		if(crowdnoise) 
			do_crowdnoise();
		if(fadech >=0) 
			update_fade();
		doqueue();				//------process snd que for announcer voice
	}
		
	/* counter 0 is a 30 Hz timer */
	if( ( _sys_aTimerCounters[2] & 0x3) == 0x3 )
		_sys_aTimerCounters[0]++;
		
	/* one second stop watches ( two of them ) */
	if( ( _sys_aTimerCounters[2] & 0x7F ) == 0x7F )
	{
		/*** Watch 1 ***/
		if( _sys_StopWatch[0] == STOPWATCH_UP )
		{
			_sys_aTimerCounters[4]++;
		}
		else
		if( _sys_StopWatch[0] == STOPWATCH_DOWN )
		{
			if( _sys_aTimerCounters[4] )
				_sys_aTimerCounters[4]--;
		}
		/* else is STOPWATCH_OFF */
		
		/*** Watch 2 ***/
		if( _sys_StopWatch[1] == STOPWATCH_UP )
		{
			_sys_aTimerCounters[5]++;
		}
		else
		if( _sys_StopWatch[1] == STOPWATCH_DOWN )
		{
			if( _sys_aTimerCounters[5] )
				_sys_aTimerCounters[5]--;
		}
		/* else is STOPWATCH_OFF */
	}
	
	return 0;
}

/*****************************************************************************
Initialize the timer manager
[06-19-92]
[11-11-94]	JC		Reworking
------------------------------------------------------------------------------
This function initializes all global timer manager variables and installs the
timer interrupt handler via TimerInstall()

Note this procedure initializes the frequency descriptors array.
*****************************************************************************/
void TimerSetup( void )
{

	//	Hook in service for a 120Hz SysCounter(0).
	sys_ctr1hdl = AIL_register_timer( SysCounterFunc );
	AIL_set_timer_frequency( sys_ctr1hdl, 120 );
	AIL_start_timer( sys_ctr1hdl );
}


/*****************************************************************************
Deinitialize the timer manager
[06-28-92]
[11-11-94]	JC	Reworking
------------------------------------------------------------------------------
*****************************************************************************/
void TimerRestore( void )
{
	AIL_stop_timer( sys_ctr1hdl );
}

/*****************************************************************************
Reset the hardware timer chip
[11-14-94]
------------------------------------------------------------------------------
*****************************************************************************/
void TimerResetChip( void )
{
	/* reset the timer chip back to 18.2 Hz */
	outp(0x43, 0x0E ); // was 0x36
	outp(0x40, 0xff );
	outp(0x40, 0xff );
}

/*****************************************************************************
Manipulate a stopwatch
------------------------------------------------------------------------------
	WhichWatch:
		0 is first stopwatch, 1 is the second
		
	ACTIONS:
		STOPWATCH_CLEAR			Zero the watch
		STOPWATCH_START			Begin counting
		STOPWATCH_STOP			Stop counting
		STOPWATCH_UP			Count upward
		STOPWATCH_DOWN			Count down
		STOPWATCH_SET	Value	Set stopwatch to a Value
*****************************************************************************/
SHORT TimerStopWatch(
	USHORT	WhichWatch,
	SHORT	Action,
	SHORT	Value
)
{
	SHORT	retval = 0;
	
	if(WhichWatch > 1)
		return retval;
		
	/* always return the current timer value */
	retval = _sys_aTimerCounters[4 + WhichWatch];
	
	switch(Action)
	{
	case STOPWATCH_CLEAR:
		_sys_aTimerCounters[4 + WhichWatch] = 0;
		break;
		
	case STOPWATCH_SET:
		_sys_aTimerCounters[4 + WhichWatch] = Value;
		break;
		
	case STOPWATCH_STOP:
		_sys_StopWatch[WhichWatch] = STOPWATCH_STOP;
		break;
		
	case STOPWATCH_UP:
		_sys_StopWatch[WhichWatch] = STOPWATCH_UP;
		break;
		
	case STOPWATCH_DOWN:
		_sys_StopWatch[WhichWatch] = STOPWATCH_DOWN;
		break;
		
	default:
		break;
	}
	
	return retval;
}

//=======================	Dummy VM cover routines	==================
// 
// 
// VM_HDL ResourceLoad(CHAR *name, LONG i)
// {
// FILE *f;
// CHAR *ptr;
// 
//       snds[i].ptr = malloc(snds[i].size);
//    	ptr = snds[i].ptr;
//       
//       if(!ptr)
// 			fatal_error("Out of memory in load sound");
// 		
//       f=fopen(name,"rb");
// 		if(!f)
// 			fatal_error("Cant load sound: %s",name);
// 
//       fread(&ptr[0],snds[i].size,1,f);			//load the sound to Real memory
// 		fclose(f);
// 
//       return( (VM_HDL) i);   	//return index as a handle
// }
// 
// 
// VM_HDL ResourceLoadmidi(CHAR *name, LONG i)
// {
// FILE *f;
// CHAR *ptr;
// 
//       xmidi[i].ptr = malloc(xmidi[i].size);
//    	ptr = xmidi[i].ptr;
//       
//       if(!ptr)
// 			fatal_error("Out of memory in load midi");
// 		
//       f=fopen(name,"rb");
// 		if(!f)
// 			fatal_error("Cant load midi: %s",name);
// 
//       fread(&ptr[0],xmidi[i].size,1,f);			//load the sound to Real memory
// 		fclose(f);
// 
//       return( (VM_HDL) i);   	//return index as a handle
// }
// 
// 
// void* VMLoad(VM_HDL i)
// {
// void * ptr;
//            					//use handle as a sound index to get real pointer
// 	ptr = snds[i].ptr;
// 	
//    return(ptr);   
// }
// 
// void* VMLoadmidi(VM_HDL i)
// {
// void * ptr;
//            					//use handle as a sound index to get real pointer
// 	ptr = xmidi[i].ptr;
// 	
//    return(ptr);   
// }
// 
// 
// VMUnLock(VM_HDL i)
// {
// 	free( snds[i].ptr );
// }
// 
// 






//===========================	crowd noise ===================


UBYTE drumseq[] = { 3, 3,   3, 3, 1, 1, 1, 3, 3, 3,0};
UBYTE defeseq[] = { 6+128,  12,               6 ,0};		//"Defense!
long drumindex,drumcnt;

UBYTE rockseq[] = {2,2,4,0};										//Rock-U

#define cG	750  //1500
#define cC	1000 //2000
#define cE	1250 //2500
#define cG1	1500 //3000

UBYTE charseq[] = {1, 1, 1, 2, 1, 3,  9,   0 };		//"Charge!
UWORD charfrq[] = {cG, cC, cE,cG1, cE, cG1, 0,   0 };		//"da da da dat da daaaaa
long organch = 0;



//frq values for a 2 8va chromatic scale
UWORD scale[] = {
	2093,2218,2350,2489,2637,2794,2960,3136,3322,3520,3730,3951			//,4186
  ,2093*2,2218*2,2350*2,2489*2,2637*2,2794*2,2960*2,3136*2,3322*2,3520*2,3730*2,3951*2
  ,4186*2
   };
enum {C1,CS1,D1,DS1,E1,F1,FS1,G1,GS1,A1,AS1,B1,
      C2,CS2,D2,DS2,E2,F2,FS2,G2,GS2,A2,AS2,B2,
      C3};
#define RR -1		//no note (rest)


UBYTE mexiseq[] = {                               //Mexican hat dance
	1,  2+128,  1,  2+128,  1,  5+128,   1,  1, 1, 1, 2+128, 1, 5+128,
	1,  2+128,  1,  2+128,  1,  5+128,   1,  1, 1, 1, 2+128, 1, 5+128, 0
     };		//+128 means drum(footstomp)  also
UBYTE mexifrq[] = {
	G1, C2, G1, C2, G1, C2,  G1, C2,D2,C2,B1,C2,D2,
	G1, B1, G1, B1, G1, B1,  G1, B1,C2,B1,A1,B1,C2, C1
     };		//




UBYTE Louiseq[] = {                               //Louie2
     1, 1, 3, 1, 2, 1, 1, 3, 1, 2, 0
     };
UBYTE Louifrq[] = {
     G1,G1,G1,C2,C2,D2,D2,D2,C2,C2
     };		//
UBYTE Loui2frq[] = {
     D2,D2,D2,E2,E2,F2,F2,F2,E2,E2
     };		//

     

UBYTE adamseq[] = {                               //Addams Family
	1,1,1, 3,   3+128,3+128,
	1,1,1, 3,   3+128,3+128,
	1,1,1, 3,   1,1,1,3,    
	1,1,1, 3,   3+128,3+128, 0
     };		//+128 means fingersnap(clap)  also
UBYTE adamfrq[] = {
	A1, B1, CS2, D2, D2, D2,
	A1, B1, CS2, D2, D2, D2,
	B1, CS2,DS2, E2, B1, CS2,DS2,E2,
   A1, B1, CS2, D2, D2, D2
     };		//

     
UBYTE chroseq[] = {                               //CHROMATIC
	3,3,3,3,
   3,3,3,3,
   3+64,3,3,3,
   3+64,3,3+64,3,
   3+64,3,3+64,3,
   3+64,3,3+64,3,
   3+64,3,3+64,3+64,
   3+64,3+64,3+64,3+64,
   1+64,1,1, 2+64,1, 3+64,
   4+128, 0
     };		//
UBYTE chrotic[] = {
   12,12,12,12,
//   11,11,11,11,
   10,10,10,10,
//    9, 9, 9, 9,
    8,8,8,8,
    7,7,7,7,
    6,6,6,6,
    5,5,5,5,
    4,4,4,4,
    3,3,3,3,

   9,9,9,9,9,9,9
     };		//
UBYTE chrofrq[] = {
	 G1, D1 ,E1,FS1,
   GS1,DS1, F1, G1,
    A1, E1,FS1,GS1,
   AS1, F1, G1, A1,
    B1,FS1,GS1,AS1,
    C2, G1, A1, B1,
   CS2, GS1,AS1,C2,
    D2, A1, B1, CS2,				//32
   A1,D2,FS2,A2,FS2,A2,
   A2		//"CHARGE!
     };		//

     
long mexiindex,mexicnt;


long crowdnoise;
long tickdel;
long iterations,repcount;


//=========================================================================

//	Start one one the crowdnoise "songs"
   

long start_crowdnoise(long what, long times)
{
	tickdel = drumcnt = drumindex = mexicnt = mexiindex = 0;      
   crowdnoise = what;
	iterations = times-1;
	repcount = 0;
   return(1);
}



//=============================================================================

//				crowd noise songs Defense, Rock-U, Charge, Mexican hat dance

//=============================================================================


void
do_crowdnoise()
{
long d;
long stompch,stompvol;

	if( _snd_Installed == FALSE )
		return ;
		

   if(tickdel)
   {
  		tickdel--;    
   }
   else
   {
		if(crowdnoise == CN_ROCKYOU)
      	tickdel = 12;
		else
      if(crowdnoise == CN_LOUIE)
    	tickdel = 15;
      else
      	tickdel = 9;
      
		if(crowdnoise == CN_DEFENSE)	//------------ chant: drum, drum, "Defense!"... -----------
   	{
      
	   	if(drumcnt)				//------------drum track
      		drumcnt--;
   		if(!drumcnt)
  			{
		      SndSayR(LODRUM,13025);			//perc11 hi drum
            //No, too much delay SndSay(STOMP);
				drumcnt = drumseq[drumindex];
      		drumindex++;
      		if(drumseq[drumindex] == 0) 	//if at end of seq
   	      {
            	if(iterations)
               {
               	iterations--;
               	drumindex = 0;		//repeat
            	}
               else
               {
                  crowdnoise = 0;
                  return;				//all done
               }
            }
         }
   		
         if(mexicnt)				//--------------defense track
		      mexicnt--;
	   	if(!mexicnt)
  			{
				d = defeseq[mexiindex];
      		if(d<128) SndSayVP(DEFEN,0,32);			//defense
				mexicnt = d&127;
		      mexiindex++;
      		if(defeseq[mexiindex] == 0) 	//if at end of seq
      		   mexiindex = 0;		//repeat
		   }
   	
      }
      
      else
		if(crowdnoise == CN_ROCKYOU) //--------------- drum beat "We Will rock-U" -------
   	{
//mprintf("mcnt:%d mindi:%d it:%d\n",mexicnt,mexiindex,iterations);         	
   		if(mexicnt)
   		   mexicnt--;
   		if(!mexicnt)
  			{
				mexicnt = rockseq[mexiindex];
      		if(mexicnt == 2)
		      {
//            	SndSayR(LODRUM,13000);			//perc11 hi drum
            	SndSay(STOMP);
				}
            else      	
         	{
//            	SndSay(LODRUM);			//perc11
            	SndSay(CLAP);
      		}
            mexiindex++;
      		if(rockseq[mexiindex] == 0) 	//if at end of seq
   	      {
            	if(iterations)
               {
               	iterations--;
               	mexiindex = 0;		//repeat
            	}
               else
               {
                  crowdnoise = 0;
                  return;				//all done
               }
            }
   		}
      }
		else
      if(crowdnoise == CN_CHARGE) //--------------- trumpet charge
   	{
   		if(mexicnt)
   		   mexicnt--;
   		if(!mexicnt)
  			{
				if(organch) stopsnd(organch);	//stop previous note
 //mprintf("mexiindex:%d frq:%d\n",mexiindex,charfrq[mexiindex]);		

            mexicnt = charseq[mexiindex];
         	if(charfrq[mexiindex])	//if not rest  (frq 0 = rest)
            {
													//play in ascending chromatic keys
               if(iterations == 3)
               {
                  //(mexicnt == 3)	//on last note, make the organ loop
                  // organch = SndSayR(BRALOOP,charfrq[mexiindex]*13);			
            		//se
                     organch = SndSayR(BRASS,charfrq[mexiindex]*13);			
               }
      			else
            	if(iterations == 2)
               {
                     organch = SndSayR(BRASS,charfrq[mexiindex]*14);			
               }
      			else
            	if(iterations == 1)
               {
                     organch = SndSayR(BRASS,charfrq[mexiindex]*15);			
               }
            	else
            	{
                  	organch = SndSayR(BRASS,charfrq[mexiindex]*16);			
               }
      		
            }
				
            else						//if rest,
            {
               SndSay(CHARGE);	//======crowd says "CHarge!
      			organch = 0;
            }

            mexiindex++;
      		if(charseq[mexiindex] == 0) 	//if at end of seq
   	      {

               if(iterations)
               {
               	iterations--;
               	mexiindex = 0;		//repeat
            	}
               else
               {
                  crowdnoise = 0;
            	 	organch = 0;
                  return;				//all done
               }
            }
   		}
      }
      else
		if(crowdnoise == CN_MEXIHAT) //--------------- Mexican hat dance -------
   	{
   		if(mexicnt)
   		   mexicnt--;
   		if(!mexicnt)
  			{
				drumcnt = mexiseq[mexiindex];
				mexicnt = mexiseq[mexiindex]&127;
//mprintf("mcnt:%d drmcnt:%d \n",mexicnt,drumcnt);         	
            										// organ
            SndSayR(ORGAN,scale[mexifrq[mexiindex]]*5);			
            if(drumcnt >= 128)
         		SndSay(CLAP);			//perc11
      		
            mexiindex++;
      		if(mexiseq[mexiindex] == 0) 	//if at end of seq
   	      {
            	if(iterations)
               {
               	iterations--;
               	mexiindex = 0;		//repeat
            	}
               else
               {
                  crowdnoise = 0;
                  return;				//all done
               }
            }
   		}
      }
      else
		if(crowdnoise == CN_LOUIE) //---------------Louis Louis
   	{
   		if(mexicnt)
   		   mexicnt--;
   		if(!mexicnt)
  			{
				mexicnt = Louiseq[mexiindex]&127;
            
//mprintf("req @ t60:%d ",t60);         	
            SndSayR(ORGAN,scale[Louifrq[mexiindex]]*4);			
          if(repcount)
             SndSayR(ORGAN,scale[Loui2frq[mexiindex]]*4);			
      		
            mexiindex++;
      		if(Louiseq[mexiindex] == 0) 	//if nxt = end of seq
   	      {
            	if(iterations)
               {
               	iterations--;
            		repcount++;
                  mexiindex = 0;		//repeat
            	}
               else
               {
                  crowdnoise = 0;
                  return;				//all done
               }
            }
   		}
      }
      else
		if(crowdnoise == CN_ADDAMS) //--------- Addams Family----------
   	{
   		if(mexicnt)
   		   mexicnt--;
   		if(!mexicnt)
  			{
				drumcnt = adamseq[mexiindex];
				mexicnt = adamseq[mexiindex]&127;
            										// organ
            if(drumcnt < 128)
            	SndSayR(ORGAN,scale[adamfrq[mexiindex]]*5);			
         	else
               SndSay(CLAP);			//perc11
      		
            mexiindex++;
      		if(adamseq[mexiindex] == 0) 	//if at end of seq
   	      {
            	if(iterations)
               {
               	iterations--;
               	mexiindex = 0;		//repeat
            	}
               else
               {
                  crowdnoise = 0;
                  return;				//all done
               }
            }
   		}
      }
      else
		if(crowdnoise == CN_CHROMATIC) //--------- Chromatic Seq ----------
   	{
	      tickdel = chrotic[mexiindex];    //special speed adjust
   
         if(mexicnt)
   		   mexicnt--;
   		if(!mexicnt)
  			{

            drumcnt = chroseq[mexiindex];
				mexicnt = chroseq[mexiindex]&63;
            										// organ

            if(drumcnt < 128)
            {
					if(organch) stopsnd(organch);	//stop previous note
            	organch = SndSayR(ORGAN,  scale[chrofrq[mexiindex]]*5);			
         	}	
            else
               SndSay(CHARGE);			

            if(drumcnt >=64 )
            {
               stompvol = mexiindex;
               if(stompvol >32) stompvol = 32;
               stompvol *=3;	//
               
					if(stompch) stopsnd(stompch);	//stop previous note
            	stompch = SndSayV(STOMP, stompvol);			
      		}
            
            mexiindex++;
      		if(chroseq[mexiindex] == 0) 	//if at end of seq
   	      {
            	if(iterations)
               {
               	iterations--;
               	mexiindex = 0;		//repeat
            	}
               else
               {
                  crowdnoise = 0;
                  return;				//all done
               }
            }
   		}
      }
   	//else invalid crowdnoise

   }
}


//===========================================================================

//									say a number

//===========================================================================


UWORD tees[11] = {SONE,TEN,TY2,TY3,TY4,TY5,TY6,TY7,TY8,TY9,TY10};
UWORD one_to_teens[20] =
				{SONE,ONE,TWO,THREE,FOUR,FIVE,SIX,SEVEN,EIGHT,NINE,TEN,
			   TEEN1,TEEN2,TEEN3,TEEN4,TEEN5,TEEN6,TEEN7,TEEN8,TEEN9};


void saynumber(long number)
{
long tens=0;
   
   if(!number)
   {
   	nsay(NOTHING);	//"Nothing!"
      return;
   }
   
   if(number >= 100)
   {
      if(number == 100)
      {
      	nsay(HUND);	//"one hundred" (down)
      	return;
      }
      if(number <110)
      {
      	nsay(TY10);	//"one hundred"
      	rest(10);
      }
      else	
         nsay(SONE);	//"one" (short)
   	
   	while(number >=100)	//never larger than 199	
      	number -=100;
   }

   if(number >=20)		//------------------------20-99
	{   
   	while(number >= 10)
   	{
      	tens++;
      	number-=10;
   	}
      nsay(tees[tens]);
   	if(tens == 2 && number < 3)		//20,21,22
         rest(15);
      if(number)
      	nsay(one_to_teens[number]);//if last digit not 0
   }
   else					//----------------------- 0-19
   {
      nsay(one_to_teens[number]);	
   }
   
}

void nsay(long number)
{
	if(loadnum)
	{
		mprintf("nsay:%d \n",number);
		loadh(number);		//load it
	}
	
	say(number);   		//que it
}

#if 01

void saytime(long Now)
{
long min,sec;

	min = Now/60;
   sec = (Now%60);

mprintf("min:%d sec:%d \n",min,sec);

   if(!min)				//if 0-59 sec				"twelve seconds"
   {
      saynumber(sec);
      nsay(SECONDS);
   }
   else
   {
      if(sec < 10)		//if 0-9 sec don't say seconds (no oh) 
      {
         if(min == 1)
         {
            nsay(ONE);						//"one minute"
         	nsay(MINUTE);
         }
         else
         {
            saynumber(min);						//"three minutes"
         	nsay(MINUTES);
         }
      }
      else
      {
      	saynumber(min);							//"four twenty five"
         saynumber(sec);
      }
   }

   
}


#endif

