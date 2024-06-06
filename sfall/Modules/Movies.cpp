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

#include <d3d9.h>
#include <dshow.h>
#include <Vmr9.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\SimplePatch.h"

#include "Graphics.h"
#include "Sound.h"
//#include "LoadGameHook.h"

#include "Movies.h"

namespace sfall
{

static DWORD MoviePtrs[MaxMovies];

class CAllocator : public IVMRSurfaceAllocator9, IVMRImagePresenter9 {

#define SAFERELEASE(a) { if (a) { a->Release(); a = nullptr; } }

private:
	ULONG RefCount;
	IVMRSurfaceAllocatorNotify9 *pAllocNotify;

	std::vector<IDirect3DSurface9*> surfaces;

	bool startPresenting = false;
	bool initialized = false;

	HRESULT __stdcall TerminateDevice(DWORD_PTR dwID) {
		dlog_f("\nTerminate Device id: %d\n", DL_INIT, dwID);

		Graphics::ReleaseMovieTexture();

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
		dlog("\nInitialize Device...", DL_INIT);
		initialized = false;

		lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface; // | VMR9AllocFlag_DXVATarget;
		lpAllocInfo->Pool = D3DPOOL_SYSTEMMEM;
		//lpAllocInfo->Format = D3DFMT_X8R8G8B8;

		// Ask the VMR-9 to allocate the surfaces for us.
		for (auto &surface : surfaces) SAFERELEASE(surface);
		surfaces.resize(*lpNumBuffers);

		HRESULT hr = pAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &surfaces[0]);
		if (FAILED(hr)) {
			dlog_f("\nAllocateSurfaceHelper error: 0x%x", DL_MAIN, hr);
			return hr;
		}

		#ifndef NDEBUG
		dlog_f(" Width: %d, Height: %d, Format: %d", DL_INIT, lpAllocInfo->dwWidth, lpAllocInfo->dwHeight, lpAllocInfo->Format);
		#endif

		D3DSURFACE_DESC desc;
		desc.Width = lpAllocInfo->dwWidth;
		desc.Height = lpAllocInfo->dwHeight;
		desc.Format = lpAllocInfo->Format;

		if (Graphics::CreateMovieTexture(desc) != D3D_OK) {
			dlogr(" Failed to create movie texture!", DL_INIT);
		} else {
			initialized = true;
			dlogr(" OK!", DL_INIT);
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
		Graphics::SetMovieTexture(true);
		startPresenting = true;
		return S_OK;
	}

	HRESULT __stdcall StopPresenting(DWORD_PTR dwUserID) {
		dlog("\nStop Presenting.", DL_INIT);
		startPresenting = false;
		return S_OK;
	}

	HRESULT __stdcall PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo) {
		if (startPresenting) {
			IDirect3DTexture9* tex;
			lpPresInfo->lpSurf->GetContainer(IID_IDirect3DTexture9, (LPVOID*)&tex);
			Graphics::ShowMovieFrame(tex);
		}
		return S_OK;
	}

public:
	CAllocator() {
		RefCount = 1;
		pAllocNotify = nullptr;
	}

	ULONG __stdcall AddRef() {
		return ++RefCount;
	}

	ULONG __stdcall Release() {
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
		return pAllocNotify->SetD3DDevice(d3d9Device, hMonitor);
	}

	bool IsInitialized() {
		return initialized;
	}
};

static struct sDSTexture {
	CAllocator *pSFAlloc;
	IGraphBuilder *pGraph;
	IBaseFilter *pVmr;
	IVMRFilterConfig9 *pConfig;
	IVMRSurfaceAllocatorNotify9 *pAlloc;
	IMediaControl *pControl;
	IMediaSeeking *pSeek;
	IBasicAudio *pAudio;
	bool released = false;
} movieInterface;

enum AviState : long {
	Stop,
	ReadyToPlay,
	Playing
};

static AviState aviPlayState;
static DWORD backgroundVolume = 0;

static void PlayMovie(sDSTexture* movie) {
	movie->pControl->Run();
	movie->pAudio->put_Volume(
		Sound::CalculateVolumeDB(fo::var::master_volume, (backgroundVolume) ? backgroundVolume : fo::var::background_volume)
	);
}

static void StopMovie() {
	aviPlayState = AviState::Stop;
	Graphics::SetMovieTexture(false);
	movieInterface.pControl->Stop();
	if (fo::var::getInt(FO_VAR_subtitles) == 0) fo::util::RefreshGNW(); // Note: it is only necessary when in the game
}

