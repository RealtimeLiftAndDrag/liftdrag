#include "TexViewer.hpp"

#include <iostream>

#include "glad/glad.h"
#include "GLSL.h"
#include "Program.h"


static constexpr int k_maxZoom(8);

static const std::string k_texVertFilename("tex.vert"), k_texFragFilename("tex.frag");

static unq<Program> s_texProg;
static uint s_squareVAO, s_squareVBO;




bool TexViewer::setup(const std::string & resourceDir) {
    // Setup tex shader
    s_texProg.reset(new Program());
    s_texProg->setShaderNames(resourceDir + "/shaders/" + k_texVertFilename, resourceDir + "/shaders/" + k_texFragFilename);
    if (!s_texProg->init()) {
        std::cerr << "Failed to initialize tex program" << std::endl;
        return false;
    }
    s_texProg->addUniform("u_tex");
    s_texProg->addUniform("u_viewBounds");
    glUseProgram(s_texProg->pid);
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
    m_viewSize(),
    m_center(0.5f),
    m_zoom(0),
    m_zoomFactor(1.0f)
{}

void TexViewer::render() const {
    glViewport(m_position.x, m_position.y, m_size.x, m_size.y);

    glUseProgram(s_texProg->pid);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex);

    vec2 corner(m_center - m_viewSize * 0.5f / m_zoomFactor);
    glUniform4f(s_texProg->getUniform("u_viewBounds"), corner.x, corner.y, m_viewSize.x / m_zoomFactor, m_viewSize.y / m_zoomFactor);

    glBindVertexArray(s_squareVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
}

void TexViewer::pack() {
    m_viewSize = vec2(m_size) / vec2(m_texSize);
}

void TexViewer::cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) {
    if (UI::isMouseButtonPressed(0)) {
        vec2 factor(m_viewSize / vec2(m_size));
        moveCenter(-factor * vec2(delta) / m_zoomFactor);
    }
}

void TexViewer::scrollEvent(const ivec2 & delta) {
    if (delta.y < 0) zoomOut();
    else if (delta.y > 0) zoomIn();
}

void TexViewer::center(const vec2 & p) {
    m_center = p;
}

void TexViewer::moveCenter(const vec2 & delta) {
    m_center += delta;
}

void TexViewer::zoom(int zoom) {
    m_zoom = glm::clamp(zoom, -k_maxZoom, k_maxZoom);
    if      (m_zoom > 0) m_zoomFactor = float(1 << m_zoom);
    else if (m_zoom < 0) m_zoomFactor = 1.0f / float(1 << -m_zoom);
}

void TexViewer::zoomIn() {
    zoom(m_zoom + 1);
}

void TexViewer::zoomOut() {
    zoom(m_zoom - 1);
}

void TexViewer::zoomReset() {
    zoom(0);
}