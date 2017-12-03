/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include "main.h"
#include "Perks.h"
#include "ScriptExtender.h"

static void __declspec(naked) GetPerkOwed() {
	__asm {
		pushad;
		mov ecx, eax;
		movzx edx, byte ptr ds:[_free_perk];
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) SetPerkOwed() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		and eax, 0xff;
		cmp eax, 250;
		jg end;
		mov byte ptr ds:[_free_perk], al
end:
		popad
		retn;
	}
}
static void __declspec(naked) set_perk_freq() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push eax;
		call SetPerkFreq;
end:
		popad
		retn;
	}
}
static void __declspec(naked) GetPerkAvailable() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		cmp eax, PERK_count;
		jge fail;
		mov edx, eax;
		mov eax, ds:[_obj_dude];
		call perk_make_list_;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) funcSetPerkName() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
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
		call interpretGetString_;
		push eax;
		push esi;
		call SetPerkName;
		jmp end;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) funcSetPerkDesc() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
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
		call interpretGetString_;
		push eax;
		push esi;
		call SetPerkDesc;
		jmp end;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) funcSetPerkValue() {
	__asm {
		pushad;
		sub edx, 0x5e0-8; // offset of value into perk struct; edx = ((edx/4) - 0x178 + 0x8) * 4
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
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

static void __declspec(naked) fSetSelectablePerk() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;

		movzx eax, word ptr [esp+12];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr [esp+20];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr ds:[esp+4];
		cmp eax, VAR_TYPE_STR2;
		je next1;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next1:
		movzx eax, word ptr ds:[esp+28];
		cmp eax, VAR_TYPE_STR2;
		je next2;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next2:
		mov eax, ecx;
		mov edx, [esp+28];
		mov ebx, [esp+24];
		call interpretGetString_;
		push eax;
		mov eax, [esp+20];
		push eax;
		mov eax, [esp+16];
		push eax;
		mov eax, ecx;
		mov edx, [esp+16];
		mov ebx, [esp+12];
		call interpretGetString_;
		push eax;

		call SetSelectablePerk;
fail:
		add esp, 32;
		popad;
		retn;
	}
}
static void __declspec(naked) fSetFakePerk() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;

		movzx eax, word ptr [esp+12];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr [esp+20];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr ds:[esp+4];
		cmp eax, VAR_TYPE_STR2;
		je next1;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next1:
		movzx eax, word ptr ds:[esp+28];
		cmp eax, VAR_TYPE_STR2;
		je next2;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next2:
		mov eax, ecx;
		mov edx, [esp+28];
		mov ebx, [esp+24];
		call interpretGetString_;
		push eax;
		mov eax, [esp+20];
		push eax;
		mov eax, [esp+16];
		push eax;
		mov eax, ecx;
		mov edx, [esp+16];
		mov ebx, [esp+12];
		call interpretGetString_;
		push eax;

		call SetFakePerk;
fail:
		add esp, 32;
		popad;
		retn;
	}
}
static void __declspec(naked) fSetFakeTrait() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;

		movzx eax, word ptr [esp+12];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr [esp+20];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr ds:[esp+4];
		cmp eax, VAR_TYPE_STR2;
		je next1;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next1:
		movzx eax, word ptr ds:[esp+28];
		cmp eax, VAR_TYPE_STR2;
		je next2;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next2:
		mov eax, ecx;
		mov edx, [esp+28];
		mov ebx, [esp+24];
		call interpretGetString_;
		push eax;
		mov eax, [esp+20];
		push eax;
		mov eax, [esp+16];
		push eax;
		mov eax, ecx;
		mov edx, [esp+16];
		mov ebx, [esp+12];
		call interpretGetString_;
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
static void __declspec(naked) fSetPerkboxTitle() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
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
static void __declspec(naked) fIgnoreDefaultPerks() {
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
static void __declspec(naked) fRestoreDefaultPerks() {
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
static void __declspec(naked) fClearSelectablePerks() {
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
static void __declspec(naked) fHasFakePerk() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push eax;
		call HasFakePerk;
end:
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) fHasFakeTrait() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push eax;
		call HasFakeTrait;
end:
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) fAddPerkMode() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
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
static void __declspec(naked) remove_trait() {
	__asm {
		pushad;
		mov ebp, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		xor ebx, ebx;
		dec ebx;
		mov ecx, ds:[_pc_trait2];
		cmp eax, ds:[_pc_trait];
		jne next;
		mov ds:[_pc_trait], ecx;
		mov ds:[_pc_trait2], ebx;
		jmp end;
next:
		cmp eax, ds:[_pc_trait2];
		jne end;
		mov ds:[_pc_trait2], ebx;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) SetPyromaniacMod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
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

static void __declspec(naked) fApplyHeaveHoFix() {
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
static void __declspec(naked) SetSwiftLearnerMod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
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