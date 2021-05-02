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
#ifndef GAME_SERVER_ENTITIES_CBASESQUAD_H
#define GAME_SERVER_ENTITIES_CBASESQUAD_H

constexpr auto MAX_SQUAD_MEMBERS = 5;

constexpr auto NUM_SLOTS = 11; // update this every time you add/remove a slot.

constexpr auto bits_NO_SLOT = 0;

// global slots
constexpr auto bits_SLOT_SQUAD_SPLIT = (1 << 10); // squad members don't all have the same enemy

class CBaseSquad : public CBaseMonster
{
public:
	using BaseClass = CBaseMonster;

	EHANDLE m_hSquadLeader = {}; // who is my leader
	EHANDLE m_hSquadMember[MAX_SQUAD_MEMBERS - 1] = {}; // valid only for leader

	int m_afSquadSlots;
	int m_iMySlot; // this is the behaviour slot that the monster currently holds in the squad. 
	bool m_fEnemyEluded;
	
	static TYPEDESCRIPTION m_SaveData[];

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);

	// squad functions still left in base class
	virtual CBaseSquad* MySquadLeader()
	{
		const auto pSquadLeader = static_cast<CBaseSquad*>(static_cast<CBaseEntity*>(m_hSquadLeader));
		if (pSquadLeader != nullptr)
			return pSquadLeader;
		
		return this;
	}

	virtual CBaseSquad* MySquadMember(int i)
	{
		if (i >= MAX_SQUAD_MEMBERS - 1)
			return this;
		
		return static_cast<CBaseSquad*>(static_cast<CBaseEntity*>(m_hSquadMember[i]));
	}

	virtual bool InSquad() { return m_hSquadLeader != nullptr; }
	virtual bool IsLeader() { return m_hSquadLeader == this; }
	bool SquadMemberInRange(const Vector& vecLocation, float flDist);
	void SquadRemove(CBaseSquad* pRemove);
	bool SquadAdd(CBaseSquad* pAdd);
	int SquadCount();
	int SquadRecruit(float searchRadius = 1024, int maxMembers = 4);
	void SquadPasteEnemyInfo();
	bool OccupySlot(int iDesiredSlot);
	void VacateSlot();
	bool SquadEnemySplit();
	void SquadCopyEnemyInfo();
};

#endif //GAME_SERVER_ENTITIES_CBASESQUAD_H