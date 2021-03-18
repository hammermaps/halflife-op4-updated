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

#ifndef CITEMARMORVEST_H
#define CITEMARMORVEST_H

class CItemArmorVest : public CItem
{
	void Spawn() override;
	void Precache() override;
	BOOL MyTouch(CBasePlayer* pPlayer) override;
};

#endif // CITEMARMORVEST_H
