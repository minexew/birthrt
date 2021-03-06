/* ========================================================================
   Copyright (c) 1990,1996	Synergistic Software
   All Rights Reserved
   ========================================================================
   Filename: gamekey.cpp  -
   Author:   Gary Powell
   
   ========================================================================
   
   Contains the following general functions:

   
   ======================================================================== */

/* ------------------------------------------------------------------------
   Includes
   ------------------------------------------------------------------------ */

#if defined(_WINDOWS)
#include <windows.h>
#endif

#include "engine.h"

#include "gamekey.hxx"

/* ------------------------------------------------------------------------
   Notes
   ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------
   Defines and Compile Flags
   ------------------------------------------------------------------------ */
   
/* ------------------------------------------------------------------------
   Macros
   ------------------------------------------------------------------------ */
   
/* ------------------------------------------------------------------------
   Prototypes
   ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------
   Global Variables
   ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------
   Local Variables
   ------------------------------------------------------------------------ */
GAME_KEYS::VERSION GAME_KEYS::fVersion(CURRENT);

#ifdef _FOREIGNVER
#ifdef _GERMANVER
SHORT	GAME_KEYS::fKeyDefinitions[MAX_KEYS] = {
	KEY_ESCAPE,		// STOP_RUNNING_DEMO
	KEY_SPACE,		// SKIP_TO_NEXT_FRAME
	
	KEY_ESCAPE,		// MAIN_MENU
	KEY_F1,			// HELP_MENU
	KEY_F3,			// TOGGLE_CHAT_WINDOW
	KEY_F4,			// TOGGLE_CHAT_DISPLAY_WINDOW
	KEY_F2,			// TOGGLE_DIRECT_DRAW
	KEY_P,			// TOGGLE_GAME_PAUSE
	
	KEY_UP,	 		// MAP_MOVE_UP,
	KEY_UPLEFT,	 	// MAP_MOVE_UPLEFT,
	KEY_UPRIGHT,	// MAP_MOVE_UPRIGHT,
	KEY_DOWN,		// MAP_MOVE_DOWN,
	KEY_DOWNLEFT,	// MAP_MOVE_DOWNLEFT,
	KEY_DOWNRIGHT,	// MAP_MOVE_DOWNRIGHT,
	KEY_LEFT,		// MAP_MOVE_LEFT,
	KEY_RIGHT,		// MAP_MOVE_RIGHT,
	
	KEY_OTHERPLUS,	// MAP_SCALE_UP,
	KEY_OTHERMINUS,	// MAP_SCALE_DOWN,
		
	KEY_S,			// PLAYER_SHOW_STATUS,
	KEY_I,			// PLAYER_SHOW_INVENTORY
	
	KEY_LEFT,		// PLAYER_MOVE_LEFT
	KEY_RIGHT,		// PLAYER_MOVE_RIGHT
	KEY_A,	  		// PLAYER_MOVE_UP
	KEY_Z,	      	// PLAYER_MOVE_DOWN
	KEY_UP,	  		// PLAYER_MOVE_FORWARD
	KEY_DOWN,		// PLAYER_MOVE_BACKWARD
	KEY_UPRIGHT,	// PLAYER_LOOK_UP
	KEY_DOWNRIGHT,	// PLAYER_LOOK_DOWN
	KEY_UPLEFT,		// PLAYER_PITCH_UP
	KEY_DOWNLEFT,	// PLAYER_PITCH_DOWN
	KEY_INSERT,		// PLAYER_SLIDE_LEFT
	KEY_DELETE,		// PLAYER_SLIDE_RIGHT
	KEY_RIGHTSHIFT,	// PLAYER_MOVE_RUN
	KEY_LEFTSHIFT, 	// PLAYER_MOVE_RUN2
	KEY_CENTER,		// PLAYER_AUTOCENTER
	KEY_CONTROL,	// PLAYER_WALKTHRUWALL
	
	KEY_SPACEBAR,	// PLAYER_OPENDOOR
	
	KEY_OTHERPLUS,	// MAP_ZOOMIN,
	KEY_OTHERMINUS,	// MAP_ZOOMOUT
	KEY_M,			// MAP_TOGGLEDISPLAY
		
	KEY_V,			// CAMERA_FOLLOW_BEHIND
	
	KEY_LEFTSQUARE,	// SHRINK_VIEWING_AREA
	KEY_RIGHTSQUARE,// EXPAND_VIEWING_AREA
		
	KEY_0,	   		// COMBAT_TOGGLEMODE
	KEY_E,	   		// COMBAT_RETURN_TO_MAP
	KEY_R,			// COMBAT_RETREAT
	KEY_D,			// COMBAT_DONE
	KEY_L,			// COMBAT_NEW_LEADER
	
	KEY_X,	   		// COMBAT_HIGH_ATTACK
	KEY_LEFTSHIFT,	// COMBAT_QUICKATTACK_MODIFIER
	KEY_RIGHTSHIFT,	// COMBAT_QUICKATTACK_MODIFIER2
	KEY_C,	 		// COMBAT_LOW_ATTACK
	KEY_Z,	   		// COMBAT_DUCK
	KEY_D,	   		// COMBAT_DEFEND
	KEY_F,	   		// COMBAT_FALLBACK
	KEY_1,	   		// COMBAT_SPELL1
	KEY_2,	   		// COMBAT_SPELL2
	KEY_3,	   		// COMBAT_SPELL3
	KEY_4,	   		// COMBAT_SPELL4
	KEY_5,	   		// COMBAT_SPELL5
	KEY_6,	   		// COMBAT_SPELL6
	
	KEY_TAB,		// COMBAT_CYCLE_ACTIVE
			   	
	KEY_F9,	   		// COMBAT_CAMERA_FIRSTPERSON
	KEY_F5,	   		// COMBAT_CAMERA_BEHINDFRIEND
	KEY_F6,	   		// COMBAT_CAMERA_BEHINDFOE
	KEY_F7,	   		// COMBAT_CAMERA_RIGHTFRIEND
	KEY_F8	   		// COMBAT_CAMERA_LEFTFRIEND
};
#endif

