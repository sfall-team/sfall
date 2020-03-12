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

#include <dshow.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "Graphics.h"
#include "LoadGameHook.h"

#include "Sound.h"

namespace sfall
{

#define SAFERELEASE(a) { if (a) { a->Release(); } }

struct sDSSound {
	DWORD id;     // should be first
	IGraphBuilder *pGraph;
	IMediaControl *pControl;
	IMediaSeeking *pSeek;
	IMediaEventEx *pEvent;
	IBasicAudio   *pAudio;
};

static std::vector<sDSSound*> playingSounds;
static std::vector<sDSSound*> loopingSounds;

DWORD playID = 0;
DWORD loopID = 0;

static HWND soundwindow = 0;
static sDSSound* musicLoopPtr = nullptr;
//static char playingMusicFile[256];

static void FreeSound(sDSSound* sound) {
	sound->pEvent->SetNotifyWindow(0, WM_APP, 0);
	SAFERELEASE(sound->pAudio);
	SAFERELEASE(sound->pEvent);
	SAFERELEASE(sound->pSeek);
	SAFERELEASE(sound->pControl);
	SAFERELEASE(sound->pGraph);
	delete sound;
}

static void WipeSounds() {
	for (size_t i = 0; i < playingSounds.size(); i++) FreeSound(playingSounds[i]);
	for (size_t i = 0; i < loopingSounds.size(); i++) FreeSound(loopingSounds[i]);
	playingSounds.clear();
	loopingSounds.clear();
	musicLoopPtr = nullptr;
	playID = 0;
	loopID = 0;
}

LRESULT CALLBACK SoundWndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l) {
	if (msg == WM_APP) {
		DWORD id = l;
		sDSSound* sound = nullptr;
		if (id & 0x80000000) {
			for (size_t i = 0; i < loopingSounds.size(); i++) {
				if (loopingSounds[i]->id == id) {
					sound = loopingSounds[i];
					break;
				}
			}
		} else {
			for (size_t i = 0; i < playingSounds.size(); i++) {
				if (playingSounds[i]->id == id) {
					sound = playingSounds[i];
					break;
				}
			}
		}
		if (!sound) return 0;
		LONG e = 0;
		LONG_PTR p1 = 0, p2 = 0;
		while (!FAILED(sound->pEvent->GetEvent(&e, &p1, &p2, 0))) {
			sound->pEvent->FreeEventParams(e, p1, p2);
			if (e == EC_COMPLETE) {
				if (id & 0x80000000) {
					LONGLONG pos = 0;
					sound->pSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning);
					sound->pControl->Run();
				} else {
					for (size_t i = 0; i < playingSounds.size(); i++) {
						if (playingSounds[i] == sound) {
							FreeSound(sound);
							playingSounds.erase(playingSounds.begin() + i);
							return 0;
						}
					}
				}
			}
		}
		return 0;
	}
	return DefWindowProc(wnd, msg, w, l);
}

static void CreateSndWnd() {
	dlog("Creating sfall sound callback windows.", DL_INIT);
	if (Graphics::mode == 0) CoInitialize(0);

	WNDCLASSEX wcx;
	memset(&wcx, 0, sizeof(wcx));
	wcx.cbSize = sizeof(wcx);
	wcx.lpfnWndProc = SoundWndProc;
	wcx.hInstance = GetModuleHandleA(0);
	wcx.lpszClassName = "sfallSndWnd";

	RegisterClassEx(&wcx);
	soundwindow = CreateWindow("sfallSndWnd", "SndWnd", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, GetModuleHandleA(0), 0);
	dlogr(" Done", DL_INIT);
}

void __stdcall PauseSfallSound(sDSSound* ptr) {
	ptr->pControl->Pause();
}

void __stdcall ResumeSfallSound(sDSSound* ptr) {
	ptr->pControl->Run();
}

