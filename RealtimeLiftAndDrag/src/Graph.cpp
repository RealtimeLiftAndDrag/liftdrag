#include "Graph.hpp"

#include <iostream>
#include <sstream>

#include "glad.h"
#include "GLSL.h"
#include "Program.h"
#include "Util.hpp"



static const std::string k_curveVertFilename("graph_curve.vert"), k_curveFragFilename("graph_curve.frag");
static const std::string k_linesVertFilename("graph_lines.vert"), k_linesFragFilename("graph_lines.frag");

static constexpr float k_zoomFactor(1.1f);
static constexpr float k_invZoomFactor(1.0f / k_zoomFactor);

static constexpr int k_gridTextPrecision(3);
static constexpr int k_gridTextLength(10);

static const vec3 k_gridColor(0.15f);
static const vec3 k_originColor(0.3f);
static const vec3 k_focusColor(0.5f);

static unq<Program> s_curveProg, s_linesProg;
static uint s_lineVAO, s_lineVBO;



static float detGridSize(float v) {
    float g(std::pow(10.0f, std::round(std::log10(v)) - 1.0f));
    float n(v / g);
    if (n > 10.0f) g *= 2.5f;
    return g;
}



Graph::Curve::Curve(const std::string & label, const vec3 & color, int maxNPoints) :
    points(),
    label(label),
    color(color),
    maxNPoints(maxNPoints),
    vao(0), vbo(0),
    isSetup(false), isChange(false)
{
    points.reserve(maxNPoints);
}

bool Graph::Curve::setup() const {
    // Setup curve vao
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, maxNPoints * sizeof(vec2), nullptr, GL_DYNAMIC_DRAW);

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

void Graph::Curve::upload() const {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, glm::min(int(points.size()), maxNPoints) * sizeof(vec2), points.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Graph::Curve::cleanup() {
    glDeleteVertexArrays(1, &vao);
    vao = 0;
    glDeleteBuffers(1, &vbo);
    vbo = 0;
}

float Graph::Curve::valAt(float x) const {
    int i(0);
    while (i < points.size() && points[i].x < x) ++i;

    if (i == 0 || i == points.size()) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    return glm::mix(points[i - 1].y, points[i].y, (x - points[i - 1].x) / (points[i].x - points[i - 1].x));
}



bool Graph::setup(const std::string & resourceDir) {
    // Setup curve shader
    s_curveProg.reset(new Program());
    s_curveProg->setShaderNames(resourceDir + "/shaders/" + k_curveVertFilename, resourceDir + "/shaders/" + k_curveFragFilename);
    if (!s_curveProg->init()) {
        std::cerr << "Failed to initialize curve program" << std::endl;
        return false;
    }
    s_curveProg->addUniform("u_viewMin");
    s_curveProg->addUniform("u_viewMax");
    s_curveProg->addUniform("u_color");

    // Setup lines shader
    s_linesProg.reset(new Program());
    s_linesProg->setShaderNames(resourceDir + "/shaders/" + k_linesVertFilename, resourceDir + "/shaders/" + k_linesFragFilename);
    if (!s_linesProg->init()) {
        std::cerr << "Failed to initialize lines program" << std::endl;
        return false;
    }
    s_linesProg->addUniform("u_origin");
    s_linesProg->addUniform("u_orient");
    s_linesProg->addUniform("u_delta");
    s_linesProg->addUniform("u_color");

    // Setup line vao
    float points[2]{ -1.0f, 1.0f };
    glGenVertexArrays(1, &s_lineVAO);
    glBindVertexArray(s_lineVAO);    
    glGenBuffers(1, &s_lineVBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float), points, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }

    return true;
}



Graph::PlotComp::PlotComp(Graph & graph, const ivec2 & minSize, const ivec2 & maxSize) :
    Single(minSize, maxSize),
    m_graph(graph),
    m_cursorText(new Text("", ivec2(1, 1), vec3(1.0f), ivec2(), ivec2())),
    m_isFocusUpdateNeeded(false)
{
    m_cursorText->size(ivec2(100, 100));
    m_cursorText->pack();
}

void Graph::PlotComp::update() {
    if (m_isFocusUpdateNeeded) {
        updateFocus();
        m_isFocusUpdateNeeded = false;
    }
}

