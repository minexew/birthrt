/*#######################################################################\
#
#	Synergistic Software
#	(c) Copyright 1993-1995  All Rights Reserved.
#	Author:  Steve Coleman
#
\#######################################################################*/


#define SILENCE 01

#ifndef _SND_H
#define _SND_H

//=================	sound stuff	================

#define NO_SOUND 0L
#define BLASTER 1L			//driver equates
#define GRAVIS 2L

#define CHANNELS 8			//# of simultaneous sounds possible
   							//NOTE: currently the max is 8 ('cause of channel scans)
#define SEQUENCES 2			//# of simultaneous midi sequences possible

#define QSIZE	16			//# of snd events that can be qued before bumping
							//  starts
// stop watch controls
enum {
 	STOPWATCH_CLEAR,
	STOPWATCH_STOP,
	STOPWATCH_UP,
	STOPWATCH_DOWN,
	STOPWATCH_SET
};

typedef struct{		//-------- sound effect structure	-------
	CHAR	*name;
	LONG	rate;		//sample rate
	UBYTE	scan;		//channel scan bits
	CHAR	loop;		//# of loops (0 = continuous)
	CHAR	volume;		//default volume (if 0, use system default of 100)
	CHAR	type;		//8 = 8bit, else 16bit signed
	LONG	size;		//size of sample in memory
	CHAR	*ptr;		//pointer to sample in real memory
	VM_HDL	hdl;		//VM handle
} SOUND;

typedef struct{	//------------ Queue entry structure	-------
	SHORT	what;
	UBYTE	vol;
	UBYTE	pan;
   unsigned long t60;
} QENTRY;


typedef struct{	//------------ Xmidi file structure	-------
	CHAR 	*name;
	CHAR 	loop;		//# of loops (0 = continuous)
	CHAR 	volume;
	LONG 	size;
	CHAR 	*ptr;		//pointer to midifile in real memory
	VM_HDL	hdl;		//VM handle
} XMIDI;

   

//---------------------	snd indexes	-------------
enum  {
	CROWD,
	CHEER,
	DRIB1,
	SWISH1,
	SQUEAK1,
	RIMOFF,
	WHISTLE,
//	PORTLAND,
	BLAZERS,
	DREXLER,
	ROBINSON,
	STRICKLAND,
//	SEATTLE,
	SONICS,
	KEMP,
	PAYTON,
	SCHREMPF,
	SHOOTS,
	SCORES,
	MISSED,
	ITSGOOD,
	ITSIN,
	NOWAY,
	NOGOOD,
	BLOCKED,
	STEALS,
	NOBASK,
	NOTHIN,
	ALLTHE,
	_2POINT,
	_3POINT,
	ITSA3,
	GRABS,
	TIMESRUN,
	GOES,
	_24SEC,
	TIED,
	THROWS,
	SHOTAT5,
//	TWENTY,
	TUNE1,
	TUNE2,
	SLAMS,
	GOESHOOP,
	GETSO,
	WIDEO,
//	BUTTON1,
//	BUTTON2,
	INTRO1,
	INTRO2,
	TIMEOUT,
	
   OHYEAH,
	AHORN,
	DREXU,
	ROBIU,
	STRIU,
	KEMPU,
	PAYTU,
	SCHRU,
	HE,
	TOMAHAWK, //SLAM,
	MONSTER, //STUFF,
	OFFREB,
	PICKS,
	RIPPED,
	TOSSES,
	THROWSIN,
	DISHES,
	PASSES,
	FIRESTO,
	GOESTO,
//	AROUND,
//	POFFTO,
//	HOFFTO,
	WITHBALL,
//	GETSPASS,
	HERECOME,
	TAKEUP,
	WRAPS,
	DEFEN,
	LODRUM,
	HIDRUM,
	ORGAN,
	CHARGE,
   SETS,
   STOMP,
   CLAP,
	BRASS,

   PREGAME1,PREGAME2,
   AINTRO1,AINTRO2,AINTRO5,
   VSTHE1,ANDTHE1,TOGO,

	MINUTE,MINUTES,SECOND,SECONDS,

	OH,NOTHING,SONE,ONE,TWO,THREE,FOUR,FIVE,SIX,SEVEN,EIGHT,NINE,TEN,
   TEEN1,TEEN2,TEEN3,TEEN4,TEEN5,TEEN6,TEEN7,TEEN8,TEEN9,
   TY2,TY3,TY4,TY5,TY6,TY7,TY8,TY9,TY10,HUND,

	PLAYERS,
	LOOSE = 339

};

#define TEAMS PLAYERS+162


//------------	module defines	-------------

