
#pragma once

#include "pch.h"

namespace OpenGL {

    struct Sphere {
        glm::vec3 center;
        float radius;
    };

    // Struct padded to a size of glm::vec4 (16 bytes) for UBO alignment.
    struct Cuboid {
        glm::vec4 dimensions;
    };

}