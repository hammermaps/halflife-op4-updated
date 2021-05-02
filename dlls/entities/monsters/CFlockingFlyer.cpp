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
#include "CFlockingFlyer.h"

TYPEDESCRIPTION CFlockingFlyer::m_SaveData[] =
{
	DEFINE_FIELD(CFlockingFlyer, m_pSquadLeader, FIELD_CLASSPTR),
	DEFINE_FIELD(CFlockingFlyer, m_pSquadNext, FIELD_CLASSPTR),
	DEFINE_FIELD(CFlockingFlyer, m_fTurning, FIELD_BOOLEAN),
	DEFINE_FIELD(CFlockingFlyer, m_vecReferencePoint, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CFlockingFlyer, m_vecAdjustedVelocity, FIELD_VECTOR),
	DEFINE_FIELD(CFlockingFlyer, m_flGoalSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CFlockingFlyer, m_flLastBlockedTime, FIELD_TIME),
	DEFINE_FIELD(CFlockingFlyer, m_flFakeBlockedTime, FIELD_TIME),
	DEFINE_FIELD(CFlockingFlyer, m_flAlertTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CFlockingFlyer, CBaseMonster);

LINK_ENTITY_TO_CLASS(monster_flyer, CFlockingFlyer);

//=========================================================
//=========================================================
void CFlockingFlyer::Spawn()
{
	Precache();
	SpawnCommonCode();

	pev->frame = 0;
	SetNextThink(0.1f);
	SetThink(&CFlockingFlyer::IdleThink);
}

//=========================================================
//=========================================================
void CFlockingFlyer::Precache()
{
	PrecacheModel("models/boid.mdl");

	PrecacheSound("boid/boid_alert1.wav");
	PrecacheSound("boid/boid_alert2.wav");

	PrecacheSound("boid/boid_idle1.wav");
	PrecacheSound("boid/boid_idle2.wav");
}

//=========================================================
//=========================================================
void CFlockingFlyer::MakeSound()
{
	if (m_flAlertTime > gpGlobals->time)
	{
		// make agitated sounds
		switch (RANDOM_LONG(0, 1))
		{
		case 0: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "boid/boid_alert1.wav", 1, ATTN_NORM);
			break;
		case 1: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "boid/boid_alert2.wav", 1, ATTN_NORM);
			break;
		}

		return;
	}

	// make normal sound
	switch (RANDOM_LONG(0, 1))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "boid/boid_idle1.wav", 1, ATTN_NORM);
		break;
	case 1: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "boid/boid_idle2.wav", 1, ATTN_NORM);
		break;
	}
}

//=========================================================
//=========================================================
void CFlockingFlyer::Killed(entvars_t* pevAttacker, int iGib)
{
	CFlockingFlyer* pSquad = m_pSquadLeader;

	while (pSquad)
	{
		pSquad->m_flAlertTime = gpGlobals->time + 15;
		pSquad = pSquad->m_pSquadNext;
	}

	if (m_pSquadLeader)
	{
		m_pSquadLeader->SquadRemove(this);
	}

	pev->deadflag = DEAD_DEAD;

	pev->framerate = 0;
	pev->effects = EF_NOINTERP;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	pev->movetype = MOVETYPE_TOSS;

	ClearShockEffect();

	SetThink(&CFlockingFlyer::FallHack);
	SetNextThink(0.1f);
}

void CFlockingFlyer::FallHack()
{
	if (pev->flags & FL_ONGROUND)
	{
		if (!FClassnameIs(pev->groundentity, "worldspawn"))
		{
			pev->flags &= ~FL_ONGROUND;
			SetNextThink(0.1f);
		}
		else
		{
			pev->velocity = g_vecZero;
			SetThink(NULL);
		}
	}
}

//=========================================================
//=========================================================
void CFlockingFlyer::SpawnCommonCode()
{
	pev->deadflag = DEAD_NO;
	pev->classname = MAKE_STRING("monster_flyer");
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->takedamage = DAMAGE_NO;
	pev->health = 1;

	m_flFieldOfView = 0.2f;

	SetModel("models/boid.mdl");

	UTIL_SetSize(pev, Vector(-5, -5, 0), Vector(5, 5, 2));
}

//=========================================================
//=========================================================
void CFlockingFlyer::BoidAdvanceFrame()
{
	float flapspeed = (pev->speed - pev->armorvalue) / AFLOCK_ACCELERATE;
	pev->armorvalue = pev->armorvalue * 0.8f + pev->speed * 0.2f;

	if (flapspeed < 0) flapspeed = -flapspeed;
	if (flapspeed < 0.25f) flapspeed = 0.25;
	if (flapspeed > 1.9f) flapspeed = 1.9f;

	pev->framerate = flapspeed;

	// lean
	pev->avelocity.x = -(pev->angles.x + flapspeed * 5);

	// bank
	pev->avelocity.z = -(pev->angles.z + pev->avelocity.y);

	StudioFrameAdvance(0.1f);
}

