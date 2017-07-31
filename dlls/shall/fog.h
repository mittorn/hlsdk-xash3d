#ifndef ENVFOG_H__
#define ENVFOG_H__
#if defined(CLIENT_FOG)
#define FOG_ENTITY_CLASSNAME "env_fog"

struct FogConfig
{
	Vector color;
	int density;
	int startDistance;
	int endDistance;
	BOOL skybox;
};

extern int gmsgClientFog;

void ClientFog_RegisterMessage();
BOOL ClientFog_IsFogEntityActive(CBaseEntity* fogEntity);
void ClientFog_UpdateClientSide(CBaseEntity* fogEntity, CBaseEntity* client);

void UTIL_ClientFog(CBaseEntity* activator, BOOL active, FogConfig* config = NULL);
void UTIL_ClientFogToAll(BOOL active, FogConfig* config = NULL);
#endif // defined(CLIENT_FOG)
#endif // ENVFOG_H__
