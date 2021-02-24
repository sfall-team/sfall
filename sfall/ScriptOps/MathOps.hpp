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

static void __stdcall op_div2() {
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

static void __declspec(naked) op_div() {
	_WRAP_OPCODE(op_div2, 2, 1)
}

static void __stdcall op_sqrt2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		opHandler.setReturn(sqrt(floatArg.asFloat()));
	} else {
		OpcodeInvalidArgs("sqrt");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_sqrt() {
	_WRAP_OPCODE(op_sqrt2, 1, 1)
}

static void __stdcall op_abs2() {
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

static void __declspec(naked) op_abs() {
	_WRAP_OPCODE(op_abs2, 1, 1)
}

static void __stdcall op_sin2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		opHandler.setReturn(sin(floatArg.asFloat()));
	} else {
		OpcodeInvalidArgs("sin");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_sin() {
	_WRAP_OPCODE(op_sin2, 1, 1)
}

static void __stdcall op_cos2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		opHandler.setReturn(cos(floatArg.asFloat()));
	} else {
		OpcodeInvalidArgs("cos");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_cos() {
	_WRAP_OPCODE(op_cos2, 1, 1)
}

static void __stdcall op_tan2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		opHandler.setReturn(tan(floatArg.asFloat()));
	} else {
		OpcodeInvalidArgs("tan");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_tan() {
	_WRAP_OPCODE(op_tan2, 1, 1)
}

static void __stdcall op_arctan2() {
	const ScriptValue &xFltArg = opHandler.arg(0),
					  &yFltArg = opHandler.arg(1);

	if (!xFltArg.isString() && !yFltArg.isString()) {
		opHandler.setReturn(atan2(xFltArg.asFloat(), yFltArg.asFloat()));
	} else {
		OpcodeInvalidArgs("arctan");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_arctan() {
	_WRAP_OPCODE(op_arctan2, 2, 1)
}

static void __stdcall op_power2() {
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

static void __declspec(naked) op_power() {
	_WRAP_OPCODE(op_power2, 2, 1)
}

static void __stdcall op_log2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		opHandler.setReturn(log(floatArg.asFloat()));
	} else {
		OpcodeInvalidArgs("log");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_log() {
	_WRAP_OPCODE(op_log2, 1, 1)
}

static void __stdcall op_exponent2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		opHandler.setReturn(exp(floatArg.asFloat()));
	} else {
		OpcodeInvalidArgs("exponent");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_exponent() {
	_WRAP_OPCODE(op_exponent2, 1, 1)
}

static void __stdcall op_ceil2() {
	const ScriptValue &floatArg = opHandler.arg(0);

	if (!floatArg.isString()) {
		opHandler.setReturn(static_cast<int>(ceil(floatArg.asFloat())));
	} else {
		OpcodeInvalidArgs("ceil");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_ceil() {
	_WRAP_OPCODE(op_ceil2, 1, 1)
}

static void __stdcall op_round2() {
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

static void __declspec(naked) op_round() {
	_WRAP_OPCODE(op_round2, 1, 1)
}

static void mf_floor2() {
	const ScriptValue &valArg = opHandler.arg(0);

	if (!valArg.isString()) {
		opHandler.setReturn(static_cast<int>(floor(valArg.asFloat())));
	} else {
		OpcodeInvalidArgs("floor2");
		opHandler.setReturn(0);
	}
}
