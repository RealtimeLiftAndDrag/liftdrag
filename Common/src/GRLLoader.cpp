#include "GRLLoader.hpp"

#include <iostream>
#include <sstream>
#include <fstream>



namespace grl {

    std::vector<Object> load(const std::string & filename) {
        std::ifstream file(filename);
        if (!file.good()) {
            std::cerr << "File not found: " << filename << std::endl;
            return {};
        }

        std::vector<Object> objects;

        int state = 0; //0 = unset; 1 = matrix; 2 = vertices

        int lineNum = 0;
        int maxVertCount = 0;
        int vertCount = 0;
        int matrixLine = 0;
        lineNum++;
        std::string line;
        while (getline(file, line)) {
            //only used for debugging
            lineNum++;

            //TODO trim whitespace before line

            //skip empty line
            if (line.length() == 0) {
                continue;
            }

            //skip comments
            if (line.rfind("//", 0) == 0) {
                continue;
            }

            //tokenize
            std::vector<std::string> tokens;
            std::stringstream lineStream(line);
            std::string token;
        
            //right now just grabbing 8 elements
            //Change to while if all are needed/wanted
            for (int i = 0; i < 8; i++){
                if (getline(lineStream, token, ' ')) {
                    tokens.push_back(token);
                }
            }

            //meta tags
            if (tokens.at(0)[0] == '#') {
                std::string metaType = tokens.at(1);
                if (metaType == "name:") {
                    //push back previous sub model unless we are on the first submodel
                    objects.resize(objects.size() + 1); // increment number of objects
                    objects.back().name = tokens.back();
                    state = 0;
                }
                else if (metaType == "origin") {
                    matrixLine = 0;
                    state = 1;
                }
                else if (metaType == "vertices_count:") {
                    maxVertCount = stoi(tokens.back());
                    vertCount = 0;
                    objects.back().posData.reserve(maxVertCount);
                    objects.back().norData.reserve(maxVertCount);
                    objects.back().texData.reserve(maxVertCount);
                    state = 2;
                }
                //TODO implement other meta tags if wanted
            }
            else { //either matrix or vertices
                if (state == 1) {
                    if (tokens.size() < 4) {
                        std::cerr << "Not a valid matrix row in " << filename << ":" << lineNum << std::endl;
                        return {};
                    }

                    //opengl uses column row so assuming same of grl
                    objects.back().originMat[matrixLine][0] = stof(tokens.at(0));
                    objects.back().originMat[matrixLine][1] = stof(tokens.at(1));
                    objects.back().originMat[matrixLine][2] = stof(tokens.at(2));
                    objects.back().originMat[matrixLine][3] = stof(tokens.at(3));
                    matrixLine++;
                }
                else if (state == 2) {
                    if (vertCount < maxVertCount) {
                        if (tokens.size() < 8) {
                            std::cerr << "Not a valid vertex in " << filename << ":" << lineNum << std::endl;
                            return {};
                        }

                        //TODO speed up this by using something else than stof
                        float pos_x = stof(tokens.at(0));
                        float pos_y = stof(tokens.at(1));
                        float pos_z = stof(tokens.at(2));

                        float tex_u = stof(tokens.at(3));
                        float tex_v = stof(tokens.at(4));

                        float nor_x = stof(tokens.at(5));
                        float nor_y = stof(tokens.at(6));
                        float nor_z = stof(tokens.at(7));                    

                        objects.back().posData.emplace_back(pos_x, pos_y, pos_z);
                        objects.back().norData.emplace_back(nor_x, nor_y, nor_z);
                        objects.back().texData.emplace_back(tex_u, tex_v);
                    }
                    else {
                        std::cerr << "Expected meta tag in " << filename << ":" << lineNum << std::endl;
                        return {};
                    }
                }
            }
        }

        return std::move(objects);
    }

}