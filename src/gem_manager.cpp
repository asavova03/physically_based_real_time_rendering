#include "gem_manager.h"

#include <algorithm>
#include <imgui/imgui.h>

#include "helpers.h"
#include "material_manager.h"
#include "glm/gtc/type_ptr.inl"

/**
 * Initialize the gem manager
 * @param meshPath the file location of the gem mesh
 * @param skyboxManager the environment of the gems
 * @param infiniteGround the ground of the gems
 * @param lightManager the surrounding lights
 * @param bloom the handler for the blooming of the gems
 */
void GemManager::init(const std::string &meshPath,
                      SkyboxManager &skyboxManager,
                      InfiniteGround &infiniteGround,
                      LightManager &lightManager,
                      Bloom &bloom) {
    m_meshes = GPUMesh::loadMeshGPU(meshPath.c_str());
    m_skyboxManager = &skyboxManager;
    m_ground = &infiniteGround;
    m_lightManager = &lightManager;
    m_bloom = &bloom;
    loadTextures();
    generateGems();
}

/**
 * Loads the textures for all gem types
 */
void GemManager::loadTextures() {
    auto loadTex = [&](GemType type, std::string path) {
        m_textures[type].loadTexture(path.c_str(), GL_CLAMP_TO_EDGE);
    };

    loadTex(AMETHYST,
            RESOURCE_ROOT "resources/textures/gems/crystals_amethyst_color.png"
    );

    loadTex(CITRINE,
            RESOURCE_ROOT "resources/textures/gems/crystals_citrine_color.png"
    );

    loadTex(EMERALD,
            RESOURCE_ROOT "resources/textures/gems/crystals_emerald_color.png"
    );

    loadTex(QUARTZ,
            RESOURCE_ROOT "resources/textures/gems/crystals_quartz_color.png"
    );

    loadTex(RUBY,
            RESOURCE_ROOT "resources/textures/gems/crystals_ruby_color.png"
    );

    loadTex(SAPPHIRE,
            RESOURCE_ROOT "resources/textures/gems/crystals_sapphire_color.png"
    );
}

/**
 * Update the bloom object holding the framebuffers (when resizing the screen)
 * @param bloom the new bloom handler
 */
void GemManager::updateBloom(Bloom &bloom) {
    m_bloom = &bloom;
}

/**
 * When ground changes, we need to update it through this method
 * @param newGround the new updated ground
 */
void GemManager::updateGround(InfiniteGround &newGround) {
    m_ground = &newGround;
}

/**
 * When ground changes, we need to update it through this method
 * @param newPos the new updated player position
 */
void GemManager::updatePlayerPosition(glm::vec3 &newPos) {
    m_playerPos = newPos;
}

/**
 * Map each coordinate to a numeric value
 * @param coord the coordinate
 * @return a hashed index of the coordate
 */
uint32_t GemManager::hashTileCoord(const TileCoord &coord) {
    uint32_t h1 = std::hash<int>{}(coord.x);
    uint32_t h2 = std::hash<int>{}(coord.z);
    return h1 * 31u + h2;
}

/**
 * Generate gems for a given ground tile using randomization of the size, position, and gem type
 * @param center the center of the ground tile
 * @param seed the randomization seed for reproducibility
 * @return a list of generated gem instances
 */
std::vector<Gem> GemManager::generateGemsForGroundTile(const glm::vec3 &center, uint32_t seed) {
    m_lightManager->reset();
    std::vector<Gem> gems;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> distPos(-m_spawnRadius, m_spawnRadius);
    std::uniform_real_distribution<float> distScale(m_minScale, m_maxScale);
    std::uniform_int_distribution<int> distType(0, 5);

    for (int i = 0; i < m_numGems; ++i) {
        Gem gem;
        float y = 0.0;
        if (m_ground->getType() == GroundType::WATER && !m_ground->hasRandomTiles) {
            y -= 5.0f; // place crystals on seabed
        } else if (m_ground->getType() == GroundType::WATER) {
            y -= 1.5f; // place crystals on seabed (water is shallow)
        }
        gem.position = center + glm::vec3(distPos(rng), y, distPos(rng));
        gem.scale = distScale(rng);
        gem.type = static_cast<GemType>(distType(rng));
        gems.push_back(gem);
    }
    return gems;
}

/**
 * Procedural generation of gems.
 * Gems are generated using the tiles of the infinite ground class to track the position of the player.
 * A seed is generated based on the hashing of the tile coordinates so the same place always has the same crystals,
 * regardless of when the player visited it.
 */
