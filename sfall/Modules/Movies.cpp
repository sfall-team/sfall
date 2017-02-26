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

#include "..\main.h"

#include <vector> // should be above DX SDK includes to avoid warning 4995
#include <d3d9.h>
#include <dshow.h>
#include <Vmr9.h>

#include "..\FalloutEngine\Fallout2.h"
#include "..\SimplePatch.h"
#include "..\Logging.h"
#include "Graphics.h"
#include "Movies.h"

static DWORD MoviePtrs[MaxMovies];
char MoviePaths[MaxMovies * 65];

extern IDirect3D9* d3d9;
extern IDirect3DDevice9* d3d9Device;

#define SAFERELEASE(a) { if (a) { a->Release(); a = 0; } }

class CAllocator : public IVMRSurfaceAllocator9, IVMRImagePresenter9 {
private:
	IDirect3DSurface9* surface;
	IVMRSurfaceAllocatorNotify9 *pAllocNotify;
	ULONG RefCount;
	IDirect3DTexture9* ptex;

public:
	IDirect3DTexture9* tex;

	CAllocator() {
		RefCount = 1;
		surface = NULL;
		pAllocNotify = NULL;
		tex = NULL;
		ptex = NULL;
	}

	ULONG _stdcall AddRef() {
		return ++RefCount;
	}

	ULONG _stdcall Release() {
		if (--RefCount == 0) {
			TerminateDevice(0);
			if (pAllocNotify) {
				pAllocNotify->Release();
				pAllocNotify = NULL;
			}
		}
		return RefCount;
	}

	HRESULT _stdcall QueryInterface(const IID &riid, void** ppvObject) {
		HRESULT hr = E_NOINTERFACE;

		if (ppvObject == NULL) {
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

	HRESULT _stdcall InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers) {
		HRESULT hr;
		//Set the device
		hr = pAllocNotify->SetD3DDevice(d3d9Device, d3d9->GetAdapterMonitor(0));
		//if(hr!=S_OK) return hr;

		lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;
		lpAllocInfo->Pool = D3DPOOL_SYSTEMMEM;
		// Ask the VMR-9 to allocate the surfaces for us.
		hr = pAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &surface);
		if (FAILED(hr)) return hr;

		hr = surface->GetContainer(IID_IDirect3DTexture9, (void**)&ptex);
		if (FAILED(hr)) {
			TerminateDevice(0);
			return hr;
		}

		d3d9Device->CreateTexture(lpAllocInfo->dwWidth, lpAllocInfo->dwHeight, 1, 0, lpAllocInfo->Format, D3DPOOL_DEFAULT, &tex, NULL);

		return S_OK;
	}

	HRESULT _stdcall TerminateDevice(DWORD_PTR dwID) {
		SAFERELEASE(ptex);
		SAFERELEASE(surface);
		SAFERELEASE(tex);
		return S_OK;
	}

	HRESULT _stdcall GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface) {
		if (SurfaceIndex != 0) return E_FAIL;
		*lplpSurface = surface;
		surface->AddRef();
		return S_OK;
	}

	HRESULT _stdcall AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify) {
		if (lpIVMRSurfAllocNotify) {
			pAllocNotify = lpIVMRSurfAllocNotify;
			pAllocNotify->AddRef();
			return S_OK;
		} else {
			return E_FAIL;
		}
	}

	HRESULT _stdcall StartPresenting(DWORD_PTR dwUserID) {
		return S_OK;
	}
	HRESULT _stdcall StopPresenting(DWORD_PTR dwUserID) {
		return S_OK;
	}
	HRESULT _stdcall PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo) {
		d3d9Device->UpdateTexture(ptex, tex);
		return S_OK;
	}
};

struct sDSTexture {
	IGraphBuilder *pGraph;
	ICaptureGraphBuilder2 *pBuild;
	IBaseFilter *pVmr;
	IVMRFilterConfig9 *pConfig;
	IVMRSurfaceAllocatorNotify9 *pAlloc;
	IMediaControl *pControl;
	CAllocator *pMyAlloc;
	IMediaSeeking *pSeek;
};

struct sDSSound {
	DWORD id;
	IGraphBuilder *pGraph;
	IMediaControl *pControl;
	IMediaSeeking *pSeek;
	IMediaEventEx *pEvent;
};

