#version 410 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec3 pos;
in vec3 normal;
in vec2 texCoord;
in vec3 tangent;
in vec3 bitangent;

out vec3 vPos;
out vec3 vNormal;
out vec2 vTexCoord;
out mat3 vTBN;
out vec4 vClipSpaceCoord;

uniform vec4 plane;

void main()
{
    vec4 worldPos = model * vec4(pos, 1.0);

    gl_ClipDistance[0] = dot(plane, worldPos);
    vPos = worldPos.xyz;

    vNormal = normalize(mat3(model) * normal);

    vTexCoord = texCoord;

    vec3 T = normalize(mat3(model) * tangent);
    vec3 B = normalize(mat3(model) * bitangent);
    vec3 N = normalize(vNormal);
    vTBN = mat3(T, B, N);
    vec4 clipSpace = projection * view * worldPos;
    gl_Position = clipSpace;
    vClipSpaceCoord = clipSpace;
}
