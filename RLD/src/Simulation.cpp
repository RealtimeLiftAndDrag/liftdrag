#include "Simulation.hpp"

#include <memory>
#include <iostream>

#include "glad/glad.h"
#include "glm/gtc/matrix_transform.hpp"

#include "Common/Shader.hpp"
#include "Common/GLSL.h"
#include "Common/Util.hpp"




namespace rld {

    static constexpr bool k_useAllCores(true); // Will utilize as many gpu cores as possible
    static constexpr int k_coresToUse(1024); // Otherwise, use this many
    static constexpr int k_warpSize(64); // Should correspond to target architecture
    static const ivec2 k_warpSize2D(8, 8); // The components multiplied must equal warp size
    static constexpr int k_maxPixelsDivisor(16); // max dense pixels is the total pixels divided by this
    static constexpr int k_maxGeoPerAir(3); // Maximum number of different geo pixels that an air pixel can be associated with
    static constexpr bool k_distinguishActivePixels(true); // In debug mode, makes certain "active" pixels brigher for visual clarity, but lowers performance
    static constexpr bool k_doTurbulence(false);
    static constexpr bool k_doWindShadow(true);



    // Mirrors GPU struct
    struct GeoPixel {
        vec2 windPos;
        ivec2 texCoord;
        vec4 normal;
    };

    // Mirrors GPU struct
    struct AirPixel {
        vec2 windPos;
        vec2 velocity;
        vec2 backforce;
        vec2 turbulence;
    };

    // Mirrors GPU struct
    struct Constants {
        s32 maxGeoPixels;
        s32 maxAirPixels;
        s32 screenSize;
        float liftC;
        float dragC;
        float windframeSize; // width and height of the windframe
        float windframeDepth; // depth of the windframe
        float sliceSize; // Distance between slices in wind space
        float turbulenceDist; // Distance at which turbulence will start (in wind space)
        float maxSearchDist; // Max distance for the shader searches (in wind space)
        float windShadDist; // How far back the wind shadow will stretch (in wind space)
        float backforceC;
        float flowback; // Fraction of lateral velocity to remain after 1 unit of distance
        float initVelC;
        float windSpeed;
        float dt;
        s32 slice;
        float sliceZ;
        float pixelSize;
    };

    // Mirrors GPU struct
    struct GeoPixelsPrefix {
        s32 geoCount;
        s32 padding0;
        s32 padding1;
        s32 padding2;
    };

    // Mirrors GPU struct
    struct AirPixelsPrefix {
        s32 airCount;
        s32 padding0;
        s32 padding1;
        s32 padding2;
    };



    static int s_workGroupSize;
    static ivec2 s_workGroupSize2D;
    static int s_texSize; // Width and height of the textures, which are square
    static int s_maxGeoPixels;
    static int s_maxAirPixels;
    static int s_sliceCount;
    static float s_liftC;
    static float s_dragC;

    static const Model * s_model;
    static mat4 s_modelMat;
    static mat3 s_normalMat;
    static float s_windframeWidth;
    static float s_windframeDepth;
    static float s_sliceSize;
    static float s_turbulenceDist;
    static float s_maxSearchDist;
    static float s_windShadDist;
    static float s_backforceC;
    static float s_flowback;
    static float s_initVelC;
    static float s_windSpeed;
    static float s_dt; // The time it would take to travel `s_sliceSize` at `s_windSpeed`
    static bool s_debug; // Whether to enable non essentials like side view or active pixel highlighting
    static bool s_doSide;
    static bool s_doCloth;

    static int s_currentSlice(0); // slice index [0, k_nSlices)
    static float s_angleOfAttack(0.0f); // IN DEGREES
    static float s_rudderAngle(0.0f); // IN DEGREES
    static float s_elevatorAngle(0.0f); // IN DEGREES
    static float s_aileronAngle(0.0f); // IN DEGREES
    static vec3 s_lift; // Total lift for entire sweep
    static vec3 s_drag; // Total drag for entire sweep
    static vec3 s_torq; // Total torque for entire sweep
    static std::vector<Result> s_results; // Results for each slice
    static Result s_result; // Cumulative result of all slices
    static int s_swap;

