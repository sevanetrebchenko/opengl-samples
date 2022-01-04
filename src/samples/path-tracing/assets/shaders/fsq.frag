
#version 450 core

in vec2 textureCoordinates;

uniform image2D image;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(texture(image, textureCoordinates).rgb, 1.0f);
}