#version 410 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D image;
uniform bool horizontal;
uniform float sigma = 10.0;      // controls blur radius/smoothness
uniform int radius = 40;     // Blur radius the larger the softer, wider bloom
const int MAX_RADIUS = 100;

void main() {
    vec2 tex_offset = 1.0 / textureSize(image, 0);

    float weights[MAX_RADIUS + 1];
    float sum = 0.0;
    for (int i = 0; i <= radius; ++i) {
        float x = float(i);
        weights[i] = exp(-(x * x) / (2.0 * sigma * sigma));
        sum += (i == 0) ? weights[i] : 2.0 * weights[i];
    }
    for (int i = 0; i <= radius; ++i) {
        weights[i] /= sum;
    }

    vec3 result = texture(image, TexCoords).rgb * weights[0];
    for (int i = 1; i <= radius; ++i) {
        vec2 offset = horizontal
        ? vec2(tex_offset.x * float(i), 0.0)
        : vec2(0.0, tex_offset.y * float(i));

        result += texture(image, TexCoords + offset).rgb * weights[i];
        result += texture(image, TexCoords - offset).rgb * weights[i];
    }

    FragColor = vec4(result, 1.0);
}
