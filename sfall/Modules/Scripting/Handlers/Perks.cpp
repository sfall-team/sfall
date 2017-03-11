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

#include "Perks.h"

namespace sfall
{
namespace script
{
using namespace fo;

void __declspec(naked) op_get_perk_owed() {
	__asm {
		pushad;
		mov ecx, eax;
		movzx edx, byte ptr ds : [FO_VAR_free_perk];
		call fo::funcoffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_set_perk_owed() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		and eax, 0xff;
		cmp eax, 250;
		jg end;
		mov byte ptr ds : [FO_VAR_free_perk], al
			end :
		popad
			retn;
	}
}

void __declspec(naked) op_set_perk_freq() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		call SetPerkFreq;
end:
		popad
			retn;
	}
}

void __declspec(naked) op_get_perk_available() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz fail;
		cmp eax, PERK_count;
		jge fail;
		mov edx, eax;
		mov eax, ds:[FO_VAR_obj_dude];
		call fo::funcoffs::perk_make_list_;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx;
		call fo::funcoffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
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
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0x9001;
		jz next;
		cmp si, 0x9801;
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
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0x9001;
		jz next;
		cmp si, 0x9801;
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
		cmp dx, 0xC001;
		jnz fail;
		cmp si, 0xC001;
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
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr[esp + 20];
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr ds : [esp + 4];
		cmp eax, 0x9001;
		je next1;
		cmp eax, 0x9801;
		jne fail;
next1:
		movzx eax, word ptr ds : [esp + 28];
		cmp eax, 0x9001;
		je next2;
		cmp eax, 0x9801;
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
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr[esp + 20];
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr ds : [esp + 4];
		cmp eax, 0x9001;
		je next1;
		cmp eax, 0x9801;
		jne fail;
next1:
		movzx eax, word ptr ds : [esp + 28];
		cmp eax, 0x9001;
		je next2;
		cmp eax, 0x9801;
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
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr[esp + 20];
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr ds : [esp + 4];
		cmp eax, 0x9001;
		je next1;
		cmp eax, 0x9801;
		jne fail;
next1:
		movzx eax, word ptr ds : [esp + 28];
		cmp eax, 0x9001;
		je next2;
		cmp eax, 0x9801;
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
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
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
		push ebx;
		push ecx;
		push edx;
		call IgnoreDefaultPerks;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_show_real_perks() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		call RestoreDefaultPerks;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_clear_selectable_perks() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		call ClearSelectablePerks;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_has_fake_perk() {
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
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push eax;
		call HasFakePerk;
end:
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov eax, edi;
		mov edx, 0xc001;
		call fo::funcoffs::interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
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
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
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
		mov edx, 0xc001;
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
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		call AddPerkMode;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_remove_trait() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		xor ebx, ebx;
		dec ebx;
		mov ecx, ds:[FO_VAR_pc_trait + 4];
		cmp eax, ds:[FO_VAR_pc_trait];
		jne next;
		mov ds : [FO_VAR_pc_trait], ecx;
		mov ds : [FO_VAR_pc_trait + 4], ebx;
		jmp end;
next:
		cmp eax, ds : [FO_VAR_pc_trait + 4];
		jne end;
		mov ds : [FO_VAR_pc_trait + 4], ebx;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_set_pyromaniac_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		push 0x424AB6;
		call SafeWrite8;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_apply_heaveho_fix() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		call ApplyHeaveHoFix;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_swiftlearner_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		push 0x4AFAE2;
		call SafeWrite32;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

}
}
