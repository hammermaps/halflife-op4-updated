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
#ifndef GAME_SERVER_ENTITIES_NPCS_CFLOCKINGFLYER_H
#define GAME_SERVER_ENTITIES_NPCS_CFLOCKINGFLYER_H

constexpr auto AFLOCK_MAX_RECRUIT_RADIUS = 1024;
constexpr auto AFLOCK_FLY_SPEED = 125;
constexpr auto AFLOCK_TURN_RATE = 75;
constexpr auto AFLOCK_ACCELERATE = 10;
constexpr auto AFLOCK_CHECK_DIST = 192;
constexpr auto AFLOCK_TOO_CLOSE = 100;
constexpr auto AFLOCK_TOO_FAR = 256;

class CFlockingFlyer : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SpawnCommonCode();
	void BoidAdvanceFrame();
	void MakeSound();
	void SpreadFlock();
	void SpreadFlock2();
	void Killed(entvars_t* pevAttacker, int iGib) override;
	bool FPathBlocked();

	void EXPORT IdleThink();
	void EXPORT FormFlock();
	void EXPORT Start();
	void EXPORT FlockLeaderThink();
	void EXPORT FlockFollowerThink();
	void EXPORT FallHack();

	int		Save(CSave& save) override;
	int		Restore(CRestore& restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	int IsLeader() { return m_pSquadLeader == this; }
	int	InSquad() { return m_pSquadLeader != nullptr; }
	int	SquadCount();
	void SquadRemove(CFlockingFlyer* pRemove);
	void SquadUnlink();
	void SquadAdd(CFlockingFlyer* pAdd);
	void SquadDisband();

	CFlockingFlyer* m_pSquadLeader;
	CFlockingFlyer* m_pSquadNext;
	
	bool	m_fTurning;// is this boid turning?
	
	Vector	m_vecReferencePoint;// last place we saw leader
	Vector	m_vecAdjustedVelocity;// adjusted velocity (used when fCourseAdjust is TRUE)
	
	float	m_flGoalSpeed;
	float	m_flLastBlockedTime;
	float	m_flFakeBlockedTime;
	float	m_flAlertTime;
	float	m_flFlockNextSoundTime;
};

#endif //GAME_SERVER_ENTITIES_NPCS_CFLOCKINGFLYER_H