static long CalculateVolumeDB(long masterVolume, long passVolume) {
	const int volOffset = -100;  // maximum volume
	const int minVolume = -2048;

	float multiply = (32767.0f / masterVolume);
	float volume = (((float)passVolume / 32767.0f) * 100.0f) / multiply; // calculate %
	volume = (minVolume * volume) / 100.0f;
	return static_cast<long>(minVolume - volume) + volOffset;
}

static void __cdecl SfallSoundVolume(sDSSound* sound, int type, long passVolume) {
	long loopVolume, sfxVolume, masterVolume = *(DWORD*)FO_VAR_master_volume;

	if (masterVolume > 0 && passVolume > 0) {
		loopVolume = CalculateVolumeDB(masterVolume, passVolume);
		if (type = 2) sfxVolume = CalculateVolumeDB(masterVolume, *(DWORD*)FO_VAR_sndfx_volume);
	} else {
		if (masterVolume == 0) {
			loopVolume = sfxVolume = -9999; // mute
		} else if (type = 0) { // for music
			if (musicLoopPtr) {
				Sound::StopSfallSound(musicLoopPtr->id);
				musicLoopPtr = nullptr;
			}
			return;
		}
	}

	if (sound) {
		sound->pAudio->put_Volume(loopVolume);
	} else {
		for (DWORD i = 0; i < loopingSounds.size(); i++) {
			loopingSounds[i]->pAudio->put_Volume(loopVolume);
		}
		if (type = 2) { // sfx
			for (DWORD i = 0; i < playingSounds.size(); i++) {
				playingSounds[i]->pAudio->put_Volume(sfxVolume);
			}
		}
	}
}

static bool IsMute(bool type) {
	//if (*(DWORD*)FO_VAR_master_volume == 0) return true;
	int value;
	if (type) {
		value = *(DWORD*)FO_VAR_background_volume;
	} else {
		value = *(DWORD*)FO_VAR_sndfx_volume;
	}
	return (value == 0);
}

static sDSSound* PlayingSound(wchar_t* path, bool loop) {
	if (!soundwindow) CreateSndWnd();

	if (IsMute(loop)) return nullptr;

	sDSSound* sound = new sDSSound();

	DWORD id = (loop) ? ++loopID : ++playID;
	if (loop) id |= 0x80000000;
	sound->id = id;

	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&sound->pGraph);
	if (hr != S_OK) {
		dlog_f("Error CoCreateInstance: %d", DL_INIT, hr);
		return nullptr;
	}
	sound->pGraph->QueryInterface(IID_IMediaControl, (void**)&sound->pControl);

	if (loop)
		sound->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&sound->pSeek);
	else
		sound->pSeek = nullptr;

	sound->pGraph->QueryInterface(IID_IMediaEventEx, (void**)&sound->pEvent);
	sound->pEvent->SetNotifyWindow((OAHWND)soundwindow, WM_APP, id);
	sound->pGraph->QueryInterface(IID_IBasicAudio, (void**)&sound->pAudio);

	sound->pControl->RenderFile(path);
	sound->pControl->Run();

	if (loop) {
		loopingSounds.push_back(sound);
		SfallSoundVolume(sound, 0, *(DWORD*)FO_VAR_background_volume); // music
	} else {
		playingSounds.push_back(sound);
		SfallSoundVolume(sound, 1, *(DWORD*)FO_VAR_sndfx_volume);
	}
	return sound;
}

static const wchar_t *SoundExtensions[] = { L"mp3", L"wma", L"wav" };

