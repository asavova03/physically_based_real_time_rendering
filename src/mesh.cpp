#include "mesh.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

GPUMaterial::GPUMaterial(const Material &material) : kd(material.kd),
                                                     ks(material.ks),
                                                     shininess(material.shininess),
                                                     transparency(material.transparency) {
}

GPUMesh::GPUMesh(const Mesh &cpuMesh) {
    // Create uniform buffer to store mesh material (https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL)
    GPUMaterial gpuMaterial(cpuMesh.material);
    glGenBuffers(1, &m_uboMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, m_uboMaterial);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPUMaterial), &gpuMaterial, GL_STATIC_READ);

    // Figure out if this mesh has texture coordinates
    m_hasTextureCoords = static_cast<bool>(cpuMesh.material.kdTexture);

    // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create vertex buffer object (VBO)
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(cpuMesh.vertices.size() * sizeof(decltype(cpuMesh.vertices)::value_type)),
        cpuMesh.vertices.data(), GL_STATIC_DRAW);

    // Create index buffer object (IBO)
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(cpuMesh.triangles.size() * sizeof(decltype(cpuMesh.triangles)::value_type)),
        cpuMesh.triangles.data(), GL_STATIC_DRAW);

    // Tell OpenGL that we will be using vertex attributes 0, 1 and 2.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    // We tell OpenGL what each vertex looks like and how they are mapped to the shader (location = ...).
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, texCoord));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, tangent));
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, bitangent));
    // Reuse all attributes for each instance
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);

    // Each triangle has 3 vertices.
    m_numIndices = static_cast<GLsizei>(3 * cpuMesh.triangles.size());


    // Set up vbo for instance rendering for procedural generation of repeated meshes (gems)
    glBindVertexArray(m_vao);
    glGenBuffers(1, &m_instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    // Render at maximum 10 000 gem instances
    glBufferData(GL_ARRAY_BUFFER, 10000 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);

    std::size_t vec4Size = sizeof(glm::vec4);
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(5 + i);
        glVertexAttribPointer(5 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                              (void*)(i * vec4Size));
        glVertexAttribDivisor(5 + i, 1);
    }

    glBindVertexArray(0);
}

GPUMesh::GPUMesh(GPUMesh &&other) {
    moveInto(std::move(other));
}

GPUMesh::~GPUMesh() {
    freeGpuMemory();
}

GPUMesh &GPUMesh::operator=(GPUMesh &&other) {
    moveInto(std::move(other));
    return *this;
}

std::vector<GPUMesh> GPUMesh::loadMeshGPU(std::filesystem::path filePath, bool normalize) {
    if (!std::filesystem::exists(filePath))
        throw MeshLoadingException(fmt::format("File {} does not exist", filePath.string().c_str()));

    // Generate GPU-side meshes for all sub-meshes
    std::vector<Mesh> subMeshes = loadMesh(filePath, {.normalizeVertexPositions = normalize});
    std::vector<GPUMesh> gpuMeshes;
    for (const Mesh &mesh: subMeshes) { gpuMeshes.emplace_back(mesh); }

    return gpuMeshes;
}

bool GPUMesh::hasTextureCoords() const {
    return m_hasTextureCoords;
}

void GPUMesh::draw(const Shader &drawingShader) {
    // Bind material data uniform (we assume that the uniform buffer objects is always called 'Material')
    // Yes, we could define the binding inside the shader itself, but that would break on OpenGL versions below 4.2
    drawingShader.bindUniformBlock("Material", 0, m_uboMaterial);

    // Draw the mesh's triangles
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

/**
 * Use instancing to draw multiple meshes at batches for performance
 * @param shader the shading for the batch
 * @param modelMatrices all positions where the gems or repeated meshes need to be drawn
 */
void GPUMesh::drawInstanced(const Shader &shader, const std::vector<glm::mat4> &modelMatrices) const {
    // shader.bindUniformBlock("Material", 0, m_uboMaterial);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, modelMatrices.size() * sizeof(glm::mat4), modelMatrices.data());
    glDrawElementsInstanced(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT,
                            nullptr, static_cast<GLsizei>(modelMatrices.size()));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void GPUMesh::drawSkybox(const Shader &drawingShader) {
    glBindVertexArray(m_vao);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);
}

void GPUMesh::drawCurve(int numOfPoints, float size = 8.0f) {
    glBindVertexArray(m_vao);
    glPointSize(size);
    glDrawArrays(GL_POINTS, 0, numOfPoints);

    glBindVertexArray(0);
}

void GPUMesh::drawParticle(float size) {
    glBindVertexArray(m_vao);
    glPointSize(size);
    glDrawArrays(GL_POINTS, 0, 1);

    glBindVertexArray(0);
}

void GPUMesh::moveInto(GPUMesh &&other) {
    freeGpuMemory();
    m_numIndices = other.m_numIndices;
    m_hasTextureCoords = other.m_hasTextureCoords;
    m_ibo = other.m_ibo;
    m_vbo = other.m_vbo;
    m_vao = other.m_vao;
    m_uboMaterial = other.m_uboMaterial;

    other.m_numIndices = 0;
    other.m_hasTextureCoords = other.m_hasTextureCoords;
    other.m_ibo = INVALID;
    other.m_vbo = INVALID;
    other.m_vao = INVALID;
    other.m_uboMaterial = INVALID;
}

void GPUMesh::freeGpuMemory() {
    if (m_vao != INVALID)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != INVALID)
        glDeleteBuffers(1, &m_vbo);
    if (m_ibo != INVALID)
        glDeleteBuffers(1, &m_ibo);
    if (m_uboMaterial != INVALID)
        glDeleteBuffers(1, &m_uboMaterial);
    if (m_instanceVBO != INVALID)
        glDeleteBuffers(1, &m_instanceVBO);

}

void GPUMesh::setMaterial(const Material &material) {
    GPUMaterial gpuMaterial(material);
    glGenBuffers(1, &m_uboMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, m_uboMaterial);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPUMaterial), &gpuMaterial, GL_STATIC_READ);
}



