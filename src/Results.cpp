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

GLuint f_squareVBO;
GLuint f_squareVAO;



}



bool setup(const std::string & resourcesDir) {
    if (!(f_window = glfwCreateWindow(k_defWindowWidth, k_defWindowHeight, "Results", nullptr, nullptr))) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    glfwMakeContextCurrent(f_window);

    f_liftCurve.reset(new Graph(int(180.0f * k_invGranularity) + 1));
    f_liftCurve->setup(resourcesDir);

    glm::vec2 locs[6]{
        { -1.0f, -1.0f },
        {  1.0f, -1.0f },
        {  1.0f,  1.0f },
        {  1.0f,  1.0f },
        { -1.0f,  1.0f },
        { -1.0f, -1.0f }
    };
    
    glGenVertexArrays(1, &f_squareVAO);
    glBindVertexArray(f_squareVAO);
    
    glGenBuffers(1, &f_squareVBO);
    glBindBuffer(GL_ARRAY_BUFFER, f_squareVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(glm::vec3), locs, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }

    return true;
}

void cleanup() {
    f_liftCurve->cleanup();

    glDeleteVertexArrays(1, &f_squareVAO);
    f_squareVAO = 0;
    glDeleteBuffers(1, &f_squareVBO);
    f_squareVBO = 0;

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