#version 410

#define MAX_LIGHTS 16

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform int numLights;
uniform Light lights[MAX_LIGHTS];
uniform samplerCube irradianceMap;
uniform sampler2D textureColor;
uniform vec3 lightPos;
uniform Material {
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
};

uniform vec3 emissiveColor;
uniform float emissiveStrength;
uniform float luminanceStrength;

uniform vec3 cameraPos;

uniform mat4 projection;
uniform mat4 view;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 BrightColor;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(cameraPos - fragPos);

    vec4 texColor = texture(textureColor, vec2(fragTexCoord.x, 1.0 - fragTexCoord.y));
    vec3 directLighting = vec3(0.0);

    for (int i = 0; i < numLights; ++i) {
        vec3 L = normalize(lights[i].position - fragPos);
        vec3 H = normalize(V + L);

        vec3 lightColor = lights[i].color * lights[i].intensity;
        vec3 diffuse = kd * texColor.rgb * max(dot(N, L), 0.0);
        vec3 specular = ks * pow(max(dot(H, N), 0.0), shininess);

        directLighting += (diffuse + specular) * lightColor;
    }

    float luminance = dot(directLighting, vec3(0.2126, 0.7152, 0.0722));
    vec3 emission = texColor.rgb * luminance * luminanceStrength + emissiveColor * emissiveStrength;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 ambientLighting = irradiance * texColor.rgb * 0.8;

    vec3 color = directLighting + ambientLighting + emission;

    outColor = vec4(color, 0.8);
    BrightColor = vec4(emissiveColor, 1.0);
//    BrightColor = luminance > 0.2 ? vec4(color, 1.0) : vec4(0.0);
}
