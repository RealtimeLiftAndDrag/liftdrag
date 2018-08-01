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

static constexpr float k_initGraphDomainMin(-90.0f), k_initGraphDomainMax(90.0f);
static constexpr float k_initGraphRangeMin(-50.0f), k_initGraphRangeMax(150.0f);
static constexpr float k_zoomFactor(1.1f);
static constexpr int k_leftMargin(80), k_rightMargin(140), k_bottomMargin(60), k_topMargin(30);
static const glm::ivec2 k_initGraphSize(300, 200);
static const glm::ivec2 k_initWindowSize(k_leftMargin + k_rightMargin + k_initGraphSize.x, k_bottomMargin + k_topMargin + k_initGraphSize.y);
static constexpr int k_valTextPrecision(3);
static const std::string k_graphTitleString("Lift and Drag Force (N) by Angle of Attack (°)");
static const std::string k_graphLegendString("Green: Lift\n  Red: Drag");



static std::map<float, std::pair<float, float>> f_record; // map of results where key is angle and value is lift/drag pair
static bool f_isChange;

static GLFWwindow * f_window;
static glm::ivec2 f_windowSize;
static bool f_mouseButtonDown;
static glm::ivec2 f_mousePos;

static std::unique_ptr<Graph> f_graph;
static glm::ivec2 f_graphPos, f_graphSize;

static std::unique_ptr<Text> f_domainMinText, f_domainMaxText;
static std::unique_ptr<Text> f_rangeMinText, f_rangeMaxText;
static bool f_isGridTextUpdateNeeded;

static std::unique_ptr<Text> f_cursorText;
static bool f_isCursorTextUpdateNeeded;

static std::unique_ptr<Text> f_titleText;
static std::unique_ptr<Text> f_legendText;
static bool f_isGraphTextUpdateNeeded;



