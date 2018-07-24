#include "Results.hpp"

#include <iostream>
#include <memory>
#include <map>
#include <sstream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#include "GLSL.h"
#include "Program.h"
#include "Graph.hpp"
#include "Text.hpp"



namespace results {



static constexpr float k_granularity(1.0f); // results kept in increments of this many degrees
static constexpr float k_invGranularity(1.0f / k_granularity);

static const glm::ivec2 k_initWindowSize(400, 600);
static constexpr float k_initGraphDomainMin(-90.0f), k_initGraphDomainMax(90.0f);
static constexpr float k_initGraphRangeMin(-1.0f), k_initGraphRangeMax(1.0f);
static constexpr float k_zoomFactor(1.1f);
static constexpr int k_leftMargin(80), k_rightMargin(40), k_bottomMargin(20), k_topMargin(20);
static constexpr int k_valTextPrecision(3);
static constexpr int k_cursorTextProximity(20);



static std::map<float, std::pair<float, float>> f_record; // map of results where key is angle and value is lift/drag pair
static bool f_isChange;

static GLFWwindow * f_window;
static glm::ivec2 f_windowSize;
static bool f_mouseButtonDown;
static glm::ivec2 f_mousePos;

static std::unique_ptr<Graph> f_liftGraph, f_dragGraph;
static glm::ivec2 f_liftGraphPos, f_dragGraphPos, f_graphSize;

static std::unique_ptr<Text> f_liftDomainMinText, f_liftDomainMaxText;
static std::unique_ptr<Text> f_liftRangeMinText, f_liftRangeMaxText;
static std::unique_ptr<Text> f_dragDomainMinText, f_dragDomainMaxText;
static std::unique_ptr<Text> f_dragRangeMinText, f_dragRangeMaxText;
static bool f_isGridTextUpdateNeeded;

static std::unique_ptr<Text> f_cursorText;
static bool f_isCursorTextUpdateNeeded;

static std::unique_ptr<Text> f_liftTitleText, f_dragTitleText;
static bool f_isTitleTextUpdateNeeded;



static void detGraphBounds() {
    f_graphSize.x = f_windowSize.x - k_leftMargin - k_rightMargin;
    f_graphSize.y = (f_windowSize.y - 2 * (k_bottomMargin + k_topMargin)) / 2;

    f_liftGraphPos.x = k_leftMargin;
    f_liftGraphPos.y = k_bottomMargin + f_graphSize.y + k_topMargin + k_bottomMargin;

    f_dragGraphPos.x = k_leftMargin;
    f_dragGraphPos.y = k_bottomMargin;
}

static std::string valToString(float v) {
    std::stringstream ss;
    ss.precision(k_valTextPrecision);
    ss << v;
    return ss.str();
}

static std::string pointToString(float angle, float lift, float drag) {
    std::stringstream ss;
    ss.precision(k_valTextPrecision);
    ss << "Angle:" << angle << "°\nLift:" << lift << "N\nDrag:" << drag << "N";
    return ss.str();
}

static bool isCursorInLiftGraph() {    
    glm::ivec2 relPos(f_mousePos - f_liftGraphPos);
    return relPos.x >= 0 && relPos.y >= 0 && relPos.x < f_graphSize.x && relPos.y < f_graphSize.y;
}

static bool isCursorInDragGraph() {    
    glm::ivec2 relPos(f_mousePos - f_dragGraphPos);
    return relPos.x >= 0 && relPos.y >= 0 && relPos.x < f_graphSize.x && relPos.y < f_graphSize.y;
}

static void updateGridText() {
    // Lift

    glm::vec2 gridMin(glm::ceil(f_liftGraph->viewMin() / f_liftGraph->gridSize()) * f_liftGraph->gridSize());
    glm::vec2 gridMax(glm::floor(f_liftGraph->viewMax() / f_liftGraph->gridSize()) * f_liftGraph->gridSize());
    glm::vec2 invViewSize(1.0f / (f_liftGraph->viewMax() - f_liftGraph->viewMin()));
    glm::vec2 graphSize(f_graphSize);
    glm::ivec2 gridMinPos(glm::round((gridMin - f_liftGraph->viewMin()) * invViewSize * graphSize));
    glm::ivec2 gridMaxPos(glm::round((gridMax - f_liftGraph->viewMin()) * invViewSize * graphSize));
    
    f_liftDomainMinText->position(glm::ivec2(f_liftGraphPos.x + gridMinPos.x, f_liftGraphPos.y));
    f_liftDomainMinText->string(valToString(gridMin.x));
    f_liftDomainMaxText->position(glm::ivec2(f_liftGraphPos.x + gridMaxPos.x, f_liftGraphPos.y));
    f_liftDomainMaxText->string(valToString(gridMax.x));

    f_liftRangeMinText->position(glm::ivec2(f_liftGraphPos.x, f_liftGraphPos.y + gridMinPos.y));
    f_liftRangeMinText->string(valToString(gridMin.y));
    f_liftRangeMaxText->position(glm::ivec2(f_liftGraphPos.x, f_liftGraphPos.y + gridMaxPos.y));
    f_liftRangeMaxText->string(valToString(gridMax.y));

    // Drag
    
    gridMin = glm::ceil(f_dragGraph->viewMin() / f_dragGraph->gridSize()) * f_dragGraph->gridSize();
    gridMax = glm::floor(f_dragGraph->viewMax() / f_dragGraph->gridSize()) * f_dragGraph->gridSize();
    invViewSize = 1.0f / (f_dragGraph->viewMax() - f_dragGraph->viewMin());
    gridMinPos = glm::round((gridMin - f_dragGraph->viewMin()) * invViewSize * graphSize);
    gridMaxPos = glm::round((gridMax - f_dragGraph->viewMin()) * invViewSize * graphSize);
    
    f_dragDomainMinText->position(glm::ivec2(f_dragGraphPos.x + gridMinPos.x, f_dragGraphPos.y));
    f_dragDomainMinText->string(valToString(gridMin.x));
    f_dragDomainMaxText->position(glm::ivec2(f_dragGraphPos.x + gridMaxPos.x, f_dragGraphPos.y));
    f_dragDomainMaxText->string(valToString(gridMax.x));

    f_dragRangeMinText->position(glm::ivec2(f_dragGraphPos.x, f_dragGraphPos.y + gridMinPos.y));
    f_dragRangeMinText->string(valToString(gridMin.y));
    f_dragRangeMaxText->position(glm::ivec2(f_dragGraphPos.x, f_dragGraphPos.y + gridMaxPos.y));
    f_dragRangeMaxText->string(valToString(gridMax.y));
}

static void updateCursorText() {
    f_cursorText->string("");
    f_liftGraph->removeFocusPoint();
    f_dragGraph->removeFocusPoint();

    bool isLift;
    if (isCursorInLiftGraph()) {
        isLift = true;
    }
    else if (isCursorInDragGraph()) {
        isLift = false;
    }
    else {
        return;
    }

    float angle;
    if (isLift) {
        angle = float(f_mousePos.x - f_liftGraphPos.x);
        angle /= f_graphSize.x;
        angle *= f_liftGraph->viewMax().x - f_liftGraph->viewMin().x;
        angle += f_liftGraph->viewMin().x;
    }
    else {
        angle = float(f_mousePos.x - f_dragGraphPos.x);
        angle /= f_graphSize.x;
        angle *= f_dragGraph->viewMax().x - f_dragGraph->viewMin().x;
        angle += f_dragGraph->viewMin().x;
    }

    float lift, drag;
    if (!valAt(angle, lift, drag)) {
        return;
    }

    f_cursorText->position(f_mousePos);
    f_cursorText->string(pointToString(angle, lift, drag));
    f_liftGraph->focusPoint(glm::vec2(angle, lift));
    f_dragGraph->focusPoint(glm::vec2(angle, drag));
}

static void updateTitleText() {
    f_liftTitleText->position(glm::ivec2(f_liftGraphPos.x + f_graphSize.x / 2, f_liftGraphPos.y + f_graphSize.y));
    f_dragTitleText->position(glm::ivec2(f_dragGraphPos.x + f_graphSize.x / 2, f_dragGraphPos.y + f_graphSize.y));
}

static void framebufferSizeCallback(GLFWwindow * window, int width, int height) {
    f_windowSize.x = width; f_windowSize.y = height;
    detGraphBounds();
    f_isGridTextUpdateNeeded = true;
    f_isCursorTextUpdateNeeded = true;
    f_isTitleTextUpdateNeeded = true;
}

static void scrollCallback(GLFWwindow * window, double x, double y) {
    if (y != 0.0f) {
        if (y < 0.0f) f_liftGraph->zoomView(k_zoomFactor);
        else if (y > 0.0f) f_liftGraph->zoomView(1.0f / k_zoomFactor);
        f_dragGraph->setView(f_liftGraph->viewMin(), f_liftGraph->viewMax());
        f_isGridTextUpdateNeeded = true;
        f_isCursorTextUpdateNeeded = true;
    }
}

static void cursorPosCallback(GLFWwindow * window, double x, double y) {
    glm::ivec2 newPos{int(x), f_windowSize.y - 1 - int(y)};
    glm::ivec2 delta(newPos - f_mousePos);

    if (f_mouseButtonDown) {
        glm::vec2 factor((f_liftGraph->viewMax() - f_liftGraph->viewMin()) / glm::vec2(f_graphSize));
        f_liftGraph->moveView(-factor * glm::vec2(delta));
        f_dragGraph->setView(f_liftGraph->viewMin(), f_liftGraph->viewMax());
        f_isGridTextUpdateNeeded = true;
    }

    f_mousePos += delta;
    f_isCursorTextUpdateNeeded = true;
}

static void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && mods == 0) {
        f_mouseButtonDown = true;
    }
    else {
        f_mouseButtonDown = false;
    }
}



