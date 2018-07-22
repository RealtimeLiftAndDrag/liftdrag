#include "Results.hpp"

#include <iostream>
#include <memory>
#include <map>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#include "GLSL.h"
#include "Program.h"
#include "Graph.hpp"
#include "Text.hpp"



namespace results {



static const glm::ivec2 k_initWindowSize(400, 300);
static constexpr float k_initGraphDomainMin(-90.0f), k_initGraphDomainMax(90.0f);
static constexpr float k_initGraphRangeMin(-1.0f), k_initGraphRangeMax(1.0f);
static constexpr float k_granularity(1.0f); // results kept in increments of this many degrees
static constexpr float k_invGranularity(1.0f / k_granularity);
static constexpr float k_zoomFactor(1.1f);

static GLFWwindow * f_window;
static glm::ivec2 f_windowSize;
static bool f_mouseButtonDown;
static glm::ivec2 f_mousePos;

static std::map<float, std::pair<float, float>> f_record; // map of results where key is angle and value is lift/drag pair
static bool f_isChange;

static std::unique_ptr<Graph> f_liftGraph;
static glm::ivec2 f_liftGraphPos, f_liftGraphSize;

static std::unique_ptr<Text> f_text;



static void detLiftGraphBounds() {
    f_liftGraphPos.x = 40;
    f_liftGraphPos.y = 40;
    f_liftGraphSize = f_windowSize - f_liftGraphPos - 10;
}

static void framebufferSizeCallback(GLFWwindow * window, int width, int height) {
    f_windowSize.x = width; f_windowSize.y = height;
    detLiftGraphBounds();
}

static void scrollCallback(GLFWwindow * window, double x, double y) {
    if (y < 0.0f) f_liftGraph->zoomView(k_zoomFactor);
    else if (y > 0.0f) f_liftGraph->zoomView(1.0f / k_zoomFactor);
}

static void cursorPosCallback(GLFWwindow * window, double x, double y) {
    glm::ivec2 newPos{int(x), int(y)};
    glm::ivec2 delta(newPos - f_mousePos);

    if (f_mouseButtonDown) {
        glm::vec2 factor((f_liftGraph->viewMax() - f_liftGraph->viewMin()) / glm::vec2(f_liftGraphSize));
        f_liftGraph->moveView(factor * glm::vec2(-delta.x, delta.y));
    }

    f_mousePos += delta;
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
    detLiftGraphBounds();

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
    glm::vec2 graphSize(k_initGraphDomainMax - k_initGraphDomainMin, k_initGraphRangeMax - k_initGraphRangeMin);
    glm::vec2 padding;//(graphSize * 0.05f);
    f_liftGraph.reset(new Graph(
        glm::vec2(k_initGraphDomainMin - padding.x, k_initGraphRangeMin - padding.y),
        glm::vec2(k_initGraphDomainMax + padding.x, k_initGraphRangeMax + padding.y),
        int(180.0f * k_invGranularity) + 1
    ));

    if (!Text::setup(resourcesDir)) {
        std::cerr << "Failed to setup text" << std::endl;
        return false;
    }
    f_text.reset(new Text("abc\nde\n\nf", glm::ivec2(40, 40), glm::ivec2(0, 0), glm::vec3(0.5f, 0.5f, 1.0f)));

    return true;
}

void cleanup() {
    f_liftGraph->cleanup();

    glfwDestroyWindow(f_window);
}

void render() {
    glfwMakeContextCurrent(f_window);

    glClear(GL_COLOR_BUFFER_BIT);

    if (f_isChange) {
        auto & liftPoints(f_liftGraph->mutatePoints());
        liftPoints.clear();
        liftPoints.reserve(f_record.size());
        for (const auto & r : f_record) {
            liftPoints.emplace_back(r.first, r.second.first);
        }

        f_isChange = false;
    }

    glViewport(f_liftGraphPos.x, f_liftGraphPos.y, f_liftGraphSize.x, f_liftGraphSize.y);
    f_liftGraph->render(f_liftGraphSize);
    glViewport(0, 0, f_windowSize.x, f_windowSize.y);

    f_text->render(f_windowSize);

    glfwSwapBuffers(f_window);
}

void submit(float angle, float lift, float drag) {
    f_record[std::round(angle * k_invGranularity) * k_granularity] = { lift, drag };
    f_isChange = true;
}



}