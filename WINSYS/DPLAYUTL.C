//-----------------------------------------------------------------------------
//
//		Copyright 1995/1996 by NW Synergistic Software, Inc.
//
//		DPlayUtl.c - Direct play routines 
//
//------------------------------------------------------------------------------

#define INITGUID                //---- Need to initialize our GUID
#include <limits.h>
#include <time.h>
#include <windows.h>
#include <windowsx.h>
#include "stdio.h"
#include "dplay.h"

#include "SWinUtil.h"

#include "DPlayUtl.h"
#include "DPlayUtS.h"


//LPGUID const GAME_GUID;       //---- Pointer used for game GUID


#define MAX_ENUM_TIMEOUT    2500   //---- Wait for 2 1/2 seconds the first time 
#define MAX_ENUM_TIMEOUT2   2500   //---- Wait for another 2 1/2 seconds

const int iREGENERATECOUNTER = 10;


//---- Helps for loading the DPlay.Dll

typedef HRESULT (__stdcall *FP_DPENUM)   ( LPDPENUMDPCALLBACK, LPVOID );
typedef HRESULT (__stdcall *FP_DPCREATE) ( LPGUID , LPDIRECTPLAY FAR *, IUnknown FAR *);


//---- The pointers MS forgot

typedef DPMSG_GENERIC       * LPDPMSG_GENERIC;
typedef DPMSG_ADDPLAYER     * LPDPMSG_ADDPLAYER;
typedef DPMSG_DELETEPLAYER  * LPDPMSG_DELETEPLAYER;



FP_DPENUM        fp_DPEnum   = NULL;    //---- Function pointer to DirectEnumProviders()
FP_DPCREATE      fp_DPCreate = NULL;    //---- Function pointer to DirectPlayCreate()                                                


static  HANDLE  dphEvent = NULL;        //---- We aren't using events 

HINSTANCE       hDPlay;                 //---- Instance for dplay.dll

LPDIRECTPLAY    lpDP = NULL;            //---- Direct Play Object

static HWND hwndNetApp;


DPCAPS    dpThexCaps;                   //---- Device Caps global for lpDP object

char szSessionName[DPSESSIONNAMELEN];   //---- Session Name to creating or joining 
UINT uiNetNumber;                       //---- Net number used for the level

BOOL fTimedout;
HWND hwndJoinWait = NULL;
USHORT usStartFrame = 0;
const int iFRAMEWAIT = 40;
const int iMAXWAIT = 3;

//----- Table of session used in for connecting 

typedef struct
{ 
    DWORD dwSession;
    char szSessionName[DPSESSIONNAMELEN];   //---- Session Name to creating or joining 
    DWORD dwUser1;
    DWORD dwUser2;
    DWORD dwUser3;
    DWORD dwUser4;    

}   DPSESS_TBL, * LPDPSESS_TBL;


//---- If I had lists ?

static DPSESS_TBL DpSess_Tbl[100];      //---- Over a hundred thexder games ?

int iDpSess;

extern HWND hwndComm;


//--------------------------------------------------------------------------------

void FinalDPlay( void );

