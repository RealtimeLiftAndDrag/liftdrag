#include "Graph.hpp"

#include <iostream>

#include "glad/glad.h"
#include "GLSL.h"
#include "Program.h"



namespace {



const std::string k_graphVertFilename("graph.vert.glsl"), k_graphFragFilename("graph.frag.glsl");

bool isStaticSetup(false);
std::unique_ptr<Program> f_graphProg;

bool doStaticSetup(const std::string & resourcesDir) {
    f_graphProg.reset(new Program());
    f_graphProg->setShaderNames(resourcesDir + "/shaders/" + k_graphVertFilename, resourcesDir + "/shaders/" + k_graphFragFilename);
    if (!f_graphProg->init()) {
        std::cerr << "Failed to initialize graph program" << std::endl;
        return false;
    }
    f_graphProg->addUniform("u_min");
    f_graphProg->addUniform("u_max");

    return true;
}



}



Graph::Graph(int maxNPoints) :
    m_points(),
    m_maxNPoints(maxNPoints),
    m_min(), m_max(),
    m_vao(0), m_vbo(0)
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

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
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
    glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
    glDeleteBuffers(1, &m_vbo);
    m_vbo = 0;
}

void Graph::render() const {
    if (m_points.size() > 0) {
        f_graphProg->bind();

        glUniform2f(f_graphProg->getUniform("u_min"), m_min.x, m_min.y);
        glUniform2f(f_graphProg->getUniform("u_max"), m_max.x, m_max.y);
        
        glBindVertexArray(m_vao);
        glDrawArrays(GL_LINE_STRIP, 0, glm::min(int(m_points.size()), m_maxNPoints));
        glBindVertexArray(0);

        f_graphProg->unbind();
    }
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

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, glm::min(int(m_points.size()), m_maxNPoints) * sizeof(glm::vec2), m_points.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}