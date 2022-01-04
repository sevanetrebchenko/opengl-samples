
#pragma once

namespace OpenGL {

    struct alignas(16) Material {
        glm::vec3 albedo = glm::vec3(1.0f);
        float ior = 1.125f;

        // Emissive material properties.
        glm::vec3 emissive = glm::vec3(0.0f);

        // Dielectric material properties.
        float refractionProbability = 1.0f;
        glm::vec3 absorbance = glm::vec3(1.0f, 0.0f, 0.0f);
        float refractivity = 0.0f;

        // Metallic material properties.
        float reflectionProbability = 0.1f;
        float reflectivity = 0.0f;
    };

}





