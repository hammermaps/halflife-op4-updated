/*
	Copyright 2001 to 2004. The Battle Grounds Team and Contributors

	This file is part of the Battle Grounds Modification for Half-Life.

	The Battle Grounds Modification for Half-Life is free software;
	you can redistribute it and/or modify it under the terms of the
	GNU Lesser General Public License as published by the Free
	Software Foundation; either version 2.1 of the License, or
	(at your option) any later version.

	The Battle Grounds Modification for Half-Life is distributed in
	the hope that it will be useful, but WITHOUT ANY WARRANTY; without
	even the implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE.  See the GNU Lesser General Public License
	for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with The Battle Grounds Modification for Half-Life;
	if not, write to the Free Software Foundation, Inc., 59 Temple Place,
	Suite 330, Boston, MA  02111-1307  USA

	You must obey the GNU Lesser General Public License in all respects for
	all of the code used other than code distributed with the Half-Life
	SDK developed by Valve.  If you modify this file, you may extend this
	exception to your version of the file, but you are not obligated to do so.
	If you do not wish to do so, delete this exception statement from your
	version.
*/

// definition of the particle system manager

#include "..\hud.h"
#include "..\cl_util.h"
#include <string.h>
#include <stdio.h>
#include <windows.h>

#include "particle_header.h"
#include "event_api.h"
#include "eventscripts.h"
#include "r_efx.h"
#include "pm_shared.h"

CParticleSystemManager* pParticleManager = NULL;
cvar_t* g_ParticleCount;
cvar_t* g_ParticleDebug;
cvar_t* g_ParticleSorts;

// updates all systems
void CParticleSystemManager::UpdateSystems(void)
{
	CParticleSystem* pSystem = nullptr;
	signed int i = 0;
	signed int iSystems = (signed)m_pParticleSystems.size();
	// iterate through all the particle systems, drawing each
	for (; i < iSystems; i++)
	{
		pSystem = m_pParticleSystems[i];
		// remove the system if the system requests it
		if (pSystem && pSystem->DrawSystem() == false)
		{
			delete pSystem;
			pSystem = nullptr;
			m_pParticleSystems.erase((m_pParticleSystems.begin() + i));
			i--;
			iSystems--;
		}
	}

	// we couldn't return earlier as we need to have the sorting before the ps updating
	// however no sorting when we can't see the particles
	if (CheckDrawSystem() == false)
		return;

	// prepare opengl
	Particle_InitOpenGL();

	// declated variables we need for both unsorted and sorted
	int iParticles = m_pUnsortedParticles.size();
	float flTimeSinceLastDraw = TimeSinceLastDraw();
	int iDrawn = 0;

	// draw all unsorted particles first, so they are at the back of the screen.
	if (iParticles > 0) {
		// loop through all particles drawing them
		CParticle* pParticle = NULL;
		for (i = 0; i < iParticles; i++) {
			if (m_pUnsortedParticles[i]) {
				pParticle = m_pUnsortedParticles[i];
				if (pParticle && pParticle->Test()) {
					pParticle->Update(flTimeSinceLastDraw);

					// don't draw in certain spec modes
					if (g_iUser1 != OBS_MAP_FREE && g_iUser1 != OBS_MAP_CHASE) {
						// unfortunately we have to prepare every particle now
						// as we can't prepare for a batch of the same type anymore
						pParticle->Prepare();
						pParticle->Draw();
						iDrawn++;
					}
					// particle wants to die, so kill it
				}
				else {
					RemoveParticle(pParticle);
					i--;
					iParticles--;
				}
			}
		}
	}

	iParticles = m_pParticles.size();

	// sort and draw the sorted particles list
	if (iParticles > 0) {
		// calculate the fraction of a second between sorts
		float flTimeSinceLastSort = (gEngfuncs.GetClientTime() - m_flLastSort);
		// 1 / time between sorts will give us a number like 5
		// if it is less than the particlesorts cvar then it is a small value 
		// and therefore a long time since last sort
		if ((((int)(1 / flTimeSinceLastSort)) < g_ParticleSorts->value)) {
			m_flLastSort = gEngfuncs.GetClientTime();
			std::sort(m_pParticles.begin(), m_pParticles.end(), less_than);
		}

		// loop through all particles drawing them
		CParticle* pParticle = nullptr;
		for (i = 0; i < iParticles; i++) {
			if (m_pParticles[i]) {
				pParticle = m_pParticles[i];
				if (pParticle && pParticle->Test()) {
					pParticle->Update(flTimeSinceLastDraw);

					// don't draw in certain spec modes
					if (g_iUser1 != OBS_MAP_FREE && g_iUser1 != OBS_MAP_CHASE) {
						// unfortunately we have to prepare every particle now
						// as we can't prepare for a batch of the same type anymore
						pParticle->Prepare();
						pParticle->Draw();
						iDrawn++;
					}
					// particle wants to die, so kill it
				}
				else {
					RemoveParticle(pParticle);
					i--;
					iParticles--;
				}
			}
		}
	}

	// finished particle drawing
	Particle_FinishOpenGL();

	// print out how fast we've been drawing the systems in debug mode
	if (g_ParticleDebug->value != 0 && ((m_flLastDebug + 1) <= gEngfuncs.GetClientTime()))
	{
		gEngfuncs.Con_Printf("%i Particles Drawn this pass in %i systems %i Textures in Cache\n\0", iDrawn, m_pParticleSystems.size(), m_pTextures.size());
		m_flLastDebug = gEngfuncs.GetClientTime();
	}

	m_flLastDraw = gEngfuncs.GetClientTime();
}

