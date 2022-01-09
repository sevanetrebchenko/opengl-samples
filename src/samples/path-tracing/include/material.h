
#pragma once

namespace OpenGL {

    struct alignas(16) Material {
        // Returns whether material data was changed.
        [[nodiscard]] bool OnImGui();

        glm::vec3 albedo;
        float ior;

        // Emissive material properties.
        glm::vec3 emissive;
        float emissiveStrength;

        // Dielectric material properties.
        glm::vec3 absorbance;
        float refractionProbability;
        float refractionRoughness;

        // Metallic material properties.
        float reflectionProbability;
        float reflectionRoughness;
    };

}





