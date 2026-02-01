#include "infinite_ground.h"

#include <algorithm>
#include <random>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtc/type_ptr.inl"

void InfiniteGround::init(const GroundType &type, const GroundMaterial &mat, unsigned int tileSize, int renderRadius) {
    this->m_material = mat;
    this->m_type = type;
    this->tileSize = tileSize - 1;
    this->renderRadius = renderRadius;
    hasRandomTiles = false;
    m_templateGround = Ground(m_type, m_material, tileSize, tileSize);
}

static GroundType getRandomGroundTypeForCoord(const TileCoord &coord, const MaterialManager &materialManager) {
    const auto allTypes = materialManager.getAllGroundTypes();

    // Deterministic seed from tile coordinates
    const std::size_t seed = std::hash<int>{}(coord.x * 19349663 ^ coord.z * 73856093);
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dist(0, static_cast<int>(allTypes.size()) - 1);

    return allTypes[dist(gen)];
}

void InfiniteGround::update(const glm::vec3 &playerPos, MaterialManager &materialManager) {
    tiles.clear();
    int playerTileX = static_cast<int>(std::floor((playerPos.x + tileSize / 2) / tileSize));
    int playerTileZ = static_cast<int>(std::floor((playerPos.z + tileSize / 2) / tileSize));

    std::unordered_map<TileCoord, Ground> newTiles;

    for (int dz = -renderRadius; dz <= renderRadius; ++dz) {
        for (int dx = -renderRadius; dx <= renderRadius; ++dx) {
            TileCoord coord = {playerTileX + dx, playerTileZ + dz};
            tiles.push_back(coord);
            auto it = activeBlocks.find(coord);
            if (it != activeBlocks.end()) {
                newTiles.emplace(coord, std::move(it->second));
            } else {
                if (hasRandomTiles) {
                    GroundType randomType = getRandomGroundTypeForCoord(coord, materialManager);
                    const GroundMaterial &randomMat = materialManager.getSomeMaterial(randomType);
                    newTiles.emplace(coord, Ground(randomType, randomMat, tileSize + 1, tileSize + 1));
                } else {
                    newTiles.emplace(coord, Ground(m_type, m_material, tileSize + 1, tileSize + 1));
                }
            }
        }
    }

    activeBlocks.swap(newTiles);
}


void InfiniteGround::updateTiling(int tiling) {
    for (auto &ground: activeBlocks | std::views::values) { ground.tiling = tiling; }
}

void InfiniteGround::updateBumpiness(float bumpiness) {
    for (auto &ground: activeBlocks | std::views::values) { ground.heightScale = bumpiness; }
}

void InfiniteGround::updateType(const GroundType &newType) {
    m_type = newType;
    m_templateGround.updateType(newType);
    if (!hasRandomTiles) { for (auto &ground: activeBlocks | std::views::values) { ground.updateType(newType); } }
}

float InfiniteGround::getHeight() const {
    if (activeBlocks.empty()) return -1.0f;
    return activeBlocks.begin()->second.getHeight();
}

Ground &InfiniteGround::getCurrentGroundBlock(const glm::vec3 &playerPos) {
    if (activeBlocks.empty()) return m_templateGround;
    int playerTileX = static_cast<int>(std::floor((playerPos.x + tileSize / 2) / tileSize));
    int playerTileZ = static_cast<int>(std::floor((playerPos.z + tileSize / 2) / tileSize));
    const TileCoord currentTile = {playerTileX, playerTileZ};
    if (const auto it = activeBlocks.find(currentTile); it != activeBlocks.end()) { return it->second; }
    return m_templateGround;
}

void InfiniteGround::updateMaterial(const GroundMaterial &newMaterial, MaterialManager &materialManager) {
    m_material = newMaterial;
    m_templateGround.updateMaterial(newMaterial);

    if (hasRandomTiles) {
        for (auto &[coord, ground]: activeBlocks) {
            GroundType randomType = getRandomGroundTypeForCoord(coord, materialManager);
            const GroundMaterial &randomMat = materialManager.getSomeMaterial(randomType);
            ground.updateType(randomType);
            ground.updateMaterial(randomMat);
        }
    } else {
        for (auto &ground: activeBlocks | std::views::values) {
            ground.updateMaterial(newMaterial);
        }
    }
}

void InfiniteGround::drawSeabedAtTile(const TileCoord &coord,
                                      const glm::vec3 &cameraPos,
                                      const glm::mat4 &model,
                                      const glm::mat4 &view,
                                      const glm::mat4 &projection,
                                      const std::vector<Light> &lights,
                                      const MaterialManager &materialManager,
                                      const Shader &groundShader,
                                      const Texture &envCubemap,
                                      const Texture &sceneColorRefraction,
                                      const Texture &sceneColorReflection,
                                      const IBL &ibl,
                                      const glm::vec4 &clipPlane,
                                      float seabedDepth) {
    seabedTile.draw(groundShader,
                    envCubemap,
                    sceneColorRefraction,
                    sceneColorReflection,
                    ibl,
                    model,
                    view,
                    projection,
                    lights,
                    cameraPos,
                    clipPlane,
                    /*isSeabed=*/true,
                    seabedDepth);
}