    static unq<Shader> s_shader, s_shaderDebug;
    static unq<Shader> s_foilShader, s_foilShaderDebug;
    static unq<Shader> s_prettyShader, s_sideShader;

    static Constants s_constants;

    static u32 s_constantsUBO;
    static u32 s_resultsSSBO;
    static u32 s_geoPixelsSSBO;
    static u32 s_airPixelsSSBO[2];
    static u32 s_airGeoMapSSBO;

    static u32 s_fbo; // The handle for the framebuffer object
    static u32 s_frontTex_uint; // The handle for the front texture (RGBA8UI)
    static u32 s_frontTex_unorm; // A view of s_fboTex that is RGBA8
    static u32 s_turbTex; // The handle for the turbulence texture (R8)
    static u32 s_prevTurbTex; // The handle for the previous turbulence texture (R8)
    static u32 s_shadTex; // The handle for the wind shadow texture (R8)
    static u32 s_fboNormTex; // The handle for the normal texture (RGBA16_SNORM)
    static u32 s_flagTex; // The handle for the flag texture (R32UI)
    static u32 s_sideTex; // The handle for the side texture (RGBA8)
    static u32 s_indexTex; // The handle for the index texture (R32UI);

    static Result * s_resultsMappedPtr; // used for persistent mapping



    static bool setupShaders() {
        std::string shadersPath(g_resourcesDir + "/RLD/shaders/");
        std::string workGroupSizeStr(std::to_string(s_workGroupSize));
        std::string workGroupSize2DStr("ivec2(" + std::to_string(s_workGroupSize2D.x) + ", " + std::to_string(s_workGroupSize2D.y) + ")");
        std::vector<duo<std::string_view>> defines{
            { "WORK_GROUP_SIZE", workGroupSizeStr },
            { "WORK_GROUP_SIZE_2D", workGroupSize2DStr },
            { "DISTINGUISH_ACTIVE_PIXELS", k_distinguishActivePixels ? "true" : "false" },
            { "DO_TURBULENCE", k_doTurbulence ? "true" : "false" },
            { "DO_WIND_SHADOW", k_doWindShadow ? "true" : "false" },
            { "DO_CLOTH", s_doCloth ? "true" : "false" }
        };
        std::vector<duo<std::string_view>> debugDefines(defines);
        defines.push_back({ "DEBUG", "false" });
        debugDefines.push_back({ "DEBUG", "true" });

        // Foil Shader
        if (!(s_foilShader = Shader::load(shadersPath + "foil.vert", shadersPath + "foil.frag", defines))) {
            std::cerr << "Failed to load foil shader" << std::endl;
            return false;
        }
        if (!(s_foilShaderDebug = Shader::load(shadersPath + "foil.vert", shadersPath + "foil.frag", debugDefines))) {
            std::cerr << "Failed to load debug foil shader" << std::endl;
            return false;
        }

        // Compute Shader
        if (!(s_shader = Shader::load(shadersPath + "rld.comp", defines))) {
            std::cerr << "Failed to load rld shader" << std::endl;
            return false;
        }
        if (!(s_shaderDebug = Shader::load(shadersPath + "rld.comp", debugDefines))) {
            std::cerr << "Failed to load rld debug shader" << std::endl;
            return false;
        }

        /*
        // Prospect Compute Shader
        if (!(s_prospectShader = Shader::load(shadersPath + "prospect.comp", defines))) {
            std::cerr << "Failed to load prospect shader" << std::endl;
            return false;
        }
        if (!(s_prospectShaderDebug = Shader::load(shadersPath + "prospect.comp", debugDefines))) {
            std::cerr << "Failed to load debug prospect shader" << std::endl;
            return false;
        }

        // Outline Compute Shader
        if (!(s_outlineShader = Shader::load(shadersPath + "outline.comp", defines))) {
            std::cerr << "Failed to load outline shader" << std::endl;
            return false;
        }
        if (!(s_outlineShaderDebug = Shader::load(shadersPath + "outline.comp", debugDefines))) {
            std::cerr << "Failed to load debug outline shader" << std::endl;
            return false;
        }

        // Move Compute Shader
        if (!(s_moveShader = Shader::load(shadersPath + "move.comp", defines))) {
            std::cerr << "Failed to load move shader" << std::endl;
            return false;
        }
        if (!(s_moveShaderDebug = Shader::load(shadersPath + "move.comp", debugDefines))) {
            std::cerr << "Failed to load debug move shader" << std::endl;
            return false;
        }

        // Draw Compute Shader
        if (!(s_drawShader = Shader::load(shadersPath + "draw.comp", defines))) {
            std::cerr << "Failed to load draw shader" << std::endl;
            return false;
        }
        if (!(s_drawShaderDebug = Shader::load(shadersPath + "draw.comp", debugDefines))) {
            std::cerr << "Failed to load debug draw shader" << std::endl;
            return false;
        }
        */

        // Pretty Shader
        if (!(s_prettyShader = Shader::load(shadersPath + "pretty.comp", defines))) {
            std::cerr << "Failed to load pretty shader" << std::endl;
            return false;
        }

        // Side Shader
        if (!(s_sideShader = Shader::load(shadersPath + "side.comp"))) {
            std::cerr << "Failed to load side shader" << std::endl;
            return false;
        }
        s_sideShader->bind();
        s_sideShader->uniform("u_texSize", s_texSize);
        Shader::unbind();

        return true;
    }

