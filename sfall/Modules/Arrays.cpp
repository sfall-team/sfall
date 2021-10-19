#include <unordered_set>
#include <algorithm>

#include "..\FalloutEngine\Fallout2.h"
#include "ScriptExtender.h"

#include "Arrays.h"

/*
	GLOBAL variable for arrays
*/
DWORD arraysBehavior = 1; // 0 - backward compatible with pre-3.4, 1 - permanent arrays don't get stored in savegames by default

// arrays map: arrayId => arrayVar
ArraysMap arrays;
// temp arrays: set of arrayId
std::tr1::unordered_set<DWORD> temporaryArrays;
// saved arrays: arrayKey => arrayId
ArrayKeysMap savedArrays;

// auto-incremented ID
DWORD nextArrayID = 1;
// special array ID for array expressions, contains the ID number of the currently created array
DWORD stackArrayId;

static char get_all_arrays_special_key[] = "...all_arrays...";

sArrayElement::sArrayElement() : len(0), intVal(0), type(DATATYPE_NONE) { }

sArrayElement::sArrayElement( DWORD _val, DWORD _dataType ) : len(0), type(_dataType), intVal(_val)
{
	if (DATATYPE_STR == _dataType) len = strlen((char*)_val);
}

sArrayElement::sArrayElement(const long& other)
{
	set(other);
}

void sArrayElement::clearData()
{
	if (type == DATATYPE_STR) delete[] strVal;
}

void sArrayElement::setByType( DWORD val, DWORD dataType )
{
	switch(dataType) {
	case DATATYPE_STR:
		set((char*)val);
		break;
	case DATATYPE_INT:
		set(*(long*)&val);
		break;
	case DATATYPE_FLOAT:
		set(*(float*)&val);
		break;
	}
}

void sArrayElement::set( long val )
{
	clearData();
	type = DATATYPE_INT;
	intVal = val;
}

void sArrayElement::set( float val )
{
	clearData();
	type = DATATYPE_FLOAT;
	floatVal = val;
}

void sArrayElement::set( const char* val, int sLen /*= -1*/ )
{
	clearData();
	type = DATATYPE_STR;
	if (sLen == -1) sLen = strlen(val);
	if (sLen >= ARRAY_MAX_STRING) sLen = ARRAY_MAX_STRING - 1; // memory safety
	len = sLen + 1;
	strVal = new char[len];
	memcpy(strVal, val, sLen);
	strVal[sLen] = '\0';
}

void sArrayElement::unset()
{
	clearData();
	type = DATATYPE_NONE;
	len = intVal = 0;
}

DWORD sArrayElement::getHashStatic(DWORD value, DWORD type) {
	switch (type) {
	case DATATYPE_STR: {
		const char* str = (const char*)value;
		DWORD hash = 0;
		while (*str) {
			hash = ((hash << 5) + hash) + *str++;
		}
		return hash;
	}
	case DATATYPE_INT:
	case DATATYPE_FLOAT:
		return value;
	default:
		return 0;
	}
}

bool sArrayElement::operator<( const sArrayElement &el ) const
{
	if (type < el.type) return true;
	if (type == el.type) {
		switch (type) {
		case DATATYPE_STR:
			return strcmp(strVal, el.strVal) < 0;
		case DATATYPE_FLOAT:
			return floatVal < el.floatVal;
		case DATATYPE_INT:
		default:
			return intVal < el.intVal;
		}
	}
	return false;
}

bool sArrayElement_EqualFunc::operator()( const sArrayElement &elA, const sArrayElement &elB ) const
{
	if (elA.type != elB.type) return false;

	switch (elA.type) {
	case DATATYPE_STR:
		return strcmp(elA.strVal, elB.strVal) == 0;
	case DATATYPE_INT:
	case DATATYPE_FLOAT:
		return elA.intVal == elB.intVal;
	default:
		return true;
	}
}

void sArrayVar::clearRange( int from, int to /*= -1*/ )
{
	if (to == -1) to = val.size();
	std::vector<sArrayElement>::iterator it, itTo = val.begin() + to;
	for (it = val.begin() + from; it < itTo; ++it) {
		it->clearData();
	}
}

