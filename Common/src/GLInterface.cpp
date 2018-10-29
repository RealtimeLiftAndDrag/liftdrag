#include "GLInterface.hpp"

#include <unordered_map>
#include <iostream>

#include "glad/glad.h"



namespace gli {

    u32 checkError() {
        u32 err(glGetError());
        if (err) std::cerr << "OpenGL error: " << errorString(err) << std::endl;
        return err;
    }

    u32 checkError(const char * file, int line, const char * statement) {
        u32 err(glGetError());
        if (err) {
            std::cerr << "OpenGL error: " << errorString(err);
            if (file || statement) {
                std::cerr << " (";
                if (file) std::cerr << file << ":" << line;
                if (file && statement) std::cerr << ":";
                if (statement) std::cerr << statement;
                std::cerr << ")";
            }
            std::cerr << std::endl;
        }
        return err;
    }

    const std::string & errorString(u32 err) {
        const static std::unordered_map<u32, std::string> k_errMap{
            {          GL_NO_ERROR,          "No error"s },
            {      GL_INVALID_ENUM,      "Invalid enum"s },
            {     GL_INVALID_VALUE,     "Invalid value"s },
            { GL_INVALID_OPERATION, "Invalid operation"s },
            {    GL_STACK_OVERFLOW,    "Stack overflow"s },
            {   GL_STACK_UNDERFLOW,   "Stack underflow"s },
            {     GL_OUT_OF_MEMORY,     "Out of memory"s }
        };

        return k_errMap.at(err);
    }

}