//=========================================================
//=========================================================
void CFlockingFlyer::IdleThink()
{
	SetNextThink(0.2f);

	// see if there's a client in the same pvs as the monster
	if (!FNullEnt(FIND_CLIENT_IN_PVS(edict())))
	{
		SetThink(&CFlockingFlyer::Start);
		SetNextThink(0.1f);
	}
}

//=========================================================
// Start - player enters the pvs, so get things going.
//=========================================================
void CFlockingFlyer::Start()
{
	SetNextThink(0.1f);

	if (IsLeader())
	{
		SetThink(&CFlockingFlyer::FlockLeaderThink);
	}
	else
	{
		SetThink(&CFlockingFlyer::FlockFollowerThink);
	}

	SetActivity(ACT_FLY);
	ResetSequenceInfo();
	BoidAdvanceFrame();

	pev->speed = AFLOCK_FLY_SPEED; // no delay!
}

//=========================================================
// Leader boid calls this to form a flock from surrounding boids
//=========================================================
void CFlockingFlyer::FormFlock()
{
	if (!InSquad())
	{
		// I am my own leader
		m_pSquadLeader = this;
		m_pSquadNext = nullptr;
		int squadCount = 1;

		CBaseEntity* pEntity = nullptr;

		while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, AFLOCK_MAX_RECRUIT_RADIUS)) != nullptr)
		{
			CBaseMonster* pRecruit = pEntity->MyMonsterPointer();

			if (pRecruit && pRecruit != this && pRecruit->IsAlive() && !pRecruit->m_pCine)
			{
				// Can we recruit this guy?
				if (FClassnameIs(pRecruit->pev, "monster_flyer"))
				{
					squadCount++;
					SquadAdd(static_cast<CFlockingFlyer*>(pRecruit));
				}
			}
		}
	}

	SetThink(&CFlockingFlyer::IdleThink); // now that flock is formed, go to idle and wait for a player to come along.
	SetNextThink(0);
}

//=========================================================
// Searches for boids that are too close and pushes them away
//=========================================================
void CFlockingFlyer::SpreadFlock()
{
	CFlockingFlyer* pList = m_pSquadLeader;
	while (pList)
	{
		if (pList != this && (pev->origin - pList->pev->origin).Length() <= AFLOCK_TOO_CLOSE)
		{
			// push the other away
			Vector vecDir = (pList->pev->origin - pev->origin);
			vecDir = vecDir.Normalize();

			// store the magnitude of the other boid's velocity, and normalize it so we
			// can average in a course that points away from the leader.
			const float flSpeed = pList->pev->velocity.Length();
			pList->pev->velocity = pList->pev->velocity.Normalize();
			pList->pev->velocity = (pList->pev->velocity + vecDir) * 0.5;
			pList->pev->velocity = pList->pev->velocity * flSpeed;
		}

		pList = pList->m_pSquadNext;
	}
}

//=========================================================
// Alters the caller's course if he's too close to others 
//
// This function should **ONLY** be called when Caller's velocity is normalized!!
//=========================================================
void CFlockingFlyer::SpreadFlock2()
{
	CFlockingFlyer* pList = m_pSquadLeader;
	while (pList)
	{
		if (pList != this && (pev->origin - pList->pev->origin).Length() <= AFLOCK_TOO_CLOSE)
		{
			Vector vecDir = (pev->origin - pList->pev->origin);
			vecDir = vecDir.Normalize();

			pev->velocity = (pev->velocity + vecDir);
		}

		pList = pList->m_pSquadNext;
	}
}

