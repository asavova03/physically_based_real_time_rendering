#pragma once
#include <unordered_map>

#include "ibl.h"
#include "ibl_cache.h"
#include "texture.h"
#include "imgui/imgui.h"

class SkyboxManager {

public:
    SkyboxManager() : currentType(SkyboxType::SUNSET) {
    }

    ~SkyboxManager() {
        cubemaps.clear();
    }

    /**
     * loads all textures from memory
     */
    void initialize() {
        loadSkybox(SkyboxType::SUNSET, "resources/skybox/sunset/");
        loadSkybox(SkyboxType::DAY, "resources/skybox/day/");
        loadSkybox(SkyboxType::UNIVERSE, "resources/skybox/universe/");

        iblCache.preloadIBL(SkyboxType::SUNSET, cubemaps[SkyboxType::SUNSET]);
        iblCache.preloadIBL(SkyboxType::DAY, cubemaps[SkyboxType::DAY]);
        iblCache.preloadIBL(SkyboxType::UNIVERSE, cubemaps[SkyboxType::UNIVERSE]);
        ibl = &iblCache.getIBL(SkyboxType::SUNSET, cubemaps[SkyboxType::SUNSET]);
    }

    void setSkybox(SkyboxType type) {
        currentType = type;

        Texture& envCubemap = cubemaps[type];

        ibl = &iblCache.getIBL(type, envCubemap);
    }


    SkyboxType getCurrentType() const {
        return currentType;
    }

    Texture &getCurrentCubemap() {
        return cubemaps.at(currentType);
    }

    /**
     * User can change the sky by using the UI.
     */
    bool renderUI() {
        const char *skyboxNames[] = {"Sunset", "Day", "Universe"};
        int selectedIndex = static_cast<int>(currentType);

        if (ImGui::Combo("Skybox", &selectedIndex, skyboxNames, 3)) {
            setSkybox(static_cast<SkyboxType>(selectedIndex));
            return true;
        }
        return false;
    }

    IBL *getIBL() const { return ibl; }

private:
    /**
     * a map that links each cubemap texture with its type
     */
    std::unordered_map<SkyboxType, Texture> cubemaps;
    SkyboxType currentType;
    /**
     * Image-Based Lighting stores the additional environment textures
     * necessary for computing the ambient light for each mesh
     */
    IBL* ibl;
    IBLManager iblCache;
    void loadSkybox(SkyboxType type, const std::string &folder) {
        std::vector<std::string> faces = {
            RESOURCE_ROOT + folder + "px.png",
            RESOURCE_ROOT + folder + "nx.png",
            RESOURCE_ROOT + folder + "py.png",
            RESOURCE_ROOT + folder + "ny.png",
            RESOURCE_ROOT + folder + "pz.png",
            RESOURCE_ROOT + folder + "nz.png"
        };

        cubemaps[type].createCubemap(faces);
    }
};

