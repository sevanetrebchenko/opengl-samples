
#pragma once

#include "pch.h"
#include "material.h"

namespace OpenGL {

    struct alignas(16) Sphere {
        glm::vec3 position = glm::vec3(0.0f);
        float radius = 5.0f;

        Material material;
    };

    // Struct padded to a size of glm::vec4 (16 bytes) for UBO alignment.
    struct AABB {
        glm::vec4 minimum;
        glm::vec4 maximum;

        Material material;
    };

}