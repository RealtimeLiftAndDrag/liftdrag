#include "Text.hpp"

#include <iostream>

#include "glad/glad.h"
#include "stb_image.h"

#include "Program.h"



static const std::string k_fontTextureFilename("ubuntu_mono.png");
static const std::string k_vertFilename("text.vert"), k_fragFilename("text.frag");

static uint s_squareVBO;
static uint s_fontTex;
static ivec2 s_fontSize;
static std::unique_ptr<Program> s_prog;
static vec2 s_charTexCoords[256];



bool Text::setup(const std::string & resourceDir) {

    // Setup square vao

    vec2 locs[6]{
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
        { 0.0f, 0.0f }
    }; 
    glGenBuffers(1, &s_squareVBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_squareVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec2), locs, GL_STATIC_DRAW);
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
    s_fontSize.x = texWidth / 16; s_fontSize.y = texHeight / 16;

    glGenTextures(1, &s_fontTex);
    glBindTexture(GL_TEXTURE_2D, s_fontTex);
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

    s_prog.reset(new Program());
    s_prog->setShaderNames(resourceDir + "/shaders/" + k_vertFilename, resourceDir + "/shaders/" + k_fragFilename);
    if (!s_prog->init()) {
        std::cerr << "Failed to initialize program" << std::endl;
        return false;
    }
    s_prog->addUniform("u_fontSize");
    s_prog->addUniform("u_viewportSize");
    s_prog->addUniform("u_tex");
    s_prog->addUniform("u_color");
    s_prog->bind();
    glUniform2f(s_prog->getUniform("u_fontSize"), float(s_fontSize.x), float(s_fontSize.y));
    glUniform1i(s_prog->getUniform("u_tex"), 0);
    s_prog->unbind();

    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error setting up shader" << std::endl;
        return false;
    }

    // Precalculate character texture coordinates;

    for (int i(0); i < 256; ++i) {
        ivec2 cell(i & 0xF, i >> 4);
        s_charTexCoords[i].x = float(cell.x) * (1.0f / 16.0f);
        s_charTexCoords[i].y = float(cell.y) * (1.0f / 16.0f);
    }

    return true;
}

Text::Text(const std::string & string, const ivec2 & position, const ivec2 & align, const vec3 & color) :
    m_string(string),
    m_position(position),
    m_align(align),
    m_color(color),
    m_isStringChange(!m_string.empty()),
    m_isPositionChange(true),
    m_vao(0), m_charVBO(0),
    m_charData()
{}

void Text::string(const std::string & string) {
    if (string != m_string) {
        m_string = string;
        m_isStringChange = true;
    }
}

void Text::position(const ivec2 & position) {
    if (position != m_position) {
        m_position = position;
        m_isPositionChange = true;
    }
}

void Text::color(const vec3 & color) {
    m_color = color;
}

void Text::render(const ivec2 & viewportSize) {
    if (m_vao == 0) {
        if (!prepare()) {
            return;
        }
    }

    if (m_string.empty()) {
        return;
    }

    if (m_isStringChange || m_isPositionChange) {
        detCharData();
        upload();
        m_isStringChange = false;
        m_isPositionChange = false;
    }

    s_prog->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_fontTex);

    glUniform3f(s_prog->getUniform("u_color"), m_color.r, m_color.g, m_color.b);
    glUniform2f(s_prog->getUniform("u_viewportSize"), float(viewportSize.x), float(viewportSize.y));

    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, int(m_string.length()));
    glBindVertexArray(0);

    s_prog->unbind();
}

void Text::cleanup() {
    glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
    glDeleteBuffers(1, &m_charVBO);
    m_charVBO = 0;
}

bool Text::prepare() {    
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_charVBO);

    glBindVertexArray(m_vao);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, s_squareVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_charVBO);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(vec2), 0);
    glVertexAttribDivisor(1, 1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(vec2), reinterpret_cast<void *>(sizeof(vec2)));
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (glGetError() != GL_NO_ERROR) {
        return false;
    }

    return true;
}

void Text::detCharData() {
    m_lineEndIndices.clear();
    for (int i(0); i < m_string.length(); ++i) {
        if (m_string[i] == '\n') {
            m_lineEndIndices.push_back(i);
        }
    }
    m_lineEndIndices.push_back(int(m_string.length()));
    int nLines(int(m_lineEndIndices.size()));
    int totalHeight(nLines * s_fontSize.y);

    m_charData.clear();

    ivec2 charPos;
    charPos.y = m_position.y - s_fontSize.y;
    if (m_align.y < 0) charPos.y += totalHeight;
    else if (m_align.y == 0) charPos.y += totalHeight / 2;

    for (int lineI(0), strI(0); lineI < nLines; ++lineI) {
        int lineLength(m_lineEndIndices[lineI] - strI);
        int lineWidth(lineLength * s_fontSize.x);

        charPos.x = m_position.x;
        if (m_align.x < 0) charPos.x -= lineWidth;
        else if (m_align.x == 0) charPos.x -= lineWidth / 2;

        for (int i(0); i < lineLength; ++i) {        
            m_charData.emplace_back(charPos);
            m_charData.emplace_back(s_charTexCoords[unsigned char(m_string[strI + i])]);

            charPos.x += s_fontSize.x;            
        }

        charPos.y -= s_fontSize.y;
        strI += lineLength + 1;
    }
}

void Text::upload() {
    glBindBuffer(GL_ARRAY_BUFFER, m_charVBO);
    glBufferData(GL_ARRAY_BUFFER, m_charData.size() * sizeof(vec2), m_charData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}