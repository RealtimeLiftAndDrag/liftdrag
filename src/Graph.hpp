#pragma once



#include <vector>
#include <memory>

#include "glm/glm.hpp"



class Graph {

    private:

    std::vector<glm::vec2> m_points;
    int m_maxNPoints;
    glm::vec2 m_min, m_max;
    unsigned int m_curveVAO, m_curveVBO;

    public:

    Graph(int maxNPoints);

    bool setup(const std::string & resourcesDir);

    void cleanup();

    void render() const;

    std::vector<glm::vec2> & accessPoints();

    void refresh();

};