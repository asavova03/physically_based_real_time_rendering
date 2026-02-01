#version 410

#define MAX_LIGHTS 16

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform int numLights;
uniform Light lights[MAX_LIGHTS];
//uniform samplerCube skybox;
uniform samplerCube irradianceMap;
uniform sampler2D texFlower;
uniform vec3 lightPos;
uniform vec3 pickedColor;
uniform int flowerComponent;
uniform Material {
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
};

uniform vec3 cameraPos;

uniform mat4 projection;
uniform mat4 view;

out vec4 outColor;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(cameraPos - fragPos);

    vec4 texColor = texture(texFlower, vec2(fragTexCoord.x, 1.0 - fragTexCoord.y));
    vec4 baseColor = texColor; // petals
    vec3 directLighting = vec3(0.0);

    if (flowerComponent == 1) { // center
        baseColor = vec4(pickedColor, 1.0f);
    }
    if (flowerComponent == 0) { // stem
        baseColor = vec4(pickedColor, 1.0f);
    }


    for (int i = 0; i < numLights; ++i) {
        vec3 L = normalize(lights[i].position - fragPos);
        vec3 H = normalize(V + L);

        vec3 lightColor = lights[i].color * lights[i].intensity;

        vec3 diffuse = kd * baseColor.rgb * max(dot(N, L), 0.0);
        vec3 specular = ks * pow(max(dot(H, N), 0.0), shininess);

        directLighting += (diffuse + specular) * lightColor;
    }

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 ambientLighting = irradiance * baseColor.rgb;

    vec3 color = directLighting + ambientLighting;

    outColor = vec4(color, 1.0);
}
