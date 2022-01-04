
#version 450 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 uvCoordinates;

out vec2 textureCoordinates;

void main() {
    textureCoordinates = uvCoordinates;
    gl_Position = vec4(vertexPosition, 1.0f);
}