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

#ifndef CLIGHTNING_H
#define CLIGHTNING_H

class CLightning : public CBeam
{
public:
	void	Spawn() override;
	void	Precache() override;
	void	KeyValue(KeyValueData* pkvd) override;
	void	Activate() override;

	void	EXPORT StrikeThink();
	void	EXPORT DamageThink();
	void	RandomArea();
	void	RandomPoint(Vector& vecSrc);
	void	Zap(const Vector& vecSrc, const Vector& vecDest);
	void	EXPORT StrikeUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void	EXPORT ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	inline BOOL ServerSide()
	{
		if (m_life == 0 && !(pev->spawnflags & SF_BEAM_RING))
			return TRUE;
		return FALSE;
	}

	int		Save(CSave& save) override;
	int		Restore(CRestore& restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void	BeamUpdateVars();

	static CLightning* LightningCreate(const char* pSpriteName, int width)
	{
		auto lightning = GetClassPtr<CLightning>(nullptr);

		lightning->BeamInit(pSpriteName, width);

		return lightning;
	}

	int		m_active;
	int		m_iszStartEntity;
	int		m_iszEndEntity;
	float	m_life;
	int		m_boltWidth;
	int		m_noiseAmplitude;
	int		m_brightness;
	int		m_speed;
	float	m_restrike;
	int		m_spriteTexture;
	int		m_iszSpriteName;
	int		m_frameStart;

	float	m_radius;
};
#endif // CLIGHTNING_H
