// Realtime Lift and Drag SURP 2018
// Christian Eckhart, William Newey, Austin Quick, Sebastian Seibert

// Allows program to be run on dedicated graphics processor for laptops with
// both integrated and dedicated graphics using Nvidia Optimus
#ifdef _WIN32
extern "C" {
    _declspec(dllexport) unsigned int NvOptimusEnablement(1);
}
#endif



#include "ClothSim.hpp"

#include <iostream>
#include <vector>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/constants.hpp"

#include "Common/Global.hpp"
#include "Common/Shader.hpp"
#include "Common/Camera.hpp"
#include "RLD/Simulation.hpp"
#include "UI/Group.hpp"
#include "UI/TexViewer.hpp"

#include "Clothier.hpp"
#include "ClothViewer.hpp"



static const ivec2 k_defWindowSize(1280, 720);
static const std::string k_windowTitle("Cloth Simulation");
static constexpr bool k_useAllCores(true); // Will utilize as many gpu cores as possible
static constexpr int k_coresToUse(1024); // Otherwise, use this many
static constexpr int k_warpSize(64); // Should correspond to target architecture
static const ivec2 k_warpSize2D(8, 8); // The components multiplied must equal warp size
static const float k_targetFPS(60.0f);
static const float k_targetDT(1.0f / k_targetFPS);
static const float k_updateDT(1.0f / 60.0f);

static constexpr bool k_doTri(true);
static constexpr bool k_doTouch(true);
static const ivec2 k_clothLOD(40, 40);
static const float k_clothLength(1.0f);
static const float k_weaveSize(k_clothLength / k_clothLOD.y);
static const vec2 k_clothSize(vec2(k_clothLOD) * k_weaveSize);
static const int k_triLOD(40);
static const float k_triClothSize(k_clothLength * 2.0f / std::sqrt(3.0f));
static const float k_triWeaveSize(k_triClothSize / k_triLOD);
static const vec3 k_gravity(0.0f, -9.8f, 0.0f);
static const int k_constraintPasses(16);

static constexpr bool k_doRLD(true);
static const int k_rldTexSize(1024);
static const int k_rldSliceCount(100);
static const float k_rldLiftK(1.0f);
static const float k_rldDragK(1.0f);
static const float k_windframeWidth(2.0f * k_clothLength * 1.125f);
static const float k_windframeDepth(k_clothLength * 1.125f);
static const float k_turbulenceDist(0.045f);
static const float k_windShadDist(0.1f);
static const float k_backforceC(1000000.0f);
static const float k_flowback(0.01f);
static const float k_initVelC(0.5f);

static const int k_minCompSize(128);

static int s_workGroupSize;
static ivec2 s_workGroupSize2D;

static unq<SoftModel> s_model;
static unq<Shader> s_clothShader;
static unq<Shader> s_normalShader;

static shr<ClothViewerComponent> s_viewerComp;
static shr<ui::TexViewer> s_frontTexComp;



static bool setupModel() {
    if (k_doTri) {
        s_model = Clothier::createTriangle(k_triLOD, k_triWeaveSize, s_workGroupSize);
    }
    else {
        s_model = Clothier::createRectangle(k_clothLOD, k_weaveSize, s_workGroupSize);
    }
    if (!s_model->load()) {
        std::cerr << "Failed to load soft model" << std::endl;
        return false;
    }
    return true;
}

static bool setupShaders() {
    std::string shadersPath(g_resourcesDir + "/ClothSim/shaders/");

    // Cloth shader
    if (!(s_clothShader = Shader::load(shadersPath + "cloth.comp", {
        { "WORK_GROUP_SIZE", std::to_string(s_workGroupSize) },
        { "CONSTRAINT_PASSES", std::to_string(k_constraintPasses) }
    }))) {
        std::cerr << "Failed to load cloth shader" << std::endl;
        return false;
    }
    s_clothShader->bind();
    s_clothShader->uniform("u_vertexCount", s_model->mesh().vertexCount());
    s_clothShader->uniform("u_constraintCount", s_model->mesh().constraintCount());
    s_clothShader->uniform("u_dt", k_updateDT);
    s_clothShader->uniform("u_gravity", k_gravity);
    s_clothShader->uniform("u_weaveSize", k_weaveSize);

    // Setup normal shader
    if (!(s_normalShader = Shader::load(shadersPath + "normal.comp", {{ "WORK_GROUP_SIZE", std::to_string(s_workGroupSize) }}))) {
        std::cerr << "Failed to load normal shader" << std::endl;
        return false;
    }
    s_normalShader->bind();
    s_normalShader->uniform("u_vertexCount", s_model->mesh().vertexCount());
    s_normalShader->uniform("u_indexCount", s_model->mesh().indexCount());

    Shader::unbind();
    return true;
}

