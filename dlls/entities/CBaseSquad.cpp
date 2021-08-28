/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

#include "CBaseSquad.h"

//=========================================================
// Save/Restore
//=========================================================
TYPEDESCRIPTION	CBaseSquad::m_SaveData[] =
{
	DEFINE_FIELD(CBaseSquad, m_hSquadLeader, FIELD_EHANDLE),
	DEFINE_ARRAY(CBaseSquad, m_hSquadMember, FIELD_EHANDLE, MAX_SQUAD_MEMBERS - 1),
	DEFINE_FIELD(CBaseSquad, m_iMySlot, FIELD_INTEGER),
	DEFINE_FIELD(CBaseSquad, m_fEnemyEluded, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CBaseSquad, CBaseMonster);

//=========================================================
// FValidateCover - determines whether or not the chosen
// cover location is a good one to move to. (currently based
// on proximity to others in the squad)
//=========================================================
bool CBaseSquad::SquadMemberInRange(const Vector& vecLocation, float flDist)
{
	if (!InSquad())
		return false;

	CBaseSquad* pSquadLeader = MySquadLeader();

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CBaseSquad* pSquadMember = pSquadLeader->MySquadMember(i);
		if (pSquadMember && (vecLocation - pSquadMember->pev->origin).Length2D() <= flDist)
			return true;
	}
	
	return false;
}

//=========================================================
// SquadRemove(), remove pRemove from my squad.
// If I am pRemove, promote m_pSquadNext to leader
//=========================================================
void CBaseSquad::SquadRemove(CBaseSquad* pRemove)
{
	ASSERT(pRemove != NULL);
	ASSERT(this->IsLeader());
	ASSERT(pRemove->m_hSquadLeader == this);

	// If I'm the leader, get rid of my squad
	if (pRemove == MySquadLeader())
	{
		for (int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++)
		{
			CBaseSquad* pMember = MySquadMember(i);
			if (pMember)
			{
				pMember->m_hSquadLeader = nullptr;
				m_hSquadMember[i] = nullptr;
			}
		}
	}
	else
	{
		CBaseSquad* pSquadLeader = MySquadLeader();
		if (pSquadLeader)
		{
			for (int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++)
			{
				if (pSquadLeader->m_hSquadMember[i] == pRemove)
				{
					pSquadLeader->m_hSquadMember[i] = nullptr;
					break;
				}
			}
		}
	}

	pRemove->m_hSquadLeader = nullptr;
}

//=========================================================
// SquadAdd(), add pAdd to my squad
//=========================================================
bool CBaseSquad::SquadAdd(CBaseSquad* pAdd)
{
	ASSERT(pAdd != NULL);
	ASSERT(!pAdd->InSquad());
	ASSERT(this->IsLeader());

	for (int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++)
	{
		if (m_hSquadMember[i] == nullptr)
		{
			m_hSquadMember[i] = pAdd;
			pAdd->m_hSquadLeader = this;
			return true;
		}
	}
	
	return false;
}

//=========================================================
// SquadCount(), return the number of members of this squad
// callable from leaders & followers
//=========================================================
int CBaseSquad::SquadCount()
{
	if (!InSquad())
		return 0;

	CBaseSquad* pSquadLeader = MySquadLeader();
	int squadCount = 0;
	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		if (pSquadLeader->MySquadMember(i) != nullptr)
			squadCount++;
	}

	return squadCount;
}

//=========================================================
// SquadRecruit(), get some monsters of my classification and
// link them as a group.  returns the group size
//=========================================================
int CBaseSquad::SquadRecruit(float searchRadius, int maxMembers)
{
	const int iMyClass = Classify();// cache this monster's class

	// Don't recruit if I'm already in a group
	if (InSquad())
		return 0;

	if (maxMembers < 2)
		return 0;

	// I am my own leader
	m_hSquadLeader = this;
	int squadCount = 1;

	CBaseEntity* pEntity = nullptr;
	
	if (!FStringNull(pev->netname))
	{
		// I have a netname, so unconditionally recruit everyone else with that name.
		pEntity = UTIL_FindEntityByString(pEntity, "netname", STRING(pev->netname));
		while (pEntity)
		{
			CBaseSquad* pRecruit = pEntity->MySquadMonsterPointer();

			if (pRecruit)
			{
				if (!pRecruit->InSquad() && pRecruit->Classify() == iMyClass && pRecruit != this)
				{
					// minimum protection here against user error.in worldcraft. 
					if (!SquadAdd(pRecruit))
						break;
					
					squadCount++;
				}
			}

			pEntity = UTIL_FindEntityByString(pEntity,"netname",STRING(pev->netname));
		}
	}
	else
	{
		while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, searchRadius)) != nullptr)
		{
			CBaseSquad* pRecruit = pEntity->MySquadMonsterPointer();

			if (pRecruit && pRecruit != this && pRecruit->IsAlive() && !pRecruit->m_pCine)
			{
				// Can we recruit this guy?
				if (!pRecruit->InSquad() && pRecruit->Classify() == iMyClass &&
					((iMyClass != CLASS_ALIEN_MONSTER) || FStrEq(STRING(pev->classname), STRING(pRecruit->pev->classname))) &&
					FStringNull(pRecruit->pev->netname))
				{
					TraceResult tr;
					UTIL_TraceLine(pev->origin + pev->view_ofs, pRecruit->pev->origin + pev->view_ofs, ignore_monsters, pRecruit->edict(), &tr);// try to hit recruit with a traceline.
					if (tr.flFraction == 1.0f)
					{
						if (!SquadAdd(pRecruit))
							break;

						squadCount++;
					}
				}
			}
		}
	}

	// no single member squads
	if (squadCount == 1)
		m_hSquadLeader = nullptr;

	return squadCount;
}

