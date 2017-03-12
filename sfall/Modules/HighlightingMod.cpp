#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "MainLoopHook.h"

#include "HighlightingMod.h"

namespace sfall
{

static DWORD highlightingToggled = 0;
static DWORD motionSensorMode;
static BYTE toggleHighlightsKey;
static DWORD highlightContainers;
static DWORD colorContainers;
static std::string highlightFailMsg1;
static std::string HighlightFailMsg2;

static void __declspec(naked) obj_outline_all_items_on() {
	__asm {
		pushad
		mov  eax, dword ptr ds:[FO_VAR_map_elevation]
		call fo::funcoffs::obj_find_first_at_
loopObject:
		test eax, eax
		jz   end
		cmp  eax, ds:[FO_VAR_outlined_object]
		je   nextObject
		xchg ecx, eax
		mov  eax, [ecx+0x20]
		and  eax, 0xF000000
		sar  eax, 0x18
		test eax, eax                             // This object is an item?
		jnz  nextObject                           // No
		cmp  dword ptr [ecx+0x7C], eax            // Owned by someone?
		jnz  nextObject                           // Yes
		test dword ptr [ecx+0x74], eax            // Already outlined?
		jnz  nextObject                           // Yes
		mov  edx, 0x10                            // yellow
		test byte ptr [ecx+0x25], dl              // NoHighlight_ flag is set (is this a container)?
		jz   NoHighlight                          // No
		cmp  highlightContainers, eax             // Highlight containers?
		je   nextObject                           // No
		mov  edx, colorContainers                // NR: should be set to yellow or purple later
NoHighlight:
		mov  [ecx+0x74], edx
nextObject:
		call fo::funcoffs::obj_find_next_at_
		jmp  loopObject
end:
		call fo::funcoffs::tile_refresh_display_
		popad
		retn
	}
}

static void __declspec(naked) obj_outline_all_items_off() {
	__asm {
		pushad
		mov  eax, dword ptr ds:[FO_VAR_map_elevation]
		call fo::funcoffs::obj_find_first_at_
loopObject:
		test eax, eax
		jz   end
		cmp  eax, ds:[FO_VAR_outlined_object]
		je   nextObject
		xchg ecx, eax
		mov  eax, [ecx+0x20]
		and  eax, 0xF000000
		sar  eax, 0x18
		test eax, eax                             // Is this an item?
		jnz  nextObject                           // No
		cmp  dword ptr [ecx+0x7C], eax            // Owned by someone?
		jnz  nextObject                           // Yes
		mov  dword ptr [ecx+0x74], eax
nextObject:
		call fo::funcoffs::obj_find_next_at_
		jmp  loopObject
end:
		call fo::funcoffs::tile_refresh_display_
		popad
		retn
	}
}

static void __declspec(naked) obj_remove_outline_hook() {
	__asm {
		call fo::funcoffs::obj_remove_outline_
		test eax, eax
		jnz  end
		cmp  highlightingToggled, 1
		jne  end
		mov  ds:[FO_VAR_outlined_object], eax
		call obj_outline_all_items_on
end:
		retn
	}
}

void ProcessMainLoop() {
	if (toggleHighlightsKey) {
		//0x48C294 to toggle
		if (KeyDown(toggleHighlightsKey)) {
			if (!highlightingToggled) {
				if (motionSensorMode&4) {
					fo::GameObject* scanner = fo::func::inven_pid_is_carried_ptr(fo::var::obj_dude, fo::PID_MOTION_SENSOR);
					if (scanner != nullptr) {
						if (motionSensorMode & 2) {
							highlightingToggled = fo::func::item_m_dec_charges(scanner) + 1;
							if (!highlightingToggled) {
								fo::func::display_print(HighlightFailMsg2.c_str());
							}
						} else highlightingToggled = 1;
					} else {
						fo::func::display_print(highlightFailMsg1.c_str());
					}
				} else {
					highlightingToggled = 1;
				}
				if (highlightingToggled) {
					obj_outline_all_items_on();
				} else {
					highlightingToggled = 2;
				}
			}
		} else if (highlightingToggled) {
			if (highlightingToggled == 1) {
				obj_outline_all_items_off();
			} 
			highlightingToggled = 0;
		}
	}
}

void HighlightingMod::init() {
	toggleHighlightsKey = GetConfigInt("Input", "ToggleItemHighlightsKey", 0);
	if (toggleHighlightsKey) {
		motionSensorMode = GetConfigInt("Misc", "MotionScannerFlags", 1);
		highlightContainers = GetConfigInt("Input", "HighlightContainers", 0);
		switch (highlightContainers) {
		case 1:
			colorContainers = 0x10; // yellow
			break;
		case 2:
			colorContainers = 0x40; // purple
			break;
		}
		//HookCall(0x44B9BA, &gmouse_bk_process_hook);
		HookCall(0x44BD1C, &obj_remove_outline_hook);
		HookCall(0x44E559, &obj_remove_outline_hook);

		highlightFailMsg1 = Translate("Sfall", "HighlightFail1", "You aren't carrying a motion sensor.");
		HighlightFailMsg2 = Translate("Sfall", "HighlightFail2", "Your motion sensor is out of charge.");

		MainLoopHook::onCombatLoop += ProcessMainLoop;
		MainLoopHook::onMainLoop += ProcessMainLoop;
	}
}

}
