#include "Graph.hpp"

#include <iostream>
#include <sstream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "Common/GLSL.h"
#include "Common/Util.hpp"
#include "Common/Shader.hpp"



namespace ui {

    static const std::string & k_curveVertFilename("graph_curve.vert"), k_curveFragFilename("graph_curve.frag");
    static const std::string & k_linesVertFilename("graph_lines.vert"), k_linesFragFilename("graph_lines.frag");

    static constexpr float k_zoomFactor(1.1f);
    static constexpr float k_invZoomFactor(1.0f / k_zoomFactor);

    static constexpr int k_gridTextPrecision(3);
    static constexpr int k_gridTextLength(10);

    static const vec3 k_gridColor(0.15f);
    static const vec3 k_originColor(0.3f);
    static const vec3 k_focusColor(0.5f);

    static unq<Shader> s_curveProg, s_linesProg;
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



    bool Graph::setup() {
        const std::string & shadersPath(g_resourcesDir + "/Interface/shaders/");

        // Setup curve shader
        if (!(s_curveProg = Shader::load(shadersPath + k_curveVertFilename, shadersPath + k_curveFragFilename))) {
            std::cerr << "Failed to load curve program" << std::endl;
            return false;
        }

        // Setup lines shader
        if (!(s_linesProg = Shader::load(shadersPath + k_linesVertFilename, shadersPath + k_linesFragFilename))) {
            std::cerr << "Failed to load lines program" << std::endl;
            return false;
        }

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
        m_cursorText(new ui::Text("", ivec2(1, 1), vec4(1.0f), ivec2(), ivec2())),
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
        setViewport();

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

        s_linesProg->uniform("u_color", k_gridColor);

        // Vertical
        s_linesProg->uniform("u_origin", vec2(gridLinesOrigin.x, 0.0f));
        s_linesProg->uniform("u_orient", vec2(0.0f, 1.0f));
        s_linesProg->uniform("u_delta", vec2(gridDelta.x, 0.0f));
        glDrawArraysInstanced(GL_LINES, 0, 2, grideLineCounts.x);
        // Horizontal
        s_linesProg->uniform("u_origin", vec2(0.0f, gridLinesOrigin.y));
        s_linesProg->uniform("u_orient", vec2(1.0f, 0.0f));
        s_linesProg->uniform("u_delta", vec2(0.0f, gridDelta.y));
        glDrawArraysInstanced(GL_LINES, 0, 2, grideLineCounts.y);

        // Origin lines

        vec2 gridOrigin(-viewCenter * viewToScreen);

        s_linesProg->uniform("u_color", k_originColor);

        // Vertical
        s_linesProg->uniform("u_origin", vec2(gridOrigin.x, 0.0f));
        s_linesProg->uniform("u_orient", vec2(0.0f, 1.0f));
        s_linesProg->uniform("u_delta", vec2(0.0f, 0.0f));
        glDrawArrays(GL_LINES, 0, 2);
        // Horizontal
        s_linesProg->uniform("u_origin", vec2(0.0f, gridOrigin.y));
        s_linesProg->uniform("u_orient", vec2(1.0f, 0.0f));
        s_linesProg->uniform("u_delta", vec2(0.0f, 0.0f));
        glDrawArrays(GL_LINES, 0, 2);

        // Focus

        if (m_graph.m_isFocusX) {
            float focusOrigin((m_graph.m_focusX - viewCenter.x) * viewToScreen.x);

            s_linesProg->uniform("u_color", k_focusColor);
            // Vertical
            s_linesProg->uniform("u_origin", vec2(focusOrigin, 0.0f));
            s_linesProg->uniform("u_orient", vec2(0.0f, 1.0f));
            s_linesProg->uniform("u_delta", vec2(0.0f, 0.0f));
            glDrawArrays(GL_LINES, 0, 2);
        }

        // Render curves

        s_curveProg->bind();
        s_curveProg->uniform("u_viewMin", m_graph.m_viewMin);
        s_curveProg->uniform("u_viewMax", m_graph.m_viewMax);

        // Draw in reverse order
        for (auto rit (m_graph.m_curves.crbegin()); rit != m_graph.m_curves.crend(); ++rit) {
            const Curve & curve(*rit);

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

            s_curveProg->uniform("u_color", curve.color);
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
        if (ui::isMouseButtonPressed(0)) {
            vec2 factor((m_graph.m_viewMax - m_graph.m_viewMin) / vec2(size()));
            m_graph.moveView(-factor * vec2(
                ui::isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 0.0f : delta.x,
                ui::isKeyPressed(GLFW_KEY_LEFT_CONTROL) ? 0.0f : delta.y
            ));
        }

        float x(float(pos.x - position().x));
        x /= size().x;
        x *= m_graph.m_viewMax.x - m_graph.m_viewMin.x;
        x += m_graph.m_viewMin.x;
        m_graph.focusX(x);

        m_isFocusUpdateNeeded = true;
    }

    void Graph::PlotComp::cursorEnterEvent() {
        ui::setTooltip(m_cursorText);
    }

    void Graph::PlotComp::cursorExitEvent() {
        m_graph.unfocusX();
        ui::setTooltip(nullptr);
    }

    void Graph::PlotComp::scrollEvent(const ivec2 & delta) {
        vec2 a0(vec2(ui::cursorPosition() - (position() + size() / 2)) / vec2(size()));
        vec2 prevViewSize(m_graph.m_viewMax - m_graph.m_viewMin);

        vec2 factor(delta.y < 0 ? k_zoomFactor : delta.y > 0 ? k_invZoomFactor : 0.0f);
        if (ui::isKeyPressed(GLFW_KEY_LEFT_SHIFT)) factor.x = 1.0f;
        if (ui::isKeyPressed(GLFW_KEY_LEFT_CONTROL)) factor.y = 1.0f;
        m_graph.zoomView(factor);

        vec2 viewSize(m_graph.m_viewMax - m_graph.m_viewMin);
        vec2 a1(a0 * viewSize / prevViewSize);
        vec2 b(a0 - a1);
        m_graph.moveView(b * prevViewSize);
    }

    void Graph::PlotComp::updateFocus() {
        float x(m_graph.focusX());

        std::stringstream ss;
        ss << m_graph.m_xLabel << ": " << util::numberString(x, false, k_gridTextPrecision);
        for (const Curve & curve : m_graph.m_curves) {
            float val(curve.valAt(x));
            if (std::isnan(val)) {
                continue;
            }
            ss << '\n' << curve.label << ": " << util::numberString(val, false, k_gridTextPrecision);
        }

        m_cursorText->string(ss.str());
        m_cursorText->size(::Text::detSize(::Text::detDimensions(m_cursorText->string())));
    }



    Graph::InnerComp::InnerComp(Graph & graph, const ivec2 & minPlotSize, const ivec2 & maxPlotSize) :
        m_graph(graph),
        m_plotComp(new PlotComp(graph, minPlotSize, maxPlotSize)),
        m_domainMinText(new ui::Number(0.0,  0, vec4(1.0f), k_gridTextLength, k_gridTextLength, false, k_gridTextPrecision)),
        m_domainMaxText(new ui::Number(0.0,  0, vec4(1.0f), k_gridTextLength, k_gridTextLength, false, k_gridTextPrecision)),
        m_rangeMinText (new ui::Number(0.0, -1, vec4(1.0f), k_gridTextLength, k_gridTextLength, false, k_gridTextPrecision)),
        m_rangeMaxText (new ui::Number(0.0, -1, vec4(1.0f), k_gridTextLength, k_gridTextLength, false, k_gridTextPrecision)),
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

        m_plotComp->position(position() + m_minMargin);
        m_plotComp->size(size() - m_minMargin - m_maxMargin);

        packComponents();

        m_isGridTextUpdateNeeded = true;
    }

