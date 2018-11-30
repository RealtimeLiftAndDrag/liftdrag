#pragma once



#include "Common/SoftModel.hpp"



unq<SoftModel> createClothRectangle(ivec2 lod, float weaveSize);

unq<SoftModel> createClothTriangle(int lod, float weaveSize);
