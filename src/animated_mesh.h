#pragma once
#include <algorithm>

#include "mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <vector>
#include <string>

#include "GLFW/glfw3.h"
#include "glm/gtc/type_ptr.hpp"
#pragma once
struct AnimatedMesh {
    std::vector<Mesh> frames;
    std::vector<GLuint> vaos;
    std::vector<GLuint> vbos;
    std::vector<GLuint> ibos;

    size_t currentFrame = 0;
    float frameTime = 1.0f / 40.0f;
    float startTime = 0.0f;

    GLsizei numIndices = 0;

    glm::vec3 newPos = glm::vec3(0.0f);
    glm::vec3 prevPos = glm::vec3(0.0f);
    float rotationY = 0.0f;

    bool isWalking = false;

    glm::vec3 moveDir = glm::vec3(0.0f);
    float moveSpeed = 0.030f;

    AnimatedMesh() = default;

    void loadFromFolder(const std::string &folderPath) {
        namespace fs = std::filesystem;
        frames.clear();
        vaos.clear();
        vbos.clear();
        ibos.clear();

        std::vector<fs::path> objFiles;
        for (auto &entry: fs::directory_iterator(folderPath)) {
            if (entry.path().extension() == ".obj") {
                objFiles.push_back(entry.path());
            }
        }

        std::sort(objFiles.begin(), objFiles.end(), [](const fs::path &a, const fs::path &b) {
            auto extractNumber = [](const std::string &name) -> int {
                size_t pos = name.find_last_not_of("0123456789");
                if (pos == std::string::npos || pos + 1 >= name.size()) return 0;
                try {
                    return std::stoi(name.substr(pos + 1));
                } catch (...) {
                    return 0;
                }
            };

            std::string sa = a.stem().string();
            std::string sb = b.stem().string();
            int na = extractNumber(sa);
            int nb = extractNumber(sb);

            if (na != 0 && nb != 0)
                return na < nb;

            return sa < sb;
        });


        for (auto &path: objFiles) {
            Mesh m = loadMesh(path.string())[0];
            frames.push_back(m);

            GLuint vbo, ibo, vao;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER,
                         m.vertices.size() * sizeof(Vertex),
                         m.vertices.data(),
                         GL_STATIC_DRAW);

            glGenBuffers(1, &ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         m.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type),
                         m.triangles.data(),
                         GL_STATIC_DRAW);

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                                  sizeof(Vertex), (void *) offsetof(Vertex, position));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                                  sizeof(Vertex), (void *) offsetof(Vertex, normal));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(Vertex), (void *) offsetof(Vertex, texCoord));

            glBindVertexArray(0);

            vaos.push_back(vao);
            vbos.push_back(vbo);
            ibos.push_back(ibo);
        }
    }

    void update(float timeNow, const Ground &ground, float cameraYaw, float deltaTime, bool characterRelative) {
        if (isWalking) {
            float elapsed = timeNow - startTime;
            float animTime = fmod(elapsed, frameTime * frames.size());
            currentFrame = static_cast<size_t>(animTime / frameTime);

            float phase = animTime / (frameTime * frames.size());
            // Movement follows a footstep rhythm, slower when stepping, faster when pushing out with the leg
            float displacement = 1.0f - cos((phase * 4.0f * M_PI) + M_PI);
            float realSpeed = moveSpeed * displacement;

            moveDir = glm::normalize(moveDir);

            // Rotate move direction for character-follow camera
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), cameraYaw, glm::vec3(0, 1, 0));
            glm::vec3 rotatedMoveDir = glm::normalize(glm::vec3(rot * glm::vec4(moveDir, 0.0f)));

            // Rotation logic
            glm::vec3 forward = glm::vec3(sin(rotationY), 0.0f, cos(rotationY));
            float dotDir = glm::dot(forward, rotatedMoveDir);
            bool movingBackward = dotDir < -0.25f;
            const float turnSpeed = 1.0f;

            if (characterRelative) {
                if (!movingBackward) {
                    float desiredYaw = atan2(rotatedMoveDir.x, rotatedMoveDir.z);
                    float deltaYaw = glm::mod(desiredYaw - rotationY + glm::pi<float>(), glm::two_pi<float>()) - glm::pi
                                     <float>();
                    rotationY += deltaYaw * (1.0f - expf(-turnSpeed * deltaTime));
                }
            } else {
                rotationY = atan2(rotatedMoveDir.x, rotatedMoveDir.z);
            }

            // Position update
            prevPos = newPos;
            newPos += rotatedMoveDir * realSpeed;
        } else {
            currentFrame = 0;
        }
        newPos.y = ground.getHeightAt(newPos.x, newPos.z) + 1.0f;
    }

    void draw(const Shader &shader) {
        if (frames.empty()) return;

        const Mesh &m = frames[currentFrame];
        numIndices = static_cast<GLsizei>(m.triangles.size() * 3);

        glBindVertexArray(vaos[currentFrame]);
        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    void moveForward() {
        moveDir.z = 1.0f;
        startWalking();
    }

    void moveBackward() {
        moveDir.z = -1.0f;
        startWalking();
    }

    void moveLeft() {
        moveDir.x = 1.0f;
        startWalking();
    }

    void moveRight() {
        moveDir.x = -1.0f;
        startWalking();
    }

    void stopMovingForward() {
        moveDir.z = 0.0f;
        checkIfStillWalking();
    }

    void stopMovingBackward() {
        moveDir.z = 0.0f;
        checkIfStillWalking();
    }

    void stopMovingLeft() {
        moveDir.x = 0.0f;
        checkIfStillWalking();
    }

    void stopMovingRight() {
        moveDir.x = 0.0f;
        checkIfStillWalking();
    }

    void startWalking() {
        if (!isWalking) {
            isWalking = true;
            startTime = glfwGetTime();
        }
    }

    void checkIfStillWalking() {
        if (glm::length(moveDir) == 0.0f) {
            isWalking = false;
        }
    }

    ~AnimatedMesh() {
        if (!vaos.empty())
            glDeleteVertexArrays(static_cast<GLsizei>(vaos.size()), vaos.data());

        if (!vbos.empty())
            glDeleteBuffers(static_cast<GLsizei>(vbos.size()), vbos.data());

        if (!ibos.empty())
            glDeleteBuffers(static_cast<GLsizei>(ibos.size()), ibos.data());
    }
};
