#include "GrlModel.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
using namespace glm;
using namespace std;



void GrlModel::loadSubModels(string filename)
{
	ifstream file(filename);
	if (file.bad())
	{
		cerr << "File not found: " << filename << endl;
		exit(EXIT_FAILURE);
	}
	grl::submodel* curSubModel = nullptr;

	int state = 0; //0 = unset; 1 = matrix; 2 = vertices

	int lineNum = 0;
	int maxVertCount = 0;
	int vertCount = 0;
	int matrixLine = 0;
	lineNum++;
	string line;
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
		vector<string> tokens;
		stringstream lineStream(line);
		string token;
		while (getline(lineStream, token, ' ')) {
			tokens.push_back(token);
		}

		//meta tags
		if (tokens.at(0)[0] == '#') {
			string metaType = tokens.at(1);
			if (metaType == "name:") {
				//push back previous sub model if we are on the first submodel
				if (curSubModel) {
					subModels.push_back(curSubModel);
				}
				curSubModel = new grl::submodel();
				curSubModel->name = tokens.back();
				state = 0;
			}
			else if (metaType == "origin") {
				matrixLine = 0;
				state = 1;
			}
			else if (metaType == "vertices_count:") {
				maxVertCount = stoi(tokens.back());
				vertCount = 0;
				curSubModel->vertices = new vector<grl::vertex*>();
				curSubModel->vertices->reserve(maxVertCount);
				state = 2;
			}
			//TODO implement other meta tags if wanted
		}
		else { //either matrix or vertices
			if (state == 1) {
				if (tokens.size() < 4) {
					cerr << "Not a valid matrix row in " << filename << ":" << lineNum << endl;
					exit(EXIT_FAILURE);
				}

				//opengl uses column row so assuming same of grl
				curSubModel->O[matrixLine][0] = stof(tokens.at(0));
				curSubModel->O[matrixLine][1] = stof(tokens.at(1));
				curSubModel->O[matrixLine][2] = stof(tokens.at(2));
				curSubModel->O[matrixLine][3] = stof(tokens.at(3));
			}
			else if (state == 2) {
				if (vertCount < maxVertCount) {
					if (tokens.size() < 8) {
						cerr << "Not a valid vertex in " << filename << ":" << lineNum << endl;
						exit(EXIT_FAILURE);
					}
					grl::vertex * v = loadVertex(tokens);
					curSubModel->vertices->push_back(v);
				}
			}
		}
	}
	//push back the last submodel
	subModels.push_back(curSubModel);
}

grl::vertex* GrlModel::loadVertex(vector<string> tokens){
	grl::vertex * v = new grl::vertex();

	float pos_x = stof(tokens.at(0));
	float pos_y = stof(tokens.at(1));
	float pos_z = stof(tokens.at(2));

	float nor_x = stof(tokens.at(3));
	float nor_y = stof(tokens.at(4));
	float nor_z = stof(tokens.at(5));

	float tex_u = stof(tokens.at(6));
	float tex_v = stof(tokens.at(7));

	v->pos = vec3(pos_x, pos_y, pos_z);
	v->norm = vec3(nor_x, nor_y, nor_z);
	v->texCoord = vec2(tex_u, tex_v);
	return v;
}