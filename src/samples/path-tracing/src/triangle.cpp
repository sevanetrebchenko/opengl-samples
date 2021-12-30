
#include "triangle.h"

namespace OpenGL {

    Triangle::Triangle(const glm::vec3& vertex1, const glm::vec3& vertex2, const glm::vec3& vertex3) : v1(glm::vec4(vertex1, 0.0f)),
                                                                                                       v2(glm::vec4(vertex1, 0.0f)),
                                                                                                       v3(glm::vec4(vertex1, 0.0f))
                                                                                                       {
        // Calculate triangle normal.
        glm::vec3 u = v2 - v1;
        glm::vec3 v = v3 - v1;
        normal = glm::vec4(glm::cross(u, v), 0.0f);
    }
}
