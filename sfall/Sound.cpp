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

#include "main.h"
#include "FalloutEngine.h"
#include "Graphics.h"

#include "Sound.h"

#define SAFERELEASE(a) { if (a) { a->Release(); } }

enum SoundType : unsigned long {
	SNDTYPE_sfx_loop    = 0, // sfall
	SNDTYPE_sfx_single  = 1, // sfall
	SNDTYPE_game_sfx    = 2,
	SNDTYPE_game_master = 3
};

enum SoundMode : long {
	SNDMODE_single_play = 0, // uses sfx volume
	SNDMODE_loop_play   = 1, // uses sfx volume
	SNDMODE_music_play  = 2, // uses background volume
	SNDMODE_engine_music_play = -1 // used when playing game music in an alternative format (not used from scripts)
};

enum SoundFlags : unsigned long {
	SNDFLAG_looping = 0x10000000,
	SNDFLAG_restore = 0x40000000, // restore background game music on stop play
	SNDFLAG_engine  = 0x80000000
};

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
static sDSSound* backgroundMusic = nullptr; // currently playing sfall background music
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

void WipeSounds() {
	for (size_t i = 0; i < playingSounds.size(); i++) FreeSound(playingSounds[i]);
	for (size_t i = 0; i < loopingSounds.size(); i++) FreeSound(loopingSounds[i]);
	playingSounds.clear();
	loopingSounds.clear();
	backgroundMusic = nullptr;
	playID = 0;
	loopID = 0;
}

LRESULT CALLBACK SoundWndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l) {
	if (msg == WM_APP) {
		DWORD id = l;
		sDSSound* sound = nullptr;
		long index = -1;
		if (id & SNDFLAG_looping) {
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
					index = i;
					break;
				}
			}
		}

		LONG e = 0;
		LONG_PTR p1 = 0, p2 = 0;
		if (sound && !FAILED(sound->pEvent->GetEvent(&e, &p1, &p2, 0))) {
			sound->pEvent->FreeEventParams(e, p1, p2);
			if (e == EC_COMPLETE) {
				if (id & SNDFLAG_looping) {
					LONGLONG pos = 0;
					sound->pSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning);
					sound->pControl->Run();
				} else {
					FreeSound(sound);
					playingSounds.erase(playingSounds.begin() + index);
				}
			}
		}
		return 0;
	}
	return DefWindowProc(wnd, msg, w, l);
}

static void CreateSndWnd() {
	dlog("Creating sfall sound callback windows.", DL_INIT);
	if (GraphicsMode == 0) CoInitialize(0);

	WNDCLASSEX wcx;
	std::memset(&wcx, 0, sizeof(wcx));

	wcx.cbSize = sizeof(wcx);
	wcx.lpfnWndProc = SoundWndProc;
	wcx.hInstance = GetModuleHandleA(0);
	wcx.lpszClassName = "sfallSndWnd";

	RegisterClassEx(&wcx);
	soundwindow = CreateWindow("sfallSndWnd", "SndWnd", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, GetModuleHandleA(0), 0);
	dlogr(" Done", DL_INIT);
}

void __stdcall PauseSfallSound(sDSSound* sound) {
	sound->pControl->Pause();
}

void __stdcall ResumeSfallSound(sDSSound* sound) {
	sound->pControl->Run();
}

void __stdcall PauseAllSfallSound() {
	std::vector<sDSSound*>::const_iterator it;
	sDSSound* sound = nullptr;
	for (it = loopingSounds.begin(); it != loopingSounds.end(); ++it) {
		sound = *it;
		sound->pControl->Pause();
	}
	for (it = playingSounds.begin(); it != playingSounds.end(); ++it) {
		sound = *it;
		sound->pControl->Pause();
	}
}