static bool __cdecl SoundFileLoad(DWORD called, const char* path) {
	if (!path || strlen(path) < 4) return false;
	wchar_t buf[256];
	mbstowcs_s(0, buf, path, 256);

	bool isExist = false;
	int len = wcslen(buf) - 3;
	for (int i = 0; i < 3; i++) {
		buf[len] = 0;
		wcscat_s(buf, SoundExtensions[i]);
		if (GetFileAttributesW(buf) & FILE_ATTRIBUTE_DIRECTORY) continue; // also file not found
		isExist = true;
		break;
	}

	bool music = (called == 0x45092B); // from gsound_background_play_
	if (music && musicLoopPtr != nullptr) {
		//if (found && strcmp(path, playingMusicFile) == 0) return true; // don't stop music
		Sound::StopSfallSound(musicLoopPtr->id);
		musicLoopPtr = nullptr;
	}
	if (!isExist) return false;

	if (music) {
		//strcpy_s(playingMusicFile, path);
		musicLoopPtr = PlayingSound(buf, true); // music loop
		if (!musicLoopPtr) return false;
	} else {
		if (!PlayingSound(buf, false)) return false;
	}
	return true;
}

static void __fastcall MakeMusicPath(const char* file) {
	const char* pathFmt = "%s%s.ACM";
	char pathBuf[256];

	sprintf_s(pathBuf, pathFmt, fo::var::sound_music_path1, file);
	if (SoundFileLoad(0x45092B, pathBuf)) return;

	sprintf_s(pathBuf, pathFmt, fo::var::sound_music_path2, file);
	SoundFileLoad(0x45092B, pathBuf);
}

DWORD Sound::PlaySfallSound(const char* path, bool loop) {
	wchar_t buf[256];
	mbstowcs_s(0, buf, path, 256);
	sDSSound* sound = PlayingSound(buf, loop);
	return (loop && sound) ? sound->id : 0;
}

void __stdcall Sound::StopSfallSound(DWORD id) {
	if (!id) return;
	for (size_t i = 0; i < loopingSounds.size(); i++) {
		if (loopingSounds[i]->id == id) {
			sDSSound* sound = loopingSounds[i];
			sound->pControl->Stop();
			FreeSound(sound);
			loopingSounds.erase(loopingSounds.begin() + i);
			return;
		}
	}
}

static void __declspec(naked) soundLoad_hack() {
	static const DWORD SoundLoadHackRet = 0x4AD49E;
	static const DWORD SoundLoadHackEnd = 0x4AD4B6;
	__asm {
		push esi;
		push edi;
		push ebp;
		mov  ebx, eax;
		// end engine code
		push ecx;
		push edx;
		push [esp + 24];
		call SoundFileLoad;
		add  esp, 4;
		pop  edx;
		pop  ecx;
		test al, al;
		jnz  playSfall;
		jmp  SoundLoadHackRet; // play acm
playSfall:
		jmp  SoundLoadHackEnd; // don't play acm (force error)
	}
}

static void __declspec(naked) gsound_background_play_hook() {
	__asm {
		mov  esi, eax;         // store
		mov  ecx, ebp;         // file
		call MakeMusicPath;
		mov  eax, esi;         // restore eax
		jmp  fo::funcoffs::soundDelete_;
	}
}

static void __declspec(naked) gmovie_play_hook_stop() {
	__asm {
		mov  eax, musicLoopPtr;
		test eax, eax;
		jz   skip;
		mov  eax, [eax]; // musicLoopPtr->id
		push ecx;
		push edx;
		push eax;
		call Sound::StopSfallSound;
		xor  eax, eax;
		mov  musicLoopPtr, eax;
		pop  edx;
		pop  ecx;
		retn;
skip:
		jmp  fo::funcoffs::gsound_background_stop_;
	}
}

static void __declspec(naked) gmovie_play_hook_pause() {
	__asm {
		mov  eax, musicLoopPtr;
		test eax, eax;
		jz   skip;
		push ecx;
		push edx;
		push eax;
		call PauseSfallSound;
		pop  edx;
		pop  ecx;
		retn;
skip:
		jmp  fo::funcoffs::gsound_background_pause_;
	}
}

static void __declspec(naked) gmovie_play_hook_unpause() {
	__asm {
		mov  eax, musicLoopPtr;
		test eax, eax;
		jz   skip;
		push ecx;
		push edx;
		push eax;
		call ResumeSfallSound;
		pop  edx;
		pop  ecx;
		retn;
skip:
		jmp  fo::funcoffs::gsound_background_unpause_;
	}
}

