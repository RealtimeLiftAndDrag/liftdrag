#pragma once



#include <string>

#include "Global.hpp"



struct GLFWwindow;



namespace Simulation {

    bool setup(const std::string & resourceDir);

    void cleanup();

    // Does one slice and returns if it was the last one
    bool step();

    // Renders current situation to screen
    void render();

    // Returns the index of the slice that would be next
    int getSlice();

    // Returns the total number of slices
    int getNSlices();

    // Angle is in degrees
    float getAngleOfAttack();

    // Angle is in degrees
    void setAngleOfAttack(float angle);

    // Returns the lift of the previous sweep
    vec3 getLift();

    // Returns the drag of the previous sweep
    vec3 getDrag();

    uint getSideTextureID();

    ivec2 getSize();

}