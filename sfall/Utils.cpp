#include <algorithm>
#include <iterator>

#include "Utils.h"

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

void ToLowerCase(std::string& line) {
	std::transform(line.begin(), line.end(), line.begin(), ::tolower);
}

bool isSpace(char c) {
	return (c == ' ' || c == '\t' /*|| c == '\n' || c == '\r'*/);
}

void strtrim(char* str) {
	if (str[0] == 0) return;
	int len = strlen(str) - 1;

	int i = len;
	while (len >= 0 && isSpace(str[len])) len--;
	if (i != len) str[len + 1] = '\0'; // delete all spaces on the right

	i = 0;
	while (i < len && isSpace(str[i])) i++;
	if (i > 0) {
		int j = 0;
		do {
			str[j] = str[j + i]; // shift all chars (including null char) to the left
		} while (++j <= len);
	}
}

// returns position, find word must be lowercase
const char* strfind(const char* source, const char* word) {
	if (source == 0 || word == 0 || *word == 0) return 0;
	const char *w_pos, *s_pos;
	while(*source != 0) {
		w_pos = word, s_pos = source++;
		while (tolower(*s_pos) == *w_pos) {
			s_pos++;
			if (*++w_pos == 0) return s_pos - (w_pos - word);
		}
	}
	return 0;
}

// replace all '/' chars to '\'
void StrNormalizePath(char* path) {
	if (*path == 0) return;
	do {
		if (*path == '/') *path = '\\';
	} while (*(++path) != 0);
}