//=========================================================
// FBoidPathBlocked - returns TRUE if there is an obstacle ahead
//=========================================================
bool CFlockingFlyer::FPathBlocked()
{
	TraceResult tr;

	if (m_flFakeBlockedTime > gpGlobals->time)
	{
		m_flLastBlockedTime = gpGlobals->time;
		return true;
	}

	UTIL_MakeVectors(pev->angles);

	bool fBlocked = false; // assume the way ahead is clear

	// check for obstacle ahead
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * AFLOCK_CHECK_DIST, ignore_monsters, ENT(pev), &tr);
	if (tr.flFraction != 1.0f)
	{
		m_flLastBlockedTime = gpGlobals->time;
		fBlocked = true;
	}

	// extra wide checks
	UTIL_TraceLine(pev->origin + gpGlobals->v_right * 12,
	               pev->origin + gpGlobals->v_right * 12 + gpGlobals->v_forward * AFLOCK_CHECK_DIST, ignore_monsters,
	               ENT(pev), &tr);
	if (tr.flFraction != 1.0f)
	{
		m_flLastBlockedTime = gpGlobals->time;
		fBlocked = true;
	}

	UTIL_TraceLine(pev->origin - gpGlobals->v_right * 12,
	               pev->origin - gpGlobals->v_right * 12 + gpGlobals->v_forward * AFLOCK_CHECK_DIST, ignore_monsters,
	               ENT(pev), &tr);
	if (tr.flFraction != 1.0f)
	{
		m_flLastBlockedTime = gpGlobals->time;
		fBlocked = true;
	}

	if (!fBlocked && gpGlobals->time - m_flLastBlockedTime > 6)
	{
		// not blocked, and it's been a few seconds since we've actually been blocked.
		m_flFakeBlockedTime = gpGlobals->time + RANDOM_LONG(1, 3);
	}

	return fBlocked;
}

//=========================================================
// Leader boids use this think every tenth
//=========================================================
void CFlockingFlyer::FlockLeaderThink()
{
	TraceResult tr;

	SetNextThink(0.1f);

	UTIL_MakeVectors(pev->angles);

	// is the way ahead clear?
	if (!FPathBlocked())
	{
		// if the boid is turning, stop the trend.
		if (m_fTurning)
		{
			m_fTurning = false;
			pev->avelocity.y = 0;
		}

		if (pev->speed <= AFLOCK_FLY_SPEED)
			pev->speed += 5;

		pev->velocity = gpGlobals->v_forward * pev->speed;

		BoidAdvanceFrame();

		return;
	}

	if (!m_fTurning) // something in the way and boid is not already turning to avoid
	{
		// measure clearance on left and right to pick the best dir to turn
		UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_right * AFLOCK_CHECK_DIST, ignore_monsters, ENT(pev),
		               &tr);
		Vector vecDist = (tr.vecEndPos - pev->origin);
		const float flRightSide = vecDist.Length();

		UTIL_TraceLine(pev->origin, pev->origin - gpGlobals->v_right * AFLOCK_CHECK_DIST, ignore_monsters, ENT(pev),
		               &tr);
		vecDist = (tr.vecEndPos - pev->origin);
		const float flLeftSide = vecDist.Length();

		// turn right if more clearance on right side
		if (flRightSide > flLeftSide)
		{
			pev->avelocity.y = -AFLOCK_TURN_RATE;
			m_fTurning = true;
		}
			// default to left turn :)
		else if (flLeftSide > flRightSide)
		{
			pev->avelocity.y = AFLOCK_TURN_RATE;
			m_fTurning = true;
		}
		else
		{
			// equidistant. Pick randomly between left and right.
			m_fTurning = true;

			if (RANDOM_LONG(0, 1) == 0)
			{
				pev->avelocity.y = AFLOCK_TURN_RATE;
			}
			else
			{
				pev->avelocity.y = -AFLOCK_TURN_RATE;
			}
		}
	}

	SpreadFlock();

	pev->velocity = gpGlobals->v_forward * pev->speed;

	// check and make sure we aren't about to plow into the ground, don't let it happen
	UTIL_TraceLine(pev->origin, pev->origin - gpGlobals->v_up * 16, ignore_monsters, ENT(pev), &tr);
	if (tr.flFraction != 1.0f && pev->velocity.z < 0)
		pev->velocity.z = 0;

	// maybe it did, though.
	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		UTIL_SetOrigin(this, pev->origin + Vector(0, 0, 1));
		pev->velocity.z = 0;
	}

	if (m_flFlockNextSoundTime < gpGlobals->time)
	{
		MakeSound();
		m_flFlockNextSoundTime = gpGlobals->time + RANDOM_FLOAT(1, 3);
	}

	BoidAdvanceFrame();
}