BOOL CALLBACK DialogCreateJoin	( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK DialogJoinWait	( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK DialogNetLogon	( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK DialogNetProvider	( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );

void ErrorDPlay( HRESULT hResult );
static BOOL InitDPlay ( HWND hwndNetApp );
void FinalDPlay( void );


BOOL DestroyCommWindow ( void );
BOOL CreateCommWindow ( void );




//------------------------------------------------------------------------------------
// Function:   SetBit() 
//
// Inputs:  
//
// Outputs: 
//
// Abstract: Sets a bit in the passed string. No checks are done on string length 
//
//------------------------------------------------------------------------------------
void SetBit ( int n, LPSTR lpBitData )
{
    USHORT Byte;
    CHAR   Bit, Mask;
        
    Byte = n >> 3;
    Bit  = n % 8;
    Mask = 1 << Bit;
    
    lpBitData[Byte] |= Mask;           

    
}

//------------------------------------------------------------------------------------
// Function:   ClearBit() 
//
// Inputs:  
//
// Outputs: 
//
// Abstract: Clears a bit in the passed string. No checks are done on string length 
//
//------------------------------------------------------------------------------------

void ClearBit ( int n, LPSTR lpBitData )
{
    USHORT Byte;
    CHAR   Bit, Mask;
        
    Byte = n >> 3;
    Bit  = n % 8;
    Mask = ~(1 << Bit);
    
    lpBitData[Byte] &= Mask;           

    
}

//------------------------------------------------------------------------------------
// Function:   IsBit() 
//
// Inputs:  
//
// Outputs: 
//
// Abstract: is bit in the passed string set. No checks are done on string length 
//
//------------------------------------------------------------------------------------

BOOL IsBit ( int n, LPSTR lpBitData )
{
    USHORT Byte;
    CHAR   Bit, Mask;
        
    Byte = n >> 3;
    Bit  = n % 8;
    Mask = 1 << Bit;
    
    return ( lpBitData[Byte] & Mask );           

    
}




//------------------------------------------------------------------------------------
// Function:   ToggleNet() 
//
// Inputs:  
//
// Outputs: 
//
// Abstract: 
//
//------------------------------------------------------------------------------------
void ToggleNet ( void )
{
    if ( fNetworked == TRUE )
    {
        FinalNet();
    }
    else
    {
        fNetworked = TRUE;
        InitNet( hwndNetApp );
    }

}   //---- End of ToggleNet()




//------------------------------------------------------------------------------------
// Function:   InitNet() 
//
// Inputs:  
//
// Outputs: 
//
// Abstract: Initialize the network. 
//
//------------------------------------------------------------------------------------
BOOL InitNet ( HWND hwndNetApp )
{

//    DWORD   Info_Size   = sizeof ( INFO_MSG );
//    DWORD   UInfo_Size  = sizeof ( UINFO_MSG );
//    DWORD   Thex_Size   = sizeof ( CTHEXDER );
//    DWORD   Shot_Size   = sizeof ( CSHOTS );
//    DWORD   Teki_Size   = sizeof ( CTEKIS );
//    DWORD   Obj_Size    = sizeof ( COBJECTS );
//    DWORD   Wea_Size    = sizeof ( CWEAPONS );

    //---- Don't let them start networking again
    
	if ( fNetworked == TRUE )
	{
		EnableMenuItem( hMenuInit, ID_APP_FILE_CONNECT, fNetworked ? MF_GRAYED : MF_ENABLED );
		return FALSE;           
	}   

	memset ( &NetPlayers, 0, sizeof ( NETPLAYERS ) );

	 // --
    // -- Initialize direct play
	 // --
	if ( InitDPlay( hwndNetApp ) == FALSE )
	{
		fNetworked = FALSE;
		EnableMenuItem( hMenuInit, ID_APP_FILE_CONNECT, fNetworked ? MF_GRAYED : MF_ENABLED );
		return FALSE;
	} 

	EnableMenuItem( hMenuInit, ID_APP_FILE_CONNECT, fNetworked ? MF_GRAYED : MF_ENABLED );

	return TRUE;

} //---- End of InitNet() 




//------------------------------------------------------------------------------------
// Function:   FinalNet() 
//
// Inputs:   Cleanup network stuff 
//
// Outputs: 
//
// Abstract:  
//
//------------------------------------------------------------------------------------

BOOL FinalNet ( void )
{
	fNetworked = FALSE;

	memset ( &NetPlayers, 0, sizeof ( NETPLAYERS ) ); 

	DestroyCommWindow ( );

	FinalDPlay();


	EnableMenuItem( hMenuInit, ID_APP_FILE_CONNECT, fNetworked ? MF_GRAYED : MF_ENABLED );

	EnableMenuItem ( hMenuInit, ID_APP_FILE_DISCONNECT, 
				                     NetPlayers.dpID ? MF_ENABLED : MF_GRAYED );


	if ( !fExitting )
	{
		 // --
		 // -- Decide what to do now
	 	 // --
		if ( IDYES == SMessageBox( hwndNetApp,	hInstApp, 
											IDS_NET_NETGAMEDISCONNECT, IDS_GAME_NAME,
	                    								MB_YESNO | MB_ICONQUESTION) )
		{
		 	 // -- Go get a game to play
//		   	SelectAndLoadGame();
		}
		else
		{
			 // -- We're through playin' now
			fSkipExitQuestion = TRUE;
			PostMessage( hwndNetApp, WM_CLOSE, 0, 0 );
		}
	}

    return TRUE;

}   //---- FinalNet()




//----------------------------------------------------------------------------
//  InitDPlay
//
//  Description:
//
//		Initialize direct play. Basically just load the dll and get proc 
//      addresses.   
//
//  Arguments:
//
//
//  Return : 
//      
//
//----------------------------------------------------------------------------
static BOOL InitDPlay ( HWND hwnd )
{

    //---- Set GUID 

//    GAME_GUID = (LPGUID) &pGuid;
//    GAME_GUID = GAME_GUID;

	 // -- save this
	hwndNetApp = hwnd;


    //---- Load direct play dll 
    
	hDPlay = LoadLibrary ( "dplay.dll" );
	if ( hDPlay == NULL)
	{
		SMessageBox( hwndNetApp, hInstApp, IDS_NET_NOTSUPPORTED, 
	                    IDS_GAME_DIRECTPLAY, MB_OK | MB_ICONINFORMATION );

		return FALSE;
	}


	 // --
    // -- Get proc address for enumeration 
	 // --

	fp_DPEnum = (FP_DPENUM) GetProcAddress( hDPlay, "DirectPlayEnumerate");

	if ( fp_DPEnum == NULL )
	{
		return FALSE;
	}		


	 // --
    // -- Get the proc address for direct play creation 
	 // --

	fp_DPCreate = (FP_DPCREATE) GetProcAddress( hDPlay, "DirectPlayCreate");

	if ( fp_DPCreate == NULL )
	{
		return FALSE;
	}



    //
    //---- We need to display a dialog so the user can select which type of provider to use.
    //     The DirectPlay object is also created in the dialog procedure.

   if ( DialogBox( NULL, "NETPROVIDER", hwndNetApp, (DLGPROC) DialogNetProvider ) == FALSE )
   {
      FinalDPlay();
      return FALSE;
   }


   fNetworked = TRUE;


   return TRUE;

}   //---- End of InitDPlay()




//----------------------------------------------------------------------------
//  FinalDPlay
//
//  Description:
//
//		Cleanup direct play. Release all objects players etc. 
//
//  Arguments:
//
//
//  Return : 
//      
//
//----------------------------------------------------------------------------
void FinalDPlay( void )
{
	if( lpDP != NULL )
	{
		lpDP->lpVtbl->Close(lpDP);
		lpDP->lpVtbl->Release(lpDP);

		lpDP = NULL;
	}

	memset ( &NetPlayers, 0, sizeof ( NETPLAYERS ) );

	fNetworked = FALSE;

    return;

}    //---- End of FinalDPlay()




// ----------------------------------------------------------------------------
//   DisconnectPlayer()
//
//   Description: 
//
//      This procedure disconnects a connected session 
//                                                                              
//   Arguments:                                                                 
//
//                                                                              
//   Returns:                                                                   
//                                                                             
// ----------------------------------------------------------------------------
void DisconnectPlayer ( void )
{
	if ( lpDP != NULL )
	{
		if ( NetPlayers.dpID != 0 )
		{
			//---- If we are master try and tell everyone to leave the game

			if ( NetPlayers.bStatus == NETSTAT_MASTER )
			{ 
				ABORT_MSG Abort_Msg;

				SendMessageNet ( DP_BROADCAST_ID,
                                 ABORT_MSG_ID,
                                 &Abort_Msg );

			}


			//---- Tell everyone we are leaving

			lpDP->lpVtbl->DestroyPlayer( lpDP, NetPlayers.dpID );

			FinalNet();

		}

	} // end if player is connected

}   //---- End of DisconnectPlayer()




// ----------------------------------------------------------------------------
//   EnumServiceProvider
//                                                                              
//   Description: Procedure used by directplayenumerate to fill our listbox. 
//                                                                              
//   Arguments:                                                                 
//                                                                              
//   Returns:                                                                   
//                                                                             
// ----------------------------------------------------------------------------
BOOL FAR PASCAL EnumServiceProvider( LPGUID lpSPGuid,      LPSTR lpFriendlyName,
                                     DWORD dwMajorVersion, DWORD dwMinorVersion,
                                     LPVOID lpContext )

{
	LONG lIndex;
	HWND hWnd = (HWND) lpContext;


    // ???????? should we check the version numbers ?????????????? only work on 1.1?????????????????
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	lIndex = SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM) lpFriendlyName );

	if ( lIndex != LB_ERR )
		SendMessage( hWnd, LB_SETITEMDATA, lIndex, (LPARAM) lpSPGuid );

	return (TRUE);

}   //---- End of EnumServiceProvider()




// ----------------------------------------------------------------------------
//   EnumSession
//                                                                              
//   Description: Procedure used by EnumSessions to fill our listbox. 
//                                                                              
//   Arguments:                                                                 
//                                                                              
//   Returns:                                                                   
//                                                                             
// ----------------------------------------------------------------------------
BOOL FAR PASCAL EnumSession( LPDPSESSIONDESC     lpDPSGameDesc,
                             LPVOID              lpContext,
                             LPDWORD             lpdwTimeOut,
                             DWORD               dwFlags         )
{
	LONG iIndex;
	HWND hWnd = (HWND) lpContext;
	char szChar[256];
	char szTemp3[3] = "N";
	char szTemp4[3] = "N";


    //----- If we timeout something is wrong ? so abort 

	if ( dwFlags & DPESC_TIMEDOUT )
	{
		if ( fTimedout == FALSE )
		{
			*lpdwTimeOut = MAX_ENUM_TIMEOUT;     //--- Try alittle longer
			fTimedout    = TRUE;
			return ( TRUE );
		}

		return( FALSE );
	}

    //---- Insert in a table ( should be a list )

	DpSess_Tbl[iDpSess].dwSession = lpDPSGameDesc->dwSession;
	strcpy ( DpSess_Tbl[iDpSess].szSessionName, lpDPSGameDesc->szSessionName );
	DpSess_Tbl[iDpSess].dwUser1 = lpDPSGameDesc->dwUser1;
	DpSess_Tbl[iDpSess].dwUser2 = lpDPSGameDesc->dwUser2;
	DpSess_Tbl[iDpSess].dwUser3 = lpDPSGameDesc->dwUser3;
	DpSess_Tbl[iDpSess].dwUser4 = lpDPSGameDesc->dwUser4;

     
	fModem = FALSE;

	if(strcmp(lpDPSGameDesc->szSessionName, "Dial New Number") == 0)
	{
		LoadString( hInstApp, IDS_NET_MODEMGAME, szChar, sizeof(szChar) );
		fModem = TRUE;
	}
	else
	{
	    sprintf ( szChar, "%-32.32s Game:%5.5lu T:%2.2u RT:%1.1s RO:%1.1s",
              lpDPSGameDesc->szSessionName,
              lpDPSGameDesc->dwUser1,
              lpDPSGameDesc->dwUser2,
              szTemp3,
              szTemp4 );
	}

	iIndex = SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM) szChar );

	if (iIndex != LB_ERR)
		SendMessage( hWnd, LB_SETITEMDATA, iIndex, (LPARAM) lpDPSGameDesc->dwSession );

	return(TRUE);

} // EnumSession




