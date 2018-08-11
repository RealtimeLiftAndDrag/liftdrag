#include "GrlLoader.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
using namespace glm;
using namespace std;
#define EPS 0.0001

namespace grl {

	class vertex {
	public:
		glm::vec3 pos, norm;
		glm::vec2 texCoord;
	};

	struct submodel
	{
	public:
		string name;
		std::vector<vertex*>* vertices;
		glm::mat4 O;
	};

	void grl() {

	}

	vector<submodel*> subModels;
	void loadSubModels(string filename)
	{
		ifstream file(filename);
		if (file.bad())
		{
			cerr << "File not found: " << filename << endl;
			exit(EXIT_FAILURE);
		}
		submodel* curSubModel = nullptr;

		int state = 0; //0 = unset; 1 = matrix; 2 = vertices

		int lineNum = 0;
		int maxVertCount = 0;
		int vertCount = 0;
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
					curSubModel = new submodel();
					curSubModel->name = tokens.back();
					state = 0;
				}
				else if (metaType == "origin") {
					state = 1;
				}
				else if (metaType == "vertices_count:") {
					maxVertCount = stoi(tokens.back());
					vertCount = 0;
					curSubModel->vertices = new vector<vertex*>();
					curSubModel->vertices->reserve(maxVertCount);
					state = 2;
				}
				//TODO implement other meta tags if wanted
			}
			else { //either matrix or vertices
				if (state == 2) {
					if (vertCount < maxVertCount) {
						if (tokens.size() < 8) {
							cerr << "Not a valid vertex in " << filename << ":" << lineNum << endl;
							exit(EXIT_FAILURE);
						}
						vertex * v = new vertex();
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

						curSubModel->vertices->push_back(v);
					}
				}
			}

		}

		//push back the last submodel
		subModels.push_back(curSubModel);

	}
}

//	if (str.text[0] == '#') //meta
//	{
//		//get meta type
//		int doublepoint = str.findchar(':');
//		if (doublepoint < 1)    continue;//try next line, its obiously just a comment
//		extstr_ meta, args;
//		str.delete_range(0, 0);
//		if (!str.get_doubplepoint_argument(meta.text, args.text))continue;
//		mode = LVL_META;
//		//meta.delete_from_char(' ', FALSE);


//		if (meta == " name")
//		{
//			args.delete_char(' ');
//			current_name = args;
//			continue;
//			//args contains now the name of the object
//		}
//		else if (meta == " origin matrix (4 x 4)")
//		{
//			args.delete_space();
//			args.delete_char(':');
//			load_matrix = load_matrix = true;
//			continue;
//			//Now read matrix
//		}
//		else if (meta == " vertices_count")
//		{
//			args.delete_space();
//			args.delete_char(':');
//			vcount = atoi(args.text);
//			load_matrix = false;
//			continue;
//			//args contains now the vertices count
//		}
//		else if (meta == " texture")
//		{

//			args.delete_space();
//			int i = args.findchar_bwd('\\');
//			if (i >= 0)
//			{
//				args.delete_range(0, i);
//			}
//			continue;
//			//args contains now the texture file
//		}
//		else if (meta == " properties")
//		{
//			extstr_ part, submeta, subarg;
//			for (int ii = 0; args.get_argument(ii, part.text); ii++)
//			{
//				if (!part.get_doubplepoint_argument(submeta.text, subarg.text))continue;
//				subarg.delete_space_anf();
//				submeta.delete_space_anf();
//				if (submeta == "ANIM")
//				{
//					continue;
//					//some animation properties, etc
//				}
//				else if (submeta == "SCALE")
//				{
//					continue;
//					//some animation properties, etc
//				}
//			}
//			continue;
//		}
//	}
//	if (str.getlen() <= 0 || str.text[0] == ' ')
//		continue;


