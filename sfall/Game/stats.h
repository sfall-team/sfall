/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#pragma once

namespace game
{

class Stats {
public:
	static void init();

	static int __stdcall trait_level(DWORD statID);
	static int __stdcall perk_level(fo::GameObject* source, DWORD perkID);

	static int __stdcall trait_adjust_stat(DWORD statID);
};

}
