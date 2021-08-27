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
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"saverestore.h"
#include	"client.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"game.h"
#include	"movewith.h"
#include	"skill.h"

void EntvarsKeyvalue( entvars_t *pev, KeyValueData *pkvd );

void PM_Move ( struct playermove_s *ppmove, int server );
void PM_Init ( struct playermove_s *ppmove  );
char PM_FindTextureType( char *name );

void OnFreeEntPrivateData(edict_s* pEdict);

extern int g_serveractive;

extern Vector VecBModelOrigin( entvars_t* pevBModel );
extern DLL_GLOBAL Vector		g_vecAttackDir;
extern DLL_GLOBAL int			g_iSkillLevel;

CUtlVector<hash_item_t> stringsHashTable;
CMemoryPool hashItemMemPool(sizeof(hash_item_t), 64);

int CaseInsensitiveHash(const char* string, int iBounds)
{
	unsigned int hash = 0;

	if (!*string)
		return 0;

	while (*string)
	{
		if (*string < 'A' || *string > 'Z')
			hash = *string + 2 * hash;
		else
			hash = *string + 2 * hash + ' ';

		string++;
	}

	return (hash % iBounds);
}

void EmptyEntityHashTable()
{
	for (int i = 0; i < stringsHashTable.Count(); i++)
	{
		hash_item_t* item = &stringsHashTable[i];
		hash_item_t* temp = item->next;

		item->pev = nullptr;
		item->pevIndex = 0;
		item->lastHash = 0;
		item->next = nullptr;

		while (temp)
		{
			hash_item_t* free = temp;
			temp = temp->next;
			hashItemMemPool.Free(free);
		}
	}
}

void AddEntityHashValue(entvars_t* pev, const char* value, hash_types_e fieldType)
{
	if (fieldType != CLASSNAME)
		return;

	if (FStringNull(pev->classname))
		return;

	int count = stringsHashTable.Count();
	int hash = CaseInsensitiveHash(value, count);
	int pevIndex = ENTINDEX(ENT(pev));
	hash_item_t* item = &stringsHashTable[hash];

	while (item->pev)
	{
		if (!strcmp(STRING(item->pev->classname), STRING(pev->classname)))
			break;

		hash = (hash + 1) % count;
		item = &stringsHashTable[hash];
	}
	
	if (item->pev)
	{
		entvars_t * pevtemp;
		hash_item_t* next = item->next;
		while (next)
		{
			if (item->pev == pev || item->pevIndex >= pevIndex)
				break;

			item = next;
			next = next->next;
		}
		
		if (pevIndex < item->pevIndex)
		{
			pevtemp = item->pev;
			item->pev = pev;
			item->lastHash = nullptr;
			item->pevIndex = pevIndex;

			pevIndex = ENTINDEX(ENT(pevtemp));
		}
		else
			pevtemp = pev;

		if (item->pev != pevtemp)
		{
			hash_item_t* temp = item->next;
			hash_item_t* newp = (hash_item_t*)hashItemMemPool.Alloc(sizeof(hash_item_t));

			item->next = newp;
			newp->pev = pevtemp;
			newp->lastHash = nullptr;
			newp->pevIndex = pevIndex;

			if (next)
				newp->next = temp;
			else
				newp->next = nullptr;
		}
	}
	else
	{
		item->pev = pev;
		item->lastHash = nullptr;
		item->pevIndex = ENTINDEX(ENT(pev));
	}
}

void RemoveEntityHashValue(entvars_t* pev, const char* value, hash_types_e fieldType)
{
	int count = stringsHashTable.Count();
	int hash = CaseInsensitiveHash(value, count);

	if (fieldType != CLASSNAME)
		return;

	hash = hash % count;
	hash_item_t* item = &stringsHashTable[hash];

	while (item->pev)
	{
		if (!strcmp(STRING(item->pev->classname), STRING(pev->classname)))
			break;

		hash = (hash + 1) % count;
		item = &stringsHashTable[hash];
	}
	if (item->pev)
	{
		hash_item_t* last = item;
		while (item->next)
		{
			if (item->pev == pev)
				break;

			last = item;
			item = item->next;
		}
		if (item->pev == pev)
		{
			if (last == item)
			{
				if (item->next)
				{
					item->pev = item->next->pev;
					item->pevIndex = item->next->pevIndex;
					item->lastHash = nullptr;
					item->next = item->next->next;
				}
				else
				{
					item->pev = nullptr;
					item->lastHash = nullptr;
					item->pevIndex = 0;
				}
			}
			else
			{
				if (stringsHashTable[hash].lastHash == item)
					stringsHashTable[hash].lastHash = nullptr;

				last->next = item->next;
				hashItemMemPool.Free(item);
			}
		}
	}
}

void printEntities()
{
	for (int i = 0; i < stringsHashTable.Count(); i++)
	{
		hash_item_t* item = &stringsHashTable[i];

		if (item->pev)
			CONSOLE_ECHO_LOGGED("Print: %s %i %p\n", STRING(stringsHashTable[i].pev->classname), ENTINDEX(ENT(item->pev)), item->pev);

		for (item = stringsHashTable[i].next; item; item = item->next)
			CONSOLE_ECHO_LOGGED("Print: %s %i %p\n", STRING(item->pev->classname), ENTINDEX(ENT(item->pev)), item->pev);
	}
}

edict_t* CREATE_NAMED_ENTITY(int iClass)
{
	edict_t* named = g_engfuncs.pfnCreateNamedEntity(iClass);

	if (named)
		AddEntityHashValue(&named->v, STRING(iClass), CLASSNAME);

	return named;
}

void REMOVE_ENTITY(edict_t* e)
{
	if (e)
		g_engfuncs.pfnRemoveEntity(e);
}

