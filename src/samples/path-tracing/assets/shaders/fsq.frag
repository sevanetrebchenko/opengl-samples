
#version 450 core

in vec2 textureCoordinates;

layout (binding = 0, rgba32f) readonly uniform image2D finalImage;

layout (location = 0) out vec4 fragColor;

void main() {
    // textureCoordinates are in the domain [0, 1], while imageLoad takes coordinates on the domain [0, width], [0, height]
    // of the output image resolution.
    ivec2 imageResolution = imageSize(finalImage);
    fragColor = vec4(imageLoad(finalImage, ivec2(textureCoordinates * imageResolution)).rgb, 1.0f);
}