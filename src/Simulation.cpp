#include "Simulation.hpp"
#include <cstring>
#include <memory>
#include <iostream>

#include "glad/glad.h"
#include "WindowManager.h"
#include "Program.h"
#include "Shape.h"
#include "GLSL.h"
#include "glm/gtc/matrix_transform.hpp"



namespace simulation {



static constexpr int k_width(720), k_height(480);
static constexpr int k_nSlices(50);
static constexpr float k_sliceSize(0.01f); // z distance between slices
static constexpr int k_estimateMaxGeoPixels(16384);
static constexpr int k_extimateMaxGeoPixelsRows(27);
static constexpr int k_estimateMaxOutlinePixels(16384);
static constexpr int k_estimateMaxOutlinePixelsRows(54);



struct ssbo_liftdrag {

    unsigned int geo_count;
    unsigned int test;
    unsigned int out_count[2];
    glm::vec4 screenSpec; // screen width, screen height, x aspect factor, y aspect factor
    glm::ivec4 momentum;
    glm::ivec4 force;
    glm::ivec4 debugshit[4096];
	glm::vec4 sideview[50][2];

    ssbo_liftdrag() :
        geo_count(0),
        test(0),
        out_count{0, 0},
        screenSpec(),
        momentum(),
        force()
    {
        for (int ii = 0; ii < 4096; ii++) {
            debugshit[ii] = glm::ivec4();
        }
    }

    void reset_geo(unsigned int swap) {
        geo_count = test = 0;
        for (int ii = 0; ii < 4096; ii++) {
            debugshit[ii] = glm::ivec4();
        }
		out_count[swap] = 0;
    }
};



static std::shared_ptr<Shape> shape;
static bool swap = true;
static int currentSlice = 0; // slice index [0, k_nSlices)
static float angleOfAttack = 0.0f; // IN DEGREES
static glm::vec3 sweepLift; // accumulating lift force for entire sweep

static WindowManager * windowManager = nullptr;

// Our shader program
static std::shared_ptr<Program> progfoil, progfb;
//framebuffer
static GLuint fbo, fbo_tex, fbo_depth_rb;
static GLuint flag_tex;

// Contains vertex information for OpenGL
static GLuint vertexArrayID;

// Data necessary to give our box to OpenGL
static GLuint vertexBufferID, vertexTexBox, indexBufferIDBox, sideviewVBO;

//ssbos
static ssbo_liftdrag liftdrag_ssbo;
static GLuint geo_tex; // texture holding the pixels of the geometry
static GLuint side_geo_tex; // texture holding the pixels of the geometry from the side
static GLuint outline_tex; // texture holding the pixels of the outline
//static unsigned int * nulldata;
static GLuint ssbo_geo;

static GLuint computeprog_move;
static GLuint computeprog_draw;
static GLuint computeprog_outline;
static GLuint uniform_location_swap_out;
static GLuint uniform_location_slice_fs;
static GLuint uniform_location_slice_generate;
static GLuint uniform_location_swap_move;
static GLuint uniform_location_swap_draw;
static GLuint uniform_location_slice_draw;



static bool setupShaders(const std::string & resourcesDir) {
    std::string shadersDir(resourcesDir + "/shaders");

    // Foil Shader -------------------------------------------------------------

    progfoil = std::make_shared<Program>();
    progfoil->setVerbose(true);
    progfoil->setShaderNames(shadersDir + "/vs.glsl", shadersDir + "/fs.glsl");
    if (!progfoil->init()) {
        std::cerr << "Failed to initialize foil shader" << std::endl;
        return false;
    }
    progfoil->addAttribute("vertPos");
    progfoil->addAttribute("vertNor");
    progfoil->addAttribute("vertTex");
    progfoil->addUniform("P");
    progfoil->addUniform("V");
    progfoil->addUniform("M");
	progfoil->addUniform("MR");
	progfoil->addUniform("slice");

    // FB Shader ---------------------------------------------------------------

    progfb = std::make_shared<Program>();
    progfb->setVerbose(true);
    progfb->setShaderNames(shadersDir + "/fbvertex.glsl", shadersDir + "/fbfrag.glsl");
    if (!progfb->init()) {
        std::cerr << "Failed to initialize fb shader" << std::endl;
        return false;
    }
    progfb->addUniform("tex");
    glUseProgram(progfb->pid);
    glUniform1i(progfb->getUniform("tex"), 0);
    glUseProgram(0);

    // Outline Compute Shader --------------------------------------------------

    std::string ShaderString = readFileAsString(shadersDir + "/compute_outline.glsl");
    const char *shader = ShaderString.c_str();
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &shader, nullptr);
    GLint rc;
    CHECKED_GL_CALL(glCompileShader(computeShader));
    CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
    if (!rc) { //error compiling the shader file
        GLSL::printShaderInfoLog(computeShader);
        std::cout << "Failed to compile outline shader" << std::endl;
        return false;
    }
    computeprog_outline = glCreateProgram();
    glAttachShader(computeprog_outline, computeShader);
    glLinkProgram(computeprog_outline);
    glGetProgramiv(computeprog_outline, GL_LINK_STATUS, &rc);
    if (!rc) {
        GLSL::printProgramInfoLog(computeprog_outline);
        std::cout << "Failed to link outline shader" << std::endl;
        return false;
    }
    uniform_location_swap_out = glGetUniformLocation(computeprog_outline, "swap");
	uniform_location_slice_generate = glGetUniformLocation(computeprog_outline, "slice");
    
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
    computeprog_move = glCreateProgram();
    glAttachShader(computeprog_move, computeShader);
    glLinkProgram(computeprog_move);
    glGetProgramiv(computeprog_move, GL_LINK_STATUS, &rc);
    if (!rc) {
        GLSL::printProgramInfoLog(computeprog_move);
        std::cout << "Failed to link move shader" << std::endl;
        return false;
    }
    uniform_location_swap_move = glGetUniformLocation(computeprog_move, "swap");
    
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
    computeprog_draw = glCreateProgram();
    glAttachShader(computeprog_draw, computeShader);
    glLinkProgram(computeprog_draw);
    glGetProgramiv(computeprog_draw, GL_LINK_STATUS, &rc);
    if (!rc) {
        GLSL::printProgramInfoLog(computeprog_draw);
        std::cout << "Failed to link draw shader" << std::endl;
        return false;
    }
	uniform_location_swap_draw = glGetUniformLocation(computeprog_draw, "swap");
	uniform_location_slice_draw = glGetUniformLocation(computeprog_draw, "slice");

