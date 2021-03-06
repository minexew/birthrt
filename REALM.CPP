/* ========================================================================
   Copyright (c) 1990,1996	Synergistic Software
   All Rights Reserved
   ========================================================================
   Filename: REALM.cpp  -
   Author:   Gary Powell & Alan Clark
   
   ========================================================================
   Contains the following general functions:
   ======================================================================== */
/* ------------------------------------------------------------------------
   Includes
   ------------------------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>

#include "system.h"
#include "realm.hxx"
#include "gamemap.hxx"
#include "regents.hxx"
#include "strenum.h"

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
// name						x	  y siz wid s t  regent				U H P PC court    first RP     
//																					         exist Surp  GB  
DEFINE_VECTOR_DATA_S(REALM_STRUCT,realm,REALM::REALM_COUNT) = {

	{STR_RLM_NAME_CHAOS,		   	0,   0,100,100,0,0,NO_REGENT,			0,0,0,0,MIN_COURT,0,0,0,50,150,20},
	{STR_RLM_NAME_AERENWE,		   	1264,1712, 85,129,1,0,LILIENE_SWORDWAITH,	1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_ALAMIE,		   	 762,1110, 85, 88,1,1,CARILON_ALAM,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_ANUIRE,		   	 546,1566, 10, 10,0,1,CALIEDHE_DOSIERE,	1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_AVANIL,		  	 524,1403, 85,107,1,1,DARIEN_AVAN,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_BARUK_AZHIK,		1644, 916, 65, 92,1,1,GRIMM_GRAYBEARD,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_BOERUINE,			 332, 930, 90,154,1,0,AERIC_BOERUINE,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_BROSENGAE,		 344,1481, 65, 34,1,1,ERIENE_MIERELEN,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_CARIELE,			  918, 619, 80, 46,0,0,ENTIER_GLADANIL,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_CHIMAERON,		1740,1102, 80, 97,0,1,DANITA_CHIMAERA,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_COERANYS,			1559,1191, 80, 75,1,1,ELUVIE_CARIELE,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_DHOESONE,			 814, 310, 85,166,1,0,FHIELE_DHOESONE,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_DIEMED,		 	 620,1635, 80,114,1,1,HEIRL_DIEM,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_ELINIE,		 	1306,1162, 90,124,1,1,ASSAN_IBN_DAOUTA,	1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_ENDIER,		 	 690,1427, 65, 65,1,1,GUILDER_KALIAN,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_FIVE_PEAKS,		 658, 786, 85,137,1,0,THE_WIZARD,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_GHOERE,			1022,1350, 90,202,1,1,GAVIN_TAEL,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_GORGONS_CROWN,	1500, 326, 90,165,1,0,THE_GORGON,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_ILIEN,				 926,1752, 65, 65,1,1,ROGR_AGLONDIER,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_MARKAZOR,			1190, 710, 85,124,1,1,RAZZIK_FANGGRABBER,	1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_MEDOERE,				 815,1642, 65, 40,1,1,SURIS_ENLIEN,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_MHORIED,				1018, 948, 90,130,1,0,DAERIC_MHORIED,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_MIERES,				 508,2074, 90, 90,1,1,UNKNOWN3,			0,0,0,0,MIN_COURT,0,0,0, 0,  0, 0},
	{STR_RLM_NAME_MONSTERS,			   0,   0, 65, 65,1,0,UNKNOWN5,			1,0,0,0,MIN_COURT,1,0,0, 0,150,20},
	{STR_RLM_NAME_MUR_KILAD,			1432, 662, 50, 36,0,1,GODAR_THURINSON,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_OSOERDE,				1376,1438, 85,118,1,1,JAISON_RAENECH,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_RHUOBHE,				 409,1117, 50, 50,0,1,RHUOBHE_THE_ELF,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_ROESONE,				1016,1624, 85, 96,1,1,MARLAE_ROESONE,		1,1,1,0,MIN_COURT,1,0,0,56,150,20},
	{STR_RLM_NAME_SIELWODE,			1437, 800, 80,172,1,0,QUEEN_ISELIE,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_SPIDERFELL,			 820,1470, 60, 60,0,0,THE_SPIDER,		1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_TAEGHAS,				 220,1270, 85, 80,1,1,HARALD_KHORIEN,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_TALINIE,				 381, 652, 90,143,1,0,THURIENE_DONALS,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_THURAZOR,			 680, 576, 85,120,1,0,TIESKAR,			1,1,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_TUARHIEVEL,			1154, 434, 85,107,1,0,FHILERAENE,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_TUORNEN,				 540,1217, 85, 72,1,1,LAELA_FLAERTES,		1,1,1,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_N_IMPORT_AND_EXPORT,		0,   0,100,100,0,0,ADAERE_DONEIM,		0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_IMPREGNABLE_HEART,		0,   0,100,100,0,0,ARMIENDIN,			0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_S_ANUIRE_SHIPPING,		0,   0,100,100,0,0,ORTHIEN_TANE,		0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_HIGH_MAGE_AELIES,			0,   0,100,100,0,0,AELIES,				0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_PORT_OF_CALL_EXCHG,		0,   0,100,100,0,0,EL_HADID,			0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_ORTH_IMP_TEMPLE,			0,   0,100,100,0,0,BRIESEN,			0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_WIZARD_HERMEDHIE,			0,   0,100,100,0,0,HERMEDHIE,			0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_JEWEL_OF_SARIMIE,			0,   0,100,100,0,0,TEMIAS_COUMAIN,		0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_TEMPLE_OF_CUIRAECEN,		0,   0,100,100,0,0,BACCAERE,			0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_UPPER_ANUIRE_TRADERS,		0,   0,100,100,0,0,GAELIN_THURIED,		0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_HAELYNS_AEGIS,			0,   0,100,100,0,0,MARICOERE,			0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_MILITANT_ORDER,			0,   0,100,100,0,0,THE_SWORD,			0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_POINTS_EAST_TRADING,		0,   0,100,100,0,0,ELAMIEN_LAMIER,		0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_SECOND_SWAMP_MAGE,		0,   0,100,100,0,0,THE_SWAMP_MAGE,		0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_N_REFORMED_CHURCH,		0,   0,100,100,0,0,LARRA_NIELEMS,		0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_WIZARD_EYELESS_ONE,		0,   0,100,100,0,0,EYELESS_ONE,		0,0,0,0,MIN_COURT,1,0,0,50,150,20},
	{STR_RLM_NAME_RUARCH_ROCKHAMMER,		0,   0,100,100,0,0,ROCKHAMMER,		0,0,0,0,MIN_COURT,1,0,0,50,150,20},
};

DEFINE_VECTOR_CLASS(REALM_STRUCT,realm);


DEFINE_MATRIX_DATA_S(UBYTE,RealmReaction,REALM::REALM_COUNT,LAND_REALM_COUNT) = {

//									  N              B                                   G                                   S           T     
//									  O              A     B     C                 F     O                                   P           U     
//									                 R  B  R     H  C  D           I     R     M           M  M           S  I        T  A     
//									  C  A           U  O  O  C  I  O  H           V     G     A  M  M     O  U  O  R  R  I  D  T  T  H  R  T  
//									  O  E  A  A  A  K  E  S  A  M  E  O  D  E  E  E  G  O     R  E  H  M  N  R  S  H  O  E  E  A  A  U  H  U  
//									  U  R  L  N  V  A  R  E  R  A  R  E  I  L  N  P  H  N  I  K  D  O  I  S  K  O  U  E  L  R  E  L  R  I  O  
//									  N  E  A  U  A  Z  U  N  I  E  A  S  E  I  D  E  O  S  L  A  O  R  E  T  I  E  O  S  W  F  G  I  A  E  R  
//									  T  N  M  I  N  H  I  G  E  R  N  O  M  N  I  A  E  C  I  Z  E  I  R  E  L  R  B  O  O  E  H  N  Z  V  N  
//									  R  W  I  R  I  I  N  A  L  O  Y  N  E  I  E  K  R  R  E  O  R  E  E  R  E  D  H  N  D  L  A  I  O  E  E  
//									  Y  E  E  E  L  K  E  E  E  N  S  E  D  E  R  S  E  N  N  R  E  D  S  S  D  E  E  E  E  L  S  E  R  L  N  
//									    NG NE LN LN NG LN NE NE CE CG NN LN LG NE CE LE LE NN LE CG CG    CE LE LE NE NN NN CE NN LG LE NN NN  
/* NO_COUNTRY				*/	{99,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50},
/* AERENWE					*/	{50,99,40,50,50,60,50,40,40,40,60,50,50,60,40,40,40,30,50,30,60,60,50,30,30,40,40,70,50,40,50,60,40,50,50},
/* ALAMIE					*/	{50,40,99,50,50,50,50,50,40,50,40,50,50,40,50,50,50,40,50,40,40,40,50,50,40,50,50,50,50,40,50,50,50,50,40},
/* ANUIRE					*/	{50,60,50,99,50,60,50,50,40,60,60,50,50,60,50,50,50,30,50,30,60,60,50,30,30,50,40,50,50,40,50,60,40,50,50},
/* AVANIL					*/	{50,50,40,50,99,50,40,90,40,50,50,50,40,50,50,50,50,40,50,40,50,50,50,30,40,50,40,50,50,40,90,40,50,50,50},
/* BARUKAZHIK				*/	{50,50,50,50,50,99,50,50,40,40,50,60,50,50,50,40,50,30,50,30,50,50,50,30,30,50,40,50,50,40,50,50,50,50,50},
/* BOERUINE					*/	{50,50,70,50,40,50,99,40,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,40,50,40,50,50,40,40,80,50,50,50},
/* BROSENGAE				*/	{50,40,50,50,90,40,50,99,40,50,40,50,50,40,50,50,50,40,50,40,40,40,50,30,40,50,50,50,50,40,50,40,50,50,50},
/* CARIELE					*/	{50,40,40,40,40,40,40,40,99,40,40,40,40,40,40,40,40,30,40,30,40,40,40,30,30,40,40,40,40,40,40,40,40,40,40},
/* CHIMAERON				*/	{50,40,40,40,40,40,40,40,40,99,40,40,40,40,40,40,40,30,40,30,40,40,40,80,30,40,40,40,40,40,40,40,40,40,40},
/* COERANYS					*/	{50,60,40,50,50,70,50,40,40,40,99,50,50,60,40,40,40,30,50,30,60,60,50,30,30,40,40,50,50,40,50,60,40,50,50},
/* DHOESONE					*/	{50,60,40,50,50,60,50,40,40,40,60,99,50,60,40,40,40,30,50,30,60,60,50,30,30,40,40,50,60,40,50,60,70,80,50},
/* DIEMED					*/	{50,50,50,50,40,50,50,50,40,50,50,50,99,50,50,50,50,40,50,40,50,50,50,30,40,50,50,50,50,40,50,50,50,50,50},
/* ELINIE					*/	{50,60,40,50,50,60,50,40,40,40,60,50,50,99,40,40,40,30,50,30,60,60,50,30,30,40,40,50,50,40,50,60,40,50,50},
/* ENDIER					*/	{50,40,50,50,50,40,50,50,40,50,40,50,50,40,99,50,50,40,50,40,40,40,50,30,40,50,50,50,50,40,50,40,50,50,50},
/* FIVEPEAKS				*/	{50,40,40,40,40,40,40,40,40,40,40,40,40,40,40,99,40,30,40,30,40,40,40,80,30,40,40,40,40,40,40,40,40,40,40},
/* GHOERE					*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,99,40,50,40,50,40,50,30,40,50,50,50,50,40,50,50,50,50,50},
/* GORGONSCROWN			*/	{50,30,40,30,30,30,30,40,40,40,30,30,30,30,40,40,40,99,30,90,30,30,30,40,90,40,40,30,30,40,30,30,40,10,30},
/* ILIEN						*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,99,40,70,50,50,30,40,50,50,70,50,40,50,50,50,50,50},
/* MARKAZOR					*/	{50,20,50,20,40,30,40,50,50,50,30,40,40,30,50,50,50,90,40,99,30,30,40,20,50,50,50,40,40,50,40,30,60,40,40},
/* MEDOERE					*/	{50,60,40,50,50,60,50,40,40,40,60,50,40,60,40,40,40,30,70,50,99,60,50,30,30,40,40,70,50,40,50,60,40,50,50},
/* MHORIED					*/	{50,60,40,60,50,60,50,40,60,40,60,50,50,60,40,40,40,30,50,30,60,99,50,30,30,40,40,50,50,40,50,60,40,50,50},
/* MIERES					*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,99,30,40,50,50,50,50,50,50,50,50,50,50},
/* MONSTERS					*/	{50,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,99,30,30,30,30,30,30,30,30,30,30,30},
/* MURKILAD					*/	{50,30,50,40,40,20,40,50,30,50,30,40,40,30,50,50,50,90,40,50,30,30,40,50,99,50,50,40,10,50,40,30,50,40,40},
/* OSOERDE					*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,40,99,50,50,50,50,50,50,50,50,50},
/* RHUOBHE					*/	{50,30,30,30,30,40,30,30,30,30,30,30,30,30,30,30,30,40,30,30,30,30,30,30,30,30,99,30,70,50,30,30,70,30,30},
/* ROESONE					*/	{50,60,40,50,50,50,50,40,40,40,60,50,40,60,40,40,40,40,70,30,70,60,50,30,30,40,40,99,50,40,50,60,40,50,50},
/* SIELWODE					*/	{50,50,50,50,50,50,50,50,40,50,50,60,50,50,50,50,50,30,50,30,50,50,50,30,10,50,60,50,99,40,50,50,40,70,50},
/* SPIDERFELL				*/	{50,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,70,30,30,99,30,30,30,30,30},
/* TAEGHAS					*/	{50,50,40,50,90,50,40,40,40,40,50,50,50,50,40,40,40,30,50,30,50,50,50,30,30,40,50,50,50,40,99,50,40,50,50},
/* TALINIE					*/	{50,50,50,50,40,50,80,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,40,50,50,50,50,40,50,99,50,50,50},
/* THURAZOR					*/	{50,40,40,40,40,40,40,40,40,40,40,70,40,40,40,40,40,50,50,50,40,40,40,30,30,40,40,40,40,40,40,40,99,40,40},
/* TUARHIEVEL				*/	{50,50,50,50,50,50,50,50,40,50,50,80,50,50,50,50,50,10,50,40,50,50,50,30,40,50,60,50,70,40,50,50,50,99,50},
/* TUORNEN					*/	{50,50,40,50,40,50,40,50,40,50,50,50,50,50,50,50,50,30,50,40,50,50,50,30,40,50,40,50,50,40,50,50,50,50,99},
//									  N              B                                   G                                   S           T     
//									  O              A     B     C                 F     O                                   P           U     
//									                 R  B  R     H  C  D           I     R     M           M  M           S  I        T  A     
//									  C  A           U  O  O  C  I  O  H           V     G     A  M  M     O  U  O  R  R  I  D  T  T  H  R  T  
//									  O  E  A  A  A  K  E  S  A  M  E  O  D  E  E  E  G  O     R  E  H  M  N  R  S  H  O  E  E  A  A  U  H  U  
//									  U  R  L  N  V  A  R  E  R  A  R  E  I  L  N  P  H  N  I  K  D  O  I  S  K  O  U  E  L  R  E  L  R  I  O  
//									  N  E  A  U  A  Z  U  N  I  E  A  S  E  I  D  E  O  S  L  A  O  R  E  T  I  E  O  S  W  F  G  I  A  E  R  
//									  T  N  M  I  N  H  I  G  E  R  N  O  M  N  I  A  E  C  I  Z  E  I  R  E  L  R  B  O  O  E  H  N  Z  V  N  
//									  R  W  I  R  I  I  N  A  L  O  Y  N  E  I  E  K  R  R  E  O  R  E  E  R  E  D  H  N  D  L  A  I  O  E  E  
//									  Y  E  E  E  L  K  E  E  E  N  S  E  D  E  R  S  E  N  N  R  E  D  S  S  D  E  E  E  E  L  S  E  R  L  N  
//									    NG NE LN LN NE LN NE NE CE CG NN LN LG NE CE LE LE NN LE CG CG    CE LE LE NE NN NN CE NN LG LE NN NN  
/* N_IMPORTS_EXPORTS		*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* IMPREGNABLE_HEART_OF_*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* S_ANUIRE_SHIPPING_AND*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* HIGH_MAGE_AELIES		*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* PORT_OF_CALL_EXCHANGE*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* ORTHODOX_IMPERIAL_TEM*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,60},
/* WIZARD_HERMEDHIE		*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* CELESTIAL_JEWEL_OF_SA*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* HIDDEN_TEMPLE_OF_CUIR*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* UPPER_ANUIRE_TRADERS	*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* HAELYNS_AEGIS	  	*/		{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* MILITANT_ORDER_OF_CUI*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,60},
/* POINTS_EAST_TRADING_G*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* WIZARD_SECOND_SWAMP_M*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* N_REFORMED_CHURCH_OF_*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* WIZARD_EYELESS_ONE	*/	{50,50,50,50,50,50,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
/* RUARCH_ROCKHAMMER  	*/	{50,50,50,50,50,70,50,50,40,50,50,50,50,50,50,50,50,40,50,40,50,50,50,30,50,50,50,50,50,40,50,50,40,50,50},
};

