
#pragma once

namespace OpenGL {

    struct alignas(16) Material {
        glm::vec3 albedo = glm::vec3(0.9f, 0.25f, 0.25f);
        float ior = 1.01f;

        // Emissive material properties.
        glm::vec3 emissive = glm::vec3(0.0f);

        // Dielectric material properties.
        float refractionProbability = 1.0f;
        glm::vec3 absorbance = glm::vec3(0.1f);
        float refractivity = 0.0f;

        // Metallic material properties.
        float reflectionProbability = 0.0f;
        float reflectivity = 0.0f;
    };

}



