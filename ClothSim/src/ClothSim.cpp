// Realtime Lift and Drag SURP 2018
// Christian Eckhart, William Newey, Austin Quick, Sebastian Seibert

// Allows program to be run on dedicated graphics processor for laptops with
// both integrated and dedicated graphics using Nvidia Optimus
#ifdef _WIN32
extern "C" {
    _declspec(dllexport) unsigned int NvOptimusEnablement(1);
}
#endif



#include <iostream>
#include <vector>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/constants.hpp"

#include "Common/Global.hpp"
#include "Common/SoftModel.hpp"
#include "Common/Shader.hpp"
#include "Common/Camera.hpp"
#include "RLD/Simulation.hpp"

#include "ClothMesher.hpp"



static const ivec2 k_defWindowSize(1280, 720);
static const std::string k_windowTitle("Cloth Simulation");
static constexpr bool k_useAllCores(true); // Will utilize as many gpu cores as possible
static constexpr int k_coresToUse(1024); // Otherwise, use this many
static constexpr int k_warpSize(64); // Should correspond to target architecture
static const ivec2 k_warpSize2D(8, 8); // The components multiplied must equal warp size
static constexpr float k_targetFPS(60.0f);
static constexpr float k_targetDT(1.0f / k_targetFPS);
static constexpr float k_updateDT(1.0f / 60.0f);

static constexpr bool k_doTri(false);
static const ivec2 k_clothLOD(40, 20);
static const float k_clothSizeMajor(1.0f);
static const float k_weaveSize(k_clothSizeMajor / glm::max(k_clothLOD.x, k_clothLOD.y));
static const int k_triLOD(3);
static const float k_triWeaveSize(1.0f / k_triLOD);
static const vec3 k_gravity(0.0f, -9.8f, 0.0f);
static constexpr int k_constraintPasses(16);

static constexpr float k_fov(glm::radians(90.0f));
static constexpr float k_near(0.01f), k_far(100.0f);
static const vec3 k_lightDir(glm::normalize(vec3(1.0f)));
static constexpr float k_minCamDist(0.5f), k_maxCamDist(3.0f);
static constexpr float k_camPanAngle(0.01f);
static const float k_camZoomAmount(0.1f);

static GLFWwindow * s_window;
static ivec2 s_windowSize(k_defWindowSize);
static int s_workGroupSize;
static ivec2 s_workGroupSize2D;

static unq<SoftModel> s_model;
static unq<Shader> s_renderShader;
static unq<Shader> s_updateShader;
static unq<Shader> s_normalShader;
static unq<Shader> s_constraintShader;
static ThirdPersonCamera s_camera(k_minCamDist, k_maxCamDist);
static u32 s_constraintVAO;



static void errorCallback(int error, const char * description) {
    std::cerr << "GLFW error " << error << ": " << description << std::endl;
}

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    // Escape exits
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
        glfwSetWindowShouldClose(s_window, true);
    }
}

static ivec2 s_prevCursorPos;
static bool s_isPrevCursorPosValid(false);

void cursorPosCallback(GLFWwindow * window, double x, double y) {
    ivec2 cursorPos{int(x), int(y)};
    if (!s_isPrevCursorPosValid) {
        s_prevCursorPos = cursorPos;
        s_isPrevCursorPosValid = true;
    }
    ivec2 delta(cursorPos - s_prevCursorPos);

    if (glfwGetMouseButton(s_window, 0)) {
        s_camera.thetaPhi(
            s_camera.theta() - float(delta.x) * k_camPanAngle,
            s_camera.phi() - float(delta.y) * k_camPanAngle
        );
    }

    s_prevCursorPos = cursorPos;
}

void cursorEnterCallback(GLFWwindow * window, int entered) {
    if (!entered) {
        s_isPrevCursorPosValid = false;
    }
}

void scrollCallback(GLFWwindow * window, double dx, double dy) {    
    s_camera.zoom(s_camera.zoom() - float(dy) * k_camZoomAmount);
}

