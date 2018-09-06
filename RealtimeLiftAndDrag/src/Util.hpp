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
        size_t pos(filename.find('.'));
        if (pos == std::string::npos) {
            return {};
        }

        std::string ext(filename.substr(pos + 1));
        toLower(ext);
        return std::move(ext);
    }

    // Formats the given number into a string
    static std::string numberString(float num, int precision) {
        std::stringstream ss;
        ss.precision(precision);
        ss << num;
        return ss.str();
    }



}