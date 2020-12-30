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

#include <unordered_map>
#include <algorithm>
#include <dsound.h>
#include <dshow.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "Graphics.h"
#include "LoadGameHook.h"

#include "Sound.h"

namespace sfall
{

#define SAFERELEASE(a) { if (a) { a->Release(); } }

enum SoundType : DWORD {
	sfx_loop    = 0, // sfall
	sfx_single  = 1, // sfall
	game_sfx    = 2,
	game_master = 3
};

enum SoundMode : long {
	single_play  = 0, // uses sfx volume
	loop_play    = 1, // uses sfx volume
	music_play   = 2, // uses background volume
	speech_play  = 3, // uses speech volume
	engine_music_play = -1 // used when playing game music in an alternative format (not used from scripts)
};

enum SoundFlags : DWORD {
	looping = 0x10000000,
	on_stop = 0x20000000,
	restore = 0x40000000, // restore background game music on stop play
	engine  = 0x80000000
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

static fo::ACMSoundData* acmSoundData = nullptr; // currently loaded ACM file

static std::unordered_map<std::string, std::wstring> soundsFiles;

DWORD playID = 0;
DWORD loopID = 0;

static HWND soundwindow = 0;

static sDSSound* speechSound = nullptr;     // currently playing sfall speech sound
static sDSSound* backgroundMusic = nullptr; // currently playing sfall background music

static bool deathSceneSpeech = false;
static bool lipsPlaying = false;

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
	backgroundMusic = nullptr;
	speechSound = nullptr;
	playID = 0;
	loopID = 0;
	lipsPlaying = false;
}

LRESULT CALLBACK SoundWndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l) {
	if (msg == WM_APP) {
		if (!(l & 0xA0000000)) return 0;
		sDSSound* sound = reinterpret_cast<sDSSound*>(l & ~0xA0000000);
		LONG e = 0;
		LONG_PTR p1 = 0, p2 = 0;
		if (!FAILED(sound->pEvent->GetEvent(&e, &p1, &p2, 0))) {
			if (e == EC_COMPLETE) {
				if (sound->id & SoundFlags::looping) {
					LONGLONG pos = 0;
					sound->pSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning);
					sound->pControl->Run();
					sound->pEvent->FreeEventParams(e, p1, p2);
				} else {
					sound->pEvent->FreeEventParams(e, p1, p2);
					if (sound->id & SoundFlags::on_stop) { // speech sound playback is completed
						fo::var::main_death_voiceover_done = 1;
						fo::var::endgame_subtitle_done = 1;
						lipsPlaying = false;
						speechSound = nullptr;
					}
					playingSounds.erase(
						std::find_if(playingSounds.cbegin(), playingSounds.cend(), [&](const sDSSound* snd) { return snd->id == sound->id; })
					);
					FreeSound(sound);
				}
			}
		}
		return 0;
	}
	return DefWindowProc(wnd, msg, w, l);
}

static void CreateSndWnd() {
	dlog("Creating sfall sound callback window.", DL_INIT);
	if (Graphics::mode == 0) CoInitialize(0);

	WNDCLASSEX wcx;
	std::memset(&wcx, 0, sizeof(wcx));

	wcx.cbSize = sizeof(wcx);
	wcx.lpfnWndProc = SoundWndProc;
	wcx.hInstance = GetModuleHandleA(0);
	wcx.lpszClassName = "sfallSndWnd";

	RegisterClassEx(&wcx);
	soundwindow = CreateWindowA("sfallSndWnd", "SndWnd", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, wcx.hInstance, 0);
	dlogr(" Done", DL_INIT);
}

// Get sound duration in seconds
static DWORD GetSpeechDurationTime() {
	if (!speechSound || !speechSound->pSeek) return 0;
	speechSound->pSeek->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
	__int64 outVal = -1;
	speechSound->pSeek->GetDuration(&outVal);
	return (outVal != -1) ? static_cast<DWORD>(outVal / 10000000) + 1 : 0;
}

static DWORD GetSpeechPlayingPosition() {
	if (!speechSound) return 0;
	__int64 pos;
	speechSound->pSeek->GetCurrentPosition(&pos);
	return static_cast<DWORD>(pos << 1);
}

void __fastcall PauseSfallSound(sDSSound* sound) {
	if (sound) sound->pControl->Pause();
}

void __fastcall ResumeSfallSound(sDSSound* sound) {
	if (sound) sound->pControl->Run();
}