void CONSOLE_ECHO_LOGGED(char* pszMsg, ...)
{
	va_list argptr;
	static char szStr[1024];

	va_start(argptr, pszMsg);
	vsprintf(szStr, pszMsg, argptr);
	va_end(argptr);

	g_engfuncs.pfnServerPrint(szStr);
	UTIL_LogPrintf(szStr);
}

static DLL_FUNCTIONS gFunctionTable = 
{
	GameDLLInit,				//pfnGameInit
	DispatchSpawn,				//pfnSpawn
	DispatchThink,				//pfnThink
	DispatchUse,				//pfnUse
	DispatchTouch,				//pfnTouch
	DispatchBlocked,			//pfnBlocked
	DispatchKeyValue,			//pfnKeyValue
	DispatchSave,				//pfnSave
	DispatchRestore,			//pfnRestore
	DispatchObjectCollsionBox,	//pfnAbsBox

	SaveWriteFields,			//pfnSaveWriteFields
	SaveReadFields,				//pfnSaveReadFields

	SaveGlobalState,			//pfnSaveGlobalState
	RestoreGlobalState,			//pfnRestoreGlobalState
	ResetGlobalState,			//pfnResetGlobalState

	ClientConnect,				//pfnClientConnect
	ClientDisconnect,			//pfnClientDisconnect
	ClientKill,					//pfnClientKill
	ClientPutInServer,			//pfnClientPutInServer
	ClientCommand,				//pfnClientCommand
	ClientUserInfoChanged,		//pfnClientUserInfoChanged
	ServerActivate,				//pfnServerActivate
	ServerDeactivate,			//pfnServerDeactivate

	PlayerPreThink,				//pfnPlayerPreThink
	PlayerPostThink,			//pfnPlayerPostThink

	StartFrame,					//pfnStartFrame
	ParmsNewLevel,				//pfnParmsNewLevel
	ParmsChangeLevel,			//pfnParmsChangeLevel

	GetGameDescription,         //pfnGetGameDescription    Returns string describing current .dll game.
	PlayerCustomization,        //pfnPlayerCustomization   Notifies .dll of new customization for player.

	SpectatorConnect,			//pfnSpectatorConnect      Called when spectator joins server
	SpectatorDisconnect,        //pfnSpectatorDisconnect   Called when spectator leaves the server
	SpectatorThink,				//pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

	Sys_Error,					//pfnSys_Error				Called when engine has encountered an error

	PM_Move,					//pfnPM_Move
	PM_Init,					//pfnPM_Init				Server version of player movement initialization
	PM_FindTextureType,			//pfnPM_FindTextureType
	
	SetupVisibility,			//pfnSetupVisibility        Set up PVS and PAS for networking for this client
	UpdateClientData,			//pfnUpdateClientData       Set up data sent only to specific client
	AddToFullPack,				//pfnAddToFullPack
	CreateBaseline,				//pfnCreateBaseline			Tweak entity baseline for network encoding, allows setup of player baselines, too.
	RegisterEncoders,			//pfnRegisterEncoders		Callbacks for network encoding
	GetWeaponData,				//pfnGetWeaponData
	CmdStart,					//pfnCmdStart
	CmdEnd,						//pfnCmdEnd
	ConnectionlessPacket,		//pfnConnectionlessPacket
	GetHullBounds,				//pfnGetHullBounds
	CreateInstancedBaselines,   //pfnCreateInstancedBaselines
	InconsistentFile,			//pfnInconsistentFile
	AllowLagCompensation,		//pfnAllowLagCompensation
};

NEW_DLL_FUNCTIONS gNewDLLFunctions =
{
	OnFreeEntPrivateData,
	OnGameShutdown,
	nullptr,
	nullptr,
	nullptr
};

static void SetObjectCollisionBox( entvars_t *pev );

extern "C" {

int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
{
	if ( !pFunctionTable || interfaceVersion != INTERFACE_VERSION )
	{
		return FALSE;
	}
	
	memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );

	stringsHashTable.SetSize(2048);

	for (int i = 0; i < stringsHashTable.Count(); i++)
		stringsHashTable[i].next = NULL;

	EmptyEntityHashTable();
	
	return TRUE;
}

int GetEntityAPI2(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != INTERFACE_VERSION)
	{
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return FALSE;
	}

	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));

	stringsHashTable.SetSize(2048);

	for (int i = 0; i < stringsHashTable.Count(); i++)
		stringsHashTable[i].next = NULL;

	EmptyEntityHashTable();

	return TRUE;
}

int GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
	{
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return FALSE;
	}
	
	pFunctionTable->pfnOnFreeEntPrivateData = gNewDLLFunctions.pfnOnFreeEntPrivateData;
	pFunctionTable->pfnGameShutdown = gNewDLLFunctions.pfnGameShutdown;
	pFunctionTable->pfnShouldCollide = gNewDLLFunctions.pfnShouldCollide;
	
	return TRUE;
}
}

void OnGameShutdown()
{
	ShutdownFileSystem();
}