static void setupUI() {
    s_viewerComp.reset(new ClothViewerComponent(*s_model, k_clothLength, ivec2(k_minCompSize)));
    s_frontTexComp.reset(new ui::TexViewer(rld::frontTex(), ivec2(rld::texSize()), ivec2(k_minCompSize)));

    shr<ui::HorizontalGroup> groupComp(new ui::HorizontalGroup());
    groupComp->add(s_frontTexComp);
    groupComp->add(s_viewerComp);

    ui::setRootComponent(groupComp);
}

static bool setup() {
    // Setup UI, which includes GLFW and GLAD
    if (!ui::setup(k_defWindowSize, "Realtime Lift and Drag Visualizer", 4, 5, false)) {
        std::cerr << "Failed to setup UI" << std::endl;
        return false;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_BLEND);
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

    // Setup model
    if (!setupModel()) {
        std::cerr << "Failed to setup model" << std::endl;
        return false;
    }

    // Setup shaders
    if (!setupShaders()) {
        std::cerr << "Failed to setup shaders" << std::endl;
        return false;
    }

    // Setup RLD
    if (!rld::setup(k_rldTexSize, k_rldSliceCount, k_rldLiftK, k_rldDragK, k_turbulenceDist, 2.0f * k_turbulenceDist, k_windShadDist, k_backforceC, k_flowback, k_initVelC)) {
        std::cerr << "Failed to setup RLD" << std::endl;
        return false;
    }

    setupUI();

    return true;
}

static void update() {
    static float s_time(0.0f);
    
    // Do RLD
    //rld::set(*s_model, mat4(), mat3(), k_windframeWidth, k_windframeDepth, k_windSpeed, true);

    s_time = glm::fract(s_time * 0.2f) * 5.0f;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_model->mesh().vertexBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_model->mesh().indexBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_model->mesh().constraintBuffer());

    // Do cloth physics
    s_clothShader->bind();
    s_clothShader->uniform("u_time", s_time * 0.2f);
    if (k_doTouch) {
        if (s_viewerComp->isTouch()) {
            vec2 aspect(s_viewerComp->aspect());
            const ThirdPersonCamera & camera(s_viewerComp->camera());
            s_clothShader->uniform("u_isTouch", true);
            s_clothShader->uniform("u_touchPos", s_viewerComp->touchPoint());
            s_clothShader->uniform("u_touchDir", -camera.w());
            s_clothShader->uniform("u_touchMat", camera.projMat() * camera.viewMat());
            s_clothShader->uniform("u_aspect", aspect);
        }
        else {
            s_clothShader->uniform("u_isTouch", false);
        }
    }
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: is this necessary?

    // Calculate normals
    s_normalShader->bind();
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: is this necessary?

    Shader::unbind();

    s_time += k_targetDT;
}



const SoftModel & model() {
    return *s_model;
}

float windframeWidth() {
    return k_windframeWidth;
}

float windframeDepth() {
    return k_windframeDepth;
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
    while (!ui::shouldExit()) {
        double now(glfwGetTime());
        float dt(float(now - then));
        then = now;
        accumDT += dt;

        ui::poll();

        if (accumDT >= k_targetDT) {
            float renderDT(float(now - prevRenderTime));
            prevRenderTime = now;
            //std::cout << (1.0f / renderDT) << std::endl;
            update();
            ui::update();
            ui::render();
            do accumDT -= k_targetDT; while (accumDT >= k_targetDT);
        }
    }

    return EXIT_SUCCESS;
}
