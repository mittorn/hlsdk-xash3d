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
#ifndef DIS_BALL_H
#define DIS_BALL_H

//=================================================
// Displacer Ball	
//=================================================

#define DISPLACER_BALL_SPEED 500

class CDisplacerBall : public CBaseMonster
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT BallTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );
	void EXPORT FlyThink( void );
	void SelfRemove( void );

    BOOL LockRing;
	BOOL m_fIsSelfTeleporter;
	float m_flWaitFinished;
	int m_iDispRing;
	int m_iBeamSprite;
	CBaseEntity *pRemoveEnt;

public:
	static CDisplacerBall *BallCreate( void );
	static CDisplacerBall *SelfCreate( void );
};

#endif // CTF_ITEMS_H
