#pragma once



#include <string>

#include "Global.hpp"
#include "Model.hpp"



struct GLFWwindow;



namespace Simulation {

    bool setup(const std::string & resourceDir);

    void cleanup();

    void set(const Model & model, const mat4 & modelMat, const mat3 & normalMat, float depth, const glm::vec3 & centerOfMass);

    // Does one slice and returns if it was the last one
    bool step();

    // Returns the index of the slice that would be next
    int slice();

    // Returns the total number of slices
    int sliceCount();

    // Returns the lift of the previous sweep
    vec3 lift();

    // Returns the drag of the previous sweep
    vec3 drag();

    uint frontTex();
    uint sideTex();

    ivec2 size();


}