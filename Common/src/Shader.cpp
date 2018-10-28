#include "Shader.hpp"

#include <iostream>

#include "glad/glad.h"
#include "GLSL.h"

#include "Util.hpp"



static bool compile(uint id, const std::string & src) {
    const char * csrc(src.c_str());
    glShaderSource(id, 1, &csrc, nullptr);
    glCompileShader(id);

    s32 status(0);
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    if (!status) {
        s32 errLen(0);
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &errLen);

        if (errLen) {
            unq<char[]> log(new char[errLen]);
            glGetShaderInfoLog(id, errLen, nullptr, log.get());
            std::cerr << log.get();
        }

        glGetError(); // clear error
        return false;
    }

    return !glGetError();
}

static bool link(uint progId, uint vertId, uint tescId, uint teseId, uint geomId, uint fragId, uint compId) {
    if (vertId) glAttachShader(progId, vertId);
    if (tescId) glAttachShader(progId, tescId);
    if (teseId) glAttachShader(progId, teseId);
    if (geomId) glAttachShader(progId, geomId);
    if (fragId) glAttachShader(progId, fragId);
    if (compId) glAttachShader(progId, compId);
    glLinkProgram(progId);

    s32 status(0);
    glGetProgramiv(progId, GL_LINK_STATUS, &status);
    if (!status) {
        int errLen(0);
        glGetProgramiv(progId, GL_INFO_LOG_LENGTH, &errLen);

        if (errLen) {
            unq<char[]> log(new char[errLen]);
            glGetProgramInfoLog(progId, errLen, nullptr, log.get());
            std::cerr << log.get();
        }
        
        glGetError(); // clear error
        return false;
    }
    
    if (vertId) glDetachShader(progId, vertId);
    if (tescId) glDetachShader(progId, tescId);
    if (teseId) glDetachShader(progId, teseId);
    if (geomId) glDetachShader(progId, geomId);
    if (fragId) glDetachShader(progId, fragId);
    if (compId) glDetachShader(progId, compId);

    return !glGetError();
}

static u32 createProgram(
    const std::string & vertSrc,
    const std::string & tescSrc,
    const std::string & teseSrc,
    const std::string & geomSrc,
    const std::string & fragSrc,
    const std::string & compSrc
) {
    u32 vertId(0), tescId(0), teseId(0), geomId(0), fragId(0), compId(0);
    if (vertSrc.size()) {
        if (!(vertId = glCreateShader(GL_VERTEX_SHADER))) {
            std::cerr << "Failed to create vertex shader" << std::endl;
            return 0;
        }
        if (!compile(vertId, vertSrc)) {
            std::cerr << "Failed to compile vertex shader" << std::endl;
            return 0;
        }
    }
    if (tescSrc.size()) {
        if (!(tescId = glCreateShader(GL_TESS_CONTROL_SHADER))) {
            std::cerr << "Failed to create tessellation control shader" << std::endl;
            return 0;
        }
        if (!compile(tescId, tescSrc)) {
            std::cerr << "Failed to compile tessellation control shader" << std::endl;
            return 0;
        }
    }
    if (teseSrc.size()) {
        if (!(teseId = glCreateShader(GL_TESS_EVALUATION_SHADER))) {
            std::cerr << "Failed to create tessellation evaluation shader" << std::endl;
            return 0;
        }
        if (!compile(teseId, teseSrc)) {
            std::cerr << "Failed to compile tessellation evaluation shader" << std::endl;
            return 0;
        }
    }
    if (geomSrc.size()) {
        if (!(geomId = glCreateShader(GL_GEOMETRY_SHADER))) {
            std::cerr << "Failed to create geometry shader" << std::endl;
            return 0;
        }
        if (!compile(geomId, geomSrc)) {
            std::cerr << "Failed to compile geometry shader" << std::endl;
            return 0;
        }
    }
    if (fragSrc.size()) {
        if (!(fragId = glCreateShader(GL_FRAGMENT_SHADER))) {
            std::cerr << "Failed to create fragment shader" << std::endl;
            return 0;
        }
        if (!compile(fragId, fragSrc)) {
            std::cerr << "Failed to compile fragment shader" << std::endl;
            return 0;
        }
    }
    if (compSrc.size()) {
        if (!(compId = glCreateShader(GL_COMPUTE_SHADER))) {
            std::cerr << "Failed to create compute shader" << std::endl;
            return 0;
        }
        if (!compile(compId, compSrc)) {
            std::cerr << "Failed to compile compute shader" << std::endl;
            return 0;
        }
    }

    u32 progId(glCreateProgram());
    if (!progId) {
        std::cerr << "Failed to create program" << std::endl;
        return 0;
    }
    if (!link(progId, vertId, tescId, teseId, geomId, fragId, compId)) {
        std::cerr << "Failed to link shaders" << std::endl;
        return 0;
    }

    if (vertId) glDeleteShader(vertId);
    if (tescId) glDeleteShader(tescId);
    if (teseId) glDeleteShader(teseId);
    if (geomId) glDeleteShader(geomId);
    if (fragId) glDeleteShader(fragId);
    if (compId) glDeleteShader(compId);
    
    if (glGetError()) {
        return 0;
    }

    return progId;
}