DWORD FreeMovie(sDSTexture* movie) {
	if (movie->released) return 0;
	movie->released = true;
	#ifndef NDEBUG
	dlog("\nRelease movie interfaces.", DL_INIT);
	#endif
	if (movie->pControl) movie->pControl->Release();
	if (movie->pSeek) movie->pSeek->Release();
	if (movie->pAlloc) movie->pAlloc->Release();
	if (movie->pSFAlloc) movie->pSFAlloc->Release();
	if (movie->pConfig) movie->pConfig->Release();
	if (movie->pVmr) movie->pVmr->Release();
	if (movie->pGraph) movie->pGraph->Release();
	if (movie->pAudio) movie->pAudio->Release();
	return 0;
}

static void BreakMovie() {
	StopMovie();
	FreeMovie(&movieInterface);
}

DWORD CreateDSGraph(wchar_t* path, sDSTexture* movie) {
	dlog("\nCreating DirectShow graph...", DL_INIT);

	ZeroMemory(movie, sizeof(sDSTexture));

	// Create the Filter Graph Manager.
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&movie->pGraph);
	if (hr != S_OK) return FreeMovie(movie);

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
	movie->pSFAlloc = new CAllocator();

	hr = movie->pVmr->QueryInterface(IID_IVMRSurfaceAllocatorNotify9, (void**)&movie->pAlloc);
	if (hr != S_OK) return FreeMovie(movie);

	hr = movie->pAlloc->AdviseSurfaceAllocator(0, (IVMRSurfaceAllocator9*)movie->pSFAlloc);
	if (hr != S_OK) return FreeMovie(movie);

	hr = movie->pSFAlloc->AdviseNotify(movie->pAlloc);
	if (hr != S_OK) return FreeMovie(movie);
	/****************************************************/

	hr = movie->pGraph->AddFilter(movie->pVmr, L"VMR9");
	if (hr != S_OK) return FreeMovie(movie);

	movie->pGraph->QueryInterface(IID_IBasicAudio, (void**)&movie->pAudio);

	dlog("\nStart rendering file.", DL_MAIN);
	hr = movie->pGraph->RenderFile(path, nullptr);
	if (hr != S_OK) dlog_f(" ERROR: 0x%x", DL_MAIN, hr);

	return (hr == S_OK && movie->pSFAlloc->IsInitialized()) ? 1 : 0;
}

static __int64 endMoviePosition;

// Movie play looping
static DWORD __fastcall PlayMovieLoop(long kCode) {
	static bool onlyOnce = false;

	if (aviPlayState == AviState::ReadyToPlay) {
		aviPlayState = AviState::Playing;
		PlayMovie(&movieInterface);
		onlyOnce = false;
	} else if (aviPlayState == AviState::Stop) { // when device is lost
		return 0;
	}

	if (fo::var::getInt(FO_VAR_subtitles) && fo::var::subtitleList) {
		__asm call fo::funcoffs::movieUpdate_; // for reading subtitles when playing mve
		if (!onlyOnce) {
			onlyOnce = true;
			fo::util::ClearWindow(fo::var::getInt(FO_VAR_GNWWin));
		}
	}

	if (kCode != -1) {
		StopMovie();
		return 0; // break play
	}
	Sleep(10); // idle delay, reduces dropping frames when playing AVI

	__int64 pos;
	movieInterface.pSeek->GetCurrentPosition(&pos);

	bool isPlayEnd = (endMoviePosition == pos);
	if (isPlayEnd) StopMovie();

	return (!isPlayEnd); // 0 - for breaking play
}

