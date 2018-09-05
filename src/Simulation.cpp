#include "Simulation.hpp"

#include <memory>
#include <iostream>

#include "glad/glad.h"
#include "Program.h"
#include "GLSL.h"
#include "glm/gtc/matrix_transform.hpp"

#include "Util.hpp"



namespace Simulation {

    static const int k_size(720); // Width and height of the textures, which are square
    static constexpr int k_sliceCount(100); // Should also change in `Results.cpp`
    static constexpr float k_windSpeed(10.0f); // Speed of air in -z direction

    static constexpr int k_maxPixelsDivisor(16); // max dense pixels is the total pixels divided by this
    static const int k_maxGeoPixels(k_size * k_size / k_maxPixelsDivisor);
    static const int k_maxAirPixels(k_maxGeoPixels);

    static constexpr int k_maxGeoPerAir(3); // Maximum number of different geo pixels that an air pixel can be associated with



    struct GeoPixel {
        vec3 worldPos;
        vec3 normal;
        ivec2 texCoord;
    };

    struct AirPixel {
        vec4 worldPos;
        vec4 velocity;
        vec4 backForce;
    };

    struct AirGeoMapElement {
        s32 geoCount;
        s32 geoIndices[k_maxGeoPerAir];
    };

    struct SSBO {
        s32 swap;
        s32 geoCount;
        s32 airCount[2];
        s32 maxGeoPixels;
        s32 maxAirPixels;
        s32 screenSize;
        s32 padding0;
        float sliceSize;
        float windSpeed;
        float dt;
        u32 debug;
        vec4 lift;
        vec4 drag;
    };



    static const Model * s_model;
    static mat4 s_modelMat;
    static mat3 s_normalMat;
    static float s_depth; // Depth of the simulation frame / total depth of all slices
    static vec3 s_centerOfMass; // Center of mass of the model in world space
    static float s_sliceSize; // Distance between slices in world space
    static float s_dt; // The time it would take to travel `s_sliceSize` at `s_windSpeed`
    static bool s_debug; // Whether to enable non essentials like side view or active pixel highlighting

    static int s_currentSlice(0); // slice index [0, k_nSlices)
    static float s_angleOfAttack(0.0f); // IN DEGREES
    static float s_rudderAngle(0.0f); // IN DEGREES
    static float s_elevatorAngle(0.0f); // IN DEGREES
    static float s_aileronAngle(0.0f); // IN DEGREES
    static vec3 s_sweepLift; // accumulating lift force for entire sweep
    static vec3 s_sweepDrag; // accumulating drag force for entire sweep
    static std::vector<vec3> s_sliceLifts; // Lift for each slice
    static std::vector<vec3> s_sliceDrags; // Drag for each slice
    static int s_swap;

    static shr<Program> s_foilProg;

    static SSBO s_ssboLocal;
    static uint s_ssbo;
    static uint s_geoPixelsSSBO;
    static uint s_airPixelsSSBO;
    static uint s_airGeoMapSSBO;

    static uint s_fbo;
    static uint s_fboTex;
    static uint s_fboPosTex;
    static uint s_fboNormTex;
    static uint s_flagTex;
    static uint s_sideTex;

    static uint prospectProg;
    static uint outlineProg;
    static uint moveProg;
    static uint drawProg;



    static uint loadShader(const std::string & vertPath, const std::string & fragPath) {
        // TODO
    }

    static uint loadShader(const std::string & compPath) {
        auto [res, str](Util::readTextFile(compPath));
        if (!res) {
            std::cerr << "Failed to read file: " << compPath << std::endl;
            return 0;
        }

        const char * cStr(str.c_str());
        uint shaderId(glCreateShader(GL_COMPUTE_SHADER));
        glShaderSource(shaderId, 1, &cStr, nullptr);
        CHECKED_GL_CALL(glCompileShader(shaderId));
        int rc; 
        CHECKED_GL_CALL(glGetShaderiv(shaderId, GL_COMPILE_STATUS, &rc));
        if (!rc) {
            GLSL::printShaderInfoLog(shaderId);
            std::cerr << "Failed to compile" << std::endl;
            return 0;
        }

        uint progId(glCreateProgram());
        glAttachShader(progId, shaderId);
        glLinkProgram(progId);
        glGetProgramiv(progId, GL_LINK_STATUS, &rc);
        if (!rc) {
            GLSL::printProgramInfoLog(progId);
            std::cerr << "Failed to link" << std::endl;
            return 0;
        }

        if (glGetError()) {
            std::cerr << "OpenGL error" << std::endl;
            return 0;
        }

        return progId;
    }

