#version 450 core

#define EPSILON 0.0001

struct Particle {
    vec4 position;
    vec4 velocity;
};

layout (std140, binding = 1) buffer ParticleSSBO {
    Particle particles[];
} ssbo;

layout (location = 0) uniform mat4 cameraTransform;

layout (location = 0) out vec4 particleColor;

void main() {
    Particle particle = ssbo.particles[gl_VertexID];
    vec3 particlePosition = vec3(particle.position);

    particleColor = vec4(1.0);
    gl_Position = cameraTransform * particle.position;
}