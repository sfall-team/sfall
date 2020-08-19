/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
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

#define DIRECTINPUT_VERSION         0x0700

#include <math.h>
#include <dinput.h>
#include <queue>

#include "main.h"
#include "Logging.h"
#include "FalloutEngine.h"
#include "DebugEditor.h"
#include "Graphics.h"
#include "HookScripts.h"
#include "Inventory.h"
#include "ScriptExtender.h"

#include "InputFuncs.h"

typedef HRESULT (__stdcall *DInputCreateProc)(HINSTANCE a,DWORD b,IDirectInputA** c,IUnknown* d);

bool useScrollWheel = true;
static DWORD wheelMod;

static bool reverseMouse;

bool middleMouseDown;
static DWORD middleMouseKey;

static bool backgroundKeyboard;
static bool backgroundMouse;

static bool adjustMouseSpeed;
static double mouseSpeedMod;
static double mousePartX;
static double mousePartY;

#define MAX_KEYS (264)
static DWORD keysDown[MAX_KEYS] = {0};

static int mouseX;
static int mouseY;

static DWORD forcingGraphicsRefresh = 0;

void __stdcall ForceGraphicsRefresh(DWORD d) {
	forcingGraphicsRefresh = (d == 0) ? 0 : 1;
}

void GetMouse(int* x, int* y) {
	*x = mouseX; *y = mouseY; mouseX = 0; mouseY = 0;
}

static BYTE LMouse = 0;
static BYTE RMouse = 0;
static int MPMouseX;
static int MPMouseY;

#define kDeviceType_KEYBOARD 0
#define kDeviceType_MOUSE 1

static std::queue<DIDEVICEOBJECTDATA> bufferedPresses;

static HKL keyboardLayout;

void SetMDown(bool down, bool right) {
	if (right) RMouse = down ? 0x80 : 0;
	else LMouse = down ? 0x80 : 0;
}

void SetMPos(int x, int y) {
	MPMouseX = x;
	MPMouseY = y;
}

void FlushInputBuffer() {
	while (!bufferedPresses.empty()) bufferedPresses.pop();
	__asm call kb_clear_;
}

DWORD __stdcall KeyDown(DWORD key) {
	if ((key & 0x80000000) > 0) { // special flag to check by VK code directly
		return GetAsyncKeyState(key & 0xFFFF) & 0x8000;
	}
	key = key & 0xFFFF;
	// combined use of DINPUT states + confirmation from GetAsyncKeyState()
	if (key < MAX_KEYS) {
		if (keysDown[key]) { // confirm pressed state
			DWORD keyVK = MapVirtualKeyEx(key, MAPVK_VSC_TO_VK, keyboardLayout);
			if (keyVK) keysDown[key] = (GetAsyncKeyState(keyVK) & 0x8000);
		}
		return (keysDown[key] > 0);
	}
	return 0;
}

void __stdcall TapKey(DWORD key) {
	DIDEVICEOBJECTDATA data;
	data.dwTimeStamp = 0;
	data.dwSequence = 0;
	data.dwOfs = key;
	data.dwData = 0x80; // flag pressed
	bufferedPresses.push(data);
	data.dwData = 0x00; // flag released
	bufferedPresses.push(data);
}

class FakeDirectInputDevice : public IDirectInputDeviceA {
private:
	IDirectInputDeviceA* RealDevice;
	DWORD DeviceType;
	ULONG Refs;
	bool formatLock;
	DIDATAFORMAT oldFormat;

public:
	/*** Constructor and misc functions ***/
	FakeDirectInputDevice(IDirectInputDevice* device,DWORD type) {
		RealDevice = device;
		DeviceType = type;
		Refs = 1;
		formatLock = false;
		oldFormat.dwDataSize = 0;
	}

	/*** IUnknown methods ***/
	HRESULT __stdcall QueryInterface(REFIID riid, LPVOID * ppvObj) {
		return RealDevice->QueryInterface(riid, ppvObj);
	}

	ULONG __stdcall AddRef(void) {
		return ++Refs;
	}

