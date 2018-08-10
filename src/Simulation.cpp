#include "Simulation.hpp"

#include <memory>
#include <iostream>

#include "glad/glad.h"
#include "Program.h"
#include "Shape.h"
#include "GLSL.h"
#include "glm/gtc/matrix_transform.hpp"



namespace Simulation {



    static constexpr int k_width(720), k_height(480);
    static constexpr int k_nSlices(50);
    static constexpr float k_sliceSize(0.025f); // z distance between slices, MUST ALSO CHANGE IN MOVE SHADER!!!

    static constexpr int k_maxGeoPixels = 16384;
    static constexpr int k_maxGeoPixelsRows = 27;
    static constexpr int k_maxGeoPixelsSum = k_maxGeoPixels * (k_maxGeoPixelsRows / 3);

    static constexpr int k_maxAirPixels = 16384;
    static constexpr int k_maxAirPixelsRows = 27 * 2;
    static constexpr int k_maxAirPixelsSum = k_maxAirPixels * (k_maxAirPixelsRows / 2 / 3);

    static constexpr bool k_doDebug(false);
    static constexpr int k_debugSize(4096); // Must also change defines in shaders



    struct SSBO {

        s32 geoCount;
        s32 test; // necessary for padding
        s32 airCount[2];
        vec4 screenSpec; // screen width, screen height, x aspect factor, y aspect factor
        ivec4 momentum;
        ivec4 force;
        ivec4 debugShit[k_debugSize];

        SSBO() :
            geoCount(0),
            test(0),
            airCount{ 0, 0 },
            screenSpec(),
            momentum(),
            force()
        {
            if (k_doDebug) std::memset(debugShit, 0, k_debugSize);
        }

        void reset(int swap) {
            geoCount = 0;
            test = 0;
            airCount[swap] = 0;
            force = ivec4();
            momentum = ivec4();
            if (k_doDebug) std::memset(debugShit, 0, k_debugSize);
        }

    };



    static std::shared_ptr<Shape> s_shape;
    static bool s_swap(true);
    static int s_currentSlice(0); // slice index [0, k_nSlices)
    static float s_angleOfAttack(0.0f); // IN DEGREES
    static vec3 s_sweepLift; // accumulating lift force for entire sweep

    static std::shared_ptr<Program> s_foilProg, s_fbProg;

    static uint s_vertexArrayID;
    static uint s_vertexBufferID, s_vertexTexBox, s_indexBufferIDBox, s_sideviewVBO;

    static SSBO s_ssbo;

    static uint s_fbo;
    static uint s_fboTex;
    static uint s_flagTex;
    static uint s_geoTex; // texture holding the pixels of the geometry
    static uint s_sideTex; // texture holding the pixels of the geometry from the side
    static uint s_airTex; // texture holding the pixels of air

    static uint s_ssboId;
    static uint s_mapSSBOId;

    namespace OutlineShader {
        static uint prog;
        static uint u_swap;
    }

    namespace MoveShader {
        static uint prog;
        static uint u_swap;
    }

    namespace DrawShader {
        static uint prog;
        static uint u_swap;
    }



