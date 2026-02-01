#pragma once
#include "mesh.h"
#include "texture.h"

GPUMesh createSkybox();
void drawSky(const Shader& skyboxShader, GPUMesh& skyboxMesh, const Texture& envCubemap,
    const glm::mat4& view, const glm::mat4& projection);
