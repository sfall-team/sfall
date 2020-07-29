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

#include "main.h"
#include "FalloutEngine.h"

#include "Graphics.h"
#include "Sound.h"

#include "Movies.h"

static DWORD MoviePtrs[MaxMovies];

class CAllocator : public IVMRSurfaceAllocator9, IVMRImagePresenter9 {

#define SAFERELEASE(a) { if (a) { a->Release(); a = nullptr; } }

private:
	ULONG RefCount;
	IVMRSurfaceAllocatorNotify9 *pAllocNotify;

	std::vector<IDirect3DSurface9*> surfaces;
	//IDirect3DTexture9* pTex;
	IDirect3DTexture9* mTex;

	bool isStartPresenting;

	HRESULT __stdcall TerminateDevice(DWORD_PTR dwID) {
		dlog_f("\nTerminate Device id: %d\n", DL_INIT, dwID);

		//SAFERELEASE(pTex);
		SAFERELEASE(mTex);

		for (std::vector<IDirect3DSurface9*>::iterator it = surfaces.begin(); it != surfaces.end(); ++it) {
			SAFERELEASE((*it));
		}
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

		lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface; // | VMR9AllocFlag_DXVATarget;
		lpAllocInfo->Pool = D3DPOOL_SYSTEMMEM;

		// Ask the VMR-9 to allocate the surfaces for us.
		for (std::vector<IDirect3DSurface9*>::iterator it = surfaces.begin(); it != surfaces.end(); ++it) {
			SAFERELEASE((*it));
		}
		surfaces.resize(*lpNumBuffers);

		HRESULT hr = pAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &surfaces[0]);
		if (FAILED(hr)) return hr;

		#ifndef NDEBUG
		dlog_f(" Width: %d,", DL_INIT, lpAllocInfo->dwWidth);
		dlog_f(" Height: %d,", DL_INIT, lpAllocInfo->dwHeight);
		dlog_f(" Format: %d", DL_INIT, lpAllocInfo->Format);
		#endif

		//hr = surfaces[0]->GetContainer(IID_IDirect3DTexture9, (void**)&pTex);
		//if (FAILED(hr)) {
		//	TerminateDevice(-1);
		//	return hr;
		//}

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
		Gfx_SetMovieTexture(mTex);
		isStartPresenting = true;
		return S_OK;
	}

	HRESULT __stdcall StopPresenting(DWORD_PTR dwUserID) {
		dlog("\nStop Presenting.", DL_INIT);
		isStartPresenting = false;
		return S_OK;
	}

	HRESULT __stdcall PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo) {
		if (isStartPresenting) {
			IDirect3DTexture9* tex;
			lpPresInfo->lpSurf->GetContainer(IID_IDirect3DTexture9, (LPVOID*)&tex);
			d3d9Device->UpdateTexture(tex, mTex);
			Gfx_ShowMovieFrame();
		}
		return S_OK;
	}

public:
	CAllocator() {
		RefCount = 1;
		pAllocNotify = nullptr;
		mTex = nullptr;
		//pTex = nullptr;
		isStartPresenting = false;
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
		HRESULT hr = pAllocNotify->SetD3DDevice(d3d9Device, hMonitor);

		return hr;
	}

	IDirect3DTexture9* GetMovieTexture() {
		return mTex;
	}
};

struct sDSTexture {
	CAllocator *pSFAlloc;
	IGraphBuilder *pGraph;
	IBaseFilter *pVmr;
	IVMRFilterConfig9 *pConfig;
	IVMRSurfaceAllocatorNotify9 *pAlloc;
	IMediaControl *pControl;
	IMediaSeeking *pSeek;
	IBasicAudio   *pAudio;
} movieInterface;

enum AviState : long {
	AVISTATE_Stop,
	AVISTATE_ReadyToPlay,
	AVISTATE_Playing
};

static AviState aviPlayState;
static DWORD backgroundVolume = 0;

void PlayMovie(sDSTexture* movie) {
	movie->pControl->Run();
	movie->pAudio->put_Volume(
		Sound_CalculateVolumeDB(*ptr_master_volume, (backgroundVolume) ? backgroundVolume : *ptr_background_volume)
	);
}

void PauseMovie(sDSTexture* movie) {
	movie->pControl->Pause();
}

void StopMovie() {
	aviPlayState = AVISTATE_Stop;
	movieInterface.pControl->Stop();
	Gfx_SetMovieTexture(0);
	if (*(DWORD*)_subtitles == 0) RefreshGNW(); // Note: it is only necessary when the game is loaded
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
	if (movie->pSFAlloc) movie->pSFAlloc->Release();
	if (movie->pConfig) movie->pConfig->Release();
	if (movie->pVmr) movie->pVmr->Release();
	if (movie->pGraph) movie->pGraph->Release();
	if (movie->pAudio) movie->pAudio->Release();
	return 0;
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

	dlog("\nStart rendering file.", DL_INIT);
	hr = movie->pGraph->RenderFile(path, nullptr);
	if (hr != S_OK) dlog_f(" ERROR: %d", DL_INIT, hr);

	return (hr == S_OK && movie->pSFAlloc->GetMovieTexture()) ? 1 : 0;
}

static __int64 endMoviePosition;