DEFINE_MATRIX_CLASS_S(UBYTE, RealmReaction, REALM::REALM_COUNT, LAND_REALM_COUNT);

DEFINE_MATRIX_DATA_S(UBYTE,RealmStatus,REALM::REALM_COUNT,LAND_REALM_COUNT) = {
//									  N              B                                   G                                   S           T     
//									  O              A     B     C                 F     O                                   P           U     
//									                 R  B  R     H  C  D           I     R     M           M  M           S  I        T  A     
//									  C  A           U  O  O  C  I  O  H           V     G     A  M  M     O  U  O  R  R  I  D  T  T  H  R  T  
//									  O  E  A  A  A  K  E  S  A  M  E  O  D  E  E  E  G  O     R  E  H  M  N  R  S  H  O  E  E  A  A  U  H  U  
//									  U  R  L  N  V  A  R  E  R  A  R  E  I  L  N  P  H  N  I  K  D  O  I  S  K  O  U  E  L  R  E  L  R  I  O  
//									  N  E  A  U  A  Z  U  N  I  E  A  S  E  I  D  E  O  S  L  A  O  R  E  T  I  E  O  S  W  F  G  I  A  E  R  
//									  T  N  M  I  N  H  I  G  E  R  N  O  M  N  I  A  E  C  I  Z  E  I  R  E  L  R  B  O  O  E  H  N  Z  V  N  
//									  R  W  I  R  I  I  N  A  L  O  Y  N  E  I  E  K  R  R  E  O  R  E  E  R  E  D  H  N  D  L  A  I  O  E  E  
//									  Y  E  E  E  L  K  E  E  E  N  S  E  D  E  R  S  E  N  N  R  E  D  S  S  D  E  E  E  E  L  S  E  R  L  N  
/* NO_COUNTRY				*/	{FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* AERENWE					*/	{NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,NU,NU},
/* ALAMIE					*/	{NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* ANUIRE					*/	{NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* AVANIL					*/	{NU,NU,NU,NU,FA,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU},
/* BARUKAZHIK				*/	{NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* BOERUINE					*/	{NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU},
/* BROSENGAE				*/	{NU,NU,NU,NU,VA,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* CARIELE					*/	{NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* CHIMAERON				*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* COERANYS					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* DHOESONE					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,PA,FA,NU},
/* DIEMED					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* ELINIE					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* ENDIER					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* FIVEPEAKS				*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* GHOERE					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* GORGONSCROWN			*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,FA,NU,NU,NU,PA,FA,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU},
/* ILIEN						*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,PA,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,NU,NU},
/* MARKAZOR					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,VA,NU,FA,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU},
/* MEDOERE					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,PA,NU,FA,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,NU,NU},
/* MHORIED					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* MIERES					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* MONSTERS					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* MURKILAD					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,VA,NU,PA,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* OSOERDE					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* RHUOBHE					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,PA,NU,NU,NU,NU,PA,NU},
/* ROESONE					*/	{NU,PA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,PA,NU,PA,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU},
/* SIELWODE					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,PA,NU,FA,NU,NU,NU,NU,PA,NU},
/* SPIDERFELL				*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU,FA,NU,NU,NU,NU,NU},
/* TAEGHAS					*/	{NU,NU,NU,NU,VA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU},
/* TALINIE					*/	{NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU},
/* THURAZOR					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU},
/* TUARHIEVEL				*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,PA,NU,NU,NU,NU,FA,NU},
/* TUORNEN					*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,FA},
//									  N              B                                   G                                   S           T     
//									  O              A     B     C                 F     O                                   P           U     
//									                 R  B  R     H  C  D           I     R     M           M  M           S  I        T  A     
//									  C  A           U  O  O  C  I  O  H           V     G     A  M  M     O  U  O  R  R  I  D  T  T  H  R  T  
//									  O  E  A  A  A  K  E  S  A  M  E  O  D  E  E  E  G  O     R  E  H  M  N  R  S  H  O  E  E  A  A  U  H  U  
//									  U  R  L  N  V  A  R  E  R  A  R  E  I  L  N  P  H  N  I  K  D  O  I  S  K  O  U  E  L  R  E  L  R  I  O  
//									  N  E  A  U  A  Z  U  N  I  E  A  S  E  I  D  E  O  S  L  A  O  R  E  T  I  E  O  S  W  F  G  I  A  E  R  
//									  T  N  M  I  N  H  I  G  E  R  N  O  M  N  I  A  E  C  I  Z  E  I  R  E  L  R  B  O  O  E  H  N  Z  V  N  
//									  R  W  I  R  I  I  N  A  L  O  Y  N  E  I  E  K  R  R  E  O  R  E  E  R  E  D  H  N  D  L  A  I  O  E  E  
//									  Y  E  E  E  L  K  E  E  E  N  S  E  D  E  R  S  E  N  N  R  E  D  S  S  D  E  E  E  E  L  S  E  R  L  N  
/* N_IMPORTS_EXPORTS		*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* IMPREGNABLE_HEART_OF_*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* S_ANUIRE_SHIPPING_AND*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* HIGH_MAGE_AELIES		*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* PORT_OF_CALL_EXCHANGE*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* ORTHODOX_IMPERIAL_TEM*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* WIZARD_HERMEDHIE		*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* CELESTIAL_JEWEL_OF_SA*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* HIDDEN_TEMPLE_OF_CUIR*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* UPPER_ANUIRE_TRADERS	*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* HAELYNS_AEGIS			*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* MILITANT_ORDER_OF_CUI*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* POINTS_EAST_TRADING_G*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* WIZARD_SECOND_SWAMP_M*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* N_REFORMED_CHURCH_OF_*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* WIZARD_EYELESS_ONE	*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
/* RUARCH_ROCKHAMMER		*/	{NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU,NU},
};

DEFINE_MATRIX_CLASS_S(UBYTE, RealmStatus, REALM::REALM_COUNT, LAND_REALM_COUNT);

char RealmNameBuffer[REALM::REALM_COUNT][22];

/*	======================================================================== */

