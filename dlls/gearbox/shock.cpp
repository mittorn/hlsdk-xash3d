/*
GEARBOX SOFTWARE
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "explode.h"
#include "shake.h"
#include "gamerules.h"
#include "effects.h"
#include "decals.h"
#include "soundent.h"
#include "customentity.h"

#define SHOCK_AIR_VELOCITY		20000

class CShockBeam : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT ShockTouch( CBaseEntity *pOther );
	void EXPORT FadeShock( void );
	void EXPORT ExplodeThink( void );
	void EXPORT BlastOn( void );
	void EXPORT BlastOff( void );
	void Glow( void );
	void GetAttachment ( int iAttachment, Vector &origin, Vector &angles );

	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;

	int m_iFadeCount;
	int m_iTrail;

	Vector m_vecForward;

	CBaseEntity *pShockedEnt;

public:
	static CShockBeam *ShockCreate( void );
};

LINK_ENTITY_TO_CLASS( shock_beam, CShockBeam );

void CShockBeam :: GetAttachment ( int iAttachment, Vector &origin, Vector &angles )
{
	GET_ATTACHMENT( ENT(pev), iAttachment, origin, angles );
}
CShockBeam *CShockBeam::ShockCreate( void )
{
	// Create a new entity with CShockBeam private data
	CShockBeam *pShock = GetClassPtr( (CShockBeam *)NULL );
	pShock->pev->classname = MAKE_STRING("shock_beam");
	pShock->Spawn();

	return pShock;
}

void CShockBeam::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/shock_effect.mdl");

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	BlastOn();
	Glow ();

	SetTouch( &CShockBeam::ShockTouch );

	if ( g_pGameRules->IsMultiplayer() )
		pev->dmg = 10;
	else
		pev->dmg = 10;

	UTIL_MakeAimVectors( pev->angles );

	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.1;
}


void CShockBeam::Precache( )
{
	PRECACHE_MODEL("models/shock_effect.mdl");

	PRECACHE_SOUND( "weapons/shock_fire.wav" );
	PRECACHE_SOUND( "weapons/shock_impact.wav" );

	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_MODEL( "sprites/glow02.spr" );
 	
}

int	CShockBeam :: Classify ( void )
{
	return	CLASS_NONE;
}

void CShockBeam::ShockTouch( CBaseEntity *pOther )
{
	if ( FClassnameIs( pOther->pev, "monster_maria" ) 
		|| FClassnameIs( pOther->pev, "monster_boris" ) )
		return;

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;

		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;

		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;

		UTIL_Remove( this );
		return;
	}

	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );

		if ( g_pGameRules->IsMultiplayer() )
			pOther->TraceAttack(pevOwner, 10, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM );
		else
			pOther->TraceAttack(pevOwner, 10,pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM );

		ApplyMultiDamage( pev, pevOwner );

		if ( pOther->pev->flags & FL_MONSTER | pOther->IsPlayer() )
		{
			pOther->pev->renderfx = kRenderFxGlowShell;
			pOther->pev->rendercolor.x = 0; // R
			pOther->pev->rendercolor.y = 255; // G
			pOther->pev->rendercolor.z = 255; // B
			pOther->pev->renderamt = 1;
			pShockedEnt = pOther;
		}

		SetThink ( &CShockBeam::FadeShock );
		pev->nextthink = gpGlobals->time + 1.0;

		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/shock_impact.wav", VOL_NORM, ATTN_NORM);
	}
		UTIL_Sparks( pev->origin );
		ExplodeThink();
}

void CShockBeam::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;
	
	BlastOff();
	iScale = 10;

	EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/shock_impact.wav", 1, ATTN_NORM);

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 6 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 255 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 10 );		// decay * 0.1
	MESSAGE_END( );


	entvars_t *pevOwner;

	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	TraceResult tr;

	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
	UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2));
}
void CShockBeam::BlastOff ( void )
{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;

		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;

		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;

}
void CShockBeam::FadeShock ( void )
{
	if ( pShockedEnt )
	{
		pShockedEnt->pev->renderfx = kRenderFxNone;
		pShockedEnt->pev->rendercolor.x = 0; // R
		pShockedEnt->pev->rendercolor.y = 0; // G
		pShockedEnt->pev->rendercolor.z = 0; // B
		pShockedEnt->pev->renderamt = 255;
	}
	UTIL_Remove( this );
}
void CShockBeam :: Glow( void )
{
	Vector		posGun, angleGun;
	int m_iAttachment = 1;
	GetAttachment( m_iAttachment, posGun, angleGun );

	m_pSprite = CSprite::SpriteCreate( "sprites/glow02.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), m_iAttachment );
	m_pSprite->pev->scale = 0.25;
	m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 125, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pSprite->pev->flags |= FL_SKIPLOCALHOST;
}
void CShockBeam::BlastOn ( void )
{
	Vector		posGun, angleGun;
	TraceResult trace;

	UTIL_MakeVectors( pev->angles );
	
	m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	GetAttachment( 1, posGun, angleGun );
	GetAttachment( 2, posGun, angleGun );

	Vector vecEnd = (gpGlobals->v_forward * 40) + posGun;
	UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &trace );

	m_pBeam->EntsInit( entindex(), entindex() );
	m_pBeam->SetStartAttachment( 1 );
	m_pBeam->SetEndAttachment( 2 );
	m_pBeam->SetBrightness( 190 );
	m_pBeam->SetScrollRate( 20 );
	m_pBeam->SetNoise( 20 );
	m_pBeam->DamageDecal( 1 );
	m_pBeam->SetFlags( BEAM_FSHADEOUT );
	m_pBeam->SetColor( 0, 255, 255 );
	m_pBeam->pev->spawnflags = SF_BEAM_TEMPORARY;

	m_pNoise = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	GetAttachment( 1, posGun, angleGun );
	GetAttachment( 2, posGun, angleGun );

	UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &trace );

	m_pNoise->EntsInit( entindex(), entindex() );
	m_pNoise->SetStartAttachment( 1 );
	m_pNoise->SetEndAttachment( 2 );
	m_pNoise->SetBrightness( 190 );
	m_pNoise->SetScrollRate( 20 );
	m_pNoise->SetNoise( 65 );
	m_pNoise->DamageDecal( 1 );
	m_pNoise->SetFlags( BEAM_FSHADEOUT );
	m_pNoise->SetColor( 255, 255, 173 );
	m_pNoise->pev->spawnflags = SF_BEAM_TEMPORARY;

	EXPORT void RelinkBeam();
}