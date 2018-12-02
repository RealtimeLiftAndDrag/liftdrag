#pragma once

#include "Common/Global.hpp"
#include "Common/Model.hpp"



struct GLFWwindow;



namespace rld {

    // Mirrors GPU struct
    struct Result {
        vec3 lift;
        float _0;
        vec3 drag;
        float _1;
        vec3 torq;
        float _2;
    };

    bool setup(int texSize, int sliceCount, float liftC, float dragC, float turbulenceDist, float maxSearchDist, float windShadDist, float backforceC, float flowback, float initVelC);

    // Sets the following variables. Must be called at the start of a sweep and after setup
    void setVariables(float turbulenceDist, float maxSearchDist, float windShadDist, float backforceC, float flowback, float initVelC);

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

    // Returns the result of the sweep
    const Result & result();

    // Returns the result for each slice
    const std::vector<Result> & results();

    u32 frontTex();
    u32 sideTex();
    u32 turbulenceTex();

    int texSize();

}