void sArrayVar::clearAll()
{
	for (std::vector<sArrayElement>::iterator it = val.begin(); it != val.end(); ++it) {
		it->clearData();
	}
}

void SaveArrayElement(sArrayElement* el, HANDLE h)
{
	DWORD unused;
	WriteFile(h, &el->type, 4, &unused, 0);
	if (el->type == DATATYPE_STR) {
		WriteFile(h, &el->len, 4, &unused, 0);
		WriteFile(h, el->strVal, el->len, &unused, 0);
	} else {
		WriteFile(h, &el->intVal, 4, &unused, 0);
	}
}

bool LoadArrayElement(sArrayElement* el, HANDLE h)
{
	DWORD unused;
	ReadFile(h, &el->type, 4, &unused, 0);
	if (el->type == DATATYPE_STR) {
		ReadFile(h, &el->len, 4, &unused, 0);
		if (el->len > 0) {
			el->strVal = new char[el->len];
			ReadFile(h, el->strVal, el->len, &unused, 0);
		} else
			el->strVal = nullptr;
	} else {
		ReadFile(h, &el->intVal, 4, &unused, 0);
		el->len = 0;
	}
	return (el->len) ? (unused != el->len) : (unused != 4);
}

static long LoadArraysOld(HANDLE h) {
	DWORD count, unused, id;
	ReadFile(h, &count, 4, &unused, 0); // count of saved arrays
	if (unused != 4) return -1;
	if (!count) return 0;

	dlogr("Loading arrays (old fmt)", DL_MAIN);

	sArrayVarOld var;
	sArrayVar varN;

	for (DWORD i = 0; i < count; i++) {
		ReadFile(h, &id, 4, &unused, 0);
		ReadFile(h, &var, 8, &unused, 0);
		if (unused != 8) return -1;

		var.types = new DWORD[var.len];
		var.data = new char[var.len * var.datalen];

		ReadFile(h, &var.types, (4 * var.len), &unused, 0);
		ReadFile(h, &var.data, (var.len * var.datalen), &unused, 0);

		varN.flags = 0;
		varN.val.resize(var.len);
		for (size_t j = 0; j < var.len; j++) {
			switch (var.types[j]) {
			case DATATYPE_INT:
				varN.val[j].set(*(long*)(&var.data[var.datalen * j]));
				break;
			case DATATYPE_FLOAT:
				varN.val[j].set(*(float*)(&var.data[var.datalen * j]));
				break;
			case DATATYPE_STR:
				varN.val[j].set(&var.data[var.datalen * j], var.datalen - 1);
				break;
			}
		}
		delete[] var.types;
		delete[] var.data;

		varN.key = sArrayElement(id, DATATYPE_INT);
		arrays.insert(array_pair(id, varN));
		savedArrays[varN.key] = id;
	}
	return 1;
}

long LoadArrays(HANDLE h) {
	nextArrayID = 1;

	long result = LoadArraysOld(h);
	if (result) return result;

	DWORD count, unused, elCount;
	ReadFile(h, &count, 4, &unused, 0); // count of saved arrays
	if (unused != 4 && !result) return 1;

	dlogr("Loading arrays (new fmt)", DL_MAIN);
	if (unused != 4) return -1;

	sArrayVar arrayVar;
	for (DWORD i = 0; i < count; i++) {
		if (LoadArrayElement(&arrayVar.key, h)) return -1;
		if (arrayVar.key.intVal == 0 || static_cast<long>(arrayVar.key.type) >= 4) { // partial compatibility with 3.4
			arrayVar.key.intVal = static_cast<long>(arrayVar.key.type);
			arrayVar.key.type = DATATYPE_INT;
		}
		ReadFile(h, &arrayVar.flags, 4, &unused, 0);
		ReadFile(h, &elCount, 4, &unused, 0); // actual number of elements: keys+values
		if (unused != 4) return -1;

		bool isAssoc = arrayVar.isAssoc();
		arrayVar.val.resize(elCount);
		for (size_t j = 0; j < elCount; j++) { // normal and associative arrays stored and loaded equally
			if (LoadArrayElement(&arrayVar.val[j], h)) return -1;
			if (isAssoc && (j % 2) == 0) { // only difference is that keyHash is filled with appropriate indexes
				arrayVar.keyHash[arrayVar.val[j]] = j;
			}
		}
		while (ArrayExists(nextArrayID)) nextArrayID++;
		if (nextArrayID == 0) nextArrayID++;

		arrays.insert(array_pair(nextArrayID, arrayVar));
		savedArrays[arrayVar.key] = nextArrayID++;

		arrayVar.keyHash.clear();
	}
	return 0;
}

