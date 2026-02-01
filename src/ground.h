#pragma once
#include <glm/glm.hpp>
#include "ibl.h"
#include "texture.h"
#include "mesh.h"
#include "light_manager.h"
#include "material_manager.h"

struct GerstnerWave {
    glm::vec2 dir = glm::vec2(1.0f, 0.2f);
    float wavelength = 12.0f;
    float amplitude = 0.25f;
    float speed = 1.0f;
    float steepness = 0.6f;
};

struct WaterSettings {
    std::vector<GerstnerWave> waves;
    float refractionDistortion;
    float tintStrength;
    glm::vec3 tintColor;
    bool enableFresnel;
    bool enableIBL;
    bool enableReflection;
    bool enableRefraction;
    bool enableSunglade;

    WaterSettings() {
        resetToDefault();
    }

    void resetToDefault() {
        waves = {
            {{1.0f, 0.2f}, 12.0f, 0.25f, 1.0f, 0.6f},
            {{0.8f, -0.6f}, 20.0f, 0.20f, 0.8f, 0.4f},
            {{-0.4f, 1.0f}, 35.0f, 0.15f, 0.5f, 0.3f},
            {{0.6f, 0.8f}, 50.0f, 0.10f, 0.3f, 0.2f}
        };
        refractionDistortion = 0.1f;
        tintStrength = 0.6f;
        tintColor = glm::vec3(0.0f, 0.25f, 0.33f);

        enableFresnel = true;
        enableIBL = true;
        enableReflection = true;
        enableRefraction = true;
        enableSunglade = true;
    }
};

class Ground {
public:
    Ground() = default;

    Ground(GroundType type, const GroundMaterial &material, unsigned int sizeX = 256, unsigned int sizeZ = 256);

    void updateMaterial(const GroundMaterial &newMaterial) {
        m_material = newMaterial;
    }

    void updateType(const GroundType &newType) {
        m_type = newType;
    }

    GroundType getType() {
        return m_type;
    }

    float getHeight() const { return m_height; }

    void draw(
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
        const glm::vec4 &clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -1e9),
        bool isSeabed = false,
        float seabedDepth = 5.0f
    );

    float getHeightAt(float x, float z) const;

    glm::vec3 kd{0.7f};
    glm::vec3 ks{0.5f};
    /**
     * a parameter used to scale the elevation of vertices depending on the height map passed to the geom shader
     */
    float heightScale = 0.5f;
    /**
     * a parameter regulating how many times the texture is tiled to fill the ground
     */
    int tiling = 8;

    WaterSettings water;

private:
    static GPUMesh generateGrid(unsigned int sizeX, unsigned int sizeZ);

    GroundMaterial m_material{};
    GroundType m_type = GroundType::CRYSTAL;
    GPUMesh m_mesh;
    static unsigned int m_sizeX, m_sizeZ;
    float m_height = -1.0f;
};
