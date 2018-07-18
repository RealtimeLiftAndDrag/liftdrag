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



namespace results {



namespace {



constexpr int k_defWindowWidth(400), k_defWindowHeight(300);
constexpr float k_granularity(1.0f); // results kept in increments of this many degrees
constexpr float k_invGranularity(1.0f / k_granularity);

GLFWwindow * f_window;

std::map<float, std::pair<float, float>> f_record; // map of results where key is angle and value is lift/drag pair
bool f_isChange;

std::unique_ptr<Graph> f_liftCurve;



}



bool setup(const std::string & resourcesDir) {
    if (!(f_window = glfwCreateWindow(k_defWindowWidth, k_defWindowHeight, "Results", nullptr, nullptr))) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    glfwMakeContextCurrent(f_window);

    glPointSize(4.0f);

    f_liftCurve.reset(new Graph(int(180.0f * k_invGranularity) + 1));
    if (!f_liftCurve->setup(resourcesDir)) {
        std::cerr << "Failed to setup lift graph" << std::endl;
        return false;
    }

    return true;
}

void cleanup() {
    f_liftCurve->cleanup();

    glfwDestroyWindow(f_window);
}

void render() {
    glfwMakeContextCurrent(f_window);

    glClear(GL_COLOR_BUFFER_BIT);

    if (f_isChange) {
        auto & liftPoints(f_liftCurve->accessPoints());
        liftPoints.clear();
        liftPoints.reserve(f_record.size());
        for (const auto & r : f_record) {
            liftPoints.emplace_back(r.first, r.second.first);
        }
        f_liftCurve->refresh();

        f_isChange = false;
    }

    f_liftCurve->render();

    glfwSwapBuffers(f_window);
}

void submit(float angle, float lift, float drag) {
    f_record[std::round(angle * k_invGranularity) * k_granularity] = { lift, drag };
    f_isChange = true;
}



}