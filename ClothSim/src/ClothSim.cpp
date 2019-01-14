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
#include "Common/Util.hpp"
#include "RLD/RLD.hpp"
#include "UI/Group.hpp"
#include "UI/TexViewer.hpp"

#include "Clothier.hpp"
#include "ClothViewer.hpp"



enum class Object { flag, sail };

enum class State { free, sweeping, autoStepping, stepping };



static constexpr Object k_object(Object::sail);

static const ivec2 k_flagLOD(50, 25);
static const float k_flagLength(1.0f);
static const float k_flagWeaveSize(k_flagLength / float(k_flagLOD.x));
static const vec2 k_flagSize(vec2(k_flagLOD) * k_flagWeaveSize);
static const float k_flagAreaDensity(0.25f); // kg per square meter

static const int k_sailLOD(50);
static const float k_sailLuffLength(13.0f);
static const float k_sailLeechLength(12.0f);
static const float k_sailFootLength(8.0f);
static const float k_sailLuffAngle(glm::radians(30.0f)); // Angle off vertical
static const float k_sailAreaDensity(0.35f); // kg per square meter (about right for 8 oz. sail)
static const float k_sailRotateSpeed(glm::pi<float>() / 3.0f);
static const float k_sailDrawSpeed(0.25f);
static const float k_sailDrawMin(k_sailLuffLength / (k_sailLeechLength + k_sailFootLength) * 1.01f);
static const float k_sailDrawMax(1.5f);

static constexpr bool k_doTouch(true);
static const vec3 k_gravity(0.0f, -9.8f, 0.0f);
static const int k_constraintPasses(16);

static const ivec2 k_defWindowSize(1280, 720);
static const std::string k_windowTitle("Cloth Simulation");
static constexpr bool k_useAllCores(true); // Will utilize as many gpu cores as possible
static constexpr int k_coresToUse(1024); // Otherwise, use this many
static constexpr int k_warpSize(64); // Should correspond to target architecture
static const ivec2 k_warpSize2D(8, 8); // The components multiplied must equal warp size
static const float k_targetFPS(60.0f);
static const float k_targetDT(1.0f / k_targetFPS);
static const float k_updateDT(1.0f / 60.0f);

static const int k_rldTexSize(1024);
static const int k_rldSliceCount(100);

static const int k_minCompSize(128);



static float s_clothScale;
static float s_clothMass;
static float s_touchForce;

static mat4 s_sailRotateCCWMat, s_sailRotateCWMat;
static float s_sailArea;

static float s_rldLiftC;
static float s_rldDragC;
static float s_windframeWidth;
static float s_windframeDepth;
static float s_windSpeed;
static float s_turbulenceDist;
static float s_windShadDist;
static float s_backforceC;
static float s_flowback;
static float s_initVelC;

static int s_workGroupSize;
static ivec2 s_workGroupSize2D;

static unq<Model> s_model;
static const SoftMesh * s_mesh;
static unq<Shader> s_clothShader;
static unq<Shader> s_normalShader;
static unq<Shader> s_transformShader;

static shr<ClothViewerComponent> s_viewerComp;
static shr<ui::TexViewer> s_frontTexComp;

static float s_sailDraw(1.0f);
static float s_sailAngle(0.0f);
static vec3 s_sailTack, s_sailHead, s_sailClew;

static State s_state(State::free);
static bool s_doStep(false);
static bool s_sailRotateCCW(false), s_sailRotateCW(false);
static bool s_sailClewIn(false), s_sailClewOut(false);
static bool s_isClothUpdateNeeded(false);



class RootComp : public ui::HorizontalGroup {

    public:

