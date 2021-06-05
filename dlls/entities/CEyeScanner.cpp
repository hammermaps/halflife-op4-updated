/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "CEyeScanner.h"

TYPEDESCRIPTION CEyeScanner::m_SaveData[] =
{
	DEFINE_FIELD(CEyeScanner, unlockedTarget, FIELD_STRING),
	DEFINE_FIELD(CEyeScanner, lockedTarget, FIELD_STRING),
	DEFINE_FIELD(CEyeScanner, unlockerName, FIELD_STRING),
	DEFINE_FIELD(CEyeScanner, activatorName, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CEyeScanner, CBaseMonster)

LINK_ENTITY_TO_CLASS(item_eyescanner, CEyeScanner)

void CEyeScanner::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "unlocked_target"))
	{
		unlockedTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_target"))
	{
		lockedTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "unlockersname"))
	{
		unlockerName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "reset_delay")) // Dunno if it affects anything in PC version of Decay
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

void CEyeScanner::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;
	pev->takedamage = DAMAGE_NO;
	pev->health = 1;
	pev->weapons = 0;

	SetModel("models/EYE_SCANNER.mdl");
	UTIL_SetOrigin(this, pev->origin);
	UTIL_SetSize(pev, Vector(-12, -16, 0), Vector(12, 16, 48));
	SetActivity(ACT_CROUCHIDLE);
	ResetSequenceInfo();
	SetThink(NULL);
}

void CEyeScanner::Precache()
{
	PrecacheModel("models/EYE_SCANNER.mdl");
	PrecacheSound("buttons/blip1.wav");
	PrecacheSound("buttons/blip2.wav");
	PrecacheSound("buttons/button11.wav");
}

void CEyeScanner::PlayBeep()
{
	pev->skin = pev->weapons % 3 + 1;
	pev->weapons++;
	if (pev->weapons < 10) {
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/blip1.wav", 1, ATTN_NORM);
		SetNextThink(0.125);
	}
	else {
		pev->skin = 0;
		pev->weapons = 0;
		if (FStringNull(unlockerName) || (!FStringNull(activatorName) && FStrEq(STRING(unlockerName), STRING(activatorName)))) {
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/blip2.wav", 1, ATTN_NORM);
			FireTargets(STRING(unlockedTarget), this, this, USE_TOGGLE, 0.0f);
		}
		else {
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/button11.wav", 1, ATTN_NORM);
			FireTargets(STRING(lockedTarget), this, this, USE_TOGGLE, 0.0f);
		}
		activatorName = iStringNull;
		SetActivity(ACT_CROUCH);
		ResetSequenceInfo();
		SetThink(&CEyeScanner::WaitForSequenceEnd);
		SetNextThink(0.1f);
	}
}

void CEyeScanner::WaitForSequenceEnd()
{
	if (m_fSequenceFinished) {
		if (m_Activity == ACT_STAND) {
			SetActivity(ACT_IDLE);
			SetThink(&CEyeScanner::PlayBeep);
			SetNextThink(0);
		}
		else if (m_Activity == ACT_CROUCH) {
			SetActivity(ACT_CROUCHIDLE);
			SetThink(NULL);
		}
		ResetSequenceInfo();
	}
	else {
		StudioFrameAdvance(0.1f);
		SetNextThink(0.1f);
	}
}

void CEyeScanner::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (m_Activity == ACT_CROUCHIDLE) {
		pActivator = pActivator ? pActivator : pCaller;
		activatorName = pActivator ? pActivator->pev->targetname : iStringNull;
		SetActivity(ACT_STAND);
		ResetSequenceInfo();
		SetThink(&CEyeScanner::WaitForSequenceEnd);
		SetNextThink(0.1f);
	}
}

int CEyeScanner::Classify()
{
	return CLASS_NONE;
}
