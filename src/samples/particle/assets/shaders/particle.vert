#version 450 core

#define EPSILON 0.001
const float DRAG = -0.2;

struct Particle {
    vec3 position;
    vec3 velocity;
};

layout (std140, binding = 0) buffer ParticleSSBO {
    Particle particles[];
} ssbo;

uniform float dt;
uniform vec3 centerOfGravity;
uniform float isActive;
uniform float isRunning;
uniform mat4 cameraTransform;

layout (location = 0) out vec4 particleColor;

void main() {
    Particle particle = ssbo.particles[gl_VertexID];

    vec3 toCenterOfGravity = centerOfGravity - particle.position;
    float distance = max(length(toCenterOfGravity), EPSILON);

    vec3 acceleration = 300.0 * isActive * isRunning / distance * (toCenterOfGravity / distance);
    particle.velocity *= mix(1.0, exp(DRAG * dt), isRunning);

    // Euler integration.
    particle.position += (dt * particle.velocity + 0.5 * acceleration * dt * dt) * isRunning;
    particle.velocity += acceleration * dt;

    ssbo.particles[gl_VertexID] = particle;

    float r = 0.0045 * dot(particle.velocity, particle.velocity);
    float g = clamp(0.08 * max(particle.velocity.x, max(particle.velocity.y, particle.velocity.z)), 0.2, 0.5);
    float b = 0.7 - r;

    particleColor = vec4(r, g, b, 0.15);
    gl_Position = cameraTransform * vec4(particle.position, 1.0f);
}