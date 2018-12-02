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

    static bool readVertex(std::istream & is, HardMesh::Vertex & r_vertex) {
        static std::string line;

        vec2 texCoord;

        if (!std::getline(is, line)) {
            return false;
        }
        std::stringstream ss(line);
        if (!(
            (ss >> r_vertex.position.x) &&
            (ss >> r_vertex.position.y) &&
            (ss >> r_vertex.position.z) &&

            (ss >> texCoord.x) &&
            (ss >> texCoord.y) &&

            (ss >> r_vertex.normal.x) &&
            (ss >> r_vertex.normal.y) &&
            (ss >> r_vertex.normal.z)
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
                objects.back().vertices.resize(vertexCount);
                for (int i(0); i < vertexCount; ++i) {
                    if (!readVertex(ifs, objects.back().vertices[i])) {
                        std::cerr << "Invalid vertex" << std::endl;
                        return {};
                    }
                }
            }
        }

        return move(objects);
    }

}