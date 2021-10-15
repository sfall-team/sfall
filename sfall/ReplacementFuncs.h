/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#pragma once

#include "Game\GUI\render.h"
#include "Game\GUI\Text.h"

#include "Game\combatAI.h"
#include "Game\inventory.h"
#include "Game\items.h"
#include "Game\skills.h"
#include "Game\stats.h"
#include "Game\tilemap.h"

__inline void InitReplacementHacks() {
	game::gui::Render::init();
	game::gui::Text::init();

	game::CombatAI::init();
	game::Inventory::init();
	game::Items::init();
	game::Skills::init();
	game::Stats::init();
	game::Tilemap::init();
}