void CParticleSystemManager::CreateDynamicLight(particle_system_management* pSystem, int radius, int red, int green, int blue, float decay)
{
	// create dynamic light
	dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
	VectorCopy(pSystem->vPosition, dl->origin);
	dl->radius = radius;
	dl->color.r = red;
	dl->color.g = green;
	dl->color.b = blue;
	dl->decay = decay;
	dl->die = (gEngfuncs.GetClientTime() + 0.1);
}

// handles all the present particle systems
void CParticleSystemManager::CreatePresetPS(unsigned int iPreset, particle_system_management* pSystem)
{
	switch (iPreset)
	{
	case iDefaultExplosion:
		gEngfuncs.Con_Printf("iPreset == iDefaultExplosion\n");

		if (pSystem == nullptr)
			return;

		// explositions are made up of 5 ps
		CreateMappedPS("particles/explosion/explo1_darksmoke.txt", pSystem);
		CreateMappedPS("particles/explosion/explo1_grounddust.txt", pSystem);
		CreateMappedPS("particles/explosion/explo1_firedust.txt", pSystem);
		CreateMappedPS("particles/explosion/explo1_default.txt", pSystem);
		CreateMappedPS("particles/explosion/explo1_shockwave.txt", pSystem);

		// play a sound as well
		switch (gEngfuncs.pfnRandomLong(0, 2))
		{
			case 0:
				gEngfuncs.pEventAPI->EV_PlaySound(0, pSystem->vPosition, 0, "explosions/explode_dist1.wav", 1.0, ATTN_NONE, 0, PITCH_NORM);
			break;
			case 1:
				gEngfuncs.pEventAPI->EV_PlaySound(0, pSystem->vPosition, 0, "explosions/explode_dist2.wav", 1.0, ATTN_NONE, 0, PITCH_NORM);
			break;
			case 2:
				gEngfuncs.pEventAPI->EV_PlaySound(0, pSystem->vPosition, 0, "explosions/explode_dist3.wav", 1.0, ATTN_NONE, 0, PITCH_NORM);
			break;
		}

		// create dynamic light
		CreateDynamicLight(pSystem, 500, 254, 160, 24, 0.2);
		break;
	case iDefaultBlood:
		gEngfuncs.Con_Printf("iPreset == iDefaultBlood\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/wallsmoke/e_impacts_blood.txt", pSystem);
		break;
	case iDefaultImpactWater:
		gEngfuncs.Con_Printf("iPreset == iDefaultImpactWater\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/impact/impact_water_drops.txt", pSystem);
		CreateMappedPS("particles/impact/impact_water_core.txt", pSystem);
		CreateMappedPS("particles/impact/impact_water_wave.txt", pSystem);
		break;
	case iDefaultExplosionWater:
		gEngfuncs.Con_Printf("iPreset == iDefaultExplosionWater\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/explosion/explo1_water_drops.txt", pSystem);
		CreateMappedPS("particles/explosion/explo1_water_core.txt", pSystem);
		CreateMappedPS("particles/explosion/explo1_water_wave.txt", pSystem);
		break;
	case iDefaultBloodRedPit:
		gEngfuncs.Con_Printf("iPreset == iDefaultBloodRedPit\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/wallsmoke/e_bloodpit_red.txt", pSystem);
		break;
	case iDefaultHitFleshRed:
		gEngfuncs.Con_Printf("iPreset == iDefaultHitFleshRed\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/wallsmoke/e_impact_flesh_human.txt", pSystem);
		CreateMappedPS("particles/wallsmoke/e_impact_flesh_human_core.txt", pSystem);
		break;
	case iDefaultHitFleshYellow:
		gEngfuncs.Con_Printf("iPreset == iDefaultHitFleshYellow\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/wallsmoke/e_impact_flesh_alien.txt", pSystem);
		CreateMappedPS("particles/wallsmoke/e_impact_flesh_alien_core.txt", pSystem);
		break;
	case iDefaultParticle:
		gEngfuncs.Con_Printf("iPreset == iDefaultParticle\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/wallsmoke/default_particle.txt", pSystem);
		break;
	case iDefaultImpactGlass:
		gEngfuncs.Con_Printf("iPreset == iDefaultImpactGlass\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/wallsmoke/e_impact_glass.txt", pSystem);
		break;
	case iDefaultImpactGrass:
		gEngfuncs.Con_Printf("iPreset == iDefaultImpactGrass\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/grass/hit_grass_core.txt", pSystem);
		CreateMappedPS("particles/grass/hit_grass.txt", pSystem);
		break;
	case iDefaultImpactBrown:
		gEngfuncs.Con_Printf("iPreset == iDefaultImpactBrown\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/brown/hit_brown_core.txt", pSystem);
		break;
	case iDefaultImpactCement:
		gEngfuncs.Con_Printf("iPreset == iDefaultImpactCement\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/cement/hit_cement.txt", pSystem);
		break;
	case iDefaultImpactWood:
		gEngfuncs.Con_Printf("iPreset == iDefaultImpactWood\n");

		CreateMappedPS("particles/wood/hit_wood.txt", pSystem);
		CreateMappedPS("particles/wood/hit_wood_core.txt", pSystem);

		//gEngfuncs.pEventAPI->EV_PlaySound(0, pSystem->vPosition, 0, "particles\\wood\\wood_solid_impact_bullet.wav", 0.15, ATTN_NONE, 0, PITCH_NORM);
		break;
	case iDefaultImpactSand:
		gEngfuncs.Con_Printf("iPreset == iDefaultImpactSand\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/sand/hit_sand.txt", pSystem);
		break;
	case iDefaultRocketTrail:
		gEngfuncs.Con_Printf("iPreset == iDefaultRocketTrail\n");

		if (pSystem == nullptr)
			return;

		CreateMappedPS("particles/rockettrail/trail.txt", pSystem);

		//gEngfuncs.pEventAPI->EV_PlaySound(0, pSystem->vPosition, 0, "particles\\wood\\wood_solid_impact_bullet.wav", 0.15, ATTN_NONE, 0, PITCH_NORM);
		break;
	}
}

