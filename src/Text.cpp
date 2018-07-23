#include "Text.hpp"

#include <iostream>

#include "glad/glad.h"
#include "stb_image.h"

#include "Program.h"



static const std::string k_fontTextureFilename("ubuntu_mono.png");
static const std::string k_vertFilename("text.vert.glsl"), k_fragFilename("text.frag.glsl");

static GLuint f_squareVBO;
static GLuint f_fontTex;
static glm::ivec2 f_fontSize;
static std::unique_ptr<Program> f_prog;
static glm::vec2 f_charTexCoords[256];



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

    // Precalculate character texture coordinates;

    for (int i(0); i < 256; ++i) {
        glm::ivec2 cell(i & 0xF, i >> 4);
        f_charTexCoords[i].x = float(cell.x) * (1.0f / 16.0f);
        f_charTexCoords[i].y = float(cell.y) * (1.0f / 16.0f);
    }

    return true;
}

Text::Text(const std::string & string, const glm::ivec2 & position, const glm::ivec2 & align, const glm::vec3 & color) :
    m_string(string),
    m_position(position),
    m_align(align),
    m_color(color),
    m_isStringChange(!m_string.empty()),
    m_vao(0), m_charVBO(0),
    m_charData()
{}

void Text::string(const std::string & string) {
    if (string != m_string) {
        m_string = string;
        m_isStringChange = true;
    }
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
        detCharData();
        upload();
        m_isStringChange = false;
    }

    f_prog->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, f_fontTex);

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
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, f_squareVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_charVBO);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec2), 0);
    glVertexAttribDivisor(1, 1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec2), reinterpret_cast<void *>(sizeof(glm::vec2)));
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
    int totalHeight(nLines * f_fontSize.y);

    m_charData.clear();

    glm::ivec2 charPos;
    charPos.y = m_position.y - f_fontSize.y;
    if (m_align.y < 0) charPos.y += totalHeight;
    else if (m_align.y == 0) charPos.y += totalHeight / 2;

    for (int lineI(0), strI(0); lineI < nLines; ++lineI) {
        int lineLength(m_lineEndIndices[lineI] - strI);
        int lineWidth(lineLength * f_fontSize.x);

        charPos.x = m_position.x;
        if (m_align.x < 0) charPos.x -= lineWidth;
        else if (m_align.x == 0) charPos.x -= lineWidth / 2;

        for (int i(0); i < lineLength; ++i) {        
            m_charData.emplace_back(charPos);
            m_charData.emplace_back(f_charTexCoords[m_string[strI + i]]);

            charPos.x += f_fontSize.x;            
        }

        charPos.y -= f_fontSize.y;
        strI += lineLength + 1;
    }
}

void Text::upload() {
    glBindBuffer(GL_ARRAY_BUFFER, m_charVBO);
    glBufferData(GL_ARRAY_BUFFER, m_charData.size() * sizeof(glm::vec2), m_charData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}