    virtual void keyEvent(int key, int action, int mods) override {
        if (key == GLFW_KEY_Z && action == GLFW_RELEASE && !mods) {
            s_state = State::free;
        }
        else if (key == GLFW_KEY_X && action == GLFW_RELEASE && !mods) {
            s_state = State::sweeping;
        }
        else if (key == GLFW_KEY_C && action == GLFW_RELEASE && !mods) {
            s_state = State::autoStepping;
        }
        else if (key == GLFW_KEY_SPACE && !mods) {
            if (s_state == State::stepping) {
                if (action != GLFW_RELEASE) {
                    s_doStep = true;
                }
            }
            else {
                if (action == GLFW_RELEASE) {
                    s_state = State::stepping;
                }
            }
        }
        // Rotate sail CCW
        else if (key == GLFW_KEY_LEFT) {
            if (action == GLFW_PRESS) s_sailRotateCCW = true;
            else if (action == GLFW_RELEASE) s_sailRotateCCW = false;
        }
        // Rotate sail CW
        else if (key == GLFW_KEY_RIGHT) {
            if (action == GLFW_PRESS) s_sailRotateCW = true;
            else if (action == GLFW_RELEASE) s_sailRotateCW = false;
        }
        // Draw in clew
        else if (key == GLFW_KEY_DOWN) {
            if (action == GLFW_PRESS) s_sailClewIn = true;
            else if (action == GLFW_RELEASE) s_sailClewIn = false;
        }
        // Draw out clew
        else if (key == GLFW_KEY_UP) {
            if (action == GLFW_PRESS) s_sailClewOut = true;
            else if (action == GLFW_RELEASE) s_sailClewOut = false;
        }
    }

};

static void detSailPoints() {
    vec2 head2D(k_sailLuffLength * std::sin(k_sailLuffAngle), k_sailLuffLength * std::cos(k_sailLuffAngle));
    vec2 clew2D(util::intersectCircles(head2D, k_sailLeechLength * s_sailDraw, vec2(), k_sailFootLength * s_sailDraw).first);
    float sinAngle(std::sin(s_sailAngle));
    float cosAngle(std::cos(s_sailAngle));
    s_sailHead = vec3(-head2D.x * sinAngle, head2D.y, -head2D.x * cosAngle);
    s_sailClew = vec3(-clew2D.x * sinAngle, clew2D.y, -clew2D.x * cosAngle);
    s_sailHead += s_sailTack;
    s_sailClew += s_sailTack;
}

