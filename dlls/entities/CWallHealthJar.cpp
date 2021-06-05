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

#include "CWallHealthJar.h"

void CWallHealthJar::Spawn()
{
	Precache();
	
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;

	SET_MODEL(ENT(pev), "models/health_charger_both.mdl");
	pev->renderamt = 180;
	pev->rendermode = kRenderTransTexture;
	InitBoneControllers();
}

void CWallHealthJar::Precache()
{
	PrecacheModel("models/health_charger_both.mdl");
}