    static bool setupFramebuffer() {
        float emptyColor[4]{};

        // Color texture
        glGenTextures(1, &s_frontTex_unorm);
        glBindTexture(GL_TEXTURE_2D, s_frontTex_unorm);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, s_texSize, s_texSize);
        glBindTexture(GL_TEXTURE_2D, 0);
        // Color texture view
        glGenTextures(1, &s_frontTex_uint);
        glTextureView(s_frontTex_uint, GL_TEXTURE_2D, s_frontTex_unorm, GL_RGBA8UI, 0, 1, 0, 1);

        // Turbulence texture
        glGenTextures(1, &s_turbTex);
        glBindTexture(GL_TEXTURE_2D, s_turbTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, s_texSize / 4, s_texSize / 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Previous turbulence texture
        glGenTextures(1, &s_prevTurbTex);
        glBindTexture(GL_TEXTURE_2D, s_prevTurbTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, s_texSize / 4, s_texSize / 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Wind shadow texture
        glGenTextures(1, &s_shadTex);
        glBindTexture(GL_TEXTURE_2D, s_shadTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, s_texSize / 4, s_texSize / 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Side texture
        glGenTextures(1, &s_sideTex);
        glBindTexture(GL_TEXTURE_2D, s_sideTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, s_texSize, s_texSize);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Normal texture
        glGenTextures(1, &s_fboNormTex);
        glBindTexture(GL_TEXTURE_2D, s_fboNormTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16_SNORM, s_texSize, s_texSize);

        // Index texture
        if (s_doCloth) {
            glGenTextures(1, &s_indexTex);
            glBindTexture(GL_TEXTURE_2D, s_indexTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, s_texSize, s_texSize);
        }

        // Depth render buffer
        u32 fboDepthRB(0);
        glGenRenderbuffers(1, &fboDepthRB);
        glBindRenderbuffer(GL_RENDERBUFFER, fboDepthRB);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, s_texSize, s_texSize);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // Create FBO
        glGenFramebuffers(1, &s_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_frontTex_uint, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, s_fboNormTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fboDepthRB);
        if (s_doCloth) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, s_indexTex, 0);
            u32 drawBuffers[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
            glDrawBuffers(3, drawBuffers);
        }
        else {
            u32 drawBuffers[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, drawBuffers);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer is incomplete" << std::endl;
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        return true;
    }

    static void compute() {
        Shader & shader(s_debug ? *s_shaderDebug : *s_shader);
        shader.bind();

        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }

    /*
    static void computeProspect() {
        Shader & prospectShader(s_debug ? *s_prospectShaderDebug : *s_prospectShader);
        prospectShader.bind();

        //glDispatchCompute((s_texSize + 7) / 8, (s_texSize + 7) / 8, 1); // Must also tweak in shader
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }

    static void computeOutline() {
        Shader & outlineShader(s_debug ? *s_outlineShaderDebug : *s_outlineShader);
        outlineShader.bind();

        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }

    static void computeMove() {
        Shader & moveShader(s_debug ? *s_moveShaderDebug : *s_moveShader);
        moveShader.bind();

        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }

    static void computeDraw() {
        Shader & drawShader(s_debug ? *s_drawShaderDebug : *s_drawShader);
        drawShader.bind();

        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }
    */

    static void computePretty() {
        s_prettyShader->bind();
        int n((s_texSize + 7) / 8);
        glDispatchCompute(n, n, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }

    static void computeSide() {
        glBindImageTexture(0, s_frontTex_unorm, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        s_sideShader->bind();
        int sideX(int((-s_constants.sliceZ / s_windframeWidth + 0.5f) * float(s_texSize)));
        s_sideShader->uniform("u_sideX", sideX);
        glDispatchCompute(1, (s_texSize + 7) / 8, 1); // Must match shader
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
        glBindImageTexture(0, s_frontTex_uint, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);
    }

    static void renderGeometry() {
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
        glViewport(0, 0, s_texSize, s_texSize);

        // Clear framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Shader & foilShader(s_debug ? *s_foilShaderDebug : *s_foilShader);
        foilShader.bind();

        float nearDist(s_windframeDepth * -0.5f + s_currentSlice * s_sliceSize);
        float windframeRadius(s_windframeWidth * 0.5f);
        mat4 projMat(glm::ortho(
            -windframeRadius, // left
            windframeRadius,  // right
            -windframeRadius, // bottom
            windframeRadius,  // top
            nearDist, // near
            nearDist + s_sliceSize // far
        ));
        foilShader.uniform("u_projMat", projMat);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        s_model->draw(s_modelMat, s_normalMat, foilShader.uniformLocation("u_modelMat"), foilShader.uniformLocation("u_normalMat"));
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        s_model->draw(s_modelMat, s_normalMat, foilShader.uniformLocation("u_modelMat"), foilShader.uniformLocation("u_normalMat"));

        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all

        foilShader.unbind();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    static void uploadConstants() {
        glBindBuffer(GL_UNIFORM_BUFFER, s_constantsUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Constants), &s_constants);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    static void resetCounters(bool both) {
        s32 zero(0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_geoPixelsSSBO);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(s32), &zero);
        if (s_swap == 0 || both) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airPixelsSSBO[0]);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(s32), &zero);
        }
        if (s_swap == 1 || both) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airPixelsSSBO[1]);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(s32), &zero);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    static void downloadResults() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_resultsSSBO);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, s_sliceCount * sizeof(Result), s_results.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        s_result.lift = vec3();
        s_result.drag = vec3();
        s_result.torq = vec3();
        for (const Result & result : s_results) {
            s_result.lift += result.lift;
            s_result.drag += result.drag;
            s_result.torq += result.torq;
        }
    }

    static void resetConstants() {
        s_constants.maxGeoPixels = s_maxGeoPixels;
        s_constants.maxAirPixels = s_maxAirPixels;
        s_constants.screenSize = s_texSize;
        s_constants.liftC = s_liftC;
        s_constants.dragC = s_dragC;
        s_constants.windframeSize = s_windframeWidth;
        s_constants.windframeDepth = s_windframeDepth;
        s_constants.sliceSize = s_sliceSize;
        s_constants.turbulenceDist = s_turbulenceDist;
        s_constants.maxSearchDist = s_maxSearchDist;
        s_constants.windShadDist = s_windShadDist;
        s_constants.backforceC = s_backforceC;
        s_constants.flowback = s_flowback;
        s_constants.initVelC = s_initVelC;
        s_constants.windSpeed = s_windSpeed;
        s_constants.dt = s_dt;
        s_constants.slice = 0;
        s_constants.sliceZ = s_windframeDepth * -0.5f;
        s_constants.pixelSize = s_windframeWidth / float(s_texSize);
    }

    static void clearTurbTex() {
        u08 clearVal[4]{};
        glClearTexImage(s_turbTex, 0, GL_RED, GL_UNSIGNED_BYTE, clearVal);
    }

    static void clearShadTex() {
        u08 clearVal[4]{};
        glClearTexImage(s_shadTex, 0, GL_RED, GL_UNSIGNED_BYTE, clearVal);
    }

    static void clearFlagTex() {
        s32 clearVal(0);
        glClearTexImage(s_flagTex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clearVal);
    }

    static void clearSideTex() {
        u08 clearVal[4]{};
        glClearTexImage(s_sideTex, 0, GL_RGBA, GL_UNSIGNED_BYTE, clearVal);
    }

    static void setBindings() {
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, s_constantsUBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_geoPixelsSSBO);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_airPixelsSSBO[s_swap]);       // done in step()
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_airPixelsSSBO[1 - s_swap]);   // done in step()
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, s_airGeoMapSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, s_resultsSSBO);
        if (s_doCloth) {
            const SoftMesh & softMesh(static_cast<const SoftMesh &>(s_model->subModels().front().mesh()));
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, softMesh.vertexBuffer());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, softMesh.indexBuffer());
        }

        glBindImageTexture(0,  s_frontTex_uint, 0, GL_FALSE, 0, GL_READ_WRITE,      GL_RGBA8UI);
        glBindImageTexture(1,     s_fboNormTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16_SNORM);
        glBindImageTexture(2,        s_flagTex, 0, GL_FALSE, 0, GL_READ_WRITE,         GL_R32I);
        glBindImageTexture(3,        s_turbTex, 0, GL_FALSE, 0, GL_READ_WRITE,           GL_R8);
        glBindImageTexture(4,    s_prevTurbTex, 0, GL_FALSE, 0, GL_READ_WRITE,           GL_R8);
        glBindImageTexture(5,        s_shadTex, 0, GL_FALSE, 0, GL_READ_WRITE,           GL_R8);
        if (s_doCloth) {
            glBindImageTexture(6,       s_indexTex, 0, GL_FALSE, 0, GL_READ_WRITE,        GL_R32UI);
        }
        glBindImageTexture(7,        s_sideTex, 0, GL_FALSE, 0, GL_READ_WRITE,        GL_RGBA8);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_turbTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, s_prevTurbTex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, s_shadTex);
    }



    bool setup(
        const int texSize,
        int sliceCount,
        float liftC,
        float dragC,
        float turbulenceDist,
        float maxSearchDist,
        float windShadDist,
        float backforceC,
        float flowback,
        float initVelC,
        bool doSide,
        bool doCloth
    ) {
        int coreCount(0);
        if (k_useAllCores) glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &coreCount);
        else coreCount = k_coresToUse;
        int warpCount(coreCount / k_warpSize);
        //s_workGroupSize = warpCount * k_warpSize; // we want workgroups to be a warp multiple
        s_workGroupSize2D.x = int(std::round(std::sqrt(float(warpCount))));
        s_workGroupSize2D.y = warpCount / s_workGroupSize2D.x;
        s_workGroupSize2D *= k_warpSize2D;
        s_workGroupSize = s_workGroupSize2D.x * s_workGroupSize2D.y; // match 1d group size to 2d so can use interchangeably in shaders

        s_texSize = texSize;
        s_maxGeoPixels = s_texSize * s_texSize / k_maxPixelsDivisor;
        s_maxAirPixels = s_maxGeoPixels;
        s_sliceCount = sliceCount;
        s_liftC = liftC;
        s_dragC = dragC;
        setVariables(
            turbulenceDist,
            maxSearchDist,
            windShadDist,
            backforceC,
            flowback,
            initVelC
        );
        s_doSide = doSide;
        s_doCloth = doCloth;

        s_results.resize(s_sliceCount);

        // Setup shaders
        if (!setupShaders()) {
            std::cerr << "Failed to setup shaders" << std::endl;
            return false;
        }

        // Setup constants UBO
        glGenBuffers(1, &s_constantsUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, s_constantsUBO);
        glBufferStorage(GL_UNIFORM_BUFFER, sizeof(Constants), nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Setup results SSBO
        glGenBuffers(1, &s_resultsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_resultsSSBO);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, s_sliceCount * sizeof(Result), nullptr, GL_MAP_READ_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Setup geometry pixels SSBO
        glGenBuffers(1, &s_geoPixelsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_geoPixelsSSBO);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(GeoPixelsPrefix) + s_maxGeoPixels * sizeof(GeoPixel), nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Setup air pixels SSBO
        glGenBuffers(2, s_airPixelsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airPixelsSSBO[0]);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(AirPixelsPrefix) + s_maxAirPixels * sizeof(AirPixel), nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airPixelsSSBO[1]);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(AirPixelsPrefix) + s_maxAirPixels * sizeof(AirPixel), nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Setup air geo map SSBO
        glGenBuffers(1, &s_airGeoMapSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airGeoMapSSBO);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, s_maxAirPixels * sizeof(int), nullptr, 0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        float emptyColor[4]{};
        u32 clearVal(0);

        // Setup flag texture
        glGenTextures(1, &s_flagTex);
        glBindTexture(GL_TEXTURE_2D, s_flagTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, s_texSize, s_texSize);
        glClearTexImage(s_flagTex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clearVal);

        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        // Setup framebuffer
        if (!setupFramebuffer()) {
            std::cerr << "Failed to setup framebuffer" << std::endl;
            return false;
        }

        return true;
    }

    void setVariables(float turbulenceDist, float maxSearchDist, float windShadDist, float backforceC, float flowback, float initVelC) {
        s_turbulenceDist = turbulenceDist;
        s_maxSearchDist = maxSearchDist;
        s_windShadDist = windShadDist;
        s_backforceC = backforceC;
        s_flowback = flowback;
        s_initVelC = initVelC;
    }

    void cleanup() {
        // TODO
    }

    void set(
        const Model & model,
        const mat4 & modelMat,
        const mat3 & normalMat,
        float windframeWidth,
        float windframeDepth,
        float windSpeed,
        bool debug
    ) {
        s_model = &model;
        s_modelMat = modelMat;
        s_normalMat = normalMat;
        s_windframeWidth = windframeWidth;
        s_windframeDepth = windframeDepth;
        s_sliceSize = s_windframeDepth / s_sliceCount;
        s_windSpeed = windSpeed;
        s_dt = s_sliceSize / s_windSpeed;
        s_debug = debug;
    }

    bool step(bool isExternalCall) {
        // Reset for new sweep
        if (s_currentSlice == 0) {
            resetConstants();
            resetCounters(true);
            clearTurbTex();
            clearShadTex();
            if (s_debug && s_doSide) clearSideTex();
            s_lift = vec3();
            s_drag = vec3();
            s_torq = vec3();
            s_swap = 1;
        }

        s_swap = 1 - s_swap;

        s_constants.slice = s_currentSlice;
        s_constants.sliceZ = s_windframeDepth * 0.5f - s_currentSlice * s_sliceSize;
        uploadConstants();
        resetCounters(false);

        if (isExternalCall) setBindings();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_airPixelsSSBO[s_swap]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_airPixelsSSBO[1 - s_swap]);

        clearFlagTex();
        renderGeometry(); // Render geometry to fbo
        compute();
        //computeProspect(); // Scan fbo and generate geo pixels
        //computeDraw(); // Draw any existing air pixels to the fbo and save their indices in the flag texture
        //computeOutline(); // Map air pixels to geometry, and generate new air pixels and draw them to the fbo
        //computeMove(); // Calculate lift/drag and move any existing air pixels in relation to the geometry
        if (s_debug) computePretty(); // transforms the contents of the fbo, turb, and shad textures into a comprehensible front and side view
        if (s_debug && s_doSide) computeSide();
        Shader::unbind();

        ++s_currentSlice;

        // Was last slice
        if (s_currentSlice >= s_sliceCount) {
            if (!s_doCloth) downloadResults();

            s_currentSlice = 0;
            return true;
        }

        return false;
    }

    void sweep() {
        s_currentSlice = 0;
        setBindings();
        while (!step(false));
    }

    void reset() {
        s_currentSlice = 0;
    }

    int slice() {
        return s_currentSlice;
    }

    int sliceCount() {
        return s_sliceCount;
    }

    const Result & result() {
        return s_result;
    }

    const std::vector<Result> & results() {
        return s_results;
    }

    u32 frontTex() {
        return s_frontTex_unorm;
    }

    u32 sideTex() {
        return s_sideTex;
    }

    u32 turbulenceTex() {
        return s_turbTex;
    }

    int texSize() {
        return s_texSize;
    }


}