// wrappers to create particle system's
// flintlock smoke ps
void CParticleSystemManager::CreateFlintPS(Vector vPosition)
{
	if (CheckDrawSystem() == false)
		return;

	AddSystem(new CFlintlockSmokeParticleSystem(vPosition));
}

// barrel smoke ps
void CParticleSystemManager::CreateBarrelPS(Vector vPosition, Vector vDirection)
{
	if (CheckDrawSystem() == false)
		return;

	AddSystem(new CBarrelSmokeParticleSystem(vPosition, vDirection));
}

// spark ps
void CParticleSystemManager::CreateSparkPS(Vector vPosition, Vector vDirection)
{
	if (CheckDrawSystem() == false)
		return;

	AddSystem(new CSparkParticleSystem(vPosition, vDirection));
}

// white smoke ps
void CParticleSystemManager::CreateWhitePS(Vector vPosition, Vector vDirection)
{
	if (CheckDrawSystem() == false)
		return;

	AddSystem(new CWhiteSmokeParticleSystem(vPosition, vDirection));
}

// brown smoke ps
void CParticleSystemManager::CreateBrownPS(Vector vPosition, Vector vDirection)
{
	if (CheckDrawSystem() == false)
		return;

	AddSystem(new CBrownSmokeParticleSystem(vPosition, vDirection));
}

