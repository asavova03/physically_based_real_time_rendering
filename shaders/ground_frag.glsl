#version 410 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;
in mat3 fragTBN;

out vec4 outColor;

#define MAX_LIGHTS 16

struct Light {
    vec3 position;
    vec3 radiance;
};

uniform int numLights;
uniform Light lights[MAX_LIGHTS];

// PBR Textures
uniform sampler2D texGroundColor;
uniform sampler2D texGroundAO;
uniform sampler2D texGroundEmissive;
uniform sampler2D texGroundHeight;
uniform sampler2D texGroundMetallic;
uniform sampler2D texGroundNormal;
uniform sampler2D texGroundOpacity;
uniform sampler2D texGroundRoughness;

// Environment / IBL
uniform samplerCube skybox;// environment map for ambient and specular approx
// Diffuse IBL. This is a blurred version of the environment.
uniform samplerCube irradianceMap;

// Specular IBL. This is environment map with mip levels for roughness.
// The idea is that smooth surface mirrors clearly while rough surface blurs the reflection.
uniform samplerCube prefilterMap;
// Number of mip levels in the prefiltered environment cubemap
uniform int maxMipLevels;

// BRDF integration lookup table (2D texture)
uniform sampler2D brdfLUT;

// Tiling of the texture (how many textures do we render on the entire grid)
uniform int groundTiling;
// Light and camera positions in world space
//uniform vec3 lightPos;
uniform vec3 cameraPos;
//uniform vec3 radiance;

const float PI = 3.14159265359;

// --------------------------------------------
// Helper functions for the Cook-Torrance model
// --------------------------------------------

// Schlick Fresnel
// F = F0 + (1 - F0) * (1 - cosTheta)^5
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    float ct = clamp(cosTheta, 0.0, 1.0);
    return F0 + (1.0 - F0) * pow(1.0 - ct, 5.0);
}

// Normal Distribution Function GGX
float distributionGGX(float NdotH, float roughness)
{
    // alpha = roughness^2
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 1e-6);// avoid division by zero
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    // k = (alpha + 1)^2 / 8 (UE4 style) provides energy-conserving smooth interpolation
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith geometry term
float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx1 = geometrySchlickGGX(NdotV, roughness);
    float ggx2 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Cook-Torrance BRDF
// Returns specular color for given N, L, V, H, material textures
vec3 cookTorranceBRDF(vec3 N, vec3 V, vec3 L, vec3 H, vec3 F0, float roughness, float metallic)
{
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // Normal distribution
    float D = distributionGGX(NdotH, roughness);

    // Geometry
    float G = geometrySmith(NdotV, NdotL, roughness);

    // Fresnel
    vec3 F = fresnelSchlick(VdotH, F0);

    // Cook-Torrance specular = numerator / denominator
    float denom = 4.0 * max(NdotV * NdotL, 1e-6);
    vec3 spec = (D * G * F) / denom;

    return spec;
}

void main()
{
    // Calculate uv for tiled textures
    vec2 uv = fragTexCoord * groundTiling;

    // Sample textures
    vec3 albedo     = texture(texGroundColor, uv).rgb;
    float ao        = texture(texGroundAO, uv).r;
    vec3 emissive   = texture(texGroundEmissive, uv).rgb;
    float height    = texture(texGroundHeight, uv).r;// used in geom shader
    float metallic  = texture(texGroundMetallic, uv).r;
    float roughness = texture(texGroundRoughness, uv).r;
    float opacity   = texture(texGroundOpacity, uv).r;

    // --------------
    // Normal mapping
    // --------------
    // Convert from [0, 1] to [-1, 1]
    vec3 normalTex = texture(texGroundNormal, uv).rgb * 2.0 - 1.0;
    // From tangent space (the texture stores the normals in tangent space) to world space
    vec3 N = normalize(fragTBN * normalTex);

    vec3 V = normalize(cameraPos - fragPos);
    float NdotV = max(dot(N, V), 0.0);
    vec3 Lo = vec3(0.0);
    vec3 F0 = vec3(0.0);

    // Direct lighting contribution for multiple lights
    for (int i = 0; i < numLights; ++i)
    {
        vec3 L = normalize(lights[i].position - fragPos);
        vec3 H = normalize(V + L);

        float NdotL = max(dot(N, L), 0.0);
        // Using IOR of 1.6 for aquamarine gemstone, F0 = [(n - 1) / (n + 1)]^2 = 5.325%.
        F0 = mix(vec3(0.05325), albedo, metallic);

        // -------------
        // Specular term
        // -------------
        vec3 specularBRDF = cookTorranceBRDF(N, V, L, H, F0, roughness, metallic);
        // ------------
        // Diffuse term
        // ------------
        vec3 kD = (1.0 - fresnelSchlick(max(dot(H, V), 0.0), F0)) * (1.0 - metallic);
        // Lambertian diffuse using PI
        vec3 diffuse = (kD * albedo) / PI;

        // add single light contribution
        Lo += (diffuse + specularBRDF) * lights[i].radiance * NdotL;
    }

    // ------------
    // Ambient term
    // ------------
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 ambientDiffuse = irradiance * albedo * ao;
    vec3 R = reflect(-V, N);
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * maxMipLevels).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specularIBL = prefilteredColor * (F0 * brdf.x + brdf.y);
    vec3 ambient = ambientDiffuse + specularIBL * ao;

    // Final color
    vec3 color = Lo + ambient + emissive;

    outColor = vec4(color, opacity);
}
