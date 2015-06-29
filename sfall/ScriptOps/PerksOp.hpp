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
		movzx edx, byte ptr ds:[0x570A29];
		call SetResult;
		mov edx, 0xc001;
		mov eax, ecx;
		call SetResultType;
		popad;
		retn;
	}
}
static void __declspec(naked) SetPerkOwed() {
	__asm {
		pushad;
		mov ecx, eax;
		call GetArgType;
		mov edx, eax;
		mov eax, ecx;
		call GetArg;
		cmp dx, 0xC001;
		jnz end;
		and eax, 0xff;
		cmp eax, 250;
		jg end;
		mov byte ptr ds:[0x570A29], al
end:
		popad
		retn;
	}
}
static void __declspec(naked) set_perk_freq() {
	__asm {
		pushad;
		mov ecx, eax;
		call GetArgType;
		mov edx, eax;
		mov eax, ecx;
		call GetArg;
		cmp dx, 0xC001;
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
		call GetArgType;
		mov edx, eax;
		mov eax, ecx;
		call GetArg;
		cmp dx, 0xC001;
		jnz fail;
		cmp eax, 119;
		jge fail;
		mov edx, eax;
		mov eax, 0x006610B8;
		mov ebx, 0x00496B44;
		call ebx;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx
		call SetResult;
		mov edx, 0xc001;
		mov eax, ecx;
		call SetResultType;
		popad;
		retn;
	}
}
static void __declspec(naked) funcSetPerkName() {
	__asm {
		pushad;
		mov ecx, eax;
		call GetArgType;
		mov esi, eax;
		mov eax, ecx;
		call GetArg;
		mov edi, eax;
		mov eax, ecx;
		call GetArgType;
		mov edx, eax;
		mov eax, ecx;
		call GetArg;
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
		mov edi, 0x4678E0;
		call edi;
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
		call GetArgType;
		mov esi, eax;
		mov eax, ecx;
		call GetArg;
		mov edi, eax;
		mov eax, ecx;
		call GetArgType;
		mov edx, eax;
		mov eax, ecx;
		call GetArg;
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
		mov edi, 0x4678E0;
		call edi;
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
		call GetArgType;
		mov esi, eax;
		mov eax, ecx;
		call GetArg;
		mov edi, eax;
		mov eax, ecx;
		call GetArgType;
		mov edx, eax;
		mov eax, ecx;
		call GetArg;
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

static void __declspec(naked) fSetSelectablePerk() {
	__asm {
		pushad;
		mov ecx, eax;
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;
		mov eax, ecx;
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;
		mov eax, ecx;
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;
		mov eax, ecx;
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;

		movzx eax, word ptr [esp+12];
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr [esp+20];
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr ds:[esp+4];
		cmp eax, 0x9001;
		je next1;
		cmp eax, 0x9801;
		jne fail;
next1:
		movzx eax, word ptr ds:[esp+28];
		cmp eax, 0x9001;
		je next2;
		cmp eax, 0x9801;
		jne fail;
next2:
		mov eax, ecx;
		mov edx, [esp+28];
		mov ebx, [esp+24];
		call GetStringVar;
		push eax;
		mov eax, [esp+20];
		push eax;
		mov eax, [esp+16];
		push eax;
		mov eax, ecx;
		mov edx, [esp+16];
		mov ebx, [esp+12];
		call GetStringVar;
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
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;
		mov eax, ecx;
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;
		mov eax, ecx;
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;
		mov eax, ecx;
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;

		movzx eax, word ptr [esp+12];
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr [esp+20];
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr ds:[esp+4];
		cmp eax, 0x9001;
		je next1;
		cmp eax, 0x9801;
		jne fail;
next1:
		movzx eax, word ptr ds:[esp+28];
		cmp eax, 0x9001;
		je next2;
		cmp eax, 0x9801;
		jne fail;
next2:
		mov eax, ecx;
		mov edx, [esp+28];
		mov ebx, [esp+24];
		call GetStringVar;
		push eax;
		mov eax, [esp+20];
		push eax;
		mov eax, [esp+16];
		push eax;
		mov eax, ecx;
		mov edx, [esp+16];
		mov ebx, [esp+12];
		call GetStringVar;
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
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;
		mov eax, ecx;
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;
		mov eax, ecx;
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;
		mov eax, ecx;
		call GetArgType;
		push eax;
		mov eax, ecx;
		call GetArg;
		push eax;

		movzx eax, word ptr [esp+12];
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr [esp+20];
		cmp eax, 0xc001;
		jne fail;
		movzx eax, word ptr ds:[esp+4];
		cmp eax, 0x9001;
		je next1;
		cmp eax, 0x9801;
		jne fail;
next1:
		movzx eax, word ptr ds:[esp+28];
		cmp eax, 0x9001;
		je next2;
		cmp eax, 0x9801;
		jne fail;
next2:
		mov eax, ecx;
		mov edx, [esp+28];
		mov ebx, [esp+24];
		call GetStringVar;
		push eax;
		mov eax, [esp+20];
		push eax;
		mov eax, [esp+16];
		push eax;
		mov eax, ecx;
		mov edx, [esp+16];
		mov ebx, [esp+12];
		call GetStringVar;
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
		mov ebx, 0x4674F0;
		call ebx;
		mov edx, eax;
		mov eax, edi;
		mov ebx, 0x467500;
		call ebx;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		mov ecx, GetStringVar;
		call ecx;
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
		call GetArgType;
		mov edx, eax;
		mov eax, edi;
		call GetArg;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		mov ecx, GetStringVar;
		call ecx;
		push eax;
		call HasFakePerk;
end:
		mov edx, eax;
		mov eax, edi;
		call SetResult;
		mov eax, edi;
		mov edx, 0xc001;
		call SetResultType;
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
		call GetArgType;
		mov edx, eax;
		mov eax, edi;
		mov ebx, 0x467500;
		call GetArg;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		mov ecx, GetStringVar;
		call ecx;
		push eax;
		call HasFakeTrait;
end:
		mov edx, eax;
		mov eax, edi;
		call SetResult;
		mov eax, edi;
		mov edx, 0xc001;
		call SetResultType;
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
		mov ebx, 0x4674F0;
		call ebx;
		mov edx, eax;
		mov eax, ecx;
		mov ebx, 0x467500;
		call ebx;
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
static void __declspec(naked) remove_trait() {
	__asm {
		pushad;
		mov ebp, eax;
		call GetArgType;
		mov edi, eax;
		mov eax, ebp;
		call GetArg;
		cmp di, 0xc001;
		jnz end;
		xor ebx, ebx;
		dec ebx;
		mov ecx, ds:[0x66BE44];
		cmp eax, ds:[0x66BE40];
		jne next;
		mov ds:[0x66BE40], ecx;
		mov ds:[0x66BE44], ebx;
		jmp end;
next:
		cmp eax, ds:[0x66BE44];
		jne end;
		mov ds:[0x66BE44], ebx;
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
		call GetArgType;
		mov edx, eax;
		mov eax, ecx;
		call GetArg;
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
		call GetArgType;
		mov edx, eax;
		mov eax, ecx;
		call GetArg;
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