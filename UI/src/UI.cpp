#pragma once



#include "UI.hpp"

#include <iostream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "Common/Text.hpp"
#include "Common/Shader.hpp"

#include "Graph.hpp"
#include "TexViewer.hpp"



namespace ui {

    static const ivec2 k_tooltipOffset(12, -12);

    static const std::string & k_compVertFilename("ui_comp.vert"), k_compFragFilename("ui_comp.frag");



    static GLFWwindow * s_window;
    static ivec2 s_windowSize;
    static ivec2 s_cursorPos;
    static bool s_cursorInside;

    static unq<Shader> s_compProg;
    static uint s_squareVBO, s_squareVAO;
    
    static shr<Component> s_root;
    static shr<Component> s_tooltip;
    static bool s_isTooltipChange;
    static Component * s_focus;

    /*
    static fptr<void(int, int, int)> s_keyCallback;
    static fptr<void(dvec2)> s_cursorPositionCallback;
    static fptr<void(bool)> s_cursorEnterCallback;
    static fptr<void(int, int, int)> s_mouseButtonCallback;
    static fptr<void(dvec2)> s_scrollCallback;
    static fptr<void()> s_exitCallback;
    */



    static void glfwErrorCallback(int error, const char * description) {
        std::cerr << "GLFW error " << error << ": " << description << std::endl;
    }

    static void glfwFramebufferSizeCallback(GLFWwindow * window, int width, int height) {
        s_windowSize.x = width;
        s_windowSize.y = height;
        s_root->size(s_windowSize);
        s_root->pack();
    }

    static void glfwKeyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {
        if (s_focus) s_focus->keyEvent(key, action, mods);
        else s_root->keyEvent(key, action, mods);

        //if (s_keyCallback) s_keyCallback(key, action, mods);
    }

    static void glfwCharCallback(GLFWwindow * window, unsigned int codepoint) {
        if (s_focus) s_focus->charEvent(char(codepoint));
        else s_root->charEvent(char(codepoint));
    }

    static void glfwCursorPositionCallback(GLFWwindow * window, double xpos, double ypos) {
        ivec2 prevCursorPos(s_cursorPos);
        s_cursorPos.x = int(xpos);
        s_cursorPos.y = s_windowSize.y - 1 - int(ypos);
        if (s_cursorInside) {
            s_root->cursorPositionEvent(s_cursorPos, s_cursorPos - prevCursorPos);
        }
        s_isTooltipChange = true;
    }

    static void glfwCursorEnterCallback(GLFWwindow * window, int entered) {
        s_cursorInside = entered;
        if (s_cursorInside) {
            s_root->cursorEnterEvent();
            s_root->cursorPositionEvent(s_cursorPos, ivec2());
        }
        else s_root->cursorExitEvent();

        //if (s_cursorEnterCallback) s_cursorEnterCallback(entered);
    }

    static void glfwMouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
        s_root->mouseButtonEvent(button, action, mods);