void SaveArrays(HANDLE h) {
	DWORD elCount, unused, count = 0;
	WriteFile(h, &count, 4, &unused, 0); // this is for backward compatibility with 3.3-

	array_itr arrayIt;
	ArrayKeysMap::iterator it = savedArrays.begin();
	while (it != savedArrays.end()) {
		arrayIt = arrays.find(it->second); // check the existence of an array
		if (arrayIt == arrays.end()) {
			it = savedArrays.erase(it); // array is not found, delete it [fix to C++11 https://en.cppreference.com/w/cpp/container/unordered_map/erase]
		} else {
			++count;
			++it;
		}
	}
	WriteFile(h, &count, 4, &unused, 0); // count saved arrays
	if (count == 0) return;
	for (it = savedArrays.begin(); it != savedArrays.end(); ++it) {
		arrayIt = arrays.find(it->second);
		if (arrayIt != arrays.end()) {
			sArrayVar &var = arrayIt->second; //arrays[it->second];
			SaveArrayElement(&var.key, h); // type, key/length of key and string of key
			WriteFile(h, &var.flags, 4, &unused, 0);
			elCount = var.val.size();
			WriteFile(h, &elCount, 4, &unused, 0); // number of elements in saved array
			for (std::vector<sArrayElement>::iterator vIt = var.val.begin(); vIt != var.val.end(); ++vIt) {
				SaveArrayElement(&(*vIt), h);
			}
		}
	}
}

int GetNumArrays() {
	return arrays.size();
}

bool ArrayExists(DWORD id) {
	return (arrays.find(id) != arrays.end());
}

void GetArrays(int* _arrays) {
	int pos = 0;
	array_citr itr = arrays.begin();
	while (itr != arrays.end()) {
		_arrays[pos++] = itr->first; // array id
		_arrays[pos++] = itr->second.isAssoc() ? 1 : 0;
		_arrays[pos++] = itr->second.val.size();
		_arrays[pos++] = itr->second.flags;
		++itr;
	}
}

void DEGetArray(int id, DWORD* types, char* data) {
	int pos = 0;
	if (types != nullptr) {
		for (size_t i = 0; i < arrays[id].val.size(); i++) {
			const sArrayElement& arVal = arrays[id].val[i];
			types[pos++] = (DWORD)arVal.type;
			types[pos++] = (arVal.type == DATATYPE_STR) ? arVal.len : 4; // in bytes
		}
	} else {
		for (size_t i = 0; i < arrays[id].val.size(); i++) {
			const sArrayElement& arVal = arrays[id].val[i];
			if (arVal.type != DATATYPE_STR) {
				*(long*)(data + pos) = arVal.intVal;
				pos += 4;
			} else {
				strcpy(data + pos, arVal.strVal);
				pos += arVal.len;
			}
		}
	}
}

void DESetArray(int id, const DWORD* types, const char* data) {
	int pos = 0;
	for (size_t i = 0; i < arrays[id].val.size(); i++) {
		sArrayElement& arVal = arrays[id].val[i];
		switch (arVal.type) {
		case DATATYPE_NONE:
			pos += 4;
			break;
		case DATATYPE_INT:
			arVal.intVal = *(long*)(data + pos);
			pos += 4;
			break;
		case DATATYPE_FLOAT:
			arVal.floatVal = *(float*)(data + pos);
			pos += 4;
			break;
		case DATATYPE_STR:
			strcpy(arVal.strVal, data + pos);
			pos += arVal.len;
		}
	}
}