int DispatchSpawn( edict_t *pent )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);

	if (pEntity)
	{
		// Initialize these or entities who don't link to the world won't have anything in here
		pEntity->pev->absmin = pEntity->pev->origin - Vector(1,1,1);
		pEntity->pev->absmax = pEntity->pev->origin + Vector(1,1,1);

		pEntity->Spawn();

		// Try to get the pointer again, in case the spawn function deleted the entity.
		// UNDONE: Spawn() should really return a code to ask that the entity be deleted, but
		// that would touch too much code for me to do that right now.
		pEntity = (CBaseEntity *)GET_PRIVATE(pent);

		if ( pEntity )
		{
			if ( g_pGameRules && !g_pGameRules->IsAllowedToSpawn( pEntity ) )
				return -1;	// return that this entity should be deleted
			
			if ( pEntity->pev->flags & FL_KILLME )
				return -1;

			if (g_iSkillLevel == SKILL_EASY && pEntity->m_iLFlags & LF_NOTEASY)
				return -1; //LRC
			
			if (g_iSkillLevel == SKILL_MEDIUM && pEntity->m_iLFlags & LF_NOTMEDIUM)
				return -1; //LRC
			
			if (g_iSkillLevel == SKILL_HARD && pEntity->m_iLFlags & LF_NOTHARD)
				return -1; //LRC
		}

		// Handle global stuff here
		if ( pEntity && pEntity->pev->globalname ) 
		{
			const globalentity_t *pGlobal = gGlobalState.EntityFromTable( pEntity->pev->globalname );
			if ( pGlobal )
			{
				// Already dead? delete
				if ( pGlobal->state == GLOBAL_DEAD )
					return -1;
				else if ( !FStrEq( STRING(gpGlobals->mapname), pGlobal->levelName ) )
					pEntity->MakeDormant();	// Hasn't been moved to this level yet, wait but stay alive
				// In this level & not dead, continue on as normal
			}
			else
			{
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd( pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON );
//				ALERT( at_console, "Added global entity %s (%s)\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->globalname) );
			}
		}

	}

	return 0;
}

void DispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
	if ( !pkvd || !pentKeyvalue )
		return;

	EntvarsKeyvalue( VARS(pentKeyvalue), pkvd );

	// If the key was an entity variable, or there's no class set yet, don't look for the object, it may
	// not exist yet.
	if ( pkvd->fHandled || pkvd->szClassName == NULL )
		return;

	// Get the actualy entity object
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pentKeyvalue);

	if ( !pEntity )
		return;

	pEntity->KeyValue( pkvd );
}


// HACKHACK -- this is a hack to keep the node graph entity from "touching" things (like triggers)
// while it builds the graph
BOOL gTouchDisabled = FALSE;
void DispatchTouch( edict_t *pentTouched, edict_t *pentOther )
{
	if ( gTouchDisabled )
		return;

	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pentTouched);
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE( pentOther );

	if ( pEntity && pOther && ! ((pEntity->pev->flags | pOther->pev->flags) & FL_KILLME) )
		pEntity->Touch( pOther );
}


void DispatchUse( edict_t *pentUsed, edict_t *pentOther )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pentUsed);
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE(pentOther);

	if (pEntity && !(pEntity->pev->flags & FL_KILLME) )
		pEntity->Use( pOther, pOther, USE_TOGGLE, 0 );
}

void DispatchThink( edict_t *pent )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);
	if (pEntity)
	{
		if ( FBitSet( pEntity->pev->flags, FL_DORMANT ) )
			ALERT( at_error, "Dormant entity %s is thinking!!\n", STRING(pEntity->pev->classname) );
				
		pEntity->Think();
	}
}

void DispatchBlocked( edict_t *pentBlocked, edict_t *pentOther )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pentBlocked );
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE( pentOther );

	if (pEntity)
		pEntity->Blocked( pOther );
}

void DispatchSave( edict_t *pent, SAVERESTOREDATA *pSaveData )
{
	gpGlobals->time = pSaveData->time;

	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);
	
	if ( pEntity && pSaveData )
	{
		ENTITYTABLE *pTable = &pSaveData->pTable[ pSaveData->currentIndex ];

		if ( pTable->pent != pent )
			ALERT( at_error, "ENTITY TABLE OR INDEX IS WRONG!!!!\n" );

		if ( pEntity->ObjectCaps() & FCAP_DONT_SAVE )
			return;

		// These don't use ltime & nextthink as times really, but we'll fudge around it.
		if (pEntity->pev->movetype == MOVETYPE_PUSH)
		{
			//LRC - rearranged so that we can correct m_fNextThink too.
			float delta = gpGlobals->time - pEntity->pev->ltime;
			pEntity->pev->ltime += delta;
			pEntity->pev->nextthink += delta;
			pEntity->m_fPevNextThink = pEntity->pev->nextthink;
			pEntity->m_fNextThink += delta;
		}

		pTable->location = pSaveData->size;		// Remember entity position for file I/O
		pTable->classname = pEntity->pev->classname;	// Remember entity class for respawn

		CSave saveHelper( pSaveData );
		pEntity->Save( saveHelper );

		pTable->size = pSaveData->size - pTable->location;	// Size of entity block is data size written to block
	}
}

void OnFreeEntPrivateData(edict_s* pEdict)
{
	CBaseEntity* pEntity = CBaseEntity::Instance(pEdict);

	if (!pEntity)
		return;

	pEntity->UpdateOnRemove();
	RemoveEntityHashValue(pEntity->pev, STRING(pEntity->pev->classname), CLASSNAME);
	pEntity->~CBaseEntity();
}

// Find the matching global entity.  Spit out an error if the designer made entities of
// different classes with the same global name
CBaseEntity *FindGlobalEntity( string_t classname, string_t globalname )
{
	edict_t *pent = FIND_ENTITY_BY_STRING( NULL, "globalname", STRING(globalname) );
	CBaseEntity *pReturn = CBaseEntity::Instance( pent );
	if ( pReturn )
	{
		if ( !FClassnameIs( pReturn->pev, STRING(classname) ) )
		{
			ALERT( at_console, "Global entity found %s, wrong class %s\n", STRING(globalname), STRING(pReturn->pev->classname) );
			pReturn = NULL;
		}
	}

	return pReturn;
}


