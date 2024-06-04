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

#include "Drugs.h"

namespace sfall
{

static const int drugsMax = 50;
static int drugsCount = 0;
static bool drugsReset = false; // true - need reset

long Drugs::addictionGvarCount = 0;
bool Drugs::JetWithdrawal = false;

sDrugs *drugs = nullptr;

static long __fastcall FindDrugVar(DWORD pid) {
	for (int i = 0; i < drugsCount; i++) {
		if (drugs[i].drugPid == pid) {
			//if (drugs[i].gvarID == 0) break;
			return drugs[i].gvarID;
		}
	}
	return -1;
}

static __declspec(naked) void pid_to_gvar_hack() {
	__asm {
		push ecx;
		push edx;
		mov  ecx, eax;
		call FindDrugVar;
		pop  edx;
		pop  ecx;
		test eax, eax;
		jg   end; // if gvar > 0
		mov  ebx, ds:[FO_VAR_drugInfoList];
		retn;
end:	// force exit func
		add  esp, 4;
		pop  edx;
		pop  ebx;
		retn;
	}
}

static long __fastcall AllowUseDrug(fo::GameObject* critter, DWORD pid) {
	for (int i = 0; i < drugsCount; i++) {
		if (drugs[i].drugPid == pid) {
			if (drugs[i].numEffects == -1) break; // use NumEffects value from engine
			if (drugs[i].numEffects == 0) return 1;
			auto queue = (fo::QueueDrugData*)fo::func::queue_find_first(critter, 0);
			if (!queue) return 1;
			int num = 0;
			while (queue->pid != pid || ++num < drugs[i].numEffects) {
				queue = (fo::QueueDrugData*)fo::func::queue_find_next(critter, 0);
				if (!queue) return 1;
			}
			return 0; // not allow
		}
	}
	return -1; // not found
}

static __declspec(naked) void drug_effect_allowed_hack() {
	__asm {
		push ecx;
		call AllowUseDrug; // ecx - critter, edx - drug pid
		pop  ecx;
		test eax, eax;
		jge  end; // if eax > -1
		mov  edi, ds:[FO_VAR_drugInfoList];
		retn;
end:	// force exit func
		add  esp, 4;
		pop  edi;
		pop  esi;
		pop  ecx;
		pop  ebx;
		retn;
	}
}

static long __stdcall FindDrugTime(DWORD pid) {
	for (int i = 0; i < drugsCount; i++) {
		if (drugs[i].drugPid == pid) {
			if (drugs[i].addictTimeOff <= 0) break;
			return drugs[i].addictTimeOff;
		}
	}
	return 10080; // default time (7 days)
}

static __declspec(naked) void perform_withdrawal_start_hack() {
	__asm {
		push ecx;
		push edi; // drug pid
		call FindDrugTime;
		pop  ecx;
		mov  ebx, eax; // time
		retn;
	}
}

static bool IsEngineAddictGvar(long gNum) {
	for (size_t i = 0; i < 9; i++) {
		if (fo::var::drugInfoList[i].addictGvar == gNum) return true;
	}
	return false;
}

static long __fastcall PrintAddictionList(long isSeparator) {
	std::vector<int> gVars; // gvar list of already displayed addictions
	long isSelect = 0;

	for (int i = 0; i < drugsCount; i++) {
		if (drugs[i].skip) continue;
		long gvarID = drugs[i].gvarID;
		if (gvarID > 0 && fo::var::game_global_vars[gvarID]) {

			if (IsEngineAddictGvar(gvarID)) {
				drugs[i].skip = 1; // skip this element next time
				continue;
			}
			if (std::find(gVars.cbegin(), gVars.cend(), gvarID) != gVars.cend()) continue;
			gVars.push_back(gvarID);

			if (!isSeparator) { // print separator line
				isSeparator = 1;
				const char* message = fo::util::GetMessageStr(&fo::var::editor_message_file, 4001);
				if (fo::func::folder_print_seperator(message)) {
					fo::var::folder_card_title = (DWORD)message;
					fo::var::folder_card_title2 = 0;
					fo::var::folder_card_desc = (DWORD)fo::util::GetMessageStr(&fo::var::editor_message_file, 4101);
					fo::var::folder_card_fid = 53;
					isSelect = 1;
				}
			}
			int msgNum = drugs[i].msgID;
			const char* message = fo::util::GetMessageStr(&fo::var::editor_message_file, msgNum);
			if (fo::func::folder_print_line(message)) {
				fo::var::folder_card_title = (DWORD)message;
				fo::var::folder_card_title2 = 0;
				if (msgNum > 0) msgNum += 100;
				fo::var::folder_card_desc = (DWORD)fo::util::GetMessageStr(&fo::var::editor_message_file, msgNum);
				fo::var::folder_card_fid = drugs[i].frmID;
				isSelect = 1;
			}
		}
	}
	return isSelect;
}

static __declspec(naked) void list_karma_hack() {
	static const DWORD list_karma_Ret = 0x43C1A3;
	__asm {
		mov  ecx, [esp + 0x168 - 0x1C + 4];
		call PrintAddictionList;
		or   eax, edi;
		jnz  end;
		mov  edi, 47;
		retn;
end:
		add  esp, 4;
		jmp  list_karma_Ret;
	}
}

static void CheckEngineNumEffects(int &set, long pid) {
	switch (pid) {
	case fo::PID_RADAWAY:
		set = 4;
		break;
	case fo::PID_MENTATS:
		set = 2;
		break;
	case fo::PID_BUFFOUT:
		set = 1;
		break;
	case fo::PID_NUKA_COLA:
		set = 0;
		break;
	case fo::PID_PSYCHO:
		set = 3;
		break;
	case fo::PID_BEER:
		set = 5;
		break;
	case fo::PID_BOOZE:
		set = 6;
		break;
	case fo::PID_JET:
		set = 7;
		break;
	case fo::PID_DECK_OF_TRAGIC_CARDS:
		set = 8;
	}
}

static void CheckValidGvarNumber() {
	for (int i = 0; i < drugsCount; i++) {
		if (drugs[i].gvarID > 0) {
			if (drugs[i].gvarID >= (long)fo::var::num_game_global_vars) {
				drugs[i].gvarID = -1;
			}
		}
	}
}

static void ResetDrugs() {
	if (!drugsReset) return;
	drugsReset = false;

	int set = -1;
	for (int i = 0; i < drugsCount; i++) {
		CheckEngineNumEffects(set, drugs[i].drugPid);
		if (set != -1) {
			fo::var::drugInfoList[set].numEffects = drugs[i].iniNumEffects;
			set = -1;
		}
		drugs[i].numEffects = drugs[i].iniNumEffects;
		drugs[i].addictTimeOff = drugs[i].iniAddictTimeOff;
	}
}

long Drugs::GetDrugCount() {
	return drugsCount;
}

long Drugs::GetDrugPid(long n) {
	return drugs[n].drugPid;
}

long Drugs::GetDrugGvar(long n) {
	return drugs[n].gvarID;
}

long Drugs::SetDrugNumEffect(long pid, long effect) {
	for (int i = 0; i < drugsCount; i++) {
		if (drugs[i].drugPid == pid) {
			if (effect < 0) effect = 0;
			int set = -1;
			CheckEngineNumEffects(set, pid);
			if (set != -1) fo::var::drugInfoList[set].numEffects = effect;
			drugs[i].numEffects = effect;
			drugsReset = true;
			return 0;
		}
	}
	return -1;
}

long Drugs::SetDrugAddictTimeOff(long pid, long time) {
	for (int i = 0; i < drugsCount; i++) {
		if (drugs[i].drugPid == pid) {
			if (time < 0) time = 0;
			drugs[i].addictTimeOff = time;
			drugsReset = true;
			return 0;
		}
	}
	return -1;
}

void Drugs::init() {
	auto drugsFile = IniReader::GetConfigString("Misc", "DrugsFile", "");
	if (!drugsFile.empty()) {
		const char* iniDrugs = drugsFile.insert(0, ".\\").c_str();
		if (GetFileAttributesA(iniDrugs) == INVALID_FILE_ATTRIBUTES) return;

		dlog("Applying drugs patch...", DL_INIT);
		JetWithdrawal = (IniReader::GetInt("main", "JetWithdrawal", 0, iniDrugs) == 1); // SafeWrite8(0x47A3A8, 0); item_wd_process_

		int count = IniReader::GetInt("main", "Count", 0, iniDrugs);
		if (count > 0) {
			if (count > drugsMax) count = drugsMax;
			drugs = new sDrugs[count]();

			int set = -1;
			char section[4];
			for (int i = 1; i <= count; i++) {
				_itoa(i, section, 10);
				int pid = IniReader::GetInt(section, "PID", 0, iniDrugs);
				if (pid > 0) {
					CheckEngineNumEffects(set, pid);
					drugs[drugsCount].drugPid = pid;
					drugs[drugsCount].addictTimeOff = drugs[drugsCount].iniAddictTimeOff = IniReader::GetInt(section, "AddictTime", 0, iniDrugs);
					long ef = IniReader::GetInt(section, "NumEffects", -1, iniDrugs);
					if (set != -1) {
						if (ef < -1) {
							ef = -1;
						} else if (ef > -1) {
							drugs[drugsCount].iniNumEffects = ef;
							fo::var::drugInfoList[set].numEffects = ef; // also set to engine
						} else {
							drugs[drugsCount].iniNumEffects = fo::var::drugInfoList[set].numEffects; // default value from engine
						}
						set = -1;
						drugs[drugsCount].numEffects = ef; // -1 to use the value from the engine
					} else {
						drugs[drugsCount].numEffects = drugs[drugsCount].iniNumEffects = max(0, ef);
						int gvar = IniReader::GetInt(section, "GvarID", 0, iniDrugs);
						drugs[drugsCount].gvarID = max(0, gvar); // not allow negative values
						if (gvar) {
							int msg = IniReader::GetInt(section, "TextID", -1, iniDrugs);
							drugs[drugsCount].msgID = (msg > 0) ? msg : -1;
							drugs[drugsCount].frmID = IniReader::GetInt(section, "FrmID", -1, iniDrugs);
							addictionGvarCount++;
						}
					}
					drugsCount++;
				}
			}
			if (drugsCount) {
				MakeCall(0x479EEC, drug_effect_allowed_hack, 1);
				MakeCall(0x43C15C, list_karma_hack, 2);
				MakeCall(0x47A5B8, pid_to_gvar_hack, 1);
				MakeCall(0x47A50C, perform_withdrawal_start_hack);
				SafeWrite32(0x47A523, 0x9090EBD1); // shr ebx, 1 (fix for trait drug addict perform_withdrawal_start_)
				SafeWrite8(0x47A527, CodeType::Nop); // perform_withdrawal_start_

				if (addictionGvarCount) {
					LoadGameHook::OnAfterGameInit() += CheckValidGvarNumber;
				}
				LoadGameHook::OnGameReset() += ResetDrugs;
			}
		}
		dlog_f(" (%d/%d drugs)\n", DL_INIT, drugsCount, count);
	}
}

void Drugs::exit() {
	if (drugs) delete[] drugs;
}

}
