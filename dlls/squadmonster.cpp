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
//=========================================================
// Squadmonster  functions
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"
#include "entities/CBaseSquad.h"
#include "squadmonster.h"
#include "plane.h"

//=========================================================
// Save/Restore
//=========================================================
TYPEDESCRIPTION	CSquadMonster::m_SaveData[] = 
{
	DEFINE_FIELD( CSquadMonster, m_flLastEnemySightTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CSquadMonster, CBaseMonster );

//=========================================================
// ScheduleChange
//=========================================================
void CSquadMonster :: ScheduleChange ()
{
	VacateSlot();
}

//=========================================================
// Killed
//=========================================================
void CSquadMonster :: Killed( entvars_t *pevAttacker, int iGib )
{
	VacateSlot();

	if ( InSquad() )
	{
		MySquadLeader()->SquadRemove( this );
	}

	CBaseMonster :: Killed ( pevAttacker, iGib );
}

//=========================================================
// 
// SquadMakeEnemy - makes everyone in the squad angry at
// the same entity.
//
//=========================================================
void CSquadMonster :: SquadMakeEnemy ( CBaseEntity *pEnemy )
{
	if (!InSquad())
		return;

	if ( !pEnemy )
	{
		ALERT ( at_console, "ERROR: SquadMakeEnemy() - pEnemy is NULL!\n" );
		return;
	}

	CBaseSquad*pSquadLeader = MySquadLeader( );
	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CBaseSquad*pMember = pSquadLeader->MySquadMember(i);
		if (pMember)
		{
			// reset members who aren't activly engaged in fighting
			if (pMember->m_hEnemy != pEnemy && !pMember->HasConditions( bits_COND_SEE_ENEMY))
			{
				if ( pMember->m_hEnemy != NULL) 
				{
					// remember their current enemy
					pMember->PushEnemy( pMember->m_hEnemy, pMember->m_vecEnemyLKP );
				}
				// give them a new enemy
				pMember->m_hEnemy = pEnemy;
				pMember->m_vecEnemyLKP = pEnemy->pev->origin;
				pMember->SetConditions ( bits_COND_NEW_ENEMY );
			}
		}
	}
}

//=========================================================
// CheckEnemy
//=========================================================
int CSquadMonster :: CheckEnemy ( CBaseEntity *pEnemy )
{
	int iUpdatedLKP = CBaseMonster::CheckEnemy(m_hEnemy);
	
	// communicate with squad members about the enemy IF this individual has the same enemy as the squad leader.
	if ( InSquad() && (CBaseEntity *)m_hEnemy == MySquadLeader()->m_hEnemy )
	{
		if ( iUpdatedLKP )
		{
			// have new enemy information, so paste to the squad.
			SquadPasteEnemyInfo();
		}
		else
		{
			// enemy unseen, copy from the squad knowledge.
			SquadCopyEnemyInfo();
		}
	}

	return iUpdatedLKP;
}

//=========================================================
// StartMonster
//=========================================================
void CSquadMonster :: StartMonster()
{
	CBaseMonster :: StartMonster();

	if ( ( m_afCapability & bits_CAP_SQUAD ) && !InSquad() )
	{
		if ( !FStringNull( pev->netname ) )
		{
			// if I have a groupname, I can only recruit if I'm flagged as leader
			if ( !( pev->spawnflags & SF_SQUADMONSTER_LEADER ) )
			{
				return;
			}
		}

		// try to form squads now.
		int iSquadSize = SquadRecruit( 1024, 4 );

		if ( iSquadSize )
		{
		  ALERT ( at_aiconsole, "Squad of %d %s formed\n", iSquadSize, STRING( pev->classname ) );
		}

		if ( IsLeader() && FClassnameIs ( pev, "monster_human_grunt" ) )
		{
			SetBodygroup( 1, 1 ); // UNDONE: truly ugly hack
			pev->skin = 0;
		}

	}
}

BOOL CSquadMonster::NoFriendlyFire()
{
	return NoFriendlyFire(FALSE); //default: don't like the player
}

