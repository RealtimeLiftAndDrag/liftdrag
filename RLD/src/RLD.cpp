#include "RLD.hpp"

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
    static constexpr int k_maxPixelsDivisor(16); // Max geo or air pixels is total pixels in texture divided by this
    static constexpr bool k_distinguishActivePixels(true); // In debug mode, makes certain "active" pixels brigher for visual clarity, but lowers performance
    static constexpr bool k_doTurbulence(false);
    static constexpr bool k_doWindShadow(true);



    // Mirrors GPU struct
    struct GeoPixel {
        vec2 windPos;
        ivec2 texCoord;
        vec3 normal;
        int edge;
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
        s32 texSize;
        float liftC;
        float dragC;
        float windframeSize; // width and height of the windframe
        float windframeDepth; // depth of the windframe
        float sliceSize; // Distance between slices in wind space
        float turbulenceDist; // Distance at which turbulence will start (in wind space)
        float maxSearchDist; // Max distance for the shader searches (in wind space)
        float windShadDist; // How far back the wind shadow will stretch (in wind space)
        float backforceC;
        float flowback; // Fraction of air's lateral velocity to remain after 1 unit of distance
        float initVelC;
        float windSpeed;
        float dt; // The time it would take to travel `s_sliceSize` at `s_windSpeed`
        s32 slice; // Current slice
        float sliceZ; // Current slice's wind z
        float pixelSize; // Pixel width or height in wind space
    };

    // Mirrors GPU struct
    struct GeoPixelsPrefix {
        s32 geoCount;
        s32 _0;
        s32 _1;
        s32 _2;
    };

    // Mirrors GPU struct
    struct AirPixelsPrefix {
        s32 airCount;
        s32 _0;
        s32 _1;
        s32 _2;
    };



    static int s_workGroupSize; // At least 1024
    static ivec2 s_workGroupSize2D; // At least 32x32
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
    static bool s_doSide; // Whether to render side texture
    static bool s_doCloth; // Whether this is a cloth simulation

    static int s_currentSlice(0); // slice index [0, sliceCount)
    static std::vector<Result> s_results; // Results for each slice
    static Result s_result; // Cumulative result of all slices
    static int s_swap; // Only used for the air pixel buffer

    static unq<Shader> s_foilShader, s_foilShaderDebug;
    static unq<Shader> s_prospectShader, s_prospectShaderDebug;
    static unq<Shader> s_drawShader, s_drawShaderDebug;
    static unq<Shader> s_outlineShader, s_outlineShaderDebug;
    static unq<Shader> s_moveShader, s_moveShaderDebug;
    static unq<Shader> s_prettyShader, s_sideShader;

    static Constants s_constants; // CPU copy of constants

    static u32 s_constantsBuffer;
    static u32 s_resultsBuffer;
    static u32 s_geoPixelsBuffer;
    static u32 s_airPixelsBuffer[2];
    static u32 s_airGeoMapBuffer;

    static u32 s_fbo; // The handle for the framebuffer object
    static u32 s_frontTex_unorm; // A view of s_fboTex that is RGBA8
    static u32 s_frontTex_uint; // The handle for the front texture (RGBA8UI)
    static u32 s_normTex; // The handle for the normal texture (RGBA16_SNORM)
    static u32 s_flagTex; // The handle for the flag texture (R32UI)
    static u32 s_turbTex; // The handle for the turbulence texture (R8)
    static u32 s_prevTurbTex; // The handle for the previous turbulence texture (R8)
    static u32 s_shadTex; // The handle for the wind shadow texture (R8)
    static u32 s_indexTex; // The handle for the index texture (R32UI);
    static u32 s_sideTex; // The handle for the side texture (RGBA8)



    static bool setupShaders() {
        std::string shadersPath(g_resourcesDir + "/RLD/shaders/");
        // External shader defines
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

        // Foil shader
        if (!(s_foilShader = Shader::load(shadersPath + "foil.vert", shadersPath + "foil.frag", defines))) {
            std::cerr << "Failed to load foil shader" << std::endl;
            return false;
        }
        if (!(s_foilShaderDebug = Shader::load(shadersPath + "foil.vert", shadersPath + "foil.frag", debugDefines))) {
            std::cerr << "Failed to load debug foil shader" << std::endl;
            return false;
        }

        // Prospect shader
        if (!(s_prospectShader = Shader::load(shadersPath + "prospect.comp", defines))) {
            std::cerr << "Failed to load prospect shader" << std::endl;
            return false;
        }
        if (!(s_prospectShaderDebug = Shader::load(shadersPath + "prospect.comp", debugDefines))) {
            std::cerr << "Failed to load debug prospect shader" << std::endl;
            return false;
        }

        // Draw Compute shader
        if (!(s_drawShader = Shader::load(shadersPath + "draw.comp", defines))) {
            std::cerr << "Failed to load draw shader" << std::endl;
            return false;
        }
        if (!(s_drawShaderDebug = Shader::load(shadersPath + "draw.comp", debugDefines))) {
            std::cerr << "Failed to load debug draw shader" << std::endl;
            return false;
        }

        // Outline compute shader
        if (!(s_outlineShader = Shader::load(shadersPath + "outline.comp", defines))) {
            std::cerr << "Failed to load outline shader" << std::endl;
            return false;
        }
        if (!(s_outlineShaderDebug = Shader::load(shadersPath + "outline.comp", debugDefines))) {
            std::cerr << "Failed to load debug outline shader" << std::endl;
            return false;
        }

        // Move compute shader
        if (!(s_moveShader = Shader::load(shadersPath + "move.comp", defines))) {
            std::cerr << "Failed to load move shader" << std::endl;
            return false;
        }
        if (!(s_moveShaderDebug = Shader::load(shadersPath + "move.comp", debugDefines))) {
            std::cerr << "Failed to load debug move shader" << std::endl;
            return false;
        }

        // Pretty shader
        if (!(s_prettyShader = Shader::load(shadersPath + "pretty.comp", defines))) {
            std::cerr << "Failed to load pretty shader" << std::endl;
            return false;
        }

        // Side shader
        if (!(s_sideShader = Shader::load(shadersPath + "side.comp"))) {
            std::cerr << "Failed to load side shader" << std::endl;
            return false;
        }
        s_sideShader->bind();
        s_sideShader->uniform("u_texSize", s_texSize);
        Shader::unbind();

        return true;
    }

    static bool setupBuffers() {
        // Constants buffer
        glGenBuffers(1, &s_constantsBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, s_constantsBuffer);
        glBufferStorage(GL_UNIFORM_BUFFER, sizeof(Constants), nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Results buffer
        glGenBuffers(1, &s_resultsBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_resultsBuffer);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, s_sliceCount * sizeof(Result), nullptr, GL_MAP_READ_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Geometry pixels buffer
        glGenBuffers(1, &s_geoPixelsBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_geoPixelsBuffer);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(GeoPixelsPrefix) + s_maxGeoPixels * sizeof(GeoPixel), nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Air pixels buffer
        glGenBuffers(2, s_airPixelsBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airPixelsBuffer[0]);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(AirPixelsPrefix) + s_maxAirPixels * sizeof(AirPixel), nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airPixelsBuffer[1]);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(AirPixelsPrefix) + s_maxAirPixels * sizeof(AirPixel), nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Air geo map buffer
        glGenBuffers(1, &s_airGeoMapBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airGeoMapBuffer);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, s_maxAirPixels * sizeof(int), nullptr, 0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        return true;
    }

    static bool setupTextures() {
        float emptyVal[4]{};

        // Front texture
        glGenTextures(1, &s_frontTex_unorm);
        glBindTexture(GL_TEXTURE_2D, s_frontTex_unorm);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyVal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, s_texSize, s_texSize);
        glBindTexture(GL_TEXTURE_2D, 0);
        // Front texture view
        glGenTextures(1, &s_frontTex_uint);
        glTextureView(s_frontTex_uint, GL_TEXTURE_2D, s_frontTex_unorm, GL_RGBA8UI, 0, 1, 0, 1);

        // Normal texture
        glGenTextures(1, &s_normTex);
        glBindTexture(GL_TEXTURE_2D, s_normTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyVal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16_SNORM, s_texSize, s_texSize);

        // Setup flag texture
        glGenTextures(1, &s_flagTex);
        glBindTexture(GL_TEXTURE_2D, s_flagTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyVal);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, s_texSize, s_texSize);

        // Turbulence texture
        glGenTextures(1, &s_turbTex);
        glBindTexture(GL_TEXTURE_2D, s_turbTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyVal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, s_texSize / 4, s_texSize / 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Previous turbulence texture
        glGenTextures(1, &s_prevTurbTex);
        glBindTexture(GL_TEXTURE_2D, s_prevTurbTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyVal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, s_texSize / 4, s_texSize / 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Wind shadow texture
        glGenTextures(1, &s_shadTex);
        glBindTexture(GL_TEXTURE_2D, s_shadTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyVal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, s_texSize / 4, s_texSize / 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Index texture
        if (s_doCloth) {
            glGenTextures(1, &s_indexTex);
            glBindTexture(GL_TEXTURE_2D, s_indexTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyVal);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, s_texSize, s_texSize);
        }

        // Side texture
        glGenTextures(1, &s_sideTex);
        glBindTexture(GL_TEXTURE_2D, s_sideTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyVal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, s_texSize, s_texSize);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        return true;
    }

    static bool setupFramebuffer() {
        // Depth render buffer
        u32 rb(0);
        glGenRenderbuffers(1, &rb);
        glBindRenderbuffer(GL_RENDERBUFFER, rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, s_texSize, s_texSize);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // Create FBO
        glGenFramebuffers(1, &s_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_frontTex_uint, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, s_normTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);
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

    static void computeProspect() {
        Shader & prospectShader(s_debug ? *s_prospectShaderDebug : *s_prospectShader);
        prospectShader.bind();

        // 8x8 work groups
        glDispatchCompute((s_texSize + 7) / 8, (s_texSize + 7) / 8, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }

    static void computeDraw() {
        Shader & drawShader(s_debug ? *s_drawShaderDebug : *s_drawShader);
        drawShader.bind();
    
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
        glBindBuffer(GL_UNIFORM_BUFFER, s_constantsBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Constants), &s_constants);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    static void resetCounters(bool both) {
        s32 zero(0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_geoPixelsBuffer);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(s32), &zero);
        if (s_swap == 0 || both) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airPixelsBuffer[0]);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(s32), &zero);
        }
        if (s_swap == 1 || both) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airPixelsBuffer[1]);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(s32), &zero);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    static void clearResults() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_resultsBuffer);
        vec4 zero;
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, &zero);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    static void downloadResults() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_resultsBuffer);
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
        s_constants.texSize = s_texSize;
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
        vec4 clearVal;
        glClearTexImage(s_turbTex, 0, GL_RED, GL_UNSIGNED_BYTE, &clearVal);
    }

    static void clearShadTex() {
        vec4 clearVal;
        glClearTexImage(s_shadTex, 0, GL_RED, GL_UNSIGNED_BYTE, &clearVal);
    }

    static void clearFlagTex() {
        vec4 clearVal;
        glClearTexImage(s_flagTex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clearVal);
    }

    static void clearSideTex() {
        vec4 clearVal;
        glClearTexImage(s_sideTex, 0, GL_RGBA, GL_UNSIGNED_BYTE, &clearVal);
    }

    static void setBindings() {
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, s_constantsBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_geoPixelsBuffer);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_airPixelsBuffer[s_swap]);       // done in step()
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_airPixelsBuffer[1 - s_swap]);   // done in step()
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, s_airGeoMapBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, s_resultsBuffer);
        if (s_doCloth) {
            const SoftMesh & softMesh(static_cast<const SoftMesh &>(s_model->subModels().front().mesh()));
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, softMesh.vertexBuffer());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, softMesh.indexBuffer());
        }

        glBindImageTexture(0,  s_frontTex_uint, 0, GL_FALSE, 0, GL_READ_WRITE,      GL_RGBA8UI);
        glBindImageTexture(1,        s_normTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16_SNORM);
        glBindImageTexture(2,        s_flagTex, 0, GL_FALSE, 0, GL_READ_WRITE,         GL_R32I);
        glBindImageTexture(3,        s_turbTex, 0, GL_FALSE, 0, GL_READ_WRITE,           GL_R8);
        glBindImageTexture(4,    s_prevTurbTex, 0, GL_FALSE, 0, GL_READ_WRITE,           GL_R8);
        glBindImageTexture(5,        s_shadTex, 0, GL_FALSE, 0, GL_READ_WRITE,           GL_R8);
        if (s_doCloth) {
            glBindImageTexture(6,   s_indexTex, 0, GL_FALSE, 0, GL_READ_WRITE,        GL_R32UI);
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
        s_workGroupSize = warpCount * k_warpSize; // we want workgroups to be a warp multiple
        // If doing 2D operations within 1D compute shaders, s_workGroupSize must instead defines as such
        //s_workGroupSize2D.x = int(std::round(std::sqrt(float(warpCount))));
        //s_workGroupSize2D.y = warpCount / s_workGroupSize2D.x;
        //s_workGroupSize2D *= k_warpSize2D;
        //s_workGroupSize = s_workGroupSize2D.x * s_workGroupSize2D.y; // match 1d group size to 2d so can use interchangeably in shaders

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

        // Setup buffers
        if (!setupBuffers()) {
            std::cerr << "Failed to setup buffers" << std::endl;
            return false;
        }

        // Setup textures
        if (!setupTextures()) {
            std::cerr << "Failed to setup textures" << std::endl;
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
            clearResults();
            if (s_debug && s_doSide) clearSideTex();
            s_swap = 1;
        }

        s_swap = 1 - s_swap;

        s_constants.slice = s_currentSlice;
        s_constants.sliceZ = s_windframeDepth * 0.5f - s_currentSlice * s_sliceSize;
        uploadConstants();
        resetCounters(false);

        if (isExternalCall) setBindings();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_airPixelsBuffer[s_swap]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_airPixelsBuffer[1 - s_swap]);

        clearFlagTex();
        renderGeometry(); // Render geometry to fbo
        computeProspect(); // Scan fbo and generate geo pixels
        glCopyImageSubData(s_turbTex, GL_TEXTURE_2D, 0, 0, 0, 0, s_prevTurbTex, GL_TEXTURE_2D, 0, 0, 0, 0, s_texSize / 4, s_texSize / 4, 1); // copy turb tex to prev turb tex
        computeDraw(); // Draw any existing air pixels to the fbo and save their indices in the flag texture
        computeOutline(); // Map air pixels to geometry, and generate new air pixels and draw them to the fbo
        computeMove(); // Calculate lift/drag and move any existing air pixels in relation to the geometry
        if (s_debug) computePretty(); // transforms the contents of the fbo, turb, and shad textures into a comprehensible front and side view
        if (s_debug && s_doSide) computeSide(); // Projects the front texture onto one column of the side texture
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