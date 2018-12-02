#pragma once



#include "Common/Model.hpp"



namespace Clothier {

    unq<Model> createRectangle(ivec2 lod, float weaveSize, int groupSize);

    unq<Model> createTriangle(int lod, float weaveSize, int groupSize);

}