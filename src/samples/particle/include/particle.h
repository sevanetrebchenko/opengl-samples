
#ifndef OPENGL_SAMPLES_PARTICLE_H
#define OPENGL_SAMPLES_PARTICLE_H

#include "pch.h"

namespace OpenGL {

    struct alignas(16u) Particle  {
        // glm::vec4 instead of glm::vec3 for buffer alignment.
        glm::vec4 position;
        glm::vec4 velocity;
    };

}

#endif //OPENGL_SAMPLES_PARTICLE_H
