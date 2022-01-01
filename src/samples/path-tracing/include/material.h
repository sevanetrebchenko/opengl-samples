
#pragma once

#define LAMBERTIAN 0
#define METALLIC   1
#define DIELECTRIC 2
#define ISOTROPIC  3
#define EMISSIVE   4

namespace OpenGL {

    struct Material {
        glm::vec3 albedo = glm::vec3(1.0f);
        int type = 0;

        // Emissive material properties.
        glm::vec3 emissive = glm::vec3(99);

        // Metallic material properties.
        float reflectivity = 9.0f;   // How clear reflections are.

        // Dielectric material properties.
        glm::vec3 absorbance = glm::vec3(9.0f); // Beer's law.
        float refractivity = 9.0f;   // How clear refractions are.
        float ior = 9.0f;            // Index of refraction.
    };

}



