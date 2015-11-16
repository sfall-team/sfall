/*
*    sfall
*    Copyright (C) 2008, 2009, 2010, 2013, 2014  The sfall team
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

#include "main.h"

#include "Define.h"
#include "FalloutEngine.h"
#include "Logging.h"

static const DWORD DamageFunctionReturn = 0x424A63;

// Damage Fix v5 by Glovz 2014.04.16.xx.xx
static __declspec(naked) void DamageFunction1() {
__asm {
				mov ebx,dword ptr ss:[esp+0x1c]; // get the number of hits
				xor ecx,ecx; // set the loop counter to 0
				cmp ebx,0x0; // compare the number of hits to 0
				jle end; // exit
begin:
				mov dword ptr ss:[esp+0x30],0x0; // clear value
				mov edx,dword ptr ds:[esi+4]; // get pointer to weapon being used by an attacker (I think)
				mov eax,dword ptr ds:[esi]; // get pointer to critter attacking
				call item_w_damage_; // get the raw damage value
				mov ebx,dword ptr ss:[esp+0x18]; // get the bonus ranged damage value
				cmp ebx,0x0; // compare the range bonus damage value to 0
				jle rdJmp; // if the RB value is less than or equal to 0 then goto rdJmp
				add eax,ebx; // add the RB value to the RD value
	rdJmp:
				cmp eax,0x0; // compare the new damage value to 0
				jle noDamageJmp; // goto noDamageJmp
				mov ebx,eax; // set the ND value
				mov edx,dword ptr ss:[esp+0x28]; // get the armorDT value
				cmp edx,0x0; // compare the armorDT value to 0
				jle bJmp; // if the armorDT value is less than or equal to 0 then goto bJmp
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to critters ammo being used in their weapon (I think)
				call item_w_dam_div_; // get the ammoY value
				cmp eax,0x0; // compare the ammoY value to 0
				jg aJmp; // if the ammoY value is greater than 0 then goto aJmp
				mov eax,0x1; // set the ammoY value to 1
aJmp:
				xor ebp,ebp; // clear value
				cmp edx,eax; // compare the dividend with the divisor
				jl lrThan; // the dividend is less than the divisor then goto lrThan
				jg grThan; // the dividend is greater than the divisor then goto grThan
				jmp setOne; // if the two values are equal then goto setOne
	lrThan:
				mov ebp,edx; // store the dividend value temporarily
				imul edx,0x2; // multiply dividend value by 2
				sub edx,eax; // subtract divisor value from the dividend value
				cmp edx,0x0; // compare the result to 0
				jl setZero; // if the result is less than 0 then goto setZero
				jg setOne; // if the result is greater than 0 then goto setOne
				mov edx,ebp; // restore dividend value
				and edx,0x1; // if true (1) then odd if false (0) then even
				jz setZero; // if the result is equal to 0 then setZero
				jmp setOne; // if the result is not 0 then goto setOne
	grThan:
				mov ebp,eax; // assign the divisor value
				xor eax,eax; // clear value
		bbbJmp:
				inc eax; // add 1 to the quotient value
				sub edx,ebp; // subtract the divisor value from the dividend value
				cmp edx,ebp; // compare the remainder value to the divisor value
				jge bbbJmp; // if the remainder value is greater or equal to the divisor value then goto bbbJmp
				jz endDiv; // if the remainder value is equal to 0 then goto endDiv
				imul edx,0x2; // multiply temp remainder value by 2
				sub edx,ebp; // subtract the divisor value from the temp remainder
				cmp edx,0x0; // compare the result to 0
				jl endDiv; // if the result is less than 0 then goto endDiv
				jg addOne; // if the result is greater than 0 then goto addOne
				mov ebp,eax; // assign the quotient value
				and ebp,0x1; // if true (1) then odd if false (0) then even
				jz endDiv; // if the result is equal to zero goto endDiv
	addOne:
				inc eax; // add 1 to the quotient value
				jmp endDiv; // goto endDiv
	setOne:
				mov eax,0x1; // set the quotient value to 1
				jmp endDiv; // goto endDiv
	setZero:
				xor eax,eax; // clear value
	endDiv:
				cmp dword ptr ss:[esp+0x30],0x2; // compare value to 2
				je divTwo; // goto divTwo
				cmp dword ptr ss:[esp+0x30],0x3; // compare value to 3
				je divThree; // goto divThree
				cmp dword ptr ss:[esp+0x30],0x4; // compare value to 4
				je divFour; // goto divFour
				cmp dword ptr ss:[esp+0x30],0x5; // compare value to 5
				je divFive; // goto divFive
				cmp dword ptr ss:[esp+0x30],0x6; // compare value to 6
				je divSix; // goto divSix
				sub ebx,eax; // subtract the new armorDT value from the RD value
				jmp cJmp; // goto cJmp
bJmp:
				mov edx,dword ptr ss:[esp+0x2c]; // get the armorDR value
				cmp edx,0x0; // compare the armorDR value to 0
				jle dJmp; // if the armorDR value is less than or equal to 0 then goto dJmp
cJmp:
				cmp ebx,0x0; // compare the new damage value to 0
				jle noDamageJmp; // goto noDamageJmp
				mov edx,dword ptr ss:[esp+0x2c]; // get the armorDR value
				cmp edx,0x0; // compare the armorDR value to 0
				jle eJmp; // if the armorDR value is less than or equal to 0 then goto eJmp
				mov eax,dword ptr ss:[esp+0x20]; // get the CD value
				cmp eax,0x64; // compare the CD value to 100
				jg sdrJmp; // if the CD value is greater than 100 then goto sdrJmp
				je aSubCJmp; // if the CD value is equal to 100 then goto aSubCJmp
				add edx,0x14; // add 20 to the armorDR value
				jmp aSubCJmp; // goto aSubCJmp
		sdrJmp:
				sub edx,0x14; // subtract 20 from the armorDR value
	aSubCJmp:
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to ammo being used in the critters weapon (I think)
				call item_w_dr_adjust_; // get the ammoDRM value
				cmp eax,0x0; // compare the ammoDRM value to 0
				jl adrJmp; // if the ammoDRM value is less than 0 then goto adrJmp
				je bSubCJmp; // if the ammoDRM value is equal to 0 then goto bSubCJmp
				xor ebp,ebp; // clear value
				sub ebp,eax; // subtract ammoDRM value from 0
				mov eax,ebp; // set new ammoDRM value
		adrJmp:
				add edx,eax; // add the ammoDRM value to the armorDR value
	bSubCJmp:
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to ammo being used in the critters weapon (I think)
				call item_w_dam_mult_; // get the ammoX value
				cmp eax,0x0; // compare the ammoX value to 0
				jg cSubCJmp; // if the ammoX value is greater than 0 then goto cSubCJmp;
				mov eax,0x1; // set the ammoX value to 1
	cSubCJmp:
				mov dword ptr ss:[esp+0x30],0x2; // set value to 2
				jmp aJmp; // goto aJmp
		divTwo:
				mov edx,ebx; // set temp value
				imul edx,eax; // multiply the ND value by the armorDR value
				mov eax,0x64; // set divisor value to 100
				mov dword ptr ss:[esp+0x30],0x3; // set value to 3
				jmp aJmp; // goto aJmp
		divThree:
				sub ebx,eax; // subtract the damage resisted value from the ND value
				jmp eJmp; // goto eJmp
dJmp:
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to ammo being used in the critters weapon (I think)
				call item_w_dam_mult_; // get the ammoX value
				cmp eax,0x1; // compare the ammoX value to 1
				jle bSubDJmp; // if the ammoX value is less than or equal to 1 then goto bSubDJmp;
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to critters ammo being used in their weapon (I think)
				call item_w_dam_div_; // get the ammoY value
				cmp eax,0x1; // compare the ammoY value to 1
				jle aSubDJmp; // if the ammoY value is less than or equal to 1 then goto aSubDJmp
				mov edx,ebx; // set temp value
				imul edx,0xf; // multiply the ND value by 15
				mov eax,0x64; // set divisor value to 100
				mov dword ptr ss:[esp+0x30],0x4; // set value to 4
				jmp aJmp; // goto aJmp
		divFour:
				add ebx,eax; // add the quotient value to the ND value
				jmp eJmp; // goto eJmp
	aSubDJmp:
				mov edx,ebx; // set temp value
				imul edx,0x14; // multiply the ND value by 20
				mov eax,0x64; // set divisor value to 100
				mov dword ptr ss:[esp+0x30],0x5; // set value to 5
				jmp aJmp; // goto aJmp
		divFive:
				add ebx,eax; // add the quotient value to the ND value
				jmp eJmp; // goto eJmp
	bSubDJmp:
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to critters ammo being used in their weapon (I think)
				call item_w_dam_div_; // get the ammoY value
				cmp eax,0x1; // compare the ammoY value to 1
				jle eJmp; // goto eJmp
				mov edx,ebx; // set temp value
				imul edx,0xa; // multiply the ND value by 10
				mov eax,0x64; // set divisor value to 100
				mov dword ptr ss:[esp+0x30],0x6; // set value to 6
				jmp aJmp; // goto aJmp
		divSix:
				add ebx,eax; // add the quotient value to the ND value
eJmp:
				cmp ebx,0x0; // compare the new damage value to 0
				jle noDamageJmp; // goto noDamageJmp
				mov eax,dword ptr ss:[esp+0x24]; // get the CM value
				cmp eax,0x2; // compare the CM value to 2
				jle addNDJmp; // if the CM value is less than or equal to 2 then goto addNDJmp
				imul ebx,eax; // multiply the ND value by the CM value
				sar ebx,0x1; // divide the result by 2
addNDJmp:
				add dword ptr ds:[edi],ebx; // accumulate damage
noDamageJmp:
				mov eax,dword ptr ss:[esp+0x1c]; // get the number of hits
				inc ecx; // the counter value increments by one
				cmp ecx,eax; // is the counter value less than the number of hits
				jl begin; // goto begin
end:
				jmp DamageFunctionReturn; // exit
	}
}

// Damage Fix v5.1 by Glovz 2014.04.16.xx.xx
static __declspec(naked) void DamageFunction2() {
__asm {
				mov ebx,dword ptr ss:[esp+0x1c]; // get the number of hits
				xor ecx,ecx; // set the loop counter to 0
				cmp ebx,0x0; // compare the number of hits to 0
				jle end; // exit
begin:
				mov dword ptr ss:[esp+0x30],0x0; // clear value
				mov edx,dword ptr ds:[esi+4]; // get pointer to weapon being used by an attacker (I think)
				mov eax,dword ptr ds:[esi]; // get pointer to critter attacking
				call item_w_damage_; // get the raw damage value
				mov ebx,dword ptr ss:[esp+0x18]; // get the bonus ranged damage value
				cmp ebx,0x0; // compare the range bonus damage value to 0
				jle rdJmp; // if the RB value is less than or equal to 0 then goto rdJmp
				add eax,ebx; // add the RB value to the RD value
	rdJmp:
				cmp eax,0x0; // compare the new damage value to 0
				jle noDamageJmp; // goto noDamageJmp
				mov ebx,eax; // set the ND value
				mov edx,dword ptr ss:[esp+0x28]; // get the armorDT value
				cmp edx,0x0; // compare the armorDT value to 0
				jle bJmp; // if the armorDT value is less than or equal to 0 then goto bJmp
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to critters ammo being used in their weapon (I think)
				call item_w_dam_div_; // get the ammoY value
				cmp eax,0x0; // compare the ammoY value to 0
				jg aJmp; // if the ammoY value is greater than 0 then goto aJmp
				mov eax,0x1; // set the ammoY value to 1
aJmp:
				xor ebp,ebp; // clear value
				cmp edx,eax; // compare the dividend with the divisor
				jl lrThan; // the dividend is less than the divisor then goto lrThan
				jg grThan; // the dividend is greater than the divisor then goto grThan
				jmp setOne; // if the two values are equal then goto setOne
	lrThan:
				mov ebp,edx; // store the dividend value temporarily
				imul edx,0x2; // multiply dividend value by 2
				sub edx,eax; // subtract divisor value from the dividend value
				cmp edx,0x0; // compare the result to 0
				jl setZero; // if the result is less than 0 then goto setZero
				jg setOne; // if the result is greater than 0 then goto setOne
				mov edx,ebp; // restore dividend value
				and edx,0x1; // if true (1) then odd if false (0) then even
				jz setZero; // if the result is equal to 0 then setZero
				jmp setOne; // if the result is not 0 then goto setOne
	grThan:
				mov ebp,eax; // assign the divisor value
				xor eax,eax; // clear value
		bbbJmp:
				inc eax; // add 1 to the quotient value
				sub edx,ebp; // subtract the divisor value from the dividend value
				cmp edx,ebp; // compare the remainder value to the divisor value
				jge bbbJmp; // if the remainder value is greater or equal to the divisor value then goto bbbJmp
				jz endDiv; // if the remainder value is equal to 0 then goto endDiv
				imul edx,0x2; // multiply temp remainder value by 2
				sub edx,ebp; // subtract the divisor value from the temp remainder
				cmp edx,0x0; // compare the result to 0
				jl endDiv; // if the result is less than 0 then goto endDiv
				jg addOne; // if the result is greater than 0 then goto addOne
				mov ebp,eax; // assign the quotient value
				and ebp,0x1; // if true (1) then odd if false (0) then even
				jz endDiv; // if the result is equal to zero goto endDiv
	addOne:
				inc eax; // add 1 to the quotient value
				jmp endDiv; // goto endDiv
	setOne:
				mov eax,0x1; // set the quotient value to 1
				jmp endDiv; // goto endDiv
	setZero:
				xor eax,eax; // clear value
	endDiv:
				cmp dword ptr ss:[esp+0x30],0x2; // compare value to 2
				je divTwo; // goto divTwo
				cmp dword ptr ss:[esp+0x30],0x3; // compare value to 3
				je divThree; // goto divThree
				cmp dword ptr ss:[esp+0x30],0x4; // compare value to 4
				je divFour; // goto divFour
				cmp dword ptr ss:[esp+0x30],0x5; // compare value to 5
				je divFive; // goto divFive
				cmp dword ptr ss:[esp+0x30],0x6; // compare value to 6
				je divSix; // goto divSix
				cmp dword ptr ss:[esp+0x30],0x7; // compare value to 7
				je divSeven; // goto divSeven
				sub ebx,eax; // subtract the new armorDT value from the RD value
				jmp cJmp; // goto cJmp
bJmp:
				mov edx,dword ptr ss:[esp+0x2c]; // get the armorDR value
				cmp edx,0x0; // compare the armorDR value to 0
				jle dJmp; // if the armorDR value is less than or equal to 0 then goto dJmp
cJmp:
				cmp ebx,0x0; // compare the new damage value to 0
				jle noDamageJmp; // goto noDamageJmp
				mov edx,dword ptr ss:[esp+0x2c]; // get the armorDR value
				cmp edx,0x0; // compare the armorDR value to 0
				jle eJmp; // if the armorDR value is less than or equal to 0 then goto eJmp
				mov eax,dword ptr ss:[esp+0x20]; // get the CD value
				cmp eax,0x64; // compare the CD value to 100
				jg sdrJmp; // if the CD value is greater than 100 then goto sdrJmp
				je aSubCJmp; // if the CD value is equal to 100 then goto aSubCJmp
				add edx,0x14; // add 20 to the armorDR value
				jmp aSubCJmp; // goto aSubCJmp
		sdrJmp:
				sub edx,0x14; // subtract 20 from the armorDR value
	aSubCJmp:
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to ammo being used in the critters weapon (I think)
				call item_w_dr_adjust_; // get the ammoDRM value
				cmp eax,0x0; // compare the ammoDRM value to 0
				jl adrJmp; // if the ammoDRM value is less than 0 then goto adrJmp
				je bSubCJmp; // if the ammoDRM value is equal to 0 then goto bSubCJmp
				xor ebp,ebp; // clear value
				sub ebp,eax; // subtract ammoDRM value from 0
				mov eax,ebp; // set new ammoDRM value
		adrJmp:
				add edx,eax; // add the ammoDRM value to the armorDR value
	bSubCJmp:
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to ammo being used in the critters weapon (I think)
				call item_w_dam_mult_; // get the ammoX value
				cmp eax,0x0; // compare the ammoX value to 0
				jg cSubCJmp; // if the ammoX value is greater than 0 then goto cSubCJmp;
				mov eax,0x1; // set the ammoX value to 1
	cSubCJmp:
				mov dword ptr ss:[esp+0x30],0x2; // set value to 2
				jmp aJmp; // goto aJmp
		divTwo:
				mov edx,ebx; // set temp value
				imul edx,eax; // multiply the ND value by the armorDR value
				mov eax,0x64; // set divisor value to 100
				mov dword ptr ss:[esp+0x30],0x3; // set value to 3
				jmp aJmp; // goto aJmp
		divThree:
				sub ebx,eax; // subtract the damage resisted value from the ND value
				jmp eJmp; // goto eJmp
dJmp:
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to ammo being used in the critters weapon (I think)
				call item_w_dam_mult_; // get the ammoX value
				cmp eax,0x1; // compare the ammoX value to 1
				jle bSubDJmp; // if the ammoX value is less than or equal to 1 then goto bSubDJmp;
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to critters ammo being used in their weapon (I think)
				call item_w_dam_div_; // get the ammoY value
				cmp eax,0x1; // compare the ammoY value to 1
				jle aSubDJmp; // if the ammoY value is less than or equal to 1 then goto aSubDJmp
				mov edx,ebx; // set temp value
				imul edx,0xf; // multiply the ND value by 15
				mov eax,0x64; // set divisor value to 100
				mov dword ptr ss:[esp+0x30],0x4; // set value to 4
				jmp aJmp; // goto aJmp
		divFour:
				add ebx,eax; // add the quotient value to the ND value
				jmp eJmp; // goto eJmp
	aSubDJmp:
				mov edx,ebx; // set temp value
				imul edx,0x14; // multiply the ND value by 20
				mov eax,0x64; // set divisor value to 100
				mov dword ptr ss:[esp+0x30],0x5; // set value to 5
				jmp aJmp; // goto aJmp
		divFive:
				add ebx,eax; // add the quotient value to the ND value
				jmp eJmp; // goto eJmp
	bSubDJmp:
				mov eax,dword ptr ds:[esi+0x8]; // get pointer to critters ammo being used in their weapon (I think)
				call item_w_dam_div_; // get the ammoY value
				cmp eax,0x1; // compare the ammoY value to 1
				jle eJmp; // goto eJmp
				mov edx,ebx; // set temp value
				imul edx,0xa; // multiply the ND value by 10
				mov eax,0x64; // set divisor value to 100
				mov dword ptr ss:[esp+0x30],0x6; // set value to 6
				jmp aJmp; // goto aJmp
		divSix:
				add ebx,eax; // add the quotient value to the ND value
eJmp:
				cmp ebx,0x0; // compare the new damage value to 0
				jle noDamageJmp; // goto noDamageJmp
				mov eax,dword ptr ss:[esp+0x24]; // get the CM value
				cmp eax,0x2; // compare the CM value to 2
				jle addNDJmp; // if the CM value is less than or equal to 2 then goto addNDJmp
				mov edx,ebx; // set temp ND value
				imul edx,eax; // multiply the temp ND value by the CM value
				imul edx,0x19; // multiply the temp ND value by 25
				mov eax,0x64; // set divisor value to 100
				mov dword ptr ss:[esp+0x30],0x7; // set value to 7
				jmp aJmp; // goto aJmp
		divSeven:
				add ebx,eax; // add the critical damage value to the ND value
addNDJmp:
				add dword ptr ds:[edi],ebx; // accumulate damage
noDamageJmp:
				mov eax,dword ptr ss:[esp+0x1c]; // get the number of hits
				inc ecx; // the counter value increments by one
				cmp ecx,eax; // is the counter value less than the number of hits
				jl begin; // goto begin
end:
				jmp DamageFunctionReturn; // exit
	}
}

// YAAM
static __declspec(naked) void DamageFunction5() {
	__asm {
		mov eax,dword ptr ds:[esi+0x8];		// Get pointer to critter's weapon
		mov edx,dword ptr ss:[esp+0x24];	// Get Critical Multiplier (passed in as argument to function)
		call item_w_dam_mult_;			// Retrieve Ammo Dividend
		imul edx,eax;				// Damage Multipler = Critical Multipler * Ammo Dividend
		mov eax,dword ptr ds:[esi+0x8];		// Get pointer to critter's weapon
		call item_w_dam_div_;			// Retrieve Ammo Divisor
		imul ebp,eax;				// Ammo Divisor = 1 * Ammo Divisor (ebp set to 1 earlier in function)
		mov ebx,dword ptr ss:[esp+0x1c];	// Get number of hits
		xor ecx,ecx;				// Set loop counter to zero
		mov dword ptr ss:[esp+0x24],edx;	// Store Damage Multiplier
		test ebx,ebx;				// Is number of hits <= 0?
		jle end;		// If yes, jump beyond damage calculation loop
ajmp:							// Start of damage calculation loop
		mov edx,dword ptr ds:[esi+0x4];		// Get pointer to weapon (?)
		mov eax,dword ptr ds:[esi];		// Get pointer to critter (?)
		mov ebx,dword ptr ss:[esp+0x18];	// Get Bonus Ranged Damage
		call item_w_damage_;		// Retrieve Raw Damage
		add ebx,eax; 				// Raw Damage = Raw Damage + Bonus Ranged Damage
		mov edx,dword ptr ss:[esp+0x28];	// Get armor DT
		mov eax,dword ptr ds:[esi+0x8];		// Get pointer to critter's weapon
		call item_w_dr_adjust_;			// Retrieve ammo DT (well, it's really Retrieve ammo DR, but since we're treating ammo DR as ammo DT...)
		sub edx,eax;				// DT = armor DT - ammo DT
		test edx,edx;				// Is DT >= 0?
		jge bjmp;				// If yes, skip the next instruction
		xor edx,edx;				// Otherwise, set DT = 0
bjmp:
		sub ebx,edx;				// Raw Damage = Raw Damage - DT
		test ebx,ebx;				// Is Raw Damage <= 0?
		jle cjmp;				// If yes, skip damage calculation and go to bottom of loop
		imul ebx,dword ptr ss:[esp+0x24];	// Otherwise, Raw Damage = Raw Damage * Damage Multiplier
		test ebp,ebp;				// Is Ammo Divisor == 0?
		je djmp;				// If yes, avoid divide by zero error
		mov edx,ebx;
		mov eax,ebx;
		sar edx,0x1f;
		idiv ebp;
		mov ebx,eax;				// Otherwise, Raw Damage = Raw Damage / Ammo Divisor
djmp:
		mov edx,ebx;
		mov eax,ebx;
		sar edx,0x1f;
		sub eax,edx;
		sar eax,0x1;				// Raw Damage = Raw Damage / 2 (related to critical hit damage multiplier bonus)
		mov edx,dword ptr ss:[esp+0x20];	// Get combat difficulty setting (75 if wimpy, 100 if normal or if attacker is player, 125 if rough)
		imul edx,eax;				// Raw Damage = Raw Damage * combat difficulty setting
		mov ebx,0x64;
		mov eax,edx;
		sar edx,0x1f;
		idiv ebx;
		mov ebx,eax;				// Raw Damage = Raw Damage / 100
		mov edx,dword ptr ss:[esp+0x28];	// Get armor DT
		mov eax,dword ptr ds:[esi+0x8];		// Get pointer to critter's weapon
		call item_w_dr_adjust_;			// Retrieve ammo DT
		sub edx,eax;				// DT = armor DT - ammo DT
		test edx,edx;				// Is DT >= 0?
		jge ejmp;				// If yes, set DT = 0
		mov eax,0x0a;
		imul eax,edx;				// Otherwise, DT = DT * 10 (note that this should be a negative value)
		jmp fjmp;
ejmp:
		xor eax,eax;
fjmp:
		mov edx,dword ptr ss:[esp+0x2c];	// Get armor DR
		add edx,eax;				// DR = armor DR + DT (note that DT should be less than or equal to zero)
		test edx,edx;				// Is DR >= 0?
		jge gjmp;
		xor edx,edx;				// If no, set DR = 0
		jmp hjmp;
gjmp:
		cmp edx,0x64;				// Otherwise, is DR >= 100?
		jge ijmp;				// If yes, damage will be zero, so stop calculating and go to bottom of loop
hjmp:
		imul edx,ebx;				// Otherwise, Resisted Damage = DR * Raw Damage
		mov dword ptr ss:[esp+0x30],0x64;
		mov eax,edx;
		sar edx,0x1f;
		idiv dword ptr ss:[esp+0x30];		// Resisted Damage = Resisted Damage / 100
		sub ebx,eax;				// Raw Damage = Raw Damage - Resisted Damage
cjmp:
		test ebx,ebx;				// Is Raw Damage <= 0?
		jle ijmp;				// If yes, don't accumulate damage
		add dword ptr ds:[edi],ebx;		// Otherwise, Accumulated Damage = Accumulated Damage + Raw Damage
ijmp:
		mov eax,dword ptr ss:[esp+0x1c];	// Get number of hits
		inc ecx;				// counter += 1
		cmp ecx,eax;				// Is counter < number of hits?
		jl ajmp;				// If yes, go back to start of damage calcuation loop (calculate damage for next hit)
end:
		jmp DamageFunctionReturn;		// Otherwise, exit loop
	}
}

static const DWORD MeleeDmgDispExit = 0x435C11;
static const DWORD MeleeDmgPrintExit = 0x439926;
static const DWORD MeleeExit = 0x47226D;
static const DWORD UnarmedExit = 0x472556;
static const DWORD FixaEnd = 0x4784B0;
static const DWORD FixbEnd = 0x478553;

static __declspec(naked) void MeleeDmgDispFix() {
	__asm {
		call stat_level_;			// Get Melee Damage
		mov ecx,eax;				// Store value
		mov eax,dword ptr ds:[_obj_dude];	// Get pointer to PC
		mov edx,PERK_bonus_hth_damage;		// perk_level_ argument: PERK_bonus_hth_damage
		call perk_level_;			// Get rank of Bonus HtH Damage
		shl eax,0x1;				// Multiply by 2
		sub ecx,eax;				// Subtract from Melee Damage
		mov eax,ecx;				// Move back to eax in preparation of push
		jmp MeleeDmgDispExit;
	}
}

static __declspec(naked) void MeleeDmgPrintFix() {
	__asm {
		call stat_level_;			// Get Melee Damage
		mov ebx,eax;				// Store value
		mov eax,dword ptr ds:[_obj_dude];	// Get pointer to PC
		mov edx,PERK_bonus_hth_damage;		// perk_level_ argument: PERK_bonus_hth_damage
		call perk_level_;			// Get rank of Bonus HtH Damage
		shl eax,0x1;				// Multiply by 2
		sub ebx,eax;				// Subtract from Melee Damage
		mov eax,ebx;				// Move back to eax in preparation of push
		mov ebx,0x27a;				// Facilitates printing to file
		jmp MeleeDmgPrintExit;
	}
}

static __declspec(naked) void MeleeWeapDmgRngDispFix() {
	__asm {
		call stat_level_;			// Get Melee Damage
		mov ebx,eax;				// Store Melee Damage value
		mov edx,dword ptr ds:[_stack];		// Get pointer to critter
		mov eax,dword ptr ds:[_obj_dude];	// Get pointer to PC
		cmp eax,edx;				// Is PC == critter?
		jnz end;					// exit if no
		mov edx,PERK_bonus_hth_damage;		// perk_level_ argument: PERK_bonus_hth_damage
		call perk_level_;			// Otherwise, get rank of Bonus HtH Damage
		shl eax,0x1;				// Multiply by 2
		sub ebx,eax;				// Subtract from Melee Damage
end:
		jmp MeleeExit;
	}
}

static __declspec(naked) void UnarmedDmgRngDispFix() {
	__asm {
		mov ebx,eax;				// Store Melee Damage value
		mov edx,dword ptr ds:[_stack];		// Get pointer to critter
		mov eax,dword ptr ds:[_obj_dude];	// Get pointer to PC
		cmp eax,edx;				// Is PC == critter?
		jnz end;					// exit if no
		mov edx,PERK_bonus_hth_damage;		// perk_level_ argument: PERK_bonus_hth_damage
		call perk_level_;			// Otherwise, get rank of Bonus HtH Damage
		shl eax,0x1;				// Multiply by 2
		sub ebx,eax;				// Subtract to Melee Damage
end:
		mov eax,ebx;				// Move back to eax in prepartion of push
		add eax,0x2;				// Add 2 (max unarmed damage value)
		push eax;					// push to print string
		mov eax,dword ptr ss:[esp+0x98];
		jmp UnarmedExit;
	}
}

static __declspec(naked) void HtHDamageFix1a() {
	__asm {
		mov eax,ecx;				// stat_level_ argument: pointer to critter
		mov edx,STAT_melee_dmg;		// stat_level_ argument: STAT_melee_dmg
		call stat_level_;			// Get Total_Melee_Damage
		add eax,2;					// Total_Melee_Damage += 2
		mov dword ptr ss:[esp],eax;			// Max_Damage = Total_Melee_Damage
		xor edx,edx;
		mov eax,dword ptr ds:[_obj_dude];	// Get pointer to PC
		cmp ecx,eax;				// Is the critter == PC?
		jnz ajmp;					// Skip to ajmp if no
		mov edx,PERK_bonus_hth_damage;		// perk_level_ argument: PERK_bonus_hth_damage
		call perk_level_;			// Return Rank_of_Bonus_HtH_Damage_perk
		shl eax,0x1;				// Rank_of_Bonus_HtH_Damage_perk *= 2
		mov edx, eax;				// Min_Damage = Rank_of_Bonus_HtH_Damage_perk
ajmp:
		add edx,1;					// Min_Damage += 1
		mov dword ptr ss:[esp+0x4],edx;		// Store Min_Damage
		sub esi,0x8;				// Set up for switch statement beyond jump
		jmp FixaEnd;
	}
}

static __declspec(naked) void HtHDamageFix1b() {
   __asm {
		call stat_level_;			// Get Total_Melee_Damage
		mov ebp, eax;
		mov eax, dword ptr ds:[_obj_dude];	// Get pointer to PC
		cmp ecx, eax;				// Is the critter == PC?
		jnz end;					// Skip to exit if no
		mov edx, PERK_bonus_hth_damage;		// perk_level_ argument: PERK_bonus_hth_damage
		call perk_level_;			// Return Rank_of_Bonus_HtH_Damage_perk
		shl eax, 0x1;				// Rank_of_Bonus_HtH_Damage_perk *= 2
		add dword ptr ss:[esp+0x4], eax;	// Min_Damage += Rank_of_Bonus_HtH_Damage_perk
end:
      jmp FixbEnd;
   }
}

static void __declspec(naked) DisplayBonusRangedDmg() {
	__asm {
		mov  edx, PERK_bonus_ranged_damage
		mov  eax, dword ptr ds:[_stack]
		call perk_level_
		shl  eax, 1
		add  dword ptr [esp+4*4], eax             // min_dmg
		add  dword ptr [esp+4*5], eax             // max_dmg
		jmp  sprintf_
	}
}

static void __declspec(naked) DisplayBonusHtHDmg1() {
	__asm {
		mov  edx, PERK_bonus_hth_damage
		mov  eax, dword ptr ds:[_stack]
		call perk_level_
		shl  eax, 1
		add  dword ptr [esp+4*4], eax             // min_dmg
		jmp  sprintf_
	}
}

static void __declspec(naked) DisplayBonusHtHDmg2() {
	__asm {
		push eax
		call stat_level_
		pop  ecx
		add  eax, 2
		push eax
		xchg ecx, eax
		mov  edx, PERK_bonus_hth_damage
		call perk_level_
		shl  eax, 1
		add  eax, 1
		push eax
		mov  eax, dword ptr [esp+0x98+0x4]
		push eax
		push 0x509EDC                             // '%s %d-%d'
		lea  eax, [esp+0xC+0x4]
		push eax
		call sprintf_
		add  esp, 4*5
		mov  eax, 0x472569
		jmp  eax
	}
}

static const DWORD UnarmedAttacksFixEnd=0x423A0D;
static void __declspec(naked) UnarmedAttacksFix() {
	__asm {
		cmp edx,0x10;				// Power Kick
		je PowKickHPunch;
		cmp edx,0x9;				// Hammer Punch
		jnz HKickJabCheck;
PowKickHPunch:
		mov edx,0x64;
		mov eax,0x1;
		call roll_random_;
		cmp eax,0x5;				// 5% chance of critical hit
		jle CriticalHit;
		jmp end;
HKickJabCheck:
		mov eax,dword ptr ds:[esi+0x4];		// get hit_mode
		cmp eax,0x12;				// Hook Kick
		je HKickJab;
		cmp eax,0xb;				// Jab
		jnz Haymaker;
HKickJab:
		mov edx,0x64;
		mov eax,0x1;
		call roll_random_;
		cmp eax,0xa;				// 10% chance of critical hit
		jle CriticalHit;
		jmp end;
Haymaker:
		cmp dword ptr ds:[esi+0x4],0xa;		// Haymaker
		jnz PalmStrike;
		mov edx,0x64;
		mov eax,0x1;
		call roll_random_;
		cmp eax,0xf;				// 15% chance of critical hit
		jle CriticalHit;
		jmp end;
PalmStrike:
		cmp dword ptr ds:[esi+0x4],0xc;		// Palm Strike
		jnz PiercingStrike;
		mov edx,0x64;
		mov eax,0x1;
		call roll_random_;
		cmp eax,0x14;				// 20% chance of critical hit
		jle CriticalHit;
		jmp end;
PiercingStrike:
		cmp dword ptr ds:[esi+0x4],0xd;		// Piercing Strike
		jnz PiercingKick;
		mov edx,0x64;
		mov eax,0x1;
		call roll_random_;
		cmp eax,0x28;				// 40% chance of critical hit
		jle CriticalHit;
		jmp end;
PiercingKick:
		cmp dword ptr ds:[esi+0x4],0x13;	// Piercing Kick
		jnz end;
		mov edx,0x64;
		mov eax,0x1;
		call roll_random_;
		cmp eax,0x32;				// 50% chance of critical hit
		jg end;
CriticalHit:
		mov ebx,0x3;				// Upgrade to critical hit
end:
		jmp UnarmedAttacksFixEnd;
	}
}

void AmmoModInit() {
	int formula;
	if (formula=GetPrivateProfileIntA("Misc", "DamageFormula", 0, ini)) {
		switch (formula) {
		case 1:
			MakeCall(0x424995, &DamageFunction1, true);
			break;
		case 2:
			MakeCall(0x424995, &DamageFunction2, true);
			break;
		/*case 3:
			MakeCall(0x424995, &DamageFunction3, true);
			break;
		case 4:
			MakeCall(0x424995, &DamageFunction4, true);
			break;*/
		case 5:
			MakeCall(0x424995, &DamageFunction5, true);
			break;
		}
	}
	int BonusHtHDmgFix = GetPrivateProfileIntA("Misc", "BonusHtHDamageFix", 1, ini);
	int DisplayBonusDmg = GetPrivateProfileIntA("Misc", "DisplayBonusDamage", 0, ini);
	if (BonusHtHDmgFix) {
		dlog("Applying Bonus HtH Damage Perk fix.", DL_INIT);
		MakeCall(0x435C0C, &MeleeDmgDispFix, true);
		MakeCall(0x43991C, &MeleeDmgPrintFix, true);
		if (!DisplayBonusDmg) {
			MakeCall(0x472266, &MeleeWeapDmgRngDispFix, true);
			MakeCall(0x47254B, &UnarmedDmgRngDispFix, true);
		}
		MakeCall(0x478492, &HtHDamageFix1a, true);
		MakeCall(0x47854C, &HtHDamageFix1b, true);
		dlogr(" Done", DL_INIT);
	}
	//if(GetPrivateProfileIntA("Misc", "SpecialUnarmedAttacksFix", 1, ini)) {
		dlog("Applying Special Unarmed Attacks fix.", DL_INIT);
		MakeCall(0x42394D, &UnarmedAttacksFix, true);
		dlogr(" Done", DL_INIT);
	//}
	if (DisplayBonusDmg) {
		dlog("Applying Display Bonus Damage patch.", DL_INIT);
		HookCall(0x4722DD, &DisplayBonusRangedDmg);
		if (BonusHtHDmgFix) {
			HookCall(0x472309, &DisplayBonusHtHDmg1);
			MakeCall(0x472546, &DisplayBonusHtHDmg2, true);
		}
		dlogr(" Done", DL_INIT);
	}
}
