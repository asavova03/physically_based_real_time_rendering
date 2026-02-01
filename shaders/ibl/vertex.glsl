#version 410 core
in vec3 pos;

out vec3 worldPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    worldPos = pos;
    gl_Position =  projection * view * vec4(worldPos, 1.0);
}