// ----------------------------------------------------------------------------
//   EnumPlayers
//                                                                              
//   Description: Procedure used by EnumPlayers fill our listbox. 
//                                                                              
//   Arguments:                                                                 
//                                                                              
//   Returns:                                                                   
//                                                                             
// ----------------------------------------------------------------------------
BOOL FAR PASCAL EnumPlayers( DPID dpID, LPSTR lpFriendlyName, LPSTR lpFormalName, DWORD dwFlags, LPVOID lpContext )
{
	LONG iIndex;
	HWND hWnd = (HWND) lpContext;
	char szChar[256];

	sprintf ( szChar, "%s", lpFriendlyName );

	iIndex = SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM) szChar );

	return(TRUE);
} // EnumPlayers




// ----------------------------------------------------------------------------
//   EnumPlayersCreateAltView
//                                                                              
//   Description: Callback procedure used by EnumPlayers after a join to create alt 
//                views for each player in the game.
//                                                                              
//   Arguments:                                                                 
//                                                                              
//   Returns:                                                                   
//                                                                             
// ----------------------------------------------------------------------------
BOOL FAR PASCAL EnumPlayersCreateAltView ( DPID dpID, LPSTR lpFriendlyName, LPSTR lpFormalName, DWORD dwFlags, LPVOID lpContext )
{
	int x;

    //---- Don't do it for yourself  

	if ( dpID == NetPlayers.dpID )
	{
		return (TRUE);
	}


    //---- Find an open spot to put this player

	for ( x = 0; x < MAX_PLAYERS; x++ )
	{
		if ( NetPlayers.NetdpID[x] == 0 )
		{
			memcpy ( NetPlayers.NetszFriendly[x], lpFriendlyName, sizeof ( NETFRIENDLY ) );
			memcpy ( NetPlayers.NetszFormal[x], lpFormalName, sizeof ( NETFORMAL ) );

			NetPlayers.NetMsgCount[x] = 0;
			NetPlayers.NetScore[x]    = 0;
			NetPlayers.NetdpID[x]     = dpID;

			break;
		}

	}

	return (TRUE);
} // EnumPlayersCreateAltView




// ----------------------------------------------------------------------------
//   DialogNetProvider( hDlg, uiMessage, wParam, lParam )                                
//                                                                              
//   Description: 
//
//      This callback handles messages belonging to the DialogNetProvider
//      dialog box.
//                                                                              
//   Arguments:                                                                 
//
//       hwnd            window handle of dialog window                   
//       uiMessage       message number                                         
//       wParam          message-dependent                                      
//       lParam          message-dependent                                      
//                                                                              
//   Returns:                                                                   
//
//       TRUE if message has been processed, else FALSE                         
//                                                                             
// ----------------------------------------------------------------------------
BOOL CALLBACK DialogNetProvider( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	LONG    lIndex;
	LPGUID  lpGuid;
	HWND    hWndCtl;
	HRESULT hResult;

	switch (msg)
	{
		case WM_INITDIALOG:
			hWndCtl = GetDlgItem( hwnd, ID_LBPROVIDER );

			if ( hWndCtl == NULL )
			{
				EndDialog( hwnd, FALSE );
				return ( TRUE );
			}


            //---- Call DirectPlayEnumerate which fills the listbox with Direct Play providers 
			hResult = (( fp_DPEnum ) ( EnumServiceProvider, (LPVOID) hWndCtl ));

			ErrorDPlay( hResult );                

			if ( hResult != DP_OK )
			{
				EndDialog( hwnd, FALSE );
			}

			return (TRUE);


		case WM_COMMAND:
		{
			switch ( LOWORD(wParam) )
			{
				case IDCANCEL:
						EndDialog( hwnd, FALSE );
						return ( TRUE );

				case IDOK:
						lIndex = SendDlgItemMessage( hwnd, ID_LBPROVIDER, LB_GETCURSEL, 0, 0 );

                        
						if ( lIndex != LB_ERR )
						{
							lpGuid = (LPGUID) SendDlgItemMessage ( hwnd, 
                                                               ID_LBPROVIDER, 
                                                               LB_GETITEMDATA, 
                                                               (WPARAM) lIndex,
                                                               0 );

                        //---- Create our direct play object

							hResult = (fp_DPCreate )( lpGuid, &lpDP, NULL );

							ErrorDPlay( hResult );

							if ( hResult == DP_OK )
							{
								EndDialog( hwnd, TRUE );
							}
							else
							{
								FinalDPlay();
								EndDialog ( hwnd, FALSE );
							}
						}
						return (TRUE);

			}   //---- End of switch (LOWORD(wParam))
 
		}   //---- End of case WM_COMMAND:

		}   //---- End of switch (msg) 

	return (FALSE);

}   //---- End of DialogNetProvider()




// ----------------------------------------------------------------------------
//                                
//   CheckNetCaps
//                                                                           
//   Description: Check the appropriate network capabilities.
//
//   Arguments:                                                                 
//       pdpCaps		- pointer to net capabilities
//			piFrameUp	- pointer to frame update counter
//                                                                              
//   Returns:                                                                   
//
//       TRUE if message has been processed, else FALSE                         
//                                                                             
// ----------------------------------------------------------------------------
BOOL CheckNetCaps( PDPCAPS pdpCaps, PINT piFrameUp )
{
	BOOL fOK = TRUE;

//         debugf ( "Baud %lu", dpCaps.dwHundredBaud );

	 //---- Check baud and set update rate 
	if ( pdpCaps->dwHundredBaud == 100000 )      //---- Network game
	{
		(*piFrameUp) = 1;
	}
   else if ( pdpCaps->dwHundredBaud == 96 )
   {
       (*piFrameUp) = 4;
   }
   else if ( pdpCaps->dwHundredBaud == 144 )
   {
       (*piFrameUp) = 3;
   }
   else if ( pdpCaps->dwHundredBaud == 288 )
   {
       (*piFrameUp) = 2;
   }
   else if ( pdpCaps->dwHundredBaud > 288 )
   {
       (*piFrameUp) = 1;
   }
	else if ( pdpCaps->dwHundredBaud == 0 )
	{
		(*piFrameUp) = 1;
	}
   else
   {
		SMessageBox ( hwndNetApp, hInstApp,
													  IDS_NET_CONNECTTOSLOW, IDS_GAME_NAME,
			                                       MB_APPLMODAL | MB_OK );
		fOK = FALSE;
	}

	return ( fOK );
} // CheckNetCaps




// ----------------------------------------------------------------------------
//   DialogCreateJoin( hDlg, uiMessage, wParam, lParam )                                
//                                                                              
//   Description: 
//
//      This callback handles messages belonging to the DialogCreateJoin
//      dialog box.
//                                                                              
//   Arguments:                                                                 
//
//       hwnd            window handle of dialog window                   
//       uiMessage       message number                                         
//       wParam          message-dependent                                      
//       lParam          message-dependent                                      
//                                                                              
//   Returns:                                                                   
//
//       TRUE if message has been processed, else FALSE                         
//                                                                             
// ----------------------------------------------------------------------------

