#include "GrlModel.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include "glad/glad.h"

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
	cout << "Loading grl model: " << filename << endl;
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
		
		//right now just grabbing 8 elements
		//Change to while if all are needed/wanted
		for (int i = 0; i < 8; i++){
			if (getline(lineStream, token, ' ')) {
				tokens.push_back(token);
			}
		}

		//meta tags
		if (tokens.at(0)[0] == '#') {
			string metaType = tokens.at(1);
			if (metaType == "name:") {
				//push back previous sub model unless we are on the first submodel
				if (curSubModel) {
					subModels.push_back(*curSubModel);
				}
				curSubModel = new grl::submodel();
				curSubModel->name = tokens.back();
				state = 0;

				cout << "\tWith submodel: " << tokens.back() << endl;
			}
			else if (metaType == "origin") {
				matrixLine = 0;
				state = 1;
			}
			else if (metaType == "vertices_count:") {
				maxVertCount = stoi(tokens.back());
				vertCount = 0;
				curSubModel->posData.reserve(maxVertCount);
				curSubModel->norData.reserve(maxVertCount);
				curSubModel->texData.reserve(maxVertCount);
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
				matrixLine++;
			}
			else if (state == 2) {
				if (vertCount < maxVertCount) {
					if (tokens.size() < 8) {
						cerr << "Not a valid vertex in " << filename << ":" << lineNum << endl;
						exit(EXIT_FAILURE);
					}

					//TODO speed up this by using something else than stof
					float pos_x = stof(tokens.at(0));
					float pos_y = stof(tokens.at(1));
					float pos_z = stof(tokens.at(2));

					float nor_x = stof(tokens.at(3));
					float nor_y = stof(tokens.at(4));
					float nor_z = stof(tokens.at(5));

					float tex_u = stof(tokens.at(6));
					float tex_v = stof(tokens.at(7));

					curSubModel->posData.push_back(vec3(pos_x, pos_y, pos_z));
					curSubModel->norData.push_back(vec3(nor_x, nor_y, nor_z));
					curSubModel->texData.push_back(vec2(tex_u, tex_v));
				}
				else {
					cerr << "Expected meta tag in " << filename << ":" << lineNum << endl;
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	//push back the last submodel
	subModels.push_back(*curSubModel);

	//initialize vao and buffers with the correct size
	size_t numSubModels = subModels.size();
	vaoID = new unsigned int[numSubModels];
	posBufID = new unsigned int[numSubModels];
	norBufID = new unsigned int[numSubModels];
	texBufID = new unsigned int[numSubModels];
}

void GrlModel::init() {
	for (int i = 0; i < subModels.size(); i++) {
		size_t vertSize = subModels[i].posData.size();

		glGenVertexArrays(1, &vaoID[i]);
		glBindVertexArray(vaoID[i]);

		//positions
		glGenBuffers(1, &posBufID[i]);
		glBindBuffer(GL_ARRAY_BUFFER, posBufID[i]);
		glBufferData(GL_ARRAY_BUFFER, vertSize * 3 * sizeof(float), subModels[i].posData.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);


		//normals
		glGenBuffers(1, &norBufID[i]);
		glBindBuffer(GL_ARRAY_BUFFER, norBufID[i]);
		glEnableVertexAttribArray(1);
		glBufferData(GL_ARRAY_BUFFER, vertSize * 3 * sizeof(float), subModels[i].norData.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//texture coordinates
		glGenBuffers(1, &texBufID[i]);
		glBindBuffer(GL_ARRAY_BUFFER, texBufID[i]);
		glBufferData(GL_ARRAY_BUFFER, vertSize * 2 * sizeof(float), subModels[i].texData.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);


		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			cerr << "Error: << " << error << "initializing submodel " << subModels[i].name << endl;
			exit(EXIT_FAILURE);
		}
	}
}

void GrlModel::draw(const shared_ptr<Program> prog, mat4 t, mat4 global_r, mat4 s, mat4 local_r) {
	for (size_t i = 0; i < subModels.size(); i++) {
		drawSubModel(prog, i, t, global_r, s, local_r);
	}
}

void GrlModel::drawSubModel(const shared_ptr<Program> prog, string subModelName, mat4 t, mat4 global_r, mat4 s, mat4 local_r) {
	//get correct index of submodel based off name
	int i;
	bool found = false;
	for (i = 0; i < subModels.size(); i++)
	{
		if (subModels[i].name == subModelName) {
			found = true;
			break;
		}
	}
	if (!found) {
		cerr << "could not find submodel: " << subModelName << endl;
		exit(EXIT_FAILURE);
	}
	drawSubModel(prog, i, t, global_r, s, local_r);
}

void GrlModel::drawSubModel(const shared_ptr<Program> prog, unsigned int subModelIndex, mat4 t, mat4 global_r, mat4 s, mat4 local_r) {
	mat4 M, T, R, S;
	
	//rotate locally
	//get translation from submodel matrix
	mat4 subModTranslate = mat4(1);
	subModTranslate[0][3] = subModels[subModelIndex].O[0][3];
	subModTranslate[1][3] = subModels[subModelIndex].O[1][3];
	subModTranslate[2][3] = subModels[subModelIndex].O[2][3];
	//total translation from origin
	mat4 combinedT = t * subModTranslate;
	//matrix where we move to origin, rotate, and move back without local transform
	T = t;
	R = global_r;
	S = s;
	mat4 localR = t * local_r * inverse(combinedT);

	//final model matrix
	M = T * R * S * subModels[subModelIndex].O;
	string name = subModels[subModelIndex].name; //for temp debugging only
	glUniformMatrix4fv(prog->getUniform("u_modelMat"), 1, GL_FALSE, &M[0][0]);
	glBindVertexArray(vaoID[subModelIndex]);
	glDrawArrays(GL_TRIANGLES, 0, subModels[subModelIndex].posData.size());
	glBindVertexArray(0);
}