static bool setupObject() {
    if (k_object == Object::flag) {
        s_clothScale = k_flagLength;
        float clothArea(k_flagSize.x * k_flagSize.y);
        s_clothMass = clothArea * k_flagAreaDensity;

        mat4 transform;
        transform = glm::translate(vec3(-0.5f * k_flagSize, 0.0f)) * transform;
        transform = glm::rotate(glm::pi<float>() * 0.5f, vec3(0.0f, 1.0f, 0.0f)) * transform;
        s_model = Clothier::createRectangle(k_flagLOD, k_flagWeaveSize, s_clothMass, s_workGroupSize, transform, bvec4(false, false, false, false), bvec4(false, false, true, true));

        s_rldLiftC = 1.0f;
        s_rldDragC = 1.0f;
        s_windframeWidth = 2.0f * k_flagLength * 1.125f;
        s_windframeDepth = k_flagLength * 1.125f;
        s_windSpeed = 10.0f;
        s_turbulenceDist = 0.5f;
        s_windShadDist = 0.1f;
        s_backforceC = 10000.0f;
        s_flowback = 0.01f;
        s_initVelC = 1.0f;
    }
    else if (k_object == Object::sail) {
        vec2 head2D(k_sailLuffLength * std::sin(k_sailLuffAngle), k_sailLuffLength * std::cos(k_sailLuffAngle));
        vec2 clew2D(util::intersectCircles(head2D, k_sailLeechLength, vec2(), k_sailFootLength).first);
        float height(glm::max(head2D.y, head2D.y - clew2D.y));
        float depth(glm::max(head2D.x, clew2D.x));
        s_sailTack = vec3(0.0f, 0.5f * height - head2D.y, depth * 0.5f);
        s_sailHead = vec3(0.0f, s_sailTack.y + head2D.y, s_sailTack.z - head2D.x);
        s_sailClew = vec3(0.0f, s_sailTack.y + clew2D.y, s_sailTack.z - clew2D.x);

        s_clothScale = k_sailLuffLength;
        float hp((k_sailLuffLength + k_sailLeechLength + k_sailFootLength) * 0.5f);
        s_sailArea = std::sqrt(hp * (hp - k_sailLuffLength) * (hp - k_sailLeechLength) * (hp - k_sailFootLength));
        s_clothMass = s_sailArea * k_sailAreaDensity;

        s_sailRotateCCWMat = glm::translate(-s_sailTack);
        s_sailRotateCCWMat = glm::rotate(k_sailRotateSpeed * k_updateDT, vec3(0.0f, 1.0f, 0.0f)) * s_sailRotateCCWMat;
        s_sailRotateCCWMat = glm::translate(s_sailTack) * s_sailRotateCCWMat;

        s_sailRotateCWMat = glm::translate(-s_sailTack);
        s_sailRotateCWMat = glm::rotate(-k_sailRotateSpeed * k_updateDT, vec3(0.0f, 1.0f, 0.0f)) * s_sailRotateCWMat;
        s_sailRotateCWMat = glm::translate(s_sailTack) * s_sailRotateCWMat;

        // Create the mesh
        unq<SoftMesh> mesh(Clothier::createTriangle(s_sailTack, s_sailClew, s_sailHead, k_sailLOD, s_clothMass, s_workGroupSize, bvec3(true, true, true), bvec3(false, false, true)));
        // Assign luff to group 1
        for (int i(0); i <= k_sailLOD; ++i) {
            mesh->vertices()[Clothier::triI(ivec2(i, i))].group = 1;
        }
        // Assign clew to group 2
        mesh->vertices()[Clothier::triI(ivec2(0, k_sailLOD))].group = 2;

        if (!mesh->load()) {
            std::cerr << "Failed to load mesh" << std::endl;
            return false;
        }
        s_model.reset(new Model(SubModel("Sail", move(mesh))));
        s_mesh = &static_cast<const SoftMesh &>(s_model->subModels().front().mesh());

        // Set RLD constants
        s_rldLiftC = 1.0f;
        s_rldDragC = 1.0f;
        s_windframeWidth = glm::max(depth * 2, height) * 1.125f;
        s_windframeDepth = depth * 1.125f;
        s_windSpeed = 10.0f;
        s_turbulenceDist = 0.5f;
        s_windShadDist = 0.1f;
        s_backforceC = 10000.0f;
        s_flowback = 0.01f;
        s_initVelC = 1.0f;
    }

    if (!s_model) {
        return false;
    }
    s_mesh = &static_cast<const SoftMesh &>(s_model->subModels().front().mesh());
    s_touchForce = s_clothMass / s_mesh->vertexCount() * 20.0f;
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
    s_clothShader->uniform("u_vertexCount", s_mesh->vertexCount());
    s_clothShader->uniform("u_constraintCount", s_mesh->constraintCount());
    s_clothShader->uniform("u_dt", k_updateDT);
    s_clothShader->uniform("u_gravity", k_gravity);
    Shader::unbind();

    // Setup normal shader
    if (!(s_normalShader = Shader::load(shadersPath + "normal.comp", {{ "WORK_GROUP_SIZE", std::to_string(s_workGroupSize) }}))) {
        std::cerr << "Failed to load normal shader" << std::endl;
        return false;
    }
    s_normalShader->bind();
    s_normalShader->uniform("u_vertexCount", s_mesh->vertexCount());
    s_normalShader->uniform("u_indexCount", s_mesh->indexCount());
    Shader::unbind();

    // Setup transform shader
    if (!(s_transformShader = Shader::load(shadersPath + "transform.comp", { { "WORK_GROUP_SIZE", std::to_string(s_workGroupSize) } }))) {
        std::cerr << "Failed to load transform shader" << std::endl;
        return false;
    }
    s_transformShader->bind();
    s_transformShader->uniform("u_vertexCount", s_mesh->vertexCount());
    Shader::unbind();

    return true;
}