int DispatchRestore( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity )
{
	gpGlobals->time = pSaveData->time;

	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);

	if ( pEntity && pSaveData )
	{
		entvars_t tmpVars;
		Vector oldOffset;

		CRestore restoreHelper( pSaveData );
		if ( globalEntity )
		{
			CRestore tmpRestore( pSaveData );
			tmpRestore.PrecacheMode( 0 );
			tmpRestore.ReadEntVars( "ENTVARS", &tmpVars );

			// HACKHACK - reset the save pointers, we're going to restore for real this time
			pSaveData->size = pSaveData->pTable[pSaveData->currentIndex].location;
			pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->size;
			// -------------------

			const globalentity_t *pGlobal = gGlobalState.EntityFromTable( tmpVars.globalname );
			
			// Don't overlay any instance of the global that isn't the latest
			// pSaveData->szCurrentMapName is the level this entity is coming from
			// pGlobla->levelName is the last level the global entity was active in.
			// If they aren't the same, then this global update is out of date.
			if ( !FStrEq( pSaveData->szCurrentMapName, pGlobal->levelName ) )
				return 0;

			// Compute the new global offset
			oldOffset = pSaveData->vecLandmarkOffset;
			CBaseEntity *pNewEntity = FindGlobalEntity( tmpVars.classname, tmpVars.globalname );
			if ( pNewEntity )
			{
//				ALERT( at_console, "Overlay %s with %s\n", STRING(pNewEntity->pev->classname), STRING(tmpVars.classname) );
				// Tell the restore code we're overlaying a global entity from another level
				restoreHelper.SetGlobalMode( 1 );	// Don't overwrite global fields
				pSaveData->vecLandmarkOffset = (pSaveData->vecLandmarkOffset - pNewEntity->pev->mins) + tmpVars.mins;
				pEntity = pNewEntity;// we're going to restore this data OVER the old entity
				pent = ENT( pEntity->pev );
				// Update the global table to say that the global definition of this entity should come from this level
				gGlobalState.EntityUpdate( pEntity->pev->globalname, gpGlobals->mapname );
			}
			else
			{
				// This entity will be freed automatically by the engine.  If we don't do a restore on a matching entity (below)
				// or call EntityUpdate() to move it to this level, we haven't changed global state at all.
				return 0;
			}

		}

		if ( pEntity->ObjectCaps() & FCAP_MUST_SPAWN )
		{
			pEntity->Restore( restoreHelper );
			pEntity->Spawn();
		}
		else
		{
			pEntity->Restore( restoreHelper );
			pEntity->Precache( );
		}

		// Again, could be deleted, get the pointer again.
		pEntity = (CBaseEntity *)GET_PRIVATE(pent);

#if 0
		if ( pEntity && pEntity->pev->globalname && globalEntity ) 
		{
			ALERT( at_console, "Global %s is %s\n", STRING(pEntity->pev->globalname), STRING(pEntity->pev->model) );
		}
#endif

		// Is this an overriding global entity (coming over the transition), or one restoring in a level
		if ( globalEntity )
		{
//			ALERT( at_console, "After: %f %f %f %s\n", pEntity->pev->origin.x, pEntity->pev->origin.y, pEntity->pev->origin.z, STRING(pEntity->pev->model) );
			pSaveData->vecLandmarkOffset = oldOffset;
			if ( pEntity )
			{
				UTIL_SetOrigin( pEntity, pEntity->pev->origin );
				pEntity->OverrideReset();
			}
		}
		else if ( pEntity && pEntity->pev->globalname ) 
		{
			const globalentity_t *pGlobal = gGlobalState.EntityFromTable( pEntity->pev->globalname );
			if ( pGlobal )
			{
				// Already dead? delete
				if ( pGlobal->state == GLOBAL_DEAD )
					return -1;
				else if ( !FStrEq( STRING(gpGlobals->mapname), pGlobal->levelName ) )
				{
					pEntity->MakeDormant();	// Hasn't been moved to this level yet, wait but stay alive
				}
				// In this level & not dead, continue on as normal
			}
			else
			{
				ALERT( at_error, "Global Entity %s (%s) not in table!!!\n", STRING(pEntity->pev->globalname), STRING(pEntity->pev->classname) );
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd( pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON );
			}
		}
	}
	return 0;
}


void DispatchObjectCollsionBox( edict_t *pent )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);
	if (pEntity)
	{
		pEntity->SetObjectCollisionBox();
	}
	else
		SetObjectCollisionBox( &pent->v );
}


void SaveWriteFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	CSave saveHelper( pSaveData );
	saveHelper.WriteFields( pname, pBaseData, pFields, fieldCount );
}


void SaveReadFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	CRestore restoreHelper( pSaveData );
	restoreHelper.ReadFields( pname, pBaseData, pFields, fieldCount );
}

//LRC
void CBaseEntity::Activate()
{
	//LRC - rebuild the new assistlist as the game starts
	if (m_iLFlags & LF_ASSISTLIST)
	{
		UTIL_AddToAssistList(this);
	}

	//LRC - and the aliaslist too
	if (m_iLFlags & LF_ALIASLIST)
	{
		UTIL_AddToAliasList((CBaseAlias*)this);
	}

	if (m_activated) return;
	m_activated = TRUE;
	SetParent(m_MoveWith);
	PostSpawn();
}

//g-cont. upgrade system to xashParentSystem 0.3 beta
void CBaseEntity::SetParent(int m_iNewParent, int m_iAttachment)
{
	if (!m_iNewParent) //unlink entity from chain
	{
		//ResetParent(); //g-cont. temporary disabled
		return; //disable
	}

	CBaseEntity* pParent;

	if (!m_iAttachment) //try to extract aiment from name
	{
		char* name = (char*)STRING(m_iNewParent);
		for (char* c = name; *c; c++)
		{
			if (*c == '.')
			{
				m_iAttachment = atoi(c + 1);
				name[strlen(name) - 2] = 0;
				pParent = UTIL_FindEntityByTargetname(NULL, name);
				SetParent(pParent, m_iAttachment);
				return;
			}
		}
	}

	pParent = UTIL_FindEntityByTargetname(NULL, STRING(m_iNewParent));
	SetParent(pParent, m_iAttachment); //check pointer to valid later
}

