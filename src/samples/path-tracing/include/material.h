
#pragma once

#define LAMBERTIAN 0
#define METALLIC   1
#define DIELECTRIC 2
#define ISOTROPIC  3
#define EMISSIVE   4

namespace OpenGL {

    struct Material {
        glm::vec3 albedo;   // Color.

        bool metallic;



        float reflectionRoughness;

        glm::vec3 emissive; // Amount of light the object gives off.

        float ior;          // Index of refraction/reflection - how strongly light gets refracted
        float transparency;

        float refractionRoughness;
    };

}



