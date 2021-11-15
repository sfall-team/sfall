/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#pragma once

#include "Game\combatAI.h"
#include "Game\inventory.h"
#include "Game\items.h"
#include "Game\render.h"
#include "Game\skills.h"
#include "Game\stats.h"
#include "Game\tilemap.h"

__inline void InitReplacementHacks() {
	game::CombatAI::init();
	game::Inventory::init();
	game::Items::init();
	game::Render::init();
	game::Skills::init();
	game::Stats::init();
	game::Tilemap::init();
}
