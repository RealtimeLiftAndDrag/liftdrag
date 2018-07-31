#include "Graph.hpp"

#include <iostream>

#include "glad/glad.h"
#include "GLSL.h"
#include "Program.h"



static const std::string k_curveVertFilename("graph_curve.vert.glsl"), k_curveFragFilename("graph_curve.frag.glsl");
static const std::string k_linesVertFilename("graph_lines.vert.glsl"), k_linesFragFilename("graph_lines.frag.glsl");

static std::unique_ptr<Program> f_curveProg, f_linesProg;
static GLuint f_squareVAO, f_squareVBO;

static float detGridSize(float v) {
    float g(std::pow(10.0f, std::round(std::log10(v)) - 1.0f));
    float n(v / g);
    if (n > 10.0f) g *= 2.5f;
    return g;
}



Graph::Curve::Curve(const glm::vec3 & color, int maxNPoints) :
    points(),
    color(color),
    maxNPoints(maxNPoints),
    vao(0), vbo(0),
    isSetup(false), isChange(false)
{
    points.reserve(maxNPoints);
}

bool Graph::Curve::setup() {
    // Setup curve vao
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, maxNPoints * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (glGetError() != GL_NO_ERROR) {
        return false;
    }

    return true;
}

void Graph::Curve::upload() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, glm::min(int(points.size()), maxNPoints) * sizeof(glm::vec2), points.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Graph::Curve::cleanup() {
    glDeleteVertexArrays(1, &vao);
    vao = 0;
    glDeleteBuffers(1, &vbo);
    vbo = 0;
}



bool Graph::setup(const std::string & resourceDir) {
    // Setup curve shader
    f_curveProg.reset(new Program());
    f_curveProg->setShaderNames(resourceDir + "/shaders/" + k_curveVertFilename, resourceDir + "/shaders/" + k_curveFragFilename);
    if (!f_curveProg->init()) {
        std::cerr << "Failed to initialize curve program" << std::endl;
        return false;
    }
    f_curveProg->addUniform("u_viewMin");
    f_curveProg->addUniform("u_viewMax");
    f_curveProg->addUniform("u_color");

    // Setup lines shader
    f_linesProg.reset(new Program());
    f_linesProg->setShaderNames(resourceDir + "/shaders/" + k_linesVertFilename, resourceDir + "/shaders/" + k_linesFragFilename);
    if (!f_linesProg->init()) {
        std::cerr << "Failed to initialize lines program" << std::endl;
        return false;
    }
    f_linesProg->addUniform("u_viewMin");
    f_linesProg->addUniform("u_viewMax");
    f_linesProg->addUniform("u_viewportSize");
    f_linesProg->addUniform("u_gridSize");
    f_linesProg->addUniform("u_isFocusX");
    f_linesProg->addUniform("u_focusX");

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

Graph::Graph(const glm::vec2 & viewMin, const glm::vec2 & viewMax) :
    m_viewMin(viewMin), m_viewMax(viewMax),
    m_gridSize(detGridSize(m_viewMax.x - m_viewMin.x), detGridSize(m_viewMax.y - m_viewMin.y)),
    m_isFocusX(false),
    m_focusX()
{}

void Graph::addCurve(const glm::vec3 & color, int maxNPoints) {
    m_curves.emplace_back(color, maxNPoints);
}

void Graph::render(const glm::ivec2 & viewportSize) {
    // Render lines

    f_linesProg->bind();
    glUniform2f(f_linesProg->getUniform("u_viewMin"), m_viewMin.x, m_viewMin.y);
    glUniform2f(f_linesProg->getUniform("u_viewMax"), m_viewMax.x, m_viewMax.y);
    glUniform2f(f_linesProg->getUniform("u_gridSize"), m_gridSize.x, m_gridSize.y);
    glUniform2f(f_linesProg->getUniform("u_viewportSize"), float(viewportSize.x), float(viewportSize.y));
    glUniform1i(f_linesProg->getUniform("u_isFocusX"), m_isFocusX);
    if (m_isFocusX) glUniform1f(f_linesProg->getUniform("u_focusX"), m_focusX);
    glBindVertexArray(f_squareVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Render curves

    f_curveProg->bind();
    glUniform2f(f_curveProg->getUniform("u_viewMin"), m_viewMin.x, m_viewMin.y);
    glUniform2f(f_curveProg->getUniform("u_viewMax"), m_viewMax.x, m_viewMax.y);

    for (Curve & curve : m_curves) {
        if (!curve.isSetup) {
            if (!curve.setup()) {
                continue;
            }
            curve.isSetup = true;
        }

        if (curve.points.empty()) {
            continue;
        }

        if (curve.isChange) {
            curve.upload();
            curve.isChange = false;
        }

        glUniform3f(f_curveProg->getUniform("u_color"), curve.color.r, curve.color.g, curve.color.b);
        glBindVertexArray(curve.vao);
        if (curve.points.size() == 1) {
            glDrawArrays(GL_POINTS, 0, 1);
        }
        else {
            glDrawArrays(GL_LINE_STRIP, 0, glm::min(int(curve.points.size()), curve.maxNPoints));
        }
    }

    glUseProgram(0);
    glBindVertexArray(0);
}

const std::vector<glm::vec2> & Graph::accessPoints(int curveI) const {
    return m_curves[curveI].points;
}

std::vector<glm::vec2> & Graph::mutatePoints(int curveI) {
    m_curves[curveI].isChange = true;
    return m_curves[curveI].points;
}

void Graph::setView(const glm::vec2 & min, const glm::vec2 & max) {
    m_viewMin = min;
    m_viewMax = max;
    m_gridSize.x = detGridSize(m_viewMax.x - m_viewMin.x);
    m_gridSize.y = detGridSize(m_viewMax.y - m_viewMin.y);
}

void Graph::zoomView(float factor) {
    glm::vec2 viewSize(m_viewMax - m_viewMin);
    glm::vec2 viewSizeDelta(viewSize * (factor - 1.0f));
    setView(m_viewMin - viewSizeDelta * 0.5f, m_viewMax + viewSizeDelta * 0.5f);
}

void Graph::moveView(const glm::vec2 & delta) {
    setView(m_viewMin + delta, m_viewMax + delta);
}

void Graph::focusX(float x) {
    m_focusX = x;
    m_isFocusX = true;
}

void Graph::removeFocusX() {
    m_isFocusX = false;
}

void Graph::cleanup() {
    for (Curve & curve : m_curves) curve.cleanup();
}