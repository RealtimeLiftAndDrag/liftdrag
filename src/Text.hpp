#pragma once



#include <string>
#include <vector>

#include "Global.hpp"
#include "UI.hpp"



class Text : public UI::Single {

    public:

    static bool setup(const std::string & resourceDir);

    static const ivec2 & fontSize();

    private:

    std::string m_string;
    ivec2 m_align;
    vec3 m_color;

    mutable bool m_isChange;
    mutable unsigned int m_vao, m_charVBO;
    mutable std::vector<vec2> m_charData;
    mutable std::vector<int> m_lineEndIndices;

    public:

    Text(const std::string & string, const ivec2 & align, const vec3 & color, int minLineSize, int maxLineSize, int maxLineCount);

    virtual void render(const ivec2 & position) const override;

    virtual void pack(const ivec2 & size) override;

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