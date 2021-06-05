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
#include "skill.h"
#include "gamerules.h"

#include "CWallHealthJar.h"
#include "CWallHealthProp.h"

TYPEDESCRIPTION CWallHealthProp::m_SaveData[] =
{
	DEFINE_FIELD(CWallHealthProp, m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD(CWallHealthProp, m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD(CWallHealthProp, m_iState, FIELD_INTEGER),
	DEFINE_FIELD(CWallHealthProp, m_flSoundTime, FIELD_TIME),
	DEFINE_FIELD(CWallHealthProp, m_jar, FIELD_CLASSPTR),
	DEFINE_FIELD(CWallHealthProp, m_playingChargeSound, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CWallHealthProp, CBaseAnimating)

void CWallHealthProp::Spawn()
{
	Precache();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;

	SetModel("models/health_charger_body.mdl");
	UTIL_SetSize(pev, Vector(-12, -16, 0), Vector(12, 16, 48));
	UTIL_SetOrigin(this, pev->origin);
	m_iJuice = gSkillData.healthchargerCapacity;
	pev->skin = 0;

	m_jar = GetClassPtr((CWallHealthJar*)NULL);
	m_jar->Spawn();
	UTIL_SetOrigin(m_jar, pev->origin);
	m_jar->pev->owner = ENT(pev);
	m_jar->pev->angles = pev->angles;

	InitBoneControllers();

	if (m_iJuice > 0)
	{
		m_iState = Still;
		SetThink(&CWallHealthProp::SearchForPlayer);
		SetNextThink(0.1f);
	}
	else
	{
		m_iState = Inactive;
	}

	//LRC
	if (m_iStyle >= 32) 
		LIGHT_STYLE(m_iStyle, "a");
	else if (m_iStyle <= -32) 
		LIGHT_STYLE(-m_iStyle, "z");
}

LINK_ENTITY_TO_CLASS(item_healthcharger, CWallHealthProp)

void CWallHealthProp::Precache()
{
	PrecacheModel("models/health_charger_body.mdl");
	
	PrecacheSound("items/medshot4.wav");
	PrecacheSound("items/medshotno1.wav");
	PrecacheSound("items/medcharge4.wav");
}

void CWallHealthProp::SearchForPlayer()
{
	CBaseEntity* pEntity = nullptr;
	float delay = 0.05f;
	UTIL_MakeVectors(pev->angles);
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, Center(), PLAYER_SEARCH_RADIUS)) != nullptr) { // this must be in sync with PLAYER_SEARCH_RADIUS from player.cpp
		if (pEntity->IsPlayer()) {
			if (DotProduct(pEntity->pev->origin - pev->origin, gpGlobals->v_forward) < 0) {
				continue;
			}
			
			TurnNeedleToPlayer(pEntity->pev->origin);
			
			switch (m_iState) {
			case RetractShot:
				SetNeedleState(Idle);
				break;
			case RetractArm:
				SetNeedleState(Deploy);
				break;
			case Still:
				SetNeedleState(Deploy);
				delay = 0.1f;
				break;
			case Deploy:
				SetNeedleState(Idle);
				break;
			case Idle:
			default:
				break;
			}
		}
		break;
	}
	
	if (!pEntity || !pEntity->IsPlayer()) {
		switch (m_iState) {
		case Deploy:
		case Idle:
		case RetractShot:
			SetNeedleState(RetractArm);
			delay = 0.2f;
			break;
		case RetractArm:
			SetNeedleState(Still);
			break;
		case Still:
			break;
		default:
			break;
		}
	}
	
	SetNextThink(delay);
}

