/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#if defined(CLIENT_FOG)
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "clientfog.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) sizeof(x) / sizeof(x[0])
#endif

typedef int BOOL;

BOOL READ_BOOLEAN()
{
	return (READ_BYTE() != 0) ? TRUE : FALSE;
}

float C_ClientFog::ConvertFogDensityFromServerToClient(float serverFogDensity)
{
	return serverFogDensity * 0.0001f;
}

void C_ClientFog::Initialize()
{
	Reset();
}

void C_ClientFog::Reset()
{
	m_fogActive = m_fogSkybox = m_startDistance = m_endDistance = 0;
	m_fogDensity = 0;
	memset(m_fogColor, 0, ARRAYSIZE(m_fogColor));

	ResetFog();
}

void C_ClientFog::Update(const struct local_state_s* curstate)
{
	UpdatePlayerWaterLevelInfo(curstate);
}

BOOL C_ClientFog::CanDraw()
{
	if (!IsActive())
		return FALSE;

	// Under water.
	if (GetCurrentPlayerWaterLevel() >= 2)
		return FALSE;

	// Invalid draw distances.
	if (GetStartDistance() < 0 || GetEndDistance() < 0)
		return FALSE;

	return TRUE;
}

int C_ClientFog::ShouldUpdate()
{
	if (!IsActive())
		return FALSE;

	return TRUE;
}

void C_ClientFog::Render()
{
	gEngfuncs.pTriAPI->FogParams(GetFogDensity(), ShouldApplyFogToSkybox());
	gEngfuncs.pTriAPI->Fog(GetFogColor(), GetStartDistance(), GetEndDistance(), CanDraw());
}

void C_ClientFog::ResetFog()
{
	static float NullFogColor[] = {0, 0, 0};

	gEngfuncs.pTriAPI->FogParams(0, 0);
	gEngfuncs.pTriAPI->Fog(NullFogColor, 0, 0, FALSE);
}

void C_ClientFog::UpdatePlayerWaterLevelInfo(const struct local_state_s* curstate)
{
	m_currentPlayerWaterLevel = curstate->client.waterlevel;
}

int C_ClientFog::ReceiveFromServer(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_fogActive = READ_BOOLEAN();

	if (m_fogActive)
	{
		for (int i = 0; i < ARRAYSIZE(m_fogColor); i++)
			m_fogColor[i] = READ_BYTE();

		m_startDistance = READ_LONG();
		m_endDistance = READ_LONG();
		m_fogDensity = ConvertFogDensityFromServerToClient(READ_BYTE());
		m_fogSkybox = READ_BOOLEAN();
	}
	else
	{
		Reset();
	}

	return 1;
}

// Singleton.
static C_ClientFog gClientFog;

C_ClientFog* ClientFog() { return &gClientFog; }

void ClientFog_Initialize()
{
	if (ClientFog() != NULL)
		ClientFog()->Initialize();
}

void ClientFog_Reset()
{
	if (ClientFog() != NULL)
		ClientFog()->Reset();
}

void ClientFog_Update(const struct local_state_s* curstate)
{
	if (ClientFog() != NULL && ClientFog()->ShouldUpdate())
		ClientFog()->Update(curstate);
}

void ClientFog_Render()
{
	if (ClientFog() != NULL && ClientFog()->CanDraw())
		ClientFog()->Render();
}

int ClientFog_ReceiveFromServer(const char *pszName, int iSize, void *pbuf)
{
	if (ClientFog() != NULL)
		return ClientFog()->ReceiveFromServer(pszName, iSize, pbuf);

	return FALSE;
}
#endif // defined(CLIENT_FOG)
