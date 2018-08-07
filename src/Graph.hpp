#pragma once



#include <vector>
#include <memory>

#include "Global.hpp"



class Graph {

    private:

    struct Curve {

        std::vector<vec2> points;
        int maxNPoints;
        vec3 color;
        unsigned int vao, vbo;
        bool isSetup, isChange;

        Curve(const vec3 & color, int maxNPoints);

        bool setup();

        void upload();

        void cleanup();

    };

    public:

    static bool setup(const std::string & resourceDir);

    private:

    std::vector<Curve> m_curves;
    vec2 m_viewMin, m_viewMax;
    vec2 m_gridSize;
    bool m_isFocusX;
    float m_focusX;

    public:

    Graph(const vec2 & viewMin, const vec2 & viewMax);

    void addCurve(const vec3 & color, int maxNPoints);

    void render(const ivec2 & viewportSize);

    const std::vector<vec2> & accessPoints(int curveI) const;

    std::vector<vec2> & mutatePoints(int curveI);

    const vec2 & viewMin() const { return m_viewMin; }
    const vec2 & viewMax() const { return m_viewMax; }

    void setView(const vec2 & min, const vec2 & max);

    void zoomView(float factor);

    void moveView(const vec2 & delta);

    void focusX(float x);
    float focusX() const { return m_focusX; }

    void removeFocusX();

    const vec2 & gridSize() const { return m_gridSize; }

    void cleanup();

};