    return true;
}


static bool setupGeom(const std::string & resourcesDir) {
    std::string objsDir = resourcesDir + "/objs";
    // Initialize mesh.
    shape = std::make_shared<Shape>();
    shape->loadMesh(objsDir + "/a12.obj");
    shape->resize();
    shape->init();

	//generate VBO for sideview
	glGenBuffers(1, &sideviewVBO);

    // generate the VAO
    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    // generate vertex buffer to hand off to OGL
    glGenBuffers(1, &vertexBufferID);
    // set the current state to focus on our vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);

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
    glm::vec2 cube_tex[] = {
        // front colors
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec2(0.0f, 1.0f),
    };
    glGenBuffers(1, &vertexTexBox);
    // set the current state to focus on our vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexTexBox);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex), cube_tex, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    // indices
    glGenBuffers(1, &indexBufferIDBox);
    // set the current state to focus on our vertex buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferIDBox);
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
	glGenTextures(1, &side_geo_tex);
	glBindTexture(GL_TEXTURE_2D, side_geo_tex);
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
    glGenTextures(1, &fbo_tex);
    glBindTexture(GL_TEXTURE_2D, fbo_tex);
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

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // Attach 2D texture to this FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);

    glGenRenderbuffers(1, &fbo_depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo_depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, k_width, k_height);

    // Attach depth buffer to FBO
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo_depth_rb);

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