static void setupUI() {
    s_viewerComp.reset(new ClothViewerComponent(*s_model, s_clothScale, ivec2(k_minCompSize)));
    s_frontTexComp.reset(new ui::TexViewer(rld::frontTex(), ivec2(rld::texSize()), ivec2(k_minCompSize)));

    shr<RootComp> rootComp(new RootComp());
    rootComp->add(s_frontTexComp);
    rootComp->add(s_viewerComp);

    ui::setRootComponent(rootComp);
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

    // Setup object
    if (!setupObject()) {
        std::cerr << "Failed to setup object" << std::endl;
        return false;
    }

    // Setup shaders
    if (!setupShaders()) {
        std::cerr << "Failed to setup shaders" << std::endl;
        return false;
    }

    // Setup RLD
    if (!rld::setup(
        k_rldTexSize,
        k_rldSliceCount,
        s_rldLiftC,
        s_rldDragC,
        s_turbulenceDist,
        2.0f * s_turbulenceDist,
        s_windShadDist,
        s_backforceC,
        s_flowback,
        s_initVelC,
        false,
        true
    )) {
        std::cerr << "Failed to setup RLD" << std::endl;
        return false;
    }
    rld::set(*s_model, mat4(), mat3(), s_windframeWidth, s_windframeDepth, s_windSpeed, true);

    setupUI();

    return true;
}

static void updateCloth() {
    static float s_time(0.0f);
    s_time = glm::fract(s_time * 0.2f) * 5.0f;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_mesh->vertexBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_mesh->indexBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_mesh->constraintBuffer());

    // Transform sail
    if (k_object == Object::sail) {
        // Handle rotation
        if (s_sailRotateCCW != s_sailRotateCW) {
            s_sailAngle += (s_sailRotateCCW ? k_sailRotateSpeed : -k_sailRotateSpeed) * k_updateDT;
            detSailPoints();

            s_transformShader->bind();
            s_transformShader->uniform("u_groups", ivec4(1, 1, 1, 2));
            s_transformShader->uniform("u_transformMat", s_sailRotateCCW ? s_sailRotateCCWMat : s_sailRotateCWMat);
            glDispatchCompute(1, 1, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: is this necessary?
            Shader::unbind();
        }
        // Handle clew draw
        if (s_sailClewIn != s_sailClewOut) {
            float deltaDraw(k_sailDrawSpeed * k_updateDT);
            if (s_sailClewIn) deltaDraw = -deltaDraw;
            float newDraw(glm::clamp(s_sailDraw + deltaDraw, k_sailDrawMin, k_sailDrawMax));
            deltaDraw = newDraw - s_sailDraw;
            if (!util::isZero(deltaDraw)) {
                vec3 prevClew(s_sailClew);
                s_sailDraw = newDraw;
                detSailPoints();

                s_transformShader->bind();
                s_transformShader->uniform("u_groups", ivec4(2));
                s_transformShader->uniform("u_transformMat", glm::translate(s_sailClew - prevClew));
                glDispatchCompute(1, 1, 1);
                glMemoryBarrier(GL_ALL_BARRIER_BITS); // TODO: is this necessary?
                Shader::unbind();
            }
        }
    }

    // Do cloth physics
    s_clothShader->bind();
    s_clothShader->uniform("u_time", s_time * 0.2f);
    if (k_doTouch) {
        if (s_viewerComp->isTouch()) {
            vec2 aspect(s_viewerComp->aspect());
            const ThirdPersonCamera & camera(s_viewerComp->camera());
            s_clothShader->uniform("u_isTouch", true);
            s_clothShader->uniform("u_touchPos", s_viewerComp->touchPoint());
            s_clothShader->uniform("u_touchForce", -camera.w() * s_touchForce);
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

static void update() {
    if (s_state == State::free) {
        // Finish if mid-sweep
        if (rld::slice()) {
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            while (!rld::step());

            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
        }

        s_isClothUpdateNeeded = true;
    }
    else if (s_state == State::sweeping) {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        // Finish if mid-sweep
        if (rld::slice()) {
            while (!rld::step());
        }
        else {
            rld::sweep();
        }

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        s_isClothUpdateNeeded = true;

    }
    else if (s_state == State::autoStepping || s_state == State::stepping && s_doStep) {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        bool done(rld::step());

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        s_doStep = false;

        if (done) s_isClothUpdateNeeded = true;
    }
}



const Model & model() {
    return *s_model;
}

float windframeWidth() {
    return s_windframeWidth;
}

float windframeDepth() {
    return s_windframeDepth;
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
            if (s_isClothUpdateNeeded) {
                updateCloth();
                s_isClothUpdateNeeded = false;
            }
            do accumDT -= k_targetDT; while (accumDT >= k_targetDT);
        }
    }

    return EXIT_SUCCESS;
}
