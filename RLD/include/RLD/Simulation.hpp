#pragma once

#include "Common/Global.hpp"
#include "Common/Model.hpp"



struct GLFWwindow;



namespace rld {

    bool setup(int texSize, int sliceCount, float liftC, float dragC, float turbulenceDist, float maxSearchDist, float windShadDist, float backforceC, float flowback, float initVelC);

    void cleanup();

    // Sets the parameters of the simulation. Should be called once before each sweep
    // `model`          : The model in question
    // `modelMat`       : The matrix that transforms the model into wind space
    // `normalMat`      : The matrix that transforms the model's normals into wind space
    // `windframeWidth` : The width and height of the windframe. Should be large enough to fully encapsulate the model with some excess
    // `windframeDepth` : The depth of the windframe. Should be large enough to fully encapsulate the model with some excess
    // `windspeed`      : The speed of the wind. Again, the wind always moves in the -z direction in wind space
    // `debug`          : Enables certain unnecessary features such as side view rendering and active pixel highlighting
    void set(
        const Model & model,
        const mat4 & modelMat,
        const mat3 & normalMat,
        float windframeWidth,
        float windframeDepth,
        float windSpeed,
        bool debug
    );

    // Does one slice and returns if it was the last one
    // Requires OpenGL state:
    // - `GL_DEPTH_TEST` disabled
    // - `GL_BLEND` enabled
    bool step(bool isExternalCall = true);

    // Does a full sweep
    // Requires OpenGL state:
    // - `GL_DEPTH_TEST` disabled
    // - `GL_BLEND` enabled
    void sweep();

    // Resets the sweep to the first slice
    void reset();

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
    const vec3 & torq();
    // Returns the torques for each slice
    const vec3 * torqs();

    u32 frontTex();
    u32 sideTex();
    u32 turbulenceTex();

    int texSize();

}