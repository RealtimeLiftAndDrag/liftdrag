#pragma once

#include "Global.hpp"



namespace gli {

    // If there is an OpenGL error, prints an error message. Returns the error
    u32 checkError();
    // May also supply additional debug info
    u32 checkError(const char * file, int line, const char * statement);

    // Returns the name of the OpenGL error code
    const std::string & errorString(u32 err);

}

// Convenient OpenGL error encapsulation
// Example:
//  if (CHECK_GL(progId = glCreateShader(GL_VERTEX_SHADER))) {
//      // There was an error
//  }
#ifndef DISABLE_OPENGL_ERROR_CHECKS
#define CHECK_GL(x) x; bool(gli::checkError(__FILE__, __LINE__, #x))
#else
#define CHECK_GL(x) x; false
#endif