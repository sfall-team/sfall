/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include <d3d9.h>
#include <dshow.h>
#include <Vmr9.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\SimplePatch.h"
#include "..\Logging.h"
#include "Graphics.h"
#include "LoadGameHook.h"

#include "Movies.h"

namespace sfall
{

static DWORD MoviePtrs[MaxMovies];
char MoviePaths[MaxMovies * 65];

static bool aviIsReadyToPlay = false;

class CAllocator : public IVMRSurfaceAllocator9, IVMRImagePresenter9 {
private:
	ULONG RefCount;
	IVMRSurfaceAllocatorNotify9 *pAllocNotify;

	std::vector<IDirect3DSurface9*> surfaces;
	IDirect3DTexture9* pTex;
	IDirect3DTexture9* mTex;

	HRESULT __stdcall TerminateDevice(DWORD_PTR dwID) {
		dlog_f("\nTerminate Device id: %d\n", DL_INIT, dwID);

		SAFERELEASE(pTex);
		SAFERELEASE(mTex);

		for (auto &surface : surfaces) SAFERELEASE(surface);
		surfaces.clear();

		return S_OK;
	}

	HRESULT __stdcall QueryInterface(const IID &riid, void** ppvObject) {
		HRESULT hr = E_NOINTERFACE;
		if (ppvObject == nullptr) {
			hr = E_POINTER;
		} else if (riid == IID_IVMRSurfaceAllocator9) {
			*ppvObject = static_cast<IVMRSurfaceAllocator9*>(this);
			AddRef();
			hr = S_OK;
		} else if (riid == IID_IVMRImagePresenter9) {
			*ppvObject = static_cast<IVMRImagePresenter9*>(this);
			AddRef();
			hr = S_OK;
		} else if (riid == IID_IUnknown) {
			*ppvObject = static_cast<IUnknown*>(static_cast<IVMRSurfaceAllocator9*>(this));
			AddRef();
			hr = S_OK;
		}
		return hr;
	}

	HRESULT __stdcall InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers) {
		dlog("\nInitialize Device:", DL_INIT);

		lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;
		lpAllocInfo->Pool = D3DPOOL_SYSTEMMEM;

		// Ask the VMR-9 to allocate the surfaces for us.
		for (auto &surface : surfaces) SAFERELEASE(surface);
		surfaces.resize(*lpNumBuffers);

		HRESULT hr = pAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &surfaces[0]);
		if (FAILED(hr)) return hr;

		dlog_f(" Height: %d,", DL_INIT, lpAllocInfo->dwHeight);
		dlog_f(" Width: %d,", DL_INIT, lpAllocInfo->dwWidth);
		dlog_f(" Format: %d", DL_INIT, lpAllocInfo->Format);

		hr = surfaces[0]->GetContainer(IID_IDirect3DTexture9, (void**)&pTex);
		if (FAILED(hr)) {
			TerminateDevice(-1);
			return hr;
		}

		if (d3d9Device->CreateTexture(lpAllocInfo->dwWidth, lpAllocInfo->dwHeight, 1, 0, lpAllocInfo->Format, D3DPOOL_DEFAULT, &mTex, nullptr) != D3D_OK) {
			dlog(" Failed to create movie texture!", DL_INIT);
		}
		return S_OK;
	}

	HRESULT __stdcall GetSurface(DWORD_PTR dwUserID, DWORD surfaceIndex, DWORD surfaceFlags, IDirect3DSurface9 **lplpSurface) {
		dlog_f("\nGet Surface index: %d", DL_INIT, surfaceIndex);
		if (surfaceIndex >= surfaces.size()) return E_FAIL;

		*lplpSurface = surfaces[surfaceIndex];
		(*lplpSurface)->AddRef();

		dlog(" OK.", DL_INIT);
		return S_OK;
	}

	HRESULT __stdcall StartPresenting(DWORD_PTR dwUserID) {
		dlog("\nStart Presenting.", DL_INIT);
		Graphics::SetMovieTexture(mTex);
		return S_OK;
	}

	HRESULT __stdcall StopPresenting(DWORD_PTR dwUserID) {
		dlog("\nStop Presenting.", DL_INIT);
		return S_OK;
	}

	HRESULT __stdcall PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo) {
		#ifndef NDEBUG
		dlog("\nPresent Image.", DL_INIT);
		#endif
		d3d9Device->UpdateTexture(pTex, mTex);
		return S_OK;
	}