    duo<ivec2> Graph::InnerComp::detSizeExtrema() const {
        return {
            ivec2(
                m_plotComp->minSize().x + m_rangeMinText->minSize().x + m_domainMaxText->minSize().x / 2,
                m_plotComp->minSize().y + m_domainMinText->minSize().y + m_rangeMaxText->minSize().y / 2
            ),
            ivec2(
                m_plotComp->maxSize().x ? m_plotComp->maxSize().x + m_rangeMinText->maxSize().x + m_domainMaxText->maxSize().x / 2 : 0,
                m_plotComp->maxSize().y ? m_plotComp->maxSize().y + m_domainMinText->maxSize().y + m_rangeMaxText->maxSize().y / 2 : 0
            )
        };
    }

    void Graph::InnerComp::updateGridText() {
        vec2 gridMin(glm::ceil(m_graph.viewMin() / m_graph.gridSize()) * m_graph.gridSize());
        vec2 gridMax(glm::floor(m_graph.viewMax() / m_graph.gridSize()) * m_graph.gridSize());
        vec2 invViewSize(1.0f / (m_graph.viewMax() - m_graph.viewMin()));
        vec2 plotSize(m_plotComp->size());
        ivec2 gridMinPos(glm::round((gridMin - m_graph.viewMin()) * invViewSize * plotSize));
        ivec2 gridMaxPos(glm::round((gridMax - m_graph.viewMin()) * invViewSize * plotSize));

        ivec2 offset(m_minMargin / 2);

        m_domainMinText->position(position() + ivec2(offset.x + gridMinPos.x, 0));
        m_domainMinText->value(gridMin.x);
        m_domainMaxText->position(position() + ivec2(offset.x + gridMaxPos.x, 0));
        m_domainMaxText->value(gridMax.x);

        m_rangeMinText->position(position() + ivec2(0, offset.y + gridMinPos.y));
        m_rangeMinText->value(gridMin.y);
        m_rangeMaxText->position(position() + ivec2(0, offset.y + gridMaxPos.y));
        m_rangeMaxText->value(gridMax.y);
    }



    Graph::Graph(const std::string & title, const std::string & xLabel, const vec2 & viewMin, const vec2 & viewMax, const ivec2 & minPlotSize, const ivec2 & maxPlotSize) :
        m_xLabel(xLabel),
        m_viewMin(viewMin), m_viewMax(viewMax),
        m_gridSize(detGridSize(m_viewMax.x - m_viewMin.x), detGridSize(m_viewMax.y - m_viewMin.y)),
        m_isFocusX(false),
        m_focusX(),
        m_innerComp(new InnerComp(*this, minPlotSize, maxPlotSize))
    {
        add(m_innerComp);

        if (!title.empty()) {
            int lineCount(::Text::detDimensions(title).y);
            shr<ui::Text> titleComp(new ui::Text(title, ivec2(), vec4(1.0f), ivec2(int(title.size()), lineCount), ivec2(0, lineCount)));
            add(titleComp);
        }
    }

    void Graph::addCurve(const std::string & xLabel, const vec3 & color, int maxNPoints) {
        m_curves.emplace_back(xLabel, color, maxNPoints);
    }

    const std::vector<vec2> & Graph::accessPoints(int curveI) const {
        return m_curves[curveI].points;
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

    void Graph::zoomView(const vec2 & factor) {
        vec2 viewSize(m_viewMax - m_viewMin);
        vec2 viewSizeDelta(viewSize * (factor - 1.0f));
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

}