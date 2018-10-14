#pragma once



#include <string>
#include <fstream>
#include <sstream>

#include "Global.hpp"



namespace Util {



    inline std::pair<bool, std::string> readTextFile(const std::string & filepath) {
        std::ifstream ifs(filepath);
        if (!ifs.good()) {
            return {};
        }
        std::string str(
            std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>{}
        );
        if (!ifs.good()) {
            return {};
        }
        return { true, std::move(str) };
    }

    inline bool writeTextFile(const std::string & filepath, const std::string & str) {
        std::ofstream ofs(filepath);
        if (!ofs.good()) {
            return false;
        }
        ofs << str;
        return ofs.good();
    }

    inline void toLower(std::string & str) {
        for (char & c : str) c = char(::tolower(c));
    }

    inline void toUpper(std::string & str) {
        for (char & c : str) c = char(::toupper(c));
    }

    inline std::string getExtension(const std::string & filename) {
        for (size_t i(0); i < filename.size() - 1; ++i) {
            if (filename[i] == '.') {
                char nextC(filename[i + 1]);
                if (nextC != '.' && nextC != '/' && nextC != '\\') {
                    std::string ext(filename.substr(i + 1));
                    toLower(ext);
                    return std::move(ext);
                }
            }
        }
        return {};
    }

    // Formats the given number into a string
    inline std::string numberString(double num, bool fixed, int precision) {
        std::stringstream ss;
        ss.precision(precision);
        if (fixed) ss << std::fixed;
        ss << num;
        return ss.str();
    }

    // Formats the given vector into a string
    inline std::string vectorString(const vec3 & v, bool fixed, int precision) {
        std::stringstream ss;
        ss.precision(precision);
        if (fixed) ss << std::fixed;
        ss << "<" << v.x << " " << v.y << " " << v.z << ">";
        return ss.str();
    }

    // Returns if the character is alphanumeric, a symbol, or a space
    inline bool isPrintable(char c) {
        return uchar(c) >= 32 && uchar(c) != 127 && uchar(c) != 255;
    }



}