BOOL CALLBACK DialogCreateJoin (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HRESULT     hResult;

	switch (msg)
	{
        //---- Fill the list box with games /sessions 

		case WM_INITDIALOG:
		{
			DPSESSIONDESC dpDesc;
			HWND hWndCtl;

			hWndCtl = GetDlgItem( hwnd, ID_LBCONNECT );


		   if (hWndCtl == NULL)
			{
				EndDialog(hwnd, FALSE );
				return(TRUE);
			}

         memset( &dpDesc, 0x00, sizeof(DPSESSIONDESC) );

         dpDesc.dwSize = sizeof(dpDesc);

         dpDesc.guidSession = GAME_GUID;        //---- Thexders unique GUID

         iDpSess = 0;     //---- Reset index to 0 for our session table clear etc

         memset ( &DpSess_Tbl, 0, sizeof ( DpSess_Tbl ) );

			fTimedout = FALSE;


            //----- Enumerate all available sessions

			hResult = lpDP->lpVtbl->EnumSessions( lpDP, &dpDesc, 
                                          MAX_ENUM_TIMEOUT, 
                                          EnumSession, 
                                          (LPVOID) hWndCtl, 
                                          DPENUMSESSIONS_AVAILABLE );

            //------ Error checks ?????? 


            // ErrorDPlay( hResult );

			return(TRUE);

		}



      case WM_COMMAND:
      {
            //---- If they select a game then display (enumerate) the players 

			if ( HIWORD( wParam ) == LBN_SELCHANGE &&
                 LOWORD( wParam ) == ID_LBCONNECT     )
			{
				HWND    hWndCtl;
				LONG    lIndex;

				hWndCtl = GetDlgItem( hwnd, ID_LBPLAYERS );

				SendDlgItemMessage( hwnd, ID_LBPLAYERS, LB_RESETCONTENT, 0, 0 );


				lIndex = SendDlgItemMessage( hwnd, ID_LBCONNECT, LB_GETCURSEL, 0, 0);

				if (lIndex != LB_ERR)
				{
					DWORD dwSession;

					dwSession    = SendDlgItemMessage( hwnd,
                                                      ID_LBCONNECT,
                                                      LB_GETITEMDATA, 
                                                      lIndex, 0);


					hResult = lpDP->lpVtbl->EnumPlayers( lpDP, dwSession, 
                                                 EnumPlayers, 
                                                 (LPVOID) hWndCtl, 
                                                 DPENUMPLAYERS_SESSION );
				}
			}


            //---- Controls 
			switch( LOWORD(wParam))
         {
                //----  Create a new game folks 

				case ID_PBMASTER: 
				{
					DPSESSIONDESC dpDesc;
					char          szUserName[DPLONGNAMELEN + 1];
					DWORD         dwUNSize = DPLONGNAMELEN + 1;

					memset ( szUserName, 0, DPLONGNAMELEN + 1 );

						// do copy protect check for creator
					if( DoesCopyProtectFail( GameInfo.szLevelDir, GameInfo.szVolName ) )
					{
						int iMsgReturn;

#ifdef _DEBUG
						iMsgReturn = SMessageBox(hwndNetApp, hInstApp,
								IDS_GAME_CDCHECKNOTFOUND, IDS_GAME_CDCHECKTITLE, 
												MB_ICONEXCLAMATION | MB_OKCANCEL);
#else
						iMsgReturn = SMessageBox(hwndNetApp, hInstApp,
								IDS_GAME_CDCHECKNOTFOUND, IDS_GAME_CDCHECKTITLE, 
												MB_ICONEXCLAMATION | MB_OK );
#endif
						if(iMsgReturn == IDOK)
						{
							EndDialog ( hwnd, FALSE );
							return ( TRUE );
						}
					}

						if ( DialogBox( NULL, "NETLOGON", hwnd, (DLGPROC) DialogNetLogon ) == TRUE )
						{
							DPCAPS dpCaps;

						//---- Setup our game description before we create 

							memset( &dpDesc, 0x00, sizeof(DPSESSIONDESC) );
							dpDesc.dwSize = sizeof(dpDesc);
							dpDesc.dwMaxPlayers = 10;                        //---- Max players 10 
							dpDesc.dwFlags = DPOPEN_CREATESESSION;           //---- Create a game 
							dpDesc.guidSession = GAME_GUID;                //---- Thexder unique GUID

//                       	dpDesc.dwUser1 = NetPlayers.uiNetNumber;         //---- Level # to use
//                        dpDesc.dwUser2 = NetPlayers.uiNetTekis;          //---- Create Enemies
//   	                    dpDesc.dwUser3 = NetPlayers.fRegEnemy;           //---- Regenerate tekis
//       	                dpDesc.dwUser4 = NetPlayers.fRegObj;             //---- regenerate objects


                        //---- Session name was entered in NETLOGON Dialog
							strcpy( dpDesc.szSessionName, szSessionName );

							//---- Create a new networked game 
							hResult = lpDP->lpVtbl->Open( lpDP, &dpDesc );
							if ( hResult != DP_OK )
							{
								ErrorDPlay(hResult);
								FinalDPlay();
								EndDialog(hwnd, FALSE );
								return ( TRUE );
							}

							memset( &dpCaps, 0x00, sizeof(DPCAPS) );
							dpCaps.dwSize = sizeof(dpCaps);

							hResult = lpDP->lpVtbl->GetCaps( lpDP, &dpCaps );
							if ( hResult == DP_OK )
							{
								if ( !CheckNetCaps( &dpCaps ) )
								{
									FinalDPlay();
									EndDialog(hwnd, FALSE );
									return(FALSE);
								} 
							}
							else
							{
								ErrorDPlay(hResult);
							}
                    

                        //---- Save our session 
							NetPlayers.dwSession = dpDesc.dwSession;


                        //---- Setup the formal name 
							GetUserName( szUserName,  &dwUNSize );	 

                        //---- Create our player
							hResult = lpDP->lpVtbl->CreatePlayer( lpDP, &NetPlayers.dpID,
                                                          (LPSTR) &GameInfo.szPlayer,
                                                           szUserName,
                                                           &dphEvent);
							if ( hResult != DP_OK)
							{
								ErrorDPlay( hResult );
								FinalDPlay();
								EndDialog(hwnd, FALSE );
								return(FALSE);
							}

							memcpy ( NetPlayers.szFriendly, &GameInfo.szPlayer, sizeof (SZPLAYER));
							memcpy ( NetPlayers.szFormal, szUserName, dwUNSize );         


//                        CreateNetLevel( NetPlayers.uiNetNumber );
//
//                        NetPlayers.bStatus = NETSTAT_MASTER; // THIS SIGNALS MESSAGING

                        
							EndDialog(hwnd, TRUE );
							return(TRUE);

						}   //---- End of if DialogBox

						return(TRUE);

					}   //---- End of case ID_PBMASTER



					case ID_PBJOIN:
               {
						DPSESSIONDESC dpDesc;
						LONG          lIndex;
						char          szUserName[DPLONGNAMELEN + 1];
						DWORD         dwUNSize = DPLONGNAMELEN + 1;
						DPCAPS        dpCaps;
						LEVEL_INFO_REQUEST_MSG Level_Info_Request_Msg;

						memset ( szUserName, 0, DPLONGNAMELEN + 1 );

						lIndex = SendDlgItemMessage( hwnd, ID_LBCONNECT, LB_GETCURSEL, 0, 0);

						if (lIndex != LB_ERR)
						{
                        //---- Setup the join description 

							memset(&dpDesc, 0x00, sizeof(DPSESSIONDESC));
							dpDesc.dwSize       = sizeof(dpDesc);
							dpDesc.guidSession  = GAME_GUID;           //---- Thexder GUID
							dpDesc.dwFlags      = DPOPEN_OPENSESSION;

                        //---- Which game we are joining 

							dpDesc.dwSession    = SendDlgItemMessage( hwnd,
                                                                  ID_LBCONNECT,
                                                                  LB_GETITEMDATA, 
                                                                  lIndex, 0);

                        //----- Open the game session 
							hResult= lpDP->lpVtbl->Open( lpDP, &dpDesc );
							if ( hResult != DP_OK )
							{
								ErrorDPlay( hResult);

                            //----- Need message box here probably !!!!!!!!!!!!!!!!
								FinalDPlay();
								EndDialog(hwnd, FALSE);
								return (TRUE);
					 		}

							memset( &dpCaps, 0x00, sizeof(DPCAPS) );
							dpCaps.dwSize = sizeof(dpCaps);

							hResult = lpDP->lpVtbl->GetCaps( lpDP, &dpCaps );
							if ( hResult == DP_OK )
							{
								if ( !CheckNetCaps( &dpCaps, &NetPlayers.iFrameUp ) )
								{
									FinalDPlay();
									EndDialog(hwnd, FALSE );
									return(FALSE);
								} 
							}
							else
							{
								ErrorDPlay(hResult);
							}

                        //---- Setup the formal name 
							GetUserName( szUserName, &dwUNSize ); 

                       //---- Create our player
							hResult = lpDP->lpVtbl->CreatePlayer( lpDP, &NetPlayers.dpID,
                                                            (LPSTR) &GameInfo.szPlayer,
                                                            szUserName,
                                                            &dphEvent);
							if ( hResult != DP_OK )
							{
								ErrorDPlay( hResult );

								 //----- Need message box here 
								FinalDPlay();
								EndDialog(hwnd, FALSE );
								return(FALSE);
							}

							//---- Get everyone in our game 
							lpDP->lpVtbl->EnumPlayers( lpDP, 0,
                                           EnumPlayersCreateAltView,
                                           NULL,
                                           0 );

							memcpy ( NetPlayers.szFriendly, &GameInfo.szPlayer, sizeof (SZPLAYER));
							memcpy ( NetPlayers.szFormal, szUserName, dwUNSize );         


                        //---- Save our session 
							NetPlayers.dwSession = dpDesc.dwSession;
							NetPlayers.bStatus = NETSTAT_WAITINGFORLEVELINFO;

							hwndJoinWait = CreateDialog( hInstApp, "NETJOINWAIT", 
											hwndNetApp, (DLGPROC) DialogJoinWait );
							// -- note when we joined
							usStartFrame = numFrames + 1;

							ShowWindow( hwndJoinWait, SW_SHOWNORMAL );

							// -- request level information from the Master
							SendMessageNet ( DP_MASTER_ID,
			        		                         LEVEL_INFO_REQUEST_MSG_ID,
							       		                 &Level_Info_Request_Msg );

							EndDialog(hwnd, TRUE);

							return(TRUE);
						}

						return(TRUE);

					}   //---- End   case ID_PBJOIN:


					case IDCANCEL:
					{
						EndDialog ( hwnd, FALSE );
						return ( TRUE );
					}
						break;

					default:
						break;

				}   //---- switch (LOWORD) wParam

			}   //---- case WM_COMMAND

	}   //---- switch ( msg )


	return(FALSE);


}