/*
	Array manipulation functions for script operators
	TODO: move somewhere else
*/

const char* __stdcall GetSfallTypeName(DWORD dataType) {
	switch (dataType) {
		case DATATYPE_NONE:
			return "(none)";
		case DATATYPE_STR:
			return "string";
		case DATATYPE_FLOAT:
			return "float";
		case DATATYPE_INT:
			return "integer";
		default:
			return "(unknown)";
	}
}

DWORD __stdcall getSfallTypeByScriptType(DWORD varType) {
	switch (varType & 0xFFFF) {
		case VAR_TYPE_STR2:
		case VAR_TYPE_STR:
			return DATATYPE_STR;
		case VAR_TYPE_FLOAT:
			return DATATYPE_FLOAT;
		case VAR_TYPE_INT:
		default:
			return DATATYPE_INT;
	}
}

DWORD __stdcall getScriptTypeBySfallType(DWORD dataType) {
	switch (dataType) {
		case DATATYPE_STR:
			return VAR_TYPE_STR;
		case DATATYPE_FLOAT:
			return VAR_TYPE_FLOAT;
		case DATATYPE_INT:
		default:
			return VAR_TYPE_INT;
	}
}

DWORD __stdcall CreateArray(int len, DWORD flags) {
	flags = (flags & ~1); // reset 1 bit
	if (len < 0) {
		flags |= ARRAYFLAG_ASSOC;
	} else if (len > ARRAY_MAX_SIZE) {
		len = ARRAY_MAX_SIZE; // safecheck
	}
	sArrayVar var(len, flags);

	if (!var.isAssoc() && len) {
		var.val.resize(len);
	}

	while (ArrayExists(nextArrayID)) nextArrayID++;
	if (nextArrayID == 0) nextArrayID++;

	if (arraysBehavior == 0) {
		var.key = sArrayElement(nextArrayID, DATATYPE_INT);
		savedArrays[var.key] = nextArrayID;
	}
	stackArrayId = nextArrayID;
	arrays[nextArrayID] = var;
	return nextArrayID++;
}

DWORD __stdcall CreateTempArray(DWORD len, DWORD flags) {
	DWORD id = CreateArray(len, flags);
	temporaryArrays.insert(id);
	return id;
}

void __stdcall FreeArray(DWORD id) {
	array_itr it = arrays.find(id);
	if (it != arrays.end()) {
		if (it->second.key.intVal) savedArrays.erase(it->second.key);
		it->second.clearArrayVar();
		arrays.erase(id);
	}
}

void DeleteAllTempArrays() {
	if (!temporaryArrays.empty()) {
		for (std::tr1::unordered_set<DWORD>::iterator it = temporaryArrays.begin(); it != temporaryArrays.end(); ++it) {
			FreeArray(*it);
		}
		temporaryArrays.clear();
	}
}

DWORD __stdcall GetArrayKey(DWORD id, int index, DWORD* resultType) {
	*resultType = VAR_TYPE_INT;
	if (!ArrayExists(id) || index < -1 || index > arrays[id].size()) {
		return 0;
	}
	if (index == -1) { // special index to indicate if array is associative
		return (DWORD)arrays[id].isAssoc();
	}
	if (arrays[id].isAssoc()) {
		index *= 2;
		// for associative array - return key at the specified index
		switch(arrays[id].val[index].type) {
		case DATATYPE_INT:
			return *(DWORD*)&arrays[id].val[index].intVal;
		case DATATYPE_FLOAT:
			*resultType = VAR_TYPE_FLOAT;
			return *(DWORD*)&arrays[id].val[index].intVal;
		case DATATYPE_STR:
			*resultType = VAR_TYPE_STR;
			return (DWORD)arrays[id].val[index].strVal;
		case DATATYPE_NONE:
		default:
			return 0;
		}
	} else {
		// just return index for normal array..
		return index;
	}
}