//	if (load_matrix)
//	{
//		switch (matrix_row)
//		{
//		case 1:
//			sscanf(str.text, "%f %f %f %f",
//				&Origin[0][0], &Origin._12, &Origin._13, &Origin._14
//			);
//			matrix_row++;
//			break;
//		case 2:
//			sscanf(str.text, "%f %f %f %f",
//				&Origin._21, &Origin._22, &Origin._23, &Origin._24
//			);
//			matrix_row++;
//			break;
//		case 3:
//			sscanf(str.text, "%f %f %f %f",
//				&Origin._31, &Origin._32, &Origin._33, &Origin._34
//			);
//			matrix_row++;
//			break;
//		case 4:
//			sscanf(str.text, "%f %f %f %f",
//				&Origin._41, &Origin._42, &Origin._43, &Origin._44
//			);
//			matrix_row = 1;
//			break;
//		default:
//			matrix_row = 1;
//			break;
//		}

//	}
//	else
//	{
//		vertex v;
//		sscanf(str.text, "%f %f %f %f %f %f %f %f",
//			&v.Pos.x, &v.Pos.y, &v.Pos.z,
//			&v.Tex.x, &v.Tex.y,
//			&v.Norm.x, &v.Norm.y, &v.Norm.z
//		);

//		v.Tex.y *= -1;
//		vertices.pushback(v);
//		if (vertices.size() >= vcount)//make the billboard
//		{
//			SimpleVertex* v_array = new SimpleVertex[vcount];
//			for (int i = 0; i < vcount; i++)
//				v_array[i] = vertices[i];


//			//***********************Roundness Calc******************************

//			for (int i = 0; i < vcount; i += 3)
//			{

//				vec3 a = v_array[i + 0].Pos;
//				XMFLOAT3 b = v_array[i + 1].Pos;
//				XMFLOAT3 c = v_array[i + 2].Pos;

//				XMFLOAT3 Cnorm = normalize(cross((a - b), (a - c)));

//				float coeff_a = saturate(dot(Cnorm, normalize(v_array[i + 0].Norm)));
//				float coeff_b = saturate(dot(Cnorm, normalize(v_array[i + 1].Norm)));
//				float coeff_c = saturate(dot(Cnorm, normalize(v_array[i + 2].Norm)));

//				float C_halfsphere = 0.42f;
//				float C_streamline = 0.04f;
//				float diff = C_halfsphere - C_streamline;

//				v_array[i + 0].Data.x = C_halfsphere - cos(coeff_a) * diff;
//				v_array[i + 1].Data.x = C_halfsphere - cos(coeff_b) * diff;
//				v_array[i + 2].Data.x = C_halfsphere - cos(coeff_c) * diff;

//				v_array[i + 0].Data.y = 1;
//				v_array[i + 1].Data.y = 1;
//				v_array[i + 2].Data.y = 1;
//				/*
//				XMFLOAT3 a = v_array[i + 0].Pos;
//				XMFLOAT3 b = v_array[i + 1].Pos;
//				XMFLOAT3 c = v_array[i + 2].Pos;

//				XMFLOAT3 Cnorm = normalize(cross((a - b), (a - c)));

//				v_array[i + 0].Data = Cnorm;
//				v_array[i + 1].Data = Cnorm;
//				v_array[i + 2].Data = Cnorm;
//				*/
//			}


//			vector<vertex*> gouraurd;
//			int *check = new int[vcount];
//			for (int i = 0; i < vcount; i++)
//				check[i] = 0;

//			for (int ii = 0; ii < vcount; ii++)
//			{
//				//v_array[ii].Pos.z -= 1;
//				if (check[ii] == 1)continue;

//				float x = v_array[ii].Data.x;
//				gouraurd.push_back(&v_array[ii]);
//				check[ii] = 1;
//				for (int uu = ii + 1; uu < vcount; uu++)
//				{
//					if (check[uu] == 1)continue;
//					if (same(v_array[uu].Pos, v_array[ii].Pos))
//					{
//						gouraurd.push_back(&v_array[uu]);
//						x += v_array[uu].Data.x;
//						check[uu] = 1;
//					}
//				}
//				x /= (float)gouraurd.size();

//				for (int uu = 0; uu < gouraurd.size(); uu++)
//					gouraurd[uu]->Data.x = x;

//				gouraurd.clear();
//			}

//			delete[] check;
//			//***********************Roundness Calc******************************




//			strcpy(model.name, current_name.text);
//			model.vert_count = vcount;

//			model.O = Origin;
//			models->pushback(model);


//			delete[] v_array;

//			vertices.reset();
//		}
//	}

//}

//return models;
//	}
//}
//}