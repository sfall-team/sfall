/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"
#include "Objects.h"

#include "CritterStats.h"

namespace sfall
{

/*
 *	Brief description of how this code works in the module:
 *		The module is designed to create additional critter prototypes inherited from their base prototypes in memory (with the exception of player and party member prototypes).
 *		The inherited prototype is written with modified stat values that are changed by the engine or script functions.
 *		Stats changed by the set_critter_base_stat and set_critter_extra_stat functions are not saved, and reset when the player leaves the location map.
 *		Stats that can be changed by the engine (e.g. when using drugs or equipping armor) or set by the set_critter_stat function are saved to a file
 *		and always affect the critter, except for stats from using drugs, the effects of which are reset when the player leaves the location map (engine behavior).
 */

static struct {
	long id;
	long* proto;
} lastGetProtoID;

struct ProtoMem {
	long* proto;
	long  pid;
	long  sharedCount; // if the value is 0, then the prototype can be deleted from memory

	//ProtoMem() {}

	ProtoMem(long pid) : proto(nullptr), pid(pid), sharedCount(0) {} // for load

	ProtoMem(long* defProto, long pid) : pid(pid), sharedCount(1) {
		CreateProtoMem(defProto);
	}

	void CreateProtoMem(long* srcProto) {
		this->proto = reinterpret_cast<long*>(new long[104]); // 416 bytes
		std::memcpy(this->proto, srcProto, 416);
	}

	~ProtoMem() { delete[] proto; }
};

std::unordered_map<long, ProtoMem> protoMem; // key - critter ID, value - pointer to critter prototype for unique ID

typedef std::unordered_map<long, ProtoMem>::iterator itProtoMem;
static void ModifyAllStats(const itProtoMem&);

#pragma pack(push, 1)
struct StatModify {
	long objID;
	long objPID;   // used for additional verification
	long stat;     // range from base_stat_srength to bonus_dr_poison
	long amount;   // current value of the stat
	long defVal;   // value of the stat before the change
	long* s_proto; // shared pointer for quick access to the prototype in memory (should not be saved to file)

	StatModify() : s_proto(nullptr) {} // for load

	StatModify(fo::GameObject* critter, long stat, long amount, long* defaultProto, long defVal) {
		objID = Objects::SetObjectUniqueID(critter);
		objPID = critter->protoId;
		this->stat = stat;
		this->amount = amount;
		this->defVal = defVal;
		fo::func::dev_printf("[SFALL] Add modify stat value: %d, def=%d, NPC ID: %d\n", amount, defVal, objID);

		itProtoMem mem = protoMem.find(objID);
		if (mem == protoMem.end()) {
			mem = protoMem.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(objID),
				std::forward_as_tuple(defaultProto, objPID)
			).first;
		} else {
			if (mem->second.proto == nullptr) { // prototype has not been created
				fo::func::dev_printf("[SFALL] No critter prototype, create new...\n");
				mem->second.CreateProtoMem(defaultProto);
				ModifyAllStats(mem);
			}
			mem->second.sharedCount++;
		}
		s_proto = mem->second.proto;
	}

	long* TryReleaseProtoMem() {
		itProtoMem mem = protoMem.find(objID);
		if (mem != protoMem.end() && mem->second.pid == objPID) {
			if (--mem->second.sharedCount <= 0) { // the counter will be reset
				protoMem.erase(mem); // removes the prototype from memory
				if (objID == lastGetProtoID.id) lastGetProtoID.id = 0;
				s_proto = nullptr;
				fo::func::dev_printf("[SFALL] Remove prototype for ID: %d\n", objID);
			}
			return s_proto;
		}
		return nullptr;
	}
};
#pragma pack(pop)

std::vector<StatModify> bonusStatProto;
std::vector<StatModify> baseStatProto;

// saveable stats
std::vector<StatModify> s_bonusStatProto;
std::vector<StatModify> s_baseStatProto;

static long isNotPartyMemberPid;

//////////////////////////////// CRITTERS STATS ////////////////////////////////

static long GetBaseStatValue(long* proto, long stat) {
	return proto[OffsetStat::base + stat];
}

static void SetBaseStatValue(long* proto, long stat, long amount) {
	proto[OffsetStat::base + stat] = amount;
}

static long GetBonusStatValue(long* proto, long stat) {
	return proto[OffsetStat::bonus + stat];
}

static void SetBonusStatValue(long* proto, long stat, long amount) {
	proto[OffsetStat::bonus + stat] = amount;
}

static long GetStatValue(long* proto, long offset) {
	return proto[offset];
}

static void SetStatValue(long* proto, long offset, long amount) {
	proto[offset] = amount;
}

