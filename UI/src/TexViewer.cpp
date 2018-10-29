#include "TexViewer.hpp"

#include <iostream>

#include "glad/glad.h"
#include "Common/GLSL.h"
#include "Common/Shader.hpp"



namespace ui {

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
        s_texProg->bind();
        s_texProg->uniform("u_tex", 0);
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
        setViewport();

        s_texProg->bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_tex);

        vec2 corner(m_center - m_viewSize * 0.5f);
        s_texProg->uniform("u_viewBounds", vec4(corner, m_viewSize));

        glBindVertexArray(s_squareVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glUseProgram(0);
    }

    void TexViewer::pack() {
        m_initViewSize = vec2(size()) / vec2(m_texSize);
        m_viewSize = m_initViewSize;
    }

    void TexViewer::cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) {
        if (ui::isMouseButtonPressed(0)) {
            vec2 factor(m_viewSize / vec2(size()));
            moveCenter(-factor * vec2(delta));
        }
    }

    void TexViewer::scrollEvent(const ivec2 & delta) {
        vec2 a0(vec2(ui::cursorPosition() - (position() + size() / 2)) / vec2(size()));
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