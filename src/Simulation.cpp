#include "Simulation.hpp"

#include <memory>
#include <iostream>

#include "glad/glad.h"
#include "Program.h"
#include "Shape.h"
#include "GLSL.h"
#include "glm/gtc/matrix_transform.hpp"

#include "Util.hpp"



namespace Simulation {



    static constexpr int k_width(720), k_height(480);
    static constexpr int k_nSlices(50);
    static constexpr float k_sliceSize(0.025f); // z distance between slices, MUST ALSO CHANGE IN MOVE SHADER!!!

    static constexpr int k_maxGeoPixels = 32768; // 1 MB worth, must also change in shaders
    static constexpr int k_maxAirPixels = 32768; // 1 MB worth, must also change in shaders



    struct GeoPixel {
        vec4 worldPos;
        vec4 normal;
    };

    struct AirPixel {
        vec4 worldPos;
        vec4 velocity;
    };

    struct SSBO {
        s32 swap;
        s32 geoCount;
        s32 airCount[2];
        ivec2 screenSize;
        vec2 screenAspectFactor;
        ivec4 momentum;
        ivec4 force;
        ivec4 dragForce;
        ivec4 dragMomentum;
    };



    static std::shared_ptr<Shape> s_shape;
    static int s_currentSlice(0); // slice index [0, k_nSlices)
    static float s_angleOfAttack(0.0f); // IN DEGREES
    static vec3 s_sweepLift; // accumulating lift force for entire sweep
    static float s_sweepDrag; // accumulating drag force for entire sweep
    static int s_swap;

    static std::shared_ptr<Program> s_foilProg, s_fbProg;