static void __declspec(naked) gsound_background_volume_set_hack() {
	__asm {
		mov  dword ptr ds:[FO_VAR_background_volume], eax;
		push ecx;
		mov  ecx, musicLoopPtr;
		test ecx, ecx;
		jz   skip;
		push edx;
		push eax;
		push 0;
		push ecx;
		call SfallSoundVolume;
		add  esp, 8;
		pop  eax;
		pop  edx;
skip:
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) gsound_master_volume_set_hack() {
	__asm {
		mov  dword ptr ds:[FO_VAR_master_volume], edx;
		push eax;
		push ecx;
		push edx;
		push dword ptr ds:[FO_VAR_background_volume];
		push 2;
		push 0;
		call SfallSoundVolume;
		add  esp, 12;
		pop  edx;
		pop  ecx;
		pop  eax;
		retn;
	}
}

///////////////////////////////////////////////////////////////////////////////

static char attackerSnd[9] = {0};
static char targetSnd[9] = {0};

static void __declspec(naked) combatai_msg_hook() {
	__asm {
		mov  edi, [esp + 0xC]; // lip file from msg
		push eax;
		cmp  eax, FO_VAR_target_str;
		jne  attacker;
		lea  eax, targetSnd;
		jmp  skip;
attacker:
		lea  eax, attackerSnd;
skip:
		push edx;
		push ebx;
		mov  edx, edi;
		mov  ebx, 8;
		call fo::funcoffs::strncpy_;
		pop  ebx;
		pop  edx;
		pop  eax;
		jmp  fo::funcoffs::strncpy_;
	}
}

static void __declspec(naked) ai_print_msg_hook() {
	__asm {
		push eax;
		cmp  edx, FO_VAR_target_str;
		jne  attacker;
		lea  eax, targetSnd;
		jmp  skip;
attacker:
		lea  eax, attackerSnd;
skip:
		push ecx;
		mov  ecx, [eax];
		test cl, cl;
		jz   end;
		call fo::funcoffs::gsound_play_sfx_file_;
end:
		pop  ecx;
		pop  eax;
		jmp  fo::funcoffs::text_object_create_;
	}
}

void Sound::init() {
	LoadGameHook::OnGameReset() += WipeSounds;
	LoadGameHook::OnBeforeGameClose() += WipeSounds;

	int allowDShowSound = GetConfigInt("Sound", "AllowDShowSound", 0);
	if (allowDShowSound > 0) {
		MakeJump(0x4AD499, soundLoad_hack);
		HookCalls(gmovie_play_hook_stop, {0x44E80A, 0x445280}); // only play looping music
		HookCalls(gmovie_play_hook_pause, {0x44E816});
		HookCalls(gmovie_play_hook_unpause, {0x44EA84});
		MakeCall(0x450525, gsound_background_volume_set_hack);
		MakeCall(0x4503CA, gsound_master_volume_set_hack, 1);
		if (allowDShowSound > 1) {
			HookCall(0x450851, gsound_background_play_hook);
		}
		CreateSndWnd();
	}

	if (int sBuff = GetConfigInt("Sound", "NumSoundBuffers", 0)) {
		SafeWrite8(0x451129, (BYTE)sBuff);
	}

	if (GetConfigInt("Sound", "AllowSoundForFloats", 0)) {
		HookCall(0x42B7C7, combatai_msg_hook); // copy msg
		HookCall(0x42B849, ai_print_msg_hook);

		//Yes, I did leave this in on purpose. Will be of use to anyone trying to add in the sound effects
		if (isDebug && iniGetInt("Debugging", "Test_ForceFloats", 0, ::sfall::ddrawIni)) {
			SafeWrite8(0x42B6F5, 0xEB); // bypass chance
		}
	}
}

void Sound::exit() {
	if (soundwindow && Graphics::mode == 0) CoUninitialize();
}

}
