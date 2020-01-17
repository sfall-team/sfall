#include "main.h"

#include "CheckAddress.h"

namespace sfall
{

#ifndef NDEBUG

std::multimap<long, long> writeAddress;

static std::vector<long> excludeAddr = {
	0x42A14F, 0x42A178,
	0x44E949, 0x44E94A, 0x44E937, 0x4F5F40, 0x4CB850, // from movies.cpp
};

struct HackPair {
	long addr;
	long len;

	HackPair(long addr, long len) : addr(addr), len(len) {}
};

static std::vector<HackPair> hackAddr = {
	// scripting: Anims
	{0x459C97, 1}, {0x459D4B, 1}, {0x459E3B, 1}, {0x459EEB, 1},	{0x459F9F, 1}, {0x45A053, 1}, {0x45A10B, 1}, {0x45AE53, 1},
	// scripting: Misc
	{0x495C50, 4}, {0x495C77, 6}, {0x495CEC, 6}, {0x495CFB, 1}, {0x495BF1, 5}, {0x4AFBC1, 1}, {0x4AFC1C, 2},
	// scripting: Perks
	{0x424AB6, 1}, {0x4AFAE2, 4},
	{0x49687F, 1}, {0x496880, 4}, // hookcall
	// scripting: Stats
	{0x43C27A, 1}, {0x4AFABD, 5},
	// scripting: Worldmap
	{0x4C06D1, 5},

	// module: AI
	{0x4290B6, 5},
	// module: BarBoxes
	{0x461342, 5}, {0x461243, 5},
	{0x461493, 5}, //{0x461495, 1},
	{0x461513, 1}, {0x461718, 1}, {0x46170A, 1},
	// module: Books
	{0x49B9FB, 5},
	// module: Combat
	{0x478EC6, 1}, {0x478EC7, 4}, // hookcall
	// module: CritterStats
	{0x4AF761, 1}, {0x4AF762, 4}, // hookcall
	// module: DamageMod
	{0x4722DD, 1}, {0x4722DE, 4}, {0x472309, 1}, {0x47230A, 4}, // hookcalls
	// module: Debug
	{0x48007E, 1},
	// module: Drugs
	{0x479EEC, 5}, {0x43C15C, 5}, {0x47A5B8, 5}, {0x47A50C, 5},
	{0x47A523, 4}, {0x47A527, 1}, {0x47A3A8, 1},
	// module: Explosions
	{0x49BCC7, 5}, {0x49BD56, 5}, {0x4A2865, 5}, {0x4737F2, 5}, {0x49C005, 5},
	{0x411B54, 1}, {0x423C93, 1},
	{0x411A19, 4}, {0x411A29, 4}, {0x411A35, 4}, {0x411A3C, 4},
	{0x479183, 4}, {0x47918B, 4},
	{0x411709, 1}, {0x4119FC, 1}, {0x411C08, 1}, {0x4517C1, 1}, {0x423BC8, 1}, {0x42381A, 1},
	// module: ExtraSave
	//{0x47B923, 6},
	// module: Inventory
	// module: Interface
	{0x460BB6, 4},
	// module: LoadOrder
	{0x43FA9F, 4}, {0x44EB5B, 4}, {0x48152E, 4},
	//{0x480A95, 5}, (it in movies.cpp)
	// module: MainMenu
	{0x481753, 4}, {0x48175C, 4}, {0x481748, 4},
	// module: Message
	{0x484B18, 1}, {0x484B19, 4}, // hookcall
	// module: Movies
	{0x44E937, 1}, {0x44E938, 4}, {0x44E949, 1}, {0x44E94A, 4}, // hookcalls
	{0x4F5F40, 1}, {0x4CB850, 1},
	// module: Objects
	{0x4A364A, 5}, {0x4831D9, 1}, {0x4831DA, 1},
	{0x4841D6, 1}, {0x4841D7, 4}, // hookcall
	// module: PartyControl
	{0x422BDE, 1}, {0x4229EC, 1},
	{0x49EB09, 1}, {0x49EB0A, 4}, {0x458242, 1}, {0x458243, 4}, {0x458326, 1}, {0x458327, 4}, // hookcalls
	// module: Perks
	{0x43C77C, 1}, {0x43C77D, 4}, // hookcall
	{0x478AC4, 1}, {0x478AC5, 4}, {0x496823, 5},
	{0x496C2D, 1}, {0x496C2E, 4}, {0x496D11, 1}, {0x496D12, 4}, // hookcalls
	// trait hookcalls
	{0x4245E0, 1}, {0x4245E1, 4}, {0x4248F9, 1}, {0x4248FA, 4}, {0x478C8A, 1}, {0x478C8B, 4},
	{0x478E70, 1}, {0x478E71, 4}, {0x410707, 1}, {0x410708, 4}, {0x42389F, 1}, {0x4238A0, 4},
	{0x47A0CD, 1}, {0x47A0CE, 4}, {0x47A51A, 1}, {0x47A51B, 4}, {0x479BE1, 1}, {0x479BE2, 4},
	{0x47A0DD, 1}, {0x47A0DE, 4}, {0x43C295, 1}, {0x43C296, 4}, {0x43C2F3, 1}, {0x43C2F4, 4},
	{0x43C2A4, 1}, {0x43C2A5, 4}, {0x44272E, 1}, {0x44272F, 4},
	// end
	{0x4B3C7C, 5}, {0x4B40FC, 5},
	{0x4B3A81, 4}, {0x4B3B80, 4}, {0x4B3AAE, 4}, {0x4B3BA0, 4}, {0x4B3BC0, 4},
	{0x478E75, 5},
	{0x42448E, 4}, {0x424489, 4}, {0x424474, 4},
	{0x42465D, 1}, {0x424636, 1}, {0x4251CE, 1},
	// module: Player model
	{0x418B88, 4}, {0x418BAB, 4}, {0x418B50, 4}, {0x418B6D, 4},
	// module: Skills
	{0x4AA59D, 5}, {0x4AA738, 5}, {0x4AA940, 5},
	{0x4AA9E1, 1}, {0x4AA9E2, 4}, {0x4AA9F1, 1}, {0x4AA9F2, 4}, {0x4AA9EC, 1}, {0x4AA9ED, 4}, // hookcalls
	// module: Stats
	{0x4AF6FC, 5},
	// module: Worldmap
	{0x4A33B8, 6}, {0x4A34EC, 1}, {0x4A3544, 1},
	{0x42E57A, 1}, {0x42E57B, 4}, // hookcall
	{0x4C2D9B, 4}, {0x499FDE, 4},
	{0x49952C, 1}, {0x497557, 1}, {0x42E587, 1}, {0x42E588, 4}, {0x499FD4, 2}, {0x499E93, 2},
	// module: MiscPatches
	{0x41276A, 1}, {0x480AAA, 4}, {0x444BA5, 4}, {0x444BCA, 4},
};

//static std::vector<long> jumpAddr = {
//};

// Checking for conflicts requires all options in ddraw.ini to be enabled
void PrintAddrList() {
	long level = iniGetInt("Debugging", "Enable", 0, ::sfall::ddrawIni);
	if (level < 10) hackAddr.clear();

	std::vector<HackPair> sortAddr(hackAddr);

	for (const auto &wa : writeAddress) {
		sortAddr.emplace_back(wa.first, wa.second);
	}
	std::sort(sortAddr.begin(), sortAddr.end(), [](const HackPair &a, const HackPair &b) {
		return (a.addr != b.addr) ? a.addr < b.addr : a.len < b.len;
	});

	unsigned long pa = 0, pl = 0;
	for (const auto &el : sortAddr)
	{
		unsigned long diff = (pa) ? (el.addr - pa) : -1; // length between two addresses
		if (diff == 0) {
			dlog_f("0x%x L:%d [Overwriting]\n", DL_MAIN, el.addr, el.len);
		} else if (diff < pl) {
			dlog_f("0x%x L:%d [Conflict] with 0x%x L:%d\n", DL_MAIN, el.addr, el.len, pa, pl);
			MessageBoxA(0, "Conflict detected!", "", MB_TASKMODAL);
		} else if (level >= 11 && diff == pl) {
			dlog_f("0x%x L:%d [Warning] Hacking near:0x%x\n", DL_MAIN, el.addr, el.len, pa);
		} else if (level >= 12) {
			dlog_f("0x%x L:%d\n", DL_MAIN, el.addr, el.len);
		}
		pa = el.addr;
		pl = el.len;
	}
}

void CheckConflict(DWORD addr, long len) {
	if (writeAddress.find(addr) != writeAddress.cend()) {
		if (std::find(excludeAddr.cbegin(), excludeAddr.cend(), addr) != excludeAddr.cend()) return;
		char buf[64];
		sprintf_s(buf, "Memory overwriting at address 0x%x", addr);
		MessageBoxA(0, buf, "", MB_TASKMODAL);
	}
	writeAddress.emplace(addr, len);
}

void AddrAddToList(DWORD addr, long len) {
	writeAddress.emplace(addr, len);
}

#else

void PrintAddrList() {}
void CheckConflict(DWORD addr, long len) {}
void AddrAddToList(DWORD addr, long len) {}

#endif

}