void Graph::PlotComp::render() const {
    glViewport(m_position.x, m_position.y, m_size.x, m_size.y);

    vec2 viewSize(m_graph.m_viewMax - m_graph.m_viewMin);
    vec2 viewCenter((m_graph.m_viewMin + m_graph.m_viewMax) * 0.5f);
    vec2 viewToScreen(2.0f / viewSize);

    s_linesProg->bind();
    glBindVertexArray(s_lineVAO);

    // Graph Lines

    vec2 gridDelta(m_graph.m_gridSize * viewToScreen);
    ivec2 grideLineCounts(ivec2(2.0f / gridDelta) + 1);
    vec2 gridLinesOrigin(1.0f - glm::fract(m_graph.m_viewMin / m_graph.m_gridSize));
    gridLinesOrigin = -1.0f + gridLinesOrigin * gridDelta;

    glUniform3f(s_linesProg->getUniform("u_color"), k_gridColor.r, k_gridColor.g, k_gridColor.b);

    // Vertical
    glUniform2f(s_linesProg->getUniform("u_origin"), gridLinesOrigin.x, 0.0f);
    glUniform2f(s_linesProg->getUniform("u_orient"), 0.0f, 1.0f);
    glUniform2f(s_linesProg->getUniform("u_delta"), gridDelta.x, 0.0f);
    glDrawArraysInstanced(GL_LINES, 0, 2, grideLineCounts.x);
    // Horizontal
    glUniform2f(s_linesProg->getUniform("u_origin"), 0.0f, gridLinesOrigin.y);
    glUniform2f(s_linesProg->getUniform("u_orient"), 1.0f, 0.0f);
    glUniform2f(s_linesProg->getUniform("u_delta"), 0.0f, gridDelta.y);
    glDrawArraysInstanced(GL_LINES, 0, 2, grideLineCounts.y);

    // Origin lines

    vec2 gridOrigin(-viewCenter * viewToScreen);

    glUniform3f(s_linesProg->getUniform("u_color"), k_originColor.r, k_originColor.g, k_originColor.b);

    // Vertical
    glUniform2f(s_linesProg->getUniform("u_origin"), gridOrigin.x, 0.0f);
    glUniform2f(s_linesProg->getUniform("u_orient"), 0.0f, 1.0f);
    glUniform2f(s_linesProg->getUniform("u_delta"), 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, 2);
    // Horizontal
    glUniform2f(s_linesProg->getUniform("u_origin"), 0.0f, gridOrigin.y);
    glUniform2f(s_linesProg->getUniform("u_orient"), 1.0f, 0.0f);
    glUniform2f(s_linesProg->getUniform("u_delta"), 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, 2);

    // Focus
    
    if (m_graph.m_isFocusX) {
        float focusOrigin((m_graph.m_focusX - viewCenter.x) * viewToScreen.x);

        glUniform3f(s_linesProg->getUniform("u_color"), k_focusColor.r, k_focusColor.g, k_focusColor.b);
        // Vertical
        glUniform2f(s_linesProg->getUniform("u_origin"), focusOrigin, 0.0f);
        glUniform2f(s_linesProg->getUniform("u_orient"), 0.0f, 1.0f);
        glUniform2f(s_linesProg->getUniform("u_delta"), 0.0f, 0.0f);
        glDrawArrays(GL_LINES, 0, 2);
    }

    // Render curves

    s_curveProg->bind();
    glUniform2f(s_curveProg->getUniform("u_viewMin"), m_graph.m_viewMin.x, m_graph.m_viewMin.y);
    glUniform2f(s_curveProg->getUniform("u_viewMax"), m_graph.m_viewMax.x, m_graph.m_viewMax.y);

    for (const Curve & curve : m_graph.m_curves) {
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

        glUniform3f(s_curveProg->getUniform("u_color"), curve.color.r, curve.color.g, curve.color.b);
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

void Graph::PlotComp::cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) {
    if (UI::isMouseButtonPressed(0)) {
        vec2 factor((m_graph.m_viewMax - m_graph.m_viewMin) / vec2(m_size));
        m_graph.moveView(-factor * vec2(delta));
    }

    float x(float(pos.x - m_position.x));
    x /= m_size.x;
    x *= m_graph.m_viewMax.x - m_graph.m_viewMin.x;
    x += m_graph.m_viewMin.x;
    m_graph.focusX(x);

    m_isFocusUpdateNeeded = true;
}

void Graph::PlotComp::cursorEnterEvent() {
    UI::setTooltip(m_cursorText);
}

void Graph::PlotComp::cursorExitEvent() {
    m_graph.unfocusX();
    UI::setTooltip(nullptr);
}

void Graph::PlotComp::scrollEvent(const ivec2 & delta) {
    if (delta.y < 0) m_graph.zoomView(k_zoomFactor);
    else if (delta.y > 0) m_graph.zoomView(k_invZoomFactor);
}

void Graph::PlotComp::updateFocus() {
    float x(m_graph.focusX());

    std::stringstream ss;
    ss << m_graph.m_xLabel << ": " << Util::numberString(x, k_gridTextPrecision);
    for (const Curve & curve : m_graph.m_curves) {
        float val(curve.valAt(x));
        if (std::isnan(val)) {
            continue;
        }
        ss << '\n' << curve.label << ": " << Util::numberString(val, k_gridTextPrecision);
    }

    m_cursorText->string(ss.str());
    m_cursorText->size(Text::detSize(Text::detDimensions(m_cursorText->string())));
}



Graph::InnerComp::InnerComp(Graph & graph, const ivec2 & minPlotSize, const ivec2 & maxPlotSize) :
    m_graph(graph),
    m_plotComp(new PlotComp(graph, minPlotSize, maxPlotSize)),
    m_domainMinText(new Text("", ivec2(0, 1), vec3(1.0f), ivec2(k_gridTextLength, 1), ivec2(k_gridTextLength, 1))),
    m_domainMaxText(new Text("", ivec2(0, 1), vec3(1.0f), ivec2(k_gridTextLength, 1), ivec2(k_gridTextLength, 1))),
    m_rangeMinText(new Text("", ivec2(-1, 0), vec3(1.0f), ivec2(k_gridTextLength, 1), ivec2(k_gridTextLength, 1))),
    m_rangeMaxText(new Text("", ivec2(-1, 0), vec3(1.0f), ivec2(k_gridTextLength, 1), ivec2(k_gridTextLength, 1))),
    m_isGridTextUpdateNeeded(true)
{
    add(m_plotComp);
    add(m_domainMinText);
    add(m_domainMaxText);
    add(m_rangeMinText);
    add(m_rangeMaxText);
}

void Graph::InnerComp::update() {
    Group::update();

    if (m_isGridTextUpdateNeeded) {
        updateGridText();
        m_isGridTextUpdateNeeded = false;
    }
}

void Graph::InnerComp::pack() {
    m_minMargin.x = m_rangeMinText->minSize().x;
    m_minMargin.y = m_domainMinText->minSize().y;

    m_maxMargin = m_minMargin / 2;

    m_domainMinText->size(m_domainMinText->minSize());
    m_domainMaxText->size(m_domainMaxText->minSize());
    m_rangeMinText->size(m_rangeMinText->minSize());
    m_rangeMaxText->size(m_rangeMaxText->minSize());

    m_plotComp->position(m_position + m_minMargin);
    m_plotComp->size(m_size - m_minMargin - m_maxMargin);

    packComponents();

    m_isGridTextUpdateNeeded = true;
}

void Graph::InnerComp::detSizeExtrema() const {
    m_minSize.x = m_plotComp->minSize().x + m_rangeMinText->minSize().x + m_domainMaxText->minSize().x / 2;
    m_minSize.y = m_plotComp->minSize().y + m_domainMinText->minSize().y + m_rangeMaxText->minSize().y / 2;
    m_maxSize.x = m_plotComp->maxSize().x ? m_plotComp->maxSize().x + m_rangeMinText->maxSize().x + m_domainMaxText->maxSize().x / 2 : 0;
    m_maxSize.y = m_plotComp->maxSize().y ? m_plotComp->maxSize().y + m_domainMinText->maxSize().y + m_rangeMaxText->maxSize().y / 2 : 0;
}

void Graph::InnerComp::updateGridText() {
    vec2 gridMin(glm::ceil(m_graph.viewMin() / m_graph.gridSize()) * m_graph.gridSize());
    vec2 gridMax(glm::floor(m_graph.viewMax() / m_graph.gridSize()) * m_graph.gridSize());
    vec2 invViewSize(1.0f / (m_graph.viewMax() - m_graph.viewMin()));
    vec2 plotSize(m_plotComp->size());
    ivec2 gridMinPos(glm::round((gridMin - m_graph.viewMin()) * invViewSize * plotSize));
    ivec2 gridMaxPos(glm::round((gridMax - m_graph.viewMin()) * invViewSize * plotSize));
    
    ivec2 offset(m_minMargin / 2);

    m_domainMinText->position(m_position + ivec2(offset.x + gridMinPos.x, 0));
    m_domainMinText->string(Util::numberString(gridMin.x, k_gridTextPrecision));
    m_domainMaxText->position(m_position + ivec2(offset.x + gridMaxPos.x, 0));
    m_domainMaxText->string(Util::numberString(gridMax.x, k_gridTextPrecision));

    m_rangeMinText->position(m_position + ivec2(0, offset.y + gridMinPos.y));
    m_rangeMinText->string(Util::numberString(gridMin.y, k_gridTextPrecision));
    m_rangeMaxText->position(m_position + ivec2(0, offset.y + gridMaxPos.y));
    m_rangeMaxText->string(Util::numberString(gridMax.y, k_gridTextPrecision));
}



Graph::Graph(const std::string & title, const std::string & xLabel, const vec2 & viewMin, const vec2 & viewMax, const bvec2 & isZoomAllowed, const ivec2 & minPlotSize, const ivec2 & maxPlotSize) :
    m_xLabel(xLabel),
    m_viewMin(viewMin), m_viewMax(viewMax),
    m_gridSize(detGridSize(m_viewMax.x - m_viewMin.x), detGridSize(m_viewMax.y - m_viewMin.y)),
    m_isFocusX(false),
    m_focusX(),
    m_isZoomAllowed(isZoomAllowed),
    m_innerComp(new InnerComp(*this, minPlotSize, maxPlotSize))
{
    add(m_innerComp);

    if (!title.empty()) {
        int lineCount(Text::detDimensions(title).y);
        shr<Text> titleComp(new Text(title, ivec2(), vec3(1.0f), ivec2(int(title.size()), lineCount), ivec2(0, lineCount)));
        add(titleComp);
    }
}

void Graph::addCurve(const std::string & xLabel, const vec3 & color, int maxNPoints) {
    m_curves.emplace_back(xLabel, color, maxNPoints);
}

const std::vector<vec2> & Graph::accessPoints(int curveI) const {
    return m_curves[curveI].points;
}

std::vector<vec2> & Graph::mutatePoints(int curveI) {
    m_curves[curveI].isChange = true;
    m_innerComp->m_plotComp->m_isFocusUpdateNeeded = true;
    return m_curves[curveI].points;
}

void Graph::setView(const vec2 & min, const vec2 & max) {
    m_viewMin = min;
    m_viewMax = max;
    m_gridSize.x = detGridSize(m_viewMax.x - m_viewMin.x);
    m_gridSize.y = detGridSize(m_viewMax.y - m_viewMin.y);

    unfocusX();

    m_innerComp->m_isGridTextUpdateNeeded = true;
}

void Graph::zoomView(float factor) {
    if (!glm::any(m_isZoomAllowed)) {
        return;
    }

    vec2 viewSize(m_viewMax - m_viewMin);
    vec2 viewSizeDelta(viewSize * (factor - 1.0f));
    if (!m_isZoomAllowed.x) viewSizeDelta.x = 0.0f;
    if (!m_isZoomAllowed.y) viewSizeDelta.y = 0.0f;
    setView(m_viewMin - viewSizeDelta * 0.5f, m_viewMax + viewSizeDelta * 0.5f);
}

void Graph::moveView(const vec2 & delta) {
    setView(m_viewMin + delta, m_viewMax + delta);
}

void Graph::focusX(float x) {
    m_focusX = x;
    m_isFocusX = true;
    m_innerComp->m_plotComp->m_isFocusUpdateNeeded = true;
}

void Graph::unfocusX() {
    m_isFocusX = false;
    m_innerComp->m_plotComp->m_isFocusUpdateNeeded = true;
}

void Graph::cleanup() {
    for (Curve & curve : m_curves) curve.cleanup();
}