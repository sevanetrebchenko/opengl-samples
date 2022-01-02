
#pragma once

namespace OpenGL {

    struct alignas(16) Material {
        glm::vec3 albedo = glm::vec3(0.9f, 0.25f, 0.25f);
        float ior = 1.05f;

        // Emissive material properties.
        glm::vec3 emissive = glm::vec3(0.0f);

        // Dielectric material properties.
        float refractionProbability = 0.98f;
        glm::vec3 absorbance = glm::vec3(0.5f);
        float refractivity = 1.0f;

        // Metallic material properties.
        float reflectionProbability = 0.02f;
        float reflectivity = 1.0f;
    };

}



