#pragma once



#include <string>
#include <vector>

#include "Global.hpp"
#include "UI.hpp"



class Text : public UI::Single {

    public:

    static bool setup(const std::string & resourceDir);

    static const ivec2 & fontSize();

    // x is in characters, y is in lines
    static ivec2 detDimensions(const std::string & str);

    // dimensions in string space, returns pixel space
    static ivec2 detSize(const ivec2 & dimensions);

    private:

    std::string m_string;
    ivec2 m_align;
    vec3 m_color;

    mutable bool m_isChange;
    mutable unsigned int m_vao, m_charVBO;
    mutable std::vector<vec2> m_charData;
    mutable std::vector<int> m_lineEndIndices;

    public:

    Text(const std::string & string, const ivec2 & align, const vec3 & color, const ivec2 & minDimensions, const ivec2 & maxDimensions);

    virtual void render() const override;

    virtual void pack() override;

    void cleanup();

    void string(const std::string & string);
    const std::string & string() const { return m_string; }

    const ivec2 & align() const { return m_align; }

    void color(const vec3 & color);
    const vec3 & color() const { return m_color; }

    private:

    bool prepare() const;

    void detCharData() const;

    void upload() const;

};