//=========================================================
// NoFriendlyFire - checks for possibility of friendly fire
//
// Builds a large box in front of the grunt and checks to see 
// if any squad members are in that box. 
//=========================================================
BOOL CSquadMonster::NoFriendlyFire(BOOL playerAlly)
{
	if (!playerAlly && !InSquad())
		return TRUE;

	CPlane	backPlane;
	CPlane  leftPlane;
	CPlane	rightPlane;

	//!!!BUGBUG - to fix this, the planes must be aligned to where the monster will be firing its gun, not the direction it is facing!!!

	if ( m_hEnemy != NULL )
	{
		UTIL_MakeVectors ( UTIL_VecToAngles( m_hEnemy->Center() - pev->origin ) );
	}
	else
	{
		// if there's no enemy, pretend there's a friendly in the way, so the grunt won't shoot.
		return FALSE;
	}

	//UTIL_MakeVectors ( pev->angles );
	
	Vector vecLeftSide = pev->origin - (gpGlobals->v_right * (pev->size.x * 1.5));
	Vector vecRightSide = pev->origin + (gpGlobals->v_right * (pev->size.x * 1.5));
	Vector v_left = gpGlobals->v_right * -1;

	leftPlane.InitializePlane ( gpGlobals->v_right, vecLeftSide );
	rightPlane.InitializePlane ( v_left, vecRightSide );
	backPlane.InitializePlane ( gpGlobals->v_forward, pev->origin );

/*
	ALERT ( at_console, "LeftPlane: %f %f %f : %f\n", leftPlane.m_vecNormal.x, leftPlane.m_vecNormal.y, leftPlane.m_vecNormal.z, leftPlane.m_flDist );
	ALERT ( at_console, "RightPlane: %f %f %f : %f\n", rightPlane.m_vecNormal.x, rightPlane.m_vecNormal.y, rightPlane.m_vecNormal.z, rightPlane.m_flDist );
	ALERT ( at_console, "BackPlane: %f %f %f : %f\n", backPlane.m_vecNormal.x, backPlane.m_vecNormal.y, backPlane.m_vecNormal.z, backPlane.m_flDist );
*/

	CBaseSquad* pSquadLeader = MySquadLeader();
	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CBaseSquad* pMember = pSquadLeader->MySquadMember(i);
		if (pMember && pMember != this)
		{

			if (backPlane.PointInFront(pMember->pev->origin) &&
				leftPlane.PointInFront(pMember->pev->origin) &&
				rightPlane.PointInFront(pMember->pev->origin))
			{
				// this guy is in the check volume! Don't shoot!
				return FALSE;
			}
		}
	}

	if (playerAlly)
	{
		edict_t* pentPlayer = FIND_CLIENT_IN_PVS(edict());
		if (!FNullEnt(pentPlayer) &&
			backPlane.PointInFront(pentPlayer->v.origin) &&
			leftPlane.PointInFront(pentPlayer->v.origin) &&
			rightPlane.PointInFront(pentPlayer->v.origin))
		{
			// the player is in the check volume! Don't shoot!
			return FALSE;
		}
	}

	return TRUE;
}

//=========================================================
// GetIdealState - surveys the Conditions information available
// and finds the best new state for a monster.
//=========================================================
MONSTERSTATE CSquadMonster :: GetIdealState ()
{
	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if ( HasConditions ( bits_COND_NEW_ENEMY ) && InSquad() )
		{
			SquadMakeEnemy ( m_hEnemy );
		}
		break;
	}

	return CBaseMonster :: GetIdealState();
}

//=========================================================
// FValidateCover - determines whether or not the chosen
// cover location is a good one to move to. (currently based
// on proximity to others in the squad)
//=========================================================
BOOL CSquadMonster :: FValidateCover ( const Vector &vecCoverLocation )
{
	if ( !InSquad() )
	{
		return TRUE;
	}

	if (SquadMemberInRange( vecCoverLocation, 128 ))
	{
		// another squad member is too close to this piece of cover.
		return FALSE;
	}

	return TRUE;
}

extern Schedule_t	slChaseEnemyFailed[];

Schedule_t *CSquadMonster::GetScheduleOfType( int iType )
{
	switch ( iType )
	{

	case SCHED_CHASE_ENEMY_FAILED:
		{
			return &slChaseEnemyFailed[ 0 ];
		}
	
	default:
		return CBaseMonster::GetScheduleOfType( iType );
	}
}

int CSquadMonster::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (flDamage >= pev->max_health)
	{
		auto squadLeader = MySquadLeader();

		for (int i = 0; i < MAX_SQUAD_MEMBERS - 1; ++i)
		{
			auto squadMember = squadLeader->m_hSquadMember[i].Entity<CSquadMonster>();

			if (squadMember)
			{
				if (!squadMember->m_hEnemy)
				{
					g_vecAttackDir = ((pevAttacker->origin + pevAttacker->view_ofs) - (squadMember->pev->origin + squadMember->pev->view_ofs)).Normalize();

					const Vector vecStart = squadMember->pev->origin + squadMember->pev->view_ofs;
					const Vector vecEnd = pevAttacker->origin + pevAttacker->view_ofs + (g_vecAttackDir * m_flDistLook);

					TraceResult tr;

					UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, squadMember->edict(), &tr);

					if (tr.flFraction == 1.0)
					{
						m_IdealMonsterState = MONSTERSTATE_HUNT;
					}
					else
					{
						squadMember->m_hEnemy = CBaseEntity::Instance(tr.pHit);
						squadMember->m_vecEnemyLKP = pevAttacker->origin;
						squadMember->SetConditions(bits_COND_NEW_ENEMY);
					}
				}
			}
		}

		if (!squadLeader->m_hEnemy)
		{
			g_vecAttackDir = ((pevAttacker->origin + pevAttacker->view_ofs) - (squadLeader->pev->origin + squadLeader->pev->view_ofs)).Normalize();

			const Vector vecStart = squadLeader->pev->origin + squadLeader->pev->view_ofs;
			const Vector vecEnd = pevAttacker->origin + pevAttacker->view_ofs + (g_vecAttackDir * m_flDistLook);

			TraceResult tr;

			UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, squadLeader->edict(), &tr);

			if (tr.flFraction == 1.0)
			{
				m_IdealMonsterState = MONSTERSTATE_HUNT;
			}
			else
			{
				squadLeader->m_hEnemy = CBaseEntity::Instance(tr.pHit);
				squadLeader->m_vecEnemyLKP = pevAttacker->origin;
				squadLeader->SetConditions(bits_COND_NEW_ENEMY);
			}
		}
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}
