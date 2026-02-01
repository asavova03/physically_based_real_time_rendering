#version 410 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float bloomStrength;

void main() {
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    vec3 result = hdrColor + bloomColor * bloomStrength;
    FragColor = vec4(result, 1.0);
}
