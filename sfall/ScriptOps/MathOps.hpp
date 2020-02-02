/*
 *    sfall
 *    Copyright (C) 2008-2020  The sfall team
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

#include <cmath>

#include "main.h"

#include "ScriptExtender.h"

static void _stdcall funcDiv2() {
	const ScriptValue &dividend = opHandler.arg(0),
					  &divisor = opHandler.arg(1);

	if (!dividend.isString() && !divisor.isString()) {
		if (divisor.rawValue() == 0) {
			opHandler.printOpcodeError("div - division by zero.");
			return;
		}

		if (dividend.isFloat() || divisor.isFloat()) {
			opHandler.setReturn(dividend.asFloat() / divisor.asFloat());
		} else {
			opHandler.setReturn(dividend.rawValue() / divisor.rawValue()); // unsigned division
		}
	} else {
		OpcodeInvalidArgs("div");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) funcDiv() {
	_WRAP_OPCODE(funcDiv2, 2, 1)
}

static void __declspec(naked) funcSqrt() {
	__asm {
		pushad;
		sub esp, 4;
		mov ecx, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_INT;
		jnz arg1l2;
		mov [esp], eax;
		fild [esp];
		jmp calc;
arg1l2:
		cmp bx, VAR_TYPE_FLOAT;
		jnz fail;
		mov [esp], eax;
		fld [esp];
calc:
		fsqrt;
		fstp [esp];
		mov edx, [esp];
		jmp end;
fail:
		fldz;
		fstp [esp];
		mov edx, [esp];
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ecx;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

static void _stdcall funcAbs2() {
	const ScriptValue &value = opHandler.arg(0);

	if (!value.isString()) {
		if (value.isInt()) {
			opHandler.setReturn(abs(static_cast<int>(value.rawValue())));
		} else {
			opHandler.setReturn(abs(value.asFloat()));
		}
	} else {
		OpcodeInvalidArgs("abs");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) funcAbs() {
	_WRAP_OPCODE(funcAbs2, 1, 1)
}

static void __declspec(naked) funcSin() {
	__asm {
		pushad;
		sub esp, 4;
		mov ecx, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_INT;
		jnz arg1l2;
		mov [esp], eax;
		fild [esp];
		jmp calc;
arg1l2:
		cmp bx, VAR_TYPE_FLOAT;
		jnz fail;
		mov [esp], eax;
		fld [esp];
calc:
		fsin;
		fstp [esp];
		mov edx, [esp];
		jmp end;
fail:
		fldz;
		fstp [esp];
		mov edx, [esp];
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ecx;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

static void __declspec(naked) funcCos() {
	__asm {
		pushad;
		sub esp, 4;
		mov ecx, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_INT;
		jnz arg1l2;
		mov [esp], eax;
		fild [esp];
		jmp calc;
arg1l2:
		cmp bx, VAR_TYPE_FLOAT;
		jnz fail;
		mov [esp], eax;
		fld [esp];
calc:
		fcos;
		fstp [esp];
		mov edx, [esp];
		jmp end;
fail:
		fldz;
		fstp [esp];
		mov edx, [esp];
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ecx;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

static void __declspec(naked) funcTan() {
	__asm {
		pushad;
		sub esp, 4;
		mov ecx, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_INT;
		jnz arg1l2;
		mov [esp], eax;
		fild [esp];
		jmp calc;
arg1l2:
		cmp bx, VAR_TYPE_FLOAT;
		jnz fail;
		mov [esp], eax;
		fld [esp];
calc:
		fptan;
		fstp [esp];
		fstp [esp];
		mov edx, [esp];
		jmp end;
fail:
		fldz;
		fstp [esp];
		mov edx, [esp];
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ecx;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

static void __declspec(naked) funcATan() {
	__asm {
		pushad;
		sub esp, 4;
		mov ecx, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_INT;
		jnz arg1l2;
		mov [esp], eax;
		fild [esp];
		jmp arg2l1;
arg1l2:
		cmp bx, VAR_TYPE_FLOAT;
		jnz fail;
		mov [esp], eax;
		fld [esp];
arg2l1:
		cmp dx, VAR_TYPE_INT;
		jnz arg2l2;
		mov [esp], edi;
		fild [esp];
		jmp calc;
arg2l2:
		cmp dx, VAR_TYPE_FLOAT;
		jnz fail2;
		mov [esp], edi;
		fld [esp];
calc:
		fpatan;
		fstp [esp];
		mov edx, [esp];
		jmp end;
fail2:
		fstp [esp];
fail:
		fldz;
		fstp [esp];
		mov edx, [esp];
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ecx;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

static void _stdcall funcPow2() {
	const ScriptValue &base = opHandler.arg(0),
					  &power = opHandler.arg(1);

	if (!base.isString() && !power.isString()) {
		float result = 0.0;
		if (power.isFloat())
			result = pow(base.asFloat(), power.floatValue());
		else
			result = pow(base.asFloat(), static_cast<int>(power.rawValue()));

		if (base.isInt() && power.isInt()) {
			opHandler.setReturn(static_cast<int>(result));
		} else {
			opHandler.setReturn(result);
		}
	} else {
		OpcodeInvalidArgs("power");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) funcPow() {
	_WRAP_OPCODE(funcPow2, 2, 1)
}

static void _stdcall funcLog2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		opHandler.setReturn(log(floatArg.asFloat()));
	} else {
		OpcodeInvalidArgs("log");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) funcLog() {
	_WRAP_OPCODE(funcLog2, 1, 1)
}

static void _stdcall funcExp2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		opHandler.setReturn(exp(floatArg.asFloat()));
	} else {
		OpcodeInvalidArgs("exponent");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) funcExp() {
	_WRAP_OPCODE(funcExp2, 1, 1)
}

static void _stdcall funcCeil2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		opHandler.setReturn(static_cast<int>(ceil(floatArg.asFloat())));
	} else {
		OpcodeInvalidArgs("ceil");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) funcCeil() {
	_WRAP_OPCODE(funcCeil2, 1, 1)
}

static void _stdcall funcRound2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		float arg = floatArg.asFloat();

		int argI = static_cast<int>(arg);
		float mod = arg - static_cast<float>(argI);
		if (abs(mod) >= 0.5) argI += (mod > 0 ? 1 : -1);
		opHandler.setReturn(argI);
	} else {
		OpcodeInvalidArgs("round");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) funcRound() {
	_WRAP_OPCODE(funcRound2, 1, 1)
}

static void sf_floor2() {
	const ScriptValue &valArg = opHandler.arg(0);

	if (!valArg.isString()) {
		opHandler.setReturn(static_cast<int>(floor(valArg.asFloat())));
	} else {
		OpcodeInvalidArgs("floor2");
		opHandler.setReturn(0);
	}
}