        //if (s_mouseButtonCallback) s_mouseButtonCallback(button, action, mods);
    }

    static void glfwScrollCallback(GLFWwindow * window, double xoffset, double yoffset) {
        s_root->scrollEvent(ivec2(int(xoffset), int(yoffset)));

        //if (s_scrollCallback) s_scrollCallback(dvec2(xoffset, yoffset));
    }

    static std::vector<int> distribute(int total, int n) {
        std::vector<int> vals(n, total / n);
        int excess(total % n);
        for (int i(0); i < excess; ++i) ++vals[i];
        return move(vals);
    }


    
    void Component::render() const {
        if (m_backColor.a == 0.0f && m_borderColor.a == 0.0f) {
            return;
        }

        s_compProg->bind();
        s_compProg->uniform("u_viewportSize", vec2(m_size));
        s_compProg->uniform("u_backColor", m_backColor);
        s_compProg->uniform("u_borderColor", m_borderColor);
        setViewport();
        glBindVertexArray(s_squareVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void Component::position(const ivec2 & position) {
        m_position = position;
    }

    void Component::size(const ivec2 & size) {
        m_size = size;
    }

    bool Component::contains(const ivec2 & point) const {
        ivec2 p(point - m_position);
        return p.x >= 0 && p.y >= 0 && p.x < m_size.x && p.y < m_size.y;
    }

    void Component::focus() {
        ui::focus(this);
    }

    void Component::unfocus() {
        if (this == s_focus) ui::focus(nullptr);
    }

    bool Component::focused() const {
        return this == s_focus;
    }

    void Component::backColor(const vec4 & backColor) {
        m_backColor = backColor;
    }

    void Component::borderColor(const vec4 & borderColor) {
        m_borderColor = borderColor;
    }

    void Component::setViewport() const {
        glViewport(m_position.x, m_position.y, m_size.x, m_size.y);
    }



    void Group::update() {
        for (const auto & comp : m_components) {
            comp->update();
        }
    }

    void Group::render() const {
        for (const auto & comp : m_components) {
            comp->render();
        }
    }

    const ivec2 & Group::minSize() const {
        if (!m_areSizeExtremaValid) {
            auto [minSize, maxSize](detSizeExtrema());
            m_minSize = minSize;
            m_maxSize = maxSize;
            m_areSizeExtremaValid = true;
        }
        return m_minSize;
    }

    const ivec2 & Group::maxSize() const {
        if (!m_areSizeExtremaValid) {
            auto [minSize, maxSize](detSizeExtrema());
            m_minSize = minSize;
            m_maxSize = maxSize;
            m_areSizeExtremaValid = true;
        }
        return m_maxSize;
    }

    void Group::keyEvent(int key, int action, int mods) {
        if (m_cursorOverComp) m_cursorOverComp->keyEvent(key, action, mods);
    }

    void Group::charEvent(char c) {
        if (m_cursorOverComp) m_cursorOverComp->charEvent(c);
    }

    void Group::cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) {
        Component * prevCursorOverComp(m_cursorOverComp);
        detCursorOverComp(pos);
        if (m_cursorOverComp != prevCursorOverComp) {
            if (prevCursorOverComp) prevCursorOverComp->cursorExitEvent();
            if (m_cursorOverComp) m_cursorOverComp->cursorEnterEvent();
        }
        if (m_cursorOverComp) m_cursorOverComp->cursorPositionEvent(pos, delta);
    }

    void Group::cursorExitEvent() {
        m_cursorOverComp = nullptr;
    }

    void Group::mouseButtonEvent(int button, int action, int mods) {
        if (m_cursorOverComp) m_cursorOverComp->mouseButtonEvent(button, action, mods);
    }

    void Group::scrollEvent(const ivec2 & delta) {
        if (m_cursorOverComp) m_cursorOverComp->scrollEvent(delta);
    }

    void Group::add(shr<Component> component) {
        m_components.emplace_back(component);
    }

    void Group::packComponents() {
        for (auto & comp : m_components) {
            comp->pack();
        }
    }

    void Group::detCursorOverComp(const ivec2 & cursorPos) const {
        m_cursorOverComp = nullptr;
        for (const auto & comp : m_components) {
            if (comp->contains(cursorPos)) {
                m_cursorOverComp = comp.get();
                return;
            }
        }
    }



    bool setup(const ivec2 & windowSize, const std::string & windowTitle, int majorGLVersion, int minorGLVersion, bool vSync) {
        glfwSetErrorCallback(glfwErrorCallback);
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, majorGLVersion);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minorGLVersion);
        if (!(s_window = glfwCreateWindow(windowSize.x, windowSize.y, windowTitle.c_str(), nullptr, nullptr))) {
            std::cerr << "Failed to create window" << std::endl;
            return false;
        }

        glfwMakeContextCurrent(s_window);
        glfwSwapInterval(vSync ? 1 : 0);

        glfwSetFramebufferSizeCallback(s_window, glfwFramebufferSizeCallback);
        glfwSetKeyCallback(s_window, glfwKeyCallback);
        glfwSetCharCallback(s_window, glfwCharCallback);
        glfwSetCursorPosCallback(s_window, glfwCursorPositionCallback);
        glfwSetCursorEnterCallback(s_window, glfwCursorEnterCallback);
        glfwSetMouseButtonCallback(s_window, glfwMouseButtonCallback);
        glfwSetScrollCallback(s_window, glfwScrollCallback);

        glfwGetFramebufferSize(s_window, &s_windowSize.x, &s_windowSize.y);

        // Setup GLAD
        if (!gladLoadGL()) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return false;
        }

        // Setup comp shader
        std::string shadersPath(g_resourcesDir + "/UI/shaders/");
        if (!(s_compProg = Shader::load(shadersPath + k_compVertFilename, shadersPath + k_compFragFilename))) {
            std::cerr << "Failed to load comp program" << std::endl;
            return false;
        }

        // Setup square vao and vbo
        vec2 locs[6]{
            { 0.0f, 0.0f },
            { 1.0f, 0.0f },
            { 1.0f, 1.0f },
            { 1.0f, 1.0f },
            { 0.0f, 1.0f },
            { 0.0f, 0.0f }
        };
        glGenVertexArrays(1, &s_squareVAO);
        glBindVertexArray(s_squareVAO);
        glGenBuffers(1, &s_squareVBO);
        glBindBuffer(GL_ARRAY_BUFFER, s_squareVBO);
        glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec2), locs, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error" << std::endl;
            return false;
        }

        if (!Text::setup()) {
            std::cerr << "Failed to setup text" << std::endl;
            return false;
        }

        if (!Graph::setup()) {
            std::cerr << "Failed to setup graph" << std::endl;
            return false;
        }

        if (!TexViewer::setup()) {
            std::cerr << "Failed to setup texture viewer" << std::endl;
            return false;
        }

        return true;
    }

    void poll() {
        glfwPollEvents();
        //if (glfwWindowShouldClose(s_window)) {
        //    if (s_exitCallback) s_exitCallback();
        //}
    }

    const ivec2 & cursorPosition() {
        return s_cursorPos;
    }

    bool isMouseButtonPressed(int button) {
        return glfwGetMouseButton(s_window, button) == GLFW_PRESS;
    }

    bool isKeyPressed(int key) {
        return glfwGetKey(s_window, key) == GLFW_PRESS;
    }

    bool shouldExit() {
        return glfwWindowShouldClose(s_window);
    }

    void update() {
        s_root->update();

        if (s_isTooltipChange && s_tooltip && s_cursorInside) {
            s_tooltip->position(ivec2(
                s_cursorPos.x + k_tooltipOffset.x,
                s_cursorPos.y + k_tooltipOffset.y - s_tooltip->size().y
            ));
        }
    }

    void render() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        s_root->render();

        if (s_tooltip && s_cursorInside) {
            s_tooltip->render();
        }

        glfwSwapBuffers(s_window);
    }

    void setRootComponent(shr<Component> root) {
        s_root = root;
        s_root->size(s_windowSize);
        s_root->pack();

        ivec2 minSize(s_root->minSize());
        ivec2 maxSize(s_root->maxSize());
        glfwSetWindowSizeLimits(
            s_window,
            minSize.x ? minSize.x : GLFW_DONT_CARE,
            minSize.y ? minSize.y : GLFW_DONT_CARE,
            maxSize.x ? maxSize.x : GLFW_DONT_CARE,
            maxSize.y ? maxSize.y : GLFW_DONT_CARE
        );
    }

    void setTooltip(shr<Component> tooltip) {
        s_tooltip = tooltip;
        s_isTooltipChange = true;
    }

    void focus(Component * component) {
        Component * prevFocus = s_focus;
        s_focus = component;
        if (s_focus != prevFocus) {
            if (prevFocus) prevFocus->unfocusEvent();
            if (s_focus) s_focus->focusEvent();
        }
    }

}