//=========================================================
// SquadPasteEnemyInfo - called by squad members that have
// current info on the enemy so that it can be stored for 
// members who don't have current info.
//=========================================================
void CBaseSquad::SquadPasteEnemyInfo()
{
	CBaseSquad* pSquadLeader = MySquadLeader();
	if (pSquadLeader)
		pSquadLeader->m_vecEnemyLKP = m_vecEnemyLKP;
}

//=========================================================
// OccupySlot - if any slots of the passed slots are 
// available, the monster will be assigned to one.
//=========================================================
bool CBaseSquad::OccupySlot(int iDesiredSlots)
{
	if (!InSquad())
		return true;

	if (SquadEnemySplit())
	{
		// if the squad members aren't all fighting the same enemy, slots are disabled
		// so that a squad member doesn't get stranded unable to engage his enemy because
		// all of the attack slots are taken by squad members fighting other enemies.
		m_iMySlot = bits_SLOT_SQUAD_SPLIT;
		return true;
	}

	CBaseSquad* pSquadLeader = MySquadLeader();

	if (!(iDesiredSlots ^ pSquadLeader->m_afSquadSlots))
	{
		// none of the desired slots are available. 
		return false;
	}

	const int iSquadSlots = pSquadLeader->m_afSquadSlots;

	for (int i = 0; i < NUM_SLOTS; i++)
	{
		const int iMask = 1 << i;
		if (iDesiredSlots & iMask) // am I looking for this bit?
		{
			if (!(iSquadSlots & iMask))	// Is it already taken?
			{
				// No, use this bit
				pSquadLeader->m_afSquadSlots |= iMask;
				m_iMySlot = iMask;
				//ALERT ( at_aiconsole, "Took slot %d - %d\n", i, m_hSquadLeader->m_afSquadSlots );
				return true;
			}
		}
	}

	return false;
}

//=========================================================
// VacateSlot 
//=========================================================
void CBaseSquad::VacateSlot()
{
	if (m_iMySlot != bits_NO_SLOT && InSquad())
	{
		//ALERT ( at_aiconsole, "Vacated Slot %d - %d\n", m_iMySlot, m_hSquadLeader->m_afSquadSlots );
		MySquadLeader()->m_afSquadSlots &= ~m_iMySlot;
		m_iMySlot = bits_NO_SLOT;
	}
}

//=========================================================
// SquadEnemySplit- returns TRUE if not all squad members
// are fighting the same enemy. 
//=========================================================
bool CBaseSquad::SquadEnemySplit()
{
	if (!InSquad())
		return false;

	CBaseSquad* pSquadLeader = MySquadLeader();
	CBaseEntity* pEnemy = pSquadLeader->m_hEnemy;

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CBaseSquad* pMember = pSquadLeader->MySquadMember(i);
		if (pMember != nullptr && pMember->HasEnemy() && pMember->m_hEnemy != pEnemy)
		{
			return true;
		}
	}
	
	return false;
}

//=========================================================
// SquadCopyEnemyInfo - called by squad members who don't
// have current info on the enemy. Reads from the same fields
// in the leader's data that other squad members write to,
// so the most recent data is always available here.
//=========================================================
void CBaseSquad::SquadCopyEnemyInfo()
{
	CBaseSquad* pSquadLeader = MySquadLeader();
	if (pSquadLeader)
		m_vecEnemyLKP = pSquadLeader->m_vecEnemyLKP;
}

//=========================================================
// 
// SquadMakeEnemy - makes everyone in the squad angry at
// the same entity.
//
//=========================================================
void CBaseSquad::SquadMakeEnemy(CBaseEntity* pEnemy)
{
	if (!InSquad())
		return;

	if (!pEnemy)
	{
		ALERT(at_console, "ERROR: SquadMakeEnemy() - pEnemy is NULL!\n");
		return;
	}

	CBaseSquad* pSquadLeader = MySquadLeader();
	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CBaseSquad* pMember = pSquadLeader->MySquadMember(i);
		if (pMember)
		{
			// reset members who aren't activly engaged in fighting
			if (pMember->m_hEnemy != pEnemy && !pMember->HasConditions(bits_COND_SEE_ENEMY)
				&& (pMember->m_pSchedule && (pMember->m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY))
				// My enemy might be not an enemy for member of my squad, e.g. if I was provoked by player.
				&& pMember->IRelationship(pEnemy) >= R_DL)
			{
				if (pMember->HasEnemy())
				{
					// remember their current enemy
					pMember->PushEnemy(pMember->m_hEnemy, pMember->m_vecEnemyLKP);
				}
				// give them a new enemy
				pMember->m_hEnemy = pEnemy;
				pMember->m_vecEnemyLKP = pEnemy->pev->origin;
				pMember->SetConditions(bits_COND_NEW_ENEMY);
			}
		}
	}
}

//=========================================================
// CheckEnemy
//=========================================================
int CBaseSquad::CheckEnemy(CBaseEntity* pEnemy)
{
	const int iUpdatedLKP = CBaseMonster::CheckEnemy(m_hEnemy);

	// communicate with squad members about the enemy IF this individual has the same enemy as the squad leader.
	if (InSquad() && (CBaseEntity*)m_hEnemy == MySquadLeader()->m_hEnemy)
	{
		if (iUpdatedLKP)
		{
			// have new enemy information, so paste to the squad.
			SquadPasteEnemyInfo();
		}
		else
		{
			// enemy unseen, copy from the squad knowledge.
			SquadCopyEnemyInfo();
		}
	}

	return iUpdatedLKP;
}