    static bool setupShaders(const std::string & resourcesDir) {
        std::string shadersDir(resourcesDir + "/shaders");

        // Foil Shader -------------------------------------------------------------

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

        // FB Shader ---------------------------------------------------------------

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

        // Outline Compute Shader --------------------------------------------------

        std::string ShaderString = readFileAsString(shadersDir + "/compute_outline.glsl");
        const char *shader = ShaderString.c_str();
        uint computeShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(computeShader, 1, &shader, nullptr);
        int rc;
        CHECKED_GL_CALL(glCompileShader(computeShader));
        CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
        if (!rc) { //error compiling the shader file
            GLSL::printShaderInfoLog(computeShader);
            std::cout << "Failed to compile outline shader" << std::endl;
            return false;
        }
        OutlineShader::prog = glCreateProgram();
        glAttachShader(OutlineShader::prog, computeShader);
        glLinkProgram(OutlineShader::prog);
        glGetProgramiv(OutlineShader::prog, GL_LINK_STATUS, &rc);
        if (!rc) {
            GLSL::printProgramInfoLog(OutlineShader::prog);
            std::cout << "Failed to link outline shader" << std::endl;
            return false;
        }
        OutlineShader::u_swap = glGetUniformLocation(OutlineShader::prog, "u_swap");
    
        // Move Compute Shader -----------------------------------------------------

        ShaderString = readFileAsString(shadersDir + "/compute_move.glsl");
        shader = ShaderString.c_str();
        computeShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(computeShader, 1, &shader, nullptr);

        CHECKED_GL_CALL(glCompileShader(computeShader));
        CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
        if (!rc) { //error compiling the shader file
            GLSL::printShaderInfoLog(computeShader);
            std::cout << "Failed to compile move shader" << std::endl;
            return false;
        }
        MoveShader::prog = glCreateProgram();
        glAttachShader(MoveShader::prog, computeShader);
        glLinkProgram(MoveShader::prog);
        glGetProgramiv(MoveShader::prog, GL_LINK_STATUS, &rc);
        if (!rc) {
            GLSL::printProgramInfoLog(MoveShader::prog);
            std::cout << "Failed to link move shader" << std::endl;
            return false;
        }
        MoveShader::u_swap = glGetUniformLocation(MoveShader::prog, "u_swap");
    
        // Draw Compute Shader -----------------------------------------------------

        ShaderString = readFileAsString(shadersDir + "/compute_draw.glsl");
        shader = ShaderString.c_str();
        computeShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(computeShader, 1, &shader, nullptr);

        CHECKED_GL_CALL(glCompileShader(computeShader));
        CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
        if (!rc) { //error compiling the shader file
            GLSL::printShaderInfoLog(computeShader);
            std::cout << "Failed to compile draw shader" << std::endl;
            return false;
        }
        DrawShader::prog = glCreateProgram();
        glAttachShader(DrawShader::prog, computeShader);
        glLinkProgram(DrawShader::prog);
        glGetProgramiv(DrawShader::prog, GL_LINK_STATUS, &rc);
        if (!rc) {
            GLSL::printProgramInfoLog(DrawShader::prog);
            std::cout << "Failed to link draw shader" << std::endl;
            return false;
        }
        DrawShader::u_swap = glGetUniformLocation(DrawShader::prog, "u_swap");

        return true;
    }


    static bool setupGeom(const std::string & resourcesDir) {
        std::string objsDir = resourcesDir + "/objs";
        // Initialize mesh.
        s_shape = std::make_shared<Shape>();
        s_shape->loadMesh(objsDir + "/0012.obj");
        //shape->resize();
        s_shape->init();

        //generate VBO for sideview
        glGenBuffers(1, &s_sideviewVBO);

        // generate the VAO
        glGenVertexArrays(1, &s_vertexArrayID);
        glBindVertexArray(s_vertexArrayID);

        // generate vertex buffer to hand off to OGL
        glGenBuffers(1, &s_vertexBufferID);
        // set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, s_vertexBufferID);

        GLfloat cube_vertices[] = {
            // front
            -1.0f, -1.0f, 0.0f, //LD
            1.0f, -1.0f, 0.0f, //RD
            1.0f,  1.0f, 0.0f, //RU
            -1.0f,  1.0f, 0.0f, //LU
        };

        // actually memcopy the data - only do this once
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

        // we need to set up the vertex array
        glEnableVertexAttribArray(0);
        // key function to get up how many elements to pull out at a time (3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        // color
        vec2 cube_tex[] = {
            // front colors
            vec2(0.0f, 0.0f),
            vec2(1.0f, 0.0f),
            vec2(1.0f, 1.0f),
            vec2(0.0f, 1.0f),
        };
        glGenBuffers(1, &s_vertexTexBox);
        // set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, s_vertexTexBox);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex), cube_tex, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        // indices
        glGenBuffers(1, &s_indexBufferIDBox);
        // set the current state to focus on our vertex buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_indexBufferIDBox);
        GLushort cube_elements[] = {
            // front
            0, 1, 2,
            2, 3, 0,
        };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);

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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, k_width, k_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        // You must reserve memory for other mipmaps levels as well either by making a series of calls to
        // glTexImage2D or use glGenerateMipmapEXT(GL_TEXTURE_2D).
        // Here, we'll use :
        glGenerateMipmap(GL_TEXTURE_2D);


        // Frame Buffer Object
        // RGBA8 2D texture, 24 bit depth texture, 256x256
        glGenTextures(1, &s_fboTex);
        glBindTexture(GL_TEXTURE_2D, s_fboTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // NULL means reserve texture memory, but texels are undefined
        // Tell OpenGL to reserve level 0
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, k_width, k_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        // You must reserve memory for other mipmaps levels as well either by making a series of calls to
        // glTexImage2D or use glGenerateMipmapEXT(GL_TEXTURE_2D).
        // Here, we'll use :
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenFramebuffers(1, &s_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
        // Attach 2D texture to this FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_fboTex, 0);

        uint fboDepthRB(0);
        glGenRenderbuffers(1, &fboDepthRB);
        glBindRenderbuffer(GL_RENDERBUFFER, fboDepthRB);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, k_width, k_height);

        // Attach depth buffer to FBO
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fboDepthRB);

