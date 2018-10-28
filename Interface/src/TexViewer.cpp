#include "TexViewer.hpp"

#include <iostream>

#include "glad/glad.h"
#include "Common/GLSL.h"
#include "Common/Shader.hpp"



namespace UI {

    static constexpr int k_maxZoom(8);

    static const std::string & k_texVertFilename("tex.vert"), k_texFragFilename("tex.frag");

    static unq<Shader> s_texProg;
    static uint s_squareVAO, s_squareVBO;



    bool TexViewer::setup() {
        // Setup tex shader
        std::string shadersPath(g_resourcesDir + "/Interface/shaders/");
        if (!(s_texProg = Shader::load(shadersPath + k_texVertFilename, shadersPath + k_texFragFilename))) {
            std::cerr << "Failed to load tex program" << std::endl;
            return false;
        }
        s_texProg->addUniform("u_tex");
        s_texProg->addUniform("u_viewBounds");
        s_texProg->bind();
        glUniform1i(s_texProg->getUniform("u_tex"), 0);
        glUseProgram(0);

        // Setup square vao
        vec2 positions[6]{
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
        glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec2), positions, GL_STATIC_DRAW);
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

    TexViewer::TexViewer(uint tex, const ivec2 & texSize, const ivec2 & minSize) :
        Single(minSize, ivec2()),
        m_tex(tex),
        m_texSize(texSize),
        m_initViewSize(),
        m_viewSize(),
        m_center(0.5f),
        m_zoom(0)
    {}

    void TexViewer::render() const {
        glViewport(m_position.x, m_position.y, m_size.x, m_size.y);

        s_texProg->bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_tex);

        vec2 corner(m_center - m_viewSize * 0.5f);
        glUniform4f(s_texProg->getUniform("u_viewBounds"), corner.x, corner.y, m_viewSize.x, m_viewSize.y);

        glBindVertexArray(s_squareVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glUseProgram(0);
    }

    void TexViewer::pack() {
        m_initViewSize = vec2(m_size) / vec2(m_texSize);
        m_viewSize = m_initViewSize;
    }

    void TexViewer::cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) {
        if (UI::isMouseButtonPressed(0)) {
            vec2 factor(m_viewSize / vec2(m_size));
            moveCenter(-factor * vec2(delta));
        }
    }

    void TexViewer::scrollEvent(const ivec2 & delta) {
        vec2 a0(vec2(UI::cursorPosition() - (m_position + m_size / 2)) / vec2(m_size));
        vec2 prevViewSize(m_viewSize);

        if (delta.y < 0) zoomOut();
        else if (delta.y > 0) zoomIn();

        vec2 a1(a0 * m_viewSize / prevViewSize);
        vec2 b(a0 - a1);
        center(m_center + b * prevViewSize);
    }

    void TexViewer::center(const vec2 & p) {
        m_center = p;
    }

    void TexViewer::moveCenter(const vec2 & delta) {
        m_center += delta;
    }

    void TexViewer::zoom(int zoom) {
        m_zoom = glm::clamp(zoom, -k_maxZoom, k_maxZoom);
        m_viewSize = m_initViewSize;
        if (m_zoom > 0) m_viewSize *= float(1 << m_zoom);
        else if (m_zoom < 0) m_viewSize /= float(1 << -m_zoom);
    }

    void TexViewer::zoomIn() {
        zoom(m_zoom - 1);
    }

    void TexViewer::zoomOut() {
        zoom(m_zoom + 1);
    }

    void TexViewer::zoomReset() {
        zoom(0);
    }

}