enum  {	GAMEMOD, BUTTONMOD, TUNE1MOD, TUNE2MOD, ANNOUNCERMOD,
			PLAYERSMOD,PREGAMEMOD,TEAMMOD,NUMBERMOD

};



/*#######################################################################\
#    Timer stuff
\#######################################################################*/

#define _SYS_TIMERS     6                       /* number of tick counters to be maintained */
#define _SYS_WATCHES    2                       /* number of stop watches */

/* stop watch control macro defines */

#ifndef _UI_H
#define SysCounter(ixCounter) \
				_sys_aTimerCounters[ixCounter]
#endif

#define		TimerSWSet( WhichWatch, Value )	\
				TimerStopWatch( (WhichWatch), STOPWATCH_SET, (Value) )

#define		TimerSWClear( WhichWatch )	\
				TimerStopWatch( (WhichWatch), STOPWATCH_SET, 0 )

#define		TimerSWStop( WhichWatch )	\
				TimerStopWatch( (WhichWatch), STOPWATCH_STOP, 0 )

#define		TimerSWCountUp( WhichWatch )	\
				TimerStopWatch( (WhichWatch), STOPWATCH_UP, 0 )

#define		TimerSWCountDown( WhichWatch )	\
				TimerStopWatch( (WhichWatch), STOPWATCH_DOWN, 0 )

//-----------------		externs	----------------

extern LONG 		_snd_DriverType;
extern DIG_DRIVER	*_snd_pDriver;
extern BOOLEAN		_snd_Installed;
extern HSAMPLE		channel[CHANNELS];		//Ail handles for each channel
extern SOUND		snds[];

/* system timer counters */
#ifndef _UI_H
extern volatile TCNT _sys_aTimerCounters[];  
#endif

extern volatile SHORT _sys_StopWatch[];

/* update counters - timer frequency handle */
extern HTIMER _sys_thdlCounters;
extern HTIMER _mouse_hdlTimer;

//jc	team and player name modules, so they can be spiked as needed.
extern USHORT teammod[];	//	two teams and one togo (togo?)
extern USHORT playersmod[];	//	six players, two names each


//-----------------		prototypes	----------------

void SndStartAIL(LONG driver);		//do this as soon as possible
									//load samples and midi files
void SndStopAIL(void);				//shut down ail lib
void SndLoadModule(LONG module, BOOLEAN lock);	//Do this after memory is set up
void SndUnloadModule(LONG module);	//do this at quit time

// play a digital effect with a volume, a pan(left,right bias), and rate
LONG SndSay(LONG what);
LONG SndSayV(LONG what, UBYTE vol);
LONG SndSayVP(LONG what, UBYTE vol, UBYTE pan);
LONG SndSayVR(LONG what, UBYTE vol, LONG rate);
LONG SndSayR(LONG what, LONG rate);
LONG SndSayVPR(LONG what, UBYTE vol, UBYTE pan, LONG rate); 

//alter playing effects
void SndVolume(LONG ch, LONG vol);
void SndRate(LONG ch, LONG rate);
void stopsnd(long ch);

//-----------	macros	-------------------

#define SndStop(ch) stopsnd(ch)
#define say(what)  SndSayV(what, sndVolumeEffects )


LONG SndStatus(LONG ch);
void SndQuiet();
void SndFade(long ch, long vol, long ticks);        

void rest(long ticks);							//silent rest for n 1/60th sec
void clearqueue(void);        //----toss everything currently in the Q


// play a midi sound
LONG SndPlay(LONG what);		//play an xmidi file
void SndEndSeq(LONG seq);			//stop a seq
LONG SndStatusSeq(LONG seq);


CALLBACK SysCounterFunc( void );
void TimerSetup( void );
void TimerRestore( void );
void TimerResetChip( void );
SHORT TimerStopWatch( USHORT WhichWatch, SHORT Action, SHORT Value );

void saynumber(long number);
void nsay(long number);
void saytime(long Now);


#endif


//-------------	crowd noise	----------

enum {CN_DEFENSE = 1, CN_ROCKYOU,CN_CHARGE,CN_MEXIHAT,CN_LOUIE,CN_ADDAMS,CN_CHROMATIC};

long start_crowdnoise(long what, long times);

extern long crowdch;
extern UBYTE master;

void upname(void);
void dnname(void);
void othername(void);


extern long qput,qplay; //pointers (indexes) into que. if qput=qplay, que empty
extern long crowdnoise;		// # of the current crowdnoise. 0 is none playing.
extern long iterations;		//setting this to 0 will end the current crowd thing
extern long loadnum;		//if true, load numbers as needed


