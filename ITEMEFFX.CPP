#include "invntory.hxx"
#include "playstat.hxx"
#include "avatar.hxx"
#include "itemtype.hxx"
#include "tmsgbox.hxx"
#include "dice.h"
#include "strenum.h"
#include "spell.hxx"
#include "blood.hxx"
#include "scnmgr.hxx"

#include "battle.h"
#include "battleui.hxx"
#include "gamemap.hxx"
#include "strenum.h"
#include "parch.hxx"

/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */

static LONG ItemEffects::Generic(LONG Combo,LONG,LONG action)
{	
return 0;}

static LONG ItemEffects::Levitate(LONG Combo,LONG Arg,LONG Action)
{
return 0;}

static LONG ItemEffects::Flight(LONG Combo,LONG Arg,LONG action)
{	
return 0;}



static LONG ItemEffects::RingFeatherFall(LONG Combo,LONG Arg,LONG action)
{
return 0;}
static LONG ItemEffects::FeatherFall(LONG Combo,LONG Arg,LONG action)
{
return 0;}

static LONG ItemEffects::RingInfravision(LONG Combo,LONG Arg,LONG action)
{
return 0;}
	

static LONG ItemEffects::SeeInDark(LONG Combo,LONG Arg,LONG action)
{
return 0;}


static LONG ItemEffects::Speed(LONG Combo,LONG Arg,LONG Action)
{
return 0;}
/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */
static LONG ItemEffects::StrideAndSpring(LONG Combo,LONG Arg,LONG Action)
{
return 0;}

static LONG ItemEffects::KeyExplanation(LONG Combo,LONG Arg,LONG Action)
{
return 0;}

/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */

static LONG ItemEffects::BookOfExaltedDeeds(LONG Combo,LONG Arg,LONG Action)
{
return 0;}
/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */
static LONG ItemEffects::BookOfVileDarkness(LONG Combo,LONG Arg,LONG Action)
{
return 0;}








/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */

static LONG ItemEffects::CloakProtect(LONG Combo,LONG Arg,LONG Action)
{
return 0;}
/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */
static LONG ItemEffects::GauntletsOfDex(LONG Combo,LONG Arg,LONG Action)
{
return 0;}

/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */
static LONG ItemEffects::GuantletsOfOgrePow(LONG Combo,LONG Arg,LONG Action)
{
return 0;}

static ITEMTYPE GetRandomSpellItem(LONG lo,LONG hi)
{
return ITEMTYPE(0);}

static ITEMTYPE GetRandomRealmSpellItem()
{
return ITEMTYPE(0);}



static LONG ItemEffects::TomeAddRandomSpells(LONG Combo,LONG Arg,LONG Action)
{
return 0;}


/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */
static LONG ItemEffects::LibrOfConjuration(LONG Combo,LONG Arg,LONG Action)
{
return 0;}
static LONG ItemEffects::LibrOfDamnation(LONG Combo,LONG Arg,LONG Action)
{
return 0;}
static LONG ItemEffects::LibrSilver(LONG Combo,LONG Arg,LONG Action)
{
return 0;}
/* -----------------------------------------------------------------

   ----------------------------------------------------------------- */



static LONG ItemEffects::Parchment(LONG Combo,LONG Arg,LONG Action)
{
return 0;}



static LONG ItemEffects::GoldCoins(LONG Combo, LONG Arg, LONG Action)
{
return 0;}

static LONG ItemEffects::RingJump(LONG Combo,LONG Arg,LONG Action)
{
return 0;}


static LONG ItemEffects::RingResistance(LONG Combo,LONG Arg,LONG Action)
{
return 0;}

