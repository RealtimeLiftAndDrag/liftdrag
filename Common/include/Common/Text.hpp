#pragma once



#include <vector>

#include "Global.hpp"



class Text {

    public:

    static bool setup(const std::string & resourceDir);

    static const ivec2 & fontSize();

    // x is in characters, y is in lines
    static ivec2 detDimensions(const std::string & str);

    // dimensions in string space, returns pixel space
    static ivec2 detSize(const ivec2 & dimensions);
    static ivec2 detSize(const std::string & str);

    private:

    std::string m_string;
    ivec2 m_align;
    vec4 m_color;
    std::vector<ivec2> m_lineSpecs; // starting index and length
    ivec2 m_dimensions; // width and height of string in characters
    ivec2 m_size;
    ivec2 m_basepoint; // basepoint within bounding rectangle

    mutable unsigned int m_vao = 0, m_charVBO = 0;
    mutable bool m_isChange = false;

    public:

    Text();
    Text(const std::string & string, const ivec2 & align, const vec4 & color);

    void render(const ivec2 & position) const;
    void renderAt(const ivec2 & basepoint) const;
    void renderWithin(const ivec2 & boundsPos, const ivec2 & boundsSize) const;

    std::string string(const std::string & string);
    std::string string(std::string && string);
    const std::string & string() const { return m_string; }

    void align(const ivec2 & align);
    const ivec2 & align() const { return m_align; }

    void color(const vec4 & color);
    const vec4 & color() const { return m_color; }

    const ivec2 & dimensions() const { return m_dimensions; }

    const ivec2 & size() const { return m_size; }

    private:

    bool prepare() const;

    void detLineSpecs();

    void detDimensions();

    void detSize();

    void detBasepoint();

    void upload() const;

};