void __stdcall PauseAllSfallSound() {
	for (sDSSound* sound : loopingSounds) sound->pControl->Pause();
	for (sDSSound* sound : playingSounds) sound->pControl->Pause();
}

void __stdcall ResumeAllSfallSound() {
	for (sDSSound* sound : loopingSounds) sound->pControl->Run();
	for (sDSSound* sound : playingSounds) sound->pControl->Run();
}

long Sound::CalculateVolumeDB(long masterVolume, long passVolume) {
	if (masterVolume <= 0 || passVolume <= 0) return -9999; // mute

	const int volOffset = -100;  // adjust the maximum volume
	const int minVolume = -2100;

	passVolume = masterVolume * passVolume / 32767;
	long volume = static_cast<long>(minVolume * ((float)passVolume / 32767.0f)); // calculate % value
	return (minVolume - volume) + volOffset;
}

/*
	master_volume:     sound=null            type=game_master     passVolume=background_volume
	background_volume: sound=backgroundMusic type=background_loop passVolume=background_volume
	sfx_volume:        sound=null            type=game_sfx        passVolume=sndfx_volume
	speech_volume:     sound=sound           type=sfx_single      passVolume=speech_volume
*/
static void __cdecl SetSoundVolume(sDSSound* sound, SoundType type, long passVolume) {
	long volume, sfxVolume, masterVolume = fo::var::master_volume;

	volume = Sound::CalculateVolumeDB(masterVolume, passVolume);
	if (type == SoundType::game_master) sfxVolume = Sound::CalculateVolumeDB(masterVolume, fo::var::sndfx_volume);

	if (sound) {
		sound->pAudio->put_Volume(volume);
	} else {
		if (type == SoundType::game_sfx) sfxVolume = volume;
		bool sfx_or_master = (type == SoundType::game_sfx || type == SoundType::game_master);
		for (size_t i = 0; i < loopingSounds.size(); i++) {
			if (sfx_or_master) {
				if ((loopingSounds[i]->id & 0xF0000000) == SoundFlags::looping) { // only for sfx loop mode
					loopingSounds[i]->pAudio->put_Volume(sfxVolume);
					continue;
				} else
					if (type == SoundType::game_sfx) continue;
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

static bool IsMute(SoundMode mode) {
	//if (fo::var::master_volume == 0) return true;

	switch (mode) {
	case SoundMode::single_play:
		return (fo::var::sndfx_volume == 0);
	case SoundMode::speech_play:
		return (fo::var::speech_volume == 0);
	default:
		return (fo::var::background_volume == 0);
	}
}

/*
	single_play: mode 0 - single sound playback (with sound overlay)
	loop_play:   mode 1 - loop sound playback (with sound overlay)
	music_play:  mode 2 - loop sound playback with the background game music turned off
	speech_play: mode 3 -

	Note: when playing via DirectShow there is a small delay of ~100 ms
*/
static sDSSound* PlayingSound(const wchar_t* pathFile, SoundMode mode, long adjustVolume = 0, bool isPaused = false) {
	if (!soundwindow) CreateSndWnd();

	if (IsMute(mode)) return nullptr;
	bool isLoop = (mode != SoundMode::single_play && mode != SoundMode::speech_play);

	sDSSound* sound = new sDSSound();

	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&sound->pGraph);
	if (hr != S_OK) {
		dlog_f("Error CoCreateInstance: %d", DL_INIT, hr);
		return nullptr;
	}
	sound->pGraph->QueryInterface(IID_IMediaControl, (void**)&sound->pControl);

	if (mode == SoundMode::speech_play || isLoop)
		sound->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&sound->pSeek);
	else
		sound->pSeek = nullptr;

	sound->pGraph->QueryInterface(IID_IMediaEventEx, (void**)&sound->pEvent);
	sound->pGraph->QueryInterface(IID_IBasicAudio, (void**)&sound->pAudio);
	sound->pControl->RenderFile((BSTR)pathFile);

	sound->id = (isLoop) ? ++loopID : ++playID;
	if (isLoop) sound->id |= SoundFlags::looping; // sfx loop sound

	switch (mode) {
	case SoundMode::engine_music_play:
		sound->id |= SoundFlags::engine; // engine play
		break;
	case SoundMode::music_play:
		sound->id |= SoundFlags::restore; // restore background game music on stop
		if (backgroundMusic)
			Sound::StopSfallSound(backgroundMusic->id);
		else {
			__asm call fo::funcoffs::gsound_background_stop_;
		}
		backgroundMusic = sound;
		break;
	case SoundMode::speech_play:
		sound->pSeek->SetTimeFormat(&TIME_FORMAT_SAMPLE);
		sound->id |= SoundFlags::on_stop;
		break;
	}

	sound->pEvent->SetNotifyWindow((OAHWND)soundwindow, WM_APP, ((unsigned long)sound | 0xA0000000));
	sound->pControl->Run();

	if (isLoop) {
		SetSoundVolume(sound, SoundType::sfx_loop, ((mode == SoundMode::loop_play) ? fo::var::sndfx_volume : fo::var::background_volume) - adjustVolume);
		loopingSounds.push_back(sound);
	} else {
		SetSoundVolume(sound, SoundType::sfx_single, ((mode == SoundMode::speech_play) ? fo::var::speech_volume : fo::var::sndfx_volume) - adjustVolume);
		if (mode == SoundMode::speech_play) speechSound = sound;
		if (isPaused) {
			sound->pControl->Pause(); // for delayed playback
		}
		playingSounds.push_back(sound);
	}
	return sound;
}

enum PlayType : signed char {
	sfx    = 0,
	music  = 1,
	lips   = 2,
	speech = 3,
	slides = 4 // speech for endgame slideshow
};

static bool PrePlaySoundFile(PlayType playType, const wchar_t* file, bool isExist) {
	if (playType == PlayType::music && backgroundMusic != nullptr) {
		//if (found && strcmp(path, playingMusicFile) == 0) return true; // don't stop music
		Sound::StopSfallSound(backgroundMusic->id);
		backgroundMusic = nullptr;
	}
	if (!isExist) return false;

	if (playType == PlayType::music) {
		backgroundMusic = PlayingSound(file, SoundMode::engine_music_play); // background music loop
		if (!backgroundMusic) return false;
	} else {
		if (!PlayingSound(file, ((playType >= PlayType::lips) ? SoundMode::speech_play : SoundMode::single_play), 0, (playType == PlayType::slides))) {
			return false;
		}
		if (playType == PlayType::lips) {
			lipsPlaying = true;
			return false;
		}
		deathSceneSpeech = false;
	}
	return true;
}

static const wchar_t *SoundExtensions[] = { L"mp3", L"wma", L"wav" };

// TODO: Add DirectSound support for wav format
static bool SearchAlternativeFormats(const char* path, PlayType playType, std::string &pathFile) {
	size_t len = 0;
	wchar_t wPath[MAX_PATH];
	if (playType != PlayType::music) {
		char* master_patches = fo::var::patches; // all sfx/speech sounds must be placed in patches folder
		while (master_patches[len]) wPath[len] = master_patches[len++];
		wPath[len++] = L'\\';
	}

	const char* _path = path - len;
	while (_path[len]) wPath[len] = _path[len++];
	if (len >= MAX_PATH) return false;
	wPath[len] = L'\0';
	len -= 3; // the position of the first character of the file extension

	bool isExist = false;
	for (int i = 0; i < 3; i++) {
		int j = len;
		wPath[j++] = SoundExtensions[i][0];
		wPath[j++] = SoundExtensions[i][1];
		wPath[j]   = SoundExtensions[i][2];

		if (GetFileAttributesW(wPath) & FILE_ATTRIBUTE_DIRECTORY) continue; // also file not found

		isExist = true;
		break;
	}
	soundsFiles.emplace(std::move(pathFile), (isExist) ? wPath : L"");

	return PrePlaySoundFile(playType, wPath, isExist);
}

static bool __fastcall SoundFileLoad(PlayType playType, const char* path) {
	if (!path) return false;
	std::string pathFile = path;
	if (pathFile.length() <= 4) return false;

	auto it = soundsFiles.find(pathFile);
	if (it != soundsFiles.cend()) {
		return PrePlaySoundFile(playType, it->second.c_str(), (it->second.length() > 0));
	}
	return SearchAlternativeFormats(path, playType, pathFile);
}

static void __fastcall MakeMusicPath(const char* file) {
	const char* pathFmt = "%s%s.ACM";
	char pathBuf[MAX_PATH];

	sprintf_s(pathBuf, pathFmt, fo::var::sound_music_path1, file);
	if (SoundFileLoad(PlayType::music, pathBuf)) return;

	sprintf_s(pathBuf, pathFmt, fo::var::sound_music_path2, file);
	SoundFileLoad(PlayType::music, pathBuf);
}

DWORD Sound::PlaySfallSound(const char* path, long mode) {
	wchar_t buf[MAX_PATH];
	size_t len = 0;

	while (len < MAX_PATH && path[len]) {
		char ch = path[len];
		if (ch == ':' || (ch == '.' && path[len + 1] == '.')) return 0;
		buf[len++] = ch;
	}
	if (len <= 3 || len >= MAX_PATH) return 0;
	buf[len] = L'\0';

	short volAdjust = (mode & 0x7FFF0000) >> 16;
	mode &= 0xF;
	if (mode > SoundMode::music_play) mode = SoundMode::music_play;
	sDSSound* sound = PlayingSound(buf, (SoundMode)mode, volAdjust);

	return (mode && sound) ? sound->id : 0;
}

void __stdcall Sound::StopSfallSound(DWORD id) {
	if (!(id & 0xFFFFFF)) return;
	for (size_t i = 0; i < loopingSounds.size(); i++) {
		if (loopingSounds[i]->id == id) {
			sDSSound* sound = loopingSounds[i];
			sound->pControl->Stop();
			FreeSound(sound);
			loopingSounds.erase(loopingSounds.begin() + i);
			if (!(id & SoundFlags::engine) && id & SoundFlags::restore) {
				__asm call fo::funcoffs::gsound_background_restart_last_;
			}
			return;
		}
	}
}

static void __fastcall ReleaseSound(sDSSound* sound) {
	sound->pControl->Stop();
	auto itEl = std::find(playingSounds.cbegin(), playingSounds.cend(), sound);
	if (itEl != playingSounds.cend()) {
		playingSounds.erase(itEl);
	} else {
		/*itEl = std::find(loopingSounds.cbegin(), loopingSounds.cend(), sound);
		if (itEl != loopingSounds.cend()) {
			loopingSounds.erase(itEl);
		}*/
	}
	FreeSound(sound);
}

static void __declspec(naked) soundLoad_hack_A() {
	static const DWORD SoundLoadHackEnd = 0x4AD4CC;
	__asm {
		cmp  dword ptr [esp + 16 + 4 + 0x11C + 4], 0x45145F + 5; // called from gsound_load_sound_volume_
		jne  skip;
		mov  acmSoundData, ebx;
		retn; // exit for SFX sounds for which the volume must be set relative to the player's location (see gsound_compute_relative_volume_)
skip:
		mov  acmSoundData, 0;
		mov  esi, [esp + 16 + 4];
		cmp  esi, 0x46620D + 5; // called from soundStartInterpret_ (op_soundplay_)
		je   playACM;
		pushadc;
		cmp  esi, 0x450926 + 5; // called from gsound_background_play_
		sete cl;                // PlayType::sfx / PlayType::music
		je   load;
		cmp  esi, 0x47B6B5 + 5; // called from lips_make_speech_
		je   jlips;
		cmp  esi, 0x450EB4 + 5; // called from gsound_speech_play_
		jne  load;
		cmp  dword ptr [esp + 28 + 4 + 0x114 + 4], 0x440200 + 5; // called from endgame_load_voiceover_
		sete cl;
		inc  cl;
jlips:
		add  cl, 2; // PlayType::lips / PlayType::speech / PlayType::slides
load:
		mov  edx, eax; // eax - path to file
		call SoundFileLoad;
		test al, al;
		popadc;
		jnz  playSfallSound;
		mov  acmSoundData, ebx;
playACM:
		retn; // play acm
playSfallSound:
		add  esp, 4;
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

static void __declspec(naked) main_death_scene_hook() {
	__asm {
		mov  deathSceneSpeech, 1;
		call fo::funcoffs::gsound_speech_play_;
		cmp  deathSceneSpeech, 0
		je   playSfall;
		mov  deathSceneSpeech, 0;
		retn; // play acm
playSfall:
		xor  eax, eax;
		retn;
	}
}

static void __declspec(naked) soundLoad_hack_B() {
	__asm {
		xor  ebp, ebp;
		cmp  dword ptr [esp + 16 + 4], 0x46620D + 5; // called from soundStartInterpret_ (op_soundplay_)
		cmovne ebp, ebx;
		mov  acmSoundData, ebp;
		retn;
	}
}

static fo::AudioDecode* __fastcall SoundFormatChange(fo::AudioDecode* decode/*, fo::AudioFile* audio*/) {
	/*
		example calculation
		nBlockAlign = (nBitsPerSample / 8) * nChannels
		nAvgBytesPerSec = nSamplesPerSec * nBlockAlign
	*/
	if (decode) {
		bool recalc = false;
		WAVEFORMATEX* wave = acmSoundData->lpwfxFormat;
		// support stereo for SFX and speech sound files with 44.1 kHz
		// this condition fixes the problem for mono sound files when the number of channels in the ACM file header is set to 2
		if (decode->out_SampleRate == 44100 && decode->out_Channels != wave->nChannels) {
			wave->nChannels = (WORD)decode->out_Channels;
			wave->nBlockAlign = (wave->wBitsPerSample / 8) * wave->nChannels;
			recalc = true;
		}
		if (decode->out_SampleRate != wave->nSamplesPerSec) {
			wave->nSamplesPerSec = decode->out_SampleRate;
			recalc = true;
		}
		if (recalc) wave->nAvgBytesPerSec = wave->nSamplesPerSec * wave->nBlockAlign;
	}
	return decode;
}

static void __declspec(naked) audioOpen_hook() {
	static DWORD audioOpen_AddrRet;
	__asm {
		cmp  acmSoundData, 0;
		je   skip;
		pop  audioOpen_AddrRet;
		push offset return;
		jmp  fo::funcoffs::Create_AudioDecoder_;
return:
		//mov  edx, ebp; // audio
		mov  ecx, eax;   // decode
		push audioOpen_AddrRet;
		jmp  SoundFormatChange;
skip:
		jmp  fo::funcoffs::Create_AudioDecoder_;
	}
}

//////////////////////// SLIDES SPEECH SOUND CONTROL //////////////////////////

static void __declspec(naked) endgame_load_voiceover_hack() {
	__asm {
		cmp  speechSound, 0;
		jnz  skip;
		mov  [esp + 0x118 - 0xC + 4], esi;
		retn;
skip:
		call GetSpeechDurationTime;
		push eax;
		fild dword ptr [esp];
		fild dword ptr ds:[FO_VAR_endgame_subtitle_characters];
		add  esp, 4;
		fdivp st(1), st;
		fstp qword ptr [esp + 0x118 - 0x8 + 4];
		retn;
	}
}

static void __declspec(naked) endgame_pan_desert_hack() {
	__asm {
		mov  ecx, speechSound;
		test ecx, ecx;
		jnz  skip;
		mov  ecx, dword ptr ds:[FO_VAR_endgame_voiceover_loaded];
skip:
		retn;
	}
}

static void __declspec(naked) endgame_pan_desert_hook() {
	__asm {
		cmp  speechSound, 0;
		jnz  GetSpeechDurationTime;
		jmp  fo::funcoffs::gsound_speech_length_get_;
	}
}

static void __declspec(naked) endgame_display_image_hack() {
	__asm {
		mov  ecx, speechSound;
		call ResumeSfallSound;
		mov  edx, dword ptr ds:[FO_VAR_endgame_voiceover_loaded];
		retn;
	}
}

static void __declspec(naked) endgame_pan_desert_hack_play() {
	__asm {
		mov  ecx, speechSound;
		call ResumeSfallSound;
		mov  eax, dword ptr ds:[FO_VAR_endgame_voiceover_loaded];
		xor  ecx, ecx;
		retn;
	}
}

//////////////////////// LIPS SPEECH SOUND CONTROL ////////////////////////////

static void __declspec(naked) lips_play_speech_hook() {
	__asm {
		cmp  lipsPlaying, 0;
		jnz  skip;
		jmp  fo::funcoffs::soundPlay_;
skip:
		xor  eax, eax;
		retn;
	}
}

static void __declspec(naked) gdialog_bk_hook() {
	__asm {
		cmp  lipsPlaying, 0;
		jnz  skip;
		jmp  fo::funcoffs::soundPlaying_;
skip:
		or   eax, 1;
		retn;
	}
}

static void __declspec(naked) lips_bkg_proc_hook() {
	__asm {
		cmp  lipsPlaying, 0;
		jnz  skip;
		jmp  fo::funcoffs::soundGetPosition_;
skip:
		jmp  GetSpeechPlayingPosition;
	}
}

static void __declspec(naked) gdialogFreeSpeech_hack() {
	__asm {
		cmp  lipsPlaying, 0;
		jz   skip;
		push ecx;
		push edx;
		mov  ecx,speechSound;
		call ReleaseSound;
		mov  speechSound, 0;
		mov  lipsPlaying, 0;
		pop  edx;
		pop  ecx;
skip:
		cmp  ds:[FO_VAR_gdialog_speech_playing], 0;
		retn;
	}
}

//////////////////// VOLUME AND PLAYBACK SOUND CONTROL ////////////////////////

static void __declspec(naked) gsound_speech_stop_hack() {
	__asm {
		mov  ecx, speechSound;
		test ecx, ecx;
		jz   skip;
		push edx;
		call ReleaseSound;
		mov  speechSound, 0;
		pop  edx;
skip:
		mov  ecx, dword ptr ds:[FO_VAR_gsound_speech_tag];
		retn;
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
		call Sound::StopSfallSound;
		xor  eax, eax;
		mov  backgroundMusic, eax;
		pop  edx;
		pop  ecx;
		retn;
skip:
		jmp  fo::funcoffs::gsound_background_stop_;
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
		jmp  fo::funcoffs::gsound_background_pause_;
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
		jmp  fo::funcoffs::gsound_background_unpause_;
skip:
		retn;
	}
}

static void __declspec(naked) gsound_background_volume_set_hack() {
	__asm {
		mov  dword ptr ds:[FO_VAR_background_volume], eax;
		push ecx;
		mov  ecx, backgroundMusic;
		test ecx, ecx;
		jz   skip;
		push edx;
		push eax;
		push 0;   // SoundType::sfx_loop
		push ecx; // set volume for background music
		call SetSoundVolume;
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
		push 3; // SoundType::game_master
		push 0; // set volume for all sounds
		call SetSoundVolume;
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
		mov  dword ptr ds:[FO_VAR_sndfx_volume], eax;
		push eax;
		push 2; // SoundType::game_sfx
		push 0; // set volume for all sounds
		call SetSoundVolume;
		add  esp, 12;
		pop  edx;
		pop  ecx;
		retn;
	}
}

void Sound::SoundLostFocus(long isActive) {
	if (!loopingSounds.empty() || !playingSounds.empty()) {
		if (isActive) {
			ResumeAllSfallSound();
		} else {
			PauseAllSfallSound();
		}
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

static void __declspec(naked) soundStartInterpret_hook() {
	__asm {
		mov  ebp, eax; // keep sound data
		call fo::funcoffs::soundSetCallback_;
		xor  ebx, ebx;
		mov  eax, [esp + 0x18 - 0x18 + 4]; // play mode flags
		and  eax, 0x80000000; // check raw format flag (for backward compatibility mode)
		jnz  rawFile;
		push ecx;
		push fo::funcoffs::audioFileSize_;
		mov  ecx, fo::funcoffs::audioRead_;
		push eax;
		mov  ebx, fo::funcoffs::audioCloseFile_;
		push fo::funcoffs::audioSeek_;
		mov  edx, fo::funcoffs::audioOpen_;
		push eax;
		mov  eax, ebp;
		call fo::funcoffs::soundSetFileIO_;
		pop  ecx;
		mov  bx, [esp + 0x18 - 0x18 + 4+2]; // get volume adjustment: 0 - max volume, 32767 - mute
		and  bx, ~0x8000;
rawFile:
		xor  edx, edx;
		mov  eax, ds:[FO_VAR_sndfx_volume];
		sub  ax, bx;    // reduce volume
		cmovg edx, eax; // volume > 0
		mov  eax, ebp;
		jmp  fo::funcoffs::soundVolume_; // set sfx volume
	}
}

///////////////////////////////////////////////////////////////////////////////

static fo::GameObject* relativeObject;

static void __declspec(naked) gsound_compute_relative_volume_hook() {
	__asm {
		mov relativeObject, ecx;
		jmp fo::funcoffs::win_get_rect_;
	}
}

static void __fastcall SetPanPosition(fo::ACMSoundData* sound) {
	if (!relativeObject) return;

	long distance = fo::func::obj_dist(fo::var::obj_dude, relativeObject);
	if (distance > 5) {
		long direction = fo::func::tile_dir(fo::var::obj_dude->tile, relativeObject->tile);
		bool isRightSide = (direction <= 2);
		long panValue = (distance < 55) ? (distance - 5) * 200 : 10000;
		sound->soundBuffer->SetPan((isRightSide) ? panValue : -panValue); // left mute 10000 ... -10000 right mute
	}
	relativeObject = nullptr; // just in case
}

static void __declspec(naked) gsound_load_sound_volume_hook() {
	__asm {
		call fo::funcoffs::soundVolume_;
		mov  ecx, ebx; // sound
		jmp  SetPanPosition;
	}
}

constexpr int SampleRate = 44100; // 44.1kHz

void Sound::init() {
	// Set the 44.1kHz sample rate for the primary sound buffer
	SafeWrite32(0x44FDBC, SampleRate);
	LoadGameHook::OnAfterGameInit() += []() { fo::var::sampleRate = SampleRate / 2; }; // Revert to 22kHz for secondary sound buffers

	LoadGameHook::OnGameReset() += WipeSounds;
	LoadGameHook::OnBeforeGameClose() += WipeSounds;

	// Enable support for panning sfx sounds
	SafeWrite8(0x45237E, (*(BYTE*)0x45237E) | 4); // set mode for DSBCAPS_CTRLPAN
	HookCall(0x45158D, gsound_compute_relative_volume_hook);
	HookCall(0x451483, gsound_load_sound_volume_hook);

	HookCall(0x44E816, gmovie_play_hook_pause);
	HookCall(0x44EA84, gmovie_play_hook_unpause);
	MakeCall(0x450525, gsound_background_volume_set_hack);
	MakeCall(0x4503CA, gsound_master_volume_set_hack, 1);
	MakeCall(0x45042C, gsound_set_sfx_volume_hack);

	void* soundLoad_func;

	int allowDShowSound = GetConfigInt("Sound", "AllowDShowSound", 0);
	if (allowDShowSound > 0) {
		soundLoad_func = soundLoad_hack_A; // main hook

		HookCalls(gmovie_play_hook_stop, {0x44E80A, 0x445280}); // only play looping music
		if (allowDShowSound > 1) {
			HookCall(0x450851, gsound_background_play_hook);
		}

		HookCall(0x4813EE, main_death_scene_hook);
		MakeCall(0x451038, gsound_speech_stop_hack, 1);
		MakeCall(0x440286, endgame_load_voiceover_hack, 2);
		MakeCall(0x43FCED, endgame_pan_desert_hack, 1);
		HookCall(0x43FCF9, endgame_pan_desert_hook);
		// play slideshow speech
		MakeCall(0x43FEF3, endgame_pan_desert_hack_play, 1);
		MakeCall(0x43FF37, endgame_pan_desert_hack_play, 1);
		MakeCall(0x4400B2, endgame_display_image_hack, 1);

		// lips sounds hacks
		HookCall(0x47ACDE, lips_play_speech_hook);
		HookCall(0x47AAF6, lips_bkg_proc_hook);
		HookCall(0x447B68, gdialog_bk_hook);
		MakeCall(0x4450C5, gdialogFreeSpeech_hack, 2);

		CreateSndWnd();
	} else {
		soundLoad_func = soundLoad_hack_B;
	}
	// Support 44.1kHz sample rate for ACM files
	MakeCall(0x4AD4D6, soundLoad_func, 1);
	HookCalls(audioOpen_hook, {
		0x41AA3A, // audiofOpen_
		0x41A4A4, // audioOpen_
		0x4A96CC  // sfxc_decode_
	});

	int sBuff = GetConfigInt("Sound", "NumSoundBuffers", 0);
	if (sBuff > 4) {
		SafeWrite8(0x451129, (sBuff > 32) ? (BYTE)32 : (BYTE)sBuff);
	}

	if (GetConfigInt("Sound", "AllowSoundForFloats", 0)) {
		HookCall(0x42B7C7, combatai_msg_hook); // copy msg
		HookCall(0x42B849, ai_print_msg_hook);

		//Yes, I did leave this in on purpose. Will be of use to anyone trying to add in the sound effects
		if (isDebug && iniGetInt("Debugging", "Test_ForceFloats", 0, ::sfall::ddrawIni)) {
			SafeWrite8(0x42B6F5, CodeType::JumpShort); // bypass chance
		}
	}

	// Support for ACM audio file playback and volume control for the soundplay script function
	HookCall(0x4661B3, soundStartInterpret_hook);
}

void Sound::exit() {
	if (soundwindow && Graphics::mode == 0) CoUninitialize();
}

}