void CBaseEntity::SetParent(CBaseEntity* pParent, int m_iAttachment)
{
	//if (!m_MoveWith) return;

	m_pMoveWith = pParent; //UTIL_FindEntityByTargetname(NULL, STRING(m_MoveWith));

	if (!m_pMoveWith)
	{
		ALERT(at_console, "Missing movewith entity %s\n", STRING(m_MoveWith));
		return;
	}

	//	if (pev->targetname)
	//		ALERT(at_console,"Init: %s %s moves with %s\n", STRING(pev->classname), STRING(pev->targetname), STRING(m_MoveWith));
	//	else
	//		ALERT(at_console,"Init: %s moves with %s\n", STRING(pev->classname), STRING(m_MoveWith));

	//check for himself parent
	if (m_pMoveWith == this)
	{
		ALERT(at_console, "%s has himself parent!\n", STRING(pev->classname));
		return;
	}

	CBaseEntity* pSibling = m_pMoveWith->m_pChildMoveWith;
	while (pSibling) // check that this entity isn't already in the list of children
	{
		if (pSibling == this) break;
		pSibling = pSibling->m_pSiblingMoveWith;
	}
	if (!pSibling) // if movewith is being set up for the first time...
	{
		// add this entity to the list of children
		m_pSiblingMoveWith = m_pMoveWith->m_pChildMoveWith; // may be null: that's fine by me.
		m_pMoveWith->m_pChildMoveWith = this;

		if (m_iAttachment) //parent has attcahment
		{
			if (m_iLFlags & LF_POINTENTITY || pev->flags & FL_MONSTER)
			{
				pev->skin = ENTINDEX(m_pMoveWith->edict());
				pev->body = m_iAttachment;
				pev->aiment = m_pMoveWith->edict();
				pev->movetype = MOVETYPE_FOLLOW;
			}
			else //error
			{
				ALERT(at_console, "%s not following with aiment %d!(yet)\n", STRING(pev->classname), m_iAttachment);
			}
			return;
		}
		else //appllayed to origin
		{
			if (pev->movetype == MOVETYPE_NONE)
			{
				if (pev->solid == SOLID_BSP)
					pev->movetype = MOVETYPE_PUSH;
				else pev->movetype = MOVETYPE_NOCLIP; // or _FLY, perhaps?
				SetBits(m_iLFlags, LF_MOVENONE); //member movetype
			}

			if (m_pMoveWith->pev->movetype == MOVETYPE_WALK) //parent is walking monster?
			{
				SetBits(m_iLFlags, LF_POSTORG); //copy pos from parent every frame
				pev->solid = SOLID_NOT; //set non solid
			}
			
			m_vecParentOrigin = m_pMoveWith->pev->origin;
			m_vecParentAngles = m_pMoveWith->pev->angles;
		}

		// was the parent shifted at spawn-time?
		if (m_pMoveWith->m_vecSpawnOffset != g_vecZero)
		{
			//ALERT(at_console,"Corrected using SpawnOffset\n");
			// shift this by the same amount the parent was shifted by.
			UTIL_AssignOrigin(this, pev->origin + m_pMoveWith->m_vecSpawnOffset);
			//...and inherit the same offset.
			m_vecSpawnOffset = m_vecSpawnOffset + m_pMoveWith->m_vecSpawnOffset;
		}

		m_vecOffsetOrigin = pev->origin - m_vecParentOrigin;
		m_vecOffsetAngles = pev->angles - m_vecParentAngles;

		if ((m_pMoveWith->m_iLFlags & LF_ANGULAR && m_vecOffsetOrigin != g_vecZero) || m_pMoveWith->m_iLFlags &
			LF_POINTENTITY)
		{
			SetBits(m_iLFlags, LF_POSTORG); //magic stuff
			//GetPInfo( this );
		}

		if (g_serveractive) //maybe parent is moving ?
		{
			pev->velocity = pev->velocity + m_pMoveWith->pev->velocity;
			pev->avelocity = pev->avelocity + m_pMoveWith->pev->avelocity;
		}
	}
}

void CBaseEntity::ResetParent()
{
	CBaseEntity* pTemp;

	if (m_iLFlags & LF_MOVENONE) //this entity was static e.g. func_wall
	{
		ClearBits(m_iLFlags, LF_MOVENONE);
		pev->movetype = MOVETYPE_NONE;
	}

	if (!g_pWorld)
	{
		ALERT(at_console, "ResetParent has no AssistList!\n");
		return;
	}

	//LRC - remove this from the AssistList.
	for (pTemp = g_pWorld; pTemp->m_pAssistLink != NULL; pTemp = pTemp->m_pAssistLink)
	{
		if (this == pTemp->m_pAssistLink)
		{
			//			ALERT(at_console,"REMOVE: %s removed from the Assist List.\n", STRING(pev->classname));
			pTemp->m_pAssistLink = this->m_pAssistLink;
			this->m_pAssistLink = NULL;
			break;
		}
	}

	//LRC
	if (m_pMoveWith)
	{
		// if I'm moving with another entity, take me out of the list. (otherwise things crash!)
		pTemp = m_pMoveWith->m_pChildMoveWith;
		if (pTemp == this)
		{
			m_pMoveWith->m_pChildMoveWith = this->m_pSiblingMoveWith;
		}
		else
		{
			while (pTemp->m_pSiblingMoveWith)
			{
				if (pTemp->m_pSiblingMoveWith == this)
				{
					pTemp->m_pSiblingMoveWith = this->m_pSiblingMoveWith;
					break;
				}
				pTemp = pTemp->m_pSiblingMoveWith;
			}
		}
		//		ALERT(at_console,"REMOVE: %s removed from the %s ChildMoveWith list.\n", STRING(pev->classname), STRING(m_pMoveWith->pev->targetname));
	}

	//LRC - do the same thing if another entity is moving with _me_.
	if (m_pChildMoveWith)
	{
		CBaseEntity* pCur = m_pChildMoveWith;
		while (pCur != NULL)
		{
			CBaseEntity* pNext = pCur->m_pSiblingMoveWith;
			// bring children to a stop
			UTIL_SetMoveWithVelocity(pCur, g_vecZero, 100);
			UTIL_SetMoveWithAvelocity(pCur, g_vecZero, 100);
			pCur->m_pMoveWith = NULL;
			pCur->m_pSiblingMoveWith = NULL;
			pCur = pNext;
		}
	}
}

