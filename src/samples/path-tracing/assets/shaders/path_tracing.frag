
#version 450 core

#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38
#define EPSILON 0.001
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



layout(std140, binding = 0) uniform GlobalData {
    // Camera information.
    mat4 inverseProjectionMatrix;
    mat4 inverseViewMatrix;
    vec3 cameraPosition;
} globalData;

layout (std140, binding = 1) readonly buffer ObjectData {
    int numSpheres;
    Sphere spheres[256];

    int numAABBs;
    AABB aabbs[256];
} objectData;

layout (binding = 0, rgba32f) readonly uniform image2D previousFrameImage;
layout (binding = 1) uniform samplerCube skyboxTexture;
uniform int frameCounter;
uniform int samplesPerPixel;
uniform int numRayBounces;
uniform float focusDistance;
uniform float apertureRadius;

layout (location = 0) out vec4 fragColor;



// https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
uint PCGHash(inout uint rngState) {
    rngState = rngState * 747796405u + 2891336453u;
    uint word = ((rngState >> ((rngState >> 28u) + 4u)) ^ rngState) * 277803737u;
    return (word >> 22u) ^ word;
}

// Returns a random float on the domain [min, max].
float RandomFloat(inout uint rngState, float min, float max) {
    // 32 bit precision spans from -2147483648 to 2147483647, which is 4294967295 unique values.
    float base = float(PCGHash(rngState)) / 4294967295.0;
    return min + base * (max - min);
}

vec2 RandomSampleUnitCircle(inout uint rngState) {
    float theta = RandomFloat(rngState, 0.0f, 1.0f) * 2.0 * PI;
    float r = sqrt(RandomFloat(rngState, 0.0f, 1.0f));
    return vec2(r * cos(theta), r * sin(theta));
}

OrthonormalBasis ConstructONB(vec3 normal) {
    OrthonormalBasis basis;
    basis.axes[2] = normalize(normal);

    vec3 a = (abs(basis.axes[2].x) > 0.9) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    basis.axes[1] = normalize(cross(basis.axes[2], a));
    basis.axes[0] = normalize(cross(basis.axes[2], basis.axes[1]));

    return basis;
}

vec3 GetLocalVector(OrthonormalBasis basis, vec3 vector) {
    return vector.x * basis.axes[0] + vector.y * basis.axes[1] + vector.z * basis.axes[2];
}

// Generates a random cosine weighted vector within the orthonormal basis surrounding the given normal 'n'.
vec3 GenerateRandomDirection(inout uint rngState, vec3 n) {
    OrthonormalBasis basis = ConstructONB(n);

    // https://www.particleincell.com/2015/cosine-distribution/
    float cosTheta = sqrt(1.0 - RandomFloat(rngState, 0.0, 1.0));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2.0 * PI * RandomFloat(rngState, 0.0, 1.0);

    vec3 vector = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    vector = normalize(GetLocalVector(basis, vector));

    // Ensure scatter direction does not cancel out normal.
    if (abs(vector.x) < EPSILON && abs(vector.y) < EPSILON && abs(vector.z) < EPSILON) {
        vector = n;
    }

    return vector;
}

// Returns a ray in world space based on normalized screen-space coordinates.
Ray GetWorldSpaceRay(vec2 ndc) {
    vec4 direction = globalData.inverseProjectionMatrix * vec4(ndc, -1.0, 0.0);
    direction.zw = vec2(-1.0, 0.0);
    return Ray(globalData.cameraPosition, normalize(globalData.inverseViewMatrix * direction).xyz);
}

