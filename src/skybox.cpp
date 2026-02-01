#include "skybox.h"
#include "texture.h"

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

/**
 * Generates the mesh on which we put the sky (a big cube)
 * @return a cube mesh
 */
GPUMesh createSkybox()
{
    Mesh skybox;

    // 36 vertices for a cube with 6 faces (2 triangles per face)
    std::vector<Vertex> vertices = {
        {{-1.0f,  1.0f, -1.0f}}, {{-1.0f, -1.0f, -1.0f}}, {{ 1.0f, -1.0f, -1.0f}},
        {{ 1.0f, -1.0f, -1.0f}}, {{ 1.0f,  1.0f, -1.0f}}, {{-1.0f,  1.0f, -1.0f}},

        {{-1.0f, -1.0f,  1.0f}}, {{-1.0f, -1.0f, -1.0f}}, {{-1.0f,  1.0f, -1.0f}},
        {{-1.0f,  1.0f, -1.0f}}, {{-1.0f,  1.0f,  1.0f}}, {{-1.0f, -1.0f,  1.0f}},

        {{ 1.0f, -1.0f, -1.0f}}, {{ 1.0f, -1.0f,  1.0f}}, {{ 1.0f,  1.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f}}, {{ 1.0f,  1.0f, -1.0f}}, {{ 1.0f, -1.0f, -1.0f}},

        {{-1.0f, -1.0f,  1.0f}}, {{-1.0f,  1.0f,  1.0f}}, {{ 1.0f,  1.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f}}, {{ 1.0f, -1.0f,  1.0f}}, {{-1.0f, -1.0f,  1.0f}},

        {{-1.0f,  1.0f, -1.0f}}, {{ 1.0f,  1.0f, -1.0f}}, {{ 1.0f,  1.0f,  1.0f}},
        {{ 1.0f,  1.0f,  1.0f}}, {{-1.0f,  1.0f,  1.0f}}, {{-1.0f,  1.0f, -1.0f}},

        {{-1.0f, -1.0f, -1.0f}}, {{-1.0f, -1.0f,  1.0f}}, {{ 1.0f, -1.0f, -1.0f}},
        {{ 1.0f, -1.0f, -1.0f}}, {{-1.0f, -1.0f,  1.0f}}, {{ 1.0f, -1.0f,  1.0f}}
    };

    skybox.vertices = std::move(vertices);

    // Generate dummy triangle indices
    for (uint32_t i = 0; i < 12; ++i) {
        skybox.triangles.push_back({ 3 * i, 3 * i + 1, 3 * i + 2 });
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox.vertices), &skybox.vertices, GL_STATIC_DRAW);

    // Skybox has no material
    skybox.material = Material{};

    return GPUMesh(skybox);
}

void drawSky(const Shader& skyboxShader, GPUMesh& skyboxMesh, const Texture& envCubemap,
    const glm::mat4& view, const glm::mat4& projection)
{
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    skyboxShader.bind();

    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
    glUniformMatrix4fv(skyboxShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(viewNoTranslation));
    glUniformMatrix4fv(skyboxShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(skyboxShader.getUniformLocation("skybox"), 0);
    envCubemap.bindCubemap();

    skyboxMesh.drawSkybox(skyboxShader);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}