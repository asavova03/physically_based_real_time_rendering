#include "ground.h"
#include "texture.h"
#include "light_manager.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "ibl.h"
#include "GLFW/glfw3.h"

Ground::Ground(GroundType type, const GroundMaterial &material,
               unsigned int sizeX, unsigned int sizeZ)
    : m_material(material),
      m_type(type),
      m_mesh(generateGrid(sizeX, sizeZ)) {
}

/**
 * Computes the tangents and bitangents for a given mesh.
 * This is done to convert the normals encoded in the normal map from tangent space back to world space.
 * @param mesh the mesh that we are going to do normal mapping on
 */
void computeTangents(Mesh &mesh) {
    for (const auto &tri: mesh.triangles) {
        // Get vertex indices
        unsigned int i1 = tri.x;
        unsigned int i2 = tri.y;
        unsigned int i3 = tri.z;

        // Get vertices
        Vertex &v0 = mesh.vertices[i1];
        Vertex &v1 = mesh.vertices[i2];
        Vertex &v2 = mesh.vertices[i3];

        // Edges of the triangle
        glm::vec3 edge1 = v1.position - v0.position;
        glm::vec3 edge2 = v2.position - v0.position;

        // Delta UV coordinates
        glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
        glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;

        // Calculate the fraction
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        // Tangent
        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        // Bitangent
        glm::vec3 bitangent;
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        v0.tangent = tangent;
        v1.tangent = tangent;
        v2.tangent = tangent;

        v0.bitangent = bitangent;
        v1.bitangent = bitangent;
        v2.bitangent = bitangent;
    }
}

unsigned int Ground::m_sizeX = 256;
unsigned int Ground::m_sizeZ = 256;

/**
 * Generates a flat rectangular grid used as a ground
 * @param sizeX the width in the x coordinate
 * @param sizeZ the width in the z coordinate
 * @return the ground plane
 */
GPUMesh Ground::generateGrid(unsigned int sizeX, unsigned int sizeZ) {
    m_sizeX = sizeX;
    m_sizeZ = sizeZ;
    Mesh mesh;
    mesh.vertices.reserve(sizeX * sizeZ);
    mesh.triangles.reserve((sizeX - 1) * (sizeZ - 1) * 2);

    // Generate vertices
    for (unsigned int z = 0; z < sizeZ; z++) {
        for (unsigned int x = 0; x < sizeX; x++) {
            Vertex v{};
            v.position = glm::vec3(
                static_cast<float>(x) - (sizeX - 1) / 2.0f,
                0.0f,
                static_cast<float>(z) - (sizeZ - 1) / 2.0f
            );
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v.texCoord = glm::vec2(
                static_cast<float>(x) / (sizeX - 1),
                static_cast<float>(z) / (sizeZ - 1)
            );

            mesh.vertices.push_back(v);
        }
    }

    // Generate triangles (two per square)
    for (unsigned int z = 0; z < sizeZ - 1; z++) {
        for (unsigned int x = 0; x < sizeX - 1; x++) {
            unsigned int i0 = z * sizeX + x;
            unsigned int i1 = i0 + 1;
            unsigned int i2 = i0 + sizeX;
            unsigned int i3 = i2 + 1;

            mesh.triangles.emplace_back(i0, i2, i1);
            mesh.triangles.emplace_back(i1, i2, i3);
        }
    }

    computeTangents(mesh);
    return {mesh};
}

/**
 * Renders the ground according to its type (crystal, lava, water, etc.)
 * @param shader of the ground
 * @param envCubemap the skybox
 * @param sceneColorRefraction a color buffer storing the ground surroundings (used for refractions)
 * @param sceneColorReflection a color buffer storing the ground surroundings (used for reflections)
 * @param ibl an object holding more environment maps for realistic shading according to the ambience
 * @param model the model matrix
 * @param view the view matrix
 * @param projection the projection matrix
 * @param light a light source
 * @param cameraPos the viewpoint
 * @param clipPlane if we are clipping for the water reflections and refractions
 * @param isSeabed if the ground is the seabed of the water
 * @param seabedDepth the denivelation of the seabed
 */