// Movie play looping
static DWORD PlayMovieLoop() {
	static bool onlyOnce = false;

	if (aviPlayState == AVISTATE_ReadyToPlay) {
		aviPlayState = AVISTATE_Playing;
		PlayMovie(&movieInterface);
		onlyOnce = false;
	}

	if (*(DWORD*)_subtitles) {
		__asm call movieUpdate_; // for reading subtitles when playing mve
		if (!onlyOnce) {
			onlyOnce = true;
			ClearWindow(*(DWORD*)_GNWWin);
		}
	}

	if (GetAsyncKeyState(VK_ESCAPE)) {
		StopMovie();
		return 0; // break play
	}

	Sleep(10); // idle delay

	__int64 pos;
	movieInterface.pSeek->GetCurrentPosition(&pos);

	bool isPlayEnd = (endMoviePosition == pos);
	if (isPlayEnd) StopMovie();

	return !isPlayEnd; // 0 - for breaking play
}

static void __declspec(naked) gmovie_play_hook() {
	__asm {
		push ecx;
		call GNW95_process_message_; // windows message pump
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

static DWORD __fastcall PreparePlayMovie(const DWORD id) {
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

	char* master_patches = *ptr_patches;
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

	HookCall(0x44E949, gmovie_play_hook_input); // block get_input_ (if subtitles are disabled then mve videos will not be played)
	HookCall(0x44E937, gmovie_play_hook);       // looping call moviePlaying_

	// patching MVE_rmStepMovie_ for game subtitles
	if (*(DWORD*)_subtitles) {
		SafeWrite8(0x4F5F40, CODETYPE_Ret); // blocking sfShowFrame_ for disabling the display of mve video frames

		#ifdef NDEBUG // mute sound because mve file is still being played to get subtitles
		backgroundVolume = GsoundBackgroundVolumeGetSet(0);
		#endif
	}
	movieInterface.pSeek->GetStopPosition(&endMoviePosition);
	aviPlayState = AVISTATE_ReadyToPlay;

	return 1; // for playing AVI
}

static void __stdcall PlayMovieRestore() {
	#ifndef NDEBUG
	dlog("\nPlay Movie Restore.", DL_INIT);
	#endif

	SafeWrite32(0x44E938, 0x3934C); // call moviePlaying_
	SafeWrite32(0x44E94A, 0x7A22A); // call get_input_

	if (*(DWORD*)_subtitles) {
		SafeWrite8(0x4F5F40, 0x53); // push ebx
		if (backgroundVolume) backgroundVolume = GsoundBackgroundVolumeGetSet(backgroundVolume); // restore volume
	}

	aviPlayState = AVISTATE_Stop;
	FreeMovie(&movieInterface);
}

static void __declspec(naked) gmovie_play_hack() {
	static const DWORD gmovie_play_addr = 0x44E695;
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

static void __declspec(naked) gmovie_play_hook_stop() {
	__asm {
		cmp  aviPlayState, AVISTATE_Playing;
		jne  skip;
		call StopMovie;
skip:
		jmp  movieStop_;
	}
}

///////////////////////////////////////////////////////////////////////////////

char MoviePaths[MaxMovies * 65];
static DWORD Artimer1DaysCheckTimer;

static void __declspec(naked) Artimer1DaysCheckHack() {
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

void SkipOpeningMoviesPatch() {
	int skipOpening = GetConfigInt("Misc", "SkipOpeningMovies", 0);
	if (skipOpening) {
		dlog("Skipping opening movies.", DL_INIT);
		SafeWrite16(0x4809C7, 0x1CEB); // jmps 0x4809E5
		if (skipOpening == 2) BlockCall(0x4426A1); // game_splash_screen_
		dlogr(" Done", DL_INIT);
	}
}

void MoviesInit() {
	dlog("Applying movie patch.", DL_INIT);

	if (*((DWORD*)0x00518DA0) != 0x00503300) {
		dlog("Error: The value at address 0x001073A0 is not equal to 0x00503300.", DL_INIT);
	}

	char optName[8] = "Movie";
	for (int i = 0; i < MaxMovies; i++) {
		int index = 65 * i;
		MoviePtrs[i] = (DWORD)&MoviePaths[index];
		MoviePaths[index + 64] = '\0';

		_itoa_s(i + 1, &optName[5], 3, 10);
		if (i < 17) {
			GetConfigString("Misc", optName, ptr_movie_list[i], &MoviePaths[index], 65);
		} else {
			GetConfigString("Misc", optName, "", &MoviePaths[index], 65);
		}
	}
	dlog(".", DL_INIT);
	const DWORD movieListAddr[] = {0x44E6AE, 0x44E721, 0x44E75E, 0x44E78A}; // gmovie_play_
	SafeWriteBatch<DWORD>((DWORD)MoviePtrs, movieListAddr);
	dlog(".", DL_INIT);

	/*
		WIP: Task
		Implement subtitle output from the need to play an mve file in the background.
	*/
	if (GraphicsMode != 0) {
		int allowDShowMovies = GetConfigInt("Graphics", "AllowDShowMovies", 0);
		if (allowDShowMovies > 0) {
			MakeJump(0x44E690, gmovie_play_hack);
			HookCall(0x44E993, gmovie_play_hook_stop);
			if (allowDShowMovies > 1) AviMovieWidthFit = true;
			/* NOTE: At this address 0x487781, HRP changes the callback procedure to display mve frames. */
		}
	}
	dlogr(" Done", DL_INIT);

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

	SkipOpeningMoviesPatch();
}
