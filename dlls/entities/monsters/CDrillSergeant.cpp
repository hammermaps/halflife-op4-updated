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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include "CDrillSergeant.h"

LINK_ENTITY_TO_CLASS(monster_drillsergeant, CDrillSergeant);

TYPEDESCRIPTION	CDrillSergeant::m_SaveData[] =
{
	DEFINE_FIELD(CDrillSergeant, m_painTime, FIELD_TIME),
	DEFINE_FIELD(CDrillSergeant, m_flPlayerDamage, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CDrillSergeant, CTalkMonster);

int CDrillSergeant::ISoundMask()
{
	return bits_SOUND_NONE;
}

int CDrillSergeant::Classify()
{
	return m_iClass ? m_iClass : CLASS_HUMAN_MILITARY_FRIENDLY;
}

void CDrillSergeant::SetYawSpeed()
{
	auto speed = 90;

	if (m_Activity != ACT_RUN)
		speed = 70;

	pev->yaw_speed = speed;
}

Schedule_t* CDrillSergeant::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}
	
	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		PlaySentence("DR_KILL", 4, VOL_NORM, ATTN_NORM);
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		// always act surprized with a new enemy
		if (HasConditions(bits_COND_NEW_ENEMY) && HasConditions(bits_COND_LIGHT_DAMAGE))
			return GetScheduleOfType(SCHED_SMALL_FLINCH);

		if (HasConditions(bits_COND_HEAVY_DAMAGE))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		if (!HasEnemy() && IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(false);
				break;
			}
			else
			{
				if (HasConditions(bits_COND_CLIENT_PUSH))
				{
					return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);
			}
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))
		{
			return GetScheduleOfType(SCHED_MOVE_AWAY);
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return BaseClass::GetSchedule();
}

void CDrillSergeant::Killed(entvars_t* pevAttacker, int iGib)
{
	SetUse(NULL);
	BaseClass::Killed(pevAttacker, iGib);
}

void CDrillSergeant::DeathSound()
{
	switch (RANDOM_LONG(0, 5))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death1.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death2.wav", 1, ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death3.wav", 1, ATTN_IDLE);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death4.wav", 1, ATTN_IDLE);
		break;
	case 4:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death5.wav", 1, ATTN_IDLE);
		break;
	case 5:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death6.wav", 1, ATTN_IDLE);
		break;
	}
}

void CDrillSergeant::Spawn()
{
	Precache();

	SetModel("models/drill.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;

	if (pev->health == 0) //LRC
		pev->health = gSkillData.hgruntAllyHealth;

	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse(&BaseClass::FollowerUse);
}

void CDrillSergeant::PainSound()
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 7))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain3.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain4.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain5.wav", 1, ATTN_NORM);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain1.wav", 1, ATTN_NORM);
		break;
	case 4:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain2.wav", 1, ATTN_NORM);
		break;
	case 5:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain6.wav", 1, ATTN_NORM);
		break;
	}
}

int CDrillSergeant::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = BaseClass::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT))
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (!HasEnemy())
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing(pevAttacker, pev->origin))
			{
				// Alright, now I'm pissed!
				PlaySentence("DR_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(true);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("DR_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			PlaySentence("DR_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}

void CDrillSergeant::Precache()
{
	if (pev->model)
		PrecacheModel((char*)STRING(pev->model)); //LRC
	else
		PrecacheModel("models/drill.mdl");

	PrecacheSound("fgrunt/death1.wav");
	PrecacheSound("fgrunt/death2.wav");
	PrecacheSound("fgrunt/death3.wav");
	PrecacheSound("fgrunt/death4.wav");
	PrecacheSound("fgrunt/death5.wav");
	PrecacheSound("fgrunt/death6.wav");

	PrecacheSound("fgrunt/pain1.wav");
	PrecacheSound("fgrunt/pain2.wav");
	PrecacheSound("fgrunt/pain3.wav");
	PrecacheSound("fgrunt/pain4.wav");
	PrecacheSound("fgrunt/pain5.wav");
	PrecacheSound("fgrunt/pain6.wav");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	BaseClass::Precache();
}

void CDrillSergeant::TalkInit()
{
	BaseClass::TalkInit();
	m_voicePitch = 100;
}

int CDrillSergeant::ObjectCaps()
{
	return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;
}