        // Does the GPU support current FBO configuration?
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

    static void debug(int swap) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_ssboId);
        GLvoid * p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
        SSBO test;
        memcpy(&test, p, sizeof(SSBO));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        //std::cout << test.geoCount << " , " << test.debugShit[0].x << " , " << test.debugShit[0].y << " , " << test.debugShit[0].z  << " , " << test.debugShit[0].w << std::endl;
    }

    static void computeOutline(int swap) {
        glUseProgram(OutlineShader::prog);

        uint block_index = glGetProgramResourceIndex(OutlineShader::prog, GL_SHADER_STORAGE_BLOCK, "SSBO");
        glShaderStorageBlockBinding(OutlineShader::prog, block_index, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_ssboId);

        block_index = glGetProgramResourceIndex(OutlineShader::prog, GL_SHADER_STORAGE_BLOCK, "MapSSBO");
        glShaderStorageBlockBinding(OutlineShader::prog, block_index, 1);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_mapSSBOId);

        glBindImageTexture(2, s_flagTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
        glBindImageTexture(3, s_fboTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glBindImageTexture(4, s_geoTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(5, s_airTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glUniform1i(OutlineShader::u_swap, swap);
        //start compute shader program		
        glDispatchCompute(1024, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_geo);
        //p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
        //memcpy(&test, p, siz);
        //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    static void computeMove(int swap) {
        glUseProgram(MoveShader::prog);

        uint block_index = glGetProgramResourceIndex(OutlineShader::prog, GL_SHADER_STORAGE_BLOCK, "SSBO");
        glShaderStorageBlockBinding(OutlineShader::prog, block_index, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_ssboId);

        block_index = glGetProgramResourceIndex(OutlineShader::prog, GL_SHADER_STORAGE_BLOCK, "MapSSBO");
        glShaderStorageBlockBinding(OutlineShader::prog, block_index, 1);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_mapSSBOId);

        glBindImageTexture(2, s_flagTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
        glBindImageTexture(3, s_fboTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glBindImageTexture(4, s_geoTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(5, s_airTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glUniform1i(MoveShader::u_swap, swap);

        //start compute shader program		
        glDispatchCompute(1024, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);        
    }

    static void computeDraw(int swap) {
        glUseProgram(DrawShader::prog);

        uint block_index = glGetProgramResourceIndex(OutlineShader::prog, GL_SHADER_STORAGE_BLOCK, "SSBO");
        glShaderStorageBlockBinding(OutlineShader::prog, block_index, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_ssboId);

        block_index = glGetProgramResourceIndex(OutlineShader::prog, GL_SHADER_STORAGE_BLOCK, "MapSSBO");
        glShaderStorageBlockBinding(OutlineShader::prog, block_index, 1);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_mapSSBOId);

        glBindImageTexture(2, s_flagTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
        glBindImageTexture(3, s_fboTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glBindImageTexture(4, s_geoTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(5, s_airTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(6, s_sideTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        glUniform1i(DrawShader::u_swap, swap);

        // start compute shader program		
        glDispatchCompute(1024, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    }

    static void downloadSSBO() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_ssboId);
        void * p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        std::memcpy(&s_ssbo, p, sizeof(SSBO));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    static void uploadSSBO() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_ssboId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SSBO), &s_ssbo, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    static void computeReset(int swap) {
        downloadSSBO();

        // TODO: is this okay?
        s_sweepLift += vec3(s_ssbo.force) * 1.0e-6f;

        s_ssbo.reset(swap);

        uploadSSBO();

        int clearColor[4]{};
        glClearTexImage(s_flagTex, 0, GL_RED_INTEGER, GL_INT, &clearColor);

        //glClearTexSubImage(air_tex, 0, 0, swap*(k_estimateMaxAirPixelsRows / 2), 0, k_estimateMaxAirPixels, k_estimateMaxAirPixelsRows / 2, 1, GL_RGBA, GL_FLOAT, clearColor);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nulldata);
    }

    static void clearFlagTex() {
        static int clearColor[4]{};
        glClearTexImage(s_flagTex, 0, GL_RED_INTEGER, GL_INT, &clearColor);
    }

    static void renderToFramebuffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 V, M, P; //View, Model and Perspective matrix

        float zNear = k_sliceSize * (s_currentSlice);
        P = glm::ortho(
            -1.0f / s_ssbo.screenSpec.z, // left
             1.0f / s_ssbo.screenSpec.z, // right
            -1.0f / s_ssbo.screenSpec.w, // bottom
             1.0f / s_ssbo.screenSpec.w, // top
            zNear, // near
            zNear + k_sliceSize // far
        );

        s_foilProg->bind();
    
        uint block_index = 0;
        block_index = glGetProgramResourceIndex(s_foilProg->pid, GL_SHADER_STORAGE_BLOCK, "SSBO");
        uint ssbo_binding_point_index = 0;
        glShaderStorageBlockBinding(s_foilProg->pid, block_index, ssbo_binding_point_index);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_binding_point_index, s_ssboId);
        
        glBindImageTexture(2, s_flagTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
        
        glBindImageTexture(4, s_geoTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glBindImageTexture(6, s_sideTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        
        glUniformMatrix4fv(s_foilProg->getUniform("u_projMat"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(s_foilProg->getUniform("u_viewMat"), 1, GL_FALSE, &V[0][0]);

        mat4 Rx = glm::rotate(mat4(1.0f), glm::radians(-s_angleOfAttack), vec3(1.0f, 0.0f, 0.0f));
        
        mat3 N;
        glUniformMatrix3fv(s_foilProg->getUniform("u_normMat"), 1, GL_FALSE, &N[0][0]);
        M = Rx;
        glUniformMatrix4fv(s_foilProg->getUniform("u_modelMat"), 1, GL_FALSE, &M[0][0]);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        s_shape->draw(s_foilProg, false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        s_shape->draw(s_foilProg, false);

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
        glGenBuffers(1, &s_ssboId);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_ssboId);
        s_ssbo.screenSpec.x = float(k_width);
        s_ssbo.screenSpec.y = float(k_height);
        if (k_width >= k_height) {
            s_ssbo.screenSpec.z = float(k_height) / float(k_width);
            s_ssbo.screenSpec.w = 1.0f;
        }
        else {
            s_ssbo.screenSpec.z = 1.0f;
            s_ssbo.screenSpec.w = float(k_width) / float(k_height);
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_ssboId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SSBO), &s_ssbo, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        // Setup map SSBO
        glGenBuffers(1, &s_mapSSBOId);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_mapSSBOId);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_mapSSBOId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, k_maxAirPixelsSum * sizeof(s32), nullptr, GL_DYNAMIC_COPY);
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
        glBindImageTexture(2, s_flagTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        // Setup dense geometry pixel texture
        glGenTextures(1, &s_geoTex);
        glBindTexture(GL_TEXTURE_2D, s_geoTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, k_estimateMaxGeoPixels, k_extimateMaxGeoPixelsRows, 0, GL_RGBA, GL_FLOAT, NULL);
        glBindImageTexture(4, s_geoTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        // Setup dense air pixel texture
        glGenTextures(1, &s_airTex);
        glBindTexture(GL_TEXTURE_2D, s_airTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, k_estimateMaxAirPixels, k_estimateMaxAirPixelsRows, 0, GL_RGBA, GL_FLOAT, NULL);
        glBindImageTexture(5, s_airTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

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
        if (s_currentSlice == 0) {
            uint clearColor[4]{};
            glClearTexImage(s_flagTex, 0, GL_RED_INTEGER, GL_INT, &clearColor);
            glClearTexImage(s_geoTex, 0, GL_RGBA, GL_FLOAT, &clearColor);
            glClearTexImage(s_airTex, 0, GL_RGBA, GL_FLOAT, &clearColor);
            glClearTexImage(s_sideTex, 0, GL_RGBA, GL_FLOAT, &clearColor);
            s_ssbo.reset(0);
            s_ssbo.reset(1);
            s_sweepLift = vec3();
        }

        s_swap = !s_swap; // set next slice to current slice
        s_ssbo.reset(s_swap);
        uploadSSBO();
        clearFlagTex();
        renderToFramebuffer();
        clearFlagTex();
        computeMove(!s_swap);
        computeDraw(!s_swap);
        downloadSSBO();
        s_ssbo.airCount[0] = 0;
        s_ssbo.airCount[1] = 0;
        uploadSSBO();
        computeOutline(s_swap);
        downloadSSBO();
        clearFlagTex(); // TODO: find a way to not repeat this
        renderToFramebuffer(); // TODO: find a way to not repeat this
        computeDraw(s_swap); // TODO: find a way to not repeat this
        s_sweepLift += vec3(s_ssbo.force) * 1.0e-6f;

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
        glBindVertexArray(s_vertexArrayID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
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
        // TODO
        return vec3();
    }

    uint getSideTextureID() {
        return s_sideTex;
    }

    ivec2 getSize() {
        return ivec2(k_width, k_height);
    }


}