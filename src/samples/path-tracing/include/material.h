
#pragma once

namespace OpenGL {

    struct alignas(16) Material {
        glm::vec3 albedo;
        float ior;

        // Emissive material properties.
        glm::vec3 emissive;

        // Dielectric material properties.
        float refractionProbability;
        glm::vec3 absorbance;
        float refractionRoughness;

        // Metallic material properties.
        float reflectionProbability;
        float reflectionRoughness;
    };

}





