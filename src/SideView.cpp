#include "SideView.hpp"

#include <iostream>

#include "glm/gtc/matrix_transform.hpp"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "Program.h"



namespace SideView {

    static constexpr int k_width(720), k_height(720);

    static GLFWwindow * s_window;
    static uint s_sideTex;
    static std::shared_ptr<Program> s_texProg;
    static uint s_boardVAO, s_boardVBO, s_boardTexVBO, s_boardIndVBO;

    static bool setupShaders(const std::string & resourcesDir) {
        std::string shadersDir(resourcesDir + "/shaders");

        // FB Shader ---------------------------------------------------------------
        s_texProg = std::make_shared<Program>();
        s_texProg->setVerbose(true);
        s_texProg->setShaderNames(shadersDir + "/tex.vert", shadersDir + "/tex.frag");
        if (!s_texProg->init()) {
            std::cerr << "Failed to initialize tex shader" << std::endl;
            return false;
        }
        s_texProg->addUniform("tex");
        glUseProgram(s_texProg->pid);
        glUniform1i(s_texProg->getUniform("tex"), 0);
        glUseProgram(0);

        return true;
    }

    bool setup(const std::string & resourcesDir, int sideTexID, GLFWwindow * mainWindow) {
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        if (!(s_window = glfwCreateWindow(k_width, k_height, "Side View", nullptr, mainWindow))) {
            std::cerr << "Failed to create window" << std::endl;
            return false;
        }
        glfwDefaultWindowHints();

        glfwMakeContextCurrent(s_window);

        s_sideTex = (uint)sideTexID;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);

        // Setup shaders
        if (!setupShaders(resourcesDir)) {
            std::cerr << "Failed to setup shaders" << std::endl;
            return false;
        }

        initGeom();

        return true;
    }

    bool initGeom() {
        glGenVertexArrays(1, &s_boardVAO);
        glBindVertexArray(s_boardVAO);

        glGenBuffers(1, &s_boardVBO);
        glBindBuffer(GL_ARRAY_BUFFER, s_boardVBO);
        GLfloat board_vertices[] = {
            -1.0f, -1.0f, 0.0f, //LD
            1.0f, -1.0f, 0.0f, //RD
            1.0f,  1.0f, 0.0f, //RU
            -1.0f,  1.0f, 0.0f, //LU
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(board_vertices), board_vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);


        // color
        vec2 board_tex[] = {
            // front colors
            vec2(0.0f, 0.0f),
            vec2(1.0f, 0.0f),
            vec2(1.0f, 1.0f),
            vec2(0.0f, 1.0f),
        };
        glGenBuffers(1, &s_boardTexVBO);
        glBindBuffer(GL_ARRAY_BUFFER, s_boardTexVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(board_tex), board_tex, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        glGenBuffers(1, &s_boardIndVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_boardIndVBO);
        GLushort board_elements[] = {
            // front
            0, 1, 2,
            2, 3, 0,
        };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(board_elements), board_elements, GL_STATIC_DRAW);
        glBindVertexArray(0);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }
        return true;
    }

    void render() {
        glfwMakeContextCurrent(s_window);
        glViewport(0, 0, k_width, k_height);
        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        s_texProg->bind();
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_sideTex);
        glBindVertexArray(s_boardVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
        s_texProg->unbind();

        glBindVertexArray(0);

        glfwSwapBuffers(s_window);
    }

    GLFWwindow * getWindow() {
        return s_window;
    }



}
