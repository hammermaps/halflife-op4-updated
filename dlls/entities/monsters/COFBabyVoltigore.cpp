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
#include "schedule.h"
#include "squadmonster.h"
#include "weapons.h"
#include "soundent.h"
#include "COFBabyVoltigore.h"

LINK_ENTITY_TO_CLASS(monster_alien_babyvoltigore, COFBabyVoltigore);

TYPEDESCRIPTION	COFBabyVoltigore::m_SaveData[] =
{
	DEFINE_FIELD(COFBabyVoltigore, m_flNextPainTime, FIELD_TIME),
	DEFINE_FIELD(COFBabyVoltigore, m_flNextSpeakTime, FIELD_TIME),
	DEFINE_FIELD(COFBabyVoltigore, m_flNextWordTime, FIELD_TIME),
	DEFINE_FIELD(COFBabyVoltigore, m_iLastWord, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(COFBabyVoltigore, CSquadMonster);

const char* COFBabyVoltigore::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* COFBabyVoltigore::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* COFBabyVoltigore::pAttackSounds[] =
{
	"voltigore/voltigore_attack_melee1.wav",
	"voltigore/voltigore_attack_melee2.wav",
};

const char* COFBabyVoltigore::pPainSounds[] =
{
	"voltigore/voltigore_pain1.wav",
	"voltigore/voltigore_pain2.wav",
	"voltigore/voltigore_pain3.wav",
	"voltigore/voltigore_pain4.wav",
};

const char* COFBabyVoltigore::pAlertSounds[] =
{
	"voltigore/voltigore_alert1.wav",
	"voltigore/voltigore_alert2.wav",
	"voltigore/voltigore_alert3.wav",
};

//=========================================================
// IRelationship - overridden because Human Grunts are 
// Alien Grunt's nemesis.
//=========================================================
int COFBabyVoltigore::IRelationship(CBaseEntity* pTarget)
{
	if (FClassnameIs(pTarget->pev, "monster_human_grunt"))
	{
		return R_NM;
	}

	return BaseClass::IRelationship(pTarget);
}

//=========================================================
// ISoundMask 
//=========================================================
int COFBabyVoltigore::ISoundMask()
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER |
		bits_SOUND_DANGER;
}

//=========================================================
// TraceAttack
//=========================================================
void COFBabyVoltigore::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	//Ignore shock damage since we have a shock based attack
	//TODO: use a filter based on attacker to identify self harm
	if (!(bitsDamageType & DMG_SHOCK))
	{
		if (ptr->iHitgroup == 10 && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB)))
		{
			// hit armor
			if (pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0, 10) < 1))
			{
				UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
				pev->dmgtime = gpGlobals->time;
			}

			if (RANDOM_LONG(0, 1) == 0)
			{
				Vector vecTracerDir = vecDir;

				vecTracerDir.x += RANDOM_FLOAT(-0.3, 0.3);
				vecTracerDir.y += RANDOM_FLOAT(-0.3, 0.3);
				vecTracerDir.z += RANDOM_FLOAT(-0.3, 0.3);

				vecTracerDir = vecTracerDir * -512;

				MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos);
				WRITE_BYTE(TE_TRACER);
				WRITE_COORD(ptr->vecEndPos.x);
				WRITE_COORD(ptr->vecEndPos.y);
				WRITE_COORD(ptr->vecEndPos.z);

				WRITE_COORD(vecTracerDir.x);
				WRITE_COORD(vecTracerDir.y);
				WRITE_COORD(vecTracerDir.z);
				MESSAGE_END();
			}

			flDamage -= 20;
			if (flDamage <= 0)
				flDamage = 0.1;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
		}
		else
		{
			SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);// a little surface blood.
			TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		}

		AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
	}
}

//=========================================================
// StopTalking - won't speak again for 10-20 seconds.
//=========================================================
void COFBabyVoltigore::StopTalking()
{
	m_flNextWordTime = m_flNextSpeakTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);
}

//=========================================================
// ShouldSpeak - Should this voltigore be talking?
//=========================================================
BOOL COFBabyVoltigore::ShouldSpeak()
{
	if (m_flNextSpeakTime > gpGlobals->time)
	{
		// my time to talk is still in the future.
		return FALSE;
	}

	if (pev->spawnflags & SF_MONSTER_GAG)
	{
		if (m_MonsterState != MONSTERSTATE_COMBAT)
		{
			// if gagged, don't talk outside of combat.
			// if not going to talk because of this, put the talk time 
			// into the future a bit, so we don't talk immediately after 
			// going into combat
			m_flNextSpeakTime = gpGlobals->time + 3;
			return FALSE;
		}
	}

	return TRUE;
}

//=========================================================
// AlertSound
//=========================================================
void COFBabyVoltigore::AlertSound()
{
	StopTalking();

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, 180);
}

//=========================================================
// AttackSound
//=========================================================
void COFBabyVoltigore::AttackSound()
{
	StopTalking();

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackSounds) - 1)], 1.0, ATTN_NORM, 0, 180);
}