DWORD __stdcall GetArray(DWORD id, DWORD key, DWORD keyType, DWORD* resultType) {
	*resultType = VAR_TYPE_INT;
	if (!ArrayExists(id)) return 0;

	int el;
	sArrayVar &arr = arrays[id];
	if (arr.isAssoc()) {
		ArrayKeysMap::iterator it = arr.keyHash.find(sArrayElement(key, getSfallTypeByScriptType(keyType)));
		if (it != arr.keyHash.end())
			el = it->second + 1;
		else
			return 0;
	} else {
		el = key;
		// check for invalid index
		if (el < 0 || el >= arr.size()) {
			return 0;
		}
	}
	switch (arr.val[el].type) {
	case DATATYPE_NONE:
		return 0;
	case DATATYPE_INT:
		return *(DWORD*)&arr.val[el].intVal;
	case DATATYPE_FLOAT:
		*resultType = VAR_TYPE_FLOAT;
		return *(DWORD*)&arr.val[el].intVal;
	case DATATYPE_STR:
		*resultType = VAR_TYPE_STR;
		return (DWORD)arr.val[el].strVal;
	}
	return 0;
}

void __stdcall setArray(DWORD id, DWORD key, DWORD keyType, DWORD val, DWORD valType, DWORD allowUnset) {
	keyType = getSfallTypeByScriptType(keyType);
	valType = getSfallTypeByScriptType(valType);
	sArrayVar &arr = arrays[id];
	if (arr.isAssoc()) {
		sArrayElement sEl(key, keyType);
		ArrayKeysMap::iterator elIter = arr.keyHash.find(sEl);
		int el = (elIter != arr.keyHash.end())
		       ? elIter->second
		       : -1;

		bool lookupMap = (arr.flags & ARRAYFLAG_CONSTVAL) != 0;
		if (lookupMap && el != -1) return; // don't update value of key

		if (allowUnset && !lookupMap && (valType == DATATYPE_INT && val == 0)) {
			// after assigning zero to a key, no need to store it, because "get_array" returns 0 for non-existent keys: try unset
			if (el >= 0) {
				// remove from hashtable
				arr.keyHash.erase(elIter);
				// shift all keyHash references
				for (elIter = arr.keyHash.begin(); elIter != arr.keyHash.end(); ++elIter) {
					if (elIter->second >= (DWORD)el)
						elIter->second -= 2;
				}
				// remove key=>value pair from vector
				arr.clearRange(el, el + 2);
				arr.val.erase(arr.val.begin() + el, arr.val.begin() + (el + 2));
			}
		} else {
			if (el == -1) {
				// size check
				if (arr.size() >= ARRAY_MAX_SIZE) return;
				// add pair
				el = arr.val.size();
				arr.val.resize(el + 2);
				arr.val[el].setByType(key, keyType); // copy data
				arr.keyHash[arr.val[el]] = el;
			}
			arr.val[el + 1].setByType(val, valType);
		}
	} else if (keyType == DATATYPE_INT) { // only update normal array if key is an integer and within array size
		size_t index = key;
		if (arr.val.size() > index) {
			arr.val[index].setByType(val, valType);
		}
	}
}

void __stdcall SetArray(DWORD id, DWORD key, DWORD keyType, DWORD val, DWORD valType, DWORD allowUnset) {
	if (ArrayExists(id)) setArray(id, key, keyType, val, valType, allowUnset);
}

int __stdcall LenArray(DWORD id) {
	array_itr it = arrays.find(id);
	return (it != arrays.end()) ? it->second.size() : -1;
}

template <class T>
static void ListSort(std::vector<T> &arr, int type) {
	switch (type) {
	case ARRAY_ACTION_SORT:    // sort ascending
		std::sort(arr.begin(), arr.end());
		break;
	case ARRAY_ACTION_RSORT:   // sort descending
		std::sort(arr.rbegin(), arr.rend());
		break;
	case ARRAY_ACTION_REVERSE: // reverse elements
		std::reverse(arr.rbegin(), arr.rend());
		break;
	case ARRAY_ACTION_SHUFFLE: // shuffle elements
		std::random_shuffle(arr.rbegin(), arr.rend());
		break;
	}
}