	ULONG __stdcall Release(void) {
		if (--Refs == 0) {
			RealDevice->Release();
			delete this;
			return 0;
		} else {
			return Refs;
		}
	}

	/*** IDirectInputDevice8A methods ***/
	HRESULT __stdcall GetCapabilities(LPDIDEVCAPS a) {
		return RealDevice->GetCapabilities(a);
	}

	HRESULT __stdcall EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA a, LPVOID b, DWORD c) {
		return RealDevice->EnumObjects(a, b, c);
	}

	HRESULT __stdcall GetProperty(REFGUID a, DIPROPHEADER* b) {
		return RealDevice->GetProperty(a, b);
	}

	HRESULT __stdcall SetProperty(REFGUID a, const DIPROPHEADER* b) {
		return RealDevice->SetProperty(a, b);
	}

	HRESULT __stdcall Acquire(void) {
		return RealDevice->Acquire();
	}

	HRESULT __stdcall Unacquire(void) {
		return RealDevice->Unacquire();
	}

	// Only called for the mouse
	HRESULT __stdcall GetDeviceState(DWORD a, LPVOID b) {
		if (forcingGraphicsRefresh) RefreshGraphics();
		if (DeviceType != kDeviceType_MOUSE) {
			return RealDevice->GetDeviceState(a, b);
		}

		DIMOUSESTATE2 MouseState;
		HRESULT hr;
		int numButtons;
		if (formatLock) hr = RealDevice->GetDeviceState(sizeof(DIMOUSESTATE2), &MouseState);
		else hr = RealDevice->GetDeviceState(sizeof(DIMOUSESTATE), &MouseState);
		if (FAILED(hr)) return hr;
		if (reverseMouse) {
			BYTE tmp = MouseState.rgbButtons[0];
			MouseState.rgbButtons[0] = MouseState.rgbButtons[1];
			MouseState.rgbButtons[1] = tmp;
		}
		if (adjustMouseSpeed) {
			double d = ((double)MouseState.lX) * mouseSpeedMod + mousePartX;
			mousePartX = modf(d, &d);
			MouseState.lX = (LONG)d;
			d = ((double)MouseState.lY) * mouseSpeedMod + mousePartY;
			mousePartY = modf(d, &d);
			MouseState.lY = (LONG)d;
		}
		if (useScrollWheel) {
			int count = 1;
			if (MouseState.lZ > 0) {
				if (*ptr_gmouse_current_cursor == 2 || *ptr_gmouse_current_cursor == 3) {
					__asm {
						call display_scroll_up_;
					}
				} else {
					if (wheelMod) count = MouseState.lZ / wheelMod;
					if (count < 1) count = 1;
					while (count--) TapKey(DIK_UP);
				}
			} else if (MouseState.lZ < 0) {
				if (*ptr_gmouse_current_cursor == 2 || *ptr_gmouse_current_cursor == 3) {
					__asm {
						call display_scroll_down_;
					}
				} else {
					if (wheelMod) count = (-MouseState.lZ) / wheelMod;
					if (count < 1) count = 1;
					while (count--) TapKey(DIK_DOWN);
				}
			}
		}
		if (middleMouseKey && MouseState.rgbButtons[2]) {
			if (!middleMouseDown) {
				TapKey(middleMouseKey);
				middleMouseDown = true;
			}
		} else middleMouseDown = false;
		mouseX = MouseState.lX;
		mouseY = MouseState.lY;

		numButtons = formatLock ? 8 : 4;
		for (int i = 0; i < numButtons; i++) {
			if ((MouseState.rgbButtons[i] & 0x80) != (keysDown[256 + i] & 0x80)) { // state changed
				MouseClickHook(i, (MouseState.rgbButtons[i] & 0x80) > 0);
			}
			keysDown[256 + i] = MouseState.rgbButtons[i];
		}
		memcpy(b, &MouseState, sizeof(DIMOUSESTATE));
		return 0;
	}

	// Only called for the keyboard (dxinput_read_keyboard_buffer_ called at 0x4E06AB)
	HRESULT __stdcall GetDeviceData(DWORD a, DIDEVICEOBJECTDATA* buf, DWORD* count, DWORD d) { // buf - DirectInputKeyboardBuffer (0x6B2560)
		if (DeviceType != kDeviceType_KEYBOARD) {
			return RealDevice->GetDeviceData(a, buf, count, d);
		}

		RunGlobalScripts2();

		if (!buf || bufferedPresses.empty() || (d & DIGDD_PEEK)) {
			HRESULT hr = RealDevice->GetDeviceData(a, buf, count, d);
			if (FAILED(hr) || !buf || !(*count)) return hr;
			for (DWORD i = 0; i < *count; i++) {
				DWORD dxKey = buf[i].dwOfs;
				DWORD state = buf[i].dwData & 0x80;
				DWORD oldState = keysDown[dxKey];
				keysDown[dxKey] = state;
				KeyPressHook(&dxKey, (state > 0), MapVirtualKeyEx(dxKey, MAPVK_VSC_TO_VK, keyboardLayout));
				if (dxKey > 0 && dxKey != buf[i].dwOfs) {
					keysDown[buf[i].dwOfs] = oldState;
					buf[i].dwOfs = dxKey; // Override key
					keysDown[buf[i].dwOfs] = state;
				}
				// OnKeyPressed
				InventoryKeyPressedHook(buf[i].dwOfs, (state > 0));
				DebugEditorKeyPressedHook(buf[i].dwOfs, (state > 0));
			}
			return hr;
		}
		//Despite passing an array of 32 data objects, fallout cant seem to cope with a key being pressed and released in the same frame...
		//TODO: Fallouts behaviour when passing multiple keypresses makes it appear like it's expecting the DIDEVICEOBJECTDATA struct to be
		//      something other than 16 bytes. afaik, fallout uses DX3 for input, which is before the appData field was added, but it could
		//      be worth checking anyway.
		*buf = bufferedPresses.front();
		bufferedPresses.pop();
		*count = 1;
		return DI_OK;
	}

	HRESULT __stdcall SetDataFormat(const DIDATAFORMAT* a) {
		if (formatLock && oldFormat.dwSize) return RealDevice->SetDataFormat(&oldFormat);
		memcpy(&oldFormat, a, sizeof(DIDATAFORMAT));
		return RealDevice->SetDataFormat(a);
	}

	HRESULT __stdcall SetEventNotification(HANDLE a) {
		return RealDevice->SetEventNotification(a);
	}

	HRESULT __stdcall SetCooperativeLevel(HWND a, DWORD b) {
		if (DeviceType == kDeviceType_KEYBOARD && backgroundKeyboard) b = DISCL_BACKGROUND | DISCL_NONEXCLUSIVE;
		if (DeviceType == kDeviceType_MOUSE && backgroundMouse) b = DISCL_BACKGROUND | DISCL_NONEXCLUSIVE;
		return RealDevice->SetCooperativeLevel(a, b);
	}

	HRESULT __stdcall GetObjectInfo(LPDIDEVICEOBJECTINSTANCEA a, DWORD b, DWORD c) {
		return RealDevice->GetObjectInfo(a, b, c);
	}

	HRESULT __stdcall GetDeviceInfo(LPDIDEVICEINSTANCEA a) {
		return RealDevice->GetDeviceInfo(a);
	}

	HRESULT __stdcall RunControlPanel(HWND a, DWORD b) {
		return RealDevice->RunControlPanel(a, b);
	}

	HRESULT __stdcall Initialize(HINSTANCE a, DWORD b, REFGUID c) {
		return RealDevice->Initialize(a, b, c);
	}

	void LockDataFormat(bool lock) {
		formatLock = lock;
	}
};