static __declspec(naked) void gmovie_play_hook() {
	__asm {
		push ecx;
		call fo::funcoffs::get_input_; // also windows message pump
		mov  ecx, eax;
		call PlayMovieLoop;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void gmovie_play_hook_input() {
	__asm {
		xor eax, eax;
		dec eax;
		retn; // return -1
	}
}

static void __stdcall PreparePlayMovie() {
	// if subtitles are disabled then mve videos will not be played
	if (fo::var::getInt(FO_VAR_subtitles) || !fo::var::subtitleList) {
		// patching MVE_rmStepMovie_ for game subtitles
		SafeWrite8(0x4F5F40, CodeType::Ret); // blocking sfShowFrame_ for disabling the display of mve video frames

		// TODO

		#ifdef NDEBUG // mute sound because mve file is still being played to get subtitles
		backgroundVolume = fo::func::gsound_background_volume_get_set(0);
		#endif
	}
}

static __declspec(naked) void gmovie_play_hook_run() {
	__asm {
		call fo::funcoffs::movieRun_;
		mov  ebx, ecx;
		call PreparePlayMovie;
		mov  ecx, ebx;
		retn;
	}
}

static DWORD __fastcall PrepareLoadMovie(const DWORD id) {
	static long isNotWMR = -1;
	// Verify that the VMR exists on this system
	if (isNotWMR == -1) {
		IBaseFilter* pBF = nullptr;
		if (SUCCEEDED(CoCreateInstance(CLSID_VideoMixingRenderer9, 0, CLSCTX_INPROC, IID_IBaseFilter, (LPVOID*)&pBF))) {
			pBF->Release();
			isNotWMR = 0;
		} else {
			dlogr("Error: Video Mixing Renderer (VMR9) capabilities are required.", DL_MAIN);
			isNotWMR = 1;
		}
	}
	if (isNotWMR) return 0;

	// Get file path in unicode
	wchar_t path[MAX_PATH];
	long pos = 0;

	char* master_patches = fo::var::patches;
	while (pos < MAX_PATH && master_patches[pos]) path[pos] = master_patches[pos++];
	if ((pos + 10) >= MAX_PATH) return 0;

	wcscpy(&path[pos], L"\\art\\cuts\\");
	pos += 10;

	char* movie = (char*)(MoviePtrs[id] - pos);
	while (pos < MAX_PATH && movie[pos]) path[pos] = movie[pos++];
	if (pos >= MAX_PATH) return 0;

	wcscpy(&path[pos - 3], L"avi");

	// Check for existance of file
	if (GetFileAttributesW(path) & FILE_ATTRIBUTE_DIRECTORY) return 0; // also file not found

	// Create texture and graph filter
	if (!CreateDSGraph(path, &movieInterface)) return FreeMovie(&movieInterface);

	HookCall(0x44E932, gmovie_play_hook_run);
	HookCall(0x44E949, gmovie_play_hook_input); // block get_input_
	HookCall(0x44E937, gmovie_play_hook);       // looping call moviePlaying_

	movieInterface.pSeek->GetStopPosition(&endMoviePosition);
	aviPlayState = AviState::ReadyToPlay;

	return 1; // for playing AVI
}

static void __stdcall PlayMovieRestore() {
	#ifndef NDEBUG
	dlog("\nPlay Movie Restore.", DL_INIT);
	#endif

	SafeWrite32(0x44E933, 0x39191); // call movieRun_
	SafeWrite32(0x44E938, 0x3934C); // call moviePlaying_
	SafeWrite32(0x44E94A, 0x7A22A); // call get_input_

	if (fo::var::getInt(FO_VAR_subtitles) || !fo::var::subtitleList) {
		SafeWrite8(0x4F5F40, 0x53); // push ebx
		if (backgroundVolume) backgroundVolume = fo::func::gsound_background_volume_get_set(backgroundVolume); // restore volume
	}
	aviPlayState = AviState::Stop;
	FreeMovie(&movieInterface);
}

static __declspec(naked) void gmovie_play_hack() {
	static const DWORD gmovie_play_addr = 0x44E695;
	__asm {
		cmp  eax, MaxMovies;
		jge  failPlayAvi;
		pushadc;
		mov  ecx, eax;
		call PrepareLoadMovie;
		test eax,eax;
		popadc;
		jz   failPlayAvi;
		push offset return; // return to here "return" label
failPlayAvi:
		push ebx;
		push ecx;
		push esi;
		push edi;
		push ebp;
		jmp  gmovie_play_addr; // continue
return:
		push ecx;
		call PlayMovieRestore;
		pop  ecx;
		xor  eax, eax;
		retn;
	}
}

static __declspec(naked) void gmovie_play_hook_stop() {
	__asm {
		cmp  aviPlayState, Playing;
		jne  skip;
		mov  ebx, ecx;
		call StopMovie;
		mov  ecx, ebx;
skip:
		jmp  fo::funcoffs::movieStop_;
	}
}

///////////////////////////////////////////////////////////////////////////////

#define DEFAULT_MOVIES    17 // max vanilla movies

char MoviePaths[MaxMovies * 65];
static DWORD Artimer1DaysCheckTimer;

static __declspec(naked) void Artimer1DaysCheckHack() {
	static const DWORD Artimer1DaysCheckJmp = 0x4A3790;
	static const DWORD Artimer1DaysCheckJmpLess = 0x4A37A9;
	__asm {
		cmp edx, Artimer1DaysCheckTimer;
		jl  less;
		jmp Artimer1DaysCheckJmp;
less:
		jmp Artimer1DaysCheckJmpLess;
	}
}

static __declspec(naked) void gmovie_play_hack_subpal() {
	__asm {
		xor eax, eax;
		lea edx, [ebp * 4];
		cmp ebp, DEFAULT_MOVIES;
		cmovb eax, edx;
		retn;
	}
}

static __declspec(naked) void op_play_gmovie_hack() {
	__asm {
		mov edx, 0xB; // default play mode flags
		cmp ecx, DEFAULT_MOVIES;
		cmovb edx, eax;
		retn;
	}
}

void SkipOpeningMoviesPatch() {
	int skipOpening = IniReader::GetConfigInt("Misc", "SkipOpeningMovies", 0);
	if (skipOpening) {
		dlogr("Skipping opening movies.", DL_INIT);
		SafeWrite16(0x4809C7, 0x1CEB); // jmps 0x4809E5
		if (skipOpening == 2) BlockCall(0x4426A1); // game_splash_screen_
	}
}

static __declspec(naked) void LostFocus() {
	long isActive; // _GNW95_isActive
	__asm { // prolog
		pushad;
		mov  ebp, esp;
		sub  esp, __LOCAL_SIZE;
		mov  isActive, eax;
	}

	Sound::SoundLostFocus(isActive);

	if (aviPlayState == AviState::Playing) {
		if (isActive)
			movieInterface.pControl->Run();
		else if (Graphics::mode == 4)
			BreakMovie();
		else
			movieInterface.pControl->Pause();
	}
	__asm { // epilog
		mov esp, ebp;
		popad;
		retn;
	}
}

/*
	WIP: Task
	Implement subtitle output from the need to play an mve file in the background.
*/
bool Movies::DirectShowMovies() {
	int allowDShowMovies = IniReader::GetConfigInt("Graphics", "AllowDShowMovies", 0);
	if (allowDShowMovies > 0) {
		Graphics::AviMovieWidthFit = (
			allowDShowMovies >= 2 ||
			(HRP::Setting::ExternalEnabled() && IniReader::GetInt("MOVIES", "MOVIE_SIZE", 0, ".\\f2_res.ini") == 2)
		);
		MakeJump(0x44E690, gmovie_play_hack);
		HookCall(0x44E993, gmovie_play_hook_stop);
		/* NOTE: At this address 0x487781 (movieStart_), HRP by Mash changes the callback procedure to display mve frames. */
		return true;
	}
	return false;
}

void Movies::init() {
	dlogr("Applying movie patch.", DL_INIT);

	// Pause and resume movie/sound playback when the game loses focus
	fo::func::set_focus_func(LostFocus);

	char optName[8] = "Movie";
	for (int i = 0; i < MaxMovies; i++) {
		int index = 65 * i;
		MoviePtrs[i] = (DWORD)&MoviePaths[index];
		MoviePaths[index + 64] = '\0';

		_itoa_s(i + 1, &optName[5], 3, 10);
		if (i < DEFAULT_MOVIES) {
			IniReader::GetConfigString("Misc", optName, fo::var::movie_list[i], &MoviePaths[index], 65);
		} else {
			IniReader::GetConfigString("Misc", optName, "", &MoviePaths[index], 65);
		}
	}
	SafeWriteBatch<DWORD>((DWORD)MoviePtrs, {0x44E6AE, 0x44E721, 0x44E75E, 0x44E78A}); // gmovie_play_
	MakeCall(0x44E896, gmovie_play_hack_subpal, 2);
	MakeCall(0x45A1C9, op_play_gmovie_hack);

	DWORD days = SimplePatch<DWORD>(0x4A36EC, "Misc", "MovieTimer_artimer4", 360, 0);
	days = SimplePatch<DWORD>(0x4A3747, "Misc", "MovieTimer_artimer3", 270, 0, days);
	days = SimplePatch<DWORD>(0x4A376A, "Misc", "MovieTimer_artimer2", 180, 0, days);
	Artimer1DaysCheckTimer = IniReader::GetConfigInt("Misc", "MovieTimer_artimer1", 90);
	if (Artimer1DaysCheckTimer != 90) {
		Artimer1DaysCheckTimer = max(0, min(days, Artimer1DaysCheckTimer));
		char s[255];
		sprintf_s(s, "Applying patch: MovieTimer_artimer1 = %d.", Artimer1DaysCheckTimer);
		dlogr(s, DL_INIT);
		MakeJump(0x4A378B, Artimer1DaysCheckHack);
	}

	SkipOpeningMoviesPatch();
}

//void Movies::exit() {
//}

}
