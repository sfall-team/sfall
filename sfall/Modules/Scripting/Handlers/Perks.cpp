/*
 *    sfall
 *    Copyright (C) 2008-2016  The sfall team
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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\Perks.h"
#include "..\..\ScriptExtender.h"
#include "..\OpcodeContext.h"

#include "Perks.h"

namespace sfall
{
namespace script
{
using namespace fo;

void __declspec(naked) op_get_perk_owed() {
	__asm {
		push edx;
		push ecx;
		movzx edx, byte ptr ds:[FO_VAR_free_perk];
		_RET_VAL_INT(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

void __declspec(naked) op_set_perk_owed() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		and  eax, 0xFF;
		cmp  eax, 250;
		jg   end;
		mov  byte ptr ds:[FO_VAR_free_perk], al;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

void __declspec(naked) op_set_perk_freq() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		push eax;
		call SetPerkFreq;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

void sf_get_perk_available(OpcodeContext& ctx) {
	int result = 0, perkId = ctx.arg(0).rawValue();
	if (perkId >= 0 && perkId < 256) { // start fake id
		result = fo::func::perk_can_add(fo::var::obj_dude, perkId);
	}
	ctx.setReturn(result);
}

void __declspec(naked) op_set_perk_name() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_STR2;
		jz next;
		cmp si, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, edi;
		mov edx, esi;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretGetString_;
		push eax;
		push esi;
		call SetPerkName;
		jmp end;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_set_perk_desc() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_STR2;
		jz next;
		cmp si, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, edi;
		mov edx, esi;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretGetString_;
		push eax;
		push esi;
		call SetPerkDesc;
		jmp end;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_set_perk_value() {
	__asm {
		pushad;
		sub edx, 0x5e0 - 8; // offset of value into perk struct; edx = ((edx/4) - 0x178 + 0x8) * 4
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		cmp si, VAR_TYPE_INT;
		jnz fail;
		push edi;
		push eax;
		call SetPerkValue;
		jmp end;
fail:
		add esp, 4;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_set_selectable_perk() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;

		movzx eax, word ptr[esp + 12];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr[esp + 20];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr ds:[esp + 4];
		cmp eax, VAR_TYPE_STR2;
		je next1;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next1:
		movzx eax, word ptr ds:[esp + 28];
		cmp eax, VAR_TYPE_STR2;
		je next2;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next2:
		mov eax, ecx;
		mov edx, [esp + 28];
		mov ebx, [esp + 24];
		call fo::funcoffs::interpretGetString_;
		push eax;
		mov eax, [esp + 20];
		push eax;
		mov eax, [esp + 16];
		push eax;
		mov eax, ecx;
		mov edx, [esp + 16];
		mov ebx, [esp + 12];
		call fo::funcoffs::interpretGetString_;
		push eax;

		call SetSelectablePerk;
fail:
		add esp, 32;
		popad;
		retn;
	}
}

void __declspec(naked) op_set_fake_perk() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;

		movzx eax, word ptr[esp + 12];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr[esp + 20];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr ds:[esp + 4];
		cmp eax, VAR_TYPE_STR2;
		je next1;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next1:
		movzx eax, word ptr ds:[esp + 28];
		cmp eax, VAR_TYPE_STR2;
		je next2;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next2:
		mov eax, ecx;
		mov edx, [esp + 28];
		mov ebx, [esp + 24];
		call fo::funcoffs::interpretGetString_;
		push eax;
		mov eax, [esp + 20];
		push eax;
		mov eax, [esp + 16];
		push eax;
		mov eax, ecx;
		mov edx, [esp + 16];
		mov ebx, [esp + 12];
		call fo::funcoffs::interpretGetString_;
		push eax;

		call SetFakePerk;
fail:
		add esp, 32;
		popad;
		retn;
	}
}

void __declspec(naked) op_set_fake_trait() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		push eax;

		movzx eax, word ptr[esp + 12];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr[esp + 20];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr ds:[esp + 4];
		cmp eax, VAR_TYPE_STR2;
		je next1;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next1:
		movzx eax, word ptr ds:[esp + 28];
		cmp eax, VAR_TYPE_STR2;
		je next2;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next2:
		mov eax, ecx;
		mov edx, [esp + 28];
		mov ebx, [esp + 24];
		call fo::funcoffs::interpretGetString_;
		push eax;
		mov eax, [esp + 20];
		push eax;
		mov eax, [esp + 16];
		push eax;
		mov eax, ecx;
		mov edx, [esp + 16];
		mov ebx, [esp + 12];
		call fo::funcoffs::interpretGetString_;
		push eax;

		call SetFakeTrait;
fail:
		add esp, 32;
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_perkbox_title() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push eax;
		call SetPerkboxTitle;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_hide_real_perks() {
	__asm {
		push ecx;
		call IgnoreDefaultPerks;
		pop  ecx;
		retn;
	}
}

void __declspec(naked) op_show_real_perks() {
	__asm {
		push ecx;
		call RestoreDefaultPerks;
		pop  ecx;
		retn;
	}
}

void __declspec(naked) op_clear_selectable_perks() {
	__asm {
		push ecx;
		call ClearSelectablePerks;
		pop  ecx;
		retn;
	}
}

void sf_has_fake_perk(OpcodeContext& ctx) {
	ctx.setReturn(HasFakePerk(ctx.arg(0).asString(), ctx.arg(0).asInt()));
}

void __declspec(naked) op_has_fake_trait() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push eax;
		call HasFakeTrait;
end:
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_perk_add_mode() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		push eax;
		call AddPerkMode;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_remove_trait() {
	__asm {
		push ecx;
		_GET_ARG_INT(end);
		test eax, eax;
		jl   end;
		mov  edx, -1;
		cmp  eax, ds:[FO_VAR_pc_trait];
		jne  next;
		mov  ecx, ds:[FO_VAR_pc_trait2];
		mov  ds:[FO_VAR_pc_trait], ecx;
		mov  ds:[FO_VAR_pc_trait2], edx;
end:
		pop  ecx;
		retn;
next:
		cmp  eax, ds:[FO_VAR_pc_trait2];
		jne  end;
		mov  ds:[FO_VAR_pc_trait2], edx;
		pop  ecx;
		retn;
	}
}

void __declspec(naked) op_set_pyromaniac_mod() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		push eax;
		push 0x424AB6;
		call SafeWrite8;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_apply_heaveho_fix() {
	__asm {
		push ecx;
		call ApplyHeaveHoFix;
		pop  ecx;
		retn;
	}
}

void __declspec(naked) op_set_swiftlearner_mod() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		push eax;
		push 0x4AFAE2;
		call SafeWrite32;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

}
}
