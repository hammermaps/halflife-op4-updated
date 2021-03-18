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

#ifndef CWARPBALL_H
#define CWARPBALL_H

#define SF_WARPBALL_FIRE_ONCE		(1 << 0)
#define SF_WARPBALL_DELAYED_DAMAGE  (1 << 1)

/**
*	@brief Alien teleportation effect
*/
class CWarpBall : public CBaseEntity
{
public:
	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int Classify() override { return CLASS_NONE; }

	void KeyValue(KeyValueData* pkvd) override;

	void Precache() override;
	void Spawn() override;

	void EXPORT WarpBallUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void EXPORT BallThink();

	static CWarpBall* CreateWarpBall(Vector vecOrigin)
	{
		auto warpBall = GetClassPtr<CWarpBall>(nullptr);

		UTIL_SetOrigin(warpBall->pev, vecOrigin);

		warpBall->pev->classname = MAKE_STRING("env_warpball");
		warpBall->Spawn();

		return warpBall;
	}

	CLightning* m_pBeams;
	CSprite* m_pSprite;
	int m_iBeams;
	float m_flLastTime;
	float m_flMaxFrame;
	float m_flBeamRadius;
	string_t m_iszWarpTarget;
	float m_flWarpStart;
	float m_flDamageDelay;
	float m_flTargetDelay;
	BOOL m_fPlaying;
	BOOL m_fDamageApplied;
	BOOL m_fBeamsCleared;
};

#endif // CWARPBALL_H
