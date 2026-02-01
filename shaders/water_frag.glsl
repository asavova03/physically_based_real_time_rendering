#version 410 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;
in mat3 fragTBN;
in vec4 fragClipSpaceCoord;

out vec4 outColor;

// PBR textures
uniform sampler2D texGroundColor;
uniform sampler2D texGroundAO;
uniform sampler2D texGroundHeight;
uniform sampler2D texGroundMetallic;
uniform sampler2D texGroundNormal;
uniform sampler2D texGroundOpacity;
uniform sampler2D texGroundRoughness;

// IBL environment maps
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform int maxMipLevels;
uniform sampler2D brdfLUT;

// color and depth buffers of the background scene used for refractions
uniform sampler2D sceneColorRefraction;// what pixel color should be refracted
uniform sampler2D sceneColorReflection;// what pixel color should be reflected

uniform mat4 projection;
uniform mat4 view;
uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform vec3 radiance;

uniform int groundTiling;
float ior = 1.333;
uniform float refractionDistortion = 0.25;// how much to distort the coordinates of reflections (0.01-0.1)
uniform float waterTintStrength = 0.6;// how much of the water tint into refraction
uniform vec3 waterTintColor = vec3(0.0, 0.5, 0.65);//  water color

uniform bool enableFresnel;
uniform bool enableIBL;
uniform bool enableSunglade;
uniform bool enableReflection;
uniform bool enableRefraction;

const float PI = 3.14159265359;

// Helper functions
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    cosTheta = clamp(cosTheta, 0.0, 1.0);
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float distributionGGX(float NdotH, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 1e-6);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx1 = geometrySchlickGGX(NdotV, roughness);
    float ggx2 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 cookTorranceBRDF(vec3 N, vec3 V, vec3 L, vec3 H, vec3 F0, float roughness, float metallic)
{
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float D = distributionGGX(NdotH, roughness);
    float G = geometrySmith(NdotV, NdotL, roughness);
    vec3 F = fresnelSchlick(VdotH, F0);

    float denom = 4.0 * max(NdotV * NdotL, 1e-6);
    vec3 spec = (D * G * F) / denom;
    return spec;
}

void main()
{
    // UV tiling
    vec2 uv = fragTexCoord * float(max(1, groundTiling));

    // --- Sample material maps ---
    vec3 albedo     = texture(texGroundColor, uv).rgb;
    float ao        = texture(texGroundAO, uv).r;
    float height    = texture(texGroundHeight, uv).r;
    float metallic  = texture(texGroundMetallic, uv).r;
    float roughness = texture(texGroundRoughness, uv).r;
    float opacityTex= texture(texGroundOpacity, uv).r;

    // --- Normal mapping ---
    vec3 normalTex = texture(texGroundNormal, uv).rgb * 2.0 - 1.0;
    vec3 N = normalize(fragTBN * normalTex);

    vec3 V = normalize(cameraPos - fragPos);
    vec3 L = normalize(lightPos - fragPos);
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // --- Fresnel term ---
    float f0_scalar = pow((ior - 1.0) / (ior + 1.0), 2.0);
    vec3 dielectricF0 = vec3(f0_scalar);
    vec3 F0 = mix(dielectricF0, albedo, metallic);

    vec3 F = enableFresnel ? fresnelSchlick(max(dot(N, V), 0.0), dielectricF0)
    : dielectricF0;

    // --- Cook-Torrance BRDF ---
    vec3 specularBRDF = enableSunglade ? cookTorranceBRDF(N, V, L, H, F0, roughness, metallic) : vec3(0.0);
    specularBRDF *= radiance;

    // --- Diffuse ---
    vec3 kD = (vec3(1.0) - fresnelSchlick(max(dot(H, V), 0.0), F0));
    kD *= (1.0 - metallic);
    vec3 diffuse = (kD * albedo) / PI;

    // --- Direct lighting ---
    vec3 Lo = (diffuse + specularBRDF) * radiance * NdotL;

    // --- IBL diffuse ---
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 ambientDiffuse = enableIBL ? (irradiance * albedo * ao) : vec3(0.0);

    // --- IBL specular ---
    vec3 R = reflect(-V, N);
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * maxMipLevels).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specularIBL = enableIBL ? prefilteredColor * (F0 * brdf.x + brdf.y) : vec3(0.0);

    vec3 I = normalize(fragPos - cameraPos);

    // --- Dispersion ---
    float iorR = ior;
    float iorG = ior * 0.995;
    float iorB = ior * 1.005;

    vec3 Rr = refract(I, N, 1.0 / iorR);
    vec3 Rg = refract(I, N, 1.0 / iorG);
    vec3 Rb = refract(I, N, 1.0 / iorB);

    vec3 refractPosR = fragPos + Rr * refractionDistortion;
    vec3 refractPosG = fragPos + Rg * refractionDistortion;
    vec3 refractPosB = fragPos + Rb * refractionDistortion;

    // --- Project into clip space ---
    vec4 clipR = projection * view * vec4(refractPosR, 1.0);
    vec4 clipG = projection * view * vec4(refractPosG, 1.0);
    vec4 clipB = projection * view * vec4(refractPosB, 1.0);

    vec2 uvR = (clipR.xy / clipR.w) * 0.5 + 0.5;
    vec2 uvG = (clipG.xy / clipG.w) * 0.5 + 0.5;
    vec2 uvB = (clipB.xy / clipB.w) * 0.5 + 0.5;

    uvR = clamp(uvR, vec2(0.0), vec2(1.0));
    uvG = clamp(uvG, vec2(0.0), vec2(1.0));
    uvB = clamp(uvB, vec2(0.0), vec2(1.0));

    // --- Refraction ---
    vec3 refractedColor = enableRefraction ? vec3(
    texture(sceneColorRefraction, uvR).r,
    texture(sceneColorRefraction, uvG).g,
    texture(sceneColorRefraction, uvB).b
    ) : vec3(0.0);

    float depthFactor = clamp(length(I) * 0.02, 0.0, 1.0);
    vec3 absorption = exp(-depthFactor * vec3(0.02, 0.05, 0.1));
    refractedColor *= absorption;

    refractedColor = mix(refractedColor, waterTintColor, waterTintStrength);

    // --- Reflection ---
    vec3 reflectionColor = enableReflection ? vec3(
    texture(sceneColorReflection, vec2(uvR.x, -uvR.y)).r,
    texture(sceneColorReflection, vec2(uvG.x, -uvG.y)).g,
    texture(sceneColorReflection, vec2(uvB.x, -uvB.y)).b
    ) : vec3(0.0);

    // --- Combine reflection/refraction ---
    float Fadj = enableFresnel ? F.r * (1.0 - roughness) : 0.5 * (1.0 - roughness);
    vec3 base = mix(refractedColor, reflectionColor, Fadj);

    // --- Final shading ---
    vec3 color = base * (1.0 - metallic)
    + specularBRDF
    + specularIBL * 0.5
    + (Lo * 0.5)
    + ambientDiffuse * 0.5;

    // --- Transparency ---
    float fresnelFactor = enableFresnel ? (F.r + F.g + F.b) / 3.0 : 0.0;
    float opacity = mix(opacityTex, 1.0, fresnelFactor * 0.9);

    outColor = vec4(color, opacity);
}