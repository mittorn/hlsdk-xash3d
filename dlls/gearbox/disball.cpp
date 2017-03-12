/***
*
*	Copyright (c) 2001, Valve LLC. All rights reserved.
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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"
#include "disball.h"
#include "customentity.h"
#include "decals.h"
#include "func_break.h"
#include "explode.h"

//=================================================
// Displacer Targets	
//=================================================
// Xen Target
LINK_ENTITY_TO_CLASS( info_displacer_xen_target, CPointEntity );
// Earth Target
LINK_ENTITY_TO_CLASS( info_displacer_earth_target, CPointEntity );

//=================================================
// Displacer Ball	
//=================================================

LINK_ENTITY_TO_CLASS( displacer_ball, CDisplacerBall );

CDisplacerBall *CDisplacerBall::BallCreate( void )
{
	CDisplacerBall *pBall = GetClassPtr( (CDisplacerBall *)NULL );
	pBall->pev->classname = MAKE_STRING("displacer_ball");
	pBall->m_fIsSelfTeleporter = FALSE;
	pBall->Spawn();

	return pBall;
}
CDisplacerBall *CDisplacerBall::SelfCreate( void )
{
	CDisplacerBall *pBall = GetClassPtr( (CDisplacerBall *)NULL );
	pBall->pev->classname = MAKE_STRING("displacer_ball");
	pBall->m_fIsSelfTeleporter = TRUE;
	pBall->Spawn();
	pBall->SelfRemove();

	return pBall;
}
void CDisplacerBall::Spawn( )
{
	Precache( );

	if ( !m_fIsSelfTeleporter )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->solid = SOLID_BBOX;
		pev->velocity = gpGlobals->v_forward * DISPLACER_BALL_SPEED;
	}

	SET_MODEL(ENT(pev), "sprites/exit1.spr");
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	if ( !m_fIsSelfTeleporter )
	{
		SetTouch( &CDisplacerBall::BallTouch );
		SetThink( &CDisplacerBall::FlyThink );
		pev->nextthink = gpGlobals->time + 0.05;
	}
}


void CDisplacerBall::Precache( )
{
	PRECACHE_MODEL("sprites/exit1.spr");
	m_iBeamSprite = PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/xflare1.spr");

	PRECACHE_SOUND("weapons/displacer_teleport.wav");
	PRECACHE_SOUND("weapons/displacer_teleport_player.wav");
	PRECACHE_SOUND("weapons/displacer_impact.wav");

	m_iDispRing = PRECACHE_MODEL ("sprites/disp_ring.spr");
}
int	CDisplacerBall :: Classify ( void )
{
	return	CLASS_PLAYER_BIOWEAPON;
}
void CDisplacerBall :: BallTouch ( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;
	Vector		vecSrc;
	pev->enemy = pOther->edict();
	CBaseEntity *pTarget = NULL;
	vecSpot = pev->origin - pev->velocity.Normalize() * 32;

	if ( FClassnameIs( pOther->pev, "monster_maria" ) 
		|| FClassnameIs( pOther->pev, "monster_boris" ) )
		return;

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, dont_ignore_monsters, ENT(pev), &tr );
	if( !LockRing )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z);
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z + 840);
			WRITE_SHORT( m_iDispRing );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 10 );
			WRITE_BYTE( 3 );
			WRITE_BYTE( 20 );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		if ( g_pGameRules->IsMultiplayer() )
		{
			if ( pOther->IsPlayer() )
			{
				for ( int i = RANDOM_LONG(1,5); i > 0; i-- )
				pTarget = UTIL_FindEntityByClassname(pTarget, "info_player_deathmatch" );

				if (pTarget)
				{
					UTIL_ScreenFade( pOther, Vector(0, 160, 0), 0.5, 0.5, 255, FFADE_IN );

					Vector tmp = pTarget->pev->origin;
					tmp.z -= pOther->pev->mins.z;
					tmp.z++;
					UTIL_SetOrigin( pOther->pev, tmp );

					pOther->pev->angles = pTarget->pev->angles;
					pOther->pev->velocity = pOther->pev->basevelocity = g_vecZero;

					CSprite *pSpr = CSprite::SpriteCreate( "sprites/xflare1.spr", vecSrc, TRUE );
					pSpr->AnimateAndDie( 6 );
					pSpr->SetTransparency(kRenderGlow, 184, 250, 214, 255, kRenderFxNoDissipation);

					EMIT_SOUND(ENT(pOther->pev), CHAN_WEAPON, "weapons/displacer_teleport_player.wav", 1, ATTN_NORM);
					
					vecSrc = pTarget->pev->origin;
					MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
						WRITE_BYTE(TE_DLIGHT);
						WRITE_COORD( vecSrc.x );	// X
						WRITE_COORD( vecSrc.y );	// Y
						WRITE_COORD( vecSrc.z );	// Z
						WRITE_BYTE( 24 );		// radius * 0.1
						WRITE_BYTE( 255 );		// r
						WRITE_BYTE( 180 );		// g
						WRITE_BYTE( 96 );		// b
						WRITE_BYTE( 20 );		// time * 10
						WRITE_BYTE( 0 );		// decay * 0.1
					MESSAGE_END( );
				}
			}
		}
		else if ( pOther->pev->flags & FL_MONSTER )
		{
			if ( pOther->pev->health <= 200 && !FClassnameIs( pOther->pev, "monster_nihilanth" )
				&& !FClassnameIs( pOther->pev, "monster_apache" ) && !FClassnameIs( pOther->pev, "monster_osprey" )
				&& !FClassnameIs( pOther->pev, "monster_superapache" ) && !FClassnameIs( pOther->pev, "monster_gargantua" )
				&& !FClassnameIs( pOther->pev, "monster_bigmomma" ))
			{
				pOther->Killed( pev, GIB_NEVER );
				pRemoveEnt = pOther;
			}
			else
			{
				pOther->TakeDamage( pev, pev, 100, DMG_ENERGYBEAM | DMG_NEVERGIB );
			}

		}

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(pev->origin.x);	// X
			WRITE_COORD(pev->origin.y);	// Y
			WRITE_COORD(pev->origin.z);	// Z
			WRITE_BYTE( 32 );		// radius * 0.1
			WRITE_BYTE( 255 );		// r
			WRITE_BYTE( 180 );		// g
			WRITE_BYTE( 96 );		// b
			WRITE_BYTE( 60 );		// time * 10
			WRITE_BYTE( 20 );		// decay * 0.1
		MESSAGE_END( );

		SetThink ( &CDisplacerBall::ExplodeThink );
		pev->nextthink = gpGlobals->time + 0.6;
	}

	LockRing = TRUE;
	pev->velocity = g_vecZero;
}
void CDisplacerBall :: SelfRemove ( void )
{
	pev->velocity = g_vecZero;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 640);
		WRITE_SHORT( m_iDispRing );
		WRITE_BYTE( 0 );	// startframe
		WRITE_BYTE( 10 );	// framerate
		WRITE_BYTE( 3 );	// life
		WRITE_BYTE( 20 );	// width
		WRITE_BYTE( 0 );	// noise
		WRITE_BYTE( 255 );	// r, g, b
		WRITE_BYTE( 255 );	// r, g, b
		WRITE_BYTE( 255 );	// r, g, b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 0 );	// speed
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 32 );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 180 );		// g
		WRITE_BYTE( 96 );		// b
		WRITE_BYTE( 60 );		// time * 10
		WRITE_BYTE( 20 );		// decay * 0.1
	MESSAGE_END( );

	SetThink ( &CDisplacerBall::ExplodeThink );
	pev->nextthink = gpGlobals->time + 0.3;
}
void CDisplacerBall::FlyThink( void )
{
	CBeam *pBeam;
	TraceResult tr;
	Vector vecDest;
	float flDist = 1.0;

	for (int i = 0; i < 10; i++)
	{
		Vector vecDir = Vector( RANDOM_FLOAT( -1.0, 1.0 ), RANDOM_FLOAT( -1.0, 1.0 ),RANDOM_FLOAT( -1.0, 1.0 ) );
		vecDir = vecDir.Normalize();
		TraceResult         tr1;
		UTIL_TraceLine( pev->origin, pev->origin + vecDir * 1024, ignore_monsters, ENT(pev), &tr1 );
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	};

	if ( flDist == 1.0 ) return;

	pBeam = CBeam::BeamCreate("sprites/lgtning.spr",200);
	pBeam->PointEntInit( tr.vecEndPos, entindex() );
	pBeam->SetStartPos( tr.vecEndPos );
	pBeam->SetEndEntity( entindex() );
	pBeam->SetColor( 96, 128, 16 );
	pBeam->SetNoise( 65 );
	pBeam->SetBrightness( 255 );
	pBeam->SetWidth( 30 );
	pBeam->SetScrollRate( 35 );
	pBeam->LiveForTime( 1 );

	pev->frame += 1; //animate teleball

	if(pev->frame > 24)
		pev->frame = fmod( pev->frame, 24 );

	pev->nextthink = gpGlobals->time + 0.05;
}

void CDisplacerBall::ExplodeThink( void )
{
	pev->effects |= EF_NODRAW;

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/displacer_teleport.wav", VOL_NORM, ATTN_NORM);

	if ( pRemoveEnt )
	{
		UTIL_Remove( pRemoveEnt );
	}

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;
		pev->owner = NULL;

	UTIL_Remove( this );
	::RadiusDamage( pev->origin, pev, pevOwner, gSkillData.plrDmgDisplacer,300 , CLASS_NONE, DMG_ENERGYBEAM );
}

//=======================
// Opposing Force Rope
//=======================