#pragma once



#include "Common/Model.hpp"



namespace Clothier {

    unq<Model> createRectangle(ivec2 lod, float weaveSize, float totalMass, int groupSize, const mat4 & transform);

    unq<Model> createTriangle(int lod, float weaveSize, float totalMass, int groupSize, const mat4 & transform);

}