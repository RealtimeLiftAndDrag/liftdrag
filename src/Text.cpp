#include "Text.hpp"

#include <iostream>

#include "glad/glad.h"
#include "stb_image.h"

#include "Program.h"



namespace {



const std::string k_fontTextureFilename("ubuntu_mono.png");
const std::string k_vertFilename("text.vert.glsl"), k_fragFilename("text.frag.glsl");

GLuint f_squareVBO;
GLuint f_fontTex;
glm::ivec2 f_fontSize;
std::unique_ptr<Program> f_prog;



}



bool Text::setup(const std::string & resourceDir) {

    // Setup square vao

    glm::vec2 locs[6]{
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
        { 0.0f, 0.0f }
    }; 
    glGenBuffers(1, &f_squareVBO);
    glBindBuffer(GL_ARRAY_BUFFER, f_squareVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(glm::vec2), locs, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error setting up square vao" << std::endl;
        return false;
    }

    // Setup font texture

    int texWidth(0), texHeight(0), texChannels(0);
    unsigned char * texData(stbi_load((resourceDir + "/" + k_fontTextureFilename).c_str(), &texWidth, &texHeight, &texChannels, 1));
    if (!texData) {
        std::cerr << stbi_failure_reason() << std::endl;
        std::cerr << "Failed to load font texture" << std::endl;
        return false;
    }
    if (texWidth % 16 != 0 || texHeight % 16 != 0) {
        std::cerr << "Font texture must be a 16 row and 16 column grid of glyphs" << std::endl;
        return false;
    }
    f_fontSize.x = texWidth / 16; f_fontSize.y = texHeight / 16;

    glGenTextures(1, &f_fontTex);
    glBindTexture(GL_TEXTURE_2D, f_fontTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texWidth, texHeight, 0, GL_RED, GL_UNSIGNED_BYTE, texData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(texData);

    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error setting up font texture" << std::endl;
        return false;
    }

    // Setup shader

    f_prog.reset(new Program());
    f_prog->setShaderNames(resourceDir + "/shaders/" + k_vertFilename, resourceDir + "/shaders/" + k_fragFilename);
    if (!f_prog->init()) {
        std::cerr << "Failed to initialize program" << std::endl;
        return false;
    }
    f_prog->addUniform("u_fontSize");
    f_prog->addUniform("u_viewportSize");
    f_prog->addUniform("u_linePos");
    f_prog->addUniform("u_tex");
    f_prog->addUniform("u_color");
    f_prog->bind();
    glUniform2f(f_prog->getUniform("u_fontSize"), float(f_fontSize.x), float(f_fontSize.y));
    glUniform1i(f_prog->getUniform("u_tex"), 0);
    f_prog->unbind();

    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error setting up shader" << std::endl;
        return false;
    }

    return true;
}

Text::Text(const std::string & string, const glm::ivec2 & position, const glm::vec3 & color) :
    m_string(string),
    m_position(position),
    m_color(color),
    m_isStringChange(!m_string.empty()),
    m_vao(0), m_charVBO(0)
{}

void Text::string(const std::string & string) {
    m_string = string;
    m_isStringChange = true;
}

void Text::position(const glm::ivec2 & position) {
    m_position = position;
}

void Text::color(const glm::vec3 & color) {
    m_color = color;
}

void Text::render(const glm::ivec2 & viewportSize) {
    if (m_vao == 0) {
        if (!prepare()) {
            return;
        }
    }

    if (m_string.empty()) {
        return;
    }

    if (m_isStringChange) {
        upload();
        m_isStringChange = false;
    }

    f_prog->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, f_fontTex);

    glUniform2f(f_prog->getUniform("u_linePos"), float(m_position.x), float(m_position.y));
    glUniform3f(f_prog->getUniform("u_color"), m_color.r, m_color.g, m_color.b);
    glUniform2f(f_prog->getUniform("u_viewportSize"), float(viewportSize.x), float(viewportSize.y));

    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, int(m_string.length()));
    glBindVertexArray(0);

    f_prog->unbind();
}

bool Text::prepare() {    
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_charVBO);

    glBindVertexArray(m_vao);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, f_squareVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_charVBO);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 0, 0);
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (glGetError() != GL_NO_ERROR) {
        return false;
    }

    return true;
}

void Text::upload() {
    glBindBuffer(GL_ARRAY_BUFFER, m_charVBO);
    glBufferData(GL_ARRAY_BUFFER, m_string.length(), m_string.c_str(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}