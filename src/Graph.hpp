#pragma once



#include <vector>
#include <memory>

#include "glm/glm.hpp"



class Graph {

    private:

    struct Curve {

        std::vector<glm::vec2> points;
        int maxNPoints;
        glm::vec3 color;
        unsigned int vao, vbo;
        bool isSetup, isChange;

        Curve(const glm::vec3 & color, int maxNPoints);

        bool setup();

        void upload();

        void cleanup();

    };

    public:

    static bool setup(const std::string & resourceDir);

    private:

    std::vector<Curve> m_curves;
    glm::vec2 m_viewMin, m_viewMax;
    glm::vec2 m_gridSize;
    bool m_isFocusX;
    float m_focusX;

    public:

    Graph(const glm::vec2 & viewMin, const glm::vec2 & viewMax);

    void addCurve(const glm::vec3 & color, int maxNPoints);

    void render(const glm::ivec2 & viewportSize);

    const std::vector<glm::vec2> & accessPoints(int curveI) const;

    std::vector<glm::vec2> & mutatePoints(int curveI);

    const glm::vec2 & viewMin() const { return m_viewMin; }
    const glm::vec2 & viewMax() const { return m_viewMax; }

    void setView(const glm::vec2 & min, const glm::vec2 & max);

    void zoomView(float factor);

    void moveView(const glm::vec2 & delta);

    void focusX(float x);
    float focusX() const { return m_focusX; }

    void removeFocusX();

    const glm::vec2 & gridSize() const { return m_gridSize; }

    void cleanup();

};