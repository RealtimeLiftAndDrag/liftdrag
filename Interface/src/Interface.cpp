#pragma once



#include "Interface.hpp"

#include <iostream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "Common/Text.hpp"
#include "Common/Shader.hpp"

#include "Graph.hpp"
#include "TexViewer.hpp"



namespace UI {

    static const ivec2 k_tooltipOffset(12, -12);

    static const std::string & k_compVertFilename("ui_comp.vert"), k_compFragFilename("ui_comp.frag");
    static const std::string & k_localResourcesPath("/Interface");



    static GLFWwindow * s_window;
    static ivec2 s_windowSize;
    static shr<Component> s_root;
    static shr<Component> s_tooltip;
    static ivec2 s_cursorPos;
    static bool s_cursorInside;
    static bool s_isTooltipChange;
    static Component * s_focus;

    static unq<Shader> s_compProg;
    static uint s_squareVBO, s_squareVAO;



    static void errorCallback(int error, const char * description) {
        std::cerr << "GLFW error " << error << ": " << description << std::endl;
    }

    static void framebufferSizeCallback(GLFWwindow * window, int width, int height) {
        s_windowSize.x = width;
        s_windowSize.y = height;
        s_root->size(s_windowSize);
        s_root->pack();
    }

    static void keyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {
        if (s_focus) s_focus->keyEvent(key, action, mods);
        else s_root->keyEvent(key, action, mods);
    }

    static void charCallback(GLFWwindow * window, unsigned int codepoint) {
        if (s_focus) s_focus->charEvent(char(codepoint));
        else s_root->charEvent(char(codepoint));
    }

    static void cursorPositionCallback(GLFWwindow * window, double xpos, double ypos) {
        ivec2 prevCursorPos(s_cursorPos);
        s_cursorPos.x = int(xpos);
        s_cursorPos.y = s_windowSize.y - 1 - int(ypos);
        if (s_cursorInside) {
            s_root->cursorPositionEvent(s_cursorPos, s_cursorPos - prevCursorPos);
        }
        s_isTooltipChange = true;
    }

    static void cursorEnterCallback(GLFWwindow * window, int entered) {
        s_cursorInside = entered;
        if (s_cursorInside) {
            s_root->cursorEnterEvent();
            s_root->cursorPositionEvent(s_cursorPos, ivec2());
        }
        else s_root->cursorExitEvent();
    }

