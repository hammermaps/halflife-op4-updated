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
#include "customentity.h"
#include "skill.h"
#include "decals.h"
#include "gamerules.h"

#include "CShockBeam.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION	CShockBeam::m_SaveData[] =
{
	DEFINE_FIELD( CShockBeam, m_pBeam1, FIELD_CLASSPTR ),
	DEFINE_FIELD( CShockBeam, m_pBeam2, FIELD_CLASSPTR ),
	DEFINE_FIELD( CShockBeam, m_pSprite, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CShockBeam, CShockBeam::BaseClass );
#endif

LINK_ENTITY_TO_CLASS( shock_beam, CShockBeam );

void CShockBeam::Precache()
{
	PrecacheModel( "sprites/flare3.spr" );
	PrecacheModel( "sprites/lgtning.spr" );
	PrecacheModel( "sprites/glow01.spr" );
	PrecacheModel( "models/shock_effect.mdl" );
	PrecacheSound( "weapons/shock_impact.wav" );
}

void CShockBeam::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SetModel("models/shock_effect.mdl" );

	UTIL_SetOrigin( this, pev->origin );

	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

	SetTouch( &CShockBeam::BallTouch );
	SetThink( &CShockBeam::FlyThink );

	m_pSprite = CSprite::SpriteCreate( "sprites/flare3.spr", pev->origin, true );

	m_pSprite->SetTransparency(kRenderTransAdd, 255, 255, 255, 80, kRenderFxNoDissipation);

	m_pSprite->SetScale( 0.35 );

	m_pSprite->SetAttachment(edict(), 1);

	m_pBeam1 = CBeam::BeamCreate( "sprites/lgtning.spr", 60 );

	if( m_pBeam1 )
	{
		UTIL_SetOrigin( m_pBeam1, pev->origin );

		m_pBeam1->EntsInit( entindex(), entindex() );

		m_pBeam1->SetStartAttachment( 1 );
		m_pBeam1->SetEndAttachment( 2 );
		
		m_pBeam1->SetColor(180, 255, 250);
		m_pBeam1->SetFlags(SF_BEAM_SHADEIN | SF_BEAM_SHADEOUT);
		m_pBeam1->SetBrightness(RANDOM_LONG(18, 20) * 10);
		m_pBeam1->SetNoise(20);
		m_pBeam1->SetScrollRate( 10 );

		if( g_pGameRules->IsMultiplayer() )
		{
			SetNextThink(0.01);
			return;
		}

		m_pBeam2 = CBeam::BeamCreate( "sprites/lgtning.spr", 20 );

		if( m_pBeam2 )
		{
			UTIL_SetOrigin( m_pBeam2, pev->origin );

			m_pBeam2->EntsInit( entindex(), entindex() );

			m_pBeam2->SetStartAttachment( 1 );
			m_pBeam2->SetEndAttachment( 2 );

			m_pBeam2->SetColor(255, 255, 157);
			m_pBeam2->SetFlags(SF_BEAM_SHADEIN | SF_BEAM_SHADEOUT);
			m_pBeam2->SetBrightness(RANDOM_LONG(18, 20) * 10);
			m_pBeam2->SetNoise(30);
			m_pBeam2->SetScrollRate(30);

			SetNextThink(0.01);
		}
	}

	InitBeams();
}

void CShockBeam::FlyThink()
{
	if( pev->waterlevel == WATERLEVEL_HEAD )
	{
		SetThink( &CShockBeam::WaterExplodeThink );
	}

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE(10);     // radius
		WRITE_BYTE(0);		// r
		WRITE_BYTE(255);	// g
		WRITE_BYTE(255);	// b
		WRITE_BYTE(1);     // life * 10
		WRITE_BYTE(0); // decay
	MESSAGE_END();

	ArmBeam(-1);
	ArmBeam(1);
	
	SetNextThink(0.01);
}

void CShockBeam::ExplodeThink()
{
	Explode();
	UTIL_Remove( this );
}

void CShockBeam::WaterExplodeThink()
{
	auto pOwner = VARS( pev->owner );

	Explode();

	::RadiusDamage( pev->origin, pev, pOwner, 100.0, 150.0, CLASS_NONE, DMG_ALWAYSGIB | DMG_BLAST );

	UTIL_Remove( this );
}

