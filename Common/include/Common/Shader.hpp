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

    void addAttribute(const std::string & name);
    void addUniform(const std::string & name);
    s32 getAttribute(const std::string & name) const;
    s32 getUniform(const std::string & name) const;

    private:

    Shader(u32 glId);

    u32 m_glId;
    std::unordered_map<std::string, s32> m_attributes;
    std::unordered_map<std::string, s32> m_uniforms;

};