// Applies all stats parameters, loaded from save file to individual critter prototype
static void ModifyAllStats(const itProtoMem &mem) {
	if (!mem->second.proto && !fo::util::CritterCopyProto(mem->second.pid, mem->second.proto)) return; // proto error

	for (auto itBonus = s_bonusStatProto.begin(); itBonus != s_bonusStatProto.end(); ++itBonus) {
		if (itBonus->objID == mem->first && itBonus->objPID == mem->second.pid) {
			itBonus->s_proto = mem->second.proto;
			SetBonusStatValue(itBonus->s_proto, itBonus->stat, itBonus->amount);
			mem->second.sharedCount++;
		}
	}
	for (auto itBase = s_baseStatProto.begin(); itBase != s_baseStatProto.end(); ++itBase) {
		if (itBase->objID == mem->first && itBase->objPID == mem->second.pid) {
			itBase->s_proto = mem->second.proto;
			SetBaseStatValue(itBase->s_proto, itBase->stat, itBase->amount);
			mem->second.sharedCount++;
		}
	}
}

static std::vector<StatModify>& GetRefVector(bool bonus, bool isSaved) {
	if (isSaved) {
		return (bonus) ? s_bonusStatProto : s_baseStatProto;
	}
	return (bonus) ? bonusStatProto : baseStatProto;
}

// Adds or modifies an existing stat value in the inherited critter prototype
// if the set amount value is equal to defVal, this stat is deleted and there is an attempt to remove the inherited prototype if it does not have any more changed stats
static void AddStat(long stat, fo::GameObject* critter, long amount, long* defaultProto, long offset, bool isSaved) {
	std::vector<StatModify> &vec = GetRefVector(offset == OffsetStat::bonus, isSaved);

	fo::func::dev_printf("[SFALL] Set bonus:%d stat:%d, to NPC pid: %d, saved:%d\n", (offset == OffsetStat::bonus), stat, (critter->protoId & 0xFFFF), isSaved);

	offset += stat;
	for (auto itStat = vec.begin(); itStat != vec.end(); ++itStat) {
		if (itStat->objID == critter->id && itStat->objPID == critter->protoId && itStat->stat == stat) {
			fo::func::dev_printf("[SFALL] Modify stat value old: %d to new: %d, ID: %d\n", itStat->s_proto[offset], amount, itStat->objID);
			if (amount == itStat->defVal) { // set value and value in regular prototype are matched
				if (itStat->TryReleaseProtoMem()) SetStatValue(itStat->s_proto, offset, amount);
				// remove vector element, without re-moving other elements
				if (itStat != vec.cend() - 1) *itStat = vec.back();
				vec.pop_back();
				return;
			}
			itStat->amount = amount;
			if (itStat->s_proto == nullptr) itStat->s_proto = CritterStats::GetProto(critter);
			SetStatValue(itStat->s_proto, offset, amount);
			return;
		}
	}
	if (vec.capacity() == vec.size()) vec.reserve(vec.size() + 20);
	vec.emplace_back(critter, stat, amount, defaultProto, defaultProto[offset]);
	SetStatValue(vec.back().s_proto, offset, amount); // set value to individuals prototype
}

static void __fastcall SetStatToProto(long stat, fo::GameObject* critter, long amount, long* protoBase, long offset, bool isSaved) {
	if (critter->protoId != fo::PID_Player) {
		if (critter->protoId == isNotPartyMemberPid || !fo::util::IsPartyMember(critter)) {
			isNotPartyMemberPid = critter->protoId;
			AddStat(stat, critter, amount, protoBase, offset, isSaved);
			return;
		}
	}
	SetStatValue(protoBase, offset + stat, amount); // set value to dude or party member prototype
}

static void UpdateDefValue(std::vector<StatModify> &vec, long stat, long pid, long amount) {
	for (auto itStat = vec.begin(); itStat != vec.end(); ++itStat) {
		if (itStat->objPID == pid && itStat->stat == stat) {
			fo::func::dev_printf("[SFALL] Update stat default value: %d to: %d, NPC ID: %d\n", itStat->defVal, amount, itStat->objID);
			itStat->defVal = amount;
		}
	}
}

static void SetStatToAllProtos(long offset, long pid, long amount) {
	offset /= 4;
	long stat;
	if (offset < OffsetStat::bonus) {
		stat = offset - OffsetStat::base;
		UpdateDefValue(baseStatProto, stat, pid, amount);
		UpdateDefValue(s_baseStatProto, stat, pid, amount);
	} else {
		stat = offset - OffsetStat::bonus;
		UpdateDefValue(bonusStatProto, stat, pid, amount);
		UpdateDefValue(s_bonusStatProto, stat, pid, amount);
	}
	for (const auto& mem : protoMem) {
		if (mem.second.pid == pid && mem.second.proto) {
			SetStatValue(mem.second.proto, offset, amount);
		}
	};
}

