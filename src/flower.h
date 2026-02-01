#pragma once
#include <algorithm>
#include <iostream>

#include "light_manager.h"
#include "mesh.h"
#include "texture.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

struct FlowerComponent
{
    GPUMesh mesh;
    float rotate;
    glm::vec3 translate;
};

struct Particle {
glm::vec4 color = glm::vec4(1.0, 0.8, 0.8, 1.0);
glm::vec3 position = glm::vec3(2.0f, -0.5f, 1.5f);
glm::vec3 velocity = glm::vec3(0.01f, 0.02f, 0.01f);
float lifetime = 1000.0f;
};

class Flower
{

public:

    std::vector<GPUMesh> flowerParts;
    FlowerComponent stem;
    FlowerComponent center;
    glm::mat4 model;
    std::vector<float> petalRotations = {glm::radians(2.0f), glm::radians(2.0f),glm::radians(2.0f),glm::radians(2.0f),glm::radians(2.0f),glm::radians(2.0f) };
    std::vector<float> maxPetalRotations = {glm::radians(34.0f), glm::radians(34.0f),glm::radians(34.0f),glm::radians(34.0f),glm::radians(34.0f),glm::radians(34.0f) };
    std::vector<std::vector<FlowerComponent>*> petals = {&petal1, &petal2, &petal3, &petal4};
    std::vector<FlowerComponent> petal1;
    std::vector<FlowerComponent> petal2;
    std::vector<FlowerComponent> petal3;
    std::vector<FlowerComponent> petal4;
    glm::vec3 floatStem = glm::vec3(0.0f, 0.5f, 0.0f);
    glm::vec3 maxFloatStem = glm::vec3(0.0f, 0.5f, 0.0f);
    float maxStemRotate = 290.0f;
    float maxCenterRotate = 30.0f;
    float startTime = 0.0f;
    bool enableAnimation = false;

    // -- animated texture parameters --
    bool enableTextureAnimation = false;
    float textureSpeed = 5.0f; // num of frames per second
    glm::vec3 centerColor = glm::vec3(0.43f, 0.052f, 0.016f);
    glm::vec3 stemColor = glm::vec3(0.231f, 0.141f, 0.502f);

    // -- particle parameters --
    int numOfParticles = 70;
    int numOfNewParticles = 28;
    std::vector<Particle> particles;
    float agingFactor = 0.75f; // how much lifetime a particle looses at every update
    int lastUsedParticle = 0;
    glm::vec3 spawnPosition = glm::vec3(1.3f, -0.5f, 1.1f); // where the particles will initially appear, the well
    glm::vec3 spawningVelocity = glm::vec3(0.01f, 0.2f, 0.01f);
    bool enableParticles = false;
    float defaultLifetime = 1000.0f;
    glm::vec4 particleColor = glm::vec4(0.67, 0.157, 0, 1.0);
    float particleSize = 5.0;


