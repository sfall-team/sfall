/*
 *    sfall
 *    Copyright (C) 2008-2019  The sfall team
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

//static long* __fastcall GetProto(fo::GameObject*);

struct ProtoMem {
	long* proto;
	long  pid;
	long  sharedCount; // if the value is 0, then the prototype can be deleted from memory

	ProtoMem() {}
	ProtoMem(long pid) : proto(nullptr), pid(pid), sharedCount(0) {}
	ProtoMem(long* proto, long pid) : proto(proto), pid(pid), sharedCount(1) {}

	~ProtoMem() { delete[] proto; }
};
std::unordered_map<long, ProtoMem> protoMem; // key - critter ID, value - pointer to critter prototype for unique ID

typedef std::unordered_map<long, ProtoMem>::iterator protoMem_iterator;
static long CreateProtoMem(const protoMem_iterator&, long*, long*&);

struct ProtoModify {
	long objID;
	long objPID;
	long stat;
	long amount;
	long defval;   // value
	long* s_proto; // shared pointer for quick access to the prototype in memory (should not be saved to file)

	ProtoModify() : s_proto(nullptr) {}
	ProtoModify(fo::GameObject* critter, long stat, long amount, long* defaultProto, long defVal) {
		objID = Objects::SetObjectUniqueID(critter);
		objPID = critter->protoId;
		this->stat = stat;
		this->amount = amount;
		this->defval = defVal;

		auto it = protoMem.find(objID);
		if (it == protoMem.end()) {
			long* newProto = new long[104]; // 416 bytes
			memcpy(newProto, defaultProto, 416);
			s_proto = newProto;
			protoMem.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(objID),
				std::forward_as_tuple(newProto, objPID));
		} else {
			s_proto = it->second.proto;
			if (s_proto == nullptr) { // prototype has not been created
				it->second.sharedCount = CreateProtoMem(it, defaultProto, s_proto);
				it->second.proto = s_proto;
			}
			it->second.sharedCount++;
		}
	}
};
std::vector<ProtoModify> bonusStatProto;
std::vector<ProtoModify> baseStatProto;
// savable
std::vector<ProtoModify> s_bonusStatProto;
std::vector<ProtoModify> s_baseStatProto;

static long isNotPartyMemberPid;

static struct {
	long id;
	long* proto;
} lastGetProtoID;

//////////////////////////////// CRITTERS STATS ////////////////////////////////
// offset 44 - from bonus_stat_srength, 9 - from base_stat_srength
static long GetStatValue(long* proto, long offset) {
	return proto[offset];
}

static void SetStatValue(long* proto, long offset, long amount) {
	proto[offset] = amount;
}

static long ApplyAllStatsToProto(const protoMem_iterator &iter, long* &proto_out) {
	long count = 0;
	for (auto itBonus = s_bonusStatProto.begin(); itBonus != s_bonusStatProto.end(); itBonus++) {
		if (itBonus->objID == iter->first && itBonus->objPID == iter->second.pid) {
			if (!proto_out && !fo::CritterCopyProto(iter->second.pid, proto_out)) return 0;
			itBonus->s_proto = proto_out;
			SetStatValue(proto_out, 44 + itBonus->stat, itBonus->amount);
			count++;
		}
	}
	for (auto itBase = s_baseStatProto.begin(); itBase != s_baseStatProto.end(); itBase++) {
		if (itBase->objID == iter->first && itBase->objPID == iter->second.pid) {
			if (!proto_out && !fo::CritterCopyProto(iter->second.pid, proto_out)) return 0;
			itBase->s_proto = proto_out;
			SetStatValue(proto_out, 9 + itBase->stat, itBase->amount);
			count++;
		}
	}
	return count;
}

// Returns the prototype NPC located in memory
long* __fastcall CritterStats::GetProto(fo::GameObject* critter) {
	if (protoMem.empty() || critter->protoId == fo::PID_Player) return nullptr;
	if (lastGetProtoID.id == critter->id) return lastGetProtoID.proto;

	long* proto = nullptr;
	auto it = protoMem.find(critter->id);
	if (it != protoMem.end() && it->second.pid == critter->protoId) {
		proto = it->second.proto;
		if (!proto) { // there is no prototype in memory, create a new one from the default prototype
			it->second.sharedCount = ApplyAllStatsToProto(it, proto);
			it->second.proto = proto;
		}
		if (proto) {
			lastGetProtoID.id = it->first;
			lastGetProtoID.proto = proto;
		}
	}
	return proto;
}

static long CreateProtoMem(const protoMem_iterator &it, long* defaultProto, long* &newProto) {
	newProto = new long[104]; // 416 bytes
	memcpy(newProto, defaultProto, 416);
	return ApplyAllStatsToProto(it, newProto);
}

static long* ReleaseProtoMem(const std::vector<ProtoModify>::iterator &itStat) {
	long* proto = nullptr;
	auto it = protoMem.find(itStat->objID);
	if (it != protoMem.end() && it->second.pid == itStat->objPID) {
		if (it->second.sharedCount <= 1) { // the counter will be reset
			if (it->first == lastGetProtoID.id) lastGetProtoID.id = 0;
			protoMem.erase(it); // removes the prototype from memory
		} else {
			it->second.sharedCount--;
			proto = it->second.proto;
		}
	}
	return proto;
}

static std::vector<ProtoModify>& GetRefVector(long extra, bool isSaved) {
	if (isSaved) {
		return (extra) ? s_bonusStatProto : s_baseStatProto;
	}
	return (extra) ? bonusStatProto : baseStatProto;;
}

static void AddStatToProto(long stat, fo::GameObject* critter, long amount, long* defaultProto, long offset, bool isSaved) {
	std::vector<ProtoModify> &vec = GetRefVector(offset == 44, isSaved);
	offset += stat;

	for (auto itStat = vec.begin(); itStat != vec.end(); itStat++) {
		if (itStat->objID == critter->id && itStat->objPID == critter->protoId && itStat->stat == stat) {
			if (amount == itStat->defval) { // set value and value in default prototype are matched
				long* proto = ReleaseProtoMem(itStat);
				if (proto) SetStatValue(proto, offset, amount);
				// removal of a vector element, without re-moving other elements
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
		if (isNotPartyMemberPid == critter->protoId || !IsPartyMember(critter)) {
			isNotPartyMemberPid = critter->protoId;
			AddStatToProto(stat, critter, amount, protoBase, offset, isSaved);
			return;
		}
	}
	SetStatValue(protoBase, offset + stat, amount); // set value to default prototype
}

static void __declspec(naked) stat_set_bonus_hack() {
	__asm {
		pushadc;
		mov  eax, [esp + 16];   // buf proto
		sub  eax, 0x20;
		push 1;
		push 44;                // offset from bonus_stat_srength
		push eax;               // regular proto
		push ebx;               // amount
		mov  edx, esi;          // critter
		call SetStatToProto;    // ecx - stat
		popadc;
		retn;
	}
}

static void __declspec(naked) stat_set_base_hack() {
	__asm {
		pushadc;
		mov  eax, [esp + 16];   // buf proto
		sub  eax, 0x20;
		push 1;
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

static void __declspec(naked) stat_get_proto() {
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
		add  [esp], 5;          // skip call proto_ptr_
		retn;
skip:
		pop  eax;
		lea  edx, [esp + 4];    // buf
		mov  eax, [eax + protoId];
		retn;
	}
}

static void __declspec(naked) stat_recalc_derived_hook() {
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

static void SetStatToAllProtos(long offset, long pid, long amount) {
	offset /= 4;
	for (const auto& elem : protoMem) {
		if (elem.second.pid == pid) {
			long* proto = elem.second.proto;
			if (proto) SetStatValue(proto, offset, amount);
		}
	};
}

// set_proto_data - sets stats to default prototype and all individual prototypes (the same PID)
long CritterStats::SetProtoData(long pid, long offset, long amount) {
	long* proto;
	long result = fo::func::proto_ptr(pid, (fo::Proto**)&proto);
	if (result != -1) {
		if (!protoMem.empty() && pid != fo::PID_Player && (pid >> 24) == fo::OBJ_TYPE_CRITTER && (offset >= 0x24 && offset <= 0x138)) {
			SetStatToAllProtos(offset, pid, amount);
		}
		*(long*)((BYTE*)proto + offset) = amount; // set to default prototype
	}
	return result;
}

// get_critter_*_stat - gets stats from an individual's prototype or from a default prototype
long CritterStats::GetStat(fo::GameObject* critter, long stat, long offset) {
	long* proto = CritterStats::GetProto(critter);
	if (proto == nullptr) fo::func::proto_ptr(critter->protoId, (fo::Proto**)&proto);
	return (proto) ? GetStatValue(proto, stat + offset) : 0; // direct get
}

// set_critter_*_stat - sets stats to an individual's critter prototype
void CritterStats::SetStat(fo::GameObject* critter, long stat, long amount, long offset) {
	long* proto;
	if (fo::func::proto_ptr(critter->protoId, (fo::Proto**)&proto) != -1) {
		SetStatToProto(stat, critter, amount, proto, offset, false); // not save stat
	}
}

void CritterStats::SaveStatData(HANDLE file) {
	DWORD sizeWrite, count = s_baseStatProto.size();
	WriteFile(file, &count, 4, &sizeWrite, 0);
	for (size_t i = 0; i < count; i++) {
		WriteFile(file, &s_baseStatProto[i], 20, &sizeWrite, 0);
	}
	count = s_bonusStatProto.size();
	WriteFile(file, &count, 4, &sizeWrite, 0);
	for (size_t i = 0; i < count; i++) {
		WriteFile(file, &s_bonusStatProto[i], 20, &sizeWrite, 0);
	}
}

bool CritterStats::LoadStatData(HANDLE file) {
	DWORD count, sizeRead;
	ReadFile(file, &count, 4, &sizeRead, 0);
	//if (sizeRead != 4) return true;
	if (count) s_baseStatProto.reserve(count + 10);
	for (size_t i = 0; i < count; i++) {
		ProtoModify data;
		ReadFile(file, &data, 20, &sizeRead, 0);
		if (sizeRead != 20) return true;
		s_baseStatProto.emplace_back(data);
		if (protoMem.find(data.objID) == protoMem.end()) {
			protoMem.emplace(data.objID, data.objPID);
		}
	}
	ReadFile(file, &count, 4, &sizeRead, 0);
	//if (sizeRead != 4) return true;
	if (count) s_bonusStatProto.reserve(count + 10);
	for (size_t i = 0; i < count; i++) {
		ProtoModify data;
		ReadFile(file, &data, 20, &sizeRead, 0);
		if (sizeRead != 20) return true;
		s_bonusStatProto.emplace_back(data);
		if (protoMem.find(data.objID) == protoMem.end()) {
			protoMem.emplace(data.objID, data.objPID);
		}
	}
	return false;
}

static void ClearAllStats() {
	bonusStatProto.clear();
	baseStatProto.clear();
	lastGetProtoID.id = 0;
}

static void FlushAllProtos() {
	for (auto itBonus = s_bonusStatProto.begin(); itBonus != s_bonusStatProto.end(); itBonus++) {
		itBonus->s_proto = nullptr;
	}
	for (auto itBase = s_baseStatProto.begin(); itBase != s_baseStatProto.end(); itBase++) {
		itBase->s_proto = nullptr;
	}
	for (auto& elem : protoMem) {
		delete[] elem.second.proto;
		elem.second.proto = nullptr;
		elem.second.sharedCount = 0;
	};
	ClearAllStats();
}

static void __declspec(naked) map_save_in_game_hook() {
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
