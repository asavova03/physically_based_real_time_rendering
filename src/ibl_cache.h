#pragma once
#include "ibl.h"
#include <unordered_map>

enum class SkyboxType {
    SUNSET = 0,
    DAY = 1,
    UNIVERSE = 2
};

class IBLManager {
public:
    ~IBLManager();

    // Loads or retrieves cached IBL for a given skybox type
    IBL& getIBL(SkyboxType type, const Texture &envCubemap);

    // Preload all IBL maps at startup
    void preloadIBL(SkyboxType type, const Texture &envCubemap);

    // For cleanup
    void clear();

private:
    std::unordered_map<SkyboxType, IBL> m_iblCache;
};