class FakeDirectInput : public IDirectInputA {
private:
	IDirectInputA* RealInput;
	ULONG Refs;
public:
	/*** Constructor ***/
	FakeDirectInput(IDirectInput* Real) {
		RealInput = Real; Refs = 1;
	}

	/*** IUnknown methods ***/
	HRESULT __stdcall QueryInterface(REFIID riid, LPVOID* ppvObj) {
		return RealInput->QueryInterface(riid, ppvObj);
	}

	ULONG __stdcall AddRef(void) {
		return ++Refs;
	}

	ULONG __stdcall Release(void) {
		if (--Refs == 0) {
			RealInput->Release();
			delete this;
			return 0;
		} else {
			return Refs;
		}
	}

	/*** IDirectInput8A methods ***/
	HRESULT __stdcall CreateDevice(REFGUID r, IDirectInputDeviceA** device, IUnknown* unused) {
		GUID GUID_SysMouse    = { 0x6F1D2B60, 0xD5A0, 0x11CF, { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };
		GUID GUID_SysKeyboard = { 0x6F1D2B61, 0xD5A0, 0x11CF, { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00} };

		if (r != GUID_SysKeyboard && r != GUID_SysMouse) {
			return RealInput->CreateDevice(r, device, unused);
		} else {
			DWORD d;
			IDirectInputDeviceA* RealDevice;
			HRESULT hr;

			if (r == GUID_SysKeyboard) d = kDeviceType_KEYBOARD;
			else d = kDeviceType_MOUSE;
			hr = RealInput->CreateDevice(r, &RealDevice, unused);
			if (hr != DI_OK) return hr;

			FakeDirectInputDevice* fd = new FakeDirectInputDevice(RealDevice, d);
			*device = fd;
			if (d == kDeviceType_MOUSE) {
				if (FAILED(fd->SetDataFormat(&c_dfDIMouse2))) {
					dlogr("ERROR: Could not set mouse format to DIMOUSESTATE2. Only 4 buttons will be available!", DL_MAIN);
				} else fd->LockDataFormat(true);
			}
			return DI_OK;
		}
	}

	HRESULT __stdcall EnumDevices(DWORD a, LPDIENUMDEVICESCALLBACKA b, void* c, DWORD d) {
		return RealInput->EnumDevices(a, b, c, d);
	}

	HRESULT __stdcall GetDeviceStatus(REFGUID r) {
		return RealInput->GetDeviceStatus(r);
	}

	HRESULT __stdcall RunControlPanel(HWND a, DWORD b) {
		return RealInput->RunControlPanel(a, b);
	}

	HRESULT __stdcall Initialize(HINSTANCE a, DWORD b) {
		return RealInput->Initialize(a, b);
	}
};

HRESULT __stdcall FakeDirectInputCreate(HINSTANCE a, DWORD b, IDirectInputA** c, IUnknown* d) {
	HMODULE dinput = LoadLibraryA("dinput.dll");
	if (!dinput || dinput == INVALID_HANDLE_VALUE) return -1;
	DInputCreateProc proc = (DInputCreateProc)GetProcAddress(dinput, "DirectInputCreateA");
	if (!proc) return -1;

	HRESULT hr = proc(a, b, c, d);
	if (FAILED(hr)) return hr;

	*c = (IDirectInputA*)new FakeDirectInput(*c);

	reverseMouse = GetConfigInt("Input", "ReverseMouseButtons", 0) != 0;

	useScrollWheel = GetConfigInt("Input", "UseScrollWheel", 1) != 0;
	wheelMod = GetConfigInt("Input", "ScrollMod", 0);
	LONG MouseSpeed = GetConfigInt("Input", "MouseSensitivity", 100);
	if (MouseSpeed != 100) {
		adjustMouseSpeed = true;
		mouseSpeedMod = ((double)MouseSpeed) / 100.0;
		mousePartX = 0;
		mousePartY = 0;
	} else adjustMouseSpeed = false;

	middleMouseKey = GetConfigInt("Input", "MiddleMouse", 0x30);
	middleMouseDown = false;

	backgroundKeyboard = GetConfigInt("Input", "BackgroundKeyboard", 0) != 0;
	backgroundMouse = GetConfigInt("Input", "BackgroundMouse", 0) != 0;

	keyboardLayout = GetKeyboardLayout(0);

	return hr;
}