// grass system
void CParticleSystemManager::CreateGrassPS(char* sFile, particle_system_management* pSystem)
{
	if (pSystem == nullptr) {
		return;
	}

	// no d3d/software
	if (IEngineStudio.IsHardware() == false)
		return;

	AddSystem(new CGrassParticleSystem(sFile, pSystem));
}

// mapped ps
void CParticleSystemManager::CreateMappedPS(char* sFile, particle_system_management* pSystem)
{
	if (pSystem == nullptr) {
		return;
	}

	// no d3d/software
	if (IEngineStudio.IsHardware() == false)
		return;

	gEngfuncs.Con_Printf("CMappedParticleSystem\n");

	AddSystem(new CMappedParticleSystem(sFile, pSystem));
}

// are we allowed to draw atm
bool CParticleSystemManager::CheckDrawSystem()
{
	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_ALL))
		return false;

	//	if(g_iUser1 == OBS_MAP_FREE || g_iUser1 == OBS_MAP_CHASE)
	//		return false;

		// no d3d/software
	if (IEngineStudio.IsHardware() == false)
		return false;

	return true;
}

// adds a new texture to out cache
// using a map would be preferable but you can't snprintf into the index
void CParticleSystemManager::AddTexture(char* sName, particle_texture_s* pTexture) {
	// create a new entry and then fill it with the values
	particle_texture_cache* pCacheEntry = new particle_texture_cache;
	_snprintf(pCacheEntry->sTexture, MAX_PARTICLE_PATH - 1, "%s\0", sName);
	pCacheEntry->pTexture = pTexture;

	// add the cache entry
	m_pTextures.push_back(pCacheEntry);
}

// check for a texture with the same path
particle_texture_s* CParticleSystemManager::HasTexture(char* sName) {

	unsigned int i = 0;
	unsigned int iTextures = m_pTextures.size();
	particle_texture_cache* pCacheEntry = nullptr;

	// loop through all cache entries, comparing stored path with parameter path
	for (; i < iTextures; i++)
	{
		pCacheEntry = m_pTextures[i];
		if (!stricmp(pCacheEntry->sTexture, sName)) {
			return pCacheEntry->pTexture;
		}
	}
	// return the texture if we've found it, otherwise null
	return NULL;
}

// cache the most used tgas, so we don't get lag on first firing the gun
void CParticleSystemManager::PrecacheTextures() {
	gEngfuncs.Con_Printf("Caching frequently used particles, this may take a few moments\n");
	LoadTGA(nullptr, const_cast<char*>(FLINTLOCK_SMOKE_PARTICLE));
	LoadTGA(nullptr, const_cast<char*>(BARREL_SMOKE_PARTICLES[0]));
	LoadTGA(nullptr, const_cast<char*>(BARREL_SMOKE_PARTICLES[1]));
	LoadTGA(nullptr, const_cast<char*>(BARREL_SMOKE_PARTICLES[2]));
	LoadTGA(nullptr, const_cast<char*>(BROWN_SMOKE_PARTICLE));
	gEngfuncs.Con_Printf("Finished caching frequently used particles, game loading will now continue\n");
}

// deletes all textures and their entries
void CParticleSystemManager::RemoveTextures() {

	unsigned int i = 0;
	unsigned int iTextures = m_pTextures.size();
	particle_texture_cache* pCacheEntry = nullptr;

	// loop through all valid entries, deleting all valid textures and entries
	for (; i < iTextures; i++) {
		pCacheEntry = m_pTextures[i];
		if (pCacheEntry) {
			if (pCacheEntry->pTexture && pCacheEntry->pTexture->imageData) {
				delete[] pCacheEntry->pTexture->imageData;
				pCacheEntry->pTexture->imageData = nullptr;
			}
			glDeleteTextures(1, pCacheEntry->pTexture->iID);
			delete pCacheEntry->pTexture;
			pCacheEntry->pTexture = nullptr;
		}
		_snprintf(pCacheEntry->sTexture, MAX_PARTICLE_PATH - 1, "\0");
		delete pCacheEntry;
		pCacheEntry = nullptr;
		m_pTextures.erase(m_pTextures.begin());
		i--;
		iTextures--;
	}

	m_pTextures.clear();
}

