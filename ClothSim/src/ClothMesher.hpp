#pragma once



#include "Common/SoftModel.hpp"



unq<SoftModel> createClothRectangle(ivec2 lod, float weaveSize, int groupSize);

unq<SoftModel> createClothTriangle(int lod, float weaveSize, int groupSize);