static IDirect3DTexture9* tex;
static sDSTexture info;

void ResumeMovie(sDSTexture* movie) {
	movie->pControl->Run();
}

void PauseMovie(sDSTexture* movie) {
	movie->pControl->Pause();
}

void StopMovie(sDSTexture* movie) {
	movie->pControl->Stop();
}

void RewindMovie(sDSTexture* movie) {
	LONGLONG time = 0;
	movie->pSeek->SetPositions(&time, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
}

void SeekMovie(sDSTexture* movie, DWORD shortTime) {
	LONGLONG time = shortTime * 10000;
	movie->pSeek->SetPositions(&time, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
}

DWORD FreeMovie(sDSTexture* info) {
	if (info->pControl) info->pControl->Release();
	if (info->pSeek) info->pSeek->Release();
	if (info->pAlloc) info->pAlloc->Release();
	if (info->pMyAlloc) info->pMyAlloc->Release();
	if (info->pConfig) info->pConfig->Release();
	if (info->pVmr) info->pVmr->Release();
	if (info->pGraph) info->pGraph->Release();
	if (info->pBuild) info->pBuild->Release();
	return 0;
}

DWORD CreateDSGraph(wchar_t* path, IDirect3DTexture9** tex, sDSTexture* result) {
	memset(result, 0, sizeof(sDSTexture));

	// Create the Capture Graph Builder.
	HRESULT hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, 0, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&result->pBuild);
	if (hr != S_OK) return FreeMovie(result);

	// Create the Filter Graph Manager.
	hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&result->pGraph);
	if (hr != S_OK) return FreeMovie(result);

	// Initialize the Capture Graph Builder.
	hr = result->pBuild->SetFiltergraph(result->pGraph);
	if (hr != S_OK) return FreeMovie(result);

	hr = CoCreateInstance(CLSID_VideoMixingRenderer9, 0, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&result->pVmr);
	if (hr != S_OK) return FreeMovie(result);

	hr = result->pGraph->AddFilter(result->pVmr, L"VMR9");
	if (hr != S_OK) return FreeMovie(result);

	result->pVmr->QueryInterface(IID_IVMRFilterConfig9, (void**)&result->pConfig);
	result->pConfig->SetRenderingMode(VMR9Mode_Renderless);
	result->pVmr->QueryInterface(IID_IVMRSurfaceAllocatorNotify9, (void**)&result->pAlloc);

	result->pMyAlloc = new CAllocator();

	result->pMyAlloc->AdviseNotify(result->pAlloc);
	result->pAlloc->AdviseSurfaceAllocator(0, (IVMRSurfaceAllocator9*)result->pMyAlloc);

	hr = result->pGraph->QueryInterface(IID_IMediaControl, (void**)&result->pControl);
	hr |= result->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&result->pSeek);
	if (hr != S_OK) return FreeMovie(result);

	result->pGraph->RenderFile(path, NULL);
	result->pControl->Run();

	*tex = result->pMyAlloc->tex;
	return 1;
}

static DWORD PlayFrameHook3() {
	__asm {
		xor eax, eax;
		call FuncOffs::GNW95_process_message_; //windows message pump
	}
	PlayMovieFrame();
	if (GetAsyncKeyState(VK_ESCAPE)) return 0;
	_int64 pos, end;
	info.pSeek->GetCurrentPosition(&pos);
	info.pSeek->GetStopPosition(&end);
	if (end == pos) return 0;
	else return 1;
}

