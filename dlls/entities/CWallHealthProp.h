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
#ifndef GAME_SERVER_CWALLHEALTHPROP_H
#define GAME_SERVER_CWALLHEALTHPROP_H

class CWallHealthProp : public CBaseAnimating
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT SearchForPlayer();
	void EXPORT Off();
	void EXPORT Recharge();
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override
	{
		return (CBaseAnimating::ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION;
	}

	void TurnNeedleToPlayer(Vector player);
	void SetNeedleState(int state);
	void SetNeedleController(float yaw);

	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;

	STATE GetState() override;

	static TYPEDESCRIPTION m_SaveData[];

	enum
	{
		Still,
		Deploy,
		Idle,
		GiveShot,
		Healing,
		RetractShot,
		RetractArm,
		Inactive
	};

	float m_flNextCharge;
	int m_iJuice;
	int m_iState;
	float m_flSoundTime;
	CWallHealthJar* m_jar;
	bool m_playingChargeSound;

protected:
	void SetMySequence(const char* sequence);
};

#endif //GAME_SERVER_CWALLHEALTHPROP_H
