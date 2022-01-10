
#pragma once

#include "pch.h"
#include "material.h"

namespace OpenGL {

    // Structs padded to a size of glm::vec4 (16 bytes) for GPU buffer alignment.

    struct alignas(16) Sphere {
        Sphere();

        // Returns whether sphere data was changed.
        [[nodiscard]] bool OnImGui();

        glm::vec3 position;
        float radius;

        Material material;
    };

    struct alignas(16) AABB {
        AABB();

        // Returns whether AABB data was changed.
        [[nodiscard]] bool OnImGui();

        glm::vec4 position;
        glm::vec4 dimensions;

        Material material;
    };

}