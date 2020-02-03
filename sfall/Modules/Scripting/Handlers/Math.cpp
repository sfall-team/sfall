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

#include <cmath>

#include "..\OpcodeContext.h"

#include "Math.h"

namespace sfall
{
namespace script
{

void sf_div(OpcodeContext& ctx) {
	if (ctx.arg(1).rawValue() == 0) {
		ctx.printOpcodeError("%s - division by zero.", ctx.getOpcodeName());
		return;
	}
	if (ctx.arg(0).isFloat() || ctx.arg(1).isFloat()) {
		ctx.setReturn(ctx.arg(0).asFloat() / ctx.arg(1).asFloat());
	} else {
		ctx.setReturn(ctx.arg(0).rawValue() / ctx.arg(1).rawValue()); // unsigned division
	}
}

void sf_sqrt(OpcodeContext& ctx) {
	ctx.setReturn(sqrt(ctx.arg(0).asFloat()));
}

void sf_abs(OpcodeContext& ctx) {
	if (ctx.arg(0).isInt()) {
		ctx.setReturn(abs(static_cast<int>(ctx.arg(0).rawValue())));
	} else {
		ctx.setReturn(abs(ctx.arg(0).asFloat()));
	}
}

void sf_sin(OpcodeContext& ctx) {
	ctx.setReturn(sin(ctx.arg(0).asFloat()));
}

void sf_cos(OpcodeContext& ctx) {
	ctx.setReturn(cos(ctx.arg(0).asFloat()));
}

void sf_tan(OpcodeContext& ctx) {
	ctx.setReturn(tan(ctx.arg(0).asFloat()));
}

void sf_arctan(OpcodeContext& ctx) {
	ctx.setReturn(atan2(ctx.arg(0).asFloat(), ctx.arg(1).asFloat()));
}

void sf_power(OpcodeContext& ctx) {
	const ScriptValue &base = ctx.arg(0),
					  &power = ctx.arg(1);
	float result = 0.0;
	if (power.isFloat())
		result = pow(base.asFloat(), power.floatValue());
	else
		result = pow(base.asFloat(), static_cast<int>(power.rawValue()));

	if (base.isInt() && power.isInt()) {
		ctx.setReturn(static_cast<int>(result));
	} else {
		ctx.setReturn(result);
	}
}

void sf_log(OpcodeContext& ctx) {
	ctx.setReturn(log(ctx.arg(0).asFloat()));
}

void sf_exponent(OpcodeContext& ctx) {
	ctx.setReturn(exp(ctx.arg(0).asFloat()));
}

void sf_ceil(OpcodeContext& ctx) {
	ctx.setReturn(static_cast<int>(ceil(ctx.arg(0).asFloat())));
}

void sf_round(OpcodeContext& ctx) {
	float arg = ctx.arg(0).asFloat();

	int argI = static_cast<int>(arg);
	float mod = arg - static_cast<float>(argI);
	if (abs(mod) >= 0.5) argI += (mod > 0 ? 1 : -1);
	ctx.setReturn(argI);
}

void sf_floor2(OpcodeContext& ctx) {
	ctx.setReturn(static_cast<int>(floor(ctx.arg(0).asFloat())));
}

}
}