    static bool setupShaders(const std::string & resourcesDir) {
        std::string shadersDir(resourcesDir + "/shaders");

        // Foil Shader
        s_foilProg = std::make_shared<Program>();
        s_foilProg->setVerbose(true);
        s_foilProg->setShaderNames(shadersDir + "/foil.vert", shadersDir + "/foil.frag");
        if (!s_foilProg->init()) {
            std::cerr << "Failed to initialize foil shader" << std::endl;
            return false;
        }
        s_foilProg->addUniform("u_projMat");
        s_foilProg->addUniform("u_viewMat");
        s_foilProg->addUniform("u_modelMat");
        s_foilProg->addUniform("u_normalMat");

        // Prospect Compute Shader
        if (!(prospectProg = loadShader(shadersDir + "/sim_prospect.comp"))) {
            std::cerr << "Failed to load prospect shader" << std::endl;
            return false;
        }

        // Outline Compute Shader
        if (!(outlineProg = loadShader(shadersDir + "/sim_outline.comp"))) {
            std::cerr << "Failed to load outline shader" << std::endl;
            return false;
        }
    
        // Move Compute Shader
        if (!(moveProg = loadShader(shadersDir + "/sim_move.comp"))) {
            std::cerr << "Failed to load move shader" << std::endl;
            return false;
        }
    
        // Draw Compute Shader
        if (!(drawProg = loadShader(shadersDir + "/sim_draw.comp"))) {
            std::cerr << "Failed to load draw shader" << std::endl;
            return false;
        }

        return true;
    }