bool setup(const std::string & resourcesDir) {
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    if (!(f_window = glfwCreateWindow(k_initWindowSize.x, k_initWindowSize.y, "Results", nullptr, nullptr))) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    glfwDefaultWindowHints();

    glfwSetFramebufferSizeCallback(f_window, framebufferSizeCallback);
    glfwSetScrollCallback(f_window, scrollCallback);
    glfwSetCursorPosCallback(f_window, cursorPosCallback);
    glfwSetMouseButtonCallback(f_window, mouseButtonCallback);

    f_windowSize = k_initWindowSize;
    detGraphBounds();

    glfwMakeContextCurrent(f_window);
    
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glPointSize(4.0f);

    if (!Graph::setup(resourcesDir)) {
        std::cerr << "Failed to setup graph" << std::endl;
        return false;
    }
    f_liftGraph.reset(new Graph(
        glm::vec2(k_initGraphDomainMin, k_initGraphRangeMin),
        glm::vec2(k_initGraphDomainMax, k_initGraphRangeMax),
        glm::vec3(0.0f, 1.0f, 0.0f),
        int(180.0f * k_invGranularity) + 1
    ));
    f_dragGraph.reset(new Graph(
        glm::vec2(k_initGraphDomainMin, k_initGraphRangeMin),
        glm::vec2(k_initGraphDomainMax, k_initGraphRangeMax),
        glm::vec3(1.0f, 0.0f, 0.0f),
        int(180.0f * k_invGranularity) + 1
    ));

    if (!Text::setup(resourcesDir)) {
        std::cerr << "Failed to setup text" << std::endl;
        return false;
    }
    f_liftDomainMinText.reset(new Text("", glm::ivec2(), glm::ivec2(0, 1), glm::vec3(1.0f)));
    f_liftDomainMaxText.reset(new Text("", glm::ivec2(), glm::ivec2(0, 1), glm::vec3(1.0f)));
    f_liftRangeMinText.reset(new Text("", glm::ivec2(), glm::ivec2(-1, 0), glm::vec3(1.0f)));
    f_liftRangeMaxText.reset(new Text("", glm::ivec2(), glm::ivec2(-1, 0), glm::vec3(1.0f)));
    f_dragDomainMinText.reset(new Text("", glm::ivec2(), glm::ivec2(0, 1), glm::vec3(1.0f)));
    f_dragDomainMaxText.reset(new Text("", glm::ivec2(), glm::ivec2(0, 1), glm::vec3(1.0f)));
    f_dragRangeMinText.reset(new Text("", glm::ivec2(), glm::ivec2(-1, 0), glm::vec3(1.0f)));
    f_dragRangeMaxText.reset(new Text("", glm::ivec2(), glm::ivec2(-1, 0), glm::vec3(1.0f)));
    f_isGridTextUpdateNeeded = true;

    f_cursorText.reset(new Text("", glm::ivec2(), glm::ivec2(1, -1), glm::vec3(1.0f)));

    f_liftTitleText.reset(new Text("Lift Force (N) by Angle (°)", glm::ivec2(), glm::ivec2(0, -1), glm::vec3(1.0f)));
    f_dragTitleText.reset(new Text("Drag Force (N) by Angle (°)", glm::ivec2(), glm::ivec2(0, -1), glm::vec3(1.0f)));
    f_isTitleTextUpdateNeeded = true;

    return true;
}

