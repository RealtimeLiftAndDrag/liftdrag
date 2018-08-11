#pragma once



#include <string>
#include <fstream>

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



}