static void framebufferSizeCallback(GLFWwindow * window, int width, int height) {
    s_windowSize.x = width;
    s_windowSize.y = height;

    if (s_windowSize.y <= s_windowSize.x) {
        s_camera.fov(vec2(k_fov, k_fov * float(s_windowSize.y) / float(s_windowSize.x)));
    }
    else {
        s_camera.fov(vec2(k_fov * float(s_windowSize.x) / float(s_windowSize.y), k_fov));
    }
}

static bool setup() {
    // Setup window
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    if (!(s_window = glfwCreateWindow(s_windowSize.x, s_windowSize.y, k_windowTitle.c_str(), nullptr, nullptr))) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    glfwMakeContextCurrent(s_window);
    glfwSwapInterval(0); // VSync on or off
    glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(s_window, keyCallback);
    glfwSetCursorPosCallback(s_window, cursorPosCallback);
    glfwSetCursorEnterCallback(s_window, cursorEnterCallback);
    glfwSetScrollCallback(s_window, scrollCallback);
    glfwSetFramebufferSizeCallback(s_window, framebufferSizeCallback);

    // Setup GLAD
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glViewport(0, 0, s_windowSize.x, s_windowSize.y);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

    // Setup work group size, needs to happen as early as possible
    int coreCount(0);
    if (k_useAllCores) glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &coreCount);
    else coreCount = k_coresToUse;
    int warpCount(coreCount / k_warpSize);
    s_workGroupSize = warpCount * k_warpSize; // we want workgroups to be a warp multiple
    s_workGroupSize2D.x = int(std::round(std::sqrt(float(warpCount))));
    s_workGroupSize2D.y = warpCount / s_workGroupSize2D.x;
    s_workGroupSize2D *= k_warpSize2D;

    // Setup mesh
    if (k_doTri) {
        s_model = createClothTriangle(k_triLOD, k_triWeaveSize);
    }
    else {
        s_model = createClothRectangle(k_clothLOD, k_weaveSize);
    }
    if (!s_model->load()) {
        std::cerr << "Failed to load soft mesh" << std::endl;
        return false;
    }

    // Setup camera
    s_camera.zoom(0.5f);
    s_camera.near(k_near);
    s_camera.far(k_far);
    if (s_windowSize.y <= s_windowSize.x) {
        s_camera.fov(vec2(k_fov * float(s_windowSize.x) / float(s_windowSize.y), k_fov));
    }
    else {
        s_camera.fov(vec2(k_fov, k_fov * float(s_windowSize.y) / float(s_windowSize.x)));
    }

    // Setup RLD
    //if (!rld::setup(k_simTexSize, k_simSliceCount, k_simLiftC, k_simDragC, s_turbulenceDist, s_maxSearchDist, s_windShadDist, s_backforceC, s_flowback, s_initVelC)) {
    //    std::cerr << "Failed to setup RLD" << std::endl;
    //    return false;
    //}

    // Setup render shader
    std::string shadersPath(g_resourcesDir + "/ClothSim/shaders/");
    if (!(s_renderShader = Shader::load(shadersPath + "render.vert", shadersPath + "render.frag"))) {
        std::cerr << "Failed to load render shader" << std::endl;
        return false;
    }
    s_renderShader->bind();
    s_renderShader->uniform("u_lightDir", k_lightDir);
    s_renderShader->uniform("u_primitiveCount", k_clothLOD.x * k_clothLOD.y * 2);
    Shader::unbind();
    // Setup update shader
    if (!(s_updateShader = Shader::load(shadersPath + "update.comp", {
        { "WORK_GROUP_SIZE", std::to_string(s_workGroupSize) },
        { "CONSTRAINT_PASSES", std::to_string(k_constraintPasses) }
    }))) {
        std::cerr << "Failed to load update shader" << std::endl;
        return false;
    }
    s_updateShader->bind();
    s_updateShader->uniform("u_vertexCount", s_model->mesh().vertexCount());
    s_updateShader->uniform("u_constraintCount", s_model->mesh().constraintCount());
    s_updateShader->uniform("u_dt", k_updateDT);
    s_updateShader->uniform("u_gravity", k_gravity);
    Shader::unbind();
    // Setup normal shader
    if (!(s_normalShader = Shader::load(shadersPath + "normal.comp", {{ "WORK_GROUP_SIZE", std::to_string(s_workGroupSize) }}))) {
        std::cerr << "Failed to load normal shader" << std::endl;
        return false;
    }
    s_normalShader->bind();
    s_normalShader->uniform("u_vertexCount", s_model->mesh().vertexCount());
    s_normalShader->uniform("u_indexCount", s_model->mesh().indexCount());
    Shader::unbind();
    // Setup constraint shader
    if (!(s_constraintShader = Shader::load(shadersPath + "constraint.vert", shadersPath + "constraint.geom", shadersPath + "constraint.frag"))) {
        std::cerr << "Failed to load constraint shader" << std::endl;
        return false;
    }

    glGenVertexArrays(1, &s_constraintVAO);

    return true;
}