// ----------------------------------------------------------------------------
//   DialogNetLogon( hDlg, uiMessage, wParam, lParam )
//                                                                                                              
//   Description: 
//
//      This callback handles messages belonging to the DialogNetLogon
//      dialog box.
//                                                                              
//   Arguments:                                                                 
//
//       hwnd            window handle of dialog window                   
//       uiMessage       message number                                         
//       wParam          message-dependent                                      
//       lParam          message-dependent                                      
//                                                                              
//   Returns:                                                                   
//
//       TRUE if message has been processed, else FALSE                         
//                                                                             
// ----------------------------------------------------------------------------

BOOL CALLBACK DialogNetLogon( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static HWND hEBox, hEBox1;
    static HWND hdWnd, hdWnd1; 
	static HWND hEBScore, hEBScore1;
	char szText[DPSESSIONNAMELEN];

    switch (msg)
    {

        //---- Fill the list box with games /sessions 

        case WM_INITDIALOG:
        {
            //---- Set length of session name max 

            SendDlgItemMessage( hwnd, ID_EFNETSESSION, EM_SETLIMITTEXT, DPSESSIONNAMELEN-1, 0 );
#if defined (_FRENCH)
			sprintf(szText, "%s jeu", GameInfo.szPlayer);
#elif defined (_GERMAN)
			sprintf(szText, "%ss Spiel", GameInfo.szPlayer);
#else
			sprintf(szText, "%s's game", GameInfo.szPlayer);
#endif
			SetDlgItemText( hwnd, ID_EFNETSESSION, szText );

       }


        case WM_COMMAND:
        {

            switch ( LOWORD(wParam) )
            {
                case IDCANCEL:
                {
                    EndDialog( hwnd, FALSE );
                    return ( TRUE );
                }
                
                case IDOK:
                {
                    
                    //--- If something was entered 

                    if ( GetDlgItemText( hwnd, ID_EFNETSESSION, szSessionName, DPSESSIONNAMELEN ) )
                    {
                        BOOL bSigned = FALSE;

                        NetPlayers.uiNetNumber = GetDlgItemInt ( hwnd, ID_EFNETNUMBER, NULL, bSigned );                        
//   	                    NetPlayers.uiNetTekis = GetDlgItemInt ( hwnd, ID_EFNETENEMIES, NULL, bSigned );
//   	                    NetPlayers.uiNetScore = GetDlgItemInt ( hwnd, ID_EFNETSCORE, NULL, bSigned );

//						if(IsDlgButtonChecked(hwnd, ID_CHNETSCOREDGAME) == FALSE)
//							NetPlayers.uiNetScore = 0;

//       	                NetPlayers.fCreateEnemy = IsDlgButtonChecked( hwnd, ID_CHNETCREATEENEMY );
//
//           	            if ( NetPlayers.fCreateEnemy == FALSE )
//               	        {
//                   	        NetPlayers.uiNetTekis = 0;
//                       	}
//
//                        NetPlayers.fRegEnemy = IsDlgButtonChecked ( hwnd, ID_CHNETREGENENEMY );
//   	                    NetPlayers.fRegObj = IsDlgButtonChecked( hwnd, ID_CHNETREGENOBJECT );

                        EndDialog( hwnd, TRUE );
                        return (TRUE);

                    }

                }


            }

        }

    }

   
    return (FALSE);


}   //---- DialogNetLogon()




//----------------------------------------------------------------------------
//  SendMessageNet
//
//  Description: Sends a message over the the net. 
//		 
//
//  Arguments:   dpToID    - Who to send to    0 - means broadcast to our session
//               BYTE      - message type 
//               lpMessage - Message to send 
//
//  Return : 
//      
//
//----------------------------------------------------------------------------

BOOL SendMessageNet( DPID   dpToId,
                     BYTE   bMsgId,
                     LPVOID lpMessage )