    /**
     * Each flower component is a different mesh in the same obj file, this method assigns them to different objects and
     * uses each getTransformation method for each component to later animate it.
     * @param modelIn
     */
    explicit Flower(const glm::mat4& modelIn) : model(modelIn)
    {
        std::vector<GPUMesh> meshesIn = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/flowey.obj");

        if (!meshesIn.empty()) center = FlowerComponent {std::move(meshesIn[0]), 57.0f, glm::vec3(1.0f)};

        if (meshesIn.size() > 1) stem = FlowerComponent {std::move(meshesIn[1]), 360.0f, glm::vec3(2.0f, 0.5f, 2.0f)};


        if (meshesIn.size() > 7)
        {
            for (size_t i = 2; i <= 7; ++i) {
                petal1.push_back(FlowerComponent {std::move(meshesIn[i]), glm::radians(3.0f), glm::vec3(1.0f)});
            }
        }
        std::ranges::reverse(petal1);
        if (meshesIn.size() > 13)
        {
            for (size_t i = 8; i <= 13; ++i) {
                petal2.push_back(FlowerComponent {std::move(meshesIn[i]), glm::radians(3.0f), glm::vec3(1.0f)});
            }
        }
        if (meshesIn.size() >= 20)
        {
            for (size_t i = 14; i <= 19; ++i) {
                petal3.push_back(FlowerComponent {std::move(meshesIn[i]), glm::radians(3.0f), glm::vec3(1.0f)});
            }
        }
        if (meshesIn.size() > 25) { // indices 20–25
            for (size_t i = 20; i <= 25; ++i) {
                petal4.push_back(FlowerComponent {std::move(meshesIn[i]), glm::radians(3.0f), glm::vec3(1.0f)});
            }
        }

        // the overall struture of the obj file is:
        // center
        // stem
        // petal 1_6 - 1_1
        // petal_2_6.001 - 2_2.001 ve 1_1
    }
    void scaleDownFlower();
    glm::mat4 getStemTransform()
    {
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), stem.translate);
        glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(stem.rotate) , glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 translateForFloating = glm::translate(glm::mat4(1.0f), floatStem);
        return translateForFloating * translate * rotate;
    }
    glm::mat4 getCenterTransform()
    {
        glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(center.rotate), glm::vec3(1.0f, 0.0f, 0.0f));
        return getStemTransform() * rotate; // Here is the hierarchal part, each component inherits the transformations of the parent
    }
    std::vector<glm::mat4> getPetalTransform(int petalNo)
    {
        std::vector <glm::mat4> transforms;
        glm::mat4 parent = getCenterTransform(); // inherit tranformation of the parent
        // the axis the petal turns around in depends on the petal
        glm::vec3 aroundAxis = glm::vec3(1.0f, 0.0f, 0.0f);
        if (petalNo == 1 || petalNo == 3) // this is the second petal
        {
            aroundAxis = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        // if the petals are facing each other, the angle of rotation should be reversed
        float angleDirection = 1.0;
        if (petalNo == 2 || petalNo == 3)
        {
            angleDirection = -1.0;
        }

        for (int i = 0; i < petalRotations.size(); ++i)
        {
            transforms.push_back(parent * glm::rotate(glm::mat4(1.0f), angleDirection * petalRotations[0], aroundAxis));
        }

        return transforms;
    }

    /**
     * Animates the transformations by mapping to a range in time
     * @param timeNow glfw current time
     */
    void updateTransforms(float timeNow)
    {
        if (!enableAnimation)
        {
            return; // do not animate if not enabled
        }
        float elapsedTime = timeNow - startTime;
        float timeOscill = sin(elapsedTime); // this will oscillate between -1 and 1
        float timeFactor = timeOscill / 2.0f + 0.5f; // now i mapped it to 0 - 1

        // update the floating of stem
        floatStem = maxFloatStem * timeFactor + 0.01f;
        stem.rotate = maxStemRotate * timeFactor + 0.01f;
        center.rotate = maxCenterRotate * timeFactor + 0.01f;

        // for angles, i actually want it to go negative so I use the og range
        for (int i = 0; i < petalRotations.size(); ++i)
        {
            petalRotations[i] = maxPetalRotations[0] * timeOscill;
        }
    }

    // --- Particle animation --

    /**
     * Initializes a list of particle objects according to default values and random position around a central point
     */
    void initParticles() {
        for (int i = 0; i < numOfParticles; ++i) {
            particles.push_back(
                Particle {
                    particleColor,
                    getRandomPosition(),
                    spawningVelocity,
                    defaultLifetime});
        }
    }

    /**
     * Regenerates new particles, updates the movement of alive particles
     */
    void updateParticles() {
        for (int i = 0; i < numOfNewParticles; ++i) {
            int deadParticleIndex = getFirstDeadParticle();
            respawnParticle(deadParticleIndex, glm::vec3(0.0, 0.0, 0.0));
        }

        for (int i = 0; i < numOfParticles; ++i) {
            Particle& currentParticle = particles[i];
            if (currentParticle.lifetime > 0.001f) // particle is alive
            {
                currentParticle.lifetime = currentParticle.lifetime - agingFactor;
                currentParticle.position = currentParticle.position + currentParticle.velocity * agingFactor; // it moves
                currentParticle.position = glm::mod(currentParticle.position, glm::vec3(1.0, 2.0, 1.0)); // we dont let it get too big
                currentParticle.color.a = currentParticle.color.a - currentParticle.position.y * 0.6 * agingFactor; // it grows transparent
                currentParticle.color.a = fmod(currentParticle.color.a, 1.0f); // we need the alpha to wrap around 1
                if (currentParticle.color.a < 0.0f) // sometimes there are negative values for alpha even when they are alive so we reset them
                {
                    currentParticle.color.a = -1.0f * currentParticle.color.a;
                }
            }
        }
    }

    /**
     * Return the first particle that already died so we can reuse it for a new particle
     */
    int getFirstDeadParticle()
    {
        for (int i = lastUsedParticle; i < numOfNewParticles; i++)
        {
            if (particles[i].lifetime <= 0.0f)
            {
                lastUsedParticle = i;
                return i;
            }
        }
        // if we couldnt find it fast, we have to do a normal search
        for (int i = 0; i < lastUsedParticle; i++)
        {
            if (particles[i].lifetime <= 0.001f)
            {
                lastUsedParticle = i;
                return i;
            }
        }
        // if everyone is alive, we have no choice but to kill the first one :(
        lastUsedParticle = 0;
        return 0;
    }

    /**
     * Used to update fields of a particle to make a new one
     * @param newIndex the index of the particle that will be updated
     */
    void respawnParticle(int newIndex, glm::vec3 offset)
    {
        Particle& particle = particles[newIndex];
        glm::vec3 r = getRandomPosition();
        particle.position = glm::vec3(r.x, 0.1f * r.y, r.z);
        particle.color = particleColor;
        particle.lifetime = defaultLifetime;
        particle.velocity = spawningVelocity * 0.1f;
    }

    glm::vec3 getRandomPosition()
    {
        float randomx = (rand() % 200 - 100) / 100.0f; // -1 to 1 range
        float randomz = (rand() % 200 - 100) / 100.0f; // -1 to 1 range
        float randomy = (rand() % 100) / 20.0f; // in the 0 - 5 range

        return spawnPosition + glm::vec3(randomx, randomy, randomz) - glm::vec3(1.0, 0.0, 1.0);
    }

    /**
     * Generates a mesh out of all the particles for them to be drawn
     * @return one mesh that has all the particles
     */
    GPUMesh createMeshFromParticles()
    {
        Mesh mesh;
        std::vector<Vertex> vertices = {};
        int numofPoints = particles.size();

        // Generate dummy triangle indices
        for (uint32_t i = 0; i < (numofPoints/3); ++i) {
            mesh.triangles.push_back({ 3 * i, 3 * i + 1, 3 * i + 2 });
        }

        for (Particle p : particles)
        {
            glm::vec3 pos = p.position;
            vertices.push_back({pos });
        }
        mesh.vertices = std::move(vertices);
        mesh.material = Material();

        glBufferData(GL_ARRAY_BUFFER, sizeof(mesh.vertices), &mesh.vertices, GL_STATIC_DRAW);

        mesh.material = Material{};

        return GPUMesh(mesh);
    }

    /**
     * Generates a mesh only from one particle, can be used instead of making one big mesh for detailed shading
     * @param particle the particle to be drawn
     * @return the mesh object
     */
    GPUMesh createMeshFromParticle(Particle particle)
    {
        Mesh mesh;
        std::vector<Vertex> vertices = {};
        int numofPoints = 1;

        // Generate dummy triangle indices
        for (uint32_t i = 0; i < (numofPoints/3); ++i) {
            mesh.triangles.push_back({ 3 * i, 3 * i + 1, 3 * i + 2 });
        }

        glm::vec3 pos = particle.position;
        vertices.push_back({pos });
        mesh.vertices = std::move(vertices);
        mesh.material = Material();

        glBufferData(GL_ARRAY_BUFFER, sizeof(mesh.vertices), &mesh.vertices, GL_STATIC_DRAW);

        mesh.material = Material{};

        return GPUMesh(mesh);
    }
    // -- end of particle animation --

    // -- Animated Texture (Lava) --
    /**
     * Gets the index of which texture frame of lava should be projected on the flower
     * @param timeNow glfw time now
     * @param numOfFrames of the texture
     * @return the integer to be used in the vector of textures
     */
    int getTextureIndex(float timeNow, int numOfFrames)
    {
        if (!enableTextureAnimation)
        {
            return 0;
        }
        // the same time calculation as before here to match the translations
        float fps = textureSpeed;
        int elapsedTimeSinceStart = static_cast<int>(timeNow * fps);
        // int numOfFramesPassed = textureSpeed * elapsedTime;
        int frameInRange = elapsedTimeSinceStart % (numOfFrames * 2); // map to range 0 - 39
        if (frameInRange > 19) {
           frameInRange = 39 - frameInRange ; // invert it to go from 0 -> 29 and 29 -> 0
        }
        return frameInRange;
    }

    // -- end of animated texture --

    void renderUI()
    {
        ImGui::Checkbox("Enable Hierarchal Animation", &enableAnimation);
        ImGui::DragFloat3("Translate stem on ground", glm::value_ptr(stem.translate), 0.1f);
        ImGui::DragFloat("Rotate center", &center.rotate, 0.1f);
        ImGui::DragFloat("Rotate stem", &stem.rotate, 0.1f);
        ImGui::DragFloat3("Float stem", glm::value_ptr(floatStem), 0.1f);
        ImGui::DragFloat("Maximum Stem Rotation", &maxStemRotate, 0.1f, 1.0f, 360.0f);
        ImGui::DragFloat("Maximum Center Rotation", &maxCenterRotate, 0.1f, 1.0f, 360.0f);
        ImGui::DragFloat("Maximum Petal Rotation", &maxPetalRotations[0], 0.1f, 1.0f, 360.0f);
        ImGui::DragFloat3("Maximum Stem Translation", glm::value_ptr(maxFloatStem), 0.1f);
        if (petal1.size() >= 1 )
        {
            ImGui::DragFloat("Rotate petals", &petalRotations[0], 0.1f);
        }
        ImGui::Text("Animated Texture Parameters");
        ImGui::Checkbox("Enable Animated Lava Texture", &enableTextureAnimation);
        ImGui::DragFloat("Lava texture speed", &textureSpeed, 0.1f, 1.0f, 20.0f);
        ImGui::ColorEdit3("Stem Color", glm::value_ptr(stemColor));
        ImGui::ColorEdit3("Center Color", glm::value_ptr(centerColor));

        ImGui::Text("Particle Parameters");
        ImGui::Checkbox("Enable Magic Particles", &enableParticles);
        ImGui::DragFloat("Particle aging factor", &agingFactor, 0.01f, 0.1f, 10.0f);
        ImGui::DragInt("Number of new particles", &numOfNewParticles, 1, 0, 60);
        ImGui::DragInt("Number of particles", &numOfParticles, 1, 0, 70);
        ImGui::DragFloat3("Initialized velocity", glm::value_ptr(spawningVelocity));
        ImGui::ColorEdit3("Particle Color", glm::value_ptr(particleColor));
        ImGui::DragFloat3("Spawn Location", glm::value_ptr(spawnPosition));
        ImGui::DragFloat("Lifetime of a particle", &defaultLifetime);
        ImGui::DragFloat("Particle Size", &particleSize, 1.0, 0, 15);


    }

};