void CBaseEntity::ClearPointers()
{
	m_pChildMoveWith = NULL;
	m_pSiblingMoveWith = NULL;
	m_pAssistLink = NULL;
}

//LRC
void CBaseEntity::DontThink()
{
	m_fNextThink = 0;
	if (m_pMoveWith == nullptr && m_pChildMoveWith == nullptr)
	{
		pev->nextthink = 0;
		m_fPevNextThink = 0;
	}

	//if(pev->targetname)
	//ALERT(at_console, "DontThink for %s\n", STRING(pev->targetname));
}

//LRC
// PUSH entities won't have their velocity applied unless they're thinking.
// make them do so for the foreseeable future.
void CBaseEntity::SetEternalThink()
{
	if (pev->movetype == MOVETYPE_PUSH)
	{
		pev->nextthink = pev->ltime + 1E6;
		m_fPevNextThink = pev->nextthink;
	}

	for (CBaseEntity* pChild = m_pChildMoveWith; pChild != nullptr; pChild = pChild->m_pSiblingMoveWith)
		pChild->SetEternalThink();
}

//LRC - for getting round the engine's preconceptions.
// MoveWith entities have to be able to think independently of moving.
// This is how we do so.
void CBaseEntity::SetNextThink(float delay, bool correctSpeed)
{
	// now monsters use this method, too.
	if (m_pMoveWith || m_pChildMoveWith || pev->flags & FL_MONSTER)
	{
		// use the Assist system, so that thinking doesn't mess up movement.
		if (pev->movetype == MOVETYPE_PUSH)
			m_fNextThink = pev->ltime + delay;
		else
			m_fNextThink = gpGlobals->time + delay;
		
		SetEternalThink();
		UTIL_MarkForAssist(this, correctSpeed);

		//if (pev->targetname) 
		//ALERT(at_console, "SetAssistedThink for %s: %f\n", STRING(pev->targetname), m_fNextThink);
	}
	else
	{
		// set nextthink as normal.
		if (pev->movetype == MOVETYPE_PUSH)
		{
			pev->nextthink = pev->ltime + delay;
		}
		else
		{
			pev->nextthink = gpGlobals->time + delay;
		}

		m_fPevNextThink = m_fNextThink = pev->nextthink;

		//if (pev->classname) 
		//	ALERT(at_console, "SetNormThink for %s: %f\n", STRING(pev->targetname), m_fNextThink);
	}
}

//LRC
void CBaseEntity::AbsoluteNextThink(float time, bool correctSpeed)
{
	if (m_pMoveWith || m_pChildMoveWith)
	{
		// use the Assist system, so that thinking doesn't mess up movement.
		m_fNextThink = time;
		SetEternalThink();
		UTIL_MarkForAssist(this, correctSpeed);
	}
	else
	{
		// set nextthink as normal.
		pev->nextthink = time;
		m_fPevNextThink = m_fNextThink = pev->nextthink;
	}
}

//LRC - check in case the engine has changed our nextthink. (which it does
// on a depressingly frequent basis.)
// for some reason, this doesn't always produce perfect movement - but it's close
// enough for government work. (the player doesn't get stuck, at least.)
void CBaseEntity::ThinkCorrection()
{
	if (pev->nextthink != m_fPevNextThink)
	{
		m_fNextThink += pev->nextthink - m_fPevNextThink;
		m_fPevNextThink = pev->nextthink;
	}
}

// give health
int CBaseEntity :: TakeHealth( float flHealth, int bitsDamageType )
{
	if (!pev->takedamage)
		return 0;

// heal
	if ( pev->health >= pev->max_health )
		return 0;

	pev->health += flHealth;

	if (pev->health > pev->max_health)
		pev->health = pev->max_health;

	return 1;
}

// inflict damage on this entity.  bitsDamageType indicates type of damage inflicted, ie: DMG_CRUSH

int CBaseEntity :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	Vector			vecTemp;

	if (!pev->takedamage)
		return 0;

	// UNDONE: some entity types may be immune or resistant to some bitsDamageType
	
	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin). 
	if ( pevAttacker == pevInflictor )	
	{
		vecTemp = pevInflictor->origin - ( VecBModelOrigin(pev) );
	}
	else
	// an actual missile was involved.
	{
		vecTemp = pevInflictor->origin - ( VecBModelOrigin(pev) );
	}

// this global is still used for glass and other non-monster killables, along with decals.
	g_vecAttackDir = vecTemp.Normalize();
		
// save damage based on the target's armor level

// figure momentum add (don't let hurt brushes or other triggers move player)
	if ((!FNullEnt(pevInflictor)) && (pev->movetype == MOVETYPE_WALK || pev->movetype == MOVETYPE_STEP) && (pevAttacker->solid != SOLID_TRIGGER) )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();

		float flForce = flDamage * ((32 * 32 * 72.0) / (pev->size.x * pev->size.y * pev->size.z)) * 5;
		
		if (flForce > 1000.0) 
			flForce = 1000.0;
		pev->velocity = pev->velocity + vecDir * flForce;
	}

// do the damage
	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		Killed( pevAttacker, GIB_NORMAL );
		return 0;
	}

	return 1;
}


void CBaseEntity :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	UTIL_Remove( this );
}


