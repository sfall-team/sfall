#pragma once

#include <functional>
#include <unordered_map>
#include <set>
#include <Windows.h>

#include "ScriptValue.h"

namespace sfall
{
namespace script
{

#define ARRAY_MAX_STRING        (255)  // maximum length of string to be stored as array key or value
#define ARRAY_MAX_SIZE       (100000)  // maximum number of array elements,
                                       // so total maximum memory/disk footprint of one array is: 16 + (ARRAY_MAX_STRING + 8) * ARRAY_MAX_SIZE

// special actions for arrays using array_resize operator
#define ARRAY_ACTION_SORT       (-2)
#define ARRAY_ACTION_RSORT      (-3)
#define ARRAY_ACTION_REVERSE    (-4)
#define ARRAY_ACTION_SHUFFLE    (-5)


// TODO: rewrite (or replace with ScriptValue)
class sArrayElement
{
public:
	DataType type;
	DWORD len; // for strings
	union {
		long  intVal;
		float floatVal;
		char* strVal;
	};

	sArrayElement();
	// this constructor does not copy strings (for performance), use set() for actual array elements
	sArrayElement(DWORD _val, DataType _dataType);
	sArrayElement(const long&);

	// free string resource from memory, has to be called manually when deleting element
	void clear();

	// set* methods will actually COPY strings, use this when acquiring data from the scripting engine
	void setByType(DWORD val, DataType dataType);

	void sArrayElement::set( const sArrayElement &el )
	{
		setByType(el.intVal, el.type);
	}

	void set(long val);
	void set(float val);
	void set(const char* val, int _len = -1);
	void unset();

	DWORD sArrayElement::getHash() const
	{
		return getHashStatic(*(DWORD*)&intVal, type);
	}

	DWORD static getHashStatic(DWORD value, DataType type);

	bool operator < (const sArrayElement &el) const;
};

class sArrayElement_HashFunc
{
public:
	size_t operator() (const sArrayElement &el) const {
		return el.getHash();
	}
};

class sArrayElement_EqualFunc
{
public:
	bool operator() (const sArrayElement &elA, const sArrayElement &elB) const;
};

struct sArrayVarOld
{
	DWORD len;
	DWORD datalen;
	DWORD* types;
	char* data;
};

#define ARRAYFLAG_ASSOC		(1) // is map
#define ARRAYFLAG_CONSTVAL	(2) // don't update value of key if the key exists in map

typedef std::unordered_map<sArrayElement, DWORD, sArrayElement_HashFunc, sArrayElement_EqualFunc> ArrayKeysMap;

/**
	This class represents sfall array
	It can be both list (normal array) and map (associative)

	TODO: rewrite for better interface (especially associate arrays), get rid of ad-hoc solutions
		Maybe use unordered_map with vector to track insertion order?
*/
class sArrayVar
{
public:
	DWORD flags;
	sArrayElement key;    // array associated key, if it was saved
	ArrayKeysMap keyHash; // key element => element index, for faster lookup
	std::vector<sArrayElement> val; // list of values or key=>value pairs (even - keys, odd - values)

	bool isAssoc() const {
		return (flags & ARRAYFLAG_ASSOC);
	}

	// logical array size (number of elements for normal arrays; number of key=>value pairs for associative)
	int size() const {
		return isAssoc()
			? val.size() / 2
			: val.size();
	}

	// usefull when filling array from within sfall code (normal lists only)
	void push_back(long _value) {
		if (!isAssoc()) {
			val.push_back(_value);
		}
	}

	sArrayVar() : flags(0), key() {}

	// free memory used by strings
	void clear() {
		clearRange(0);
		key.clear();
	}

	void clearRange(int from, int to = -1);
};

// arrays map: arrayId => arrayVar
typedef std::unordered_map<DWORD, sArrayVar> ArraysMap;
extern ArraysMap arrays;
typedef ArraysMap::const_iterator array_citr;
typedef ArraysMap::iterator array_itr;
typedef std::pair<DWORD, sArrayVar> array_pair;

// auto-incremented ID
extern DWORD nextArrayID;

extern DWORD arraysBehavior;

// temp arrays: set of arrayId
extern std::set<DWORD> tempArrays;

// saved arrays: arrayKey => arrayId
extern ArrayKeysMap savedArrays;

bool LoadArrays(HANDLE h);
void SaveArrays(HANDLE h);
int GetNumArrays();
void GetArrays(int* arrays);

void DEGetArray(int id, DWORD* types, void* data);
void DESetArray(int id, const DWORD* types, const void* data);

// creates new normal (persistent) array. len == -1 specifies associative array (map)
DWORD _stdcall CreateArray(int len, DWORD flags);

// same as CreateArray, but creates temporary array instead (will die at the end of the frame)
DWORD _stdcall TempArray(DWORD len, DWORD flags);

// destroys array
void _stdcall FreeArray(DWORD id);

/*
	op_get_array_key can be used to iterate over all keys in associative array
*/
ScriptValue _stdcall GetArrayKey(DWORD id, int index);

// get array element by index (list) or key (map)
ScriptValue _stdcall GetArray(DWORD id, const ScriptValue& key);

// set array element by index or key
void _stdcall SetArray(DWORD id, const ScriptValue& key, const ScriptValue& val, bool allowUnset);

// number of elements in list or pairs in map
int _stdcall LenArray(DWORD id);

// change array size (only works with list)
long _stdcall ResizeArray(DWORD id, int newlen);

// make temporary array persistent
void _stdcall FixArray(DWORD id);

// searches for a given element in array and returns it's index (for list) or key (for map) or int(-1) if not found
ScriptValue _stdcall ScanArray(DWORD id, const ScriptValue& val);

// get saved array by it's key
DWORD _stdcall LoadArray(const ScriptValue& key);

// make array saved into the savegame with associated key
void _stdcall SaveArray(const ScriptValue& key, DWORD id);

// special function that powers array expressions
DWORD _stdcall StackArray(const ScriptValue& key, const ScriptValue& val);

}
}
