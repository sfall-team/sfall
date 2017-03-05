/*
 *    sfall
 *    Copyright (C) 2012  The sfall team
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

#include "Module.h"

namespace sfall 
{

class AI : public Module {
	const char* name() { return "AI"; }
	void init();
};

// TODO: use subscription instead
void _stdcall AICombatStart();
void _stdcall AICombatEnd();

DWORD _stdcall AIGetLastAttacker(DWORD target);
DWORD _stdcall AIGetLastTarget(DWORD source);

void _stdcall AIBlockCombat(DWORD i);

}
