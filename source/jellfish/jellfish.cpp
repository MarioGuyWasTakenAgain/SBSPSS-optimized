/*=========================================================================

	jellfish.h

	Author:		CRB
	Created: 
	Project:	Spongebob
	Purpose: 

	Copyright (c) 2001 Climax Development Ltd

===========================================================================*/

#ifndef __JELLFISH_JELLFISH_H__
#include "jellfish\jellfish.h"
#endif

#ifndef __GAME_GAME_H__
#include	"game\game.h"
#endif

#ifndef __ENEMY_NSJBACK_H__
#include "enemy\nsjback.h"
#endif

#ifndef __ENEMY_NSJ2BACK_H__
#include "enemy\nsj2back.h"
#endif

#ifndef __ENEMY_NBUTTFLY_H__
#include "enemy\nbuttfly.h"
#endif

#ifndef __ENEMY_NPC_H__
#include "enemy\npc.h"
#endif

#ifndef	__UTILS_HEADER__
#include	"utils\utils.h"
#endif

#ifndef __VID_HEADER_
#include "system\vid.h"
#endif

#include	"game\game.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u8 CJellyfishGenerator::m_jellyfishCount;
s32 CJellyfishGenerator::m_timer;
u8 CJellyfishGenerator::m_on;
u8 CJellyfishGenerator::m_level;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CJellyfishGenerator::init()
{
	m_timer = 1 * GameState::getOneSecondInFrames();
	m_jellyfishCount = 0;

	m_on = true;
	m_level = 1;

	switch( CLevel::getCurrentChapter() )
	{
		case 1:
		{
			switch( CLevel::getCurrentChapterLevel() )
			{
				case 2:
				{
					m_level = 3;
					m_on = false;

					break;
				}

				case 4:
				{
					m_level = 2;

					break;
				}
			}

			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CJellyfishGenerator::think(int _frames, CLevel *level)
{
	if (m_on)
	{
		if (m_timer <= 0 && m_jellyfishCount < 10)
		{
			m_timer = GameState::getOneSecondInFrames();
			m_jellyfishCount++;

			DVECTOR const &offset = CLevel::getCameraPos();
			DVECTOR startPos;

			CNpcEnemy *enemy = nullptr;
			int npcType = CNpcEnemy::NPC_SMALL_JELLYFISH_BACKGROUND;

			if (m_level == 3)
			{
				npcType = CNpcEnemy::NPC_BUTTERFLY_BACKGROUND;
			}

			enemy = CNpcEnemy::Create(npcType);
			CNpcSmallJellyfishBackgroundEnemy *jfish = dynamic_cast<CNpcSmallJellyfishBackgroundEnemy *>(enemy);

			if (jfish)
			{
				if ((getRnd() % 10) > 4)
				{
					jfish->setTargetHeading(0);
					startPos.vx = offset.vx - 20;
				}
				else
				{
					jfish->setTargetHeading(2048);
					startPos.vx = offset.vx + VidGetScrW() + 20;
				}

				startPos.vy = offset.vy + (getRnd() % VidGetScrH());
				jfish->setStartPos(startPos.vx >> 4, startPos.vy >> 4);
				jfish->postInit();
				jfish->updateCollisionArea();
			}
		}
		else
		{
			m_timer -= _frames;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CJellyfishGenerator::decCounter()
{
	if ( m_jellyfishCount > 0 )
	{
		m_jellyfishCount--;
	}
}
