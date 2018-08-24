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

    // Renders current situation to screen
    void render();

    // Returns the index of the slice that would be next
    int getSlice();

    // Returns the total number of slices
    int getSliceCount();

    // Returns the lift of the previous sweep
    vec3 getLift();

    // Returns the drag of the previous sweep
    vec3 getDrag();

    uint getSideTextureID();

    ivec2 getSize();


}