void cleanup() {
    f_liftGraph->cleanup();
    f_dragGraph->cleanup();
    
    f_liftDomainMinText->cleanup();
    f_liftDomainMaxText->cleanup();
    f_liftRangeMinText->cleanup();
    f_liftRangeMaxText->cleanup();
    f_dragDomainMinText->cleanup();
    f_dragDomainMaxText->cleanup();
    f_dragRangeMinText->cleanup();
    f_dragRangeMaxText->cleanup();

    f_cursorText->cleanup();

    f_liftTitleText->cleanup();
    f_dragTitleText->cleanup();

    glfwDestroyWindow(f_window);
}

void render() {
    glfwMakeContextCurrent(f_window);

    glClear(GL_COLOR_BUFFER_BIT);

    if (f_isChange) {
        auto & liftPoints(f_liftGraph->mutatePoints());
        auto & dragPoints(f_dragGraph->mutatePoints());
        liftPoints.clear();
        dragPoints.clear();
        liftPoints.reserve(f_record.size());
        dragPoints.reserve(f_record.size());
        for (const auto & r : f_record) {
            liftPoints.emplace_back(r.first, r.second.first);
            dragPoints.emplace_back(r.first, r.second.second);
        }

        f_isChange = false;
        f_isCursorTextUpdateNeeded = true;
    }


    glViewport(f_liftGraphPos.x, f_liftGraphPos.y, f_graphSize.x, f_graphSize.y);
    f_liftGraph->render(f_graphSize);
    glViewport(f_dragGraphPos.x, f_dragGraphPos.y, f_graphSize.x, f_graphSize.y);
    f_dragGraph->render(f_graphSize);
    glViewport(0, 0, f_windowSize.x, f_windowSize.y);

    if (f_isGridTextUpdateNeeded) {
        updateGridText();
        f_isGridTextUpdateNeeded = false;
    }
    f_liftDomainMinText->render(f_windowSize);
    f_liftDomainMaxText->render(f_windowSize);
    f_liftRangeMinText->render(f_windowSize);
    f_liftRangeMaxText->render(f_windowSize);
    f_dragDomainMinText->render(f_windowSize);
    f_dragDomainMaxText->render(f_windowSize);
    f_dragRangeMinText->render(f_windowSize);
    f_dragRangeMaxText->render(f_windowSize);

    if (f_isCursorTextUpdateNeeded) {
        updateCursorText();
        f_isCursorTextUpdateNeeded = false;
    }
    f_cursorText->render(f_windowSize);

    if (f_isTitleTextUpdateNeeded) {
        updateTitleText();
        f_isTitleTextUpdateNeeded = false;
    }
    f_liftTitleText->render(f_windowSize);
    f_dragTitleText->render(f_windowSize);

    glfwSwapBuffers(f_window);
}

void submit(float angle, float lift, float drag) {
    f_record[std::round(angle * k_invGranularity) * k_granularity] = { lift, drag };
    f_isChange = true;
}

bool valAt(float angle, float & r_lift, float & r_drag) {
    auto it(f_record.find(angle));
    if (it != f_record.end()) {
        r_lift = it->second.first;
        r_drag = it->second.second;
        return true;
    }

    auto preIt(f_record.lower_bound(angle)), postIt(f_record.upper_bound(angle));
    if (preIt == f_record.begin() || postIt == f_record.end()) {
        return false;
    }
    --preIt;

    float preAngle(preIt->first), preLift(preIt->second.first), preDrag(preIt->second.second);
    float postAngle(postIt->first), postLift(postIt->second.first), postDrag(postIt->second.second);

    float interpP((angle - preAngle) / (postAngle - preAngle));
    r_lift = glm::mix(preLift, postLift, interpP);
    r_drag = glm::mix(preDrag, postDrag, interpP);
    return true;
}



}