void CShockBeam::BallTouch( CBaseEntity* pOther )
{
	SetTouch( nullptr );
	SetThink( nullptr );

	// Do not collide with the owner.
	if (ENT(pOther->pev) == pev->owner)
		return;

	if (UTIL_PointContents(pev->origin) == CONTENT_SKY) {
		if (m_pBeam1) {
			UTIL_Remove(m_pBeam1);
			m_pBeam1 = NULL;
		}

		if (m_pBeam2) {
			UTIL_Remove(m_pBeam2);
			m_pBeam2 = NULL;
		}
		
		UTIL_Remove(m_pSprite);
		m_pSprite = NULL;
		UTIL_Remove(this);
		return;
	}

	if( pOther->pev->takedamage != DAMAGE_NO )
	{
		TraceResult tr = UTIL_GetGlobalTrace();

		ClearMultiDamage();

		const auto damage = g_pGameRules->IsMultiplayer() ? gSkillData.plrDmgShockRoachM : gSkillData.plrDmgShockRoachS;

		auto bitsDamageTypes = DMG_ALWAYSGIB | DMG_SHOCK;

		auto pMonster = pOther->MyMonsterPointer();

		if( pMonster )
		{
			bitsDamageTypes = DMG_BLAST;

			if( pMonster->m_flShockDuration > 1 )
			{
				bitsDamageTypes = DMG_ALWAYSGIB;
			}
		}

		pOther->TraceAttack( VARS( pev->owner ), damage, pev->velocity.Normalize(), &tr, bitsDamageTypes );

		if( pMonster )
		{
			pMonster->AddShockEffect( 63.0, 152.0, 208.0, 16.0, 0.5 );
		}

		ApplyMultiDamage( pev, VARS( pev->owner ) );

		pev->velocity = g_vecZero;
	}

	if( pOther->pev->takedamage == DAMAGE_NO )
	{
		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, edict(), &tr );
		UTIL_DecalTrace( &tr, DECAL_OFSCORCH1 + RANDOM_LONG( 0, 2 ) );
		UTIL_Sparks(tr.vecPlaneNormal);
		
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		g_engfuncs.pfnWriteByte( TE_SPARKS );
		g_engfuncs.pfnWriteCoord( pev->origin.x );
		g_engfuncs.pfnWriteCoord( pev->origin.y );
		g_engfuncs.pfnWriteCoord( pev->origin.z );
		MESSAGE_END();
	}

	SetThink(&CShockBeam::ExplodeThink);
	SetNextThink(0.01);
}

void CShockBeam::Explode()
{
	ClearBeams();
	
	pev->dmg = 40;
	
	// lighting on impact
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE(10);		// radius * 0.1
		WRITE_BYTE(0);		// r
		WRITE_BYTE(255);	// g
		WRITE_BYTE(255);	// b
		WRITE_BYTE(10);		// time * 10
		WRITE_BYTE(10);		// decay * 0.1
	MESSAGE_END();
	
	pev->owner = nullptr;

	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/shock_impact.wav", RANDOM_FLOAT( 0.8, 0.9 ), ATTN_NORM );
}

CShockBeam* CShockBeam::CreateShockBeam( const Vector& vecOrigin, const Vector& vecAngles, CBaseEntity* pOwner )
{
	auto pBeam = GetClassPtr<CShockBeam>( nullptr );

	pBeam->pev->angles = vecAngles;
	pBeam->pev->angles.x = -pBeam->pev->angles.x;

	UTIL_SetOrigin( pBeam, vecOrigin );

	UTIL_MakeVectors( pBeam->pev->angles );

	pBeam->pev->velocity = gpGlobals->v_forward * 2000.0;
	pBeam->pev->velocity.z = -pBeam->pev->velocity.z;

	pBeam->pev->classname = MAKE_STRING( "shock_beam" );

	pBeam->Spawn();

	pBeam->pev->owner = pOwner->edict();

	return pBeam;
}

void CShockBeam::InitBeams()
{
	memset(m_pBeam, 0, sizeof(m_pBeam));

	m_uiBeams = 0;

	pev->skin = 0;
}

void CShockBeam::ClearBeams()
{
	if (m_pSprite)
	{
		UTIL_Remove(m_pSprite);
		m_pSprite = nullptr;
	}

	if (m_pBeam1)
	{
		UTIL_Remove(m_pBeam1);
		m_pBeam1 = nullptr;
	}

	if (m_pBeam2)
	{
		UTIL_Remove(m_pBeam2);
		m_pBeam2 = nullptr;
	}
	
	for (auto& pBeam : m_pBeam)
	{
		if (pBeam)
		{
			UTIL_Remove(pBeam);
			pBeam = nullptr;
		}
	}

	m_uiBeams = 0;
}

void CShockBeam::ArmBeam(int iSide)
{
	//This method is identical to the Alien Slave's ArmBeam, except it treats m_pBeam as a circular buffer.
	if (m_uiBeams >= NUM_BEAMS)
		m_uiBeams = 0;

	TraceResult tr;
	float flDist = 1.0;

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * iSide * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 6; i++)
	{
		Vector vecAim = gpGlobals->v_right * iSide * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1);
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT(pev), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if (flDist == 1.0)
		return;

	//The beam might already exist if we've created all beams before. - Solokiller
	if (!m_pBeam[m_uiBeams])
		m_pBeam[m_uiBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 5);

	if (!m_pBeam[m_uiBeams])
		return;

	auto pHit = Instance(tr.pHit);
	if (pHit && pHit->pev->takedamage != DAMAGE_NO)
	{
		m_pBeam[m_uiBeams]->EntsInit(pHit->entindex(), entindex());
		m_pBeam[m_uiBeams]->SetColor(180, 255, 250);
		m_pBeam[m_uiBeams]->SetBrightness(RANDOM_LONG(14, 18) * 10);
		::RadiusDamage(tr.vecEndPos, pev, VARS(pev->owner), 25, 15, CLASS_NONE,DMG_ENERGYBEAM);
	}
	else
	{
		m_pBeam[m_uiBeams]->PointEntInit(tr.vecEndPos, entindex());
		m_pBeam[m_uiBeams]->SetEndAttachment(iSide < 0 ? 2 : 1);
		m_pBeam[m_uiBeams]->SetColor(180, 255, 250);
		m_pBeam[m_uiBeams]->SetBrightness(RANDOM_LONG(14, 18) * 10);
		m_pBeam[m_uiBeams]->SetNoise(80);
	}

	++m_uiBeams;
}