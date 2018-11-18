#pragma once



#include "Global.hpp"



class SkyBox {

    public:
    
    // Six files in following order: +x, -x, +y, -y, +z, -z
    static unq<SkyBox> create(std::initializer_list<std::string_view> files);

    void render(const mat4 & viewMat, const mat4 & projMat) const;

    private:

    SkyBox(u32 cubemap);

    u32 m_cubemap;

};