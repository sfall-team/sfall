/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
