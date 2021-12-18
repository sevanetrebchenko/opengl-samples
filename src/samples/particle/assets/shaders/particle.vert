#version 450 core

#define EPSILON 0.001
const float DRAG = -0.2;

struct Particle {
    vec4 position;
    vec4 velocity;
};

layout (std140, binding = 1) buffer ParticleSSBO {
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

    vec3 particleVelocity = vec3(particle.velocity);
    vec3 particlePosition = vec3(particle.position);

    vec3 toCenterOfGravity = centerOfGravity - particlePosition;
    float distance = max(length(toCenterOfGravity), EPSILON);

    vec3 acceleration = 300.0 * isActive * isRunning / distance * (toCenterOfGravity / distance);
    particleVelocity *= mix(1.0, exp(DRAG * dt), isRunning);

    // Euler integration.
    particlePosition += (dt * particleVelocity + 0.5 * acceleration * dt * dt) * isRunning;
    particleVelocity += acceleration * dt;

    particle.position = vec4(particlePosition, 1.0f);
    particle.velocity = vec4(particleVelocity, 1.0f);

    ssbo.particles[gl_VertexID] = particle;

    float r = 0.0045 * dot(particle.velocity, particle.velocity);
    float g = clamp(0.08 * max(particle.velocity.x, max(particle.velocity.y, particle.velocity.z)), 0.2, 0.5);
    float b = 0.7 - r;

    particleColor = vec4(r, g, b, 0.15);
    gl_Position = cameraTransform * particle.position;
}