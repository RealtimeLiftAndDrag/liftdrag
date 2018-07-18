#pragma once



#include <string>
#include <fstream>



namespace util {



inline bool readTextFile(const std::string & filepath, std::string & r_dst) {
    std::ifstream ifs(filepath);
    if (!ifs.good()) {
        return false;
    }
    std::string str(
        std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>{}
    );
    if (!ifs.good()) {
        return false;
    }
    r_dst = std::move(str);
    return true;
}

inline bool writeTextFile(const std::string & filepath, const std::string & src) {
    std::ofstream ofs(filepath);
    if (!ofs.good()) {
        return false;
    }
    ofs << src;
    return ofs.good();
}



}