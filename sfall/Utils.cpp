#include <algorithm>
#include <iterator>

#include "Utils.h"

namespace sfall
{

WORD ByteSwapW(WORD w) {
	return ((w & 0xFF) << 8) | ((w & 0xFF00) >> 8);
}

DWORD ByteSwapD(DWORD dw) {
	return ((dw & 0xFF) << 24) | ((dw & 0xFF00) << 8) | ((dw & 0xFF0000) >> 8) | ((dw & 0xFF000000) >> 24);
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}

std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first) {
		return str;
	}
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

void trim(char* str) {
	if (str[0] == 0) return;
	int len = strlen(str) - 1;

	int i = len;
	while (len >= 0 && isspace(str[len])) len--;
	if (i != len) str[len + 1] = '\0'; // delete all spaces on the right

	i = 0;
	while (i < len && isspace(str[i])) i++;
	if (i > 0) {
		int j = 0;
		do {
			str[j] = str[j + i]; // shift all chars (including null char) to the left
		} while (++j <= len);
	}
}

void ToLowerCase(std::string& line) {
	std::transform(line.begin(), line.end(), line.begin(), ::tolower);
}

// returns position, find word must be lowercase
const char* strfind(const char* source, const char* word) {
	if (source == 0 || word == 0 || *word == 0) return 0;
	const char *w_pos, *s_pos;
	while (*source != 0) {
		w_pos = word, s_pos = source++;
		while (tolower(*s_pos) == *w_pos) {
			s_pos++;
			if (*++w_pos == 0) return s_pos - (w_pos - word);
		}
	}
	return 0;
}

// replace all '/' chars with '\'
void StrNormalizePath(char* path) {
	if (*path == 0) return;
	do {
		if (*path == '/') *path = '\\';
	} while (*(++path) != 0);
}

long StrToLong(const char* str, int base /* = 0 */) {
	// Trim leading whitespaces.
	while (isspace(static_cast<unsigned char>(*str))) ++str;
	// Support 0b prefix to detect binary values (for compatibility with GetPrivateProfile* functions).
	if ((base == 0 || base == 2) && str[0] == '0' && str[1] == 'b' && str[2] != '\0') {
		str = &str[2];
		base = 2;
	}
	char* end;
	errno = 0;
	return strtol(str, &end, base); // see https://stackoverflow.com/a/6154614
}


// max range 0-32767
//long GetRandom(long min, long max) { // uncomment the srand() in main.cpp before use
//	return (min + (std::rand() % (max - (min - 1))));
//}

}
