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
#include "defaultai.h"
#include "weapons.h"
#include "soundent.h"
#include "COtis.h"

LINK_ENTITY_TO_CLASS(monster_otis, COtis);

TYPEDESCRIPTION COtis::m_SaveData[] =
{
	DEFINE_FIELD(COtis, m_fGunDrawn, FIELD_BOOLEAN),
	DEFINE_FIELD(COtis, m_painTime, FIELD_TIME),
	DEFINE_FIELD(COtis, m_checkAttackTime, FIELD_TIME),
	DEFINE_FIELD(COtis, m_lastAttackCheck, FIELD_BOOLEAN),
	DEFINE_FIELD(COtis, m_flPlayerDamage, FIELD_FLOAT),
	DEFINE_FIELD(COtis, m_iOtisBody, FIELD_INTEGER),
	DEFINE_FIELD(COtis, m_iOtisHead, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(COtis, CTalkMonster);

extern Schedule_t slBaFollow[];
extern Schedule_t slBarneyEnemyDraw[];
extern Schedule_t slBaFaceTarget[];
extern Schedule_t slIdleBaStand[];

DEFINE_CUSTOM_SCHEDULES(COtis)
{
	slBaFollow,
	slBarneyEnemyDraw,
	slBaFaceTarget,
	slIdleBaStand,
};

IMPLEMENT_CUSTOM_SCHEDULES(COtis, CTalkMonster);

void COtis::StartTask(Task_t* pTask)
{
	CTalkMonster::StartTask(pTask);
}

void COtis::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		if (HasEnemy() && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask(pTask);
		break;
	default:
		CTalkMonster::RunTask(pTask);
		break;
	}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int COtis::ISoundMask()
{
	return bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int COtis::Classify()
{
	return m_iClass ? m_iClass : CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - otis says "Freeze!"
//=========================================================
void COtis::AlertSound()
{
	if (HasEnemy())
	{
		if (FOkToSpeak())
		{
			PlaySentence("OT_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COtis::SetYawSpeed()
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL COtis::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= 1024 && flDot >= 0.5)
	{
		if (gpGlobals->time > m_checkAttackTime)
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector(0, 0, 55);
			CBaseEntity* pEnemy = m_hEnemy;
			Vector shootTarget = ((pEnemy->BodyTarget(shootOrigin) - pEnemy->pev->origin) + m_vecEnemyLKP);
			UTIL_TraceLine(shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr);
			m_checkAttackTime = gpGlobals->time + 1;
			if (tr.flFraction == 1.0 || (tr.pHit != nullptr && Instance(tr.pHit) == pEnemy))
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		
		return m_lastAttackCheck;
	}
	
	return FALSE;
}


//=========================================================
// OtisFirePistol - shoots one round from the pistol at
// the enemy otis is facing.
//=========================================================
void COtis::OtisFirePistol()
{
	UTIL_MakeVectors(pev->angles);
	Vector vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_PLAYER_357);

	int pitchShift = RANDOM_LONG(0, 20);

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(vecShootOrigin.x); // origin
	WRITE_COORD(vecShootOrigin.y);
	WRITE_COORD(vecShootOrigin.z);
	WRITE_BYTE(20); // radius
	WRITE_BYTE(255); // R
	WRITE_BYTE(255); // G
	WRITE_BYTE(128); // B
	WRITE_BYTE(0); // life * 10
	WRITE_BYTE(0); // decay
	MESSAGE_END();

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/de_shot1.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--; // take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void COtis::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case OTIS_AE_SHOOT:
		OtisFirePistol();
		break;

	case OTIS_AE_DRAW:
		// otis's bodygroup switches here so he can pull gun from holster
		if (GetBodygroup(OtisBodyGroup::Weapons) == OtisWeapon::None)
		{
			SetBodygroup(OtisBodyGroup::Weapons, OtisWeapon::DesertEagle);
			m_iOtisBody = OtisWeapon::DesertEagle;
			m_fGunDrawn = true;
		}

		break;

	case OTIS_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		if (GetBodygroup(OtisBodyGroup::Weapons) == OtisWeapon::DesertEagle)
		{
			SetBodygroup(OtisBodyGroup::Weapons, OtisWeapon::None);
			m_iOtisBody = OtisWeapon::None;
			m_fGunDrawn = false;
		}
		break;

	default:
		CTalkMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void COtis::Spawn()
{
	Precache();

	if (pev->model)
		SetModel(pev->model); //LRC
	else
		SetModel("models/otis.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;

	if (pev->health == 0) //LRC
		pev->health = gSkillData.otisHealth;

	pev->view_ofs = Vector(0, 0, 50); // position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	//Note: This originally didn't use SetBodygroup
	if (m_iOtisHead == OtisHead::Random)
	{
		m_iOtisHead = RANDOM_LONG(0, 1);
	}

	if (m_iOtisBody == OtisWeapon::Random)
	{
		m_iOtisBody = OtisWeapon::None;
	}

	SetBodygroup(OtisBodyGroup::Weapons, m_iOtisBody);
	SetBodygroup(OtisBodyGroup::Heads, m_iOtisHead);

	m_fGunDrawn = m_iOtisBody == OtisWeapon::DesertEagle;

	MonsterInit();
	SetUse(&COtis::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COtis::Precache()
{
	if (pev->model)
		PrecacheModel((char*)STRING(pev->model)); //LRC
	else
		PrecacheModel("models/otis.mdl");

	PrecacheSound("barney/ba_attack1.wav");
	PrecacheSound("barney/ba_attack2.wav");

	PrecacheSound("weapons/de_shot1.wav");

	PrecacheSound("barney/ba_pain1.wav");
	PrecacheSound("barney/ba_pain2.wav");
	PrecacheSound("barney/ba_pain3.wav");

	PrecacheSound("barney/ba_die1.wav");
	PrecacheSound("barney/ba_die2.wav");
	PrecacheSound("barney/ba_die3.wav");

	// every new otis must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

// Init talk data
void COtis::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "OT_ANSWER";
	m_szGrp[TLK_QUESTION] = "OT_QUESTION";
	m_szGrp[TLK_IDLE] = "OT_IDLE";
	m_szGrp[TLK_STARE] = "OT_STARE";
	m_szGrp[TLK_USE] = "OT_OK";
	m_szGrp[TLK_UNUSE] = "OT_WAIT";
	m_szGrp[TLK_STOP] = "OT_STOP";

	m_szGrp[TLK_NOSHOOT] = "OT_SCARED";
	m_szGrp[TLK_HELLO] = "OT_HELLO";

	m_szGrp[TLK_PLHURT1] = "!BA_CUREA";
	m_szGrp[TLK_PLHURT2] = "!BA_CUREB";
	m_szGrp[TLK_PLHURT3] = "!BA_CUREC";

	m_szGrp[TLK_PHELLO] = nullptr; //"OT_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = nullptr; //"OT_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "OT_PQUEST"; // UNDONE

	m_szGrp[TLK_SMELL] = "OT_SMELL";

	m_szGrp[TLK_WOUND] = "OT_WOUND";
	m_szGrp[TLK_MORTAL] = "OT_MORTAL";

	// get voice for head - just one otis voice for now
	m_voicePitch = 100;
}

int COtis::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
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
		if (!HasEnemy())
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing(pevAttacker, pev->origin))
			{
				// Alright, now I'm pissed!
				PlaySentence("OT_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(true);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("OT_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			PlaySentence("OT_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}


//=========================================================
// PainSound
//=========================================================
void COtis::PainSound()
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void COtis::DeathSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

void COtis::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
		//TODO: Otis doesn't have a helmet, probably don't want his dome being bulletproof
	case 10:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet(ptr->vecEndPos, 1.0);
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

void COtis::Killed(entvars_t* pevAttacker, int iGib)
{
	if (GetBodygroup(OtisBodyGroup::Weapons) == OtisWeapon::DesertEagle)
	{
		// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup(OtisBodyGroup::Weapons, OtisWeapon::None);
		m_iOtisBody = OtisWeapon::None;

		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity* pGun = DropItem("weapon_eagle", vecGunPos, vecGunAngles);
	}

	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Schedule_t* COtis::GetScheduleOfType(int Type)
{
	Schedule_t* psched;

	switch (Type)
	{
	case SCHED_ARM_WEAPON:
		if (HasEnemy())
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;

		// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that otis will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBaFaceTarget; // override this for different target face behavior
		return psched;

	case SCHED_TARGET_CHASE:
		return slBaFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		return psched;
	}

	return CTalkMonster::GetScheduleOfType(Type);
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t* COtis::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}
	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		PlaySentence("OT_KILL", 4, VOL_NORM, ATTN_NORM);
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

			// wait for one schedule to draw gun
			if (!m_fGunDrawn)
				return GetScheduleOfType(SCHED_ARM_WEAPON);

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

MONSTERSTATE COtis::GetIdealState()
{
	return CTalkMonster::GetIdealState();
}

void COtis::DeclineFollowing()
{
	PlaySentence("OT_POK", 2, VOL_NORM, ATTN_NORM);
}

void COtis::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_iOtisHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("bodystate", pkvd->szKeyName))
	{
		m_iOtisBody = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CBaseMonster::KeyValue(pkvd);
	}
}