{
    HRESULT         hResult;
    DWORD           dwLenMsg  = sizeof (NETMSG);
    LPNETMSG        lpSndBuff = NULL;                  //---- Send buffer


    //---- Make sure we have a Direct play object 

    if ( lpDP == NULL                    ||
         NetPlayers.bStatus == NETSTAT_NONE )
    {
        return ( FALSE );
    }

	// -- Make sure we have actually started the game before sending
	// --  any messages except level info requests
	if ( (NETSTAT_WAITINGFORLEVELINFO == NetPlayers.bStatus) &&
			(bMsgId != LEVEL_INFO_REQUEST_MSG_ID) )
	{
		if ( 0 == ((numFrames - usStartFrame) % iFRAMEWAIT) )
		{
			LEVEL_INFO_REQUEST_MSG Level_Info_Request_Msg;

			 // -- resend the request
            SendMessageNet ( DP_MASTER_ID, LEVEL_INFO_REQUEST_MSG_ID,
                		                 		&Level_Info_Request_Msg );
		}

		 // -- waiting for a level info message
		return ( FALSE );
	}


    //---- Allocate our send buffer 

    lpSndBuff = (LPNETMSG) GlobalAllocPtr ( GPTR, MAX_NETBUFF );

    if ( lpSndBuff == NULL )
    {
//        debugf ( "Fatal Net Alloc S" );
        return ( FALSE );
    }


    //---- Setup header

    lpSndBuff->MsgId = bMsgId;
    lpSndBuff->usSync  = NetPlayers.MsgCount;
    

    switch ( bMsgId )
    {

        //---- Client send info to master 

        case INFO_MSG_ID:
        {
            LPINFO_MSG lpInfo_Msg = (LPINFO_MSG) &lpSndBuff->cData[0];

            int x;

//
//	 CODE HERE
//


            dwLenMsg += sizeof ( INFO_MSG );


            break;
            
        }


        //---- Master Info sent to client every frame 

        case UINFO_MSG_ID:
        {
            LPUINFO_MSG lpUInfo_Msg = (LPUINFO_MSG) &lpSndBuff->cData[0];


//
//	 CODE HERE
//


            dwLenMsg += sizeof ( UINFO_MSG );


            break;
        }



        case TALK_MSG_ID:
        {
            memcpy ( lpSndBuff->cData, lpMessage, sizeof ( TALK_MSG ) );
            dwLenMsg += sizeof ( TALK_MSG );
            break;
        }


        case ABORT_MSG_ID:
        {
            memcpy ( lpSndBuff->cData, lpMessage, sizeof ( ABORT_MSG ) );
            dwLenMsg += sizeof ( ABORT_MSG );
            break;
        }


        case LEVEL_INFO_REQUEST_MSG_ID:
        {
            memcpy ( lpSndBuff->cData, lpMessage, sizeof ( LEVEL_INFO_REQUEST_MSG ) );
            dwLenMsg += sizeof ( LEVEL_INFO_REQUEST_MSG );
            break;
        }


        case LEVEL_INFO_MSG_ID:
        {
            LPLEVEL_INFO_MSG lpLevel_Info_Msg = (LPLEVEL_INFO_MSG) &lpSndBuff->cData[0];

           	lpLevel_Info_Msg->uiNetNumber 	= NetPlayers.uiNetNumber;         //---- Level # to use
//            lpLevel_Info_Msg->uiNetTekis 	= NetPlayers.uiNetTekis;          //---- Create Enemies
//            lpLevel_Info_Msg->uiNetScore 	= NetPlayers.uiNetScore;          //---- Create ending score
//	        lpLevel_Info_Msg->fCreateEnemy 	= NetPlayers.fCreateEnemy;           //---- Regenerate tekis
//	        lpLevel_Info_Msg->fRegEnemy 	= NetPlayers.fRegEnemy;           //---- Regenerate tekis
//        	lpLevel_Info_Msg->fRegObj 		= NetPlayers.fRegObj;             //---- regenerate objects

            dwLenMsg += sizeof ( LEVEL_INFO_MSG );
            break;
        }

        default:
        {
//            debugf ( "Unknown msg ID -- Unable to transmit %u", bMsgId );

            //---- Free our buffers 
        
            GlobalFree ( lpSndBuff );

            return TRUE;   //---- Invalid message returns TRUE 
        }



    }


    if ( dwLenMsg > MAX_NETBUFF )
    {
//        debugf ( "Exceeded network buffer length !" );
    }


    //---- Send the message out 

    hResult = lpDP->lpVtbl->Send( lpDP, NetPlayers.dpID,  // from
                          dpToId,           // to
                          DPSEND_TRYONCE,
                          lpSndBuff,
                          dwLenMsg    );   


    //---- We get this message if no one is connected and we try to broadcast

    if ( hResult != DPERR_INVALIDPLAYER ) 
    {
        ErrorDPlay ( hResult );
    }

    //---- If we are trying to send to the master and he is not there then disconnect

    if ( hResult == DPERR_INVALIDPLAYER &&
         dpToId == 1                       )
    {
        DisconnectPlayer();
    }


    if ( hResult != DP_OK )
    {
        //---- Free our buffers 
        

        GlobalFree ( lpSndBuff );

        return ( FALSE );
    }


    //---- Update our message count 

    if ( NetPlayers.MsgCount > 30000 )
    {
        NetPlayers.MsgCount = 0;
    }
    else
    {
        NetPlayers.MsgCount++;
    }    


    //---- Free our buffers 
        
    GlobalFree ( lpSndBuff );


    return ( TRUE );



}   //---- End of SendMessageNet()




//----------------------------------------------------------------------------
//  ReceiveMessageNet
//  
//  Description: Receive a message over the the net. 
//     
//		 
//
//  Arguments:   dwWait - max how long to wait for a messages  
//
//  Return : 
//      
//
//----------------------------------------------------------------------------

BOOL ReceiveMessageNet( DWORD dwWait )