CBaseEntity *CBaseEntity::GetNextTarget()
{
	if ( FStringNull( pev->target ) )
		return NULL;
	edict_t *pTarget = FIND_ENTITY_BY_TARGETNAME ( NULL, STRING(pev->target) );
	if ( FNullEnt(pTarget) )
		return NULL;

	return Instance( pTarget );
}

// Global Savedata for Delay
TYPEDESCRIPTION	CBaseEntity::m_SaveData[] = 
{
	DEFINE_FIELD( CBaseEntity, m_pGoalEnt, FIELD_CLASSPTR ),

	DEFINE_FIELD(CBaseEntity, m_MoveWith, FIELD_STRING), //LRC
	DEFINE_FIELD(CBaseEntity, m_pMoveWith, FIELD_CLASSPTR), //LRC
	DEFINE_FIELD(CBaseEntity, m_pChildMoveWith, FIELD_CLASSPTR), //LRC
	DEFINE_FIELD(CBaseEntity, m_pSiblingMoveWith, FIELD_CLASSPTR), //LRC

	DEFINE_FIELD(CBaseEntity, m_iLFlags, FIELD_INTEGER), //LRC
	DEFINE_FIELD(CBaseEntity, m_iStyle, FIELD_INTEGER), //LRC
	DEFINE_FIELD(CBaseEntity, m_vecMoveWithOffset, FIELD_VECTOR), //LRC
	DEFINE_FIELD(CBaseEntity, m_vecRotWithOffset, FIELD_VECTOR), //LRC
	DEFINE_FIELD(CBaseEntity, m_activated, FIELD_BOOLEAN), //LRC
	DEFINE_FIELD(CBaseEntity, m_fNextThink, FIELD_TIME), //LRC
	DEFINE_FIELD(CBaseEntity, m_fPevNextThink, FIELD_TIME), //LRC
	DEFINE_FIELD(CBaseEntity, m_vecPostAssistVel, FIELD_VECTOR), //LRC
	DEFINE_FIELD(CBaseEntity, m_vecPostAssistAVel, FIELD_VECTOR), //LRC
	
	DEFINE_FIELD( CBaseEntity, m_pfnThink, FIELD_FUNCTION ),		// UNDONE: Build table of these!!!
	DEFINE_FIELD( CBaseEntity, m_pfnTouch, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseEntity, m_pfnUse, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseEntity, m_pfnBlocked, FIELD_FUNCTION ),
};


int CBaseEntity::Save( CSave &save )
{
	ThinkCorrection(); //LRC

	if (save.WriteEntVars("ENTVARS", pev))
	{
		if (pev->targetname)
			return save.WriteFields("BASE", this, m_SaveData, ARRAYSIZE(m_SaveData));
		else
			return save.WriteFields("BASE", this, m_SaveData, ARRAYSIZE(m_SaveData));
	}

	return 0;
}

int CBaseEntity::Restore( CRestore &restore )
{
	int status = restore.ReadEntVars("ENTVARS", pev);
	if ( status )
		status = restore.ReadFields( "BASE", this, m_SaveData, ARRAYSIZE(m_SaveData) );

    if ( pev->modelindex != 0 && !FStringNull(pev->model) )
	{
		Vector mins = pev->mins;	// Set model is about to destroy these
		Vector maxs = pev->maxs;


		PRECACHE_MODEL( (char *)STRING(pev->model) );
		SetModel( pev->model);
		UTIL_SetSize(pev, mins, maxs);	// Reset them
	}

	return status;
}


// Initialize absmin & absmax to the appropriate box
void SetObjectCollisionBox( entvars_t *pev )
{
	if ( (pev->solid == SOLID_BSP) && 
		 (pev->angles.x || pev->angles.y|| pev->angles.z) )
	{	// expand for rotation
		int			i;

		float max = 0;
		for (i=0 ; i<3 ; i++)
		{
			float v = fabs(((float*)pev->mins)[i]);
			if (v > max)
				max = v;
			v = fabs( ((float *)pev->maxs)[i]);
			if (v > max)
				max = v;
		}
		for (i=0 ; i<3 ; i++)
		{
			((float *)pev->absmin)[i] = ((float *)pev->origin)[i] - max;
			((float *)pev->absmax)[i] = ((float *)pev->origin)[i] + max;
		}
	}
	else
	{
		pev->absmin = pev->origin + pev->mins;
		pev->absmax = pev->origin + pev->maxs;
	}

	pev->absmin.x -= 1;
	pev->absmin.y -= 1;
	pev->absmin.z -= 1;
	pev->absmax.x += 1;
	pev->absmax.y += 1;
	pev->absmax.z += 1;
}


void CBaseEntity::SetObjectCollisionBox()
{
	::SetObjectCollisionBox( pev );
}


int	CBaseEntity :: Intersects( CBaseEntity *pOther )
{
	if ( pOther->pev->absmin.x > pev->absmax.x ||
		 pOther->pev->absmin.y > pev->absmax.y ||
		 pOther->pev->absmin.z > pev->absmax.z ||
		 pOther->pev->absmax.x < pev->absmin.x ||
		 pOther->pev->absmax.y < pev->absmin.y ||
		 pOther->pev->absmax.z < pev->absmin.z )
		 return 0;
	return 1;
}

void CBaseEntity :: MakeDormant()
{
	SetBits( pev->flags, FL_DORMANT );
	
	// Don't touch
	pev->solid = SOLID_NOT;
	// Don't move
	pev->movetype = MOVETYPE_NONE;
	// Don't draw
	SetBits( pev->effects, EF_NODRAW );
	// Don't think
	DontThink();
	// Relink
	UTIL_SetOrigin( this, pev->origin );
}

int CBaseEntity :: IsDormant()
{
	return FBitSet( pev->flags, FL_DORMANT );
}

