#pragma once

#include <string>
#include <sstream>
#include <vector>

namespace sfall
{

// splits a string by given delimiter
// taken from: http://stackoverflow.com/a/236803/4331475
template <typename T>
void split(const std::string &s, char delim, T result, size_t limit = -1) {
	std::stringstream ss;
	ss.str(s);
	std::string item;
	size_t i = 0;
	while (std::getline(ss, item, delim) && (limit == -1 || i < limit)) {
		*(result++) = item;
		i++;
	}
}

template <typename T>
T clamp(T value, T min, T max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

WORD  ByteSwapW(WORD w);
DWORD ByteSwapD(DWORD dw);

// Splits a string by given delimiter
std::vector<std::string> split(const std::string &s, char delim);

std::string trim(const std::string& str);

void trim(char* str);

void ToLowerCase(std::string& line);

const char* strfind(const char* source, const char* word);

void StrNormalizePath(char* path);

// Uses standard strtol with base of 0 (auto, support 0x for hex and 0 for octal) and an addition of 0b prefix for binary.
long StrToLong(const char* str, int base = 0);

//long GetRandom(long min, long max);


// Case-insensitive less
// Taken from https://stackoverflow.com/a/1801913
struct ci_less
{
	// case-independent (ci) compare_less binary function
	struct nocase_compare
	{
		bool operator() (const unsigned char& c1, const unsigned char& c2) const {
			return tolower(c1) < tolower(c2);
		}
	};
	bool operator() (const std::string &s1, const std::string &s2) const {
		return std::lexicographical_compare(
			s1.begin (), s1.end (), // source range
			s2.begin (), s2.end (), // dest range
			nocase_compare ()       // comparison
		);
	}
};

}
