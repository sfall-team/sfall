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

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "..\LoadGameHook.h"

#include "ObjectName.h"

namespace sfall
{

static std::unordered_map<int, std::string> overrideScrName;

static long lastNameScrIdx = -1;
static long lastNameSid = -1;
static long lastItemPid = -1;

void ObjectName::SetName(long sid, const char* name) {
	if (sid == lastNameSid) lastNameSid = -1;
	if (!name) name = "";
	overrideScrName.emplace(sid, name);
}

const char* __stdcall ObjectName::GetName(fo::GameObject* object) {
	if (!overrideScrName.empty()) {
		const auto &name = overrideScrName.find(object->scriptId);
		if (name != overrideScrName.cend()) {
			return (name->second.length() > 0)
			       ? name->second.c_str()
			       : fo::func::proto_get_msg_info(object->protoId, 0);
		}
	}
	return nullptr;
}

static __declspec(naked) void critter_name_hack() {
	static DWORD critter_name_hack_ret = 0x42D125;
	using namespace fo::Fields;
	__asm {
		push ebx; // object
		call ObjectName::GetName;
		test eax, eax;
		jnz  override;
		mov  edi, [ebx + scriptIndex];
		retn;
override:
		add  esp, 4;
		jmp  critter_name_hack_ret;
	}
}

static __declspec(naked) void critter_name_hack_check() {
	static DWORD critter_name_hack_ret = 0x42D12F;
	using namespace fo::Fields;
	__asm {
		mov  ecx, [ebx + scriptId];
		cmp  ecx, -1;
		je   checkScrIdx; // has no script, check the script index instead
		cmp  ecx, lastNameSid;
		jne  default;
		add  esp, 4;
		mov  eax, ds:[FO_VAR_name_critter];
		jmp  critter_name_hack_ret;
checkScrIdx:
		mov  ecx, [ebx + scriptIndex];
		cmp  ecx, -1; // no inherited script index
		je   end;
		cmp  ecx, lastNameScrIdx;
		jne  end;
		add  esp, 4;
		mov  eax, ds:[FO_VAR_name_critter];
		jmp  critter_name_hack_ret;
default:
		mov  ecx, [ebx + scriptIndex];
end:
		retn;
	}
}

static __declspec(naked) void critter_name_hack_end() {
	using namespace fo::Fields;
	__asm {
		mov  edx, [ebx + scriptIndex];
		mov  lastNameScrIdx, edx;
		mov  ecx, [ebx + scriptId];
		mov  lastNameSid, ecx;
		retn;
	}
}

static __declspec(naked) void item_name_hook() {
	__asm {
		cmp  eax, lastItemPid;
		je   getLast;
		mov  lastItemPid, eax;
		jmp  fo::funcoffs::proto_name_;
getLast:
		mov  eax, ds:[FO_VAR_name_item];
		retn;
	}
}

static void Reset() {
	overrideScrName.clear();
	lastNameSid = -1;
}

void ObjectName::init() {
	// Returns the redefined object name
	MakeCall(0x42D0F2, critter_name_hack, 1);

	// Tweak for quickly getting last object name
	MakeCall(0x42D0C4, critter_name_hack_check, 1);
	MakeCall(0x42D12A, critter_name_hack_end);
	HookCall(0x477AE7, item_name_hook);

	LoadGameHook::OnBeforeMapLoad() += Reset;
	LoadGameHook::OnGameReset() += []() {
		Reset();
		lastNameScrIdx = -1;
		lastItemPid = -1;
	};
}

}