//-----------------------------------------------------------------------------
//
//        Copyright 1995/1996 by NW Synergistic Software, Inc.
//
//        DPlayUtl.h - Provides interface for Direct Play networking.
//
//------------------------------------------------------------------------------

#ifndef _DPLAYUTL_H
#define _DPLAYUTL_H

#define MAX_TALK					(80)
#define MAX_PLAYER_NAME			(52)
#define MAX_NET_PLAYER			(10)    


//---- Game network status 
#define NETSTAT_NONE       0
#define NETSTAT_MASTER     1
#define NETSTAT_JOINED     2
#define NETSTAT_WAITINGFORLEVELINFO	3


//---- Net work defines 

#define MAX_PLAYERS                 10    //---- Should be 10 
#define MAX_NETBUFF                 512   //---- Should be 512 but ?

#define MAX_NET_NOMSG_TICK          120000 //---- Kick them off if no message in 2 minutes ?

#define DP_BROADCAST_ID          0        //---- Broadcast id used to send to everybody
#define DP_MASTER_ID             1        //---- This is always 1

//---- Message ids 

#define ABORT_MSG_ID             1        //---- master or name server tells everyone to abort
#define INFO_MSG_ID              2
#define UINFO_MSG_ID             3
#define TALK_MSG_ID              4       //---- Talk message from and to
#define LEVEL_INFO_REQUEST_MSG_ID 	5		 // -- client requesting level information
#define LEVEL_INFO_MSG_ID			6		 // -- mastyer sending level information


 
typedef char SZPLAYER[MAX_PLAYER_NAME];               //---- This is the Logon name

typedef char SZTALK[MAX_TALK];     


//---- Basic network packet 
typedef struct 
{
    BYTE             MsgId;
    USHORT           usSync;
    char             cData[1];

}   NETMSG , * LPNETMSG; 


//#define     MAX_NET_TEKIS       30
//#define     MAX_NET_SHOTS       50
//#define     MAX_NET_OBJECTS     100
//#define     MAX_NET_WEAPONS     20

//#define     NET_SHOT_TARGET     127    //---- Whether X contains target or target

#define     MAX_NET_BYTEOBJECTS 13

 
//---- Structure used to send info to the master 

typedef struct 
{
    USHORT  iX       : 9;
    USHORT  iY       : 7;
    UCHAR   bBitmNum;
    UCHAR   bFrame;
}   MTHEXDER,   *   LPMTHEXDER;


//---- Structure used to send to each client 

typedef struct 
{
	   USHORT  iX  :   9;
		USHORT  iY  :   7;
		USHORT  usScore;	// added 12 OCT 95 CAM
		UCHAR   bFrame;
		UCHAR   bBitmNum;
		UCHAR   bdpID;
		UCHAR   bColor;                      
}   CTHEXDER,   *   LPCTHEXDER;



typedef struct 
{
    USHORT  iX  :   9;
    USHORT  iY  :   7;
    UCHAR   bDir;

}   CTEKIS,     *   LPCTEKIS;



typedef struct
{

    USHORT  iX                 : 9;
    USHORT  iY                 : 7;
    USHORT  bValid             : 1;
    USHORT  bShotType          : 6;
    USHORT  bShotInstigator    : 9;     //---- Could be seven but 8 bits fit in the struct
    UCHAR   bdpID;
}   CSHOTS,     *   LPCSHOTS;



typedef struct
{

    byte    fVisible;                //---- There will be 13 ( 100 bits )

}   COBJECTS,   * LPCOBJECTS;



typedef struct 
{

    USHORT  iX  :   9;
    USHORT  iY  :   7;
    UCHAR   bWhichObj : 2;             //---- 1 is 6    2 is 7    3 is 21  0 non used    
    UCHAR   bdpID     : 6;

}   CWEAPONS,   *   LPCWEAPONS;



//----- Network messages ------------------------------------------------------------------


//---- Info message sent to master every frame 

typedef struct 
{
//    MTHEXDER    MThexder;
//    CSHOTS      CShots[MAX_NET_SHOTS];
//    CTEKIS      CTekis[MAX_NET_TEKIS];
//    COBJECTS    CObjects[MAX_NET_BYTEOBJECTS];  //---- used a hundred bits
//    CWEAPONS    CWeapons[MAX_NET_WEAPONS];
    DWORD       Score;

}  INFO_MSG,    *   LPINFO_MSG;


//---- Info sent to each client 