void __stdcall ResumeAllSfallSound() {
	std::vector<sDSSound*>::const_iterator it;
	sDSSound* sound = nullptr;
	for (it = loopingSounds.begin(); it != loopingSounds.end(); ++it) {
		sound = *it;
		sound->pControl->Run();
	}
	for (it = playingSounds.begin(); it != playingSounds.end(); ++it) {
		sound = *it;
		sound->pControl->Run();
	}
}

static long CalculateVolumeDB(long masterVolume, long passVolume) {
	if (masterVolume <= 0 || passVolume <= 0) return -9999; // mute

	const int volOffset = -100;  // adjust the maximum volume
	const int minVolume = -2048;

	float multiply = (32767.0f / masterVolume);
	float volume = (((float)passVolume / 32767.0f) * 100.0f) / multiply; // calculate %
	volume = (minVolume * volume) / 100.0f;
	return static_cast<long>(minVolume - volume) + volOffset;
}

/*
	master_volume:     sound=0 type=game_master passVolume=background_volume
	background_volume: sound=backgroundMusic type=background_loop passVolume=background_volume
	sfx_volume:        sound=0 type=game_sfx passVolume=sndfx_volume
 */
static void __cdecl SfallSoundVolume(sDSSound* sound, SoundType type, long passVolume) {
	long volume, sfxVolume, masterVolume = *ptr_master_volume;

	volume = CalculateVolumeDB(masterVolume, passVolume);
	if (type == SNDTYPE_game_master) sfxVolume = CalculateVolumeDB(masterVolume, *ptr_sndfx_volume);

	if (sound) {
		sound->pAudio->put_Volume(volume);
	} else {
		if (type == SNDTYPE_game_sfx) sfxVolume = volume;
		bool sfx_or_master = (type == SNDTYPE_game_sfx || type == SNDTYPE_game_master);
		for (size_t i = 0; i < loopingSounds.size(); i++) {
			if (sfx_or_master) {
				if ((loopingSounds[i]->id & 0xF0000000) == SNDFLAG_looping) { // only for sfx loop mode
					loopingSounds[i]->pAudio->put_Volume(sfxVolume);
					continue;
				} else
					if (type == SNDTYPE_game_sfx) continue;
			}
			loopingSounds[i]->pAudio->put_Volume(volume);
		}
		if (sfx_or_master) {
			for (size_t i = 0; i < playingSounds.size(); i++) {
				playingSounds[i]->pAudio->put_Volume(sfxVolume);
			}
		}
	}
}

static bool IsMute(bool type) {
	//if (*ptr_master_volume == 0) return true;
	int value;
	if (type) {
		value = *ptr_background_volume;
	} else {
		value = *ptr_sndfx_volume;
	}
	return (value == 0);
}

/*
	single_play: mode 0 - single sound playback (with sound overlay)
	loop_play:   mode 1 - loop sound playback (with sound overlay)
	music_play:  mode 2 - loop sound playback with the background game music turned off
*/
static sDSSound* PlayingSound(wchar_t* path, SoundMode mode) {
	if (!soundwindow) CreateSndWnd();

	bool isLoop = (mode != SNDMODE_single_play);
	if (IsMute(isLoop)) return nullptr;

	sDSSound* sound = new sDSSound();

	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&sound->pGraph);
	if (hr != S_OK) {
		dlog_f("Error CoCreateInstance: %d", DL_INIT, hr);
		return nullptr;
	}
	sound->pGraph->QueryInterface(IID_IMediaControl, (void**)&sound->pControl);

	if (isLoop)
		sound->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&sound->pSeek);
	else
		sound->pSeek = nullptr;

	sound->pGraph->QueryInterface(IID_IMediaEventEx, (void**)&sound->pEvent);
	sound->pGraph->QueryInterface(IID_IBasicAudio, (void**)&sound->pAudio);

	sound->id = (isLoop) ? ++loopID : ++playID;
	if (isLoop) sound->id |= SNDFLAG_looping; // sfx loop sound

	if (mode == SNDMODE_music_play) {
		sound->id |= SNDFLAG_restore; // restore background game music on stop
		if (backgroundMusic)
			StopSfallSound(backgroundMusic->id);
		else {
			__asm call gsound_background_stop_;
		}
		backgroundMusic = sound;
	}
	else if (mode == SNDMODE_engine_music_play) sound->id |= SNDFLAG_engine; // engine play

	sound->pEvent->SetNotifyWindow((OAHWND)soundwindow, WM_APP, sound->id);
	sound->pControl->RenderFile(path);
	sound->pControl->Run();

	if (isLoop) {
		loopingSounds.push_back(sound);
		SfallSoundVolume(sound, SNDTYPE_sfx_loop, (mode == SNDMODE_loop_play) ? *ptr_sndfx_volume : *ptr_background_volume);
	} else {
		playingSounds.push_back(sound);
		SfallSoundVolume(sound, SNDTYPE_sfx_single, *ptr_sndfx_volume);
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
	if (music && backgroundMusic != nullptr) {
		//if (found && strcmp(path, playingMusicFile) == 0) return true; // don't stop music
		StopSfallSound(backgroundMusic->id);
		backgroundMusic = nullptr;
	}
	if (!isExist) return false;

	if (music) {
		//strcpy_s(playingMusicFile, path);
		backgroundMusic = PlayingSound(buf, SNDMODE_engine_music_play); // background music loop
		if (!backgroundMusic) return false;
	} else {
		if (!PlayingSound(buf, SNDMODE_single_play)) return false;
	}
	return true;
}