    static uint s_screenVAO;
    static uint s_screenVBO;
    static uint s_sideVBO;

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
        std::cout << "Loading shader: " << compPath << std::endl;

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
            std::cout << "Failed to compile" << std::endl;
            return 0;
        }

        uint progId(glCreateProgram());
        glAttachShader(progId, shaderId);
        glLinkProgram(progId);
        glGetProgramiv(progId, GL_LINK_STATUS, &rc);
        if (!rc) {
            GLSL::printProgramInfoLog(progId);
            std::cout << "Failed to link" << std::endl;
            return 0;
        }

        if (glGetError()) {
            std::cout << "OpenGL error" << std::endl;
            return 0;
        }

        return progId;
    }

    static bool setupShaders(const std::string & resourcesDir) {
        std::string shadersDir(resourcesDir + "/shaders");

        // Foil Shader
        s_foilProg = std::make_shared<Program>();
        s_foilProg->setVerbose(true);
        s_foilProg->setShaderNames(shadersDir + "/foil.vert.glsl", shadersDir + "/foil.frag.glsl");
        if (!s_foilProg->init()) {
            std::cerr << "Failed to initialize foil shader" << std::endl;
            return false;
        }
        s_foilProg->addUniform("u_projMat");
        s_foilProg->addUniform("u_viewMat");
        s_foilProg->addUniform("u_modelMat");
        s_foilProg->addUniform("u_normMat");

        // FB Shader
        s_fbProg = std::make_shared<Program>();
        s_fbProg->setVerbose(true);
        s_fbProg->setShaderNames(shadersDir + "/fb.vert.glsl", shadersDir + "/fb.frag.glsl");
        if (!s_fbProg->init()) {
            std::cerr << "Failed to initialize fb shader" << std::endl;
            return false;
        }
        s_fbProg->addUniform("u_tex");
        glUseProgram(s_fbProg->pid);
        glUniform1i(s_fbProg->getUniform("u_tex"), 0);
        glUseProgram(0);


        // Prospect Compute Shader
        if (!(prospectProg = loadShader(shadersDir + "/sim_prospect.comp.glsl"))) {
            std::cerr << "Failed to load prospect shader" << std::endl;
            return false;
        }

        // Outline Compute Shader
        if (!(outlineProg = loadShader(shadersDir + "/sim_outline.comp.glsl"))) {
            std::cerr << "Failed to load outline shader" << std::endl;
            return false;
        }
    
        // Move Compute Shader
        if (!(moveProg = loadShader(shadersDir + "/sim_move.comp.glsl"))) {
            std::cerr << "Failed to load move shader" << std::endl;
            return false;
        }
    
        // Draw Compute Shader
        if (!(drawProg = loadShader(shadersDir + "/sim_draw.comp.glsl"))) {
            std::cerr << "Failed to load draw shader" << std::endl;
            return false;
        }

        return true;
    }


    static bool setupGeom(const std::string & resourcesDir) {
        std::string objsDir = resourcesDir + "/objs";
        // Initialize mesh.
        s_shape = std::make_shared<Shape>();
        s_shape->loadMesh(objsDir + "/0012.obj");
        //shape->resize();
        s_shape->init();

        glGenBuffers(1, &s_sideVBO);

        glGenBuffers(1, &s_screenVBO);
        glBindBuffer(GL_ARRAY_BUFFER, s_screenVBO);

        float vertData[]{
            -1.0f, -1.0f,
            1.0f, -1.0f,
            1.0f,  1.0f,

            1.0f,  1.0f,
            -1.0f,  1.0f,
            -1.0f, -1.0f
        };
        glBufferData(GL_ARRAY_BUFFER, 2 * 3 * 2 * sizeof(float), vertData, GL_STATIC_DRAW);

        glGenVertexArrays(1, &s_screenVAO);
        glBindVertexArray(s_screenVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBindVertexArray(0);

        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        return true;
    }

    static bool setupFramebuffer() {
        //sideview texture
        glGenTextures(1, &s_sideTex);
        glBindTexture(GL_TEXTURE_2D, s_sideTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // NULL means reserve texture memory, but texels are undefined
        // Tell OpenGL to reserve level 0
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, k_width, k_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        // You must reserve memory for other mipmaps levels as well either by making a series of calls to
        // glTexImage2D or use glGenerateMipmapEXT(GL_TEXTURE_2D).
        // Here, we'll use :
        glGenerateMipmap(GL_TEXTURE_2D);


        // Color texture
        glGenTextures(1, &s_fboTex);
        glBindTexture(GL_TEXTURE_2D, s_fboTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, k_width, k_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Position texture
        glGenTextures(1, &s_fboPosTex);
        glBindTexture(GL_TEXTURE_2D, s_fboPosTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, k_width, k_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Normal texture
        glGenTextures(1, &s_fboNormTex);
        glBindTexture(GL_TEXTURE_2D, s_fboNormTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, k_width, k_height, 0, GL_RGBA, GL_FLOAT, nullptr);

        // Depth render buffer
        uint fboDepthRB(0);
        glGenRenderbuffers(1, &fboDepthRB);
        glBindRenderbuffer(GL_RENDERBUFFER, fboDepthRB);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, k_width, k_height);
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

        glDispatchCompute((k_width + 7) / 8, (k_height + 7) / 8, 1); // Must also tweak in shader
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all     
    }

    static void computeOutline() {
        glUseProgram(outlineProg);

        glDispatchCompute(1024, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }

    static void computeMove() {
        glUseProgram(moveProg);

        glDispatchCompute(1024, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all  
    }

    static void computeDraw() {
        glUseProgram(drawProg);
    
        glDispatchCompute(1024, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all
    }

    static void renderGeometry() {
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);

        uint drawBuffers[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, drawBuffers); // TODO: can this be moved to fbo setup?

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        s_foilProg->bind();

        float zNear(k_sliceSize * (s_currentSlice));
        mat4 projMat(glm::ortho(
            -1.0f / s_ssboLocal.screenAspectFactor.x, // left
             1.0f / s_ssboLocal.screenAspectFactor.x, // right
            -1.0f / s_ssboLocal.screenAspectFactor.y, // bottom
             1.0f / s_ssboLocal.screenAspectFactor.y, // top
            zNear, // near
            zNear + k_sliceSize // far
        ));        
        glUniformMatrix4fv(s_foilProg->getUniform("u_projMat"), 1, GL_FALSE, reinterpret_cast<const float *>(&projMat));

        mat4 modelMat(glm::rotate(mat4(1.0f), glm::radians(-s_angleOfAttack), vec3(1.0f, 0.0f, 0.0f)));        
        glUniformMatrix4fv(s_foilProg->getUniform("u_modelMat"), 1, GL_FALSE, reinterpret_cast<const float *>(&modelMat));

        mat3 normMat(glm::transpose(glm::inverse(modelMat)));
        glUniformMatrix3fv(s_foilProg->getUniform("u_normMat"), 1, GL_FALSE, reinterpret_cast<const float *>(&normMat));

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        s_shape->draw(s_foilProg, false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        s_shape->draw(s_foilProg, false);
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: don't need all

        s_foilProg->unbind();

        /*
        glViewport(width / 2, height / 4, width / 2, height / 2);
        V = glm::rotate(mat4(1),0.23f, vec3(0, 1, 0));
        progfoil->bind();
        glUniformMatrix4fv(progfoil->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        shape->draw(progfoil, false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        shape->draw(progfoil, false);
        progfoil->unbind();
        */

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glBindTexture(GL_TEXTURE_2D, FBOtex);
        //glGenerateMipmap(GL_TEXTURE_2D);
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

    static void clearFlagTex() {
        int clearVal(0);
        glClearTexImage(s_flagTex, 0, GL_RED_INTEGER, GL_INT, &clearVal);
    }

    static void clearSideTex() {
        u08 clearVal[4]{};
        glClearTexImage(s_sideTex, 0, GL_RGBA, GL_UNSIGNED_BYTE, &clearVal);
    }



    bool setup(const std::string & resourceDir) {
        glViewport(0, 0, k_width, k_height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);

        // Setup shaders
        if (!setupShaders(resourceDir)) {
            std::cerr << "Failed to setup shaders" << std::endl;
            return false;
        }

        // Setup geometry
        if (!setupGeom(resourceDir)) {
            std::cerr << "Failed to setup geometry" << std::endl;
            return false;
        }

        // Setup SSBO
        glGenBuffers(1, &s_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_ssbo);
        s_ssboLocal.screenSize.x = k_width;
        s_ssboLocal.screenSize.y = k_height;
        if (k_width >= k_height) {
            s_ssboLocal.screenAspectFactor.x = float(k_height) / float(k_width);
            s_ssboLocal.screenAspectFactor.y = 1.0f;
        }
        else {
            s_ssboLocal.screenAspectFactor.x = 1.0f;
            s_ssboLocal.screenAspectFactor.y = float(k_width) / float(k_height);
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SSBO), &s_ssboLocal, GL_DYNAMIC_COPY);
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
        glBufferData(GL_SHADER_STORAGE_BUFFER, k_maxAirPixels * sizeof(s32), nullptr, GL_DYNAMIC_COPY);
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, k_width, k_height, 0, GL_RED_INTEGER, GL_INT, nullptr);
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

    bool step() {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_geoPixelsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_airPixelsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, s_airGeoMapSSBO);

        glBindImageTexture(0,     s_fboTex, 0, GL_FALSE, 0, GL_READ_WRITE,   GL_RGBA8);
        glBindImageTexture(1,  s_fboPosTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(2, s_fboNormTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(3,    s_flagTex, 0, GL_FALSE, 0, GL_READ_WRITE,    GL_R32I);
        glBindImageTexture(4,    s_sideTex, 0, GL_FALSE, 0, GL_READ_WRITE,   GL_RGBA8);

        // Reset for new sweep
        if (s_currentSlice == 0) {
            clearSideTex();
            s_ssboLocal.airCount[1] = 0;
            s_ssboLocal.force = ivec4();
            s_ssboLocal.momentum = ivec4();
            s_ssboLocal.dragForce = ivec4();
            s_ssboLocal.dragMomentum = ivec4();
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
        s_sweepLift += vec3(s_ssboLocal.force) * 1.0e-6f;
        s_sweepDrag += float(s_ssboLocal.dragForce.x) * 1.0e-6f;

        if (++s_currentSlice >= k_nSlices) {
            s_currentSlice = 0;
            return true;
        }

        return false;
    }

    void render() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        s_fbProg->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_fboTex);
        glBindVertexArray(s_screenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        s_fbProg->unbind();
    }

    int getSlice() {
        return s_currentSlice;
    }

    int getNSlices() {
        return k_nSlices;
    }

    float getAngleOfAttack() {
        return s_angleOfAttack;
    }

    void setAngleOfAttack(float angle) {
        s_angleOfAttack = angle;
    }

    vec3 getLift() {
        return s_sweepLift;
    }

    vec3 getDrag() {
        return vec3(s_sweepDrag);
    }

    uint getSideTextureID() {
        return s_sideTex;
    }

    ivec2 getSize() {
        return ivec2(k_width, k_height);
    }


}