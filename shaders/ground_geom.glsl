#version 410 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// Input from previous shader stage
in vec3 vNormal[];
in vec2 vTexCoord[];
in vec3 vPos[];
in mat3 vTBN[];

// Output to fragment shader
out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragTexCoord;
out mat3 fragTBN;

uniform sampler2D texGroundHeight;
uniform float heightScale;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform int groundTiling;
uniform bool randomTiles = false;
uniform bool seabed = false;
uniform float seabedDepth = 5.0f;

void main()
{
    for (int i = 0; i < 3; i++) {
        vec2 uv = vTexCoord[i] * groundTiling;

        float height = texture(texGroundHeight, uv).r;

        vec3 displacedPos = vPos[i] + vec3(0.0, height * heightScale, 0.0);

        if (seabed) {
            vec2 groundBlockUV = vTexCoord[i];

            float waveX = sin(groundBlockUV.x * 3.14159) * 0.5 + 0.5;
            float waveY = sin(groundBlockUV.y * 3.14159) * 0.5 + 0.5;
            float edge = min(waveX, waveY);

            float curve = smoothstep(0.0, 1.0, edge);
            float seabedCurveY = mix(seabedDepth / 2, -seabedDepth / 2, curve);

            displacedPos.y *= (1.0 - curve * 0.5);
            displacedPos.y += seabedCurveY / heightScale;
        }

        if (randomTiles) {
            vec2 groundBlockUV = vTexCoord[i];
            float distanceToEdge = min(groundBlockUV.x, min(groundBlockUV.y, min(1.0 - groundBlockUV.x, 1.0 - groundBlockUV.y)));
            float edgeFade = smoothstep(0.0, 0.01, distanceToEdge);
            displacedPos.y = mix(-1.0, displacedPos.y, edgeFade);
        }

        fragPos      = displacedPos;
        fragNormal   = normalize(vNormal[i]);
        fragTexCoord = vTexCoord[i];
        fragTBN      = vTBN[i];

        gl_Position = projection * view * vec4(displacedPos, 1.0);

        EmitVertex();
    }
    EndPrimitive();
}
