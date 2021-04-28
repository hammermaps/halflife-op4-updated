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

#ifndef GAME_SERVER_ENTITIES_NPCS_CAGRUNT_H
#define GAME_SERVER_ENTITIES_NPCS_CAGRUNT_H

#define	AGRUNT_MELEE_DIST	  100
#define HITGROUP_AGRUNT_ARMOR 10

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_AGRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_AGRUNT_THREAT_DISPLAY,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_AGRUNT_SETUP_HIDE_ATTACK = LAST_COMMON_TASK + 1,
	TASK_AGRUNT_GET_PATH_TO_ENEMY_CORPSE,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

enum
{
	AGRUNT_AE_HORNET1 = 1,
	AGRUNT_AE_HORNET2,
	AGRUNT_AE_HORNET3,
	AGRUNT_AE_HORNET4,
	AGRUNT_AE_HORNET5,
	AGRUNT_AE_PUNCH,
	AGRUNT_AE_BITE,
	AGRUNT_AE_LEFT_FOOT = 10,
	AGRUNT_AE_RIGHT_FOOT,
	AGRUNT_AE_LEFT_PUNCH,
	AGRUNT_AE_RIGHT_PUNCH
};

//LRC - body definitions for the Agrunt model
enum
{
	AGRUNT_BODY_HASGUN,
	AGRUNT_BODY_NOGUN,
};

class CAGrunt : public CSquadMonster
{
public:
	using BaseClass = CSquadMonster;
	
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int  Classify() override;
	int  ISoundMask() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void SetObjectCollisionBox() override
	{
		if (pev->deadflag != DEAD_NO) {
			pev->absmin = pev->origin + Vector(-98, -98, 0);
			pev->absmax = pev->origin + Vector(98, 98, 80);
		}
		else {
			pev->absmin = pev->origin + Vector(-32, -32, 0);
			pev->absmax = pev->origin + Vector(32, 32, 85);
		}
	}

	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;
	BOOL FCanCheckAttacks() override;
	BOOL CheckMeleeAttack1(float flDot, float flDist) override;
	BOOL CheckRangeAttack1(float flDot, float flDist) override;
	void StartTask(Task_t* pTask) override;
	void AlertSound() override;
	void DeathSound() override;
	void PainSound() override;
	void AttackSound();
	void PrescheduleThink() override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	int IRelationship(CBaseEntity* pTarget) override;
	void StopTalking();
	BOOL ShouldSpeak();
	void Killed(entvars_t* pevAttacker, int iGib) override;
	
	CUSTOM_SCHEDULES;

	int		Save(CSave& save) override;
	int		Restore(CRestore& restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	static const char* pAttackSounds[];
	static const char* pDieSounds[];
	static const char* pPainSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];

	bool	m_fCanHornetAttack;
	float	m_flNextHornetAttackCheck;
	float	m_flNextPainTime;

	// three hacky fields for speech stuff. These don't really need to be saved.
	float	m_flNextSpeakTime;
	float	m_flNextWordTime;
	int		m_iLastWord;
	
	int		iAgruntMuzzleFlash;
};

#endif //GAME_SERVER_ENTITIES_NPCS_CAGRUNT_H