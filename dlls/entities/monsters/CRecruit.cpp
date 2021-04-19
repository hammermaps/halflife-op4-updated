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
#include "monsters.h"
#include "talkmonster.h"
#include "schedule.h"
#include "soundent.h"
#include "CRecruit.h"

LINK_ENTITY_TO_CLASS(monster_recruit, CRecruit);

TYPEDESCRIPTION CRecruit::m_SaveData[] =
{
	DEFINE_FIELD(CRecruit, m_painTime, FIELD_TIME),
	DEFINE_FIELD(CRecruit, m_checkAttackTime, FIELD_TIME),
	DEFINE_FIELD(CRecruit, m_flPlayerDamage, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CRecruit, CTalkMonster);

int CRecruit::ISoundMask()
{
	return bits_SOUND_NONE;
}

int CRecruit::Classify()
{
	return m_iClass ? m_iClass : CLASS_HUMAN_MILITARY_FRIENDLY;
}

void CRecruit::SetYawSpeed()
{
	auto speed = 90;

	if (m_Activity != ACT_RUN)
		speed = 70;

	pev->yaw_speed = speed;
}

Schedule_t* CRecruit::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
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

		if (m_hEnemy == nullptr && IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(FALSE);
				break;
			}
			if (HasConditions(bits_COND_CLIENT_PUSH))
			{
				return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
			}
			return GetScheduleOfType(SCHED_TARGET_FACE);
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))
		{
			return GetScheduleOfType(SCHED_MOVE_AWAY);
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return CTalkMonster::GetSchedule();
}

void CRecruit::Killed(entvars_t* pevAttacker, int iGib)
{
	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}

void CRecruit::DeathSound()
{
	switch (RANDOM_LONG(0, 5))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/death1.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/death2.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/death3.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/death4.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/death5.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 5: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/death6.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

void CRecruit::Spawn()
{
	Precache();

	SetModel("models/recruit.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;

	if (pev->health == 0) //LRC
		pev->health = gSkillData.hgruntAllyHealth;

	pev->view_ofs = Vector(0, 0, 50); // position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse(&CTalkMonster::FollowerUse);
}

void CRecruit::PainSound()
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 5))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/gr_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/gr_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/gr_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/gr_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/gr_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 5: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "fgrunt/gr_pain6.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

int CRecruit::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT))
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == nullptr)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing(pevAttacker, pev->origin))
			{
				// Alright, now I'm pissed!
				PlaySentence("RC_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(TRUE);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("RC_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
	}

	return ret;
}

void CRecruit::Precache()
{
	if (pev->model)
		PrecacheModel((char*)STRING(pev->model)); //LRC
	else
		PrecacheModel("models/recruit.mdl");

	PrecacheSound("fgrunt/death1.wav");
	PrecacheSound("fgrunt/death2.wav");
	PrecacheSound("fgrunt/death3.wav");
	PrecacheSound("fgrunt/death4.wav");
	PrecacheSound("fgrunt/death5.wav");
	PrecacheSound("fgrunt/death6.wav");

	PrecacheSound("fgrunt/gr_pain1.wav");
	PrecacheSound("fgrunt/gr_pain2.wav");
	PrecacheSound("fgrunt/gr_pain3.wav");
	PrecacheSound("fgrunt/gr_pain4.wav");
	PrecacheSound("fgrunt/gr_pain5.wav");
	PrecacheSound("fgrunt/gr_pain6.wav");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}