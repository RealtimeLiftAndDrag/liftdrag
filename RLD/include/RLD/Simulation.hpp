#pragma once



#include <string>

#include "Common/Global.hpp"
#include "Common/Model.hpp"



struct GLFWwindow;



namespace rld {

    bool setup(const std::string & resourceDir);

    void cleanup();

    void set(const Model & model, const mat4 & modelMat, const mat3 & normalMat, float momentOfInertia, float windframeWidth, float windframeDepth, float windSpeed, bool debug);

    // Does one slice and returns if it was the last one
    bool step(bool isExternalCall = true);

    // Does a full sweep
    void sweep();

    // Returns the index of the slice that would be NEXT
    int slice();    

    // Returns the total number of slices
    int sliceCount();

    // Returns the lift of the sweep
    const vec3 & lift();
    // Returns the lifts for each slice
    const vec3 * lifts();
    
    // Returns the drag of the sweep
    const vec3 & drag();
    // Returns the drags for each slice
    const vec3 * drags();

    // Returns the torque of the sweep
    const vec3 & torque();
    // Returns the torques for each slice
    const vec3 * torques();

    uint frontTex();
    uint sideTex();

    int size();

}