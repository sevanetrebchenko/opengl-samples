
#pragma once

#include "pch.h"

namespace OpenGL {

    // Struct padded to a size of glm::vec4 (16 bytes) for UBO alignment.
    struct Triangle {
        // Computes triangle normal.
        Triangle(const glm::vec3& vertex1, const glm::vec3& vertex2, const glm::vec3& vertex3);
        ~Triangle() = default;

        glm::vec4 v1;
        glm::vec4 v2;
        glm::vec4 v3;
        glm::vec4 normal;
    };

}