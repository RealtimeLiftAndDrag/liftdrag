#include "Text.hpp"

#include <iostream>

#include "glad/glad.h"
#include "stb/stb_image.h"

#include "Shader.hpp"



static const std::string & k_fontTextureFilename("ubuntu_mono.png");
static const std::string & k_vertFilename("text.vert"), k_fragFilename("text.frag");

static u32 s_squareVBO;
static u32 s_fontTex;
static ivec2 s_fontSize;
static unq<Shader> s_prog;
static vec2 s_charTexCoords[256];



bool Text::setup() {

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

    // TODO: abstract this out
    int texWidth(0), texHeight(0), texChannels(0);
    std::string resourcesPath(g_resourcesDir + "/Common/");
    unsigned char * texData(stbi_load((resourcesPath + k_fontTextureFilename).c_str(), &texWidth, &texHeight, &texChannels, 1));
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

    std::string shadersPath(resourcesPath + "shaders/");
    if (!(s_prog = Shader::load(shadersPath + k_vertFilename, shadersPath + k_fragFilename))) {
        std::cerr << "Failed to load program" << std::endl;
        return false;
    }
    s_prog->bind();
    s_prog->uniform("u_fontSize", vec2(s_fontSize));
    s_prog->uniform("u_tex", 0);
    Shader::unbind();

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

const ivec2 & Text::fontSize() {
    return s_fontSize;
}

ivec2 Text::detDimensions(const std::string & str) {
    if (str.empty()) {
        return {};
    }

    ivec2 size(0, 1);

    int width(0);
    for (char c : str) {
        if (c == '\n') {
            if (width > size.x) size.x = width;
            ++size.y;

            width = 0;
            continue;
        }

        ++width;
    }
    if (width > size.x) size.x = width;

    return size;
}

ivec2 Text::detSize(const ivec2 & dimensions) {
    return dimensions * s_fontSize;
}

ivec2 Text::detSize(const std::string & str) {
    return detSize(detDimensions(str));
}

Text::Text() :
    m_string(),
    m_align(1),
    m_color(0.5f, 0.5f, 0.5f, 1.0f)
{}

Text::Text(const std::string & string, const ivec2 & align, const vec4 & color) :
    m_string(string),
    m_align(align),
    m_color(color)
{
    if (string.size()) {
        detLineSpecs();
        detDimensions();
        detSize();
        detBasepoint();
        m_isChange = true;
    }
}

void Text::render(const ivec2 & position) const {
    if (m_vao == 0) {
        if (!prepare()) {
            return;
        }
    }

    if (m_string.empty() || m_color.a == 0.0f) {
        return;
    }

    if (m_isChange) {
        upload();
        m_isChange = false;
    }

    glViewport(position.x, position.y, m_size.x, m_size.y);

    s_prog->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_fontTex);

    s_prog->uniform("u_color", m_color);
    s_prog->uniform("u_viewportSize", vec2(m_size));

    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, int(m_string.length()));
    glBindVertexArray(0);

    Shader::unbind();
}

void Text::renderAt(const ivec2 & basepoint) const {
    render(basepoint - m_basepoint);
}

void Text::renderWithin(const ivec2 & boundsPos, const ivec2 & boundsSize) const {
    ivec2 offset;

    if (m_align.x > 0) offset.x = 0;
    else if (m_align.x < 0) offset.x = boundsSize.x - m_size.x;
    else offset.x = (boundsSize.x - m_size.x) / 2;

    if (m_align.y > 0) offset.y = boundsSize.y - m_size.y;
    else if (m_align.y < 0) offset.y = 0;
    else offset.y = (boundsSize.y - m_size.y) / 2;

    render(boundsPos + offset);
}

std::string Text::string(const std::string & string) {
    return this->string(std::string(string));
}

std::string Text::string(std::string && string) {
    if (string != m_string) {
        std::swap(m_string, string);

        detLineSpecs();
        detDimensions();
        detSize();
        detBasepoint();

        m_isChange = true;
    }

    return move(string);
}

void Text::color(const vec4 & color) {
    m_color = color;
}

void Text::align(const ivec2 & align) {
    if (align != m_align) {
        m_align = align;

        detBasepoint();

        m_isChange = true;
    }
}

bool Text::prepare() const {
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

void Text::detLineSpecs() {
    m_lineSpecs.clear();
    int startI(0);
    for (int i(0); i < int(m_string.length()); ++i) {
        if (m_string[i] == '\n') {
            m_lineSpecs.emplace_back(startI, i - startI);
            startI = i + 1;
        }
    }
    m_lineSpecs.emplace_back(startI, int(m_string.length()) - startI);
}

void Text::detDimensions() {
    m_dimensions.x = 0;
    for (const auto & spec : m_lineSpecs) {
        if (spec.y > m_dimensions.x) m_dimensions.x = spec.y;
    }
    m_dimensions.y = int(m_lineSpecs.size());
}

void Text::detSize() {
    m_size = m_dimensions * s_fontSize;
}

void Text::detBasepoint() {
    if (m_align.x > 0) m_basepoint.x = 0;
    else if (m_align.x < 0) m_basepoint.x = m_size.x;
    else m_basepoint.x = m_size.x / 2;

    if (m_align.y > 0) m_basepoint.y = m_size.y - s_fontSize.y;
    else if (m_align.y < 0) m_basepoint.y = 0;
    else m_basepoint.y = m_size.y / 2;
}

void Text::upload() const {
    std::vector<vec2> charData;
    charData.reserve(m_string.size() * 2);

    ivec2 charPos;
    charPos.y = m_size.y - s_fontSize.y;

    for (int lineI(0); lineI < m_dimensions.y; ++lineI) {
        int strI(m_lineSpecs[lineI].x);
        int lineLength(m_lineSpecs[lineI].y);
        int lineWidth(lineLength * s_fontSize.x);

        if (m_align.x > 0) charPos.x = 0;
        else if (m_align.x < 0) charPos.x = m_size.x - lineWidth;
        else charPos.x = (m_size.x - lineWidth) / 2;

        for (int i(0); i < lineLength; ++i) {
            charData.emplace_back(charPos);
            charData.emplace_back(s_charTexCoords[unsigned char(m_string[strI + i])]);

            charPos.x += s_fontSize.x;
        }

        charPos.y -= s_fontSize.y;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_charVBO);
    glBufferData(GL_ARRAY_BUFFER, charData.size() * sizeof(vec2), charData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}