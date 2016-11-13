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

#include "..\..\..\main.h"
#include "..\..\..\InputFuncs.h"
#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\Graphics.h"
#include "..\..\ScriptExtender.h"

#include "Graphics.h"

void __declspec(naked) op_graphics_funcs_available() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		xor edx, edx;
		mov ebx, GraphicsMode;
		cmp ebx, 3;
		jle end;
		inc edx;
end:
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_load_shader() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz error;
next:
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push eax;
		call LoadShader;
		mov edx, eax;
		jmp result;
error:
		mov edx, -1;
result:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xC001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_free_shader() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		call FreeShader;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_activate_shader() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		call ActivateShader;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_deactivate_shader() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		call DeactivateShader;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_get_shader_texture() {
	__asm {
		//Store registers
		push ebx;
		push ecx;
		push edx;
		push edi;
		//Get function args
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov ebx, [esp];
		cmp bx, 0xC001;
		jnz fail;
		mov ebx, [esp + 4];
		cmp bx, 0xc001;
		jnz fail;
		//set the new value
		push ecx;
		push edi;
		push eax;
		call GetShaderTexture;
		mov edx, eax;
		pop ecx;
		jmp end;
fail:
		mov edx, -1;
end:
		//Pass back the result
		mov eax, ecx;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
		//Restore registers and return
		add esp, 8;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_shader_int() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		push ebp;
		sub esp, 0xc;
		mov ecx, eax;
		//Get args
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov[esp + 8], eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov ebp, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov[esp], eax;
		//Error check
		cmp di, 0xC001;
		jnz fail;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz fail;
next:
		cmp si, 0xc001;
		jnz fail;
		mov eax, ecx;
		mov ebx, ebp;
		call FuncOffs::interpretGetString_;
		mov[esp + 4], eax;
		call SetShaderInt;
		jmp end;
fail:
		add esp, 0xc;
end:
		pop ebp;
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_shader_texture() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		push ebp;
		sub esp, 0xc;
		mov ecx, eax;
		//Get args
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov[esp + 8], eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov ebp, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov[esp], eax;
		//Error check
		cmp di, 0xC001;
		jnz fail;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz fail;
next:
		cmp si, 0xc001;
		jnz fail;
		mov eax, ecx;
		mov ebx, ebp;
		call FuncOffs::interpretGetString_;
		mov[esp + 4], eax;
		call SetShaderTexture;
		jmp end;
fail:
		add esp, 0xc;
end:
		pop ebp;
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_shader_float() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		push ebp;
		sub esp, 0xc;
		mov ecx, eax;
		//Get args
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov[esp + 8], eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov ebp, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov[esp], eax;
		//Error check
		cmp di, 0xa001;
		jz paramWasFloat;
		cmp di, 0xc001;
		jnz fail;
		fild[esp + 8];
		fstp[esp + 8];
paramWasFloat:
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz fail;
next:
		cmp si, 0xc001;
		jnz fail;
		mov eax, ecx;
		mov ebx, ebp;
		call FuncOffs::interpretGetString_;
		mov[esp + 4], eax;
		call SetShaderFloat;
		jmp end;
fail:
		add esp, 0xc;
end:
		pop ebp;
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_shader_vector() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push ebp;
		sub esp, 0x2a;
		mov ebp, eax;
		//Get args
		mov ecx, 6;
argloopstart:
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov word ptr[esp + ecx * 2 + 0x16], ax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov[esp + ecx * 4 - 0x4], eax;
		loop argloopstart;
		//Error check
		mov ecx, 4;
checkloopstart:
		cmp word ptr[esp + ecx * 2 + 0x1a], 0xa001;
		jz paramWasFloat;
		cmp word ptr[esp + ecx * 2 + 0x1a], 0xc001;
		jnz fail;
		fild[esp + ecx * 4 + 0x4];
		fstp[esp + ecx * 4 + 0x4];
paramWasFloat:
		loop checkloopstart;
		cmp word ptr[esp + 0x1a], 0x9001;
		jz next;
		cmp word ptr[esp + 0x1a], 0x9801;
		jnz fail;
next:
		cmp word ptr[esp + 0x18], 0xc001;
		jnz fail;
		mov eax, ebp;
		mov ebx, [esp + 4];
		xor edx, edx;
		mov dx, word ptr[esp + 0x1a];
		call FuncOffs::interpretGetString_;
		mov[esp + 4], eax;
		call SetShaderVector;
		add esp, 0x12;
		jmp end;
fail:
		add esp, 0x2a;
end:
		pop ebp;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_get_shader_version() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call GetShaderVersion;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_shader_mode() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		push eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz fail;
		cmp si, 0xC001;
		jnz fail;
		push eax;
		call SetShaderMode;
		jmp end;
fail:
		pop eax;
end:
		pop esi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_force_graphics_refresh() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		call ForceGraphicsRefresh;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
