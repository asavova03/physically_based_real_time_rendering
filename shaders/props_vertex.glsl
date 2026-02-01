#version 410

// Model/view/projection matrix
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Per-vertex attributes
in vec3 pos;// World-space position
in vec3 normal;// World-space normal
in vec2 texCoord;

// Data to pass to fragment shader
out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragTexCoord;

uniform vec4 plane;

void main() {
    // Transform 3D position into on-screen position
    vec4 worldPos = model * vec4(pos, 1.0);
    gl_ClipDistance[0] = dot(plane, worldPos);

    gl_Position = projection * view * model * vec4(pos, 1.0);

    // Pass position and normal through to fragment shader
    fragPos = worldPos.xyz;
    fragNormal = normal;
    fragTexCoord = texCoord;
}