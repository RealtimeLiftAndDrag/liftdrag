#pragma once



#include "UI.hpp"



class TexViewer : public UI::Single {    

    public:

    static bool setup(const std::string & resourceDir);

    private:

    uint m_tex;
    ivec2 m_texSize;

    public:

    TexViewer(uint tex, const ivec2 & texSize, const ivec2 & minSize);

    virtual void render(const ivec2 & position) const override;

    virtual void pack(const ivec2 & size) override;

    void cleanup();

};