//=========================================================
// PainSound
//=========================================================
void COFBabyVoltigore::PainSound()
{
	if (m_flNextPainTime > gpGlobals->time)
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.6;

	StopTalking();

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, 180);
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	COFBabyVoltigore::Classify()
{
	return m_iClass ? m_iClass : CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COFBabyVoltigore::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 80;
		break;
	default:			ys = 70;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void COFBabyVoltigore::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BABYVOLTIGORE_AE_LEFT_FOOT:
	case BABYVOLTIGORE_AE_RIGHT_FOOT:
		switch (RANDOM_LONG(0, 2))
		{
			// left foot
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "voltigore/voltigore_footstep1.wav", 0.8, ATTN_IDLE, 0, 130);	break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "voltigore/voltigore_footstep2.wav", 0.8, ATTN_IDLE, 0, 130);	break;
		case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "voltigore/voltigore_footstep3.wav", 0.8, ATTN_IDLE, 0, 130);	break;
		}
		break;

	case BABYVOLTIGORE_AE_LEFT_PUNCH:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(BABYVOLTIGORE_MELEE_DIST, gSkillData.babyvoltigoreDmgPunch, DMG_CLUB);

		if (pHurt)
		{
			pHurt->pev->punchangle.y = -25;
			pHurt->pev->punchangle.x = 8;

			// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
			if (pHurt->IsPlayer())
			{
				// this is a player. Knock him around.
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 250;
			}

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_IDLE, 0, 130);

			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);
			SpawnBlood(vecArmPos, pHurt->BloodColor(), 25);// a little surface blood.
		}
		else
		{
			// Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_IDLE, 0, 130);
		}
	}
	break;

	case BABYVOLTIGORE_AE_RIGHT_PUNCH:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(BABYVOLTIGORE_MELEE_DIST, gSkillData.babyvoltigoreDmgPunch, DMG_CLUB);

		if (pHurt)
		{
			pHurt->pev->punchangle.y = 25;
			pHurt->pev->punchangle.x = 8;

			// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
			if (pHurt->IsPlayer())
			{
				// this is a player. Knock him around.
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -250;
			}

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_IDLE, 0, 130);

			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);
			SpawnBlood(vecArmPos, pHurt->BloodColor(), 25);// a little surface blood.
		}
		else
		{
			// Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_IDLE, 0, 130);
		}
	}
	break;

	case BABYVOLTIGORE_AE_RUN:
		switch (RANDOM_LONG(0, 1))
		{
			// left foot
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "voltigore/voltigore_run_grunt1.wav", 1, ATTN_NORM, 0, 180);	break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "voltigore/voltigore_run_grunt2.wav", 1, ATTN_NORM, 0, 180);	break;
		}
		break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void COFBabyVoltigore::Spawn()
{
	Precache();

	SetModel("models/baby_voltigore.mdl");

	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 32));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;

	if (pev->health == 0) //LRC
		pev->health = gSkillData.babyvoltigoreHealth;

	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = 0;
	m_afCapability |= bits_CAP_SQUAD | bits_CAP_TURN_HEAD;

	m_HackedGunPos = Vector(24, 64, 48);

	m_flNextSpeakTime = m_flNextWordTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);

	m_fDeathCharge = false;
	m_flDeathStartTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COFBabyVoltigore::Precache()
{
	if (pev->model)
		PrecacheModel((char*)STRING(pev->model)); //LRC
	else
		PrecacheModel("models/baby_voltigore.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);

	PrecacheSound("voltigore/voltigore_attack_shock.wav");

	PrecacheSound("voltigore/voltigore_communicate1.wav");
	PrecacheSound("voltigore/voltigore_communicate2.wav");
	PrecacheSound("voltigore/voltigore_communicate3.wav");

	PrecacheSound("voltigore/voltigore_die1.wav");
	PrecacheSound("voltigore/voltigore_die2.wav");
	PrecacheSound("voltigore/voltigore_die3.wav");

	PrecacheSound("voltigore/voltigore_footstep1.wav");
	PrecacheSound("voltigore/voltigore_footstep2.wav");
	PrecacheSound("voltigore/voltigore_footstep3.wav");

	PrecacheSound("voltigore/voltigore_run_grunt1.wav");
	PrecacheSound("voltigore/voltigore_run_grunt2.wav");

	PrecacheSound("hassault/hw_shoot1.wav");

	PrecacheSound("debris/beamstart2.wav");

	m_iBabyVoltigoreGibs = PrecacheModel("models/vgibs.mdl");
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

extern Schedule_t slVoltigoreFail[];
extern Schedule_t slVoltigoreCombatFail[];
extern Schedule_t slVoltigoreStandoff[];
extern Schedule_t slVoltigoreTakeCoverFromEnemy[];
extern Schedule_t slVoltigoreVictoryDance[];
extern Schedule_t slVoltigoreThreatDisplay[];

DEFINE_CUSTOM_SCHEDULES(COFBabyVoltigore)
{
	slVoltigoreFail,
	slVoltigoreCombatFail,
	slVoltigoreStandoff,
	slVoltigoreTakeCoverFromEnemy,
	slVoltigoreVictoryDance,
	slVoltigoreThreatDisplay,
};

IMPLEMENT_CUSTOM_SCHEDULES(COFBabyVoltigore, CSquadMonster);

//=========================================================
// FCanCheckAttacks - this is overridden for alien grunts
// because they can use their smart weapons against unseen
// enemies. Base class doesn't attack anyone it can't see.
//=========================================================
BOOL COFBabyVoltigore::FCanCheckAttacks()
{
	if (!HasConditions(bits_COND_ENEMY_TOOFAR))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// CheckMeleeAttack1 - alien grunts zap the crap out of 
// any enemy that gets too close. 
//=========================================================
BOOL COFBabyVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= BABYVOLTIGORE_MELEE_DIST && flDot >= 0.6 && m_hEnemy != NULL)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 
//
// !!!LATER - we may want to load balance this. Several
// tracelines are done, so we may not want to do this every
// server frame. Definitely not while firing. 
//=========================================================
BOOL COFBabyVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	return false;
}

//=========================================================
// StartTask
//=========================================================
void COFBabyVoltigore::StartTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_BABYVOLTIGORE_GET_PATH_TO_ENEMY_CORPSE:
	{
		UTIL_MakeVectors(pev->angles);
		if (BuildRoute(m_vecEnemyLKP - gpGlobals->v_forward * 50, bits_MF_TO_LOCATION, NULL))
		{
			TaskComplete();
		}
		else
		{
			ALERT(at_aiconsole, "BabyVoltigoreGetPathToEnemyCorpse failed!!\n");
			TaskFail();
		}
	}
	break;
	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

void COFBabyVoltigore::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_DIE:
	{
		if (m_fSequenceFinished)
		{
			if (pev->frame >= 255)
			{
				pev->deadflag = DEAD_DEAD;

				pev->framerate = 0;

				//Flatten the bounding box so players can step on it
				if (BBoxFlat())
				{
					const auto maxs = Vector(pev->maxs.x, pev->maxs.y, pev->mins.z + 1);
					UTIL_SetSize(pev, pev->mins, maxs);
				}
				else
				{
					UTIL_SetSize(pev, { -4, -4, 0 }, { 4, 4, 1 });
				}

				if (ShouldFadeOnDeath())
				{
					SUB_StartFadeOut();
				}
				else
				{
					CSoundEnt::InsertSound(bits_SOUND_CARCASS, pev->origin, 384, 30.0);
				}
			}
		}
	}
	break;

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t* COFBabyVoltigore::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
		{
			// dangerous sound nearby!
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
		}
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

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		// zap player!
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			AttackSound();// this is a total hack. Should be parto f the schedule
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		if (HasConditions(bits_COND_HEAVY_DAMAGE))
		{
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		if (OccupySlot(bits_SLOT_AGRUNT_CHASE))
		{
			return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}

		return GetScheduleOfType(SCHED_STANDOFF);
	}
	}

	return BaseClass::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* COFBabyVoltigore::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return &slVoltigoreTakeCoverFromEnemy[0];
		break;
	case SCHED_BABYVOLTIGORE_THREAT_DISPLAY:
		return &slVoltigoreThreatDisplay[0];
		break;
	case SCHED_STANDOFF:
		return &slVoltigoreStandoff[0];
		break;
	case SCHED_VICTORY_DANCE:
		return &slVoltigoreVictoryDance[0];
		break;
	case SCHED_FAIL:
		// no fail schedule specified, so pick a good generic one.
	{
		if (m_hEnemy != NULL)
		{
			// I have an enemy
			// !!!LATER - what if this enemy is really far away and i'm chasing him?
			// this schedule will make me stop, face his last known position for 2 
			// seconds, and then try to move again
			return &slVoltigoreCombatFail[0];
		}

		return &slVoltigoreFail[0];
	}
	break;

	}

	return CSquadMonster::GetScheduleOfType(Type);
}

//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount 
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
//
// Used for many contact-range melee attacks. Bites, claws, etc.
//=========================================================
CBaseEntity* COFBabyVoltigore::CheckTraceHullAttack(float flDist, int iDamage, int iDmgType)
{
	TraceResult tr;

	if (IsPlayer())
		UTIL_MakeVectors(pev->angles);
	else
		UTIL_MakeAimVectors(pev->angles);

	Vector vecStart = pev->origin;
	//Don't rescale the Z size for us since we're just a baby
	vecStart.z += pev->size.z;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = Instance(tr.pHit);

		if (iDamage > 0)
		{
			pEntity->TakeDamage(pev, pev, iDamage, iDmgType);
		}

		return pEntity;
	}

	return NULL;
}
