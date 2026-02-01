#pragma once
#include <ranges>
#include <unordered_map>
#include <glm/vec3.hpp>
#include "ground.h"

struct TileCoord {
    int x;
    int z;

    bool operator==(const TileCoord &other) const {
        return x == other.x && z == other.z;
    }
};

// Hash function for TileCoord for faster retrieval
namespace std {
    template<>
    struct hash<TileCoord> {
        size_t operator()(const TileCoord &coord) const noexcept {
            return hash<int>()(coord.x) ^ (hash<int>()(coord.z) << 1);
        }
    };
}

class InfiniteGround {
public:
    InfiniteGround() = default;

    void init(const GroundType &type, const GroundMaterial &material, unsigned int tileSize = 128,
              int renderRadius = 1);

    void updateMaterial(const GroundMaterial &newMaterial, MaterialManager& materialManager);

    void updateTiling(int tiling);
    void updateBumpiness(float bumpiness);
    void updateType(const GroundType &newType);
    GroundType getType() const { return m_type; };
    float getHeight() const;
    Ground &getCurrentGroundBlock(const glm::vec3 &playerPos);
    void renderUI(MaterialManager &materialManager, glm::vec3 playerPosition);

    void update(const glm::vec3 &playerPos, MaterialManager &materialManager);

    void drawSeabedAtTile(const TileCoord &coord,
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
                                     float seabedDepth = 5.0f);

    void drawAll(Shader& groundShader,
                 Shader& waterShader,
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
                 const glm::vec4 &clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -1e9),
                 bool isCurved = false);

    bool hasRandomTiles;
    unsigned int tileSize;
    std::vector<TileCoord> tiles = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0}, {0,  0}, {1,  0},
        {-1,  1}, {0,  1}, {1,  1}
    };

private:
    std::unordered_map<TileCoord, Ground> activeBlocks;
    Ground m_templateGround;
    Ground seabedTile;
    GroundMaterial m_material;
    GroundType m_type;
    int renderRadius;
    float groundScale = 1.0f;
    int groundTextureTiling = 4;
    float groundBumpiness = 0.5f;
    WaterSettings water;
};