//=========================================================
// follower boids execute this code when flocking
//=========================================================
void CFlockingFlyer::FlockFollowerThink()
{
	SetNextThink(0.1f);

	if (IsLeader() || !InSquad())
	{
		// the leader has been killed and this flyer suddenly finds himself the leader. 
		SetThink(&CFlockingFlyer::FlockLeaderThink);
		return;
	}

	Vector vecDirToLeader = (m_pSquadLeader->pev->origin - pev->origin);
	const float flDistToLeader = vecDirToLeader.Length();

	// match heading with leader
	pev->angles = m_pSquadLeader->pev->angles;

	//
	// We can see the leader, so try to catch up to it
	//
	if (FInViewCone(m_pSquadLeader))
	{
		// if we're too far away, speed up
		if (flDistToLeader > AFLOCK_TOO_FAR)
		{
			m_flGoalSpeed = m_pSquadLeader->pev->velocity.Length() * 1.5f;
		}

			// if we're too close, slow down
		else if (flDistToLeader < AFLOCK_TOO_CLOSE)
		{
			m_flGoalSpeed = m_pSquadLeader->pev->velocity.Length() * 0.5f;
		}
	}
	else
	{
		// wait up! the leader isn't out in front, so we slow down to let him pass
		m_flGoalSpeed = m_pSquadLeader->pev->velocity.Length() * 0.5f;
	}

	SpreadFlock2();

	pev->speed = pev->velocity.Length();
	pev->velocity = pev->velocity.Normalize();

	// if we are too far from leader, average a vector towards it into our current velocity
	if (flDistToLeader > AFLOCK_TOO_FAR)
	{
		vecDirToLeader = vecDirToLeader.Normalize();
		pev->velocity = (pev->velocity + vecDirToLeader) * 0.5;
	}

	// clamp speeds and handle acceleration
	if (m_flGoalSpeed > AFLOCK_FLY_SPEED * 2)
	{
		m_flGoalSpeed = AFLOCK_FLY_SPEED * 2;
	}

	if (pev->speed < m_flGoalSpeed)
	{
		pev->speed += AFLOCK_ACCELERATE;
	}
	else if (pev->speed > m_flGoalSpeed)
	{
		pev->speed -= AFLOCK_ACCELERATE;
	}

	pev->velocity = pev->velocity * pev->speed;

	BoidAdvanceFrame();
}

//=========================================================
//
// SquadUnlink(), Unlink the squad pointers.
//
//=========================================================
void CFlockingFlyer::SquadUnlink()
{
	m_pSquadLeader = nullptr;
	m_pSquadNext = nullptr;
}

//=========================================================
//
// SquadAdd(), add pAdd to my squad
//
//=========================================================
void CFlockingFlyer::SquadAdd(CFlockingFlyer* pAdd)
{
	ASSERT(pAdd != NULL);
	ASSERT(!pAdd->InSquad());
	ASSERT(this->IsLeader());

	pAdd->m_pSquadNext = m_pSquadNext;
	m_pSquadNext = pAdd;
	pAdd->m_pSquadLeader = this;
}

//=========================================================
//
// SquadRemove(), remove pRemove from my squad.
// If I am pRemove, promote m_pSquadNext to leader
//
//=========================================================
void CFlockingFlyer::SquadRemove(CFlockingFlyer* pRemove)
{
	ASSERT(pRemove != NULL);
	ASSERT(this->IsLeader());
	ASSERT(pRemove->m_pSquadLeader == this);

	if (SquadCount() > 2)
	{
		// Removing the leader, promote m_pSquadNext to leader
		if (pRemove == this)
		{
			CFlockingFlyer* pLeader = m_pSquadNext;

			// copy the enemy LKP to the new leader
			pLeader->m_vecEnemyLKP = m_vecEnemyLKP;

			if (pLeader)
			{
				CFlockingFlyer* pList = pLeader;

				while (pList)
				{
					pList->m_pSquadLeader = pLeader;
					pList = pList->m_pSquadNext;
				}
			}
			SquadUnlink();
		}
		else // removing a node
		{
			CFlockingFlyer* pList = this;

			// Find the node before pRemove
			while (pList->m_pSquadNext != pRemove)
			{
				// assert to test valid list construction
				ASSERT(pList->m_pSquadNext != NULL);
				pList = pList->m_pSquadNext;
			}
			// List validity
			ASSERT(pList->m_pSquadNext == pRemove);

			// Relink without pRemove
			pList->m_pSquadNext = pRemove->m_pSquadNext;

			// Unlink pRemove
			pRemove->SquadUnlink();
		}
	}
	else
		SquadDisband();
}

//=========================================================
//
// SquadCount(), return the number of members of this squad
// callable from leaders & followers
//
//=========================================================
int CFlockingFlyer::SquadCount()
{
	CFlockingFlyer* pList = m_pSquadLeader;
	int squadCount = 0;
	while (pList)
	{
		squadCount++;
		pList = pList->m_pSquadNext;
	}

	return squadCount;
}

//=========================================================
//
// SquadDisband(), Unlink all squad members
//
//=========================================================
void CFlockingFlyer::SquadDisband()
{
	CFlockingFlyer* pList = m_pSquadLeader;

	while (pList)
	{
		CFlockingFlyer* pNext = pList->m_pSquadNext;
		pList->SquadUnlink();
		pList = pNext;
	}
}
