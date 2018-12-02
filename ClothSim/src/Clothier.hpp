#pragma once



#include "Common/SoftModel.hpp"



namespace Clothier {

    unq<SoftModel> createRectangle(ivec2 lod, float weaveSize, int groupSize);

    unq<SoftModel> createTriangle(int lod, float weaveSize, int groupSize);

}