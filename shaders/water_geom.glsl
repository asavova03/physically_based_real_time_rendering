#version 410 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 vNormal[];
in vec2 vTexCoord[];
in vec3 vPos[];
in mat3 vTBN[];
in vec4 vClipSpaceCoord[];

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragTexCoord;
out mat3 fragTBN;
out vec4 fragClipSpaceCoord;

uniform mat4 view;
uniform mat4 projection;
uniform float time;

uniform sampler2D texGroundHeight;

#define MAX_WAVES 8

uniform int numWaves;                         // how many are active
uniform vec2 waveDir[MAX_WAVES];              // direction vectors
uniform float waveWavelength[MAX_WAVES];      // wavelength
uniform float waveAmplitude[MAX_WAVES];       // amplitude
uniform float waveSpeed[MAX_WAVES];           // speed
uniform float waveSteepness[MAX_WAVES];       // steepness

void gerstnerWave(in vec3 pos,
out vec3 displaced,
out vec3 dpdx,
out vec3 dpdz,
vec2 texCoord)
{
    displaced = pos;
    dpdx = vec3(1.0, 0.0, 0.0);
    dpdz = vec3(0.0, 0.0, 1.0);

    for (int i = 0; i < numWaves; i++) {
        vec2 dir = normalize(waveDir[i]);
        float k = 2.0 * 3.14159265 / waveWavelength[i];
        float c = waveSpeed[i];
        float d = dot(dir, pos.xz);
        float phase = k * d + time * c;

        float cosP = cos(phase);
        float sinP = sin(phase);

        float A = waveAmplitude[i];
        float Q = waveSteepness[i];

        // Horizontal displacement
        displaced.x += Q * A * dir.x * cosP;
        displaced.z += Q * A * dir.y * cosP;

        // Vertical displacement
        displaced.y += A * sinP;

        dpdx.x += -dir.x * dir.x * Q * A * k * sinP;
        dpdx.y +=  dir.x * A * k * cosP;
        dpdx.z += -dir.x * dir.y * Q * A * k * sinP;

        dpdz.x += -dir.y * dir.x * Q * A * k * sinP;
        dpdz.y +=  dir.y * A * k * cosP;
        dpdz.z += -dir.y * dir.y * Q * A * k * sinP;
    }

    float h = texture(texGroundHeight, texCoord).r;
}

void main()
{
    for (int i = 0; i < 3; i++) {
        vec3 displaced, dpdx, dpdz;
        gerstnerWave(vPos[i], displaced, dpdx, dpdz, vTexCoord[i]);

        fragPos = displaced;
        fragNormal = normalize(cross(dpdz, dpdx));
        fragTexCoord = vTexCoord[i];
        fragClipSpaceCoord = vClipSpaceCoord[i];
        fragTBN = vTBN[i];

        gl_Position = projection * view * vec4(displaced, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}