// Changes the bonus value of a stat (for example, with the adjust_ac_ or perform_drug_effect_ functions when using drugs)
// in the saved individual critter prototype
static __declspec(naked) void stat_set_bonus_hack() {
	__asm {
		pushadc;
		mov  eax, [esp + 16];   // buf proto
		sub  eax, 0x20;
		push 1;                 // saved
		push 44;                // offset from bonus_stat_srength
		push eax;               // regular proto
		push ebx;               // amount
		mov  edx, esi;          // critter
		call SetStatToProto;    // ecx - stat
		popadc;
		retn;
	}
}

// Changes the base stat value with the set_critter_stat script function in the saved individual critter prototype
static __declspec(naked) void stat_set_base_hack() {
	__asm {
		pushadc;
		mov  eax, [esp + 16];   // buf proto
		sub  eax, 0x20;
		push 1;                 // saved
		push 9;                 // offset from base_stat_srength
		push eax;               // regular proto
		push ebx;               // amount
		mov  edx, esi;          // critter
		call SetStatToProto;    // ecx - stat
		popadc;
		test ecx, ecx;
		retn;
	}
}

// Gets critter stat value from the base or individual critter prototype when using any engine method (for example, the get_critter_stat script function)
static __declspec(naked) void stat_get_proto() {
	using namespace fo::Fields;
	__asm {
		push eax;
		push ecx;
		mov  ecx, eax;          // critter;
		call CritterStats::GetProto;
		pop  ecx;
		test eax, eax;
		jz   skip;
		add  esp, 4;
		mov  [esp + 4], eax;    // replace proto
		add  dword ptr [esp], 5; // skip call proto_ptr_
		retn;
skip:
		pop  eax;
		lea  edx, [esp + 4];    // buf
		mov  eax, [eax + protoId];
		retn;
	}
}

static __declspec(naked) void stat_recalc_derived_hook() {
	__asm {
		pushadc;
		mov  ecx, ebx;
		call CritterStats::GetProto;
		pop  ecx;
		pop  edx;
		test eax, eax;
		jz   skip;
		add  esp, 4;
		mov  [esp + 4], eax;    // replace proto
		retn;
skip:
		pop  eax;
		jmp  fo::funcoffs::proto_ptr_;
	}
}

////////////////////////////////////////////////////////////////////////////////

// Returns the individual critter prototype, or null if it is missing
long* __fastcall CritterStats::GetProto(fo::GameObject* critter) {
	if (critter->protoId == fo::PID_Player || protoMem.empty()) return nullptr;
	if (lastGetProtoID.id == critter->id) return lastGetProtoID.proto;

	auto itMem = protoMem.find(critter->id);
	if (itMem != protoMem.end() && itMem->second.pid == critter->protoId) {
		if (!itMem->second.proto) { // there is no prototype in memory, create a new one from the regular prototype
			fo::func::dev_printf("[SFALL] Modify all stats to NPC ID: %d\n", critter->id);
			ModifyAllStats(itMem); // also create prototype
			if (!itMem->second.proto) fo::func::debug_printf("[SFALL] Error get critter prototype pid: %d\n", itMem->second.pid);
		}
		if (itMem->second.proto) {
			lastGetProtoID.id = itMem->first;
			lastGetProtoID.proto = itMem->second.proto;
		}
		return itMem->second.proto;
	}
	return nullptr; // no prototype in memory with this ID, regular prototype will be used
}

// set_proto_data: sets value parameter to base(regular) object prototype, and to all individual(inherited) critter prototypes with the same PID
long CritterStats::SetProtoData(long pid, long offset, long amount) {
	long* proto;
	long result = fo::func::proto_ptr(pid, (fo::Proto**)&proto);
	if (result != -1) {
		if (!protoMem.empty() && pid != fo::PID_Player && (pid >> 24) == fo::OBJ_TYPE_CRITTER && (offset >= 0x24 && offset <= 0x138)) {
			SetStatToAllProtos(offset, pid, amount);
		}
		*(long*)((BYTE*)proto + offset) = amount; // set to regular prototype
	}
	return result;
}

// get_critter_base_stat/get_critter_extra_stat: gets stat value from an individual's prototype (inherited from the base prototype)
// or from the base prototype if there were no changes to the stat for this critter
long CritterStats::GetStat(fo::GameObject* critter, long stat, long offset) {
	long* proto = CritterStats::GetProto(critter);
	if (proto == nullptr) fo::func::proto_ptr(critter->protoId, (fo::Proto**)&proto); // regular prototype
	return (!proto) ? 0 : GetStatValue(proto, stat + offset); // direct get
}