public:
	CAllocator() {
		RefCount = 1;
		pAllocNotify = nullptr;
		mTex = nullptr;
		pTex = nullptr;
	}

	ULONG __stdcall AddRef() {
		return ++RefCount;
	}

	ULONG _stdcall Release() {
		if (--RefCount == 0) {
			TerminateDevice(-2);
			if (pAllocNotify) {
				pAllocNotify->Release();
				pAllocNotify = nullptr;
			}
		}
		return RefCount;
	}

	HRESULT __stdcall AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify) {
		if (!lpIVMRSurfAllocNotify) return E_FAIL;

		pAllocNotify = lpIVMRSurfAllocNotify;
		pAllocNotify->AddRef();

		// Set the device
		HMONITOR hMonitor = d3d9->GetAdapterMonitor(D3DADAPTER_DEFAULT);
		HRESULT hr = pAllocNotify->SetD3DDevice(d3d9Device, hMonitor);

		return hr;
	}

	IDirect3DTexture9* GetMovieTexture() {
		return mTex;
	}
};

struct sDSTexture {
	CAllocator *pMyAlloc;
	IGraphBuilder *pGraph;
	//ICaptureGraphBuilder2 *pBuild;
	IBaseFilter *pVmr;
	IVMRFilterConfig9 *pConfig;
	IVMRSurfaceAllocatorNotify9 *pAlloc;
	IMediaControl *pControl;
	IMediaSeeking *pSeek;
} movieInterface;

void PlayMovie(sDSTexture* movie) {
	movie->pControl->Run();
}

void PauseMovie(sDSTexture* movie) {
	movie->pControl->Pause();
}

void StopMovie(sDSTexture* movie) {
	movie->pControl->Stop();
}
/*
void RewindMovie(sDSTexture* movie) {
	LONGLONG time = 0;
	movie->pSeek->SetPositions(&time, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
}

void SeekMovie(sDSTexture* movie, DWORD shortTime) {
	LONGLONG time = shortTime * 10000;
	movie->pSeek->SetPositions(&time, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
}
*/
DWORD FreeMovie(sDSTexture* movie) {
	#ifndef NDEBUG
	dlog("\nRelease movie interfaces.", DL_INIT);
	#endif
	if (movie->pControl) movie->pControl->Release();
	if (movie->pSeek) movie->pSeek->Release();
	if (movie->pAlloc) movie->pAlloc->Release();
	if (movie->pMyAlloc) movie->pMyAlloc->Release();
	if (movie->pConfig) movie->pConfig->Release();
	if (movie->pVmr) movie->pVmr->Release();
	if (movie->pGraph) movie->pGraph->Release();
	//if (movie->pBuild) movie->pBuild->Release();
	return 0;
}