bool Intersects(Ray ray, Sphere sphere, float tMin, float tMax, inout HitRecord hitRecord) {
    // https://antongerdelan.net/opengl/raycasting.html
    vec3 sphereToRayOrigin = ray.origin - sphere.position;

    float b = dot(sphereToRayOrigin, ray.direction);
    float c = dot(sphereToRayOrigin, sphereToRayOrigin) - (sphere.radius * sphere.radius);

    float discriminant = b * b - c;
    if (discriminant < 0.0) {
        // No real roots, no intersection.
        return false;
    }

    float sqrtDiscriminant = sqrt(discriminant);

    float t1 = -b - sqrtDiscriminant;
    float t2 = -b + sqrtDiscriminant;

    if (t2 < 0.0) {
        // Ray exited behind the origin (sphere is behind the camera).
        return false;
    }

    // Bounds check.
    float t = t1 < 0.0 ? t2 : t1;
    if (t < tMin || t > tMax) {
        return false;
    }

    hitRecord.t = t;
    hitRecord.point = ray.origin + ray.direction * hitRecord.t;

    vec3 normal = normalize(hitRecord.point - sphere.position);

    // Positive dot product means vectors point in the same direction.
    hitRecord.fromInside = dot(ray.direction, normal) > 0.0;
    hitRecord.normal = hitRecord.fromInside ? -normal : normal;

    hitRecord.material = sphere.material;

    return true;
}

bool Intersects(Ray ray, AABB aabb, float tMin, float tMax, inout HitRecord hitRecord) {
    // https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
    vec3 inverseRayDirection = vec3(1.0) / ray.direction;

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
    vec3 center = (aabb.minimum + aabb.maximum) * 0.5;
    vec3 dimensions = abs(aabb.maximum - aabb.minimum) * 0.5;

    vec3 pc = hitRecord.point - center;

    vec3 normal = vec3(0.0);
    normal += vec3(sign(pc.x), 0.0, 0.0) * step(abs(abs(pc.x) - dimensions.x), EPSILON);
    normal += vec3(0.0, sign(pc.y), 0.0) * step(abs(abs(pc.y) - dimensions.y), EPSILON);
    normal += vec3(0.0, 0.0, sign(pc.z)) * step(abs(abs(pc.z) - dimensions.z), EPSILON);

    // Ensure normal always points against the incident ray.
    normal = normalize(normal);

    hitRecord.fromInside = dot(ray.direction, normal) > 0.0;
    hitRecord.normal = hitRecord.fromInside ? -normal : normal;

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
    for (int i = 0; i < objectData.numAABBs; ++i) {
        if (Intersects(ray, objectData.aabbs[i], tMin, nearestIntersectionTime, temp)) {
            intersected = true;
            nearestIntersectionTime = temp.t;
        }
    }

    if (intersected) {
        hitRecord = temp;
    }

    return intersected;
}

// Schlick approximation.
float SchlickApproximation(float cosTheta, float n1, float n2) {
    float f = (n1 - n2) / (n1 + n2);
    f *= f;
    return f + (1.0 - f) * pow(1.0 - cosTheta, 5.0);
}

// n1 - ior of the material the ray originated in.
// n2 - ior of the material the ray is entering.
float FresnelReflectAmount(vec3 v, vec3 n, float n1, float n2) {
    float cosTheta = dot(-v, n);

    if (n2 < n1) {
        // Ray originated from a denser material, and is entering a lighter one.
        float eta = n1 / n2;
        float sinTheta = sqrt(1.0 - cosTheta * cosTheta); // Trig identity.

        if (eta * sinTheta > 1.0) {
            // Total internal reflection, full reflection.
            return 1.0;
        }
    }

    return SchlickApproximation(cosTheta, n1, n2); // Solve Fresnel equations.
}

