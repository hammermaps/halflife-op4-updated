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
#include "weapons.h"
#include "player.h"
#include "items.h"
#include "UserMessages.h"
#include "CItemHelmet.h"

LINK_ENTITY_TO_CLASS(item_helmet, CItemHelmet);

void CItemHelmet::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/Barney_Helmet.mdl");
	CItem::Spawn();
}

void CItemHelmet::Precache()
{
	PRECACHE_MODEL("models/Barney_Helmet.mdl");
	PRECACHE_SOUND("items/gunpickup2.wav");
}

BOOL CItemHelmet::MyTouch(CBasePlayer* pPlayer)
{
	if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) &&
		(pPlayer->pev->weapons & (1 << WEAPON_SUIT)))
	{
		char szcharge[64];

		pPlayer->pev->armorvalue += 40;
		pPlayer->pev->armorvalue = V_min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

		EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

		MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
		WRITE_STRING(STRING(pev->classname));
		MESSAGE_END();


		// Suit reports new power level
		// For some reason this wasn't working in release build -- round it.
		int pct = (int)((float)(pPlayer->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
		pct = (pct / 5);
		if (pct > 0)
			pct--;

		sprintf(szcharge, "!HEV_%1dP", pct);

		//EMIT_SOUND_SUIT(ENT(pev), szcharge);
		pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);
		return TRUE;
	}

	return FALSE;
}