typedef struct 
{
//    CTHEXDER    CThexder[MAX_PLAYERS];
//    CSHOTS      CShots[MAX_NET_SHOTS];
//    CTEKIS      CTekis[MAX_NET_TEKIS];
//    COBJECTS    CObjects[MAX_NET_BYTEOBJECTS];  //---- used a hundred bits
//    CWEAPONS    CWeapons[MAX_NET_WEAPONS];
	DWORD	dwFiller; 
}   UINFO_MSG,  * LPUINFO_MSG;              


typedef struct
{
    SZTALK szTalk;

} TALK_MSG, * LPTALK_MSG;           //---- talk message 


typedef struct
{
    byte filler;

} ABORT_MSG, * LPABORT_MSG;         //---- abort message


 // -- 
 // -- Level Info Request message
 // --
typedef struct
{
    byte filler;

} LEVEL_INFO_REQUEST_MSG, * LPLEVEL_INFO_REQUEST_MSG;

 // -- 
 // -- Level Info message
 // --
typedef struct
{
    UINT   	uiNetNumber;           	//---- Level number to use
//	UINT	uiNetTekis;				//---- Number of tekis to create
//	UINT	uiNetScore;				//---- Score to stop at, none if zero
//    BOOL    fCreateEnemy;           //---- Create tekis
//    BOOL    fRegEnemy;              //---- Regenerate Tekis
//    BOOL    fRegObj;                //---- Regenerate Objects
} LEVEL_INFO_MSG, * LPLEVEL_INFO_MSG; 


typedef char NETFORMAL[52];         //---- these parallel DPLONGNAME in dplay.h
typedef char NETFRIENDLY[20];


//---- This is filled by networking code 

typedef struct 
{
    byte            bStatus;                    //---- local status
    DWORD           dpID;                       //---- local Dplay ID
    DWORD           dwSession;                  //---- DPlay session we are in 
    NETFRIENDLY     szFriendly;                 //---- local friendly name
    NETFORMAL       szFormal;                   //---- local formal name

    UINT            uiNetNumber;                //---- Level number to use
//	UINT			uiNetTekis;					//---- Number of tekis to create
//	UINT			uiNetScore;					//---- Score to stop at, none if zero
//    BOOL            fCreateEnemy;               //---- Create tekis
//    BOOL            fRegEnemy;                  //---- Regenerate Tekis
//    BOOL            fRegObj;                    //---- Regenerate Objects
//    int             iColor;                     //---- Color if not the game master

    USHORT          MsgCount;                   //---- Last msg number I sent 
    int             iFrameUp;                   //---- Divisor for sending receiving 

    DWORD           dwTick[MAX_PLAYERS];        //---- Last msg time from this thexder

//    THEXDER         NetThexder[MAX_PLAYERS];    //---- Should be each players data 
    DWORD           NetdpID[MAX_PLAYERS];       //---- If filled in then we are getting messages ?
    int             NetColor[MAX_PLAYERS];      //---- Color sent by master 

    USHORT          NetMsgCount[MAX_PLAYERS];   //---- Last message number sent ( how out of sync )
    DWORD           NetMsgDropped[MAX_PLAYERS]; //---- Number of messages not in sync 
    

    NETFRIENDLY     NetszFriendly[MAX_PLAYERS]; //---- Network short name 
    NETFORMAL       NetszFormal[MAX_PLAYERS];   //---- Computer name 
    DWORD           NetScore[MAX_PLAYERS];      //---- Player score 

//    NSHOTS          nShots[MAX_SHOTS];
//    NTEKIS          nTekis[MAX_TEKIS];
//    DWORD           nTekiRegTime[MAX_TEKIS];
//    NOBJECTS        nObjects[MAX_NET_OBJECTS];      //---- 
//    DWORD           nObjRegTime[MAX_NET_OBJECTS];   //---- Time we regenerated this object if master 
//    NWEAPONS        nWeapons[MAX_NET_WEAPONS];

}   NETPLAYERS,     * LPNETPLAYERS;


// --- Prototypes ---

BOOL FinalNet				( void );
BOOL InitNet				( HWND hwndApp );
BOOL ReceiveMessageNet	( DWORD dwWait );
BOOL SendMessageNet		( DWORD  dpToId, BYTE   bMsgId,LPVOID lpMessage );
void ToggleNet				( void );

void KillLatePlayers ( void );

#endif

//---- End of DPlayUtl.h