{

    DWORD    dwExit    = FALSE;   //---- exit read loop

    DPID     dpFromId  = 0;
    DPID     dpToId    = NetPlayers.dpID;   //---- Not used but set it anyway paranoia check 
    DWORD    dwBytes   = MAX_NETBUFF;
    DWORD    x = 0;
    DWORD    dwNumMsg  = 20;    //---- Set number at 20 just in case 
    int      y = 1;             //---- first index to Update message
    DWORD    dwCount   = 0;
    HRESULT  hResult   = DP_OK;
    BOOL     fWaited   = FALSE;
    LPNETMSG lpRecBuff = NULL;       //---- Receive buffer 

    
    //---- Make sure we have a Direct play object and we have created a player

    if ( lpDP == NULL                    || 
         NetPlayers.bStatus == NETSTAT_NONE )
    {
        return ( FALSE );
    }


    lpRecBuff = (LPNETMSG) GlobalAllocPtr ( GPTR, MAX_NETBUFF );

    if ( lpRecBuff == NULL )
    {
//        debugf ( "Fatal Net Alloc R" );
        return ( FALSE );
    }


    //---- Get number of messages  

    lpDP->lpVtbl->GetMessageCount( lpDP, NetPlayers.dpID, &dwNumMsg );


    //---- Waited receives  

    if ( dwWait != 0 )
    {
        fWaited = TRUE;

        dwWait += GetTickCount();
    }



    while ( dwExit == FALSE )        
    {
    
        hResult = lpDP->lpVtbl->Receive( lpDP, &dpFromId,
                                 &dpToId,
                                 DPRECEIVE_ALL,
                                 lpRecBuff,
                                 &dwBytes      );


        if ( hResult == DP_OK )
        {

            //----- System Message 

            if ( dpFromId == DP_BROADCAST_ID )
            {

                LPDPMSG_GENERIC lpdpMsgGen = (LPDPMSG_GENERIC) lpRecBuff;
                
                switch ( lpdpMsgGen->dwType )
                {

                    case DPSYS_ADDPLAYER:
                    {
                        int x;
                        LPDPMSG_ADDPLAYER lpdpMsgAdd = ( LPDPMSG_ADDPLAYER ) lpRecBuff; 
                        BOOL fFound = FALSE;


                        //---- Make sure we don't have this player

                        for ( x = 0; x < MAX_PLAYERS; x++ )
                        {
                            if ( NetPlayers.NetdpID[x] == lpdpMsgAdd->dpId )
                            {
                                fFound = TRUE;
                                break;
                            }    
                        }


                        //---- Don't add them again 

                        if ( fFound == FALSE )
                        {

                            //---- Find an open spot to put this player

                            for ( x = 0; x < MAX_PLAYERS; x++ )
                            {
        
                                if ( NetPlayers.NetdpID[x] == 0 )
                                {
                                    NetPlayers.NetdpID[x] = lpdpMsgAdd->dpId;
                    
                                    memcpy ( NetPlayers.NetszFriendly[x], 
                                             lpdpMsgAdd->szShortName, 
                                             sizeof ( NETFRIENDLY ) );

                                    memcpy ( NetPlayers.NetszFormal[x], 
                                             lpdpMsgAdd->szLongName, 
                                             sizeof ( NETFORMAL ) );

                                    NetPlayers.dwTick[x] = GetTickCount();
                                    NetPlayers.NetMsgCount[x] = 0;

                                    break;
                                }
                            }


                            //---- Tell the communication window to update 

                            if ( hwndComm )
                            {
//                                debugf ( "Update CommWind" );

                                PostMessage ( hwndComm,
                                              WM_COMMAND,
                                              ID_UPCOMMWIN,
                                              0 );

                            }

                        }


                        break;



                    }   //---- End case ADDPLAYER



                    case DPSYS_DELETEPLAYER:
                    {
                        int x;

                        LPDPMSG_DELETEPLAYER lpdpMsgDel = ( LPDPMSG_DELETEPLAYER ) lpRecBuff; 


                        //---- If master is telling me to leave 
                                                
                        if ( lpdpMsgDel->dpId == NetPlayers.dpID )
                        {
//                            debugf ( "Master killed me" );

                            DisconnectPlayer();
                        }
                        else
                        {          

                            for ( x = 0; x < MAX_PLAYERS; x++ )
                            {

                                if ( NetPlayers.NetdpID[x] == lpdpMsgDel->dpId )
                                {
//                                    debugf ( "Player %lu %s deleted",
//                                             NetPlayers.NetdpID[x],
//                                             NetPlayers.NetszFriendly[x] );   


                                    NetPlayers.NetdpID[x] = 0;

                                    memset ( NetPlayers.NetszFriendly[x], 
                                            0, 
                                             sizeof ( NETFRIENDLY ) );

                                    memset ( NetPlayers.NetszFormal[x], 
                                            0, 
                                             sizeof ( NETFORMAL ) );

//                                    memset ( &NetPlayers.NetThexder[x],
//                                             0,
//                                             sizeof ( THEXDER ) );

    
                                    if ( hwndComm )
                                    {
                
                                        PostMessage ( hwndComm,
                                                      WM_COMMAND,
                                                      ID_UPCOMMWIN,
                                                      0 );

                                    }


                                    break;

                                }

                            }

                        }
                        break;


                    }   //---- End of DELETEPLAYER

                    default:
                    {
//                        debugf ( "System message not handled %lu", lpdpMsgGen->dwType );                
                        break;
                    }
                
                }   //---- End of switch dwType    

 
            }
            else
            {

                int x;
                BOOL fBreak = FALSE; 

                //---- Check message sync / update message sync / last message time 

                for ( x = 0; x < MAX_PLAYERS; x ++ )
                {
                    if ( dpFromId == NetPlayers.NetdpID[x] )
                    {

                        //---- If the 

                        if ( (lpRecBuff->usSync != 0) &&
                        		(lpRecBuff->usSync <= NetPlayers.NetMsgCount[x]) )
                        {
//                             debugf ( "Sync Error" );
                             fBreak = TRUE;
                        }

                        NetPlayers.NetMsgCount[x] = lpRecBuff->usSync;
                        NetPlayers.dwTick[x] = GetTickCount();

                        break;

                     }

                }


                
                if ( fBreak == FALSE )
                {

                  //---- Its from a player 

                  switch ( lpRecBuff->MsgId )
                  {

                    //---- info to master 

                    case INFO_MSG_ID:
                    {

                        LPINFO_MSG lpInfo_Msg = (LPINFO_MSG) &lpRecBuff->cData[0];

//
// --- CODE HERE
//

                        break;
                    }




                    //----- Master to client 

                    case UINFO_MSG_ID:
                    {

                        LPUINFO_MSG lpUInfo_Msg = (LPUINFO_MSG) lpRecBuff->cData;
                        int x;
                        int i;

								 // -- we can't receive game messages in this state
								if ( NETSTAT_WAITINGFORLEVELINFO == NetPlayers.bStatus )
									break;

//
// --- CODE HERE
//

                        break;
                    }


                    //---- Someones talking 

                    case TALK_MSG_ID:
                    {
                        LPTALK_MSG lpTalk_Msg = (LPTALK_MSG) lpRecBuff->cData;
                        DWORD dwTemp;

                        //---- Send to CommWindow if it exists

                        if ( hwndComm )
                        {

                            dwTemp = SendDlgItemMessage( hwndComm,	
                                                         ID_LBCOMMHISTORY,
                                                         LB_GETCOUNT,	
                                                         0,	
                                                         0  );	

                            if ( dwTemp >= 30 )
                            {
                                SendDlgItemMessage ( hwndComm,
                                                     ID_LBCOMMHISTORY,
                                                     LB_DELETESTRING,
                                                     29,                   //---- Last one 
                                                     0 );                                                                       
                            }


                            SendDlgItemMessage( hwndComm,	
                                                ID_LBCOMMHISTORY,
                                                LB_INSERTSTRING,	
                                                0,	
                                                (LPARAM) lpTalk_Msg->szTalk );	

                        }

                        break;

                    }


                    //---- Master is telling us to abort / disconnect 

                    case ABORT_MSG_ID:
                    {
                        DisconnectPlayer();
                        dwExit = TRUE;
                        break;
                    }


                    //---- Client requested Master for level information

                    case LEVEL_INFO_REQUEST_MSG_ID:
                    {
						LEVEL_INFO_MSG Level_Info_Msg;

						// -- send the appropriate response
		                SendMessageNet ( DP_BROADCAST_ID,
                                 			LEVEL_INFO_MSG_ID,
                                 				&Level_Info_Msg );
		                break;
                    }


                    //---- Master sent Client level information

                    case LEVEL_INFO_MSG_ID:
                    {
			            LPLEVEL_INFO_MSG lpLevel_Info_Msg = (LPLEVEL_INFO_MSG) &lpRecBuff->cData;

						if ( NETSTAT_WAITINGFORLEVELINFO == NetPlayers.bStatus )
						{
							 // -- read the message
			           		NetPlayers.uiNetNumber 	= lpLevel_Info_Msg->uiNetNumber;         //---- Level # to use
//            				NetPlayers.uiNetTekis 	= lpLevel_Info_Msg->uiNetTekis;          //---- Create Enemies
//            				NetPlayers.uiNetScore 	= lpLevel_Info_Msg->uiNetScore;          //---- Create score
//	        				NetPlayers.fCreateEnemy = lpLevel_Info_Msg->fCreateEnemy;           //---- Regenerate tekis
//	        				NetPlayers.fRegEnemy 	= lpLevel_Info_Msg->fRegEnemy;           //---- Regenerate tekis
//        					NetPlayers.fRegObj 		= lpLevel_Info_Msg->fRegObj;             //---- regenerate objects

//							 // -- start the net game
//							CreateNetLevel( NetPlayers.uiNetNumber );

							 // -- destroy the wait window
						    if( hwndJoinWait != NULL )
							{
		    					DestroyWindow( hwndJoinWait );
								hwndJoinWait = NULL;
							}

							NetPlayers.bStatus = NETSTAT_JOINED;
						}
                        break;
                    }


                    default:

                    {
//                        debugf ( "Unknown message %lu from %lu", lpRecBuff->MsgId, dpFromId );
                        break;
                    }



                  }   //---- End of switch(MsgId)
                
                }   //---- End if fBreak == FALSE


            }    //---- End of if BROADCAST_ID


        }
        else    //---- if hResult != DP_OK
        {

            if ( hResult != DPERR_NOMESSAGES &&
                 hResult != DPERR_GENERIC       )
            {
                ErrorDPlay( hResult );
            }

            dwExit = TRUE;



        }


        //---- Seek an exit condition

        if ( fWaited == TRUE )
        {

            //---- Check to see if we expired 

            if ( dwWait < GetTickCount() )
            {
                dwExit = TRUE;
            }

        }
        else
        {
            //---- try and play catchup if we have too

            if ( x >= dwNumMsg )
            {
                dwExit = TRUE;
            }

            x++;

        }


    }   //---- End of while( dwExit )

    //---- Free our buffers 
        
    GlobalFree ( lpRecBuff );

    return (TRUE);


}   //---- End of ReceiveMessageNet()