static void __declspec(naked) PlayFrameHook1() {
	__asm {
		push ecx;
		call PlayFrameHook3;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) PlayFrameHook2() {
	__asm {
		xor eax,eax;
		dec eax;
		retn;
	}
}

static DWORD _stdcall PlayMovieHook2(DWORD id) {
	//Get file path in unicode
	wchar_t path[MAX_PATH];
	char* master_patches = VarPtr::patches;
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

	//Check for existance of file
	HANDLE h = CreateFileW(path, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if (h == INVALID_HANDLE_VALUE) return 0;
	CloseHandle(h);

	//Create texture and dshow info
	if (!CreateDSGraph(path, &tex, &info)) return 0;

	GetAsyncKeyState(VK_ESCAPE);
	SetMovieTexture(tex);
	HookCall(0x44E937, PlayFrameHook1);
	HookCall(0x44E949, PlayFrameHook2);

	return 1;
}

static void _stdcall PlayMovieHook3() {
	SafeWrite32(0x44E938, 0x3934c);
	SafeWrite32(0x44E94A, 0x7a22a);
	FreeMovie(&info);
}

static const DWORD PlayMovieAddr = 0x44E690;
static void __declspec(naked) PlayMovieHook() {
	__asm {
		cmp eax, MaxMovies;
		jge fail;
		push ecx;
		push ebx;
		push edx;
		push eax;
		push eax;
		call PlayMovieHook2;
		test eax,eax;
		jnz playavi;
		pop eax;
		pop edx;
		pop ebx;
		pop ecx;
		call PlayMovieAddr;
		retn;
playavi:
		pop eax;
		pop edx;
		call PlayMovieAddr;
		call PlayMovieHook3;
		pop ebx;
		pop ecx;
		xor eax, eax;
		retn;
fail:
		jmp PlayMovieAddr;
	}
}

static std::vector<sDSSound*> playingSounds;
static std::vector<sDSSound*> loopingSounds;
DWORD playID = 0;
DWORD loopID = 0;
static HWND soundwindow=0;

static void FreeSound(sDSSound* sound) {
	sound->pEvent->SetNotifyWindow(0, WM_APP, 0);
	SAFERELEASE(sound->pEvent);
	SAFERELEASE(sound->pSeek);
	SAFERELEASE(sound->pControl);
	SAFERELEASE(sound->pGraph);
	delete sound;
}

void WipeSounds() {
	for(DWORD d=0;d<playingSounds.size();d++) FreeSound(playingSounds[d]);
	for(DWORD d=0;d<loopingSounds.size();d++) FreeSound(loopingSounds[d]);
	playingSounds.clear();
	loopingSounds.clear();
}

LRESULT CALLBACK SoundWndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l) {
	if (msg == WM_APP) {
		DWORD id = l;
		sDSSound* dssound = 0;
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

static sDSSound* LoadSound(wchar_t* path, bool loop) {
	if (!soundwindow) {
		WNDCLASSEX wcx;

		memset(&wcx, 0, sizeof(wcx));
		wcx.cbSize = sizeof(wcx);
		wcx.lpfnWndProc = SoundWndProc;
		wcx.hInstance = GetModuleHandleA(0);
		wcx.lpszClassName = "SfallSndWnd";

		RegisterClassEx(&wcx);
		soundwindow = CreateWindow("SfallSndWnd", "SndWnd", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, GetModuleHandleA(0), 0);
	}
	sDSSound* result = new sDSSound();

	DWORD id;

	if (loop)id = loopID++;
	else id = playID++;
	if (loop) id |= 0x80000000;
	result->id = id;

	CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&result->pGraph);
	result->pGraph->QueryInterface(IID_IMediaControl, (void**)&result->pControl);
	if (loop) result->pGraph->QueryInterface(IID_IMediaSeeking, (void**)&result->pSeek);
	else result->pSeek = 0;
	result->pGraph->QueryInterface(IID_IMediaEventEx, (void**)&result->pEvent);
	result->pEvent->SetNotifyWindow((OAHWND)soundwindow, WM_APP, id);
	result->pControl->RenderFile(path);
	result->pControl->Run();

	if (loop) loopingSounds.push_back(result);
	else playingSounds.push_back(result);

	return result;
}

static const wchar_t *SoundExtensions[] = { L"mp3", L"wma", L"wav" };
static void _stdcall SoundLoadHook2(const void*, const char* path) {
	if (!path || strlen(path) < 4) return;
	wchar_t buf[256];
	mbstowcs_s(0, buf, path, 256);

	//CleanupSounds();

	bool found = false;
	for (int i = 0; i < 3; i++) {
		buf[wcslen(buf) - 3] = 0;
		wcscat_s(buf, SoundExtensions[i]);

		HANDLE h = CreateFileW(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		if (h == INVALID_HANDLE_VALUE) continue;
		CloseHandle(h);
		found = true;
		break;
	}
	if (!found) return;

	LoadSound(buf, true);
}

void* _stdcall PlaySfallSound(const char* path, int loop) {
	wchar_t buf[256];
	mbstowcs_s(0, buf, path, 256);
	if (loop) return LoadSound(buf, true);
	LoadSound(buf, false);
	return 0;
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

static const DWORD SoundLoadHookRet=0x4AD49E;
static void __declspec(naked) SoundLoadHook() {
	__asm {
		push eax;
		push ecx;
		push edx;
		push edx;
		push eax;
		call SoundLoadHook2;
		pop edx;
		pop ecx;
		pop eax;
		push ebx;
		push esi;
		push edi;
		push ebp;
		mov ebx, eax;
		jmp SoundLoadHookRet;
	}
}

static const DWORD Artimer1DaysCheckJmp = 0x4A3790;
static const DWORD Artimer1DaysCheckJmpLess = 0x4A37A9;
static DWORD Artimer1DaysCheckTimer;
static void __declspec(naked) Artimer1DaysCheckHook() {
	__asm {
		cmp edx, Artimer1DaysCheckTimer;
		jl less;
		jmp Artimer1DaysCheckJmp;
less:
		jmp Artimer1DaysCheckJmpLess;
	}
}

void Movies::init() {
	dlog("Applying movie patch.", DL_INIT);
	if (*((DWORD*)0x00518DA0) != 0x00503300) {
		dlog("Error!", DL_INIT);
	}
	for (int i = 0; i < MaxMovies; i++) {
		MoviePtrs[i] = (DWORD)&MoviePaths[65 * i];
		MoviePaths[i * 65 + 64] = 0;
		char ininame[8];
		strcpy_s(ininame, "Movie");
		_itoa_s(i + 1, &ininame[5], 3, 10);
		if (i < 17) {
			GetPrivateProfileString("Misc", ininame, (char*)(0x518DA0 + i * 4), &MoviePaths[i * 65], 65, ini);
		} else {
			GetPrivateProfileString("Misc", ininame, "", &MoviePaths[i * 65], 65, ini);
		}
	}
	dlog(".", DL_INIT);
	SafeWrite32(0x44E6AE, (DWORD)MoviePtrs);
	SafeWrite32(0x44E721, (DWORD)MoviePtrs);
	SafeWrite32(0x44E75E, (DWORD)MoviePtrs);
	SafeWrite32(0x44E78A, (DWORD)MoviePtrs);
	dlog(".", DL_INIT);
	if (GraphicsMode != 0) {
		HookCall(0x440C4F, PlayMovieHook);
		HookCall(0x45A1D0, PlayMovieHook);
		HookCall(0x4809CB, PlayMovieHook);
		HookCall(0x4809D4, PlayMovieHook);
		HookCall(0x4809E0, PlayMovieHook);
		HookCall(0x480A66, PlayMovieHook);
		HookCall(0x480A72, PlayMovieHook);
		HookCall(0x480A9A, PlayMovieHook);
		HookCall(0x4810CA, PlayMovieHook);
		HookCall(0x4993B3, PlayMovieHook);
		HookCall(0x4A37CD, PlayMovieHook);
	}
	dlog(".", DL_INIT);
	if (GetPrivateProfileInt("Sound", "AllowDShowSound", 0, ini)) {
		MakeCall(0x4AD498, &SoundLoadHook, true);
	}
	dlogr(" Done", DL_INIT);

	DWORD tmp;
	tmp = SimplePatch<DWORD>(0x4A36EC, "Misc", "MovieTimer_artimer4", 360, 0);
	tmp = SimplePatch<DWORD>(0x4A3747, "Misc", "MovieTimer_artimer3", 270, 0, tmp);
	tmp = SimplePatch<DWORD>(0x4A376A, "Misc", "MovieTimer_artimer2", 180, 0, tmp);
	Artimer1DaysCheckTimer = GetPrivateProfileIntA("Misc", "MovieTimer_artimer1", 90, ini);
	if (Artimer1DaysCheckTimer != 90) {
		Artimer1DaysCheckTimer = max(0, min(tmp, Artimer1DaysCheckTimer));
		char s[255];
		sprintf_s(s, "Applying patch: MovieTimer_artimer1 = %d. ", Artimer1DaysCheckTimer);
		dlog(s, DL_INIT);
		MakeCall(0x4A378B, &Artimer1DaysCheckHook, true);
		dlogr("Done", DL_INIT);
	}
}
