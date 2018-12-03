#pragma once



#include <fstream>
#include <sstream>
#include <vector>

#include "Global.hpp"



namespace util {

    inline bool readTextFile(const std::string & file, std::string & r_dst) {
        std::ifstream ifs(file);
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
        r_dst = move(str);
        return true;
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

    inline vec3 fromSpherical(float theta, float phi) {
        float sinPhi(std::sin(phi));
        return vec3(std::cos(theta) * sinPhi, std::sin(theta) * sinPhi, std::cos(phi));
    }

    inline vec3 fromSpherical(float radius, float theta, float phi) {
        return radius * fromSpherical(theta, phi);
    }

    inline bool isZero(float v) {
        return glm::abs(v) < std::numeric_limits<float>::epsilon();
    }

    inline bool isZero(const vec2 & v) {
        return isZero(v.x) && isZero(v.y);
    }

    inline bool isZero(const vec3 & v) {
        return isZero(v.x) && isZero(v.y) && isZero(v.z);
    }

    inline bool isZero(const vec4 & v) {
        return isZero(v.x) && isZero(v.y) && isZero(v.z) && isZero(v.w);
    }

    template <typename T>
    inline std::vector<T> singleToVector(T && v) {
        std::vector<T> vec;
        vec.reserve(1);
        vec.emplace_back(move(v));
        return vec;
    }

    inline float lawOfCosines(float a, float b, float c) {
        return std::acos((a * a + b * b - c * c) / (2.0f * a * b));
    }

}