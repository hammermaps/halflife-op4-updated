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
#ifndef GAME_SERVER_ENTITIES_CEYESCANNER_H
#define GAME_SERVER_ENTITIES_CEYESCANNER_H

class CEyeScanner : public CBaseMonster
{
public:
	void KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Precache() override;
	void EXPORT PlayBeep();
	void EXPORT WaitForSequenceEnd();
	int ObjectCaps() override { return CBaseMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	int Classify() override;

	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	string_t unlockedTarget;
	string_t lockedTarget;
	string_t unlockerName;
	string_t activatorName;
};

#endif //GAME_SERVER_ENTITIES_CEYESCANNER_H