void InfiniteGround::drawAll(Shader &groundShader,
                             Shader &waterShader,
                             const Texture &envCubemap,
                             const Texture &sceneColorRefraction,
                             const Texture &sceneColorReflection,
                             const IBL &ibl,
                             const glm::mat4 &model,
                             const glm::mat4 &view,
                             const glm::mat4 &projection,
                             const std::vector<Light> &lights,
                             const glm::vec3 &cameraPos,
                             const MaterialManager &materialManager,
                             const glm::vec4 &clipPlane,
                             bool isCurved) {
    for (auto &[coord, ground]: activeBlocks) {
        float worldX = coord.x * static_cast<float>(tileSize) * groundScale;
        float worldZ = coord.z * static_cast<float>(tileSize) * groundScale;

        glm::mat4 currentModel = glm::translate(model, glm::vec3(worldX, 0.0f, worldZ))
                                 * glm::scale(glm::mat4(1.0f), glm::vec3(groundScale));

        Shader &shader = (ground.getType() == GroundType::WATER) ? waterShader : groundShader;
        glUniform1i(groundShader.getUniformLocation("randomTiles"), hasRandomTiles);
        ground.draw(shader, envCubemap, sceneColorRefraction, sceneColorReflection, ibl, currentModel, view, projection,
                    lights, cameraPos, clipPlane, isCurved);

        if (hasRandomTiles && ground.getType() == GroundType::WATER) {
            seabedTile = Ground(GroundType::SAND, materialManager.getSomeMaterial(GroundType::SAND), tileSize + 1,
                                tileSize + 1);
            seabedTile.tiling = ground.tiling;
            drawSeabedAtTile(coord, cameraPos, currentModel, view, projection, lights, materialManager, groundShader,
                             envCubemap,
                             sceneColorRefraction, sceneColorReflection, ibl, clipPlane);
        }
    }
}


void InfiniteGround::renderUI(MaterialManager &materialManager, glm::vec3 playerPosition) {
    if (materialManager.getCurrentType() == GroundType::WATER && !hasRandomTiles) {
        if (ImGui::CollapsingHeader("Water Settings", ImGuiTreeNodeFlags_DefaultOpen)) {

            ImGui::Separator();
            ImGui::Text("General");

            ImGui::SliderFloat("Water Level", &groundBumpiness, 0.0f, 3.0f);
            updateBumpiness(groundBumpiness);

            ImGui::SliderInt("Water Texture Tiles", &groundTextureTiling, 1, 16);
            updateTiling(groundTextureTiling);

            ImGui::ColorEdit3("Water Tint Color", &water.tintColor[0]);
            ImGui::SliderFloat("Water Tint Strength", &water.tintStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Refraction Distortion Strength", &water.refractionDistortion, 0.0f, 0.5f);

            ImGui::Separator();
            if (ImGui::CollapsingHeader("Gerstner Waves")) {
                static int selectedWave = 0;

                if (ImGui::Button("Add Wave")) {
                    water.waves.push_back(GerstnerWave());
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove This Wave") && !water.waves.empty()) {
                    water.waves.erase(water.waves.begin() + selectedWave);
                    selectedWave = std::clamp(selectedWave - 1, 0, static_cast<int>(water.waves.size()) - 1);
                }

                ImGui::Text("Number of waves: %zu", water.waves.size());
                ImGui::Separator();

                if (ImGui::BeginListBox("Wave List", ImVec2(-FLT_MIN, 6 * ImGui::GetTextLineHeightWithSpacing()))) {
                    for (int i = 0; i < static_cast<int>(water.waves.size()); ++i) {
                        const bool isSelected = (selectedWave == i);
                        std::string label = "Wave " + std::to_string(i + 1);
                        if (ImGui::Selectable(label.c_str(), isSelected)) {
                            selectedWave = i;
                        }
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }

                if (!water.waves.empty()) {
                    auto &wave = water.waves[selectedWave];
                    ImGui::Separator();
                    ImGui::Text("Editing Wave %d", selectedWave + 1);

                    ImGui::SliderFloat2("Direction", glm::value_ptr(wave.dir), -1.0f, 1.0f, "%.2f");
                    wave.dir = glm::normalize(wave.dir);

                    ImGui::SliderFloat("Wavelength", &wave.wavelength, 1.0f, 100.0f, "%.2f");
                    ImGui::SliderFloat("Amplitude", &wave.amplitude, 0.0f, 2.0f, "%.3f");
                    ImGui::SliderFloat("Speed", &wave.speed, 0.0f, 5.0f, "%.2f");
                    ImGui::SliderFloat("Steepness", &wave.steepness, 0.0f, 2.0f, "%.2f");

                    ImGui::Separator();
                }
            }

            if (ImGui::CollapsingHeader("Shading Contributions")) {
                ImGui::Checkbox("IBL", &water.enableIBL);
                ImGui::Checkbox("Fresnel", &water.enableFresnel);
                ImGui::Checkbox("Reflections", &water.enableReflection);
                ImGui::Checkbox("Refractions", &water.enableRefraction);
                ImGui::Checkbox("Sunglade", &water.enableSunglade);
            }

            if (ImGui::Button("Reset to Default")) {
                water.resetToDefault();
            }
        }
        for (auto &ground: activeBlocks | std::views::values) { ground.water = water; }
    } else {
        if (ImGui::CollapsingHeader("Ground Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Bumpiness", &groundBumpiness, 0.0f, 3.0f);
            updateBumpiness(groundBumpiness);

            ImGui::SliderInt("Ground Tiles", &groundTextureTiling, 1, 16);
            updateTiling(groundTextureTiling);
        }
    }
}
