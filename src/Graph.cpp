#include "Graph.hpp"

#include <iostream>

#include "glad/glad.h"
#include "GLSL.h"
#include "Program.h"



namespace {



const std::string k_curveVertFilename("graph_curve.vert.glsl"), k_curveFragFilename("graph_curve.frag.glsl");
const std::string k_linesVertFilename("graph_lines.vert.glsl"), k_linesFragFilename("graph_lines.frag.glsl");

bool isStaticSetup(false);
std::unique_ptr<Program> f_curveProg, f_linesProg;
GLuint f_squareVAO, f_squareVBO;

bool doStaticSetup(const std::string & resourcesDir) {
    // Setup curve shader
    f_curveProg.reset(new Program());
    f_curveProg->setShaderNames(resourcesDir + "/shaders/" + k_curveVertFilename, resourcesDir + "/shaders/" + k_curveFragFilename);
    if (!f_curveProg->init()) {
        std::cerr << "Failed to initialize curve program" << std::endl;
        return false;
    }
    f_curveProg->addUniform("u_min");
    f_curveProg->addUniform("u_max");

    // Setup lines shader
    f_linesProg.reset(new Program());
    f_linesProg->setShaderNames(resourcesDir + "/shaders/" + k_linesVertFilename, resourcesDir + "/shaders/" + k_linesFragFilename);
    if (!f_linesProg->init()) {
        std::cerr << "Failed to initialize lines program" << std::endl;
        return false;
    }
    f_linesProg->addUniform("u_min");
    f_linesProg->addUniform("u_max");
    f_linesProg->addUniform("u_viewSize");
    f_linesProg->addUniform("u_gridSize");

    // Setup square vao
    glm::vec2 locs[6]{
        { -1.0f, -1.0f },
        {  1.0f, -1.0f },
        {  1.0f,  1.0f },
        {  1.0f,  1.0f },
        { -1.0f,  1.0f },
        { -1.0f, -1.0f }
    };    
    glGenVertexArrays(1, &f_squareVAO);
    glBindVertexArray(f_squareVAO);    
    glGenBuffers(1, &f_squareVBO);
    glBindBuffer(GL_ARRAY_BUFFER, f_squareVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(glm::vec2), locs, GL_STATIC_DRAW);
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

float detGridSize(float v) {
    return std::pow(10.0f, std::round(std::log10(v)) - 1.0f);
}



}



Graph::Graph(int maxNPoints) :
    m_points(),
    m_maxNPoints(maxNPoints),
    m_min(), m_max(),
    m_curveVAO(0), m_curveVBO(0)
{
    m_points.reserve(m_maxNPoints);
}

bool Graph::setup(const std::string & resourcesDir) {
    if (!isStaticSetup) {
        if (!doStaticSetup(resourcesDir)) {
            std::cerr << "Failed static setup" << std::endl;
            return false;
        }
        isStaticSetup = true;
    }

    // Setup curve vao
    glGenVertexArrays(1, &m_curveVAO);
    glBindVertexArray(m_curveVAO);    
    glGenBuffers(1, &m_curveVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_curveVBO);
    glBufferData(GL_ARRAY_BUFFER, m_maxNPoints * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

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

void Graph::cleanup() {
    glDeleteVertexArrays(1, &m_curveVAO);
    m_curveVAO = 0;
    glDeleteBuffers(1, &m_curveVBO);
    m_curveVBO = 0;
}

void Graph::render() const {
    // Render lines    
    f_linesProg->bind();
    glUniform2f(f_linesProg->getUniform("u_min"), m_min.x, m_min.y);
    glUniform2f(f_linesProg->getUniform("u_max"), m_max.x, m_max.y);
    glUniform2f(f_linesProg->getUniform("u_viewSize"), 400.0f, 300.0f);
    glUniform2f(f_linesProg->getUniform("u_gridSize"), m_gridSize.x, m_gridSize.y);
    glBindVertexArray(f_squareVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Render curve    
    if (m_points.size() > 0) {
        f_curveProg->bind();
        glUniform2f(f_curveProg->getUniform("u_min"), m_min.x, m_min.y);
        glUniform2f(f_curveProg->getUniform("u_max"), m_max.x, m_max.y);        
        glBindVertexArray(m_curveVAO);
        if (m_points.size() == 1) {
            glDrawArrays(GL_POINTS, 0, 1);
        }
        else {
            glDrawArrays(GL_LINE_STRIP, 0, glm::min(int(m_points.size()), m_maxNPoints));
        }
    }

    glUseProgram(0);
    glBindVertexArray(0);
}

std::vector<glm::vec2> & Graph::accessPoints() {
    return m_points;
}

void Graph::refresh() {
    m_min.x = m_min.y = std::numeric_limits<float>::infinity();
    m_max.x = m_max.y = -std::numeric_limits<float>::infinity();
    for (const auto & p : m_points) {
        if (p.x < m_min.x) m_min.x = p.x;
        if (p.x > m_max.x) m_max.x = p.x;
        if (p.y < m_min.y) m_min.y = p.y;
        if (p.y > m_max.y) m_max.y = p.y;
    }
    glm::vec2 size(m_max - m_min);
    glm::vec2 padding(size * 0.1f);
    if (padding.x == 0.0f) padding.x = 0.1f;
    if (padding.y == 0.0f) padding.y = 0.1f;
    m_min -= padding;
    m_max += padding;
    size = m_max - m_min;

    m_gridSize.x = detGridSize(size.x);
    m_gridSize.y = detGridSize(size.y);

    glBindBuffer(GL_ARRAY_BUFFER, m_curveVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, glm::min(int(m_points.size()), m_maxNPoints) * sizeof(glm::vec2), m_points.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}