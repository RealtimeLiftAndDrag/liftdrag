#pragma once



#include "Common/Model.hpp"



namespace Clothier {

    unq<Model> createRectangle(
        ivec2 lod,
        float weaveSize,
        float totalMass,
        int groupSize,
        const mat4 & transform,
        const bvec4 & pinVerts,
        const bvec4 & pinEdges
    );

    unq<SoftMesh> createTriangle(
        const vec3 & a,
        const vec3 & b,
        const vec3 & c,
        int lod,
        float totalMass,
        int groupSize,
        const bvec3 & pinVerts,
        const bvec3 & pinEdges
    );

    unq<SoftMesh> createSail(
        float luffLength,
        float leechLength,
        float footLength,
        float areaDensity,
        int lod,
        int groupSize
    );

}