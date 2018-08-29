#include "TexViewer.hpp"

#include <iostream>

#include "glad/glad.h"
#include "GLSL.h"
#include "Program.h"



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
    glUseProgram(s_texProg->pid);
    glUniform1i(s_texProg->getUniform("u_tex"), 0);
    glUseProgram(0);

    // Setup square vao
    vec2 positions[6]{
        { -1.0f, -1.0f },
        {  1.0f, -1.0f },
        {  1.0f,  1.0f },
        {  1.0f,  1.0f },
        { -1.0f,  1.0f },
        { -1.0f, -1.0f }
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
    m_texSize(texSize)
{}

void TexViewer::render(const ivec2 & position) const {
    glViewport(position.x, position.y, m_size.x, m_size.y);

    glUseProgram(s_texProg->pid);
    glBindVertexArray(s_squareVAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glUseProgram(0);
}

void TexViewer::pack(const ivec2 & size) {
    m_size = size;
}

void TexViewer::cleanup() {
    
}