    static void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
        s_root->mouseButtonEvent(button, action, mods);
    }

    static void scrollCallback(GLFWwindow * window, double xoffset, double yoffset) {
        s_root->scrollEvent(ivec2(int(xoffset), int(yoffset)));
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
        glViewport(m_position.x, m_position.y, m_size.x, m_size.y);
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
        UI::focus(this);
    }

    void Component::unfocus() {
        if (this == s_focus) UI::focus(nullptr);
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
            detSizeExtrema();
            m_areSizeExtremaValid = true;
        }
        return m_minSize;
    }

    const ivec2 & Group::maxSize() const {
        if (!m_areSizeExtremaValid) {
            detSizeExtrema();
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



    void HorizontalGroup::pack() {
        std::vector<int> tempWidths(m_components.size());

        int remainingWidth(m_size.x);
        for (int i(0); i < int(m_components.size()); ++i) {
            tempWidths[i] = m_components[i]->minSize().x;
            remainingWidth -= tempWidths[i];
        }

        bool wasChange;
        while (remainingWidth > 0) {
            wasChange = false;
            for (int i(0); i < int(m_components.size()); ++i) {
                if (tempWidths[i] < m_components[i]->maxSize().x || !m_components[i]->maxSize().x) {
                    ++tempWidths[i];
                    --remainingWidth;
                    wasChange = true;
                    if (!remainingWidth) {
                        break;
                    }
                }
            }
            if (!wasChange) {
                break;
            }
        }

        ivec2 pos(m_position);
        for (int i(0); i < int(m_components.size()); ++i) {
            m_components[i]->position(pos);
            m_components[i]->size(ivec2(tempWidths[i], m_size.y));
            pos.x += tempWidths[i];
        }

        packComponents();
    }

    void HorizontalGroup::detSizeExtrema() const {
        m_minSize.x = 0;
        m_minSize.y = 0;
        m_maxSize.x = 0;
        m_maxSize.y = std::numeric_limits<int>::max();
        bool isMaxWidth(true);

        for (const auto & comp : m_components) {
            m_minSize.x += comp->minSize().x;

            if (comp->minSize().y > m_minSize.y) m_minSize.y = comp->minSize().y;

            if (comp->maxSize().x) m_maxSize.x += comp->maxSize().x;
            else isMaxWidth = false;

            if (comp->maxSize().y && comp->maxSize().y < m_maxSize.y) m_maxSize.y = comp->maxSize().y;
        }

        if (!isMaxWidth) m_maxSize.x = 0;
        if (m_maxSize.y == std::numeric_limits<int>::max()) m_maxSize.y = 0;
    }



    void VerticalGroup::pack() {
        std::vector<int> tempHeights(m_components.size());

        int remainingHeight(m_size.y);
        for (int i(0); i < int(m_components.size()); ++i) {
            tempHeights[i] = m_components[i]->minSize().y;
            remainingHeight -= tempHeights[i];
        }

        bool wasChange;
        while (remainingHeight > 0) {
            wasChange = false;
            for (int i(0); i < int(m_components.size()); ++i) {
                if (tempHeights[i] < m_components[i]->maxSize().y || !m_components[i]->maxSize().y) {
                    ++tempHeights[i];
                    --remainingHeight;
                    wasChange = true;
                    if (!remainingHeight) {
                        break;
                    }
                }
            }
            if (!wasChange) {
                break;
            }
        }

        ivec2 pos(m_position);
        for (int i(0); i < int(m_components.size()); ++i) {
            m_components[i]->position(pos);
            m_components[i]->size(ivec2(m_size.x, tempHeights[i]));
            pos.y += tempHeights[i];
        }

        packComponents();
    }

    void VerticalGroup::detSizeExtrema() const {
        m_minSize.x = 0;
        m_minSize.y = 0;
        m_maxSize.x = std::numeric_limits<int>::max();
        m_maxSize.y = 0;
        bool isMaxHeight(true);

        for (const auto & comp : m_components) {
            if (comp->minSize().x > m_minSize.x) m_minSize.x = comp->minSize().x;

            m_minSize.y += comp->minSize().y;

            if (comp->maxSize().x && comp->maxSize().x < m_maxSize.x) m_maxSize.x = comp->maxSize().x;

            if (comp->maxSize().y) m_maxSize.y += comp->maxSize().y;
            else isMaxHeight = false;
        }

        if (m_maxSize.x == std::numeric_limits<int>::max()) m_maxSize.x = 0;
        if (!isMaxHeight) m_maxSize.y = 0;
    }



    bool setup(const ivec2 & windowSize, const std::string & windowTitle, int majorGLVersion, int minorGLVersion, bool vSync) {
        glfwSetErrorCallback(errorCallback);
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

        glfwSetFramebufferSizeCallback(s_window, framebufferSizeCallback);
        glfwSetKeyCallback(s_window, keyCallback);
        glfwSetCharCallback(s_window, charCallback);
        glfwSetCursorPosCallback(s_window, cursorPositionCallback);
        glfwSetCursorEnterCallback(s_window, cursorEnterCallback);
        glfwSetMouseButtonCallback(s_window, mouseButtonCallback);
        glfwSetScrollCallback(s_window, scrollCallback);

        glfwGetFramebufferSize(s_window, &s_windowSize.x, &s_windowSize.y);

        // Setup GLAD
        if (!gladLoadGL()) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return false;
        }

        // Setup comp shader
        std::string shadersPath(g_resourcesDir + "/Interface/shaders/");
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

    void poll() {
        glfwPollEvents();
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

    bool shouldClose() {
        return glfwWindowShouldClose(s_window);
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

    void focus(Component * component) {
        Component * prevFocus = s_focus;
        s_focus = component;
        if (s_focus != prevFocus) {
            if (prevFocus) prevFocus->unfocusEvent();
            if (s_focus) s_focus->focusEvent();
        }
    }

}