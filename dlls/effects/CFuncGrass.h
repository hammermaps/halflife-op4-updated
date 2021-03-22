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

#ifndef CFUNCGRASS_H
#define CFUNCGRASS_H
#define SF_GRASS_STARTON 1
#define SF_GRASS_NO_PVS_CHECK 2

class CFuncGrass : public CPointEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT	GrassThink();
	void EXPORT	GrassTurnOn();
	void EXPORT	GrassThinkContinous();

	virtual int	Save(CSave& save);
	virtual int	Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];

	static int	ms_iNextFreeKey;

	int		m_iIDNumber;
	bool	m_fGrassActive;
	bool	m_fGrassNoUpdate;
	bool	m_fGrassDeactivatedByPVS;
	bool	m_fGrassDeactivated;
};
#endif // CFUNCGRASS_H
