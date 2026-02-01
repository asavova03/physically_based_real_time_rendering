#version 410

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in mat4 instanceModel;

uniform vec4 plane;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragTexCoord;

void main() {
    mat4 model = instanceModel;
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_ClipDistance[0] = dot(plane, worldPos);

    fragPos = vec3(worldPos);
    fragNormal = mat3(transpose(inverse(model))) * aNormal;
    fragTexCoord = aTexCoord;
    gl_Position = projection * view * worldPos;
}
