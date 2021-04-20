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
#ifndef GAME_SERVER_ENTITIES_NPCS_COFBABYVOLTIGORE_H
#define GAME_SERVER_ENTITIES_NPCS_COFBABYVOLTIGORE_H

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_BABYVOLTIGORE_THREAT_DISPLAY = LAST_COMMON_SCHEDULE + 1,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_BABYVOLTIGORE_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BABYVOLTIGORE_AE_LEFT_FOOT	 ( 10 )
#define		BABYVOLTIGORE_AE_RIGHT_FOOT ( 11 )
#define		BABYVOLTIGORE_AE_LEFT_PUNCH ( 12 )
#define		BABYVOLTIGORE_AE_RIGHT_PUNCH ( 13 )
#define		BABYVOLTIGORE_AE_RUN			14

#define		BABYVOLTIGORE_MELEE_DIST	128

class COFBabyVoltigore : public CSquadMonster
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
		pev->absmin = pev->origin + Vector(-16, -16, 0);
		pev->absmax = pev->origin + Vector(16, 16, 32);
	}

	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;
	BOOL FCanCheckAttacks() override;
	BOOL CheckMeleeAttack1(float flDot, float flDist) override;
	BOOL CheckRangeAttack1(float flDot, float flDist) override;
	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;
	void AlertSound() override;
	void AttackSound();
	void PainSound() override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	int IRelationship(CBaseEntity* pTarget) override;
	void StopTalking();
	BOOL ShouldSpeak();

	CBaseEntity* CheckTraceHullAttack(float flDist, int iDamage, int iDmgType);

	CUSTOM_SCHEDULES;

	int		Save(CSave& save) override;
	int		Restore(CRestore& restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	static const char* pAttackSounds[];
	static const char* pPainSounds[];
	static const char* pAlertSounds[];

	float m_flNextPainTime;

	// three hacky fields for speech stuff. These don't really need to be saved.
	float	m_flNextSpeakTime;
	float	m_flNextWordTime;
	int		m_iLastWord;

	int m_iBabyVoltigoreGibs;
	BOOL m_fDeathCharge;
	float m_flDeathStartTime;
};

#endif //GAME_SERVER_ENTITIES_NPCS_COFBABYVOLTIGORE_H
