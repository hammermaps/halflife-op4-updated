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
#ifndef GAME_SERVER_ENTITIES_NPCS_COTIS_H
#define GAME_SERVER_ENTITIES_NPCS_COTIS_H

constexpr auto OTIS_AE_DRAW    = (2);
constexpr auto OTIS_AE_SHOOT   = (3);
constexpr auto OTIS_AE_HOLSTER = (4);

enum
{
	OTIS_BODY_GUNHOLSTERED = 0,
	OTIS_BODY_GUNDRAWN,
	OTIS_BODY_GUNGONE,
};

namespace OtisBodyGroup
{
	enum OtisBodyGroup
	{
		Weapons = 1,
		Heads = 2
	};
}

namespace OtisWeapon
{
	enum OtisWeapon
	{
		Random = -1,
		None = 0,
		DesertEagle,
		Donut
	};
}

namespace OtisHead
{
	enum OtisHead
	{
		Random = -1,
		Hair = 0,
		Balding
	};
}

class COtis : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int ISoundMask() override;
	void OtisFirePistol();
	void AlertSound() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	void RunTask(Task_t* pTask) override;
	void StartTask(Task_t* pTask) override;
	int ObjectCaps() override { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	BOOL CheckRangeAttack1(float flDot, float flDist) override;

	void DeclineFollowing() override;

	// Override these to set behavior
	Schedule_t* GetScheduleOfType(int Type) override;
	Schedule_t* GetSchedule() override;
	MONSTERSTATE GetIdealState() override;

	void DeathSound() override;
	void PainSound() override;

	void TalkInit() override;

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr,
	                 int bitsDamageType) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;

	void KeyValue(KeyValueData* pkvd) override;

	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fGunDrawn;
	float m_painTime;
	float m_checkAttackTime;
	BOOL m_lastAttackCheck;

	//These were originally used to store off the setting AND track state,
	//but state is now tracked by calling GetBodygroup
	int m_iOtisBody;
	int m_iOtisHead;

	// UNDONE: What is this for?  It isn't used?
	float m_flPlayerDamage; // how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

#endif //GAME_SERVER_ENTITIES_NPCS_COTIS_H
