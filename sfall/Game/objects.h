/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma once

namespace game
{

class Objects {
public:
	//static void init();

	// Implementation of is_within_perception_ engine function with the HOOK_WITHINPERCEPTION hook
	static long is_within_perception(fo::GameObject* watcher, fo::GameObject* target, long hookType);

	// Alternative implementation of objFindObjPtrFromID_ engine function with the type of object to find
	static fo::GameObject* __fastcall FindObjectFromID(long id, long type);
};

}