// adds a particle into the global particle tracker
void CParticleSystemManager::AddParticle(CParticle* pParticle) {
	if (pParticle->sParticle.bIgnoreSort == true) {
		m_pUnsortedParticles.push_back(pParticle);
	}
	else {
		m_pParticles.push_back(pParticle);
	}
	pParticle = nullptr;
}

// removes a particle from the global tracker and from the system
void CParticleSystemManager::RemoveParticle(CParticle* pParticle) {
	unsigned int i = 0;
	unsigned int iParticles = m_pParticles.size();

	// remove a particle from the sorted list
	for (; i < iParticles; i++) {
		if (pParticle == m_pParticles[i]) {
			delete m_pParticles[i];
			pParticle = nullptr;
			m_pParticles.erase(m_pParticles.begin() + i);
			i--;
			iParticles--;
			return;
		}
	}

	// remove a particle from the unsorted list
	iParticles = m_pUnsortedParticles.size();
	for (i = 0; i < iParticles; i++) {
		if (pParticle == m_pUnsortedParticles[i]) {
			delete m_pUnsortedParticles[i];
			pParticle = nullptr;
			m_pUnsortedParticles.erase(m_pUnsortedParticles.begin() + i);
			i--;
			iParticles--;
			return;
		}
	}
}

// remove all trackers in the system
void CParticleSystemManager::RemoveParticles() {
	unsigned int i = 0;
	unsigned int iParticles = m_pParticles.size();

	// remove the sorted particles
	for (i = 0; i < iParticles; i++) {
		delete m_pParticles[i];
		m_pParticles[i] = nullptr;
		m_pParticles.erase(m_pParticles.begin() + i);
		i--;
		iParticles--;
	}
	m_pParticles.clear();

	// remove the unsorted particles
	iParticles = m_pUnsortedParticles.size();
	for (i = 0; i < iParticles; i++) {
		delete m_pUnsortedParticles[i];
		m_pUnsortedParticles[i] = nullptr;
		m_pUnsortedParticles.erase(m_pUnsortedParticles.begin() + i);
		i--;
		iParticles--;
	}
	m_pUnsortedParticles.clear();
}


// adds a new system
void CParticleSystemManager::AddSystem(CParticleSystem* pSystem) {
	m_pParticleSystems.push_back(pSystem);
}

// tbh highly inefficent but we shouldn't have any large number of ps's,
// and we won't be force removing very often so this won't be too bad
void CParticleSystemManager::RemoveSystem(unsigned int iSystem)
{
	unsigned int i = 0;
	unsigned int iParticles = m_pParticles.size();
	CParticle* pParticle = nullptr;
	// remove the sorted particles
	for (i = 0; i < iParticles; i++) {
		pParticle = m_pParticles[i];
		if (pParticle && pParticle->SystemID() == iSystem) {
			delete pParticle;
			pParticle = nullptr;
			m_pParticles.erase(m_pParticles.begin() + i);
			i--;
			iParticles--;
		}
	}

	// remove the unsorted particles
	iParticles = m_pUnsortedParticles.size();
	for (i = 0; i < iParticles; i++) {
		pParticle = m_pUnsortedParticles[i];
		if (pParticle && pParticle->SystemID() == iSystem) {
			delete pParticle;
			pParticle = nullptr;
			m_pUnsortedParticles.erase(m_pUnsortedParticles.begin() + i);
			i--;
			iParticles--;
		}
	}

	CParticleSystem* pSystem = nullptr;
	unsigned int iSystems = m_pParticleSystems.size();
	for (; i < iSystems; i++)
	{
		pSystem = m_pParticleSystems[i];
		// i != the system id, as the server or the client can generate these
		if (pSystem && pSystem->SystemID() == iSystem) {
			delete pSystem;
			pSystem = nullptr;
			m_pParticleSystems.erase(m_pParticleSystems.begin() + i);
			i--;
			iSystems--;
		}
	}
}


// deletes all systems
void CParticleSystemManager::RemoveSystems()
{
	unsigned int i = 0;
	unsigned int iSystems = m_pParticleSystems.size();
	for (; i < iSystems; i++) {
		delete m_pParticleSystems[i];
		m_pParticleSystems[i] = nullptr;
		m_pParticleSystems.erase(m_pParticleSystems.begin() + i);
		i--;
		iSystems--;
	}
	m_pParticleSystems.clear();
}