void KillLatePlayers ( void )
{
    int x;
    
    for ( x = 0; x < MAX_PLAYERS; x ++ )
    {

        //---- if the players has not sent a message then kick them out

        if ( ( NetPlayers.NetdpID[x] != 0 )                                       &&
             (( NetPlayers.dwTick[x] + MAX_NET_NOMSG_TICK ) < GetTickCount() )    )
        {
            HRESULT hResult;

//            debugf ( "Kill Player %lu", NetPlayers.NetdpID[x] );

            hResult = lpDP->lpVtbl->DestroyPlayer( lpDP, NetPlayers.NetdpID[x] );

            ErrorDPlay ( hResult );

            NetPlayers.NetdpID[x] = 0;

            memset ( NetPlayers.NetszFriendly[x], 
                     0, 
                     sizeof ( NETFRIENDLY ) );

            memset ( NetPlayers.NetszFormal[x], 
                     0, 
                     sizeof ( NETFORMAL ) );

//            memset ( &NetPlayers.NetThexder[x],
//                     0,
//                     sizeof ( THEXDER ) );


        }

    }

}



//----------------------------------------------------------------------------
//  ErrorDPlay
//
//  Description:
//
//		Sends direct play errors to debug window
//
//  Arguments: Result 
//
//
//  Return : 
//      
//
//----------------------------------------------------------------------------

void ErrorDPlay ( HRESULT hResult )
{

    #if defined ( _DEBUG )



    switch ( hResult )
    {

        case DP_OK:
            {
            break;
            }

        case DPERR_ALREADYINITIALIZED:
            {
//            debugf ( "DPlay Already Initialized" );
            break;
            }

        case DPERR_ACCESSDENIED:
            {
//            debugf ( "DPlay Access Denied" );
            break;
            }

        case DPERR_ACTIVEPLAYERS:
            {
//            debugf ( "DPlay Active Players" );
            break;
            }

        case DPERR_BUFFERTOOSMALL:
            {
//            debugf ( "DPlay Buffer too small" );
            break;
            }

        case DPERR_CANTADDPLAYER:
            {
//            debugf ( "DPlay can't add player" );
            break;
            }

        case DPERR_CANTCREATEGROUP:
            {
//            debugf ( "DPlay can't create group" );
            break;
            }

        case DPERR_CANTCREATEPLAYER:
            {
//            debugf ( "DPlay can't create player" );
            break;
            }

        case DPERR_CANTCREATESESSION:
            {
//            debugf ( "DPlay can't create session" );
            break;
            }

        case DPERR_CAPSNOTAVAILABLEYET:
            {
//            debugf ( "DPlay caps not available yet" );
            break;
            }

        case DPERR_EXCEPTION:
            {
//            debugf ( "DPlay exception" );
            break;
            }

        case DPERR_GENERIC:
            {
//            debugf ( "DPlay generic" );
            break;
            }

        case DPERR_INVALIDFLAGS:
            {
//            debugf ( "DPlay invalid flags" );
            break;
            }

        case DPERR_INVALIDOBJECT:
            {
//            debugf ( "DPlay invalid object" );
            break;
            }

        case DPERR_INVALIDPARAM:
            {
//            debugf ( "DPlay invalid param" );
            break;
            }

        case DPERR_INVALIDPLAYER:
            {
//            debugf ( "DPlay invalid player" );
            break;
            }

        case DPERR_NOCAPS:
            {
//            debugf ( "DPlay no caps" );
            break;
            }

        case DPERR_NOCONNECTION:
            {
//            debugf ( "DPlay no connection" );
            break;
            }

        case DPERR_NOMEMORY:
            {
//            debugf ( "DPlay no memory" );
            break;
            }

        case DPERR_NOMESSAGES:
            {
//            debugf ( "DPlay no messages" );
            break;
            }

        case DPERR_NONAMESERVERFOUND:
            {
//            debugf ( "DPlay no name server found" );
            break;
            }

        case DPERR_NOPLAYERS:
            {
//            debugf ( "DPlay no players" );
            break;
            }

        case DPERR_NOSESSIONS:
            {
//            debugf ( "DPlay no sessions" );
            break;
            }

        case DPERR_SENDTOOBIG:
            {
//            debugf ( "DPlay send too big" );
            break;
            }

        case DPERR_TIMEOUT:
            {
//            debugf ( "DPlay timeout" );
            break;
            }

        case DPERR_UNAVAILABLE:
            {
//            debugf ( "DPlay unavailable" );
            break;
            }

        case DPERR_UNSUPPORTED:
            {
//            debugf ( "DPlay unsupported" );
            break;
            }

        case DPERR_BUSY:
            {
//            debugf ( "DPlay Busy" );
            break;
            }

        case DPERR_USERCANCEL:
            {
//            debugf ( "DPlay User Cancel" );
            break;
            }

        default:
            {
//            debugf ( "Unknown DPlay Error" );
            break;
            }

    }

    #endif



    return;



}   //---- End of ErrorDPlay ()


// ------------------------------------------------------------------------
//
// BOOL DialogJoinWait() 
//
// Description: Dialog for waiting to load waves
//
// Arguments:
//		hwnd 	- window handle
//		message	- window message
//		wParam,
//		 lParam	- message parameters
//
// Return: BOOL
//
// ------------------------------------------------------------------------
BOOL CALLBACK DialogJoinWait( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch ( message )
    {   
        case WM_INITDIALOG:
			CenterInWindow( hwnd, hwndNetApp, TRUE );
			return ( TRUE );
   
        case WM_COMMAND:
            if ( LOWORD(wParam) == IDCANCEL )
			{
				EndDialog( hwnd, TRUE );
				return ( TRUE );
            }
			break;

        default:
            break;
    }

    return ( FALSE );

} // DialogJoinWait



//---- End of DPlay.cpp