#ifdef _FRENCHVER
SHORT	GAME_KEYS::fKeyDefinitions[MAX_KEYS] = {
	KEY_ESCAPE,		// STOP_RUNNING_DEMO
	KEY_SPACE,		// SKIP_TO_NEXT_FRAME
	
	KEY_ESCAPE,		// MAIN_MENU
	KEY_F1,			// HELP_MENU
	KEY_F3,			// TOGGLE_CHAT_WINDOW
	KEY_F4,			// TOGGLE_CHAT_DISPLAY_WINDOW
	KEY_F2,			// TOGGLE_DIRECT_DRAW
	KEY_P,			// TOGGLE_GAME_PAUSE
	
	KEY_UP,	 		// MAP_MOVE_UP,
	KEY_UPLEFT,	 	// MAP_MOVE_UPLEFT,
	KEY_UPRIGHT,	// MAP_MOVE_UPRIGHT,
	KEY_DOWN,		// MAP_MOVE_DOWN,
	KEY_DOWNLEFT,	// MAP_MOVE_DOWNLEFT,
	KEY_DOWNRIGHT,	// MAP_MOVE_DOWNRIGHT,
	KEY_LEFT,		// MAP_MOVE_LEFT,
	KEY_RIGHT,		// MAP_MOVE_RIGHT,
	
	KEY_OTHERPLUS,	// MAP_SCALE_UP,
	KEY_OTHERMINUS,	// MAP_SCALE_DOWN,
		
	KEY_S,			// PLAYER_SHOW_STATUS,
	KEY_I,			// PLAYER_SHOW_INVENTORY
	
	KEY_LEFT,		// PLAYER_MOVE_LEFT
	KEY_RIGHT,		// PLAYER_MOVE_RIGHT
	KEY_A,	  		// PLAYER_MOVE_UP
	KEY_Z,	      	// PLAYER_MOVE_DOWN
	KEY_UP,	  		// PLAYER_MOVE_FORWARD
	KEY_DOWN,		// PLAYER_MOVE_BACKWARD
	KEY_UPRIGHT,	// PLAYER_LOOK_UP
	KEY_DOWNRIGHT,	// PLAYER_LOOK_DOWN
	KEY_UPLEFT,		// PLAYER_PITCH_UP
	KEY_DOWNLEFT,	// PLAYER_PITCH_DOWN
	KEY_INSERT,		// PLAYER_SLIDE_LEFT
	KEY_DELETE,		// PLAYER_SLIDE_RIGHT
	KEY_RIGHTSHIFT,	// PLAYER_MOVE_RUN
	KEY_LEFTSHIFT, 	// PLAYER_MOVE_RUN2
	KEY_CENTER,		// PLAYER_AUTOCENTER
	KEY_CONTROL,	// PLAYER_WALKTHRUWALL
	
	KEY_SPACEBAR,	// PLAYER_OPENDOOR
	
	KEY_OTHERPLUS,	// MAP_ZOOMIN,
	KEY_OTHERMINUS,	// MAP_ZOOMOUT
	KEY_M,			// MAP_TOGGLEDISPLAY
		
	KEY_V,			// CAMERA_FOLLOW_BEHIND
	
	KEY_LEFTSQUARE,	// SHRINK_VIEWING_AREA
	KEY_RIGHTSQUARE,// EXPAND_VIEWING_AREA
		
	KEY_0,	   		// COMBAT_TOGGLEMODE
	KEY_Q,	   		// COMBAT_RETURN_TO_MAP
	KEY_R,			// COMBAT_RETREAT
	KEY_D,			// COMBAT_DONE
	KEY_L,			// COMBAT_NEW_LEADER
	
	KEY_X,	   		// COMBAT_HIGH_ATTACK
	KEY_LEFTSHIFT,	// COMBAT_QUICKATTACK_MODIFIER
	KEY_RIGHTSHIFT,	// COMBAT_QUICKATTACK_MODIFIER2
	KEY_C,	 		// COMBAT_LOW_ATTACK
	KEY_Z,	   		// COMBAT_DUCK
	KEY_D,	   		// COMBAT_DEFEND
	KEY_F,	   		// COMBAT_FALLBACK
	KEY_1,	   		// COMBAT_SPELL1
	KEY_2,	   		// COMBAT_SPELL2
	KEY_3,	   		// COMBAT_SPELL3
	KEY_4,	   		// COMBAT_SPELL4
	KEY_5,	   		// COMBAT_SPELL5
	KEY_6,	   		// COMBAT_SPELL6
	
	KEY_TAB,		// COMBAT_CYCLE_ACTIVE
			   	
	KEY_F9,	   		// COMBAT_CAMERA_FIRSTPERSON
	KEY_F5,	   		// COMBAT_CAMERA_BEHINDFRIEND
	KEY_F6,	   		// COMBAT_CAMERA_BEHINDFOE
	KEY_F7,	   		// COMBAT_CAMERA_RIGHTFRIEND
	KEY_F8	   		// COMBAT_CAMERA_LEFTFRIEND
};
#endif

