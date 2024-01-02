/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "..\Modules\HookScripts\MiscHs.h"

#include "objects.h"

namespace game
{

namespace sf = sfall;

// Implementation of is_within_perception_ engine function with the HOOK_WITHINPERCEPTION hook
long Objects::is_within_perception(fo::GameObject* watcher, fo::GameObject* target, long hookType) {
	return sf::PerceptionRangeHook_Invoke(watcher, target, hookType, fo::func::is_within_perception(watcher, target));
}

// Alternative implementation of objFindObjPtrFromID_ engine function with the type of object to find
fo::GameObject* __fastcall Objects::FindObjectFromID(long id, long type) {
	fo::GameObject* obj = fo::func::obj_find_first();
	while (obj) {
		if (obj->id == id && obj->Type() == type) return obj;
		obj = fo::func::obj_find_next();
	}
	return nullptr;
}

//void Objects::init() {
//}

}
