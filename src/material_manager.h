#pragma once
#include <ranges>
#include <unordered_map>
#include "texture.h"
#include "imgui/imgui.h"

enum class GroundType {
    CRYSTAL = 0,
    LAVA = 1,
    WATER = 2,
    SAND = 3,
    ICE = 4
};

/**
 * GroundMaterial holds pointers to all textures for shading the ground.
 * That is, every PBR texture is kept in a GroundMaterial struct.
 * It stores pointers, so that we don't pass the heavy textures by copying them.
 */
struct GroundMaterial {
    Texture *color;
    Texture *ao;
    Texture *emissive;
    Texture *height;
    Texture *metallic;
    Texture *normal;
    Texture *opacity;
    Texture *roughness;

    GroundMaterial() = default;

    GroundMaterial(Texture &c, Texture &a, Texture &e,
                   Texture &h, Texture &m, Texture &n,
                   Texture &o, Texture &r)
        : color(&c), ao(&a), emissive(&e),
          height(&h), metallic(&m), normal(&n),
          opacity(&o), roughness(&r) {
    }
};

class MaterialManager {
private:
    /**
     * a map linking each type of material with the struct holding its PBR textures, e.g., type SAND with PBR texture for rendering sand
     */
    std::unordered_map<GroundType, GroundMaterial> materials;
    /**
     * a map linking each type of material with an array of textures.
     * This is where the textures are actually stored, so that they stay in the materialManager,
     * while we pass around their references with the GroundMaterial struct.
     */
    std::unordered_map<GroundType, std::array<Texture, 8> > textureStorage;
    GroundType currentType;

public:
    MaterialManager() : currentType(GroundType::CRYSTAL) {
    }

    ~MaterialManager() {
        materials.clear();
        textureStorage.clear();
    }

    /**
     * Loads all textures from memory. Can take a while depending on the number of textures and their resolution.
     */
    void initialize() {
        loadCrystalMaterial();
        loadLavaMaterial();
        loadWaterMaterial();
        loadSandMaterial();
        loadIceMaterial();
    }

    void setGroundType(GroundType type) {
        currentType = type;
    }

    const GroundMaterial &getCurrentMaterial() const {
        return materials.at(currentType);
    }

    const GroundMaterial &getSomeMaterial(GroundType type) const {
        return materials.at(type);
    }

    GroundType getCurrentType() const {
        return currentType;
    }

    std::vector<GroundType> getAllGroundTypes() const {
        std::vector<GroundType> result;
        result.reserve(materials.size());
        for (const auto &key: materials | std::views::keys)
            result.push_back(key);
        return result;
    }

    void renderUI() {
        const char *materialNames[] = {"Crystal", "Lava", "Water", "Sand", "Ice"};
        int selectedIndex = static_cast<int>(currentType);

        if (ImGui::Combo("Ground Material", &selectedIndex, materialNames, materials.size())) {
            setGroundType(static_cast<GroundType>(selectedIndex));
        }
    }

private:
    /**
     * Helper functions for loading the textures from memory.
     * Since not all materials have the same types of textures, when a PBR lacks a certain texture type,
     * we use a fallback texture which is one pixel of a certain color,
     * e.g, if there is no emissive texture we store a black color meaning the texture is not emissing anything.
     */
    void loadLavaMaterial() {
        auto &textures = textureStorage[GroundType::LAVA];

        textures[0].loadTexture(RESOURCE_ROOT "resources/textures/lava/lava_color.png");
        textures[1].loadTexture(RESOURCE_ROOT "resources/textures/lava/lava_ao.png");
        textures[2].loadTexture(RESOURCE_ROOT "resources/textures/lava/lava_emissive.png");
        textures[3].keepCPUData(true);
        textures[3].loadTexture(RESOURCE_ROOT "resources/textures/lava/lava_height.png");
        textures[4].loadTexture(RESOURCE_ROOT "resources/textures/lava/lava_metalic.png");
        textures[5].loadTexture(RESOURCE_ROOT "resources/textures/lava/lava_normal.png");
        textures[6].loadTexture(RESOURCE_ROOT "resources/textures/lava/lava_opacity.png");
        textures[7].loadTexture(RESOURCE_ROOT "resources/textures/lava/lava_roughness.png");

        materials[GroundType::LAVA] = GroundMaterial(
            textures[0], textures[1], textures[2], textures[3],
            textures[4], textures[5], textures[6], textures[7]
        );
    }