static void update() {
    static float s_time(0.0f);

    s_time = glm::fract(s_time * 0.2f) * 5.0f;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_model->mesh().vertexBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_model->mesh().indexBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_model->mesh().constraintBuffer());

    s_updateShader->bind();
    s_updateShader->uniform("u_time", s_time * 0.2f);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: is this necessary?

    s_normalShader->bind();
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: is this necessary?

    Shader::unbind();

    s_time += k_targetDT;
}

static void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    //glDisable(GL_BLEND);

    //rld::set(*s_model, rldModelMat, rldNormalMat, k_windframeWidth, k_windframeDepth, glm::length(wind), false);
    //if (s_unpaused) {
    //    rld::sweep();
    //}

    //glEnable(GL_BLEND);

    glViewport(0, 0, s_windowSize.x, s_windowSize.y);

    //s_renderShader->bind();
    //s_renderShader->uniform("u_modelMat", mat4());
    //s_renderShader->uniform("u_normalMat", mat3());
    //s_renderShader->uniform("u_viewMat", s_camera.viewMat());
    //s_renderShader->uniform("u_projMat", s_camera.projMat());
    //s_renderShader->uniform("u_camPos", s_camera.position());
    //s_model->draw();

    s_constraintShader->bind();
    s_constraintShader->uniform("u_modelMat", mat4());
    s_constraintShader->uniform("u_normalMat", mat3());
    s_constraintShader->uniform("u_viewMat", s_camera.viewMat());
    s_constraintShader->uniform("u_projMat", s_camera.projMat());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_model->mesh().vertexBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_model->mesh().constraintBuffer());
    glBindVertexArray(s_constraintVAO);
    glDrawArrays(GL_POINTS, 0, s_model->mesh().constraintCount());
    glBindVertexArray(0);

    Shader::unbind();

    glDisable(GL_DEPTH_TEST);
}



int main(int argc, char ** argv) {
    if (!setup()) {
        std::cerr << "Failed setup" << std::endl;
        std::cin.get();
        return EXIT_FAILURE;
    }

    double then(glfwGetTime());
    double prevRenderTime(then);
    float accumDT(k_targetDT);
    while (!glfwWindowShouldClose(s_window)) {
        double now(glfwGetTime());
        float dt(float(now - then));
        then = now;
        accumDT += dt;

        glfwPollEvents();

        if (accumDT >= k_targetDT) {
            float renderDT(float(now - prevRenderTime));
            prevRenderTime = now;
            //std::cout << (1.0f / renderDT) << std::endl;
            if (!k_doTri) update();
            render();
            glfwSwapBuffers(s_window);
            do accumDT -= k_targetDT; while (accumDT >= k_targetDT);
        }
    }

    return EXIT_SUCCESS;
}
