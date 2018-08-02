#pragma once



#include <string>
#include <vector>

#include "glm/glm.hpp"



class Text {

    public:

    static bool setup(const std::string & resourceDir);

    private:

    std::string m_string;
    glm::ivec2 m_position;
    glm::ivec2 m_align;
    glm::vec3 m_color;
    bool m_isStringChange, m_isPositionChange;
    unsigned int m_vao, m_charVBO;
    std::vector<glm::vec2> m_charData;
    std::vector<int> m_lineEndIndices;

    public:

    Text(const std::string & string, const glm::ivec2 & position, const glm::ivec2 & align,const glm::vec3 & color);

    void string(const std::string & string);
    const std::string & string() const { return m_string; }

    void position(const glm::ivec2 & position);
    const glm::ivec2 & position() const { return m_position; }

    void color(const glm::vec3 & color);
    const glm::vec3 & color() const { return m_color; }

    void render(const glm::ivec2 & viewportSize);

    void cleanup();

    private:

    bool prepare();

    void detCharData();

    void upload();

};