DWORD CreateDSGraph(wchar_t* path, sDSTexture* movie) {
	dlog("\nCreating DirectShow graph...", DL_INIT);

	ZeroMemory(movie, sizeof(sDSTexture));

	// Create the Capture Graph Builder.
	//HRESULT hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, 0, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&movie->pBuild);
	//if (hr != S_OK) return FreeMovie(movie);

	// Create the Filter Graph Manager.
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&movie->pGraph);
	if (hr != S_OK) return FreeMovie(movie);

	// Initialize the Capture Graph Builder.
	//hr = movie->pBuild->SetFiltergraph(movie->pGraph);
	//if (hr != S_OK) return FreeMovie(movie);

	hr = CoCreateInstance(CLSID_VideoMixingRenderer9, 0, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&movie->pVmr);
	if (hr != S_OK) return FreeMovie(movie);

	hr = movie->pGraph->QueryInterface(IID_IMediaControl, (void**)&movie->pControl);
	hr |= movie->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&movie->pSeek);
	if (hr != S_OK) return FreeMovie(movie);

	hr = movie->pVmr->QueryInterface(IID_IVMRFilterConfig9, (void**)&movie->pConfig);
	if (hr != S_OK) return FreeMovie(movie);

	hr = movie->pConfig->SetRenderingMode(VMR9Mode_Renderless);
	if (hr != S_OK) return FreeMovie(movie);

	/*
		Custom Allocator-Presenter for VMR-9:
		https://docs.microsoft.com/en-us/windows/win32/directshow/supplying-a-custom-allocator-presenter-for-vmr-9
	*/
	movie->pMyAlloc = new CAllocator();

	hr = movie->pVmr->QueryInterface(IID_IVMRSurfaceAllocatorNotify9, (void**)&movie->pAlloc);
	if (hr != S_OK) return FreeMovie(movie);

	hr = movie->pAlloc->AdviseSurfaceAllocator(0, (IVMRSurfaceAllocator9*)movie->pMyAlloc);
	if (hr != S_OK) return FreeMovie(movie);

	hr = movie->pMyAlloc->AdviseNotify(movie->pAlloc);
	if (hr != S_OK) return FreeMovie(movie);
	/****************************************************/

	hr = movie->pGraph->AddFilter(movie->pVmr, L"VMR9");
	if (hr != S_OK) return FreeMovie(movie);

	dlog_f("\nStart rendering file.", DL_INIT);
	hr = movie->pGraph->RenderFile(path, nullptr);
	if (hr != S_OK) dlog_f(" ERROR: %d", DL_INIT, hr);

	#ifndef NDEBUG
	if (movie->pMyAlloc->GetMovieTexture()) dlog_f("\nMovieTex: %d", DL_INIT, *(DWORD*)movie->pMyAlloc->GetMovieTexture());
	#endif
	return (movie->pMyAlloc->GetMovieTexture()) ? 1 : 0;
}

// Movie play looping
static DWORD PlayMovieLoop() {
	Graphics::ShowMovieFrame();

	if (GetAsyncKeyState(VK_ESCAPE)) {
		StopMovie(&movieInterface);
		return 0; // break play
	}

	_int64 pos, end;
	movieInterface.pSeek->GetCurrentPosition(&pos);
	movieInterface.pSeek->GetStopPosition(&end);

	bool isPlayEnd = (end == pos);
	if (isPlayEnd) StopMovie(&movieInterface);

	return !isPlayEnd; // 0 - for breaking play
}

