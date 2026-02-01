#include "ibl_cache.h"

IBLManager::~IBLManager() {
    clear();
}

void IBLManager::clear() {
    m_iblCache.clear();
}

void IBLManager::preloadIBL(const SkyboxType type, const Texture& envCubemap) {
    if (m_iblCache.contains(type))
        return;

    m_iblCache[type].init(envCubemap.id());
    m_iblCache[type].generateIrradianceMap();
    m_iblCache[type].generatePrefilterMap();
    m_iblCache[type].generateBRDFLUT();
}

IBL& IBLManager::getIBL(const SkyboxType type, const Texture& envCubemap) {
    if (!m_iblCache.contains(type))
        preloadIBL(type, envCubemap);

    return m_iblCache.at(type);
}
