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

//#include "..\InputFuncs.h"
#include "Graphics.h"
#include "ScriptShaders.h"

static void __declspec(naked) op_graphics_funcs_available() {
	__asm {
		cmp  GraphicsMode, 3;
		seta dl;
		and  edx, 0xFF;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_load_shader() {
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
		jnz error;
next:
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push eax;
		call LoadShader;
		mov edx, eax;
		jmp result;
error:
		mov edx, -1;
result:
		mov eax, edi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_free_shader() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call FreeShader;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_activate_shader() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call ActivateShader;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_deactivate_shader() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call DeactivateShader;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_get_shader_texture() {
	__asm {
		//Store registers
		push ebx;
		push ecx;
		push edx;
		push edi;
		//Get function args
		mov ecx, eax;
		call interpretPopShort_;
		push eax
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov ebx, [esp];
		cmp bx, VAR_TYPE_INT;
		jnz fail;
		mov ebx, [esp + 4];
		cmp bx, VAR_TYPE_INT;
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
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		//Restore registers and return
		add esp, 8;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_set_shader_int() {
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
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov [esp + 8], eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov ebp, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov [esp], eax;
		//Error check
		cmp di, VAR_TYPE_INT;
		jnz fail;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz fail;
next:
		cmp si, VAR_TYPE_INT;
		jnz fail;
		mov eax, ecx;
		mov ebx, ebp;
		call interpretGetString_;
		mov [esp + 4], eax;
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

static void __declspec(naked) op_set_shader_texture() {
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
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov [esp + 8], eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov ebp, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov [esp], eax;
		//Error check
		cmp di, VAR_TYPE_INT;
		jnz fail;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz fail;
next:
		cmp si, VAR_TYPE_INT;
		jnz fail;
		mov eax, ecx;
		mov ebx, ebp;
		call interpretGetString_;
		mov [esp + 4], eax;
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

static void __declspec(naked) op_set_shader_float() {
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
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov [esp + 8], eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov ebp, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov [esp], eax;
		//Error check
		cmp di, VAR_TYPE_FLOAT;
		jz paramWasFloat;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		fild [esp + 8];
		fstp [esp + 8];
paramWasFloat:
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz fail;
next:
		cmp si, VAR_TYPE_INT;
		jnz fail;
		mov eax, ecx;
		mov ebx, ebp;
		call interpretGetString_;
		mov [esp + 4], eax;
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

static void __declspec(naked) op_set_shader_vector() {
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
		call interpretPopShort_;
		mov word ptr [esp + ecx * 2 + 0x16], ax;
		mov eax, ebp;
		call interpretPopLong_;
		mov [esp + ecx * 4 - 0x4], eax;
		dec ecx;
		jnz argloopstart;
		//Error check
		mov ecx, 4;
checkloopstart:
		cmp word ptr [esp + ecx * 2 + 0x1a], VAR_TYPE_FLOAT;
		jz paramWasFloat;
		cmp word ptr [esp + ecx * 2 + 0x1a], VAR_TYPE_INT;
		jnz fail;
		fild [esp + ecx * 4 + 0x4];
		fstp [esp + ecx * 4 + 0x4];
paramWasFloat:
		dec ecx;
		jnz checkloopstart;
		cmp word ptr [esp + 0x1a], VAR_TYPE_STR2;
		jz next;
		cmp word ptr [esp + 0x1a], VAR_TYPE_STR;
		jnz fail;
next:
		cmp word ptr [esp + 0x18], VAR_TYPE_INT;
		jnz fail;
		mov eax, ebp;
		mov ebx, [esp + 4];
		xor edx, edx;
		mov dx, word ptr [esp + 0x1a];
		call interpretGetString_;
		mov [esp + 4], eax;
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

static void __declspec(naked) op_get_shader_version() {
	__asm {
		mov  esi, ecx;
		call Gfx_GetShaderVersion;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_set_shader_mode() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		cmp si, VAR_TYPE_INT;
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

static void __declspec(naked) op_force_graphics_refresh() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call Gfx_ForceGraphicsRefresh;
end:
		mov  ecx, esi;
		retn;
	}
}
