#pragma once



#include <vector>
#include <memory>

#include "glm/glm.hpp"



class Graph {

    public:

    static bool setup(const std::string & resourceDir);

    private:

    std::vector<glm::vec2> m_points;
    int m_maxNPoints;
    glm::vec2 m_viewMin, m_viewMax;
    glm::vec3 m_color;
    glm::vec2 m_focusPoint;
    bool m_isFocusPoint;
    glm::vec2 m_gridSize;
    unsigned int m_curveVAO, m_curveVBO;
    bool m_isPointChange;

    public:

    Graph(const glm::vec2 & viewMin, const glm::vec2 & viewMax, const glm::vec3 & color, int maxNPoints);

    void render(const glm::ivec2 & viewportSize);

    const std::vector<glm::vec2> & accessPoints() const;

    std::vector<glm::vec2> & mutatePoints();

    const glm::vec2 & viewMin() const { return m_viewMin; }
    const glm::vec2 & viewMax() const { return m_viewMax; }

    void setView(const glm::vec2 & min, const glm::vec2 & max);

    void zoomView(float factor);

    void moveView(const glm::vec2 & delta);

    void focusPoint(const glm::vec2 & point);
    const glm::vec2 & focusPoint() const { return m_focusPoint; }

    void removeFocusPoint();

    const glm::vec2 & gridSize() const { return m_gridSize; }

    void cleanup();

    private:

    bool prepare();
    
    void upload();

};