void GemManager::generateGems() {
    if (m_numGems == 0) return;

    auto isTileVisible = [&](const TileCoord& coord) {
        for (const auto& visible : m_ground->tiles) {
            if (visible == coord)
                return true;
        }
        return false;
    };

    // Remove gems that belong to ground tiles that are no longer visible
    for (auto it = m_tileGems.begin(); it != m_tileGems.end(); ) {
        if (!isTileVisible(it->first)) {
            it = m_tileGems.erase(it);  // tile is gone, free its gems
        } else {
            ++it;
        }
    }

    for (const auto &coord: m_ground->tiles) {
        // If the tile is saved, skip it because we have already generated crystals for it
        if (m_tileGems.contains(coord)) continue;

        uint32_t seed = hashTileCoord(coord) ^ m_globalSeed;
        glm::vec3 tileCenter = glm::vec3(
            (coord.x + 0.5) * m_ground->tileSize,
            m_ground->getHeight(),
            (coord.z + 0.5) * m_ground->tileSize
        );

        std::vector<Gem> gems = generateGemsForGroundTile(tileCenter, seed);
        m_tileGems.emplace(coord, std::move(gems));
    }

    // std::vector<const Gem*> allGems;
    // allGems.reserve(m_tileGems.size() * m_numGems);
    // m_lightManager->reset();
    //
    // for (const auto& [coord, gems] : m_tileGems) {
    //     for (const auto& gem : gems)
    //         allGems.push_back(&gem);
    // }
    //
    // std::sort(allGems.begin(), allGems.end(), [&](const Gem* a, const Gem* b) {
    //     float da = glm::length(a->position - m_playerPos);
    //     float db = glm::length(b->position - m_playerPos);
    //     return da < db;
    // });
    //
    //
    // // Add lights for the 5 closest gems (or fewer if less than 5)
    // size_t numLights = std::min<size_t>(5, allGems.size());
    // for (size_t i = 0; i < numLights; i++) {
    //     const Gem* gem = allGems[i];
    //     const GemLighting &lighting = GEM_LIGHTING.at(gem->type);
    //     m_lightManager->addLight(Light(gem->position, lighting.emissiveColor, lighting.lightIntensity));
    // }
}

/**
 * Draws all gems of a given type using instanced rendering for improved performance
 * @param type the gem type to draw
 * @param shader the shader
 * @param lights lights in the scene
 * @param matrices the positions of the gems
 * @param view the view matrix
 * @param projection the projection matrix
 * @param cameraPos the position of the camera
 * @param clipPlane clipping plane (relevant only for water rendering)
 */
void GemManager::drawAllGemsOfType(GemType type, Shader &shader, const std::vector<Light> &lights,
                         const std::vector<glm::mat4> &matrices, const glm::mat4 &view,
                         const glm::mat4 &projection, const glm::vec3 cameraPos, glm::vec4 clipPlane) const {
    shader.bind();
    m_textures.at(type).bind(0);
    glUniform1i(shader.getUniformLocation("textureColor"), 0);
    glUniform1i(shader.getUniformLocation("numLights"), (int) lights.size());
    for (int i = 0; i < lights.size(); i++) {
        std::string idx = std::to_string(i);
        glUniform3fv(shader.getUniformLocation(("lights[" + idx + "].position").c_str()), 1,
                     glm::value_ptr(lights[i].position));
        glUniform3fv(shader.getUniformLocation(("lights[" + idx + "].color").c_str()), 1,
                     glm::value_ptr(lights[i].color));
        glUniform1f(shader.getUniformLocation(("lights[" + idx + "].intensity").c_str()), lights[i].intensity);
    }
    const GemLighting &lighting = GEM_LIGHTING.at(type);

    glUniform3fv(shader.getUniformLocation("emissiveColor"), 1, glm::value_ptr(lighting.emissiveColor));
    glUniform1f(shader.getUniformLocation("emissiveStrength"), emissiveStrength);
    glUniform1f(shader.getUniformLocation("luminanceStrength"), lighting.luminanceStrength);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxManager->getIBL()->getIrradianceMap());
    glUniform1i(shader.getUniformLocation("irradianceMap"), 1);
    glUniformMatrix4fv(shader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(shader.getUniformLocation("projection"), 1, GL_FALSE,
                       glm::value_ptr(projection));
    glUniform3fv(shader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
    glUniform4fv(shader.getUniformLocation("plane"), 1, glm::value_ptr(clipPlane));
    m_meshes[0].drawInstanced(shader, matrices);
}

/**
 * Renders the gems to the scene.
 * @param shader the shader
 * @param lights lights in the scene
 * @param view the view matrix
 * @param projection the projection matrix
 * @param cameraPos the position of the camera
 * @param clipPlane clipping plane (relevant only for water rendering)
 */
void GemManager::drawAll(Shader &shader,
                         const glm::mat4 &model,
                         const glm::mat4 &view,
                         const glm::mat4 &projection,
                         const glm::vec3 &cameraPos,
                         const std::vector<Light> &lights,
                         glm::vec4 clipPlane) {
    if (m_tileGems.empty()) return;

    std::unordered_map<GemType, std::vector<glm::mat4> > instanceMatrices;

    for (const auto &gems: m_tileGems | std::views::values) {
        for (const auto &gem: gems) {
            glm::mat4 transform = glm::translate(model, gem.position);
            transform = glm::scale(transform, glm::vec3(gem.scale));
            instanceMatrices[gem.type].push_back(transform);
        }
    }

    for (auto &[type, matrices]: instanceMatrices) {
        if (matrices.empty()) continue;
        drawAllGemsOfType(type, shader, lights, matrices, view, projection, cameraPos, clipPlane);
    }
}

/**
 * UI controllers that affect gem generation and appearance
 */
void GemManager::renderUI() {
    if (ImGui::CollapsingHeader("Gem Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderInt("Number of Gems", &m_numGems, 0, 500);
        ImGui::SliderInt("Randomization Seed", &m_globalSeed, 0, 100);
        ImGui::SliderFloat("Gem Emission Strength", &emissiveStrength, 0.0f, 1.0f);
        m_bloom->renderUI();


        if (ImGui::Button("Regenerate Gems")) {
            m_tileGems.clear();
            generateGems();
        }

        int totalGems = 0;
        for (const auto &gems: m_tileGems | std::views::values)
            totalGems += static_cast<int>(gems.size());

        ImGui::Text("Active Gems: %d", totalGems);
    }
}