// set_critter_base_stat/set_critter_extra_stat: changes stat value in the individual prototype of the critter inherited from the base prototype
// the changed value will not be saved when saving the game and will be reset when the player leaves the location map
void CritterStats::SetStat(fo::GameObject* critter, long stat, long amount, long offset) {
	long* proto;
	if (fo::util::GetProto(critter->protoId, (fo::Proto**)&proto)) {
		SetStatToProto(stat, critter, amount, proto, offset, false); // non-saveable stat
		if (stat <= fo::STAT_lu) fo::func::stat_recalc_derived(critter); // Prerequisite: stat >= 0
	}
}

static void ClearAllStats() {
	bonusStatProto.clear();
	baseStatProto.clear();
	lastGetProtoID.id = 0;
}

static void FlushAllProtos() {
	for (auto itBonus = s_bonusStatProto.begin(); itBonus != s_bonusStatProto.end(); ++itBonus) {
		itBonus->s_proto = nullptr;
	}
	for (auto itBase = s_baseStatProto.begin(); itBase != s_baseStatProto.end(); ++itBase) {
		itBase->s_proto = nullptr;
	}
	for (auto& mem : protoMem) {
		delete[] mem.second.proto;
		mem.second.proto = nullptr;
		mem.second.sharedCount = 0;
	};
	ClearAllStats();
}

// Removes all the inherited prototypes on leaving the map
static __declspec(naked) void map_save_in_game_hook() {
	__asm {
		call FlushAllProtos;
		jmp  fo::funcoffs::proto_remove_all_;
	}
}

static void StatsProtoReset() {
	ClearAllStats();
	s_bonusStatProto.clear();
	s_baseStatProto.clear();
	protoMem.clear();
	isNotPartyMemberPid = 0;
}

void CritterStats::RecalcDerivedHook() {
	HookCall(0x4AF761, stat_recalc_derived_hook);
}

void CritterStats::SaveStatData(HANDLE file) {
	DWORD sizeWrite, count = s_baseStatProto.size();
	WriteFile(file, &count, 4, &sizeWrite, 0);
	for (size_t i = 0; i < count; i++) {
		WriteFile(file, &s_baseStatProto[i], 20, &sizeWrite, 0);
	}
	fo::func::debug_printf("LOADSAVE: Total critter base stats saved: %d\n", count);

	count = s_bonusStatProto.size();
	WriteFile(file, &count, 4, &sizeWrite, 0);
	for (size_t i = 0; i < count; i++) {
		WriteFile(file, &s_bonusStatProto[i], 20, &sizeWrite, 0);
	}
	fo::func::debug_printf("LOADSAVE: Total critter bonus stats saved: %d\n", count);
}

bool CritterStats::LoadStatData(HANDLE file) {
	DWORD count, sizeRead;
	ReadFile(file, &count, 4, &sizeRead, 0);
	if (sizeRead != 4) return false; // wrong/old file data version

	if (count) s_baseStatProto.reserve(count + 10);
	for (size_t i = 0; i < count; i++) {
		StatModify data;
		ReadFile(file, &data, 20, &sizeRead, 0);
		if (sizeRead != 20) return true;

		s_baseStatProto.emplace_back(data);
		//if (protoMem.find(data.objID) == protoMem.end()) {
			protoMem.emplace(data.objID, data.objPID);
		//}
		fo::func::debug_printf("LOADSAVE: Critter PID/ID: %d/%d, saved base stat: %d, value: %d, def: %d\n", data.objPID, data.objID, data.stat, data.amount, data.defVal);
	}

	ReadFile(file, &count, 4, &sizeRead, 0);
	if (sizeRead != 4) return true;

	if (count) s_bonusStatProto.reserve(count + 10);
	for (size_t i = 0; i < count; i++) {
		StatModify data;
		ReadFile(file, &data, 20, &sizeRead, 0);
		if (sizeRead != 20) return true;

		s_bonusStatProto.emplace_back(data);
		//if (protoMem.find(data.objID) == protoMem.end()) {
			protoMem.emplace(data.objID, data.objPID);
		//}
		fo::func::debug_printf("LOADSAVE: Critter PID/ID: %d/%d, saved bonus stat: %d, value: %d, def: %d\n", data.objPID, data.objID, data.stat, data.amount, data.defVal);
	}
	return false;
}

void CritterStats::init() {

	MakeCall(0x4AF6AD, stat_set_bonus_hack, 1);
	MakeCall(0x4AF5B8, stat_set_base_hack);
	MakeCalls(stat_get_proto, {
		0x4AF49B, // stat_get_bonus_
		0x4AF455  // stat_get_base_direct_
	});
	HookCall(0x483E0B, map_save_in_game_hook); // remove all protos

	LoadGameHook::OnGameReset() += StatsProtoReset;
}

}
