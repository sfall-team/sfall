/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma once

namespace game
{

class Stats {
public:
	static void init();

	static int trait_level(DWORD statID);
	static int perk_level(fo::GameObject* source, DWORD perkID);

	static int __stdcall trait_adjust_stat(DWORD statID);
};

}