static void detGraphBounds() {
    f_graphPos.x = k_leftMargin;
    f_graphPos.y = k_bottomMargin;
    f_graphSize.x = f_windowSize.x - k_leftMargin - k_rightMargin;
    f_graphSize.y = f_windowSize.y - k_bottomMargin - k_topMargin;
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

static bool isCursorInGraph() {    
    glm::ivec2 relPos(f_mousePos - f_graphPos);
    return relPos.x >= 0 && relPos.y >= 0 && relPos.x < f_graphSize.x && relPos.y < f_graphSize.y;
}

static void updateGridText() {
    glm::vec2 gridMin(glm::ceil(f_graph->viewMin() / f_graph->gridSize()) * f_graph->gridSize());
    glm::vec2 gridMax(glm::floor(f_graph->viewMax() / f_graph->gridSize()) * f_graph->gridSize());
    glm::vec2 invViewSize(1.0f / (f_graph->viewMax() - f_graph->viewMin()));
    glm::vec2 graphSize(f_graphSize);
    glm::ivec2 gridMinPos(glm::round((gridMin - f_graph->viewMin()) * invViewSize * graphSize));
    glm::ivec2 gridMaxPos(glm::round((gridMax - f_graph->viewMin()) * invViewSize * graphSize));
    
    f_domainMinText->position(glm::ivec2(f_graphPos.x + gridMinPos.x, f_graphPos.y));
    f_domainMinText->string(valToString(gridMin.x));
    f_domainMaxText->position(glm::ivec2(f_graphPos.x + gridMaxPos.x, f_graphPos.y));
    f_domainMaxText->string(valToString(gridMax.x));

    f_rangeMinText->position(glm::ivec2(f_graphPos.x, f_graphPos.y + gridMinPos.y));
    f_rangeMinText->string(valToString(gridMin.y));
    f_rangeMaxText->position(glm::ivec2(f_graphPos.x, f_graphPos.y + gridMaxPos.y));
    f_rangeMaxText->string(valToString(gridMax.y));
}

static void updateCursorText() {
    f_cursorText->string("");
    f_graph->removeFocusX();

    if (!isCursorInGraph()) {
        return;
    }

    float angle(float(f_mousePos.x - f_graphPos.x));
    angle /= f_graphSize.x;
    angle *= f_graph->viewMax().x - f_graph->viewMin().x;
    angle += f_graph->viewMin().x;

    float lift, drag;
    if (!valAt(angle, lift, drag)) {
        return;
    }

    f_cursorText->position(f_mousePos + glm::ivec2(10, -10));
    f_cursorText->string(pointToString(angle, lift, drag));
    f_graph->focusX(angle);
}

static void updateGraphText() {
    f_titleText->position(glm::ivec2(f_windowSize.x / 2, f_graphPos.y + f_graphSize.y + 10));
    f_legendText->position(f_graphPos + f_graphSize + glm::ivec2(10, -10));
}

static void framebufferSizeCallback(GLFWwindow * window, int width, int height) {
    f_windowSize.x = width; f_windowSize.y = height;
    detGraphBounds();
    f_isGridTextUpdateNeeded = true;
    f_isCursorTextUpdateNeeded = true;
    f_isGraphTextUpdateNeeded = true;
}

static void scrollCallback(GLFWwindow * window, double x, double y) {
    if (y != 0.0f) {
        if (y < 0.0f) f_graph->zoomView(k_zoomFactor);
        else if (y > 0.0f) f_graph->zoomView(1.0f / k_zoomFactor);
        f_isGridTextUpdateNeeded = true;
        f_isCursorTextUpdateNeeded = true;
    }
}

static void cursorPosCallback(GLFWwindow * window, double x, double y) {
    glm::ivec2 newPos{int(x), f_windowSize.y - 1 - int(y)};
    glm::ivec2 delta(newPos - f_mousePos);

    if (f_mouseButtonDown) {
        glm::vec2 factor((f_graph->viewMax() - f_graph->viewMin()) / glm::vec2(f_graphSize));
        f_graph->moveView(-factor * glm::vec2(delta));
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



bool setup(const std::string & resourcesDir, GLFWwindow * mainWindow) {
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    if (!(f_window = glfwCreateWindow(k_initWindowSize.x, k_initWindowSize.y, "Results", nullptr, mainWindow))) {
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

    f_graph.reset(new Graph(
        glm::vec2(k_initGraphDomainMin, k_initGraphRangeMin),
        glm::vec2(k_initGraphDomainMax, k_initGraphRangeMax)
    ));
    f_graph->addCurve(glm::vec3(0.0f, 1.0f, 0.0f), int(180.0f * k_invGranularity) + 1);
    f_graph->addCurve(glm::vec3(1.0f, 0.0f, 0.0f), int(180.0f * k_invGranularity) + 1);

    if (!Text::setup(resourcesDir)) {
        std::cerr << "Failed to setup text" << std::endl;
        return false;
    }

    f_domainMinText.reset(new Text("", glm::ivec2(), glm::ivec2(0, 1), glm::vec3(1.0f)));
    f_domainMaxText.reset(new Text("", glm::ivec2(), glm::ivec2(0, 1), glm::vec3(1.0f)));
    f_rangeMinText.reset(new Text("", glm::ivec2(), glm::ivec2(-1, 0), glm::vec3(1.0f)));
    f_rangeMaxText.reset(new Text("", glm::ivec2(), glm::ivec2(-1, 0), glm::vec3(1.0f)));
    f_isGridTextUpdateNeeded = true;

    f_cursorText.reset(new Text("", glm::ivec2(), glm::ivec2(1, 1), glm::vec3(1.0f)));

    f_titleText.reset(new Text(k_graphTitleString, glm::ivec2(), glm::ivec2(0, -1), glm::vec3(1.0f)));
    f_legendText.reset(new Text(k_graphLegendString, glm::ivec2(), glm::ivec2(1, 1), glm::vec3(0.5f)));
    f_isGraphTextUpdateNeeded = true;

    return true;
}

void cleanup() {
    f_graph->cleanup();
    
    f_domainMinText->cleanup();
    f_domainMaxText->cleanup();
    f_rangeMinText->cleanup();
    f_rangeMaxText->cleanup();

    f_cursorText->cleanup();

    f_titleText->cleanup();

    glfwDestroyWindow(f_window);
}

void render() {
    glfwMakeContextCurrent(f_window);

    glClear(GL_COLOR_BUFFER_BIT);

    if (f_isChange) {
        auto & liftPoints(f_graph->mutatePoints(0));
        auto & dragPoints(f_graph->mutatePoints(1));
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


    glViewport(f_graphPos.x, f_graphPos.y, f_graphSize.x, f_graphSize.y);
    f_graph->render(f_graphSize);
    glViewport(0, 0, f_windowSize.x, f_windowSize.y);

    if (f_isGridTextUpdateNeeded) {
        updateGridText();
        f_isGridTextUpdateNeeded = false;
    }
    f_domainMinText->render(f_windowSize);
    f_domainMaxText->render(f_windowSize);
    f_rangeMinText->render(f_windowSize);
    f_rangeMaxText->render(f_windowSize);

    if (f_isCursorTextUpdateNeeded) {
        updateCursorText();
        f_isCursorTextUpdateNeeded = false;
    }
    f_cursorText->render(f_windowSize);

    if (f_isGraphTextUpdateNeeded) {
        updateGraphText();
        f_isGraphTextUpdateNeeded = false;
    }
    f_titleText->render(f_windowSize);
    f_legendText->render(f_windowSize);

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