static void __fastcall MakeMusicPath(const char* file) {
	const char* pathFmt = "%s%s.ACM";
	char pathBuf[256];

	sprintf_s(pathBuf, pathFmt, *ptr_sound_music_path1, file);
	if (SoundFileLoad(0x45092B, pathBuf)) return;

	sprintf_s(pathBuf, pathFmt, *ptr_sound_music_path2, file);
	SoundFileLoad(0x45092B, pathBuf);
}

DWORD __stdcall PlaySfallSound(const char* path, long mode) {
	wchar_t buf[256];
	mbstowcs_s(0, buf, path, 256);
	sDSSound* sound = PlayingSound(buf, (SoundMode)mode);
	return (mode && sound) ? sound->id : 0;
}

void __stdcall StopSfallSound(DWORD id) {
	if (!(id & 0xFFFFFF)) return;
	for (size_t i = 0; i < loopingSounds.size(); i++) {
		if (loopingSounds[i]->id == id) {
			sDSSound* sound = loopingSounds[i];
			sound->pControl->Stop();
			FreeSound(sound);
			loopingSounds.erase(loopingSounds.begin() + i);
			if (!(id & SNDFLAG_engine) && id & SNDFLAG_restore) {
				__asm call gsound_background_restart_last_;
			}
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
		jmp  soundDelete_;
	}
}

static void __declspec(naked) gmovie_play_hook_stop() {
	__asm {
		mov  eax, backgroundMusic;
		test eax, eax;
		jz   skip;
		mov  eax, [eax]; // backgroundMusic->id
		push ecx;
		push edx;
		push eax;
		call StopSfallSound;
		xor  eax, eax;
		mov  backgroundMusic, eax;
		pop  edx;
		pop  ecx;
		retn;
skip:
		jmp  gsound_background_stop_;
	}
}

static void __declspec(naked) gmovie_play_hook_pause() {
	__asm {
		push ecx;
		push edx;
		call PauseAllSfallSound;
		pop  edx;
		pop  ecx;
		cmp  dword ptr backgroundMusic, 0;
		jnz  skip;
		jmp  gsound_background_pause_;
skip:
		retn;
	}
}

static void __declspec(naked) gmovie_play_hook_unpause() {
	__asm {
		push ecx;
		push edx;
		call ResumeAllSfallSound;
		pop  edx;
		pop  ecx;
		cmp  dword ptr backgroundMusic, 0;
		jnz  skip;
		jmp  gsound_background_unpause_;
skip:
		retn;
	}
}

static void __declspec(naked) gsound_background_volume_set_hack() {
	__asm {
		mov  dword ptr ds:[_background_volume], eax;
		push ecx;
		mov  ecx, backgroundMusic;
		test ecx, ecx;
		jz   skip;
		push edx;
		push eax;
		push 0;   // SNDTYPE_sfx_loop
		push ecx; // set volume for background music
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
		mov  dword ptr ds:[_master_volume], edx;
		push eax;
		push ecx;
		push edx;
		push dword ptr ds:[_background_volume];
		push 3; // SNDTYPE_game_master
		push 0; // set volume for all sounds
		call SfallSoundVolume;
		add  esp, 12;
		pop  edx;
		pop  ecx;
		pop  eax;
		retn;
	}
}

static void __declspec(naked) gsound_set_sfx_volume_hack() {
	__asm {
		push ecx;
		push edx;
		mov  dword ptr ds:[_sndfx_volume], eax;
		push eax;
		push 2; // SNDTYPE_game_sfx
		push 0; // set volume for all sounds
		call SfallSoundVolume;
		add  esp, 12;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void SoundLostFocus() {
	long isActive;
	__asm push ecx;
	__asm mov  isActive, eax;

	if (!loopingSounds.empty() || !playingSounds.empty()) {
		if (isActive) {
			ResumeAllSfallSound();
		} else {
			PauseAllSfallSound();
		}
	}
	__asm pop ecx;
}

///////////////////////////////////////////////////////////////////////////////

static char attackerSnd[9] = {0};
static char targetSnd[9] = {0};

static void __declspec(naked) combatai_msg_hook() {
	__asm {
		mov  edi, [esp + 0xC]; // lip file from msg
		push eax;
		cmp  eax, _target_str;
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
		call strncpy_;
		pop  ebx;
		pop  edx;
		pop  eax;
		jmp  strncpy_;
	}
}

static void __declspec(naked) ai_print_msg_hook() {
	__asm {
		push eax;
		cmp  edx, _target_str;
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
		call gsound_play_sfx_file_;
end:
		pop  ecx;
		pop  eax;
		jmp  text_object_create_;
	}
}

void SoundInit() {
	HookCall(0x44E816, gmovie_play_hook_pause);
	HookCall(0x44EA84, gmovie_play_hook_unpause);
	MakeCall(0x450525, gsound_background_volume_set_hack);
	MakeCall(0x4503CA, gsound_master_volume_set_hack, 1);
	MakeCall(0x45042C, gsound_set_sfx_volume_hack);

	// Pause and resume sound playback when the game loses focus
	SetFocusFunc(SoundLostFocus);

	int allowDShowSound = GetConfigInt("Sound", "AllowDShowSound", 0);
	if (allowDShowSound > 0) {
		MakeJump(0x4AD499, soundLoad_hack);
		const DWORD gmoviePlayStopAddr[] = {0x44E80A, 0x445280};
		HookCalls(gmovie_play_hook_stop, gmoviePlayStopAddr); // only play looping music
		if (allowDShowSound > 1) {
			HookCall(0x450851, gsound_background_play_hook);
		}
		CreateSndWnd();
	}

	int sBuff = GetConfigInt("Sound", "NumSoundBuffers", 0);
	if (sBuff > 0 && sBuff <= 32) {
		SafeWrite8(0x451129, (BYTE)sBuff);
	}

	if (GetConfigInt("Sound", "AllowSoundForFloats", 0)) {
		HookCall(0x42B7C7, combatai_msg_hook); // copy msg
		HookCall(0x42B849, ai_print_msg_hook);

		//Yes, I did leave this in on purpose. Will be of use to anyone trying to add in the sound effects
		if (isDebug && iniGetInt("Debugging", "Test_ForceFloats", 0, ddrawIniDef)) {
			SafeWrite8(0x42B6F5, 0xEB); // bypass chance
		}
	}
}

void SoundExit() {
	if (soundwindow && GraphicsMode == 0) CoUninitialize();
}