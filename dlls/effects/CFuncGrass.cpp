/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gamerules.h"
#include "CFuncGrass.h"

#include "UserMessages.h"

//===============================================
// Entity light - Finally it works, we can freely use
// it under the limits, as only a certain number of
// elights are allowed to affect a model at a time.
//===============================================

LINK_ENTITY_TO_CLASS(func_grass, CFuncGrass);

TYPEDESCRIPTION	CFuncGrass::m_SaveData[] =
{
	DEFINE_FIELD(CFuncGrass, m_iIDNumber, FIELD_INTEGER),
	DEFINE_FIELD(CFuncGrass, m_fGrassActive, FIELD_BOOLEAN),
	DEFINE_FIELD(CFuncGrass, m_fGrassDeactivatedByPVS, FIELD_BOOLEAN),
	DEFINE_FIELD(CFuncGrass, m_fGrassDeactivated, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CFuncGrass, CPointEntity);

int CFuncGrass::ms_iNextFreeKey = 1;

void CFuncGrass::Spawn()
{
	pev->solid = SOLID_NOT;// always solid_not 
	SET_MODEL(ENT(pev), STRING(pev->model));
	pev->effects |= EF_NODRAW;

	m_iIDNumber = ms_iNextFreeKey;
	ms_iNextFreeKey++;

	if (FStringNull(pev->targetname))
		pev->spawnflags |= 1;

	pev->solid = SOLID_NOT;	// Remove model & collisions
	
	if (!g_pGameRules->IsMultiplayer())
	{
		pev->nextthink = gpGlobals->time + 1.0;
		SetThink(&CFuncGrass::GrassThink);
	}
}
void CFuncGrass::GrassThink()
{
	if (!(pev->spawnflags & SF_GRASS_STARTON))
	{
		m_fGrassActive = false;
		m_fGrassDeactivated = true;
	}
	else
	{
		m_fGrassActive = true;
		m_fGrassDeactivated = false;
		SetThink(&CFuncGrass::GrassTurnOn);
		pev->nextthink = gpGlobals->time + 0.001;
	}
}

void CFuncGrass::GrassTurnOn()
{
	if (m_fGrassActive)
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgGrassParticles);
			WRITE_SHORT(m_iIDNumber);
			WRITE_BYTE(0);
			WRITE_COORD(pev->absmax.x);
			WRITE_COORD(pev->absmax.y);
			WRITE_COORD(pev->absmax.z);
			WRITE_COORD(pev->absmin.x);
			WRITE_COORD(pev->absmin.y);
			WRITE_COORD(pev->absmin.z);
			WRITE_STRING(STRING(pev->message));
		MESSAGE_END();
		ALERT(at_console, "Activating Grass!!!\n");
	}
	else
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgGrassParticles);
		WRITE_SHORT(m_iIDNumber);
		WRITE_BYTE(1);
		MESSAGE_END();
	}
	
	m_fGrassNoUpdate = true;
	SetThink(&CFuncGrass::GrassThinkContinous);
	pev->nextthink = gpGlobals->time + 0.5;
}

void CFuncGrass::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (m_fGrassDeactivated)
	{
		m_fGrassActive = true;
		m_fGrassDeactivated = false;
		SetThink(&CFuncGrass::GrassTurnOn);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		m_fGrassActive = false;
		m_fGrassDeactivated = true;
		SetThink(&CFuncGrass::GrassTurnOn);
		pev->nextthink = gpGlobals->time + 0.001;
	}
}

void CFuncGrass::GrassThinkContinous()
{
	if (!m_fGrassNoUpdate && m_fGrassActive && !m_fGrassDeactivated)
	{
		SetThink(&CFuncGrass::GrassTurnOn);
		pev->nextthink = gpGlobals->time + 0.001;
		ALERT(at_console, "Activating Grass!!!\n");
	}
	else
	{
		SetThink(&CFuncGrass::GrassThinkContinous);
		pev->nextthink = gpGlobals->time + 0.1;
		ALERT(at_console, "Thinking!!!\n");
	}
}