#else
// AMERICAN / ENGLISH VERSION
SHORT	GAME_KEYS::fKeyDefinitions[MAX_KEYS] = {
	KEY_ESCAPE,		// STOP_RUNNING_DEMO
	KEY_SPACE,		// SKIP_TO_NEXT_FRAME
	
	KEY_ESCAPE,		// MAIN_MENU
	KEY_F1,			// HELP_MENU
	KEY_F3,			// TOGGLE_CHAT_WINDOW
	KEY_F4,			// TOGGLE_CHAT_DISPLAY_WINDOW
	KEY_F2,			// TOGGLE_DIRECT_DRAW
	KEY_P,			// TOGGLE_GAME_PAUSE
	
	KEY_UP,	 		// MAP_MOVE_UP,
	KEY_UPLEFT,	 	// MAP_MOVE_UPLEFT,
	KEY_UPRIGHT,	// MAP_MOVE_UPRIGHT,
	KEY_DOWN,		// MAP_MOVE_DOWN,
	KEY_DOWNLEFT,	// MAP_MOVE_DOWNLEFT,
	KEY_DOWNRIGHT,	// MAP_MOVE_DOWNRIGHT,
	KEY_LEFT,		// MAP_MOVE_LEFT,
	KEY_RIGHT,		// MAP_MOVE_RIGHT,
	
	KEY_OTHERPLUS,	// MAP_SCALE_UP,
	KEY_OTHERMINUS,	// MAP_SCALE_DOWN,
		
	KEY_S,			// PLAYER_SHOW_STATUS,
	KEY_I,			// PLAYER_SHOW_INVENTORY
	
	KEY_LEFT,		// PLAYER_MOVE_LEFT
	KEY_RIGHT,		// PLAYER_MOVE_RIGHT
	KEY_A,	  		// PLAYER_MOVE_UP
	KEY_Z,	      	// PLAYER_MOVE_DOWN
	KEY_UP,	  		// PLAYER_MOVE_FORWARD
	KEY_DOWN,		// PLAYER_MOVE_BACKWARD
	KEY_UPRIGHT,	// PLAYER_LOOK_UP
	KEY_DOWNRIGHT,	// PLAYER_LOOK_DOWN
	KEY_UPLEFT,		// PLAYER_PITCH_UP
	KEY_DOWNLEFT,	// PLAYER_PITCH_DOWN
	KEY_INSERT,		// PLAYER_SLIDE_LEFT
	KEY_DELETE,		// PLAYER_SLIDE_RIGHT
	KEY_RIGHTSHIFT,	// PLAYER_MOVE_RUN
	KEY_LEFTSHIFT, 	// PLAYER_MOVE_RUN2
	KEY_CENTER,		// PLAYER_AUTOCENTER
	KEY_CONTROL,	// PLAYER_WALKTHRUWALL
	
	KEY_SPACEBAR,	// PLAYER_OPENDOOR
	
	KEY_OTHERPLUS,	// MAP_ZOOMIN,
	KEY_OTHERMINUS,	// MAP_ZOOMOUT
	KEY_M,			// MAP_TOGGLEDISPLAY
		
	KEY_V,			// CAMERA_FOLLOW_BEHIND
	
	KEY_LEFTSQUARE,	// SHRINK_VIEWING_AREA
	KEY_RIGHTSQUARE,// EXPAND_VIEWING_AREA
		
	KEY_0,	   		// COMBAT_TOGGLEMODE
	KEY_E,	   		// COMBAT_RETURN_TO_MAP
	KEY_R,			// COMBAT_RETREAT
	KEY_D,			// COMBAT_DONE
	KEY_L,			// COMBAT_NEW_LEADER
	
	KEY_X,	   		// COMBAT_HIGH_ATTACK
	KEY_LEFTSHIFT,	// COMBAT_QUICKATTACK_MODIFIER
	KEY_RIGHTSHIFT,	// COMBAT_QUICKATTACK_MODIFIER2
	KEY_C,	 		// COMBAT_LOW_ATTACK
	KEY_Z,	   		// COMBAT_DUCK
	KEY_D,	   		// COMBAT_DEFEND
	KEY_F,	   		// COMBAT_FALLBACK
	KEY_1,	   		// COMBAT_SPELL1
	KEY_2,	   		// COMBAT_SPELL2
	KEY_3,	   		// COMBAT_SPELL3
	KEY_4,	   		// COMBAT_SPELL4
	KEY_5,	   		// COMBAT_SPELL5
	KEY_6,	   		// COMBAT_SPELL6
	
	KEY_TAB,		// COMBAT_CYCLE_ACTIVE
			   	
	KEY_F9,	   		// COMBAT_CAMERA_FIRSTPERSON
	KEY_F5,	   		// COMBAT_CAMERA_BEHINDFRIEND
	KEY_F6,	   		// COMBAT_CAMERA_BEHINDFOE
	KEY_F7,	   		// COMBAT_CAMERA_RIGHTFRIEND
	KEY_F8	   		// COMBAT_CAMERA_LEFTFRIEND
};
#endif