static void MapSort(sArrayVar& arr, int type) {
	std::vector<std::pair<sArrayElement, sArrayElement>> vmap;
	vmap.reserve(arr.val.size());

	bool sortByValue = false;
	if (type < ARRAY_ACTION_SHUFFLE) {
		type += 4;
		sortByValue = true;
	}

	sArrayElement key, val;
	for (size_t i = 0; i < arr.val.size(); ++i) {
		if (sortByValue) {
			val = arr.val[i++];    // map key > value
			key = arr.val[i];      // map value > key
		} else {
			key = arr.val[i];      // key
			val = arr.val[++i];    // value
		}
		vmap.push_back(std::make_pair(key, val));
	}
	ListSort(vmap, type);

	arr.val.clear();
	arr.keyHash.clear();
	for (size_t i = 0; i < vmap.size(); ++i) {
		size_t el = arr.val.size();
		if (sortByValue) {
			arr.val.push_back(vmap[i].second); // map value > key
			arr.val.push_back(vmap[i].first);  // map key > value
		} else {
			arr.val.push_back(vmap[i].first);
			arr.val.push_back(vmap[i].second);
		}
		arr.keyHash[arr.val[el]] = el;
	}
}

void __stdcall ResizeArray(DWORD id, int newlen) {
	if (newlen == -1 || !ArrayExists(id)) return;

	sArrayVar &arr = arrays[id];
	int arrSize = arr.size();
	if (arrSize == newlen) return;

	if (arr.isAssoc()) {
		// only allow to reduce number of elements (adding range of elements is meaningless for maps)
		if (newlen >= 0 && newlen < arrSize) {
			ArrayKeysMap::iterator itHash;
			std::vector<sArrayElement>::iterator itVal;
			int actualLen = newlen * 2;
			for (itVal = arr.val.begin() + actualLen; itVal != arr.val.end(); itVal += 2) {
				if ((itHash = arr.keyHash.find(*itVal)) != arr.keyHash.end())
					arr.keyHash.erase(itHash);
			}
			if (actualLen == 0)
				arr.clearAll();
			else
				arr.clearRange(actualLen);
			arr.val.resize(actualLen);
		} else if (newlen < 0) {
			if (newlen < (ARRAY_ACTION_SHUFFLE - 2)) goto errorResize;
			MapSort(arr, newlen);
		}
		return;
	}
	if (newlen >= 0) { // actual resize
		if (newlen == 0) {
			arr.clearAll();
		} else {
			if (newlen > ARRAY_MAX_SIZE) newlen = ARRAY_MAX_SIZE; // safety
			if (newlen < arrSize) arr.clearRange(newlen);
		}
		arr.val.resize(newlen);
	} else { // special functions for lists...
		if (newlen < ARRAY_ACTION_SHUFFLE) goto errorResize;
		ListSort(arr.val, newlen);
	}
	return;

errorResize:
	fo::func::debug_printf("\nOPCODE ERROR: resize_array() - array sorting error.");
}

void __stdcall FixArray(DWORD id) {
	temporaryArrays.erase(id);
}

int __stdcall ScanArray(DWORD id, DWORD val, DWORD datatype, DWORD* resultType) {
	*resultType = VAR_TYPE_INT;
	datatype = getSfallTypeByScriptType(datatype);
	if (!ArrayExists(id)) {
		return -1;
	}
	char step = arrays[id].isAssoc() ? 2 : 1;
	for (size_t i = 0; i < arrays[id].val.size(); i += step) {
		sArrayElement &el = arrays[id].val[i + step - 1];
		if (el.type == datatype) {
			if ((datatype != DATATYPE_STR && *(DWORD*)&(el.intVal) == val) ||
				(datatype == DATATYPE_STR && strcmp(el.strVal, (char*)val) == 0)) {
				if (arrays[id].isAssoc()) { // return key instead of index for associative arrays
					*resultType = getScriptTypeBySfallType(arrays[id].val[i].type);
					return *(DWORD*)&arrays[id].val[i].intVal;
				} else {
					return i;
				}
			}
		}
	}
	return -1;
}

