//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "Exports.h"

#include "particleman.h"
#include "tri.h"
#include "particle_header.h"
#include <windows.h>
#include <gl/gl.h>
#include <gl/glaux.h>

class CException;
extern IParticleMan *g_pParticleMan;

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles()
{
//	RecClDrawNormalTriangles();

	gHUD.m_Spectator.DrawOverview();
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles()
{
//	RecClDrawTransparentTriangles();

	try {
		pParticleManager->UpdateSystems();
	}
	catch (CException* e) {
		e = nullptr;
		gEngfuncs.Con_Printf("There was a serious error within the particle engine. Particles will return on map change\n");
		delete pParticleManager;
		pParticleManager = nullptr;
	}

	if ( g_pParticleMan )
		 g_pParticleMan->Update();
}
