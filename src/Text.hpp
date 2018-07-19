#pragma once



#include <string>

#include "glm/glm.hpp"



class Text {

    public:

    static bool setup(const std::string & resourceDir);

    private:

    std::string m_string;
    glm::ivec2 m_position;
    glm::vec3 m_color;
    mutable bool m_isStringChange, m_isPositionChange, m_isColorChange;
    mutable unsigned int m_vao, m_charVBO;

    public:

    Text(const std::string & string, const glm::ivec2 & position, const glm::vec3 & color);

    void string(const std::string & string);
    const std::string & string() const { return m_string; }

    void position(const glm::ivec2 & position);
    const glm::ivec2 & position() const { return m_position; }

    void color(const glm::vec3 & color);
    const glm::vec3 & color() const { return m_color; }

    void render() const;

    private:

    bool prepare() const;
    void upload() const;

};