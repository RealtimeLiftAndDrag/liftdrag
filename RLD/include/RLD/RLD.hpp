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

    // Must be called once at the start of the program after OpenGL has been setup
    bool setup(
        int texSize,
        int sliceCount,
        float liftC, // Constant lift multiplier
        float dragC, // Constant drag multiplier
        float turbulenceDist, // Distance in wind space before turbulence starts
        float maxSearchDist, // Max distance in wind space to look for air from geometry or vice versa
        float windShadDist, // Distance in wind space z that wind shadow will extend
        float backforceC, // Constant backforce multiplier
        float flowback, // Fraction of air's lateral velocity to remain after 1 unit of distance
        float initVelC, // Constant muliplier for initial velocity of newly created air
        bool doSide, // Should the side texture be created
        bool doCloth // Is this cloth simulation
    );

    // Sets these variables at the start of a sweep. Optional
    void setVariables(float turbulenceDist, float maxSearchDist, float windShadDist, float backforceC, float flowback, float initVelC);

    // Sets the parameters of the simulation. Should be called once before the first sweep or whenever these variables change
    void set(
        const Model & model,
        const mat4 & modelMat, // The matrix that transforms the model into wind space
        const mat3 & normalMat, // The matrix that transforms the model's normals into wind space
        float windframeWidth, // The width and height of the windframe. Should be large enough to fully encapsulate the model with some excess
        float windframeDepth, // The depth of the windframe. Should be large enough to fully encapsulate the model with some excess
        float windSpeed, // The speed of the wind. Again, the wind always moves in the -z direction in wind space
        bool debug // Enables certain unnecessary features such as side view rendering and active pixel highlighting
    );

    // Does one slice and returns if it was the last one
    // Requires OpenGL state:
    // - `GL_DEPTH_TEST` disabled
    // - `GL_BLEND` enabled
    bool step(bool isExternalCall = true);

    // Does a full sweep
    // Requires OpenGL state:
    // - `GL_DEPTH_TEST` enabled
    // - `GL_BLEND` disabled
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