vec3 Radiance(uint rngState, Ray ray) {
    vec3 throughput = vec3(1.0);
    vec3 radiance = vec3(0.0);

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
            if (reflectionProbability > 0.0) {
                float n1;
                float n2;

                // Determine material indices.
                // Assumes camera is in air (1.0).
                if (hitRecord.fromInside) {
                    n1 = material.ior;
                    n2 = 1.0;
                }
                else {
                    n1 = 1.0;
                    n2 = material.ior;
                }

                reflectionProbability = mix(material.reflectionProbability, 1.0, FresnelReflectAmount(v, n, n1, n2));

                // Need to maintain the same probability ratio for refraction and diffuse later.
                refractionProbability *= (1.0 - reflectionProbability) / (1.0 - material.reflectionProbability);
            }

            // Randomly determine which ray to follow based on material properties.
            float rayProbability;
            float raySelectRoll = RandomFloat(rngState, 0.0, 1.0);

            float reflectionFactor = 0.0;
            float refractionFactor = 0.0;

            if (reflectionProbability > 0.0 && raySelectRoll < reflectionProbability) {
                // Reflection ray.
                reflectionFactor = 1.0;
                rayProbability = reflectionProbability;
            }
            else if (refractionProbability > 0.0 && raySelectRoll < (reflectionProbability + refractionProbability)) {
                // Refraction ray.
                refractionFactor = 1.0;
                rayProbability = refractionProbability;
            }
            else {
                // Diffuse ray.
                rayProbability = 1.0 - (reflectionProbability + refractionProbability);
            }

            // Avoid division by 0.
            rayProbability = max(rayProbability, EPSILON);

            // Prevent floating point error from triggering an intersection with the object we just intersected.
            if (refractionFactor > 0.5) {
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
            float eta = hitRecord.fromInside ? material.ior : 1.0 / material.ior;
            vec3 refractionRayDirection = refract(v, n, eta);
            refractionRayDirection = normalize(mix(refractionRayDirection, GenerateRandomDirection(rngState, -n), material.refractionRoughness * material.refractionRoughness));

            ray.direction = mix(diffuseRayDirection, reflectionRayDirection, reflectionFactor);
            ray.direction = mix(ray.direction, refractionRayDirection, refractionFactor);

            ray.direction = normalize(ray.direction);

            // Emissive lighting.
            radiance += material.emissive * throughput;

            // Refraction alone has no final color contribution, need to trace again until the new ray direction hits another object.
            // Apply light absorbtion over distance through refractive object.
            if (refractionFactor < 0.5) {
                throughput *= material.albedo;
            }

            // Only one final ray was selected to trace, account for not choosing the other two.
            throughput /= rayProbability;

            // Russian Roulette.
            // As the throughput gets smaller and smaller, the ray has a higher chance of being terminated.
            float probability = max(throughput.r, max(throughput.g, throughput.b));
            if (probability < RandomFloat(rngState, 0.0, 1.0)) {
                break;
            }

            // Add the energy that is lost by randomly terminating paths.
            throughput /= probability;
        }
        else {
            // Ray didn't hit anything, sample skybox texture.
            radiance += texture(skyboxTexture, ray.direction).rgb * throughput;
            break;
        }
    }

    return radiance;
}

void main() {
    vec3 color = vec3(0.0);
    uint rngState = uint(gl_FragCoord.x * 1973 + gl_FragCoord.y * 9277 + frameCounter * 2699) | uint(1);

    ivec2 resolution = imageSize(previousFrameImage);

    for (int i = 0; i < samplesPerPixel; ++i) {
        // Generate random sub-pixel offset for antialiasing.
        vec2 subPixelOffset = vec2(RandomFloat(rngState, 0.0, 1.0), RandomFloat(rngState, 0.0, 1.0)) - 0.5;
        vec2 ndc = (gl_FragCoord.xy + subPixelOffset) / resolution * 2.0 - 1.0;

        Ray ray = GetWorldSpaceRay(ndc); // Ray

        // Everything in the virtual film plane 'focusDistance' away from the camera eye position is in perfect focus.
        vec3 focalPoint = ray.origin + ray.direction * focusDistance;

        // Jittering the start of the ray based on the aperture size increases the effect of depth of field (DOF).
        vec2 jitter = apertureRadius * RandomSampleUnitCircle(rngState);

        ray.origin = (globalData.inverseViewMatrix * vec4(jitter, 0.0, 1.0)).xyz;
        ray.direction = normalize(focalPoint - ray.origin);

        color += Radiance(rngState, ray);
    }

    color /= samplesPerPixel;

    vec4 lastFrameColor = imageLoad(previousFrameImage, ivec2(gl_FragCoord.xy));
    float blend = (lastFrameColor.a == 0.0f) ? 1.0f : 1.0f / (1.0f + (1.0f / lastFrameColor.a));
    color = mix(lastFrameColor.rgb, color, blend);

    fragColor = vec4(color, blend);
}

