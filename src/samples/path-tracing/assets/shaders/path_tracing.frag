
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
    float refractivity;

    // Metallic material properties.
    float reflectionProbability;
    float reflectivity;
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
uint rng;
uint GetPCGHash(inout uint seed) {
    seed = seed * 747796405u + 2891336453u;
    uint word = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    return (word >> 22u) ^ word;
}

float GetRandomFloat01() {
    return float(GetPCGHash(rng)) / 4294967296.0;
}

//// Returns pseudo-random float on the domain [min, max].
//float RandomFloat(float min, float max) {
//    // 32 bit precision spans from -2147483648 to 2147483647, which is 4294967295 unique values.
//    float base = float(PCGHash()) / 4294967295.0f;
//    return min + base * (max - min);
//}

//vec3 RandomCosineDirection() {
//    float r1 = GetRandomFloat01(0.0f, 1.0f);
//    float r2 = RandomFloat(0.0f, 1.0f);
//    float z = sqrt(1.0f - r2);
//
//    float phi = 2.0f * PI * r1;
//    float x = cos(phi) * sqrt(r2);
//    float y = sin(phi) * sqrt(r2);
//
//    return vec3(x, y, z);
//}
//
//vec3 RandomDirection(float min, float max) {
//    return vec3(RandomFloat(min, max), RandomFloat(min, max), RandomFloat(min, max));
//}

// Returns a ray in world space based on normalized screen-space coordinates.
Ray GetWorldSpaceRay(vec2 ndc) {
    vec4 direction = inverseProjectionMatrix * vec4(ndc, -1.0f, 0.0f);
    direction.zw = vec2(-1.0f, 0.0f);
    return Ray(cameraPosition, normalize(inverseViewMatrix * direction).xyz);
}

bool Intersects(Ray ray, Sphere sphere, float tMin, float tMax, inout HitRecord hitRecord) {
    vec3 sphereToRayOrigin = ray.origin - sphere.position;

    float a = dot(ray.direction, ray.direction);
    float b = 2.0f * dot(sphereToRayOrigin, ray.direction);
    float c = dot(sphereToRayOrigin, sphereToRayOrigin) - (sphere.radius * sphere.radius);

    float discriminant = (b * b - 4.0f * a * c);
    if (discriminant < 0.0f) {
        // No real roots, no intersection.
        return false;
    }

    float sqrtDiscriminant = sqrt(discriminant);
    float denominator = 2.0f / a;

    float root = (-b - sqrtDiscriminant) * denominator;

    // Find the nearest root that lies in the acceptable range.
    if (root < tMin || root > tMax) {
        root = (-b + sqrtDiscriminant) * denominator;

        if (root < tMin || root > tMax) {
            return false;
        }
    }

    // Intersection detected.
    hitRecord.t = root;
    hitRecord.point = ray.origin + ray.direction * root;

    // Ensure normal always points against the incident ray.
    vec3 normal = normalize(hitRecord.point - sphere.position);
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
    float tMin = 0.001f; // Slight offset.
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

float FresnelSchlick(float cosTheta, float n1, float n2) {
    float f = (n1 - n2) / (n1 + n2);
    f *= f;
    return f + (1.0f - f) * pow(1.0f - cosTheta, 5.0f);
}

vec3 CosineSampleHemisphere(vec3 normal) {
    // Source: https://blog.demofox.org/2020/05/25/casual-shadertoy-path-tracing-1-basic-camera-diffuse-emissive/

    float z = GetRandomFloat01() * 2.0 - 1.0;
    float a = GetRandomFloat01() * 2.0 * PI;
    float r = sqrt(1.0 - z * z);
    float x = r * cos(a);
    float y = r * sin(a);

    // Convert unit vector in sphere to a cosine weighted vector in hemisphere
    return normalize(normal + vec3(x, y, z));
}

// Bidirectional scattering distribution function (deals with full sphere surrounding surface normal).
// Returns the probability that the given ray was chosen.
float BSDF(inout Ray ray, HitRecord hitRecord) {
    float reflectionProbability = hitRecord.material.reflectionProbability;
    float refractionProbability = hitRecord.material.refractionProbability;

    vec3 v = normalize(ray.direction);
    vec3 n = normalize(hitRecord.normal);

    if (reflectionProbability > 0.0f) {
        // Apply Fresnel effect to all objects (affects ray reflection probability at grazing angles of incidence).
        // Assume outside medium to be air (n = 1.0f).
        float t;
        float cosTheta = dot(-v, n);

        if (hitRecord.fromInside) {
            t = FresnelSchlick(cosTheta, hitRecord.material.ior, 1.0f);
        }
        else {
            t = FresnelSchlick(cosTheta, 1.0f, hitRecord.material.ior);
        }

        reflectionProbability = mix(reflectionProbability, 1.0f, t);
        float diffuseProbability = 1.0f - reflectionProbability - refractionProbability;
        refractionProbability = 1.0f - reflectionProbability - diffuseProbability;
    }

    float probability = 1.0f;

    // Select which ray to follow based.
    float raySelect = GetRandomFloat01();
    vec3 direction;

    if (reflectionProbability > raySelect) {
        // Reflect.
        direction = reflect(v, n);
        probability = reflectionProbability;
    }
    else if (reflectionProbability + refractionProbability > raySelect) {
        // Refract.
        float eta = hitRecord.fromInside ? hitRecord.material.ior : 1.0f / hitRecord.material.ior;
        direction = refract(v, n, eta);
        probability = refractionProbability;
    }
    else {
        // Diffuse ray (Lambert's cosine law).
        direction = CosineSampleHemisphere(n);
        probability = 1.0f - reflectionProbability - refractionProbability;
    }

    ray.direction = normalize(direction);

    // Prevent floating point error from triggering an intersection with the object we just intersected.
    ray.origin = hitRecord.point + ray.direction * EPSILON;

    return max(probability, EPSILON);
}

vec3 Radiance(Ray ray) {
    vec3 throughput = vec3(1.0f);
    vec3 radiance = vec3(0.0f);

    HitRecord hitRecord;

    for (int i = 0; i < numRayBounces; ++i) {
        if (Trace(ray, hitRecord)) {
            if (hitRecord.fromInside) {
                hitRecord.normal *= -1.0f;

                // Emerging from within medium, apply Beer's law.
                // Beer's law is scaled over the distance the ray traveled while inside the medium. This can be simulated
                // by scaling the absorbance of the medium by the intersection time of the ray. The longer the intersection
                // time is, the further the ray traveled before emerging from the medium.
                throughput *= exp(-hitRecord.material.absorbance * hitRecord.t);
            }

            float selectionProbability = BSDF(ray, hitRecord);

            radiance += hitRecord.material.emissive * throughput;

            throughput *= hitRecord.material.albedo;
            throughput /= selectionProbability;

            float p = max(throughput.x, max(throughput.y, throughput.z));
            if (GetRandomFloat01() > p)
                break;

            throughput /= p;
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
    rng = uint(gl_FragCoord.x * 1973 + gl_FragCoord.y * 9277) | uint(1);

    for (int i = 0; i < samplesPerPixel; ++i) {
        // Generate random sub-pixel offset for antialiasing.
        vec2 offset = vec2(GetRandomFloat01(), GetRandomFloat01()) - 0.5f;
        vec2 ndc = (gl_FragCoord.xy + offset) / imageResolution * 2.0f - 1.0f;

        Ray ray = GetWorldSpaceRay(ndc);
        color += Radiance(ray);
    }

    color /= samplesPerPixel;
    fragColor = vec4(color, 1.0f);
}

