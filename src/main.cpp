// Realtime Lift and Drag SURP 2018
// Christian Eckhart, William Newey, Austin Quick, Sebastian Seibert



// allows program to be run on dedicated graphics processor for laptops with
// both integrated and dedicated graphics using Nvidia Optimus
#ifdef _WIN32
extern "C" {
    _declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
}
#endif



#include <iostream>

#include "glad/glad.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Shape.h"
#include "glm/gtc/matrix_transform.hpp"

#define ESTIMATEMAXGEOPIXELS 16384
#define ESTIMATEMAXGEOPIXELSROWS 27
#define ESTIMATEMAXOUTLINEPIXELS 16384
#define ESTIMATEMAXOUTLINEPIXELSROWS 54



namespace {



std::shared_ptr<Shape> shape;
int sweep = 0;
int progress = 0;



class ssbo_geopixels {

    public:

    unsigned int geo_count;
    unsigned int test;
    unsigned int out_count[2];
    glm::vec4 momentum;
    glm::vec4 force;
    glm::ivec4 debugshit[4096];

    ssbo_geopixels() {
        geo_count = test = out_count[0] = out_count[1] = 0;
        momentum = force = glm::ivec4();
        reset_geo(2);
        for (int ii = 0; ii < 4096; ii++) {
            debugshit[ii] = glm::ivec4();
        }
    }

    void reset_geo(unsigned int swap) {
        geo_count = test = 0;
        for (int ii = 0; ii < 4096; ii++) {
            debugshit[ii] = glm::ivec4();
        }
        switch (swap) {
            case 0:	out_count[0] = 0; break;
            case 1:	out_count[1] = 0; break;
            case 2:
                out_count[0] = 0;
                out_count[1] = 0;
                break;
            }
        }
    };

/*class ssbo_outline {

    public:

    vec3 worldpos[ESTIMATEMAXOUTLINEPIXELS];
    vec3 momentum[ESTIMATEMAXOUTLINEPIXELS];
    ivec2 texpos[ESTIMATEMAXOUTLINEPIXELS];
    int pixelcount;

    ssbo_outline() {
        pixelcount = 0;
        for (int ii = 0; ii < ESTIMATEMAXOUTLINEPIXELS; ii++) {
            momentum[ii] = worldpos[ii] = vec3();
            texpos[ii] = ivec2();
        }
    }

};

class ssbo_object {

    public:

    vec3 momentum;
    vec3 force;
    int fragcount;

    ssbo_object() {
        fragcount = 0;
        momentum = force = vec3();
    }

};*/

double get_last_elapsed_time() {
    static double lasttime = glfwGetTime();
    double actualtime = glfwGetTime();
    double difference = actualtime - lasttime;
    lasttime = actualtime;
    return difference;
}

class Camera {

    public:

    glm::vec3 pos, rot;
    int w, a, s, d;

    Camera() {
        w = a = s = d = 0;
        pos = rot = glm::vec3();
    }

    glm::mat4 process(float ftime) {
        float speed = 0.0f;
        if (w == 1) {
            speed = 10 * ftime;
        }
        else if (s == 1) {
            speed = -10 * ftime;
        }
        float yangle = 0.0f;
        if (a == 1) {
            yangle = -3 * ftime;
        }
        else if (d == 1) {
            yangle = 3 * ftime;
        }
        rot.y += yangle;
        glm::mat4 R = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec4 dir = glm::vec4(0.0f, 0.0f, speed, 1.0f);
        dir = dir * R;
        pos.x += dir.x; pos.y += dir.y; pos.z += dir.z;
        glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
        return R * T;
    }
};

Camera mycam;

class Application : public EventCallbacks {

    public:

    WindowManager * windowManager = nullptr;

    // Our shader program
    std::shared_ptr<Program> prog, progfoil, progfb;
    //framebuffer
    GLuint FBOtex, FrameBufferObj, depth_rb, FlagTex;
    GLuint FlagBuff, FlagBuffTex;

    // Contains vertex information for OpenGL
    GLuint VertexArrayID;

