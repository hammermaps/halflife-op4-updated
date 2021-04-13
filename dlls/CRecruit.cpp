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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"soundent.h"

#define	BARNEY_AE_DRAW		( 2 )

/**
*	@brief A copy of Barney that speaks military
*/
class CRecruit : public CTalkMonster
{
public:
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	int ISoundMask() override;

	int Classify() override;

	void SetYawSpeed() override;

	Schedule_t* GetSchedule() override;

	void Killed( entvars_t *pevAttacker, int iGib ) override;

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType ) override;

	void DeathSound() override;

	void Spawn() override;

	void StartTask( Task_t* pTask ) override;

	void AlertSound() override;

	void RunTask( Task_t *pTask ) override;

	void PainSound() override;

	BOOL CheckRangeAttack1( float flDot, float flDist ) override;

	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType ) override;

	void Precache() override;

	int ObjectCaps() override;

public:
	BOOL m_fGunDrawn;
	float m_painTime;
	float m_checkAttackTime;
	BOOL m_lastAttackCheck;
	float m_flPlayerDamage;
};

LINK_ENTITY_TO_CLASS( monster_recruit, CRecruit );

TYPEDESCRIPTION	CRecruit::m_SaveData[] =
{
	DEFINE_FIELD( CRecruit, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRecruit, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CRecruit, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CRecruit, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRecruit, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CRecruit, CTalkMonster );

int CRecruit::ISoundMask()
{
	return bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE |
		bits_SOUND_DANGER;
}

int CRecruit::Classify()
{
	return m_iClass ? m_iClass : CLASS_HUMAN_MILITARY_FRIENDLY;
}

void CRecruit::SetYawSpeed()
{
	auto speed = 90;

	if( m_Activity != ACT_RUN )
		speed = 70;

	pev->yaw_speed = speed;
}

Schedule_t* CRecruit::GetSchedule()
{
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence( "RC_KILL", 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule();
			}

			// always act surprized with a new enemy
			if( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE ) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );

			// wait for one schedule to draw gun
			if( !m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if( m_hEnemy == NULL && IsFollowing() )
		{
			if( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return CTalkMonster::GetSchedule();
}

void CRecruit::Killed( entvars_t *pevAttacker, int iGib )
{
	SetUse( NULL );
	CTalkMonster::Killed( pevAttacker, iGib );
}

void CRecruit::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	switch( ptr->iHitgroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_BLAST ) )
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10:
		if( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB ) )
		{
			flDamage -= 20;
			if( flDamage <= 0 )
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void CRecruit::DeathSound()
{
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0: EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	case 1: EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	case 2: EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	}
}

void CRecruit::Spawn()
{
	Precache();

	SetModel("models/recruit.mdl" );
	
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;

	if (pev->health == 0) //LRC
		pev->health = gSkillData.barneyHealth;

	pev->view_ofs = Vector( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster
	m_fGunDrawn = FALSE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse( &CTalkMonster::FollowerUse );
}

void CRecruit::StartTask( Task_t* pTask )
{
	CTalkMonster::StartTask( pTask );
}

void CRecruit::AlertSound()
{
	if( m_hEnemy != NULL )
	{
		if( FOkToSpeak() )
		{
			PlaySentence( "RC_ATTACK", RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_IDLE );
		}
	}
}

void CRecruit::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if( m_hEnemy != NULL && ( m_hEnemy->IsPlayer() ) )
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

void CRecruit::PainSound()
{
	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );

	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0: EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	case 1: EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	case 2: EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	}
}

BOOL CRecruit::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDist <= 1024 && flDot >= 0.5 )
	{
		if( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( ( pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin ) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT( pev ), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if( tr.flFraction == 1.0 || ( tr.pHit != NULL && CBaseEntity::Instance( tr.pHit ) == pEnemy ) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}

int CRecruit::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	if( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if( m_MonsterState != MONSTERSTATE_PRONE && ( pevAttacker->flags & FL_CLIENT ) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if( ( m_afMemory & bits_MEMORY_SUSPICIOUS ) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				PlaySentence( "RC_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "RC_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if( !( m_hEnemy->IsPlayer() ) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "RC_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

void CRecruit::Precache()
{
	if (pev->model)
		PrecacheModel((char*)STRING(pev->model)); //LRC
	else
		PrecacheModel( "models/recruit.mdl" );
	
	PrecacheSound( "barney/ba_pain1.wav" );
	PrecacheSound( "barney/ba_pain2.wav" );
	PrecacheSound( "barney/ba_pain3.wav" );

	PrecacheSound( "barney/ba_die1.wav" );
	PrecacheSound( "barney/ba_die2.wav" );
	PrecacheSound( "barney/ba_die3.wav" );

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

int CRecruit::ObjectCaps()
{
	return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;
}
