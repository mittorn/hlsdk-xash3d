#ifndef CLIENT_FOG_H__
#define CLIENT_FOG_H__
#if defined(CLIENT_FOG)
//
// Client fog class.
//
class C_ClientFog
{
public:
	virtual void Initialize();
	virtual void Reset();
	virtual void Update(const struct local_state_s* curstate);
	virtual int CanDraw();
	virtual int ShouldUpdate();
	virtual void Render();

	static float ConvertFogDensityFromServerToClient(float serverFogDensity);

	int IsActive() { return m_fogActive; }
	float GetFogDensity() { return m_fogDensity; }

	int ReceiveFromServer(const char *pszName, int iSize, void *pbuf);

protected:

	float* GetFogColor() { return m_fogColor; }
	int ShouldApplyFogToSkybox() { return m_fogSkybox; }
	float GetStartDistance() { return m_startDistance; }
	float GetEndDistance() { return m_endDistance; }
	int GetCurrentPlayerWaterLevel() { return m_currentPlayerWaterLevel; }

private:
	void ResetFog();
	void UpdatePlayerWaterLevelInfo(const struct local_state_s* curstate);

	int m_fogActive;
	int m_fogSkybox;
	float m_fogColor[3];
	float m_fogDensity;
	int m_startDistance;
	int m_endDistance;
	int m_currentPlayerWaterLevel;
};

// Client fog singleton.
C_ClientFog* ClientFog();

// Functions to ease use.
void ClientFog_Initialize();
void ClientFog_Reset();
void ClientFog_Update(const struct local_state_s* curstate);
void ClientFog_Render();
int ClientFog_ReceiveFromServer(const char *pszName, int iSize, void *pbuf);
#endif // defined(CLIENT_FOG)
#endif // CLIENT_FOG_H__
