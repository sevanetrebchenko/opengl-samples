
#version 450 core

in vec2 textureCoordinates;

layout (binding = 0, rgba32f) readonly uniform image2D finalImage;
uniform float exposure;

layout (location = 0) out vec4 fragColor;

// ACES tone mapping curve fit to go from HDR to SDR.
//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 color)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0f, 1.0f);
}

void main() {
    // textureCoordinates are in the domain [0, 1], while imageLoad takes coordinates on the domain [0, width], [0, height]
    // of the output image resolution.
    ivec2 imageResolution = imageSize(finalImage);
    vec3 color = imageLoad(finalImage, ivec2(textureCoordinates * imageResolution)).rgb;

    // Convert from HDR (unbounded) color range to SDR (standard) color range.
    color *= exposure;
    color = ACESFilm(color);

    fragColor = vec4(color, 1.0f);
}