static void __declspec(naked) gmovie_play_hook() {
	__asm {
		push ecx;
		xor  eax, eax;
		call fo::funcoffs::GNW95_process_message_; // windows message pump
		call PlayMovieLoop;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) gmovie_play_hook_wsub() {
	__asm {
		push ecx;
		xor  eax, eax;
		call fo::funcoffs::GNW95_process_message_; // windows message pump
		call fo::funcoffs::movieUpdate_; // for playing mve
		call PlayMovieLoop;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) gmovie_play_hook_input() {
	__asm {
		xor eax,eax;
		dec eax;
		retn; // return -1
	}
}

static DWORD backgroundVolume = 0;

static DWORD __fastcall PreparePlayMovie(const DWORD id) {
	// Get file path in unicode
	wchar_t path[MAX_PATH];
	char* master_patches = fo::var::patches;
	DWORD len = 0;
	while (master_patches[len]) {
		path[len] = master_patches[len]; len++;
	}
	path[len] = 0;
	wcscat_s(path, L"\\art\\cuts\\");
	len = wcslen(path);
	char* movie = &MoviePaths[id * 65] - len;
	while (movie[len]) {
		path[len] = movie[len]; len++;
	}
	wcscpy_s(&path[len - 3], 5, L"avi");

	// Check for existance of file
	if (GetFileAttributesW(path) & FILE_ATTRIBUTE_DIRECTORY) return 0; // also file not found

	// Create texture and graph filter
	if (!CreateDSGraph(path, &movieInterface)) return FreeMovie(&movieInterface);

	HookCall(0x44E949,gmovie_play_hook_input); // block get_input_ (if subtitles are disabled then mve videos will not be played)
	// patching gmovie_play_ for disabled game subtitles
	if (*(DWORD*)FO_VAR_subtitles == 0) {
		HookCall(0x44E937, gmovie_play_hook); // looping call moviePlaying_
		SafeWrite8(0x4CB850, 0xC3); // GNW95_ShowRect_ blocking image rendering from the 'descSurface' surface when subtitles are disabled (optional)
	} else {
		HookCall(0x44E937, gmovie_play_hook_wsub); // looping call moviePlaying_
		//SafeWrite8(0x486654, 0xC3); // blocking movie_MVE_ShowFrame_
		//SafeWrite8(0x486900, 0xC3); // blocking movieShowFrame_
		SafeWrite8(0x4F5F40, 0xC3); // blocking sfShowFrame_ for disabling the display of mve video frames

		#ifdef NDEBUG // mute sound because mve file is still being played to get subtitles
		backgroundVolume = fo::func::gsound_background_volume_get_set(0);
		#endif
	}

	//WIP
	//SafeWrite32(0x4C73B2, 0x2E);
	//BlockCall(0x48827E);
	//BlockCall(0x44E92B);
	//aviIsReadyToPlay = true;

	PlayMovie(&movieInterface);

	return 1; // play AVI
}

static void _stdcall PlayMovieRestore() {
	#ifndef NDEBUG
	dlog("\nPlay Movie Restore.", DL_INIT);
	#endif

	SafeWrite32(0x44E938, 0x3934C); // call moviePlaying_
	SafeWrite32(0x44E94A, 0x7A22A); // call get_input_

	if (*(DWORD*)FO_VAR_subtitles == 0) {
		SafeWrite8(0x4CB850, 0x53); // GNW95_ShowRect_
	} else {
		SafeWrite8(0x4F5F40, 0x53); // push ebx
		if (backgroundVolume) backgroundVolume = fo::func::gsound_background_volume_get_set(backgroundVolume); // restore volume
	}

	Graphics::SetMovieTexture(0);
	FreeMovie(&movieInterface);
	//aviIsReadyToPlay = false;
}

static const DWORD gmovie_play_addr = 0x44E695;
static void __declspec(naked) gmovie_play_hack() {
	__asm {
		cmp  eax, MaxMovies;
		jge  failPlayAvi;
		push ecx;
		push edx;
		push eax;
		mov  ecx, eax;
		call PreparePlayMovie;
		test eax,eax;
		pop  eax;
		pop  edx;
		pop  ecx;
		jz   failPlayAvi;
		push offset return; // return here
failPlayAvi:
		push ebx;
		push ecx;
		push esi;
		push edi;
		push ebp;
		jmp  gmovie_play_addr;
return:
		push ecx;
		call PlayMovieRestore;
		pop  ecx;
		xor  eax, eax;
		retn;
	}
}

///////////////////////////////////////////////////////////////////////////////

struct sDSSound {
	DWORD id;
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
static void* musicLoopPtr = nullptr;
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
	for (DWORD i = 0; i < playingSounds.size(); i++) FreeSound(playingSounds[i]);
	for (DWORD i = 0; i < loopingSounds.size(); i++) FreeSound(loopingSounds[i]);
	playingSounds.clear();
	loopingSounds.clear();
	musicLoopPtr = nullptr;
}

LRESULT CALLBACK SoundWndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l) {
	if (msg == WM_APP) {
		DWORD id = l;
		sDSSound* dssound = nullptr;
		if (id & 0x80000000) {
			for (DWORD i = 0; i < loopingSounds.size(); i++) {
				if (loopingSounds[i]->id == id) {
					dssound = loopingSounds[i];
					break;
				}
			}
		} else {
			for (DWORD i = 0; i < playingSounds.size(); i++) {
				if (playingSounds[i]->id == id) {
					dssound = playingSounds[i];
					break;
				}
			}
		}
		if (!dssound) return 0;
		LONG e = 0;
		LONG_PTR p1 = 0, p2 = 0;
		while (!FAILED(dssound->pEvent->GetEvent(&e, &p1, &p2, 0))) {
			dssound->pEvent->FreeEventParams(e, p1, p2);
			if (e == EC_COMPLETE) {
				if (id & 0x80000000) {
					LONGLONG pos = 0;
					dssound->pSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning);
					dssound->pControl->Run();
				} else {
					for (DWORD i = 0; i < playingSounds.size(); i++) {
						if (playingSounds[i] == dssound) {
							FreeSound(dssound);
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

void _stdcall PauseSfallSound(sDSSound* ptr) {
	ptr->pControl->Pause();
}

void _stdcall ResumeSfallSound(sDSSound* ptr) {
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
				StopSfallSound(musicLoopPtr);
				musicLoopPtr = nullptr;
			}
			return;
		}
	}

	if (sound) {
		sound->pAudio->put_Volume(loopVolume);
	} else {
		for(DWORD i = 0; i < loopingSounds.size(); i++) {
			loopingSounds[i]->pAudio->put_Volume(loopVolume);
		}
		if (type = 2) { // sfx
			for(DWORD i = 0; i < playingSounds.size(); i++) {
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

	sDSSound* result = new sDSSound();

	DWORD id = (loop) ? loopID++ : playID++;
	if (loop) id |= 0x80000000;
	result->id = id;

	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&result->pGraph);
	if (hr != S_OK) {
		dlog_f("Error CoCreateInstance: %d", DL_INIT, hr);
		return nullptr;
	}
	result->pGraph->QueryInterface(IID_IMediaControl, (void**)&result->pControl);

	if (loop)
		result->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&result->pSeek);
	else
		result->pSeek = nullptr;

	result->pGraph->QueryInterface(IID_IMediaEventEx, (void**)&result->pEvent);
	result->pEvent->SetNotifyWindow((OAHWND)soundwindow, WM_APP, id);

	result->pGraph->QueryInterface(IID_IBasicAudio, (void**)&result->pAudio);

	result->pControl->RenderFile(path);
	result->pControl->Run();

	if (loop) {
		loopingSounds.push_back(result);
		SfallSoundVolume(result, 0, *(DWORD*)FO_VAR_background_volume); // music
	} else {
		playingSounds.push_back(result);
		SfallSoundVolume(result, 1, *(DWORD*)FO_VAR_sndfx_volume);
	}
	return result;
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
		StopSfallSound(musicLoopPtr);
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

void* _stdcall PlaySfallSound(const char* path, bool loop) {
	wchar_t buf[256];
	mbstowcs_s(0, buf, path, 256);
	sDSSound* result = PlayingSound(buf, loop);
	return (loop) ? result: 0;
}

void _stdcall StopSfallSound(void* _ptr) {
	sDSSound* ptr = (sDSSound*)_ptr;
	for (DWORD i = 0; i < loopingSounds.size(); i++) {
		if (loopingSounds[i] == ptr) {
			FreeSound(ptr);
			loopingSounds.erase(loopingSounds.begin() + i);
			return;
		}
	}
}

static const DWORD SoundLoadHackRet = 0x4AD49E;
static const DWORD SoundLoadHackEnd = 0x4AD4B6;
static void __declspec(naked) soundLoad_hack() {
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
		jmp  SoundLoadHackRet;  // play acm
playSfall:
		jmp  SoundLoadHackEnd;  // don't play acm (force error)
	}
}

static void __declspec(naked) gsound_background_play_hook() {
	__asm {
		mov  esi, eax;                   // store
		mov  ecx, ebp;                   // file
		call MakeMusicPath;
		mov  eax, esi;                   // restore eax
		jmp  fo::funcoffs::soundDelete_;
	}
}

static void __declspec(naked) gmovie_play_hook_stop() {
	__asm {
		mov  eax, musicLoopPtr;
		test eax, eax;
		jz   skip;
		push ecx;
		push edx;
		push eax;
		call StopSfallSound;
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

static const DWORD Artimer1DaysCheckJmp = 0x4A3790;
static const DWORD Artimer1DaysCheckJmpLess = 0x4A37A9;
static DWORD Artimer1DaysCheckTimer;
static void __declspec(naked) Artimer1DaysCheckHack() {
	__asm {
		cmp edx, Artimer1DaysCheckTimer;
		jl  less;
		jmp Artimer1DaysCheckJmp;
less:
		jmp Artimer1DaysCheckJmpLess;
	}
}

void SkipOpeningMoviesPatch() {
	int skipOpening = GetConfigInt("Misc", "SkipOpeningMovies", 0);
	if (skipOpening) {
		dlog("Skipping opening movies.", DL_INIT);
		SafeWrite16(0x4809C7, 0x1CEB); // jmps 0x4809E5
		if (skipOpening == 2) BlockCall(0x4426A1); // game_splash_screen_
		dlogr(" Done", DL_INIT);
	}
}

void Movies::init() {
	dlog("Applying movie patch.", DL_INIT);

	LoadGameHook::OnGameReset() += WipeSounds;
	LoadGameHook::OnBeforeGameClose() += WipeSounds;

	if (*((DWORD*)0x00518DA0) != 0x00503300) {
		dlog("Error: The value at address 0x001073A0 is not equal to 0x00503300.", DL_INIT);
	}
	for (int i = 0; i < MaxMovies; i++) {
		MoviePtrs[i] = (DWORD)&MoviePaths[65 * i];
		MoviePaths[i * 65 + 64] = 0;
		char ininame[8];
		strcpy_s(ininame, "Movie");
		_itoa_s(i + 1, &ininame[5], 3, 10);
		if (i < 17) {
			GetConfigString("Misc", ininame, (char*)(0x518DA0 + i * 4), &MoviePaths[i * 65], 65);
		} else {
			GetConfigString("Misc", ininame, "", &MoviePaths[i * 65], 65);
		}
	}
	dlog(".", DL_INIT);
	SafeWrite32(0x44E6AE, (DWORD)MoviePtrs);
	SafeWrite32(0x44E721, (DWORD)MoviePtrs);
	SafeWrite32(0x44E75E, (DWORD)MoviePtrs);
	SafeWrite32(0x44E78A, (DWORD)MoviePtrs);
	dlog(".", DL_INIT);
	/*
		WIP: Task
		Necessary to implement setting the volume according to the volume control in Fallout settings.
		Add fade effects and brightness adjustment for videos.
		Implement subtitle output from the need to play an mve file in the background.
		Fix minor bugs.
	*/
	if (Graphics::mode != 0 && GetConfigInt("Graphics", "AllowDShowMovies", 0)) {
		MakeJump(0x44E690, gmovie_play_hack);
		/* NOTE: At this address 0x487781, HRP changes the callback procedure to display mve frames. */
	}
	dlogr(" Done", DL_INIT);

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

	DWORD days = SimplePatch<DWORD>(0x4A36EC, "Misc", "MovieTimer_artimer4", 360, 0);
	days = SimplePatch<DWORD>(0x4A3747, "Misc", "MovieTimer_artimer3", 270, 0, days);
	days = SimplePatch<DWORD>(0x4A376A, "Misc", "MovieTimer_artimer2", 180, 0, days);
	Artimer1DaysCheckTimer = GetConfigInt("Misc", "MovieTimer_artimer1", 90);
	if (Artimer1DaysCheckTimer != 90) {
		Artimer1DaysCheckTimer = max(0, min(days, Artimer1DaysCheckTimer));
		char s[255];
		sprintf_s(s, "Applying patch: MovieTimer_artimer1 = %d.", Artimer1DaysCheckTimer);
		dlog(s, DL_INIT);
		MakeJump(0x4A378B, Artimer1DaysCheckHack);
		dlogr(" Done", DL_INIT);
	}

	// Should be AFTER the PlayMovieHook setup above
	SkipOpeningMoviesPatch();
}

void Movies::exit() {
	if (soundwindow && Graphics::mode == 0) CoUninitialize();
}

}
