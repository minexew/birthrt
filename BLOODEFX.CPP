#include "itemtype.hxx"
#include "blood.hxx"


static LONG ItemEffects::BAHealing(LONG Combo,LONG AbilityStrength,LONG Action)
{
	return OK; //everything went ok	
}


//blood abilities
// 10% more success in diplomacy
static LONG ItemEffects::BABloodMark(LONG Combo,LONG Arg,LONG Action)	
{return 0;}
// 15% more success in diplomacy
static LONG ItemEffects::BADivineAura(LONG Combo,LONG Arg,LONG Action)
{return 0;}

//this one is easy to implement... it doesn't do anything.
static LONG ItemEffects::BAIronWill(LONG Combo,LONG Arg,LONG Action)
{return 0;}


static LONG ItemEffects::BAProtectionEvil(LONG Combo,LONG Arg,LONG Action)
{return 0;}

static LONG ItemEffects::BARegeneration(LONG Combo,LONG Arg,LONG Action)
{
	return OK; //everything went ok	
}


static LONG ItemEffects::BATravel(LONG Combo,LONG Arg,LONG Action)
{
	return 0;
}

static LONG ItemEffects::BAResistance(LONG Combo,LONG Arg,LONG Action)
{
	return OK; //everything went ok	
}



//essentially, MapHighlightSecretDoors
static LONG ItemEffects::BADetectIllusion(LONG Combo,LONG Arg,LONG Action)
{
	return 0;
}



static LONG ItemEffects::BABattleWise(LONG Combo,LONG Arg,LONG Action)
{
	return OK; //everything went ok	
}
/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */

static LONG ItemEffects::BACourage(LONG Combo,LONG Arg,LONG Action)
{
	return OK; //everything went ok	
}
/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */

static LONG ItemEffects::BADevineWrath(LONG Combo,LONG Arg,LONG Action)
{
	return OK; //everything went ok	
}
/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */

static LONG ItemEffects::BAHeightendAbility(LONG Combo,LONG Arg,LONG Action)
{
	return OK; //everything went ok	
}

/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */
static LONG ItemEffects::BAEnhancedSense(LONG Combo,LONG Arg,LONG Action)
{
	return OK; //everything went ok	
}
/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */
static LONG ItemEffects::BAFear(LONG Combo,LONG Arg,LONG Action)
{
	return OK; //everything went ok	
}

