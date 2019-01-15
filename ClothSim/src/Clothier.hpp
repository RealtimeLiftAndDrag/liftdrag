#pragma once



#include "Common/Model.hpp"



namespace Clothier {

    // Converts triangular coordinates to cartesian coordinates
    // A is the origin, AB is the x axis, and AC is a ray at 60 degrees
    vec2 triToCart(vec2 p);

    // Converts triangular vertex point to triangular index
    u32 triI(ivec2 p);

    unq<SoftMesh> createRectangle(
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

}