DWORD __stdcall LoadArray(DWORD key, DWORD keyType) {
	int dataType = getSfallTypeByScriptType(keyType);
	if (dataType != DATATYPE_INT || key != 0) { // returns arrayId by it's key (ignoring int(0) because it is used to "unsave" array)
		sArrayElement keyEl = sArrayElement(key, dataType);

		if (keyEl.type == DATATYPE_STR && strcmp(keyEl.strVal, get_all_arrays_special_key) == 0) { // this is a special case to get temp array containing all saved arrays
			DWORD tmpArrId = CreateTempArray(savedArrays.size(), 0);
			if (tmpArrId > 0) {
				std::vector<sArrayElement>::iterator elIt;
				ArrayKeysMap::iterator it;
				sArrayVar &tmpArr = arrays[tmpArrId];
				for (it = savedArrays.begin(), elIt = tmpArr.val.begin(); it != savedArrays.end(); ++it, ++elIt) {
					elIt->set(it->first);
				}
				std::sort(tmpArr.val.begin(), tmpArr.val.end()); // sort in ascending order
				return tmpArrId;
			}
		} else {
			ArrayKeysMap::iterator it = savedArrays.find(keyEl);
			if (it != savedArrays.end()) {
				return it->second;
			}
		}
	}
	return 0; // not found
}

void __stdcall SaveArray(DWORD key, DWORD keyType, DWORD id) {
	array_itr it, itArray = arrays.find(id); // arrayId => arrayVar
	int dataType = getSfallTypeByScriptType(keyType);
	if (itArray != arrays.end()) {
		if (dataType != DATATYPE_INT || key != 0) {
			// make array permanent
			FixArray(itArray->first);
			// if another array is saved under the same key, clear it
			ArrayKeysMap::iterator savedIt = savedArrays.find(sArrayElement(key, dataType));
			if (savedIt != savedArrays.end()) {
				if (savedIt->second == id) return; // exit, array is already saveable
				// arrays have different ID, search the ID of the "saved" array in collection of arrays
				if ((it = arrays.find(savedIt->second)) != arrays.end()) {
					// array exists, delete key value
					savedArrays.erase(savedIt); /* added for fix */
					it->second.key.unset();
				}
			}
			// make array "saved"
			itArray->second.key.setByType(key, dataType);
			savedArrays[itArray->second.key] = id;
		} else { // key of int(0) is used to "unsave" array without destroying it
			savedArrays.erase(itArray->second.key);
			itArray->second.key.unset();
		}
	}
}

/*
	Sets element to array created in last CreateArray call (used for array expressions)
	For normal arrays, this method should be called in strict linear index order:
	(create_array(0, 0) + arrayexpr(0, 20) + arrayexpr(1, 25) + arrayexpr(2, 777) + ...)

	For assoc arrays order doesn't matter:
	(create_array(0, 1) + arrayexpr("key1", "val1") + arrayexpr("key2", 25) + arrayexpr(3, 6.1242) + ...)

	Should always return 0!
*/
long __stdcall StackArray(DWORD key, DWORD keyType, DWORD val, DWORD valType) {
	if (stackArrayId == 0 || !ArrayExists(stackArrayId)) return 0;

	if (!arrays[stackArrayId].isAssoc()) { // automatically resize array to fit one new element
		size_t size = arrays[stackArrayId].val.size();
		if (size >= ARRAY_MAX_SIZE) return 0;
		if (key >= size) arrays[stackArrayId].val.resize(size + 1);
	}
	setArray(stackArrayId, key, keyType, val, valType, 0);
	return 0;
}

sArrayVar* __stdcall GetRawArray(DWORD id) {
	array_itr it = arrays.find(id);
	return (it != arrays.end()) ? &it->second : nullptr;
}