    static bool setupFramebuffer() {
        float emptyColor[4]{};

        // Color texture
        glGenTextures(1, &s_fboTex);
        glBindTexture(GL_TEXTURE_2D, s_fboTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, k_size, k_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        //sideview texture
        glGenTextures(1, &s_sideTex);
        glBindTexture(GL_TEXTURE_2D, s_sideTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, k_size, k_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Position texture
        glGenTextures(1, &s_fboPosTex);
        glBindTexture(GL_TEXTURE_2D, s_fboPosTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, k_size, k_size, 0, GL_RGBA, GL_FLOAT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Normal texture
        glGenTextures(1, &s_fboNormTex);
        glBindTexture(GL_TEXTURE_2D, s_fboNormTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, emptyColor);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, k_size, k_size, 0, GL_RGBA, GL_FLOAT, nullptr);

        // Depth render buffer
        uint fboDepthRB(0);
        glGenRenderbuffers(1, &fboDepthRB);
        glBindRenderbuffer(GL_RENDERBUFFER, fboDepthRB);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, k_size, k_size);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create FBO
        glGenFramebuffers(1, &s_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_fboTex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, s_fboPosTex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, s_fboNormTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fboDepthRB);

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
        glUseProgram(prospectProg);

        glDispatchCompute((k_size + 7) / 8, (k_size + 7) / 8, 1); // Must also tweak in shader
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all     
    }

    static void computeOutline() {
        glUseProgram(outlineProg);

        glDispatchCompute(1024, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }

    static void computeMove() {
        glUseProgram(moveProg);

        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all  
    }

    static void computeDraw() {
        glUseProgram(drawProg);
    
        glDispatchCompute(1024, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }

    static void renderGeometry() {
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
        glViewport(0, 0, k_size, k_size);

        uint drawBuffers[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, drawBuffers); // TODO: can this be moved to fbo setup?

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        s_foilProg->bind();

        float zNear(s_sliceSize * s_currentSlice);
        mat4 projMat(glm::ortho(
            -1.0f, // left
             1.0f, // right
            -1.0f, // bottom
             1.0f, // top
            zNear, // near
            zNear + s_sliceSize // far
        ));        
        glUniformMatrix4fv(s_foilProg->getUniform("u_projMat"), 1, GL_FALSE, reinterpret_cast<const float *>(&projMat));

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        s_model->draw(s_modelMat, s_normalMat, s_foilProg->getUniform("u_modelMat"), s_foilProg->getUniform("u_normalMat"));
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        s_model->draw(s_modelMat, s_normalMat, s_foilProg->getUniform("u_modelMat"), s_foilProg->getUniform("u_normalMat"));

        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all

        s_foilProg->unbind();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    static void downloadSSBO() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_ssbo);
        void * p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        std::memcpy(&s_ssboLocal, p, sizeof(SSBO));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    static void uploadSSBO() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SSBO), &s_ssboLocal, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    } 

    static void resetSSBO() {
        s_ssboLocal.swap = 0;
        s_ssboLocal.geoCount = 0;
        s_ssboLocal.airCount[0] = 0;
        s_ssboLocal.airCount[1] = 0;
        s_ssboLocal.maxGeoPixels = k_maxGeoPixels;
        s_ssboLocal.maxAirPixels = k_maxAirPixels;
        s_ssboLocal.screenSize = k_size;
        s_ssboLocal.sliceSize = s_sliceSize;
        s_ssboLocal.windSpeed = k_windSpeed;
        s_ssboLocal.dt = s_dt;
        s_ssboLocal.debug = s_debug;
        s_ssboLocal.lift = vec4();
        s_ssboLocal.drag = vec4();
    }

    static void clearFlagTex() {
        int clearVal(0);
        glClearTexImage(s_flagTex, 0, GL_RED_INTEGER, GL_INT, &clearVal);
    }

    static void clearSideTex() {
        u08 clearVal[4]{};
        glClearTexImage(s_sideTex, 0, GL_RGBA, GL_UNSIGNED_BYTE, &clearVal);
    }

    static void setBindings() {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_geoPixelsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_airPixelsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, s_airGeoMapSSBO);

        glBindImageTexture(0,     s_fboTex, 0, GL_FALSE, 0, GL_READ_WRITE,   GL_RGBA8);
        glBindImageTexture(1,  s_fboPosTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(2, s_fboNormTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(3,    s_flagTex, 0, GL_FALSE, 0, GL_READ_WRITE,    GL_R32I);
        glBindImageTexture(4,    s_sideTex, 0, GL_FALSE, 0, GL_READ_WRITE,   GL_RGBA8);    
    }



    bool setup(const std::string & resourceDir) {
        // Setup shaders
        if (!setupShaders(resourceDir)) {
            std::cerr << "Failed to setup shaders" << std::endl;
            return false;
        }

        // Setup SSBO
        glGenBuffers(1, &s_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SSBO), nullptr, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        // Setup geometry pixels SSBO
        glGenBuffers(1, &s_geoPixelsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_geoPixelsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_geoPixelsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, k_maxGeoPixels * sizeof(GeoPixel), nullptr, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        // Setup air pixels SSBO
        glGenBuffers(1, &s_airPixelsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airPixelsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_airPixelsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, k_maxAirPixels * 2 * sizeof(AirPixel), nullptr, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        // Setup air geo map SSBO
        glGenBuffers(1, &s_airGeoMapSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_airGeoMapSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_airGeoMapSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, k_maxAirPixels * sizeof(AirGeoMapElement), nullptr, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        // Setup flag texture
        glGenTextures(1, &s_flagTex);
        glBindTexture(GL_TEXTURE_2D, s_flagTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, k_size, k_size, 0, GL_RED_INTEGER, GL_INT, nullptr);
        uint clearcolor = 0;
        glClearTexImage(s_flagTex, 0, GL_RED_INTEGER, GL_INT, &clearcolor);
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

    void cleanup() {
        // TODO
    }

    void set(const Model & model, const mat4 & modelMat, const mat3 & normalMat, float depth, const vec3 & centerOfMass, bool debug) {
        s_model = &model;
        s_modelMat = modelMat;
        s_normalMat = normalMat;
        s_depth = depth;
        s_centerOfMass = centerOfMass;
        s_sliceSize = s_depth / k_sliceCount;
        s_dt = s_sliceSize / k_windSpeed;
        s_debug = debug;
    }

    bool step(bool isExternalCall) {
        if (isExternalCall) {
            setBindings();
        }

        // Reset for new sweep
        if (s_currentSlice == 0) {
            resetSSBO();
            clearSideTex();
            s_sweepLift = vec3();
            s_sweepDrag = vec3();
            s_sliceLifts.clear();
            s_sliceDrags.clear();
            s_swap = 1;
        }
        
        s_swap = 1 - s_swap;

        s_ssboLocal.swap = s_swap;
        s_ssboLocal.geoCount = 0;
        s_ssboLocal.airCount[s_swap] = 0;
        uploadSSBO();

        renderGeometry(); // Render geometry to fbo
        computeProspect(); // Scan fbo and generate geo pixels
        clearFlagTex();
        computeDraw(); // Draw any existing air pixels to the fbo and save their indices in the flag texture
        computeOutline(); // Map air pixels to geometry, and generate new air pixels and draw them to the fbo
        computeMove(); // Calculate lift/drag and move any existing air pixels in relation to the geometry

        downloadSSBO();
        vec3 lift(s_ssboLocal.lift);
        vec3 drag(s_ssboLocal.drag);
        s_sweepLift += lift;
        s_sweepDrag += drag;
        s_sliceLifts.push_back(lift);
        s_sliceDrags.push_back(drag);

        if (++s_currentSlice >= k_sliceCount) {
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

    int slice() {
        return s_currentSlice;
    }

    int sliceCount() {
        return k_sliceCount;
    }

    const vec3 & lift() {
        return s_sweepLift;
    }

    const vec3 & lift(int slice) {
        return s_sliceLifts[slice];
    }

    const vec3 & drag() {
        return s_sweepDrag;
    }

    const vec3 & drag(int slice) {
        return s_sliceDrags[slice];
    }

    uint frontTex() {
        return s_fboTex;
    }

    uint sideTex() {
        return s_sideTex;
    }

    int size() {
        return k_size;
    }


}