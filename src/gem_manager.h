#pragma once

#include <vector>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "bloom_effect.h"
#include "infinite_ground.h"
#include "mesh.h"
#include "texture.h"
#include "light_manager.h"
#include "skybox_manager.h"

/**
 * Gem Types corresponds to the different shadings a gem mesh can take,
 * such as textures and emission color
 */
enum GemType {
    AMETHYST = 0,
    CITRINE = 1,
    EMERALD = 2,
    QUARTZ = 3,
    RUBY = 4,
    SAPPHIRE = 5
};

/**
 * A gem instance that has:
 * type - what parameters to pass to the fragment shaders
 * position - where is the gem
 * scale - size of the gem
 */
struct Gem {
    GemType type;
    glm::vec3 position;
    float scale;
};

/**
 * What color does the crystal emit for the glow and how strong is the emission
 */
struct GemLighting {
    glm::vec3 emissiveColor;
    float luminanceStrength;
    float lightIntensity;
};

class GemManager {
public:
    void init(const std::string &meshPath,
              SkyboxManager &skyboxManager,
              InfiniteGround &infiniteGround,
              LightManager &lightManager,
              Bloom &bloom);

    void updateBloom(Bloom &bloom);

    void updateGround(InfiniteGround &newGround);

    void updatePlayerPosition(glm::vec3 &newPos);

    void drawAll(Shader &shader,
                 const glm::mat4 &model,
                 const glm::mat4 &view,
                 const glm::mat4 &projection,
                 const glm::vec3 &cameraPos,
                 const std::vector<Light> &lights,
                 glm::vec4 clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -1e9));

    void generateGems();

    void renderUI();

private:
    std::vector<GPUMesh> m_meshes;
    std::unordered_map<GemType, Texture> m_textures;
    SkyboxManager *m_skyboxManager = nullptr;
    InfiniteGround *m_ground = nullptr;
    LightManager *m_lightManager = nullptr;
    Bloom *m_bloom = nullptr;

    int m_numGems = 0;
    float m_spawnRadius = 64.0f;
    float m_minScale = 0.1f;
    float m_maxScale = 1.0f;
    float emissiveStrength = 0.4f;
    int m_globalSeed = 0;
    glm::vec3 m_playerPos = glm::vec3(0, 0, 0);

    static uint32_t hashTileCoord(const TileCoord &coord);

    std::vector<Gem> generateGemsForGroundTile(const glm::vec3 &center, uint32_t seed);

    void loadTextures();

    void drawAllGemsOfType(GemType type, Shader &shader, const std::vector<Light> &lights,
                           const std::vector<glm::mat4> &matrices, const glm::mat4 &view,
                           const glm::mat4 &projection, const glm::vec3 cameraPos,
                           glm::vec4 clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -1e9)) const;

    const std::unordered_map<GemType, GemLighting> GEM_LIGHTING = {
        {AMETHYST, {glm::vec3(0.2f, 0.1f, 1.0f), 1.5f, 0.65f}},
        {CITRINE, {glm::vec3(0.8f, 0.4f, 0.1f), 0.5f, 0.8f}},
        {EMERALD, {glm::vec3(0.3f, 0.6f, 0.0f), 1.5f, 0.7f}},
        {QUARTZ, {glm::vec3(0.5f, 0.45f, 0.5f), 0.1f, 0.4f}},
        {RUBY, {glm::vec3(1.0f, 0.2f, 0.25f), 1.5f, 0.9f}},
        {SAPPHIRE, {glm::vec3(0.1f, 0.35f, 0.9f), 1.5f, 0.8f}}
    };

    std::unordered_map<TileCoord, std::vector<Gem> > m_tileGems;
};
