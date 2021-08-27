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
//=========================================================
// CSquadMonster - all the extra data for monsters that
// form squads.
//=========================================================

#ifndef SQUADMONSTER_H
#define SQUADMONSTER_H

#define	SF_SQUADMONSTER_LEADER	32

#include "entities/CBaseSquad.h"

// HUMAN GRUNT SLOTS
#define bits_SLOT_HGRUNT_ENGAGE1	( 1 << 0 )
#define bits_SLOT_HGRUNT_ENGAGE2	( 1 << 1 )
#define bits_SLOTS_HGRUNT_ENGAGE	( bits_SLOT_HGRUNT_ENGAGE1 | bits_SLOT_HGRUNT_ENGAGE2 )

#define bits_SLOT_HGRUNT_GRENADE1	( 1 << 2 )
#define bits_SLOT_HGRUNT_GRENADE2	( 1 << 3 )
#define bits_SLOTS_HGRUNT_GRENADE	( bits_SLOT_HGRUNT_GRENADE1 | bits_SLOT_HGRUNT_GRENADE2 )

// ALIEN GRUNT SLOTS
#define bits_SLOT_AGRUNT_HORNET1	( 1 << 4 )
#define bits_SLOT_AGRUNT_HORNET2	( 1 << 5 )
#define bits_SLOT_AGRUNT_CHASE		( 1 << 6 )
#define bits_SLOTS_AGRUNT_HORNET	( bits_SLOT_AGRUNT_HORNET1 | bits_SLOT_AGRUNT_HORNET2 )

// HOUNDEYE SLOTS
#define bits_SLOT_HOUND_ATTACK1		( 1 << 7 )
#define bits_SLOT_HOUND_ATTACK2		( 1 << 8 )
#define bits_SLOT_HOUND_ATTACK3		( 1 << 9 )
#define bits_SLOTS_HOUND_ATTACK		( bits_SLOT_HOUND_ATTACK1 | bits_SLOT_HOUND_ATTACK2 | bits_SLOT_HOUND_ATTACK3 )

//=========================================================
// CSquadMonster - for any monster that forms squads.
//=========================================================
class CSquadMonster : public CBaseSquad
{
public:
	using BaseClass = CBaseSquad;

	// squad leader info
	int m_afSquadSlots;
	float m_flLastEnemySightTime; // last time anyone in the squad saw the enemy

	void StartMonster() override;
	void ScheduleChange() override;
	void Killed(entvars_t* pevAttacker, int iGib) override;
	bool NoFriendlyFire() override;
	bool NoFriendlyFire(bool playerAlly);

	// squad functions still left in base class
	CSquadMonster* MySquadLeader() override
	{
		CSquadMonster* pSquadLeader = static_cast<CSquadMonster*>(static_cast<CBaseEntity*>(m_hSquadLeader));
		if (pSquadLeader != nullptr)
			return pSquadLeader;
		return this;
	}

	CSquadMonster* MySquadMember(int i) override
	{
		if (i >= MAX_SQUAD_MEMBERS - 1)
			return this;
		return static_cast<CSquadMonster*>(static_cast<CBaseEntity*>(m_hSquadMember[i]));
	}

	CSquadMonster* MySquadMonsterPointer() override { return this; }

	static TYPEDESCRIPTION m_SaveData[];

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	BOOL FValidateCover(const Vector& vecCoverLocation) override;

	MONSTERSTATE GetIdealState() override;
	Schedule_t* GetScheduleOfType(int iType) override;

	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
};

#endif // SQUADMONSTER_H