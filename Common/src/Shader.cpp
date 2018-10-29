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
        if (!util::readTextFile(vertFile, vertSrc)) {
            std::cerr << "Failed to read vertex shader file: " << vertFile << std::endl;
            return 0;
        }
    }
    if (tescFile.size()) {
        if (!util::readTextFile(tescFile, tescSrc)) {
            std::cerr << "Failed to read tessellation control shader file: " << tescFile << std::endl;
            return 0;
        }
    }
    if (teseFile.size()) {
        if (!util::readTextFile(teseFile, teseSrc)) {
            std::cerr << "Failed to read tessellation evaluation shader file: " << teseFile << std::endl;
            return 0;
        }
    }
    if (geomFile.size()) {
        if (!util::readTextFile(geomFile, geomSrc)) {
            std::cerr << "Failed to read geometry shader file: " << geomFile << std::endl;
            return 0;
        }
    }
    if (fragFile.size()) {
        if (!util::readTextFile(fragFile, fragSrc)) {
            std::cerr << "Failed to read fragment shader file: " << fragFile << std::endl;
            return 0;
        }
    }
    if (compFile.size()) {
        if (!util::readTextFile(compFile, compSrc)) {
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

template <typename T>
bool Shader::uniform(const std::string & name, const T & val) {
    s32 location(uniformLocation(name));
    if (location == -1) {
        return false;
    }
    return uniform(location, val);
}
// Explicit template instatiation
template bool Shader::uniform<float>(const std::string &, const float &);
template bool Shader::uniform< vec2>(const std::string &, const  vec2 &);
template bool Shader::uniform< vec3>(const std::string &, const  vec3 &);
template bool Shader::uniform< vec4>(const std::string &, const  vec4 &);
template bool Shader::uniform<  s32>(const std::string &, const   s32 &);
template bool Shader::uniform<ivec2>(const std::string &, const ivec2 &);
template bool Shader::uniform<ivec3>(const std::string &, const ivec3 &);
template bool Shader::uniform<ivec4>(const std::string &, const ivec4 &);
template bool Shader::uniform<  u32>(const std::string &, const   u32 &);
template bool Shader::uniform<uvec2>(const std::string &, const uvec2 &);
template bool Shader::uniform<uvec3>(const std::string &, const uvec3 &);
template bool Shader::uniform<uvec4>(const std::string &, const uvec4 &);
template bool Shader::uniform< bool>(const std::string &, const  bool &);
template bool Shader::uniform<bvec2>(const std::string &, const bvec2 &);
template bool Shader::uniform<bvec3>(const std::string &, const bvec3 &);
template bool Shader::uniform<bvec4>(const std::string &, const bvec4 &);
template bool Shader::uniform< mat2>(const std::string &, const  mat2 &);
template bool Shader::uniform< mat3>(const std::string &, const  mat3 &);
template bool Shader::uniform< mat4>(const std::string &, const  mat4 &);

template <typename T>
bool Shader::uniform(s32 location, const T & val) {
    if      constexpr (std::is_same_v<T, float>) glUniform1f (location, val);
    else if constexpr (std::is_same_v<T,  vec2>) glUniform2f (location, val.x, val.y);
    else if constexpr (std::is_same_v<T,  vec3>) glUniform3f (location, val.x, val.y, val.z);
    else if constexpr (std::is_same_v<T,  vec4>) glUniform4f (location, val.x, val.y, val.z, val.w);
    else if constexpr (std::is_same_v<T,   s32>) glUniform1i (location, val);
    else if constexpr (std::is_same_v<T, ivec2>) glUniform2i (location, val.x, val.y);
    else if constexpr (std::is_same_v<T, ivec3>) glUniform3i (location, val.x, val.y, val.z);
    else if constexpr (std::is_same_v<T, ivec4>) glUniform4i (location, val.x, val.y, val.z, val.w);
    else if constexpr (std::is_same_v<T,   u32>) glUniform1ui(location, val);
    else if constexpr (std::is_same_v<T, uvec2>) glUniform2ui(location, val.x, val.y);
    else if constexpr (std::is_same_v<T, uvec3>) glUniform3ui(location, val.x, val.y, val.z);
    else if constexpr (std::is_same_v<T, uvec4>) glUniform4ui(location, val.x, val.y, val.z, val.w);
    else if constexpr (std::is_same_v<T,  bool>) glUniform1i (location, val);
    else if constexpr (std::is_same_v<T, bvec2>) glUniform2i (location, val.x, val.y);
    else if constexpr (std::is_same_v<T, bvec3>) glUniform3i (location, val.x, val.y, val.z);
    else if constexpr (std::is_same_v<T, bvec4>) glUniform4i (location, val.x, val.y, val.z, val.w);
    else if constexpr (std::is_same_v<T,  mat2>) glUniformMatrix2fv(location, 1, false, reinterpret_cast<const float *>(&val));
    else if constexpr (std::is_same_v<T,  mat3>) glUniformMatrix3fv(location, 1, false, reinterpret_cast<const float *>(&val));
    else if constexpr (std::is_same_v<T,  mat4>) glUniformMatrix4fv(location, 1, false, reinterpret_cast<const float *>(&val));
    else static_assert(false, "Uniform type unsupported");
    return true;
}
// Explicit template instatiation
template bool Shader::uniform<float>(s32, const float &);
template bool Shader::uniform< vec2>(s32, const  vec2 &);
template bool Shader::uniform< vec3>(s32, const  vec3 &);
template bool Shader::uniform< vec4>(s32, const  vec4 &);
template bool Shader::uniform<  s32>(s32, const   s32 &);
template bool Shader::uniform<ivec2>(s32, const ivec2 &);
template bool Shader::uniform<ivec3>(s32, const ivec3 &);
template bool Shader::uniform<ivec4>(s32, const ivec4 &);
template bool Shader::uniform<  u32>(s32, const   u32 &);
template bool Shader::uniform<uvec2>(s32, const uvec2 &);
template bool Shader::uniform<uvec3>(s32, const uvec3 &);
template bool Shader::uniform<uvec4>(s32, const uvec4 &);
template bool Shader::uniform< bool>(s32, const  bool &);
template bool Shader::uniform<bvec2>(s32, const bvec2 &);
template bool Shader::uniform<bvec3>(s32, const bvec3 &);
template bool Shader::uniform<bvec4>(s32, const bvec4 &);
template bool Shader::uniform< mat2>(s32, const  mat2 &);
template bool Shader::uniform< mat3>(s32, const  mat3 &);
template bool Shader::uniform< mat4>(s32, const  mat4 &);

s32 Shader::uniformLocation(const std::string & name) {
    auto it(m_uniforms.find(name));
    if (it == m_uniforms.end()) {
        s32 loc(glGetUniformLocation(m_glId, name.c_str()));
        if (loc < 0) {
            return -1;
        }
        it = m_uniforms.emplace(name, loc).first;
    }
    return it->second;
}

Shader::Shader(u32 glId) :
    m_glId(glId),
    m_uniforms()
{}