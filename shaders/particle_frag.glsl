#version 410

uniform vec4 color;

uniform vec3 cameraPos;

uniform mat4 projection;
uniform mat4 view;

out vec4 outColor;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

void main()
{
    outColor = color;
}
