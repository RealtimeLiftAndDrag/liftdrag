#include "GRLLoader.hpp"

#include <iostream>
#include <sstream>
#include <fstream>



namespace grl {

    static bool isEmpty(std::istream & is) {
        return (is >> std::ws).peek() == std::char_traits<char>::eof();
    }

    static bool readMatrix(std::istream & is, mat4 & r_m) {
        static std::string line;

        for (int c(0); c < 4; ++c) {
            if (!std::getline(is, line)) {
                return false;
            }
            std::stringstream ss(line);
            for (int r(0); r < 4; ++r) {
                if (!(ss >> r_m[c][r])) {
                    return false;
                }
            }
            if (!isEmpty(ss)) {
                return false;
            }
        }

        return true;
    }

    static bool readVertex(std::istream & is, vec3 & r_position, vec3 & r_normal, vec2 & r_texCoord) {
        static std::string line;

        if (!std::getline(is, line)) {
            return false;
        }
        std::stringstream ss(line);
        if (!(
            (ss >> r_position.x) &&
            (ss >> r_position.y) &&
            (ss >> r_position.z) &&

            (ss >> r_texCoord.x) &&
            (ss >> r_texCoord.y) &&

            (ss >> r_normal.x) &&
            (ss >> r_normal.y) &&
            (ss >> r_normal.z)
        )) {
            return false;
        }

        return true;
    }

    std::vector<Object> load(const std::string & filename) {
        std::ifstream ifs(filename);
        if (!ifs.good()) {
            std::cerr << "File not found: " << filename << std::endl;
            return {};
        }

        std::vector<Object> objects;

        std::string line, word, tag;
        while (std::getline(ifs, line)) {
            // Skip empty line
            if (line.length() == 0) {
                continue;
            }

            std::stringstream ss(line);
            
            ss >> word;

            // Skip comment
            if (word == "//") {
                continue;
            }

            // Anything else should be meta tag
            if (word != "#") {
                std::cerr << "Unknown content: " << line << std::endl;
                return {};
            }

            // Get meta tag
            if (!(ss >> tag)) {
                // Empty meta tag
                continue;
            }

            // Name
            if (tag == "name:") {
                objects.resize(objects.size() + 1); // increment number of objects
                if (!(ss >> objects.back().name)) {
                    std::cerr << "Missing name" << std::endl;
                    return {};
                }
            }
            // Origin matrix
            else if (tag == "origin" && (ss >> word) && word == "matrix") {
                if (!readMatrix(ifs, objects.back().originMat)) {
                    std::cerr << "Invalid matrix" << std::endl;
                    return {};
                }
            }
            // Vertices
            else if (tag == "vertices_count:") {
                int vertexCount;
                if (!(ss >> vertexCount)) {
                    std::cerr << "Invalid vertex count";
                    return {};
                }
                objects.back().posData.resize(vertexCount);
                objects.back().norData.resize(vertexCount);
                objects.back().texData.resize(vertexCount);
                for (int i(0); i < vertexCount; ++i) {
                    if (!readVertex(
                        ifs,
                        objects.back().posData[i],
                        objects.back().norData[i],
                        objects.back().texData[i]
                    )) {
                        std::cerr << "Invalid vertex" << std::endl;
                        return {};
                    }
                }
            }
        }

        return move(objects);
    }

}