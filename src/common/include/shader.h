
#ifndef OPENGL_SAMPLES_SHADER_H
#define OPENGL_SAMPLES_SHADER_H

#include "pch.h"

namespace OpenGL {

    // Compiles all shader components and returns a linked program.
    // Throws std::runtime_error on compilation error.
    [[nodiscard]] GLuint LoadShader(std::initializer_list<std::pair<std::string, GLenum>> shaderComponents);

}

#endif //OPENGL_SAMPLES_SHADER_H