void Ground::draw(
    const Shader &shader,
    const Texture &envCubemap,
    const Texture &sceneColorRefraction,
    const Texture &sceneColorReflection,
    const IBL &ibl,
    const glm::mat4 &model,
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const std::vector<Light> &lights,
    const glm::vec3 &cameraPos,
    const glm::vec4 &clipPlane,
    bool isSeabed,
    float seabedDepth) {
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    shader.bind();
    glUniform1i(shader.getUniformLocation("numLights"), (int)lights.size());

    for (int i = 0; i < lights.size(); ++i) {
        std::string idx = std::to_string(i);
        glUniform3fv(shader.getUniformLocation(("lights[" + idx + "].position").c_str()), 1, glm::value_ptr(lights[i].position));
        glm::vec3 radiance = lights[i].color * lights[i].intensity;
        glUniform3fv(shader.getUniformLocation(("lights[" + idx + "].radiance").c_str()), 1, glm::value_ptr(radiance));
    }

    Texture *texIDs[] = {
        m_material.color, m_material.ao, m_material.emissive,
        m_material.height, m_material.metallic, m_material.normal,
        m_material.opacity, m_material.roughness
    };

    const char *names[] = {
        "texGroundColor", "texGroundAO", "texGroundEmissive",
        "texGroundHeight", "texGroundMetallic", "texGroundNormal",
        "texGroundOpacity", "texGroundRoughness"
    };

    for (int i = 0; i < 8; i++) {
        texIDs[i]->bind(i);
        glUniform1i(shader.getUniformLocation(names[i]), i);
    }

    envCubemap.bindCubemap();

    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.getIrradianceMap());
    glUniform1i(shader.getUniformLocation("irradianceMap"), 9);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.getPrefilterMap());
    glUniform1i(shader.getUniformLocation("prefilterMap"), 10);

    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_2D, ibl.getBRDFLUT());
    glUniform1i(shader.getUniformLocation("brdfLUT"), 11);

    glUniform1i(shader.getUniformLocation("maxMipLevels"), ibl.getMaxMipLevels());
    glm::mat4 modelGround = glm::translate(model, glm::vec3(0.0f, -0.5f * heightScale + m_height, 0.0f));
    glUniformMatrix4fv(shader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(modelGround));
    glUniformMatrix4fv(shader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(shader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(shader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));

    glUniform1i(shader.getUniformLocation("groundTiling"), tiling);
    glUniform4fv(shader.getUniformLocation("plane"), 1, glm::value_ptr(clipPlane));

    if (m_type == GroundType::WATER) {
        glUniform3fv(shader.getUniformLocation(("lightPos")), 1, glm::value_ptr(lights[0].position));
        glUniform3fv(shader.getUniformLocation(("radiance")), 1, glm::value_ptr(lights[0].color * lights[0].intensity));
        sceneColorRefraction.bind(12);
        glUniform1i(shader.getUniformLocation("sceneColorRefraction"), 12);
        sceneColorReflection.bind(13);
        glUniform1i(shader.getUniformLocation("sceneColorReflection"), 13);

        const auto time = static_cast<float>(glfwGetTime());
        glUniform1f(shader.getUniformLocation("time"), time);
        int numWaves = static_cast<int>(water.waves.size());
        glUniform1i(shader.getUniformLocation("numWaves"), numWaves);
        glUniform1f(shader.getUniformLocation("refractionDistortion"), water.refractionDistortion);
        glUniform1f(shader.getUniformLocation("waterTintStrength"), water.tintStrength);
        glUniform3fv(shader.getUniformLocation("waterTintColor"), 1, glm::value_ptr(water.tintColor));

        for (int i = 0; i < numWaves; ++i) {
            std::string idx = std::to_string(i);
            const auto &wave = water.waves[i];

            glUniform2fv(shader.getUniformLocation(("waveDir[" + idx + "]").c_str()), 1, glm::value_ptr(wave.dir));
            glUniform1f(shader.getUniformLocation(("waveWavelength[" + idx + "]").c_str()), wave.wavelength);
            glUniform1f(shader.getUniformLocation(("waveAmplitude[" + idx + "]").c_str()), wave.amplitude);
            glUniform1f(shader.getUniformLocation(("waveSpeed[" + idx + "]").c_str()), wave.speed);
            glUniform1f(shader.getUniformLocation(("waveSteepness[" + idx + "]").c_str()), wave.steepness);
        }

        glUniform1i(shader.getUniformLocation("enableFresnel"), water.enableFresnel);
        glUniform1i(shader.getUniformLocation("enableIBL"), water.enableIBL);
        glUniform1i(shader.getUniformLocation("enableReflection"), water.enableReflection);
        glUniform1i(shader.getUniformLocation("enableRefraction"), water.enableRefraction);
        glUniform1i(shader.getUniformLocation("enableSunglade"), water.enableSunglade);
    } else {
        glUniform1f(shader.getUniformLocation("heightScale"), heightScale);
        glUniform1i(shader.getUniformLocation("seabed"), isSeabed);
        glUniform1f(shader.getUniformLocation("seabedDepth"), seabedDepth);
    }

    m_mesh.draw(shader);
}

float Ground::getHeightAt(float x, float z) const {
    if (m_type == GroundType::WATER) {
        return m_height;
    }
    float u = (x / static_cast<float>(m_sizeX) + 0.5f);
    float v = (z / static_cast<float>(m_sizeZ) + 0.5f);

    u = fmod(u * tiling, 1.0f);
    v = fmod(v * tiling, 1.0f);
    if (u < 0) u += 1.0f;
    if (v < 0) v += 1.0f;

    float heightValue = m_material.height->getValueAt(u, v);
    return m_height + heightScale * (heightValue - 0.5f);
}


