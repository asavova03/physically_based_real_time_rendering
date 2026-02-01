#version 410

#define MAX_LIGHTS 16

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform int numLights;
uniform Light lights[MAX_LIGHTS];

uniform sampler2D sceneColor;// color buffer of the surrondings of the crystal guy, used to compute refractions

uniform float ior;
uniform float emissionScale;
uniform float colorTransformation;
uniform sampler2D texCrystal;
uniform samplerCube irradianceMap;
uniform vec3 lightPos;

uniform Material {
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
};
uniform vec3 cameraPos;

uniform mat4 projection;
uniform mat4 view;
uniform float baseWeight = 0.7f;
uniform float emissiveWeight = 1.0f;
uniform float refractedWeight = 0.3f;

out vec4 outColor;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(cameraPos - fragPos);
    vec4 texColor = texture(texCrystal, vec2(fragTexCoord.x, 1.0 - fragTexCoord.y));
    vec3 directLighting = vec3(0.0);

    for (int i = 0; i < numLights; ++i) {
        vec3 L = normalize(lights[i].position - fragPos);
        vec3 H = normalize(V + L);

        vec3 lightColor = lights[i].color * lights[i].intensity;
        vec3 diffuse = kd * texColor.rgb * max(dot(N, L), 0.0);
        vec3 specular = ks * pow(max(dot(H, N), 0.0), shininess);

        directLighting += (diffuse + specular) * lightColor;
    }

    // transform crystal color
    if (colorTransformation >= 0.0 && colorTransformation < 1.0) {
        // green crystals
        texColor.rgb = colorTransformation * texColor.grb + (1.0 - colorTransformation) * texColor.rgb;
    }
    else if (colorTransformation < 0.0 && colorTransformation >= -1.0) {
        // purple crystalls
        texColor.rgb = -colorTransformation * texColor.gbr + (1.0 + colorTransformation) * texColor.rgb;
    }
    else {
        // blue crystals
        texColor.rgb = (colorTransformation - 1.0) * texColor.bgr + (2.0 - colorTransformation) * texColor.grb;
    }

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 ambientLighting = irradiance * texColor.rgb;

    vec3 baseColor = directLighting + ambientLighting;

    // ---------------------------
    // Depth-based refraction
    // ---------------------------
    vec3 I = normalize(fragPos - cameraPos);

    float iorR = ior;
    float iorG = ior * 0.99;
    float iorB = ior * 1.01;

    vec3 Rr = refract(I, N, 1.0 / iorR);
    vec3 Rg = refract(I, N, 1.0 / iorG);
    vec3 Rb = refract(I, N, 1.0 / iorB);

    vec4 clipPos = projection * view * vec4(fragPos, 1.0);
    vec3 ndc = clipPos.xyz / clipPos.w;
    vec2 uv = ndc.xy * 0.5 + 0.5;

    float scale = (ior - 1.0) * 0.05;
    vec2 uvR = uv + Rr.xy * scale;
    vec2 uvG = uv + Rg.xy * scale;
    vec2 uvB = uv + Rb.xy * scale;

    vec3 refractedColor = vec3(
    texture(sceneColor, uvR).r,
    texture(sceneColor, uvG).g,
    texture(sceneColor, uvB).b
    );

    // ---------------------------
    // Emissive term (mask brighter texCrystal pixels)
    // ---------------------------
    float brightness = dot(texColor.rgb, vec3(0.2126, 0.7152, 0.0722));// luminance
    vec3 emissive = texColor.rgb * smoothstep(0.0, 0.2, brightness) * emissionScale;

    // ---------------------------
    // Final color composition
    // ---------------------------
    vec3 finalColor = baseColor * baseWeight + refractedColor * refractedWeight + emissive * emissiveWeight;

    outColor = vec4(finalColor, 1.0);
}