void CWallHealthProp::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Make sure that we have a caller
	if (!pActivator)
		return;
	// if it's not a player, ignore
	if (!pActivator->IsPlayer())
		return;

	if (m_iState != Idle && m_iState != GiveShot && m_iState != Healing && m_iState != Inactive)
		return;

	// if there is no juice left, turn it off
	if ((m_iState == Healing || m_iState == GiveShot) && m_iJuice <= 0)
	{
		pev->skin = 1;
		SetThink(&CWallHealthProp::Off);
		SetNextThink(0);
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if ((m_iJuice <= 0) || (!(pActivator->pev->weapons & (1 << ITEM_SUIT))) || (pActivator->pev->armorvalue == 100))
	{
		if (m_flSoundTime <= gpGlobals->time)
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM);
		}
		return;
	}

	SetThink(&CWallHealthProp::Off);
	SetNextThink(0.25);

	// Time to recharge yet?
	if (m_flNextCharge >= gpGlobals->time)
		return;

	TurnNeedleToPlayer(pActivator->pev->origin);
	switch (m_iState) {
	case Idle:
		m_flSoundTime = 0.56 + gpGlobals->time;
		SetNeedleState(GiveShot);
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM);
		break;
	case GiveShot:
		SetNeedleState(Healing);
		break;
	case Healing:
		if (!m_playingChargeSound && m_flSoundTime <= gpGlobals->time)
		{
			EMIT_SOUND(ENT(pev), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM);
			m_playingChargeSound = true;
		}
		break;
	default:
		ALERT(at_console, "Unexpected healthcharger state on use: %d\n", m_iState);
		break;
	}

	// charge the player
	if (pActivator->TakeHealth(1, DMG_GENERIC))
	{
		m_iJuice--;
		const float jarBoneControllerValue = (m_iJuice / gSkillData.healthchargerCapacity) * 11 - 11;
		m_jar->SetBoneController(0, jarBoneControllerValue);
	}

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1;
}

void CWallHealthProp::Recharge()
{
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM);
	m_iJuice = gSkillData.healthchargerCapacity;
	m_jar->SetBoneController(0, 0);
	pev->skin = 0;
	//LRC
	if (m_iStyle >= 32) LIGHT_STYLE(m_iStyle, "a");
	else if (m_iStyle <= -32) LIGHT_STYLE(-m_iStyle, "z");
	SetNeedleState(Still);
	SetThink(&CWallHealthProp::SearchForPlayer);
	SetNextThink(0);
}

void CWallHealthProp::Off()
{
	switch (m_iState) {
	case GiveShot:
	case Healing:
		if (m_playingChargeSound) {
			STOP_SOUND(ENT(pev), CHAN_STATIC, "items/medcharge4.wav");
			m_playingChargeSound = false;
		}
		SetNeedleState(RetractShot);
		SetNextThink(0.1f);
		break;
	case RetractShot:
		if (m_iJuice > 0) {
			SetNeedleState(Idle);
			SetThink(&CWallHealthProp::SearchForPlayer);
			SetNextThink(0);
		}
		else {
			SetNeedleState(RetractArm);
			SetNextThink(0.2f);
		}
		break;
	case RetractArm:
	{
		if ((m_iJuice <= 0))
		{
			SetNeedleState(Inactive);
			const float rechargeTime = g_pGameRules->FlHealthChargerRechargeTime();
			if (rechargeTime > 0) {
				SetNextThink(rechargeTime);
				SetThink(&CWallHealthProp::Recharge);
			}

			if (m_iStyle >= 32) LIGHT_STYLE(m_iStyle, "z");
			else if (m_iStyle <= -32) LIGHT_STYLE(-m_iStyle, "a");
		}
		break;
	}
	default:
		break;
	}
}

STATE CWallHealthProp::GetState()
{
	if (m_playingChargeSound)
		return STATE_IN_USE;
	if (m_iState != Healing)
		return STATE_ON;
	
	return STATE_OFF;
}

void CWallHealthProp::SetMySequence(const char* sequence)
{
	pev->sequence = LookupSequence(sequence);
	if (pev->sequence == -1) {
		ALERT(at_error, "unknown sequence: %s\n", sequence);
		pev->sequence = 0;
	}
	pev->frame = 0;
	ResetSequenceInfo();
}

void CWallHealthProp::SetNeedleState(int state)
{
	m_iState = state;
	if (state == RetractArm)
		SetNeedleController(0);
	switch (state) {
	case Still:
		SetMySequence("still");
		break;
	case Deploy:
		SetMySequence("deploy");
		break;
	case Idle:
		SetMySequence("prep_shot");
		break;
	case GiveShot:
		SetMySequence("give_shot");
		break;
	case Healing:
		SetMySequence("shot_idle");
		break;
	case RetractShot:
		SetMySequence("retract_shot");
		break;
	case RetractArm:
		SetMySequence("retract_arm");
		break;
	case Inactive:
		SetMySequence("inactive");
	default:
		break;
	}
}

void CWallHealthProp::TurnNeedleToPlayer(const Vector player)
{
	float yaw = UTIL_VecToYaw(player - pev->origin) - pev->angles.y;

	if (yaw > 180)
		yaw -= 360;
	if (yaw < -180)
		yaw += 360;

	SetNeedleController(yaw);
}

void CWallHealthProp::SetNeedleController(float yaw)
{
	SetBoneController(0, yaw);
}