    // Data necessary to give our box to OpenGL
    GLuint VertexBufferID, VertexNormDBox, VertexTexBox, IndexBufferIDBox, InstanceBuffer;

    //ssbos
    ssbo_geopixels geometry_ssbo;
    GLuint geo_tex; //texture holding the pixels of the geometry (its a buffer rather than a texture)
    GLuint outline_tex; //texture holding the pixels of the outline (its a buffer rather than a texture)
    GLuint ac_buffer = 0;
    //ssbo_outline outline_ssbo;
    //ssbo_object object_ssbo;
    unsigned int *nulldata = nullptr;
    GLuint ssbo_geo;
    GLuint ssbo_out;
    GLuint ssbo_obj;

    GLuint computeprog_move, computeprog_draw;
    GLuint computeprog_outline;
    GLuint uniform_location_swap_out = 0;
    GLuint uniform_location_swap_move = 0;
    GLuint uniform_location_swap_draw = 0;

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        else if (key == GLFW_KEY_W && action == GLFW_PRESS) {
            mycam.w = 1;
        }
        else if (key == GLFW_KEY_W && action == GLFW_RELEASE) {
            mycam.w = 0;
        }
        else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
            mycam.s = 1;
        }
        else if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
            mycam.s = 0;
        }
        else if (key == GLFW_KEY_A && action == GLFW_PRESS) {
            mycam.a = 1;
        }
        else if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
            mycam.a = 0;
        }
        else if (key == GLFW_KEY_D && action == GLFW_PRESS) {
            mycam.d = 1;
        }
        else if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
            mycam.d = 0;
        }
        else if (key == GLFW_KEY_L && action == GLFW_PRESS) {
            sweep++;
            std::cout << sweep << std::endl;
        }
        else if (key == GLFW_KEY_K && action == GLFW_PRESS) {
            sweep--;
            std::cout << sweep << std::endl;
        }
    }

    // callback for the mouse when clicked
    void mouseCallback(GLFWwindow *window, int button, int action, int mods) {
        double posX, posY;
        if (action == GLFW_PRESS) {
            glfwGetCursorPos(window, &posX, &posY);
        }
    }

    // if the window is resized, capture the new size and reset the viewport
    void resizeCallback(GLFWwindow *window, int in_width, int in_height) {
        //get the window size - may be different then pixels for retina
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
    }

    // Note that any gl calls must always happen after a GL state is initialized
    void initGeom() {
        std::string resourceDirectory = "../resources";
        // Initialize mesh.
        shape = std::make_shared<Shape>();
        //shape->loadMesh(resourceDirectory + "/t800.obj");
        shape->loadMesh(resourceDirectory + "/a12.obj");
        //shape->loadMesh(resourceDirectory + "/sphere.obj");
        shape->resize();
        shape->init();

        // generate the VAO
        glGenVertexArrays(1, &VertexArrayID);
        glBindVertexArray(VertexArrayID);

        // generate vertex buffer to hand off to OGL
        glGenBuffers(1, &VertexBufferID);
        // set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);

        GLfloat cube_vertices[] = {
            // front
            -1.0f, -1.0f, 0.0f, //LD
            1.0f, -1.0f, 0.0f, //RD
            1.0f,  1.0f, 0.0f, //RU
            -1.0f,  1.0f, 0.0f, //LU
        };

        // actually memcopy the data - only do this once
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_DYNAMIC_DRAW);

        // we need to set up the vertex array
        glEnableVertexAttribArray(0);
        // key function to get up how many elements to pull out at a time (3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        // color
        glm::vec2 cube_tex[] = {
            // front colors
            glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(0.0f, 0.0f),
        };
        glGenBuffers(1, &VertexTexBox);
        // set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VertexTexBox);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex), cube_tex, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        // indices
        glGenBuffers(1, &IndexBufferIDBox);
        // set the current state to focus on our vertex buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
        GLushort cube_elements[] = {
            // front
            1, 0, 2,
            2, 3, 0,
        };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);

        glBindVertexArray(0);

        // make SSBOs

        glGenBuffers(1, &ssbo_geo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_geo);
        geometry_ssbo.geo_count = 0;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_geo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_geopixels), &geometry_ssbo, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        //glGenBuffers(1, &ssbo_out);
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_out);
        //glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_outline), &outline_ssbo, GL_DYNAMIC_COPY);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_out);
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 

        //glGenBuffers(1, &ssbo_obj);
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_obj);
        //glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_object), &object_ssbo, GL_DYNAMIC_COPY);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_obj);
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glGenBuffers(1, &ac_buffer);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
        GLuint initac = 0;
        glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initac, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    }

    void init_framebuffer() {
        int width, height;
        // flag texture
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        glGenTextures(1, &FlagTex);
        glBindTexture(GL_TEXTURE_2D, FlagTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nulldata);
        glActiveTexture(GL_TEXTURE2);
        glBindImageTexture(2, FlagTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

        glGenBuffers(1, &FlagBuff);
        glBindBuffer(GL_TEXTURE_BUFFER, FlagBuff);

        // initialize buffer object
        unsigned int size = width * height * sizeof(unsigned int);
        glBufferData(GL_TEXTURE_BUFFER, size, 0, GL_DYNAMIC_DRAW);

        //tex
        //glGenTextures(1, &FlagBuffTex);
        //glBindTexture(GL_TEXTURE_BUFFER, FlagBuffTex);
        //glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, FlagBuff);
        //glBindBuffer(GL_TEXTURE_BUFFER, 0);

        // dense geo pixel texture

        glGenTextures(1, &geo_tex);
        glBindTexture(GL_TEXTURE_2D, geo_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        int mx = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mx);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, ESTIMATEMAXGEOPIXELS, ESTIMATEMAXGEOPIXELSROWS, 0, GL_BGRA, GL_FLOAT, NULL); // Texture width is too big, this causes error!!!
        glActiveTexture(GL_TEXTURE4);
        glBindImageTexture(4, geo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        // outline pixel texture

        glGenTextures(1, &outline_tex);
        glBindTexture(GL_TEXTURE_2D, outline_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, ESTIMATEMAXOUTLINEPIXELS, ESTIMATEMAXOUTLINEPIXELSROWS, 0, GL_BGRA, GL_FLOAT, NULL); // Texture width is too big, this causes error!!!
        glActiveTexture(GL_TEXTURE5);
        glBindImageTexture(5, outline_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        // Frame Buffer Object
        // RGBA8 2D texture, 24 bit depth texture, 256x256
        glGenTextures(1, &FBOtex);
        glBindTexture(GL_TEXTURE_2D, FBOtex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // NULL means reserve texture memory, but texels are undefined
        // Tell OpenGL to reserve level 0
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
        // You must reserve memory for other mipmaps levels as well either by making a series of calls to
        // glTexImage2D or use glGenerateMipmapEXT(GL_TEXTURE_2D).
        // Here, we'll use :
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenFramebuffers(1, &FrameBufferObj);
        glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferObj);
        // Attach 2D texture to this FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOtex, 0);

        glGenRenderbuffers(1, &depth_rb);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

        // Attach depth buffer to FBO
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

        // Does the GPU support current FBO configuration?
        GLenum status;
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        switch (status) {
            case GL_FRAMEBUFFER_COMPLETE:
                std::cout << "status framebuffer: good";
                break;
            default:
                std::cout << "status framebuffer: bad!!!!!!!!!!!!!!!!!!!!!!!!!";
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GLuint Tex1Location = glGetUniformLocation(progfb->pid, "tex");
        glUseProgram(progfb->pid);
        glUniform1i(Tex1Location, 0);
    }

    //General OGL initialization - set OGL state here
    void init(const std::string & resourceDirectory) {
        GLSL::checkVersion();

        // Set background color.
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // Enable z-buffer test.
        glEnable(GL_DEPTH_TEST);
        //glDisable(GL_DEPTH_TEST);
        // Initialize the GLSL program.
        prog = std::make_shared<Program>();
        prog->setVerbose(true);
        prog->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
        if (!prog->init()) {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog->addUniform("P");
        prog->addUniform("V");
        prog->addUniform("M");
        prog->addUniform("campos");
        prog->addAttribute("vertPos");
        prog->addAttribute("vertNor");
        prog->addAttribute("vertTex");
        prog->addAttribute("InstancePos");

        progfoil = std::make_shared<Program>();
        progfoil->setVerbose(true);
        progfoil->setShaderNames(resourceDirectory + "/vs.glsl", resourceDirectory + "/fs.glsl");
        if (!progfoil->init()) {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        progfoil->addUniform("P");
        progfoil->addUniform("V");
        progfoil->addUniform("M");
        progfoil->addUniform("MR");
        progfoil->addUniform("campos");
        progfoil->addAttribute("vertPos");
        progfoil->addAttribute("vertNor");
        progfoil->addAttribute("vertTex");

        progfb = std::make_shared<Program>();
        progfb->setVerbose(true);
        progfb->setShaderNames(resourceDirectory + "/fbvertex.glsl", resourceDirectory + "/fbfrag.glsl");
        if (!progfb->init()) {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        progfb->addAttribute("vertPos");
        progfb->addAttribute("vertTex");

        // load the compute shader OUTLINE
        std::string ShaderString = readFileAsString("../resources/compute_outline.glsl");
        const char *shader = ShaderString.c_str();
        GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(computeShader, 1, &shader, nullptr);
        GLint rc;
        CHECKED_GL_CALL(glCompileShader(computeShader));
        CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
        if (!rc) { //error compiling the shader file
            GLSL::printShaderInfoLog(computeShader);
            std::cout << "Error compiling fragment shader " << std::endl;
            exit(1);
        }
        computeprog_outline = glCreateProgram();
        glAttachShader(computeprog_outline, computeShader);
        glLinkProgram(computeprog_outline);
        glUseProgram(computeprog_outline);
        uniform_location_swap_out = glGetUniformLocation(computeprog_outline, "swap");

        //load the compute shader MOVE
        ShaderString = readFileAsString("../resources/compute_move.glsl");
        shader = ShaderString.c_str();
        computeShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(computeShader, 1, &shader, nullptr);

        CHECKED_GL_CALL(glCompileShader(computeShader));
        CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
        if (!rc) { //error compiling the shader file
            GLSL::printShaderInfoLog(computeShader);
            std::cout << "Error compiling fragment shader " << std::endl;
            exit(1);
        }
        computeprog_move = glCreateProgram();
        glAttachShader(computeprog_move, computeShader);
        glLinkProgram(computeprog_move);
        glUseProgram(computeprog_move);
        uniform_location_swap_move = glGetUniformLocation(computeprog_move, "swap");

        //load the compute shader DRAW (outline)
        ShaderString = readFileAsString("../resources/compute_draw.glsl");
        shader = ShaderString.c_str();
        computeShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(computeShader, 1, &shader, nullptr);

        CHECKED_GL_CALL(glCompileShader(computeShader));
        CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
        if (!rc) { //error compiling the shader file
            GLSL::printShaderInfoLog(computeShader);
            std::cout << "Error compiling fragment shader " << std::endl;
            exit(1);
        }
        computeprog_draw = glCreateProgram();
        glAttachShader(computeprog_draw, computeShader);
        glLinkProgram(computeprog_draw);
        glUseProgram(computeprog_draw);
        uniform_location_swap_draw = glGetUniformLocation(computeprog_draw, "swap");

        // init nulldata
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

        nulldata = new unsigned int[width*height];
        for (int ii = 0; ii < width*height; ii++) {
            nulldata[ii] = 0;
        }
    }

    void debug_buff(unsigned int swap) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_geo);
        GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
        int siz = sizeof(ssbo_geopixels);
        ssbo_geopixels test;
        memcpy(&test, p, siz);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        //static unsigned int pboID = 0;
        //if (pboID == 0) {
        //    glGenBuffers(1, &pboID);
        //    glBindBuffer(GL_PIXEL_PACK_BUFFER, pboID);
        //    glBufferData(GL_PIXEL_PACK_BUFFER, 720 * 480 * 4, 0, GL_DYNAMIC_READ);
        //    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        //}
    }

    void compute_generate_outline(unsigned int swap) {
        glUseProgram(computeprog_outline);

        //bind compute buffers
        //glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, ac_buffer);

        GLuint block_index = 0;
        block_index = glGetProgramResourceIndex(computeprog_outline, GL_SHADER_STORAGE_BLOCK, "ssbo_geopixels");
        GLuint ssbo_binding_point_index = 0;
        glShaderStorageBlockBinding(computeprog_outline, block_index, ssbo_binding_point_index);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_binding_point_index, ssbo_geo);

        glActiveTexture(GL_TEXTURE2);
        glBindImageTexture(2, FlagTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
        glActiveTexture(GL_TEXTURE3);
        glBindImageTexture(3, FBOtex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glActiveTexture(GL_TEXTURE4);
        glBindImageTexture(4, geo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glActiveTexture(GL_TEXTURE5);
        glBindImageTexture(5, outline_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glUniform1ui(uniform_location_swap_out, swap);
        //start compute shader program		
        glDispatchCompute((GLuint)1024, (GLuint)1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_geo);
        //p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
        //memcpy(&test, p, siz);
        //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    void compute_move_outline(unsigned int swap) {
        glUseProgram(computeprog_move);

        //bind compute buffers
        GLuint block_index = 0;
        block_index = glGetProgramResourceIndex(computeprog_move, GL_SHADER_STORAGE_BLOCK, "ssbo_geopixels");
        GLuint ssbo_binding_point_index = 0;
        glShaderStorageBlockBinding(computeprog_move, block_index, ssbo_binding_point_index);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_binding_point_index, ssbo_geo);

        glActiveTexture(GL_TEXTURE2);
        glBindImageTexture(2, FlagTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
        glActiveTexture(GL_TEXTURE3);
        glBindImageTexture(3, FBOtex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glActiveTexture(GL_TEXTURE5);
        glBindImageTexture(5, outline_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glUniform1ui(uniform_location_swap_move, swap);

        //start compute shader program		
        glDispatchCompute(1024, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);        
    }

    void compute_draw_outline(unsigned int swap) {
        glUseProgram(computeprog_draw);

        // bind compute buffers
        GLuint block_index = 0;
        block_index = glGetProgramResourceIndex(computeprog_draw, GL_SHADER_STORAGE_BLOCK, "ssbo_geopixels");
        GLuint ssbo_binding_point_index = 0;
        glShaderStorageBlockBinding(computeprog_draw, block_index, ssbo_binding_point_index);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_binding_point_index, ssbo_geo);

        glActiveTexture(GL_TEXTURE2);
        glBindImageTexture(2, FlagTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
        glActiveTexture(GL_TEXTURE3);
        glBindImageTexture(3, FBOtex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glActiveTexture(GL_TEXTURE5);
        glBindImageTexture(5, outline_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glUniform1ui(uniform_location_swap_draw, swap);

        // start compute shader program		
        glDispatchCompute((GLuint)1024, (GLuint)1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    }

    void compute_reset(unsigned int swap) {
        // erase atomic counter
        //glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
        //GLuint* ptr = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint),
        //GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
        //ptr[0] = 0;
        //glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
        //glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_geo);
        GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
        int siz = sizeof(ssbo_geopixels);
        memcpy(&geometry_ssbo, p, siz);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        geometry_ssbo.reset_geo(swap);
        // reset pixel ssbo and flag_img
        //static ssbo_geopixels temp;
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_geopixels), &geometry_ssbo, GL_DYNAMIC_COPY);
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        glBindTexture(GL_TEXTURE_2D, FlagTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nulldata);
    }

    void render_to_framebuffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferObj);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float frametime = float(get_last_elapsed_time());

        // Get current frame buffer size.
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create the matrix stacks - please leave these alone for now

        glm::mat4 V, M, P; //View, Model and Perspective matrix
        V = mycam.process(frametime);
        M = glm::mat4(1);

        // ...but we overwrite it (optional) with a perspective projection.
        P = glm::perspective(glm::pi<float>() / 4.0f, float(width) / float(height), 0.1f, 1000.0f);
        float stepwidth = 0.01f;

        static float counttime = 0.0f;
        if (counttime > 0.2f) {
            sweep++;
            counttime = 0.0f;
            progress = 1;
        }
        if (sweep > 50)  {
            sweep = 0;
        }

        float offset_plane = stepwidth * (float)(sweep + 28);
        P = glm::ortho(-aspect, aspect, -1.0f, 1.0f, 0.0f + offset_plane, stepwidth + offset_plane);

        counttime += frametime;

        //P = glm::ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, 0.0f ,100000.0f);

        //animation with the model matrix:
        static float w = 0.0f;
        w += 1.0f * frametime; //rotation angle
        float trans = 0.0f; // sin(t) * 2;
        glm::mat4 RotateY = glm::rotate(glm::mat4(1.0f), w, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -0.5f));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));

        progfoil->bind();

        //bind compute buffers
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, ac_buffer);
        //
        GLuint block_index = 0;
        block_index = glGetProgramResourceIndex(progfoil->pid, GL_SHADER_STORAGE_BLOCK, "ssbo_geopixels");
        GLuint ssbo_binding_point_index = 0;
        glShaderStorageBlockBinding(progfoil->pid, block_index, ssbo_binding_point_index);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_binding_point_index, ssbo_geo);
        //
        glActiveTexture(GL_TEXTURE2);
        glBindImageTexture(2, FlagTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
        //
        glActiveTexture(GL_TEXTURE4);
        glBindImageTexture(4, geo_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        //
        glUniformMatrix4fv(progfoil->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(progfoil->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniform3fv(progfoil->getUniform("campos"), 1, &mycam.pos[0]);
        w = glm::pi<float>() / 2.0f;
        glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), w, glm::vec3(0.0f, 1.0f, 0.0f));
        S = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 5.0f, 1.0f));
        //
        glm::mat4 MR(1);
        glUniformMatrix4fv(progfoil->getUniform("MR"), 1, GL_FALSE, &MR[0][0]);
        M = TransZ * S;
        glUniformMatrix4fv(progfoil->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        shape->draw(progfoil, false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        shape->draw(progfoil, false);
        progfoil->unbind();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glBindTexture(GL_TEXTURE_2D, FBOtex);
        //glGenerateMipmap(GL_TEXTURE_2D);
    }

    void render() {
        progress = 0;
        // Get current frame buffer size.
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height); // Clear framebuffer.
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        progfb->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, FBOtex);
        glBindVertexArray(VertexArrayID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
        progfb->unbind();
    }

};



}



int main(int argc, char **argv) {
    std::string resourceDir = "../resources"; // Where the resources are loaded from
    if (argc >= 2)  {
        resourceDir = argv[1];
    }

    Application *application = new Application();
    /* your main will always include a similar set up to establish your window
    and GL context, etc. */
    WindowManager * windowManager = new WindowManager();
    if (!windowManager->init(720, 480)) {
        std::cerr << "Failed to initialize window manager" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    windowManager->setEventCallbacks(application);
    application->windowManager = windowManager;

    /* This is the code that will likely change program to program as you
    may need to initialize or set up different data and state */
    // Initialize scene.
    application->init(resourceDir);
    application->initGeom();
    application->init_framebuffer();
    unsigned int swap = 1;
    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(windowManager->getHandle())) {
        // Render scene.
        application->render_to_framebuffer();

        if (progress) {
            application->compute_move_outline(swap);
            application->compute_draw_outline(swap);
            swap = !swap;
            application->compute_generate_outline(swap);
            application->debug_buff(swap);
        }
        application->compute_draw_outline(!swap);
        application->compute_reset(!swap); 
        application->debug_buff(swap);
        //render from the side here ...

        //get the whole thing on the screen
        application->render();

        // Swap front and back buffers.
        glfwSwapBuffers(windowManager->getHandle());
        // Poll for and process events.
        glfwPollEvents();
    }

    // Quit program.
    windowManager->shutdown();
    return 0;
}
