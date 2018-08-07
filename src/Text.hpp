#pragma once



#include <string>
#include <vector>

#include "Global.hpp"



class Text {

    public:

    static bool setup(const std::string & resourceDir);

    private:

    std::string m_string;
    ivec2 m_position;
    ivec2 m_align;
    vec3 m_color;
    bool m_isStringChange, m_isPositionChange;
    unsigned int m_vao, m_charVBO;
    std::vector<vec2> m_charData;
    std::vector<int> m_lineEndIndices;

    public:

    Text(const std::string & string, const ivec2 & position, const ivec2 & align,const vec3 & color);

    void string(const std::string & string);
    const std::string & string() const { return m_string; }

    void position(const ivec2 & position);
    const ivec2 & position() const { return m_position; }

    void color(const vec3 & color);
    const vec3 & color() const { return m_color; }

    void render(const ivec2 & viewportSize);

    void cleanup();

    private:

    bool prepare();

    void detCharData();

    void upload();

};