BOOL CBaseEntity :: IsInWorld()
{
	if (pev->origin.x >= 4096 || pev->origin.y >= 4096 || pev->origin.z >= 4096)
		return FALSE;

	if (pev->origin.x <= -4096 || pev->origin.y <= -4096 || pev->origin.z <= -4096)
		return FALSE;

	if (pev->velocity.x >= 2000 || pev->velocity.y >= 2000 || pev->velocity.z >= 2000)
		return FALSE;

	if (pev->velocity.x <= -2000 || pev->velocity.y <= -2000 || pev->velocity.z <= -2000)
		return FALSE;

	return TRUE;
}

BOOL CBaseEntity::ShouldToggle(USE_TYPE useType, BOOL currentState)
{
	if (useType != USE_TOGGLE && useType != USE_SET)
	{
		if ((currentState && useType == USE_ON) || (!currentState && useType == USE_OFF))
			return FALSE;
	}
	return TRUE;
}

BOOL CBaseEntity::ShouldToggle(USE_TYPE useType)
{
	STATE currentState = GetState();
	if (useType != USE_TOGGLE && useType != USE_SET)
	{
		switch (currentState)
		{
		case STATE_ON:
		case STATE_TURN_ON:
			if (useType == USE_ON) return FALSE;
			break;
		case STATE_OFF:
		case STATE_TURN_OFF:
			if (useType == USE_OFF) return FALSE;
			break;
		}
	}
	return TRUE;
}

int	CBaseEntity :: DamageDecal(int bitsDamageType )
{
	if ( pev->rendermode == kRenderTransAlpha )
		return -1;

	if ( pev->rendermode != kRenderNormal )
		return DECAL_BPROOF1;

	return DECAL_GUNSHOT1 + RANDOM_LONG(0,4);
}

void CBaseEntity::SetModel(edict_t* pev, const char* const pszModelName)
{
	if (!pszModelName || !(*pszModelName))
	{
		SET_MODEL(pev, "models/null.mdl");
		return;
	}

	//is this brush model?
	if (pszModelName[0] == '*')
	{
		SET_MODEL(pev, pszModelName);
		return;
	}

	//verify file exists
	if (g_pFileSystem->FileExists(pszModelName))
	{
		SET_MODEL(pev, pszModelName);
		return;
	}

	char* ext = UTIL_FileExtension(const_cast<char*>(pszModelName));
	if (FStrEq(ext, "mdl"))
	{
		//this is model
		SET_MODEL(pev, "models/error.mdl");
	}
	else if (FStrEq(ext, "spr"))
	{
		//this is sprite
		SET_MODEL(pev, "sprites/error.spr");
	}
	else
	{
		//set null model
		SET_MODEL(pev, "models/null.mdl");
	}
}

int CBaseEntity::PrecacheModel(const char* const pszModelName)
{
	if (!pszModelName || !*pszModelName)
	{
		ALERT(at_console, "Warning: modelname not specified\n");
		return g_sModelIndexNullModel; //set null model
	}
	
	//no need to precacahe brush
	if (pszModelName[0] == '*')
		return 0;

	//verify file exists
	if (g_pFileSystem->FileExists(pszModelName))
		return PRECACHE_MODEL(pszModelName);

	char* ext = UTIL_FileExtension(const_cast<char*>(pszModelName));

	if (FStrEq(ext, "mdl"))
	{
		//this is model
		ALERT(at_console, "Warning: model \"%s\" not found!\n", pszModelName);
		return g_sModelIndexErrorModel;
	}

	if (FStrEq(ext, "spr"))
	{
		//this is sprite
		ALERT(at_console, "Warning: sprite \"%s\" not found!\n", pszModelName);
		return g_sModelIndexErrorSprite;
	}

	//unknown format
	ALERT(at_console, "Warning: invalid name \"%s\"!\n", pszModelName);
	return g_sModelIndexNullModel; //set null model
}

int CBaseEntity::PrecacheSound(const char* const pszSoundName)
{
	if (!pszSoundName || !*pszSoundName)
		return g_sSoundIndexNullSound; //set null sound

	char path[256];	//g-cont.
	const char* sound = pszSoundName; //sounds from model events can contains a symbol '*'.
				//remove this for sucessfully loading a sound	
	if (sound[0] == '*')
		sound++;	//only for fake path, engine needs this prefix!
	
	sprintf(path, "sound/%s", sound);

	//verify file exists
	if (g_pFileSystem->FileExists(path))
		return PRECACHE_SOUND(pszSoundName);

	char* ext = UTIL_FileExtension(const_cast<char*>(pszSoundName));
	if (FStrEq(ext, "wav"))
	{
		//this is sound
		ALERT(at_console, "Warning: sound \"%s\" not found!\n", pszSoundName);
		return g_sSoundIndexNullSound; //set null sound
	}

	//unknown format
	ALERT(at_console, "Warning: invalid name \"%s\"!\n", pszSoundName);
	return g_sSoundIndexNullSound; //set null sound
}

unsigned short CBaseEntity::PrecacheEvent(int type, const char* psz)
{
	if(g_pFileSystem->FileExists(psz))
		return PRECACHE_EVENT(type, psz);

	ALERT(at_console, "Warning: event \"%s\" not found!\n", psz);
	return PRECACHE_EVENT(type, "events/null.sc");
}

// NOTE: szName must be a pointer to constant memory, e.g. "monster_class" because the entity
// will keep a pointer to it after this call.
CBaseEntity * CBaseEntity::Create( const char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner )
{
	edict_t* pent = CREATE_NAMED_ENTITY(MAKE_STRING(szName));
	
	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in Create!\n" );
		return NULL;
	}
	
	CBaseEntity* pEntity = Instance(pent);
	pEntity->pev->owner = pentOwner;
	pEntity->pev->origin = vecOrigin;
	pEntity->pev->angles = vecAngles;
	DispatchSpawn( pEntity->edict() );
	return pEntity;
}