    void loadCrystalMaterial() {
        auto &textures = textureStorage[GroundType::CRYSTAL];

        textures[0].loadTexture(RESOURCE_ROOT "resources/textures/crystal/crystal_color.png");
        textures[1].loadTexture(RESOURCE_ROOT "resources/textures/crystal/crystal_ao.png");
        textures[2].loadTexture(RESOURCE_ROOT "resources/textures/crystal/crystal_emission.png");
        textures[3].keepCPUData(true);
        textures[3].loadTexture(RESOURCE_ROOT "resources/textures/crystal/crystal_height.png");
        textures[4].loadTexture(RESOURCE_ROOT "resources/textures/crystal/crystal_metallic.png");
        textures[5].loadTexture(RESOURCE_ROOT "resources/textures/crystal/crystal_normal.png");
        textures[6].createFallback(255);
        textures[7].loadTexture(RESOURCE_ROOT "resources/textures/crystal/crystal_roughness.png");

        materials[GroundType::CRYSTAL] = GroundMaterial(
            textures[0], textures[1], textures[2], textures[3],
            textures[4], textures[5], textures[6], textures[7]
        );
    }

    void loadWaterMaterial() {
        auto &textures = textureStorage[GroundType::WATER];

        textures[0].loadTexture(RESOURCE_ROOT "resources/textures/water/water_color.png");
        textures[1].loadTexture(RESOURCE_ROOT "resources/textures/water/water_ao.png");
        textures[2].createFallback();
        textures[3].loadTexture(RESOURCE_ROOT "resources/textures/water/water_height.png");
        textures[4].createFallback();
        textures[5].loadTexture(RESOURCE_ROOT "resources/textures/water/water_normal.png");
        textures[6].createFallback(255);
        textures[7].loadTexture(RESOURCE_ROOT "resources/textures/water/water_roughness.png");
        materials[GroundType::WATER] = GroundMaterial(
            textures[0], textures[1], textures[2], textures[3],
            textures[4], textures[5], textures[6], textures[7]
        );
    }

    void loadSandMaterial() {
        auto &textures = textureStorage[GroundType::SAND];

        textures[0].loadTexture(RESOURCE_ROOT "resources/textures/sand/sand_color.png");
        textures[1].loadTexture(RESOURCE_ROOT "resources/textures/sand/sand_ao.png");
        textures[2].createFallback();
        textures[3].keepCPUData(true);
        textures[3].loadTexture(RESOURCE_ROOT "resources/textures/sand/sand_height.png");
        textures[4].loadTexture(RESOURCE_ROOT "resources/textures/sand/sand_metallic.png");
        textures[5].loadTexture(RESOURCE_ROOT "resources/textures/sand/sand_normal.png");
        textures[6].createFallback(255);
        textures[7].loadTexture(RESOURCE_ROOT "resources/textures/sand/sand_roughness.png");
        materials[GroundType::SAND] = GroundMaterial(
            textures[0], textures[1], textures[2], textures[3],
            textures[4], textures[5], textures[6], textures[7]
        );
    }

    void loadIceMaterial() {
        auto &textures = textureStorage[GroundType::ICE];

        textures[0].loadTexture(RESOURCE_ROOT "resources/textures/ice/ice_color.png");
        textures[1].loadTexture(RESOURCE_ROOT "resources/textures/ice/ice_ao.png");
        textures[2].createFallback(0);
        textures[3].keepCPUData(true);
        textures[3].loadTexture(RESOURCE_ROOT "resources/textures/ice/ice_height.png");
        textures[4].createFallback(128);
        textures[5].loadTexture(RESOURCE_ROOT "resources/textures/ice/ice_normal.png");
        textures[6].loadTexture(RESOURCE_ROOT "resources/textures/ice/ice_transmissive.png");
        textures[7].loadTexture(RESOURCE_ROOT "resources/textures/ice/ice_roughness.png");

        materials[GroundType::ICE] = GroundMaterial(
            textures[0], textures[1], textures[2], textures[3],
            textures[4], textures[5], textures[6], textures[7]
        );
    }
};
