#pragma once

namespace sfall 
{

template <typename T>
T SimplePatch(DWORD addr, const char* iniSection, const char* iniKey, T defaultValue, T minValue = 0, T maxValue = INT_MAX)
{
	return SimplePatch<T>(&addr, 1, iniSection, iniKey, defaultValue, minValue, maxValue);
}

template <typename T>
T SimplePatch(DWORD *addrs, int numAddrs, const char* iniSection, const char* iniKey, T defaultValue, T minValue = 0, T maxValue = INT_MAX)
{
	T value;
	char msg[255];
	value = (T)IniReader::GetConfigInt(iniSection, iniKey, defaultValue);
	if (value != defaultValue) {
		if (value < minValue) {
			value = minValue;
		} else if (value > maxValue) {
			value = maxValue;
		}
		_snprintf_s(msg, sizeof(msg), _TRUNCATE, "Applying patch: %s = %d.", iniKey, value);
		dlogr((const char*)msg, DL_INIT);
		for (int i = 0; i < numAddrs; i++) {
			SafeWrite<T>(addrs[i], (T)value);
		}
	}
	return value;
}

}
