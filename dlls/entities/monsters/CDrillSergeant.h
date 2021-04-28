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

#ifndef GAME_SERVER_ENTITIES_NPCS_CDRILLSERGEANT_H
#define GAME_SERVER_ENTITIES_NPCS_CDRILLSERGEANT_H

class CDrillSergeant : public CTalkMonster
{
public:
	using BaseClass = CTalkMonster;

	int		Save(CSave& save) override;
	int		Restore(CRestore& restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	int ISoundMask() override;
	int Classify() override;
	void SetYawSpeed() override;
	Schedule_t* GetSchedule() override;
	void Killed(entvars_t* pevAttacker, int iGib) override;
	void DeathSound() override;
	void Spawn() override;
	void PainSound() override;
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void Precache() override;
	void TalkInit();
	int ObjectCaps() override;
	
	float m_painTime;
	float m_flPlayerDamage;
};

#endif //GAME_SERVER_ENTITIES_NPCS_CDRILLSERGEANT_H