static u32 loadProgram(
    const std::string & vertFile,
    const std::string & tescFile,
    const std::string & teseFile,
    const std::string & geomFile,
    const std::string & fragFile,
    const std::string & compFile
) {
    std::string vertSrc, tescSrc, teseSrc, geomSrc, fragSrc, compSrc;
    if (vertFile.size()) {
        if (!Util::readTextFile(vertFile, vertSrc)) {
            std::cerr << "Failed to read vertex shader file: " << vertFile << std::endl;
            return 0;
        }
    }
    if (tescFile.size()) {
        if (!Util::readTextFile(tescFile, tescSrc)) {
            std::cerr << "Failed to read tessellation control shader file: " << tescFile << std::endl;
            return 0;
        }
    }
    if (teseFile.size()) {
        if (!Util::readTextFile(teseFile, teseSrc)) {
            std::cerr << "Failed to read tessellation evaluation shader file: " << teseFile << std::endl;
            return 0;
        }
    }
    if (geomFile.size()) {
        if (!Util::readTextFile(geomFile, geomSrc)) {
            std::cerr << "Failed to read geometry shader file: " << geomFile << std::endl;
            return 0;
        }
    }
    if (fragFile.size()) {
        if (!Util::readTextFile(fragFile, fragSrc)) {
            std::cerr << "Failed to read fragment shader file: " << fragFile << std::endl;
            return 0;
        }
    }
    if (compFile.size()) {
        if (!Util::readTextFile(compFile, compSrc)) {
            std::cerr << "Failed to read compute shader file: " << compFile << std::endl;
            return 0;
        }
    }

    return createProgram(vertSrc, tescSrc, teseSrc, geomSrc, fragSrc, compSrc);
}

unq<Shader> Shader::load(
    const std::string & vertFile,
    const std::string & fragFile
) {
    return load(vertFile, "", "", "", fragFile);
}

unq<Shader> Shader::load(
    const std::string & vertFile,
    const std::string & geomFile,
    const std::string & fragFile
) {
    return load(vertFile, "", "", geomFile, fragFile);
}

unq<Shader> Shader::load(
    const std::string & vertFile,
    const std::string & tescFile,
    const std::string & teseFile,
    const std::string & fragFile
) {
    return load(vertFile, tescFile, teseFile, "", fragFile);
}

unq<Shader> Shader::load(
    const std::string & vertFile,
    const std::string & tescFile,
    const std::string & teseFile,
    const std::string & geomFile,
    const std::string & fragFile
) {
    if (vertFile.empty()) {
        std::cerr << "Missing vertex shader" << std::endl;
        return nullptr;
    }
    if (fragFile.empty()) {
        std::cerr << "Missing fragment shader" << std::endl;
        return nullptr;
    }
    if (tescFile.size() || teseFile.size()) {
        if (tescFile.empty()) {
            std::cerr << "Missing tessellation control shader" << std::endl;
            return nullptr;
        }
        if (teseFile.empty()) {
            std::cerr << "Missing tessellation evaluation shader" << std::endl;
            return nullptr;
        }
    }

    u32 progId(loadProgram(vertFile, tescFile, teseFile, geomFile, fragFile, ""));
    if (!progId) {
        return nullptr;
    }

    return unq<Shader>(new Shader(progId));
}

unq<Shader> Shader::load(const std::string & compFile) {
    if (compFile.empty()) {
        std::cerr << "Missing compute shader" << std::endl;
        return nullptr;
    }

    u32 progId(loadProgram("", "", "", "", "", compFile));
    if (!progId) {
        return nullptr;
    }

    return unq<Shader>(new Shader(progId));
}

void Shader::bind() {
    glUseProgram(m_glId);
}

void Shader::unbind() {
    glUseProgram(0);
}

void Shader::addAttribute(const std::string & name) {
    m_attributes[name] = GLSL::getAttribLocation(m_glId, name.c_str());
}

void Shader::addUniform(const std::string & name) {
    m_uniforms[name] = GLSL::getUniformLocation(m_glId, name.c_str());
}

GLint Shader::getAttribute(const std::string & name) const {
    auto it(m_attributes.find(name.c_str()));
    return it == m_attributes.end() ? -1 : it->second;
}

GLint Shader::getUniform(const std::string & name) const {
    auto it(m_uniforms.find(name.c_str()));
    return it == m_uniforms.end() ? -1 : it->second;
}

Shader::Shader(u32 glId) :
    m_glId(glId),
    m_attributes(),
    m_uniforms()
{}