static void debug_buff(unsigned int swap) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_geo);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
    int siz = sizeof(ssbo_liftdrag);
    ssbo_liftdrag test;
    memcpy(&test, p, siz);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    //std::cout << test.geo_count << " , " << test.debugshit[0].x << " , " << test.debugshit[0].y << " , " << test.debugshit[0].z  << " , " << test.debugshit[0].w << std::endl;

    //static unsigned int pboID = 0;
    //if (pboID == 0) {
    //    glGenBuffers(1, &pboID);
    //    glBindBuffer(GL_PIXEL_PACK_BUFFER, pboID);
    //    glBufferData(GL_PIXEL_PACK_BUFFER, 720 * 480 * 4, 0, GL_DYNAMIC_READ);
    //    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    //}
}

static void compute_generate_outline(unsigned int swap) {
    glUseProgram(computeprog_outline);

    //bind compute buffers
    //glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, ac_buffer);

    GLuint block_index = 0;
    block_index = glGetProgramResourceIndex(computeprog_outline, GL_SHADER_STORAGE_BLOCK, "ssbo_geopixels");
    GLuint ssbo_binding_point_index = 0;
    glShaderStorageBlockBinding(computeprog_outline, block_index, ssbo_binding_point_index);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_binding_point_index, ssbo_geo);

    glActiveTexture(GL_TEXTURE2);
    glBindImageTexture(2, flag_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
    glActiveTexture(GL_TEXTURE3);
    glBindImageTexture(3, fbo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glActiveTexture(GL_TEXTURE4);
    glBindImageTexture(4, geo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glActiveTexture(GL_TEXTURE5);
    glBindImageTexture(5, outline_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	glUniform1ui(uniform_location_swap_out, swap);
	glUniform1ui(uniform_location_slice_generate, currentSlice);
    //start compute shader program		
    glDispatchCompute((GLuint)1024, (GLuint)1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_geo);
    //p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
    //memcpy(&test, p, siz);
    //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

static void compute_move_outline(unsigned int swap) {
    glUseProgram(computeprog_move);

    //bind compute buffers
    GLuint block_index = 0;
    block_index = glGetProgramResourceIndex(computeprog_move, GL_SHADER_STORAGE_BLOCK, "ssbo_geopixels");
    GLuint ssbo_binding_point_index = 0;
    glShaderStorageBlockBinding(computeprog_move, block_index, ssbo_binding_point_index);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_binding_point_index, ssbo_geo);

    glActiveTexture(GL_TEXTURE2);
    glBindImageTexture(2, flag_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
    glActiveTexture(GL_TEXTURE3);
    glBindImageTexture(3, fbo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glActiveTexture(GL_TEXTURE4);
    glBindImageTexture(4, geo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glActiveTexture(GL_TEXTURE5);
    glBindImageTexture(5, outline_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    glUniform1ui(uniform_location_swap_move, swap);

    //start compute shader program		
    glDispatchCompute(1024, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);        
}

static void compute_draw_outline(unsigned int swap) {
    glUseProgram(computeprog_draw);

    // bind compute buffers
    GLuint block_index = 0;
    block_index = glGetProgramResourceIndex(computeprog_draw, GL_SHADER_STORAGE_BLOCK, "ssbo_geopixels");
    GLuint ssbo_binding_point_index = 0;
    glShaderStorageBlockBinding(computeprog_draw, block_index, ssbo_binding_point_index);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_binding_point_index, ssbo_geo);

    glActiveTexture(GL_TEXTURE2);
    glBindImageTexture(2, flag_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
    glActiveTexture(GL_TEXTURE3);
    glBindImageTexture(3, fbo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
	glActiveTexture(GL_TEXTURE4);
	glBindImageTexture(4, geo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glActiveTexture(GL_TEXTURE5);
    glBindImageTexture(5, outline_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glActiveTexture(GL_TEXTURE6);
	glBindImageTexture(6, side_geo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

	glUniform1ui(uniform_location_swap_draw, swap);
	glUniform1ui(uniform_location_slice_draw, currentSlice);

    // start compute shader program		
    glDispatchCompute((GLuint)1024, (GLuint)1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
}

static void compute_reset(unsigned int swap) {
    // erase atomic counter
    //glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
    //GLuint* ptr = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint),
    //GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    //ptr[0] = 0;
    //glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    //glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_geo);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
    int siz = sizeof(ssbo_liftdrag);
    memcpy(&liftdrag_ssbo, p, siz);

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // TODO: is this okay?
    sweepLift += glm::vec3(liftdrag_ssbo.force) * 1.0e-6f;

    liftdrag_ssbo.reset_geo(swap);
    liftdrag_ssbo.force = glm::ivec4();
    liftdrag_ssbo.momentum = glm::ivec4();
    // reset pixel ssbo and flag_img
    //static ssbo_liftdrag temp;
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_liftdrag), &liftdrag_ssbo, GL_DYNAMIC_COPY);
	GLuint clearColor[4]{};
	glClearTexImage(flag_tex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clearColor);

	//glClearTexSubImage(outline_tex, 0, 0, swap*(k_estimateMaxOutlinePixelsRows / 2), 0, k_estimateMaxOutlinePixels, k_estimateMaxOutlinePixelsRows / 2, 1, GL_RGBA, GL_FLOAT, clearColor);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nulldata);
}

static void render_to_framebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Get current frame buffer size.
    glViewport(0, 0, k_width, k_height);

    // Clear framebuffer.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Create the matrix stacks - please leave these alone for now

    glm::mat4 V, M, P; //View, Model and Perspective matrix
    V = glm::mat4();
    M = glm::mat4(1);

    float zNear = k_sliceSize * (float)(currentSlice + 28);
    P = glm::ortho(	-1.f/liftdrag_ssbo.screenSpec.z, 1.f / liftdrag_ssbo.screenSpec.z, //left and right
					-1.f / liftdrag_ssbo.screenSpec.w, 1.f / liftdrag_ssbo.screenSpec.w, //bottom and top
					zNear, zNear + k_sliceSize); //near and far

    //animation with the model matrix:
    glm::mat4 TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.5f));
    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));

    progfoil->bind();

    
    GLuint block_index = 0;
    block_index = glGetProgramResourceIndex(progfoil->pid, GL_SHADER_STORAGE_BLOCK, "ssbo_geopixels");
    GLuint ssbo_binding_point_index = 0;
    glShaderStorageBlockBinding(progfoil->pid, block_index, ssbo_binding_point_index);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_binding_point_index, ssbo_geo);
        
    glActiveTexture(GL_TEXTURE2);
    glBindImageTexture(2, flag_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
        
    glActiveTexture(GL_TEXTURE4);
    glBindImageTexture(4, geo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE6);
	glBindImageTexture(6, side_geo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        
    glUniformMatrix4fv(progfoil->getUniform("P"), 1, GL_FALSE, &P[0][0]);
    glUniformMatrix4fv(progfoil->getUniform("V"), 1, GL_FALSE, &V[0][0]);

    glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), glm::radians(-angleOfAttack), glm::vec3(1.0f, 0.0f, 0.0f));
    S = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
        
    glm::mat4 MR(1);
    glUniformMatrix4fv(progfoil->getUniform("MR"), 1, GL_FALSE, &MR[0][0]);
    M = TransZ * Rx * S;
    glUniformMatrix4fv(progfoil->getUniform("M"), 1, GL_FALSE, &M[0][0]);
	glUniform1ui(progfoil->getUniform("slice"), currentSlice);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    shape->draw(progfoil, false);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    shape->draw(progfoil, false);

    progfoil->unbind();

    /*
    glViewport(width / 2, height / 4, width / 2, height / 2);
    V = glm::rotate(glm::mat4(1),0.23f, glm::vec3(0, 1, 0));
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
    windowManager = new WindowManager();
    if (!windowManager->init(k_width, k_height)) {
        std::cerr << "Failed to initialize window manager" << std::endl;
        return false;
    }    

    // Initialize nulldata
    //nulldata = new unsigned int[k_width * k_height * 2]{}; // zero initialization

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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

    // Setup liftdrag_ssbo
    glGenBuffers(1, &ssbo_geo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_geo);
    liftdrag_ssbo.geo_count = 0;
    liftdrag_ssbo.screenSpec.x = float(k_width);
    liftdrag_ssbo.screenSpec.y = float(k_height);
	if (k_width >= k_height) {
		liftdrag_ssbo.screenSpec.z = float(k_height) / float(k_width);
		liftdrag_ssbo.screenSpec.w = 1.0f;
	}
	else {
		liftdrag_ssbo.screenSpec.z = 1.0f;
		liftdrag_ssbo.screenSpec.w = float(k_width) / float(k_height);
	}
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_geo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_liftdrag), &liftdrag_ssbo, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }

    // Setup flag texture
    glGenTextures(1, &flag_tex);
    glBindTexture(GL_TEXTURE_2D, flag_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, k_width, k_height * 2, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
	GLuint clearcolor = 0;
	glClearTexImage(flag_tex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clearcolor);
    glActiveTexture(GL_TEXTURE2);
    glBindImageTexture(2, flag_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }

    // Setup dense geo pixel texture
    glGenTextures(1, &geo_tex);
    glBindTexture(GL_TEXTURE_2D, geo_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, k_estimateMaxGeoPixels, k_extimateMaxGeoPixelsRows, 0, GL_RGBA, GL_FLOAT, NULL);
    glActiveTexture(GL_TEXTURE4);
    glBindImageTexture(4, geo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }

    // Setup outline pixel texture
    glGenTextures(1, &outline_tex);
    glBindTexture(GL_TEXTURE_2D, outline_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, k_estimateMaxOutlinePixels, k_estimateMaxOutlinePixelsRows, 0, GL_RGBA, GL_FLOAT, NULL);
    glActiveTexture(GL_TEXTURE5);
    glBindImageTexture(5, outline_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

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

bool step() {
    if (currentSlice == 0) {
        // TODO: reset everything here
		GLuint clearColor[4]{};
		glClearTexImage(outline_tex, 0, GL_RGBA, GL_FLOAT, &clearColor);
		glClearTexImage(geo_tex, 0, GL_RGBA, GL_FLOAT, &clearColor);
		glClearTexImage(flag_tex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clearColor);
		glClearTexImage(side_geo_tex, 0, GL_RGBA, GL_FLOAT, &clearColor);
		memset(&liftdrag_ssbo, 0, sizeof(ssbo_liftdrag));
        sweepLift = glm::vec3();
    }
    render_to_framebuffer();
    compute_move_outline(swap); //move current slice
    compute_draw_outline(swap); //draw current slice
    compute_generate_outline(!swap); //generate next slice outline
    compute_reset(swap); //reset current slice
    //debug_buff(swap);
	swap = !swap; //set next slice to current slice

    if (++currentSlice >= k_nSlices) {
        currentSlice = 0;
        return true;
    }

    return false;
}

void render() {
    glViewport(0, 0, k_width, k_height);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    progfb->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo_tex);
    glBindVertexArray(vertexArrayID);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
    progfb->unbind();

    glfwSwapBuffers(windowManager->getHandle());
}

void cleanup() {
    windowManager->shutdown();
}

GLFWwindow * getWindow() {
    return windowManager->getHandle();
}

int getSlice() {
    return currentSlice;
}

int getNSlices() {
    return k_nSlices;
}

float getAngleOfAttack() {
    return angleOfAttack;
}

void setAngleOfAttack(float angle) {
    angleOfAttack = angle;
}

glm::vec3 getLift() {
    return sweepLift;
}

glm::vec3 getDrag() {
    // TODO
    return glm::vec3();
}

std::pair<glm::vec3, glm::vec3> getSideviewOutline(){
	 return std::make_pair(glm::vec3(liftdrag_ssbo.sideview[currentSlice-1][0]), glm::vec3(liftdrag_ssbo.sideview[currentSlice-1][1]));
}

int getSideTextureID() {
	return (int)side_geo_tex;
}


}
