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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "fog.h"

#ifndef WRITE_BOOLEAN
#define WRITE_BOOLEAN(x) WRITE_BYTE((x != 0) ? 1 : 0)
#endif

static BOOL IsHalloweenModMapWithFog()
{
	return IsCurrentMap("hell") ||
		IsCurrentMap("styx") ||
		IsCurrentMap("witch") ||
		IsCurrentMap("woods");
}

int gmsgClientFog = 0;

//========================================
// Client fog entity
//========================================

#define SF_FOG_START_ACTIVE		0x01

class CClientFog : public CPointEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL IsFogActive() { return m_fogActive; }
	void UpdateClientSide(CBaseEntity* activator);

protected:
	virtual BOOL ShouldActivateFogOnSpawn();
	void NotifyFogUpdateToAllClients(BOOL forceClientDataUpdateImmediately = TRUE);
	void ActivateAndTurnOffPeers();

	int m_startDistance;
	int m_endDistance;
	int m_fogDensity;
	BOOL m_fogActive;
	BOOL m_fogSkybox;
};

LINK_ENTITY_TO_CLASS(env_fog, CClientFog);

TYPEDESCRIPTION	CClientFog::m_SaveData[] =
{
	DEFINE_FIELD(CClientFog, m_startDistance, FIELD_INTEGER),
	DEFINE_FIELD(CClientFog, m_endDistance, FIELD_INTEGER),
	DEFINE_FIELD(CClientFog, m_fogDensity, FIELD_INTEGER),
	DEFINE_FIELD(CClientFog, m_fogActive, FIELD_BOOLEAN),
	DEFINE_FIELD(CClientFog, m_fogSkybox, FIELD_BOOLEAN),
};
IMPLEMENT_SAVERESTORE(CClientFog, CPointEntity)

void CClientFog::Spawn(void)
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;							// Remove model & collisions
	pev->renderamt = 0;								// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;

	m_fogActive = ShouldActivateFogOnSpawn();

	if (m_fogActive)
		ActivateAndTurnOffPeers();

	if (IsHalloweenModMapWithFog())
		m_fogDensity = 5;
}

void CClientFog::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "startdist"))
	{
		m_startDistance = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "enddist"))
	{
		m_endDistance = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fogdensity"))
	{
		m_fogDensity = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fogskybox"))
	{
		m_fogSkybox = (atoi(pkvd->szValue) != 0) ? TRUE : FALSE;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CClientFog::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	switch (useType)
	{
	case USE_ON:
		m_fogActive = TRUE;
		break;
	case USE_OFF:
		m_fogActive = FALSE;
		break;
	case USE_TOGGLE:
		m_fogActive = !m_fogActive;
		break;
	case USE_SET:
	default:
		m_fogActive = static_cast<BOOL>(value);
		break;
	}

	if(m_fogActive)
		ActivateAndTurnOffPeers();

	NotifyFogUpdateToAllClients();
}

BOOL CClientFog::ShouldActivateFogOnSpawn()
{
	return (pev->spawnflags & SF_FOG_START_ACTIVE) != 0 || FStringNull(pev->targetname);
}

void CClientFog::NotifyFogUpdateToAllClients(BOOL forceClientDataUpdateImmediately)
{
	CBasePlayer* client = NULL;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		client = dynamic_cast<CBasePlayer*>(UTIL_PlayerByIndex(i));
		if (client != NULL)
		{
			client->TellClientToUpdateFog();

			if(forceClientDataUpdateImmediately)
				client->ForceClientDllUpdate();
		}
	}
}

void CClientFog::ActivateAndTurnOffPeers()
{
	CBaseEntity* peer = NULL;
	while ((peer = UTIL_FindEntityByClassname(peer, FOG_ENTITY_CLASSNAME)) != NULL)
	{
		if (peer != this)
		{
			CClientFog* fogPeer = dynamic_cast<CClientFog*>(peer);
			if (fogPeer != NULL)
				fogPeer->m_fogActive = FALSE;
		}
	}
}

void CClientFog::UpdateClientSide(CBaseEntity* activator)
{
	FogConfig fogConfig;
	fogConfig.color = pev->rendercolor;
	fogConfig.density = m_fogDensity;
	fogConfig.startDistance = m_startDistance;
	fogConfig.endDistance = m_endDistance;
	fogConfig.skybox = m_fogSkybox;

	UTIL_ClientFog(activator, m_fogActive, &fogConfig);
}

void ClientFog_RegisterMessage()
{
	gmsgClientFog = REG_USER_MSG("ClientFog", -1);
}

BOOL ClientFog_IsFogEntityActive(CBaseEntity* fogEntity)
{
	if (fogEntity == NULL || !FClassnameIs(fogEntity->pev, FOG_ENTITY_CLASSNAME))
		return FALSE;

	CClientFog* fog = dynamic_cast<CClientFog*>(fogEntity);
	if (fog)
		return fog->IsFogActive();

	return FALSE;
}

void ClientFog_UpdateClientSide(CBaseEntity* fogEntity, CBaseEntity* client)
{
	if (fogEntity == NULL || !FClassnameIs(fogEntity->pev, FOG_ENTITY_CLASSNAME))
		return;

	CClientFog* fog = dynamic_cast<CClientFog*>(fogEntity);
	if (fog)
		fog->UpdateClientSide(client);
}

void UTIL_ClientFog( CBaseEntity* activator, BOOL active, FogConfig* config)
{
	if (activator == NULL || !activator->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgClientFog, NULL, activator->edict());
		WRITE_BOOLEAN(active);
		if (active)
		{
			for(int i = 0; i < 3; i++)
				WRITE_BYTE(config->color[i]);
			WRITE_LONG(config->startDistance);
			WRITE_LONG(config->endDistance);
			WRITE_BYTE(config->density);
			WRITE_BOOLEAN(config->skybox);
		}
	MESSAGE_END();
}

void UTIL_ClientFogToAll(BOOL active, FogConfig* config)
{
	CBaseEntity* client = NULL;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		client = UTIL_PlayerByIndex(i);
		if (client != NULL)
			UTIL_ClientFog(client, active, config);
	}
}
#endif // defined(CLIENT_FOG)
