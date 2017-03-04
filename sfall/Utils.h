#pragma once

#include <string>
#include <sstream>
#include <vector>

// splits a string by given delimeter
// taken from: http://stackoverflow.com/a/236803/4331475
template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim);
