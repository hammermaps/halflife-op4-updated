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
#include "effects.h"
#include "weapons.h"
#include "effects/CLightning.h"

#include "CWarpBall.h"

LINK_ENTITY_TO_CLASS(env_warpball, CWarpBall);

TYPEDESCRIPTION	CWarpBall::m_SaveData[] =
{
	DEFINE_FIELD(CWarpBall, m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(CWarpBall, m_flLastTime, FIELD_FLOAT),
	DEFINE_FIELD(CWarpBall, m_flMaxFrame, FIELD_FLOAT),
	DEFINE_FIELD(CWarpBall, m_flBeamRadius, FIELD_FLOAT),
	DEFINE_FIELD(CWarpBall, m_iszWarpTarget, FIELD_STRING),
	DEFINE_FIELD(CWarpBall, m_flWarpStart, FIELD_FLOAT),
	DEFINE_FIELD(CWarpBall, m_flDamageDelay, FIELD_FLOAT),
	DEFINE_FIELD(CWarpBall, m_flTargetDelay, FIELD_FLOAT),
	DEFINE_FIELD(CWarpBall, m_fPlaying, FIELD_BOOLEAN),
	DEFINE_FIELD(CWarpBall, m_fDamageApplied, FIELD_BOOLEAN),
	DEFINE_FIELD(CWarpBall, m_fBeamsCleared, FIELD_BOOLEAN),
	DEFINE_FIELD(CWarpBall, m_pBeams, FIELD_CLASSPTR),
	DEFINE_FIELD(CWarpBall, m_pSprite, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CWarpBall, CBaseEntity);

void CWarpBall::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("radius", pkvd->szKeyName))
	{
		m_flBeamRadius = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("warp_target", pkvd->szKeyName))
	{
		m_iszWarpTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("damage_delay", pkvd->szKeyName))
	{
		m_flDamageDelay = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		pkvd->fHandled = false;
	}
}

void CWarpBall::Precache()
{
	PRECACHE_MODEL("sprites/Fexplo1.spr");
	PRECACHE_MODEL("sprites/XFlare1.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("debris/alien_teleport.wav");
}

void CWarpBall::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	pev->rendermode = kRenderGlow;
	pev->renderamt = 255;
	pev->renderfx = kRenderFxNoDissipation;
	pev->framerate = 10;

	m_pSprite = CSprite::SpriteCreate("sprites/Fexplo1.spr", pev->origin, true);
	m_pSprite->TurnOff();

	SetUse(&CWarpBall::WarpBallUse);
}

void CWarpBall::WarpBallUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!m_fPlaying)
	{
		if (!FStringNull(m_iszWarpTarget))
		{
			auto targetEntity = g_engfuncs.pfnFindEntityByString(0, "targetname", STRING(m_iszWarpTarget));
			if (targetEntity)
				UTIL_SetOrigin(pev, targetEntity->v.origin);
		}

		SET_MODEL(pev->pContainingEntity, "sprites/XFlare1.spr");

		m_flMaxFrame = MODEL_FRAMES(pev->modelindex) - 1;

		pev->rendercolor.x = 77;
		pev->rendercolor.y = 210;
		pev->rendercolor.z = 130;
		pev->scale = 1.2;
		pev->frame = 0;

		if (m_pSprite)
		{
			m_pSprite->pev->rendermode = kRenderGlow;
			m_pSprite->pev->rendercolor.x = 77;
			m_pSprite->pev->rendercolor.y = 210;
			m_pSprite->pev->rendercolor.z = 130;
			m_pSprite->pev->renderamt = 255;
			m_pSprite->pev->renderfx = kRenderFxNoDissipation;
			m_pSprite->pev->scale = 1;
			m_pSprite->pev->framerate = 10;
			m_pSprite->TurnOn();
		}

		if (!m_pBeams)
		{
			m_pBeams = CLightning::LightningCreate("sprites/lgtning.spr", 18);

			m_pBeams->m_iszSpriteName = MAKE_STRING("sprites/lgtning.spr");

			m_pBeams->pev->origin = pev->origin;
			UTIL_SetOrigin(m_pBeams->pev, pev->origin);

			m_pBeams->m_restrike = -0.5;
			m_pBeams->m_noiseAmplitude = 65;
			m_pBeams->m_boltWidth = 18;
			m_pBeams->m_life = 0.5;

			m_pBeams->pev->rendercolor.x = 0;
			m_pBeams->pev->rendercolor.y = 255;
			m_pBeams->pev->rendercolor.z = 0;

			m_pBeams->pev->spawnflags |= 0x20u;
			m_pBeams->pev->spawnflags |= 2u;

			m_pBeams->m_radius = m_flBeamRadius;
			m_pBeams->m_iszStartEntity = pev->targetname;

			m_pBeams->BeamUpdateVars();
		}

		if (m_pBeams)
		{
			m_pBeams->pev->solid = 0;
			m_pBeams->Precache();
			m_pBeams->SetThink(&CLightning::StrikeThink);
			m_pBeams->pev->nextthink = gpGlobals->time + 0.1;
		}

		SetThink(&CWarpBall::BallThink);
		pev->nextthink = gpGlobals->time + 0.1;

		m_flLastTime = gpGlobals->time;
		m_fBeamsCleared = 0;
		m_fPlaying = true;

		if (m_flDamageDelay == 0)
		{
			::RadiusDamage(pev->origin, pev, pev, 300, 48, CLASS_NONE, DMG_SHOCK);
			m_fDamageApplied = true;
		}
		else
		{
			m_fDamageApplied = false;
		}

		SUB_UseTargets(this, USE_TOGGLE, 0);

		UTIL_ScreenShake(pev->origin, 4, 100, 2, 1000);

		m_flWarpStart = gpGlobals->time;

		EMIT_SOUND(edict(), CHAN_WEAPON, "debris/alien_teleport.wav", VOL_NORM, ATTN_NORM);
	}
}

void CWarpBall::BallThink()
{
	pev->frame = ((gpGlobals->time - m_flLastTime) * pev->framerate) + pev->frame;

	if (pev->frame > m_flMaxFrame)
	{
		SET_MODEL(edict(), "");

		SetThink(nullptr);

		if (pev->spawnflags & SF_WARPBALL_FIRE_ONCE)
			UTIL_Remove(this);

		if (m_pSprite)
			m_pSprite->TurnOff();

		m_fPlaying = false;
	}
	else
	{
		//TODO: this flag is probably supposed to be a "do radius damage" flag, but it isn't used in the Use method
		if (pev->spawnflags & SF_WARPBALL_DELAYED_DAMAGE
			&& !m_fDamageApplied
			&& (gpGlobals->time - m_flWarpStart) >= m_flDamageDelay)
		{
			::RadiusDamage(pev->origin, pev, pev, 300, 48, CLASS_NONE, DMG_SHOCK);
			m_fDamageApplied = true;
		}

		if (m_pBeams)
		{
			if (pev->frame >= (m_flMaxFrame - 4.0))
			{
				m_pBeams->SetThink(nullptr);
				m_pBeams->pev->nextthink = gpGlobals->time;
			}
		}

		pev->nextthink = gpGlobals->time + 0.1;
		m_flLastTime = gpGlobals->time;
	}
}