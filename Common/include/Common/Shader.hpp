#pragma once

#include <unordered_map>
#include <string>

#include "Global.hpp"



class Shader {

    public:

    static unq<Shader> load(
        const std::string & vertFile,
        const std::string & fragFile
    );
    static unq<Shader> load(
        const std::string & vertFile,
        const std::string & geomFile,
        const std::string & fragFile
    );
    static unq<Shader> load(
        const std::string & vertFile,
        const std::string & tescFile,
        const std::string & teseFile,
        const std::string & fragFile
    );
    static unq<Shader> load(
        const std::string & vertFile,
        const std::string & tescFile,
        const std::string & teseFile,
        const std::string & geomFile,
        const std::string & fragFile
    );
    static unq<Shader> load(
        const std::string & compFile
    );

    u32 glId() const { return m_glId; }

    void bind();
    void unbind();

    template <typename T> bool uniform(const std::string & name, const T & val);
    template <typename T> bool uniform(s32 location, const T & val);

    s32 uniformLocation(const std::string & name);

    private:

    Shader(u32 glId);

    u32 m_glId;
    std::unordered_map<std::string, s32> m_uniforms;

};
