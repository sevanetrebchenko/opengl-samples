
#version 450 core

#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38
#define EPSILON 0.0001f
#define PI 3.14159265359

struct Material {
    vec3 albedo;
    float ior;

    // Emissive material properties.
    vec3 emissive;

    // Dielectric material properties.
    float refractionProbability;
    vec3 absorbance;
    float refractionRoughness;

    // Metallic material properties.
    float reflectionProbability;
    float reflectionRoughness;
};

struct Sphere {
    vec3 position;
    float radius;

    Material material;
};

struct AABB {
    vec3 minimum;
    vec3 maximum;

    Material material;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct HitRecord {
    float t;
    vec3 point;
    vec3 normal;
    bool fromInside;

    Material material;
};

struct OrthonormalBasis {
    vec3[3] axes;
};

uniform samplerCube skybox;

layout (std140, binding = 1) readonly buffer ObjectData {
    int numSpheres;
    Sphere spheres[256];
} objectData;

uniform mat4 inverseProjectionMatrix;
uniform mat4 inverseViewMatrix;

uniform vec3 cameraPosition;

uniform vec2 imageResolution;
uniform uint frame;

uniform int samplesPerPixel;
uniform int numRayBounces;

out vec4 fragColor;

// https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
uint PCGHash(inout uint rngState) {
    uint state = rngState;
    rngState = rngState * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Returns a random float on the domain [min, max].
float RandomFloat(inout uint rngState, float min, float max) {
    // 32 bit precision spans from -2147483648 to 2147483647, which is 4294967295 unique values.
    float base = float(PCGHash(rngState)) / 4294967295.0f;
    return min + base * (max - min);
}

OrthonormalBasis ConstructONB(vec3 normal) {
    OrthonormalBasis basis;
    basis.axes[2] = normalize(normal);

    vec3 a = (abs(basis.axes[2].x) > 0.9f) ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 0.0f, 0.0f);
    basis.axes[1] = normalize(cross(basis.axes[2], a));
    basis.axes[0] = normalize(cross(basis.axes[2], basis.axes[1]));

    return basis;
}

vec3 GetLocalVector(OrthonormalBasis basis, vec3 vector) {
    return vector.x * basis.axes[0] + vector.y * basis.axes[1] + vector.z * basis.axes[2];
}

// Generates a random cosine weighted vector within the orthonormal basis surrounding the given normal 'n'.
// https://www.particleincell.com/2015/cosine-distribution/
vec3 GenerateRandomDirection(inout uint rngState, vec3 n) {
    OrthonormalBasis basis = ConstructONB(n);

    float cosTheta = sqrt(1.0f - RandomFloat(rngState, 0.0f, 1.0f));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    float phi = 2.0f * PI * RandomFloat(rngState, 0.0f, 1.0f);

    vec3 vector = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    return normalize(GetLocalVector(basis, vector));
}

// Returns a ray in world space based on normalized screen-space coordinates.
Ray GetWorldSpaceRay(vec2 ndc) {
    vec4 direction = inverseProjectionMatrix * vec4(ndc, -1.0f, 0.0f);
    direction.zw = vec2(-1.0f, 0.0f);
    return Ray(cameraPosition, normalize(inverseViewMatrix * direction).xyz);
}

bool Intersects(Ray ray, Sphere sphere, float tMin, float tMax, inout HitRecord hitRecord) {
    // https://antongerdelan.net/opengl/raycasting.html
    vec3 sphereToRayOrigin = ray.origin - sphere.position;

    float b = dot(sphereToRayOrigin, ray.direction);
    float c = dot(sphereToRayOrigin, sphereToRayOrigin) - (sphere.radius * sphere.radius);

    float discriminant = b * b - c;
    if (discriminant < 0.0f) {
        // No real roots, no intersection.
        return false;
    }

    float sqrtDiscriminant = sqrt(discriminant);

    float t1 = -b - sqrtDiscriminant;
    float t2 = -b + sqrtDiscriminant;

    if (t2 < 0.0f) {
        // Ray exited behind the origin (sphere is behind the camera).
        return false;
    }

    // Bounds check.
    float t = t1 < 0.0f ? t2 : t1;
    if (t < tMin || t > tMax) {
        return false;
    }

    hitRecord.t = t;
    hitRecord.point = ray.origin + ray.direction * hitRecord.t;

    vec3 normal = normalize(hitRecord.point - sphere.position);

    // Positive dot product means vectors point in the same direction.
    hitRecord.fromInside = dot(ray.direction, normal) > 0.0f;
    hitRecord.normal = hitRecord.fromInside ? -normal : normal;

    hitRecord.material = sphere.material;

    return true;
}

bool Intersects(Ray ray, AABB aabb, float tMin, float tMax, inout HitRecord hitRecord) {
    // https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
    vec3 inverseRayDirection = vec3(1.0f) / ray.direction;

    vec3 t0s = (aabb.minimum - ray.origin) * inverseRayDirection;
    vec3 t1s = (aabb.maximum - ray.origin) * inverseRayDirection;

    vec3 tMinimum = min(t0s, t1s);
    vec3 tMaximum = max(t0s, t1s);

    tMin = max(tMin, max(tMinimum[0], max(tMinimum[1], tMinimum[2])));
    tMax = min(tMax, min(tMaximum[0], min(tMaximum[1], tMaximum[2])));

    // tMin >= tMax means no intersection.
    if (tMin > tMax || abs(tMax - tMin) < EPSILON) {
        return false;
    }

    // float root = tMin;
    hitRecord.t = tMin;
    hitRecord.point = ray.origin + ray.direction * tMin;

    // https://gist.github.com/Shtille/1f98c649abeeb7a18c5a56696546d3cf
    vec3 center = (aabb.minimum + aabb.maximum) * 0.5f;
    vec3 dimensions = (aabb.maximum - aabb.minimum) * 0.5f;

    vec3 pc = hitRecord.point - center;

    vec3 normal = vec3(0.0f);
    normal += vec3(sign(pc.x), 0.0f, 0.0f) * step(abs(abs(pc.x) - dimensions.x), EPSILON);
    normal += vec3(0.0f, sign(pc.y), 0.0f) * step(abs(abs(pc.y) - dimensions.y), EPSILON);
    normal += vec3(0.0f, 0.0f, sign(pc.z)) * step(abs(abs(pc.z) - dimensions.z), EPSILON);

    // Ensure normal always points against the incident ray.
    hitRecord.normal = normalize(normal);
    hitRecord.fromInside = dot(ray.direction, hitRecord.normal) > 0.0f;

    hitRecord.material = aabb.material;

    return true;
}

bool Trace(Ray ray, out HitRecord hitRecord) {
    float tMin = EPSILON; // Slight offset.
    float tMax = FLT_MAX;

    bool intersected = false;
    float nearestIntersectionTime = tMax;

    HitRecord temp;
    temp.t = tMax;

    // Intersect with all spheres.
    for (int i = 0; i < objectData.numSpheres; ++i) {
        if (Intersects(ray, objectData.spheres[i], tMin, nearestIntersectionTime, temp)) {
            intersected = true;
            nearestIntersectionTime = temp.t;
        }
    }

    // Intersect with all AABBs.
    //for (int i = 0; i < objectData.aabbs.length(); ++i) {
    //    if (Intersects(ray, objectData.aabbs[i], tMin, nearestIntersectionTime, temp)) {
    //        intersected = true;
    //        nearestIntersectionTime = temp.t;
    //    }
    //}

    if (intersected) {
        hitRecord = temp;
    }

    return intersected;
}

// Schlick approximation.
float SchlickApproximation(float cosTheta, float n1, float n2) {
    float f = (n1 - n2) / (n1 + n2);
    f *= f;
    return f + (1.0f - f) * pow(1.0f - cosTheta, 5.0f);
}

// n1 - ior of the material the ray originated in.
// n2 - ior of the material the ray is entering.
float FresnelReflectAmount(vec3 n, vec3 v, float n1, float n2) {
    float cosTheta = dot(-v, n);

    if (n2 < n1) {
        // Ray originated from a denser material, and is entering a lighter one.
        float eta = n1 / n2;
        float sinTheta = sqrt(1.0f - cosTheta * cosTheta); // Trig identity.

        if (eta * sinTheta > 1.0f) {
            // Total internal reflection, full reflection.
            return 1.0f;
        }
    }

    return SchlickApproximation(cosTheta, n1, n2); // Solve Fresnel equations.
}

vec3 Radiance(uint rngState, Ray ray) {
    vec3 throughput = vec3(1.0f);
    vec3 radiance = vec3(0.0f);

    HitRecord hitRecord;

    for (int i = 0; i < numRayBounces; ++i) {
        if (Trace(ray, hitRecord)) {

            Material material = hitRecord.material;
            vec3 v = normalize(ray.direction);
            vec3 n = normalize(hitRecord.normal);

            // https://blog.demofox.org/2020/06/14/casual-shadertoy-path-tracing-3-fresnel-rough-refraction-absorption-orbit-camera/

            if (hitRecord.fromInside) {
                // Emerging from within medium, apply Beer's law.
                // Beer's law is scaled over the distance the ray traveled while inside the medium. This can be simulated
                // by scaling the absorbance of the medium by the intersection time of the ray. The longer the intersection
                // time is, the further the ray traveled before emerging from the medium.
                throughput *= exp(-material.absorbance * hitRecord.t);
            }

            // Pre-Fresnel.
            float reflectionProbability = material.reflectionProbability;
            float refractionProbability = material.refractionProbability;

            // Adjust probabilities for Fresnel effect.
            if (reflectionProbability > 0.0f) {
                float n1;
                float n2;

                // Determine material indices.
                // Assumes camera is in air (1.0f).
                if (hitRecord.fromInside) {
                    n1 = material.ior;
                    n2 = 1.0f;
                }
                else {
                    n1 = 1.0f;
                    n2 = material.ior;
                }

                float t = FresnelReflectAmount(v, n, n1, n2);
                reflectionProbability = mix(material.reflectionProbability, 1.0f, t);

                // Need to maintain the same probability ratio for refraction and diffuse later.
                float scalingFactor = (1.0f - reflectionProbability) / (1.0f - material.reflectionProbability);
                refractionProbability *= scalingFactor;
            }

            // Randomly determine which ray to follow based on material properties.
            float rayProbability;
            float raySelectRoll = RandomFloat(rngState, 0.0f, 1.0f);

            float reflectionFactor = 0.0f;
            float refractionFactor = 0.0f;

            if (reflectionProbability > 0.0f && raySelectRoll < reflectionProbability) {
                // Reflection ray.
                reflectionFactor = 1.0f;
                rayProbability = reflectionProbability;
            }
            else if (refractionProbability > 0.0f && raySelectRoll < (reflectionProbability + refractionProbability)) {
                // Refraction ray.
                refractionFactor = 1.0f;
                rayProbability = refractionProbability;
            }
            else {
                // Diffuse ray.
                rayProbability = 1.0f - (reflectionProbability + refractionProbability);
            }

            // Avoid division by 0.
            rayProbability = max(rayProbability, EPSILON);

            // Prevent floating point error from triggering an intersection with the object we just intersected.
            if (refractionFactor > 0.5f) {
                // Refraction goes into the surface.
                ray.origin = hitRecord.point - hitRecord.normal * EPSILON;
            }
            else {
                ray.origin = hitRecord.point + hitRecord.normal * EPSILON;
            }

            // Calculate new ray direction.
            vec3 diffuseRayDirection = GenerateRandomDirection(rngState, n);

            // Interpolate between smooth specular and rough diffuse directions by the surface material properties.
            vec3 reflectionRayDirection = reflect(v, n);
            reflectionRayDirection = normalize(mix(reflectionRayDirection, diffuseRayDirection, material.reflectionRoughness * material.reflectionRoughness));

            // Interpolate between smooth refraction and rough diffuse directions by the surface material properties.
            float eta = hitRecord.fromInside ? material.ior : 1.0f / material.ior;
            vec3 refractionRayDirection = refract(v, n, eta);
            refractionRayDirection = normalize(mix(refractionRayDirection, GenerateRandomDirection(rngState, -n), material.refractionRoughness * material.refractionRoughness));

            ray.direction = mix(diffuseRayDirection, reflectionRayDirection, reflectionFactor);
            ray.direction = mix(ray.direction, refractionRayDirection, refractionFactor);

            ray.direction = normalize(ray.direction);

            // Emissive lighting.
            radiance += material.emissive * throughput;

            // Refraction alone has no final color contribution, need to trace again until the new ray direction hits another object.
            // Apply light absorbtion over distance through refractive object.
            if (refractionFactor < 0.5f) {
                throughput *= material.albedo;
            }

            // Only one final ray was selected to trace, account for not choosing the other two.
            throughput /= rayProbability;

            // Russian Roulette.
            // As the throughput gets smaller and smaller, the ray has a higher chance of being terminated.
            float probability = max(throughput.r, max(throughput.g, throughput.b));
            if (probability < RandomFloat(rngState, 0.0f, 1.0f)) {
                break;
            }

            // Add the energy that is lost by randomly terminating paths.
            throughput /= probability;
        }
        else {
            // Ray didn't hit anything, sample skybox texture.
            radiance += texture(skybox, ray.direction).rgb * throughput;
            break;
        }
    }

    return radiance;
}

void main() {
    vec3 color = vec3(0.0f);
    uint rngState = uint(gl_FragCoord.x * 1973 + gl_FragCoord.y * 9277 + frame * 26699) | uint(1);

    for (int i = 0; i < samplesPerPixel; ++i) {
        // Generate random sub-pixel offset for antialiasing.
        vec2 offset = vec2(RandomFloat(rngState, 0.0f, 1.0f), RandomFloat(rngState, 0.0f, 1.0f)) - 0.5f;
        vec2 ndc = (gl_FragCoord.xy + offset) / imageResolution.xy * 2.0f - 1.0f;

        Ray ray = GetWorldSpaceRay(ndc);
        color += Radiance(rngState, ray);
    }

    color /= samplesPerPixel;
    fragColor = vec4(color, 1.0f);
}

