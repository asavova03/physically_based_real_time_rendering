#include "mesh.h"
#include "skybox.h"
#include "bezier_curve.h"
#include "texture.h"
#include "ground.h"
#include "light_manager.h"
#include "material_manager.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>

#include "animated_mesh.h"
#include "camera_manager.h"
#include "flower.h"
#include "gem_manager.h"
#include "infinite_ground.h"
#include "skybox_manager.h"
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>
#include "../framework/third_party/stb/include/stb/stb_image.h"

class Application {
    bool show_imgui = true;
    bool crystalTextureLighting = true;
    bool shiftPressed = false;

    float colorTransformation = 0.0f;

    struct {
        float ior = 2.0f;
        float emissionScale = 0.5f;
        float emissionPulse = 1.5f;
        float baseWeight = 0.7f;
        float refractedWeight = 0.3f;
        float emissiveWeight = 1.0f;
    } shadingData;

    Material mat_character{
        glm::vec3(0.5f),
        glm::vec3(0.5f),
        120.0f
    };

    void imgui() {
        if (!show_imgui)
            return;

        ImGui::Begin("Final Project");

        // === Crystal Character ===
        if (ImGui::CollapsingHeader("Crystal Character")) {
            ImGui::Separator();
            ImGui::Text("Material Parameters");
            ImGui::SliderFloat("Shininess", &mat_character.shininess, 0.0f, 200.0f);
            ImGui::ColorEdit3("Kd (Diffuse)", &mat_character.kd[0]);
            ImGui::ColorEdit3("Ks (Specular)", &mat_character.ks[0]);
            ImGui::SliderFloat("Color Transformation", &colorTransformation, -1.0f, 2.0f);
            ImGui::SliderFloat("Base Contribution", &shadingData.baseWeight, 0.0f, 1.0f);

            ImGui::Separator();
            ImGui::Text("Glow Parameters");
            ImGui::SliderFloat("Glow Pulse", &shadingData.emissionPulse, 0.0f, 5.0f);
            ImGui::SliderFloat("Glow Contribution", &shadingData.emissiveWeight, 0.0f, 1.0f);

            ImGui::Separator();
            ImGui::Text("Refraction Parameters");
            ImGui::SliderFloat("IOR", &shadingData.ior, 1.0f, 3.0f);
            ImGui::SliderFloat("Refraction Contribution", &shadingData.refractedWeight, 0.0f, 1.0f);

            idlePlayer.setMaterial(mat_character);

            if (ImGui::CollapsingHeader("Animation Parameters")) {
                static float fps = 40.0f;
                ImGui::SliderFloat("Animation Speed [FPS]:", &fps, 16.0f, 60.0f);
                walkingPlayer.frameTime = 1.0f / fps;

                static float speed = 25;
                ImGui::SliderFloat("Moving Speed:", &speed, 1.0f, 200.0f);
                walkingPlayer.moveSpeed = speed / 1000.0f;
            }
        }

        // === Gem Generation ===
        gems.renderUI();

        // === Scene Setup ===
        if (ImGui::CollapsingHeader("Scene Setup", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Separator();
            ImGui::Text("Environment");
            if (skyboxManager.renderUI()) {
                lightManager.resetSun(skyboxManager.getCurrentType(), walkingPlayer.newPos);
            }

            ImGui::Separator();
            if (infiniteGround.hasRandomTiles)
                ImGui::BeginDisabled(true);

            materialManager.renderUI();

            if (infiniteGround.hasRandomTiles)
                ImGui::EndDisabled();

            ImGui::Checkbox("Shuffle Ground Tiles", &infiniteGround.hasRandomTiles);
        }

        // === Water Settings Panel ===
        infiniteGround.renderUI(materialManager, walkingPlayer.newPos);

        // === Camera Settings ===
        if (ImGui::CollapsingHeader("Camera Settings")) {
            cameraController.renderUI();
        }

        // === Lights ===
        if (ImGui::CollapsingHeader("Lights")) {
            lightManager.renderUI();
        }

        // == Flower Animation ==
        if (ImGui::CollapsingHeader("Flower Animation")) {
            flowerManager.renderUI();
        }


        ImGui::End();
        ImGui::Render();
    }

    static size_t getClosestVertexIndex(const Mesh &mesh, const glm::vec3 &pos) {
        const auto iter = std::min_element(
            std::begin(mesh.vertices), std::end(mesh.vertices),
            [&](const Vertex &lhs, const Vertex &rhs) {
                return glm::length(lhs.position - pos) < glm::length(rhs.position - pos);
            });
        return static_cast<size_t>(std::distance(std::begin(mesh.vertices), iter));
    }

    void createFrameBuffer(GLuint &sceneFBO, Texture &sceneColorTex, Texture &sceneDepthTex,
                           GLuint &sceneDepthBuffer) const {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Generate framebuffer
        glGenFramebuffers(1, &sceneFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

        // Use current window size
        int width = m_window.getWindowSize().x;
        int height = m_window.getWindowSize().y;

        // Create empty color texture
        sceneColorTex.createEmptyColorBuffer(GL_RGB, width, height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, sceneColorTex.id(), 0);

        // Depth texture
        sceneDepthTex.createEmptyDepthBuffer(width, height);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTex.id(), 0);
        glGenRenderbuffers(1, &sceneDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sceneDepthBuffer);


        // Check completeness
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "FBO incomplete!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void cleanUp() {
        glDeleteFramebuffers(1, &sceneFBOCrystal);
        glDeleteFramebuffers(1, &sceneFBOCrystal);
        glDeleteFramebuffers(1, &sceneFBORefraction);
    }

    static std::vector<Texture> loadLavaAnimation(const std::string& folder_path)
    {
        std::vector<Texture> textures = {};
        std::vector<std::filesystem::path> texture_paths = {};
        for (auto &entry: std::filesystem::directory_iterator(folder_path)) {
            if (entry.path().extension() == ".png") {
                texture_paths.push_back(entry.path());
            }
        }
        std::ranges::sort(texture_paths);
        for (auto &path : texture_paths)
        {
            textures.emplace_back(path);
        }
        return textures;
    }

public:
    Application()
        : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
          , skybox(createSkybox()), flowerManager(glm::mat4(1.0f)) {
        m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS)
                onKeyPressed(key, mods);
            else if (action == GLFW_RELEASE)
                onKeyReleased(key, mods);
        });
        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
            if (action == GLFW_PRESS)
                onMouseClicked(button, mods);
            else if (action == GLFW_RELEASE)
                onMouseReleased(button, mods);
        });
        m_window.registerWindowResizeCallback([this](const glm::ivec2 &size) {
            onResize(size);
        });
        lightManager = LightManager();
        walkingPlayer.loadFromFolder("resources/walk");
        idlePlayer = std::move(GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/crystal_guy.obj").front());

        idlePlayer.setMaterial(mat_character);

        props = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/well.obj");

        const Material mat_props{
            glm::vec3(0.5f),
            glm::vec3(0.5f),
            120.0f
        };

        for (auto &mesh: props) {
            mesh.setMaterial(mat_props);
        }
        texCrystal.loadTexture(RESOURCE_ROOT "resources/textures/crystal_guy_map.png", GL_CLAMP_TO_EDGE);
        texWell.loadTexture(RESOURCE_ROOT "resources/textures/well.png", GL_CLAMP_TO_EDGE);
        texFlower = loadLavaAnimation(RESOURCE_ROOT "resources/textures/lava-texture-2");

        cameraController.buildConstantSpeedLookupPoints();

        materialManager.initialize();
        skyboxManager.initialize();

        createFrameBuffer(sceneFBOCrystal, sceneTexColorCrystal, sceneTexDepthCrystal, sceneDepthBufferCrystal);
        createFrameBuffer(sceneFBORefraction, sceneTexColorRefraction, sceneTexDepthRefraction,
                          sceneDepthBufferRefraction);
        createFrameBuffer(sceneFBOReflection, sceneTexColorReflection, sceneTexDepthReflection,
                          sceneDepthBufferReflection);

        infiniteGround.init(materialManager.getCurrentType(), materialManager.getCurrentMaterial());
        bloom.initBloom(m_window.getWindowSize().x, m_window.getWindowSize().y, 0);
        gems.init(RESOURCE_ROOT "resources/gems.obj",
                  skyboxManager,
                  infiniteGround,
                  lightManager,
                  bloom);
        flowerManager.initParticles();

        try {
            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
            crystalGuyShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/props_vertex.glsl").
                    addStage(
                        GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/crystal_guy.glsl").build();
            simpleTextureShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/props_vertex.glsl").
                    addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/well_frag.glsl").build();
            flowerShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/props_vertex.glsl").
                    addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/flower_frag.glsl").build();
            particleShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/particles_vertex.glsl").
                    addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/particle_frag.glsl").build();
            gemShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/gem_vert.glsl").
                    addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/gem_frag.glsl").build();
            skyboxShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vertex.glsl").
                    addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/skybox_frag.glsl").build();
            groundShader = ShaderBuilder()
                    .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/ground_vertex.glsl")
                    .addStage(GL_GEOMETRY_SHADER, RESOURCE_ROOT "shaders/ground_geom.glsl")
                    .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/ground_frag.glsl")
                    .build();
            waterShader = ShaderBuilder()
                    .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/ground_vertex.glsl")
                    .addStage(GL_GEOMETRY_SHADER, RESOURCE_ROOT "shaders/water_geom.glsl")
                    .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/water_frag.glsl")
                    .build();
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void captureCrystalRefraction(const glm::vec3 cameraPos, const glm::mat4 &model, const glm::mat4 &view,
                                  const glm::mat4 &projection, const std::vector<Light> lights) {
        GLint prevFBO;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBOCrystal);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        int width = m_window.getWindowSize().x;
        int height = m_window.getWindowSize().y;
        glViewport(0, 0, width, height);
        // Draw sky and ground for the framebuffer to capture
        drawSky(skyboxShader, skybox, skyboxManager.getCurrentCubemap(), view, projection);
        for (GPUMesh &mesh: props) {
            drawProp(mesh, lights, model, view, projection, cameraPos);
        }
        gems.drawAll(gemShader, model, view, projection, cameraPos, lights);
        infiniteGround.drawAll(groundShader, waterShader,
                               skyboxManager.getCurrentCubemap(),
                               sceneTexColorRefraction,
                               sceneTexColorReflection,
                               *skyboxManager.getIBL(),
                               model, view, projection,
                               lights, cameraPos, materialManager);

        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);

        glViewport(0, 0, width, height);
    }

    void drawSeabed(const glm::vec3 &cameraPos, const glm::mat4 &model, const glm::mat4 &view,
                    const glm::mat4 &projection, const std::vector<Light> lights, bool isCurved = false,
                    const glm::vec4 &clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -1e9)) {
        infiniteSeabed.init(GroundType::SAND, materialManager.getSomeMaterial(GroundType::SAND));
        infiniteSeabed.update(walkingPlayer.newPos, materialManager);
        glm::mat4 underwaterModel = glm::translate(model, glm::vec3(0.0f, -5.0f, 0.0f));
        infiniteSeabed.drawAll(groundShader, waterShader,
                               skyboxManager.getCurrentCubemap(),
                               sceneTexColorRefraction,
                               sceneTexColorReflection,
                               *skyboxManager.getIBL(),
                               underwaterModel, view, projection,
                               lights, cameraPos, materialManager, clipPlane, isCurved);
    }

    void captureWaterSurroundings(GLuint sceneFBO,
                                  const glm::vec3 &cameraPos,
                                  const glm::mat4 &model,
                                  const glm::mat4 &view,
                                  const glm::mat4 &projection,
                                  const glm::vec4 &clipPlane,
                                  const std::vector<Light> lights,
                                  bool hasCurvedSeabed = false) {
        GLint prevFBO;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

        int width = m_window.getWindowSize().x;
        int height = m_window.getWindowSize().y;
        glViewport(0, 0, width, height);

        // Clear with a specific background color instead of undefined values
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        drawSky(skyboxShader, skybox, skyboxManager.getCurrentCubemap(), view, projection);

        if (clipPlane.w == infiniteGround.getHeight()) {
            drawSeabed(cameraPos, model, view, projection, lights, hasCurvedSeabed, clipPlane);
        }

        drawCrystalGuy(idlePlayer, lights, model, view, projection, cameraPos, clipPlane);
        gems.drawAll(gemShader, model, view, projection, cameraPos, lights, clipPlane);

        for (GPUMesh &mesh: props) {
            drawProp(mesh, lights, model, view, projection, cameraPos, clipPlane);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        glViewport(0, 0, width, height);
    }

    void drawCrystalGuy(GPUMesh &mesh, const std::vector<Light> &lights, const glm::mat4 &model, const glm::mat4 &view,
                        const glm::mat4 &projection,
                        const glm::vec3 cameraPos,
                        glm::vec4 clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -1e9)) {
        float timeNow = static_cast<float>(glfwGetTime());
        float deltaTime = timeNow - cameraController.lastTime;
        cameraController.lastTime = timeNow;
        bool characterRelative = (cameraController.mode == CameraMode::CharacterFollow);
        cameraController.update(deltaTime, walkingPlayer);

        walkingPlayer.update(glfwGetTime(), infiniteGround.getCurrentGroundBlock(walkingPlayer.newPos),
                             cameraController.cameraBaseYaw,
                             deltaTime, characterRelative);
        crystalGuyShader.bind();
        glUniform1i(crystalGuyShader.getUniformLocation("numLights"), (int) lights.size());
        for (int i = 0; i < lights.size(); i++) {
            std::string idx = std::to_string(i);
            glUniform3fv(crystalGuyShader.getUniformLocation(("lights[" + idx + "].position").c_str()), 1,
                         glm::value_ptr(lights[i].position));
            glUniform3fv(crystalGuyShader.getUniformLocation(("lights[" + idx + "].color").c_str()), 1,
                         glm::value_ptr(lights[i].color));
            glUniform1f(crystalGuyShader.getUniformLocation(("lights[" + idx + "].intensity").c_str()),
                        lights[i].intensity);
        }
        texCrystal.bind(0);
        glUniform1i(crystalGuyShader.getUniformLocation("texCrystal"), 0);
        sceneTexColorCrystal.bind(1);
        glUniform1i(crystalGuyShader.getUniformLocation("sceneColor"), 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxManager.getIBL()->getIrradianceMap());
        glUniform1i(crystalGuyShader.getUniformLocation("irradianceMap"), 2);
        glUniform1f(crystalGuyShader.getUniformLocation("ior"), shadingData.ior);
        shadingData.emissionScale = 0.6f + 0.3f * sin(glfwGetTime() * shadingData.emissionPulse);
        glUniform1f(crystalGuyShader.getUniformLocation("emissionScale"), shadingData.emissionScale);
        glUniform1f(crystalGuyShader.getUniformLocation("baseWeight"), shadingData.baseWeight);
        glUniform1f(crystalGuyShader.getUniformLocation("refractedWeight"), shadingData.refractedWeight);
        glUniform1f(crystalGuyShader.getUniformLocation("emissiveWeight"), shadingData.emissiveWeight);
        glUniform1f(crystalGuyShader.getUniformLocation("colorTransformation"), colorTransformation);
        glUniform3fv(crystalGuyShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
        glm::mat4 moved = model;
        if (walkingPlayer.isWalking) {
            // Because the animated mesh is bent over and centered around the origin, the feet levitate a bit,
            // so this is just hard-coded value so the guy steps on the ground while walking
            moved = glm::translate(model, glm::vec3(0.0, -0.25, 0.0));
        }
        moved = glm::translate(moved, walkingPlayer.newPos);
        moved = glm::rotate(moved, walkingPlayer.rotationY, glm::vec3(0, 1, 0));
        glUniformMatrix4fv(crystalGuyShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(moved));
        glUniformMatrix4fv(crystalGuyShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(crystalGuyShader.getUniformLocation("projection"), 1, GL_FALSE,
                           glm::value_ptr(projection));
        glUniform4fv(crystalGuyShader.getUniformLocation("plane"), 1, glm::value_ptr(clipPlane));

        if (walkingPlayer.isWalking) {
            walkingPlayer.draw(crystalGuyShader);
        } else {
            mesh.draw(crystalGuyShader);
        }
    }

    void drawProp(GPUMesh &mesh, const std::vector<Light> &lights, const glm::mat4 &model, const glm::mat4 &view,
                  const glm::mat4 &projection, const glm::vec3 cameraPos,
                  glm::vec4 clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -1e9)) {
        simpleTextureShader.bind();
        texWell.bind(0);
        glUniform1i(simpleTextureShader.getUniformLocation("numLights"), (int) lights.size());
        for (int i = 0; i < lights.size(); i++) {
            std::string idx = std::to_string(i);
            glUniform3fv(simpleTextureShader.getUniformLocation(("lights[" + idx + "].position").c_str()), 1,
                         glm::value_ptr(lights[i].position));
            glUniform3fv(simpleTextureShader.getUniformLocation(("lights[" + idx + "].color").c_str()), 1,
                         glm::value_ptr(lights[i].color));
            glUniform1f(simpleTextureShader.getUniformLocation(("lights[" + idx + "].intensity").c_str()),
                        lights[i].intensity);
        }
        glUniform1i(simpleTextureShader.getUniformLocation("texWell"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxManager.getIBL()->getIrradianceMap());
        glUniform1i(simpleTextureShader.getUniformLocation("irradianceMap"), 1);
        glm::mat4 moved = glm::translate(model, glm::vec3(2.0f, -0.5f, 2.0f));
        glUniformMatrix4fv(simpleTextureShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(moved));
        glUniformMatrix4fv(simpleTextureShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(simpleTextureShader.getUniformLocation("projection"), 1, GL_FALSE,
                           glm::value_ptr(projection));
        glUniform3fv(simpleTextureShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform4fv(simpleTextureShader.getUniformLocation("plane"), 1, glm::value_ptr(clipPlane));
        mesh.draw(simpleTextureShader);
    }

    void drawParticles(const std::vector<Light> &lights, const glm::mat4 &model, const glm::mat4 &view,
                  const glm::mat4 &projection, const glm::vec3 cameraPos,
                  glm::vec4 clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -1e9)) {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        particleShader.bind();
        glm::mat4 moved = glm::translate(model, glm::vec3(1.5f, -0.5f, 1.5f));
        glUniformMatrix4fv(particleShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(moved));
        glUniformMatrix4fv(particleShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(particleShader.getUniformLocation("projection"), 1, GL_FALSE,
                           glm::value_ptr(projection));
        glUniform3fv(particleShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform4fv(particleShader.getUniformLocation("plane"), 1, glm::value_ptr(clipPlane));

        for (Particle& particle : flowerManager.particles)
        {

            if (particle.lifetime > 0.001f)
            {
                glUniform3fv(particleShader.getUniformLocation("offset"), 1, glm::value_ptr(particle.position));
                glUniform4fv(particleShader.getUniformLocation("color"), 1, glm::value_ptr(particle.color));
                flowerManager.createMeshFromParticle(particle).drawParticle(flowerManager.particleSize);
            }
        }
    }

    void drawBezier(GPUMesh &mesh, const std::vector<Light> &lights, const glm::mat4 &model, const glm::mat4 &view,
                  const glm::mat4 &projection, const glm::vec3 cameraPos,
                  int numOfPoints = 540 ,  glm::vec4 clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -1e9)) {
        simpleTextureShader.bind();
        texWell.bind(0);
        glUniform1i(simpleTextureShader.getUniformLocation("numLights"), (int) lights.size());
        for (int i = 0; i < lights.size(); i++) {
            std::string idx = std::to_string(i);
            glUniform3fv(simpleTextureShader.getUniformLocation(("lights[" + idx + "].position").c_str()), 1,
                         glm::value_ptr(lights[i].position));
            glUniform3fv(simpleTextureShader.getUniformLocation(("lights[" + idx + "].color").c_str()), 1,
                         glm::value_ptr(lights[i].color));
            glUniform1f(simpleTextureShader.getUniformLocation(("lights[" + idx + "].intensity").c_str()),
                        lights[i].intensity);
        }
        glUniform1i(simpleTextureShader.getUniformLocation("texWell"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxManager.getIBL()->getIrradianceMap());
        glUniform1i(simpleTextureShader.getUniformLocation("irradianceMap"), 1);
        // glm::mat4 moved = glm::translate(model, glm::vec3(2.0f, 1.5f, 2.0f));
        glUniformMatrix4fv(simpleTextureShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(simpleTextureShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(simpleTextureShader.getUniformLocation("projection"), 1, GL_FALSE,
                           glm::value_ptr(projection));
        glUniform3fv(simpleTextureShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform4fv(simpleTextureShader.getUniformLocation("plane"), 1, glm::value_ptr(clipPlane));

        mesh.drawCurve(numOfPoints, 8.0f);
    }

    void drawFlower(const std::vector<Light> &lights, const glm::mat4 &model, const glm::mat4 &view,
                  const glm::mat4 &projection, const glm::vec3 cameraPos,
                  glm::vec4 clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -1e9)) {
        flowerShader.bind();
        // we need to map time to a texture of lava
        int currentTextureIndex = flowerManager.getTextureIndex(glfwGetTime(), texFlower.size());

        texFlower[currentTextureIndex].bind(0);
        glUniform1i(flowerShader.getUniformLocation("numLights"), (int) lights.size());
        for (int i = 0; i < lights.size(); i++) {
            std::string idx = std::to_string(i);
            glUniform3fv(flowerShader.getUniformLocation(("lights[" + idx + "].position").c_str()), 1,
                         glm::value_ptr(lights[i].position));
            glUniform3fv(flowerShader.getUniformLocation(("lights[" + idx + "].color").c_str()), 1,
                         glm::value_ptr(lights[i].color));
            glUniform1f(flowerShader.getUniformLocation(("lights[" + idx + "].intensity").c_str()),
                        lights[i].intensity);
        }
        glUniform1i(flowerShader.getUniformLocation("texFlower"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxManager.getIBL()->getIrradianceMap());
        glUniform1i(flowerShader.getUniformLocation("irradianceMap"), 1);
        glUniformMatrix4fv(flowerShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(flowerShader.getUniformLocation("projection"), 1, GL_FALSE,
                           glm::value_ptr(projection));
        glUniform3fv(flowerShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform4fv(flowerShader.getUniformLocation("plane"), 1, glm::value_ptr(clipPlane));

        flowerManager.updateTransforms(glfwGetTime());
        // first draw stem
        glUniformMatrix4fv(flowerShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(flowerManager.getStemTransform() * model));
        glUniform1i(flowerShader.getUniformLocation("flowerComponent"), 0);
        glUniform3fv(flowerShader.getUniformLocation("pickedColor"), 1, glm::value_ptr(flowerManager.stemColor));
        flowerManager.stem.mesh.draw(flowerShader);

        // draw the center of the flower
        glUniformMatrix4fv(flowerShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(flowerManager.getCenterTransform() * model));
        glUniform1i(flowerShader.getUniformLocation("flowerComponent"), 1);
        glUniform3fv(flowerShader.getUniformLocation("pickedColor"), 1, glm::value_ptr(flowerManager.centerColor));
        flowerManager.center.mesh.draw(flowerShader);

        glUniform1i(flowerShader.getUniformLocation("flowerComponent"), 2);
        for (int p = 0; p < flowerManager.petals.size(); p++)
        {
            std::vector<FlowerComponent>* currentPetal = flowerManager.petals[p];
            std::vector<FlowerComponent>& petalVec = *currentPetal;
            std::vector<glm::mat4> petalTransforms = flowerManager.getPetalTransform(p);
            for (int i = 0; i < petalTransforms.size(); i++)
            {
                glm::mat4 m = petalTransforms[i];
                glUniformMatrix4fv(flowerShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(m * model));

                petalVec[i].mesh.draw(flowerShader);
            }
        }
    }

    void update() {
        while (!m_window.shouldClose()) {
            // This is your game loop
            // Put your real-time logic and rendering in here
            m_window.updateInput();

            imgui();

            glViewport(0, 0, m_window.getWindowSize().x, m_window.getWindowSize().y);
            // glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glBindFramebuffer(GL_FRAMEBUFFER, bloom.m_hdrFBO);
            GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, attachments);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            Light sun = lightManager.sun(skyboxManager.getCurrentType(), walkingPlayer.newPos);
            std::vector<Light> lights = lightManager.getAllLights();
            lights.insert(lights.begin(), sun);

            bool curvedSeabed = infiniteGround.hasRandomTiles;

            const glm::mat4 view = cameraController.trackball.viewMatrix();
            const glm::mat4 projection = cameraController.trackball.projectionMatrix();
            constexpr glm::mat4 model{1.0f};
            glm::vec3 cameraPos = cameraController.trackball.position();
            glm::vec3 cameraDir = cameraController.trackball.forward();

            infiniteGround.update(walkingPlayer.newPos, materialManager);
            infiniteGround.updateMaterial(materialManager.getCurrentMaterial(), materialManager);
            infiniteGround.updateType(materialManager.getCurrentType());
            gems.updateGround(infiniteGround);
            gems.updatePlayerPosition(walkingPlayer.newPos);
            gems.generateGems();

            glEnable(GL_CLIP_DISTANCE0);

            float waterHeight = infiniteGround.getHeight();
            float heightOffset = infiniteGround.getCurrentGroundBlock(walkingPlayer.newPos).heightScale;

            captureWaterSurroundings(sceneFBORefraction, cameraPos, model, view, projection,
                                     glm::vec4(0.0f, waterHeight + heightOffset, 0.0f, infiniteGround.getHeight()),
                                     lights,
                                     curvedSeabed);

            glm::vec3 reflectedPos = cameraPos;
            reflectedPos.y = 2.0f * waterHeight - heightOffset - cameraPos.y;
            glm::vec3 reflectedDir = cameraDir;
            reflectedDir.y = -cameraDir.y;


            glm::mat4 viewReflected = glm::lookAt(reflectedPos,
                                                  reflectedPos + reflectedDir,
                                                  glm::vec3(0.0f, 1.0f, 0.0f));

            captureWaterSurroundings(sceneFBOReflection, reflectedPos, model, viewReflected, projection,
                                     glm::vec4(0.0f, -(waterHeight + heightOffset), 0.0f, -infiniteGround.getHeight()),
                                     lights, curvedSeabed);

            glDisable(GL_CLIP_DISTANCE0);

            captureCrystalRefraction(cameraPos, model, view, projection, lights);
            drawSky(skyboxShader, skybox, skyboxManager.getCurrentCubemap(), view, projection);

            infiniteGround.drawAll(groundShader, waterShader, skyboxManager.getCurrentCubemap(),
                                   sceneTexColorRefraction, sceneTexColorReflection,
                                   *skyboxManager.getIBL(),
                                   model, view, projection, lights, cameraPos, materialManager);
            if (materialManager.getCurrentType() == GroundType::WATER && !infiniteGround.hasRandomTiles) {
                drawSeabed(cameraPos, model, view, projection, lights);
            }


            drawCrystalGuy(idlePlayer, lights, model, view, projection, cameraPos);

            for (GPUMesh &mesh: props) {
                drawProp(mesh, lights, model, view, projection, cameraPos);
            }

            drawFlower(lights, model, view, projection, cameraPos);

            if (cameraController.displayCurve && !cameraController.enableConstantSpeed) // draw normal curve
            {
                cameraController.updateCurveParameters();
                GPUMesh curve = createCurve(cameraController.curve0, cameraController.curve1, cameraController.curve2);
                drawBezier(curve, lights, model, view, projection, cameraPos);
            }

            if (cameraController.displayCurve && cameraController.enableConstantSpeed) // draw constant curve
            {
                cameraController.updateCurveParameters();
                cameraController.buildConstantSpeedLookupPoints();
                GPUMesh curve = createCurveFromPoints(cameraController.spacedPoints);
                drawBezier(curve, lights, model, view, projection, cameraPos,  cameraController.spacedPoints.size());
            }
            if (flowerManager.enableParticles)
            {
                flowerManager.updateParticles();
                drawParticles(lights, model, view, projection, cameraPos);
            }

            gems.drawAll(gemShader, model, view, projection, cameraPos, lights);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            bloom.render(0);

            m_window.swapBuffers();
        }
        cleanUp();
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods) {
        if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
            shiftPressed = true;

        std::cout << "Key pressed: " << key << std::endl;

        switch (key) {
            case GLFW_KEY_L: {
                if (shiftPressed)
                    lightManager.addLight(Light{cameraController.trackball.position(), glm::vec3(1)});
                else if (!lightManager.getAllLights().empty())
                    lightManager.setLightPosition(cameraController.trackball.position());
                break;
            }
            case GLFW_KEY_UP: walkingPlayer.moveForward();
                break;
            case GLFW_KEY_DOWN: walkingPlayer.moveBackward();
                break;
            case GLFW_KEY_LEFT: walkingPlayer.moveLeft();
                break;
            case GLFW_KEY_RIGHT: walkingPlayer.moveRight();
                break;
        }
    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int key, int mods) {
        if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
            shiftPressed = false;
        switch (key) {
            case GLFW_KEY_UP: walkingPlayer.stopMovingForward();
                break;
            case GLFW_KEY_DOWN: walkingPlayer.stopMovingBackward();
                break;
            case GLFW_KEY_LEFT: walkingPlayer.stopMovingLeft();
                break;
            case GLFW_KEY_RIGHT: walkingPlayer.stopMovingRight();
                break;
        }
    }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2 &cursorPos) {
        std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods) {
        std::cout << "Pressed mouse button: " << button << std::endl;
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods) {
        std::cout << "Released mouse button: " << button << std::endl;
    }

    /**
     * When window is resized, the buffers that capture the character and water surroundings
     * have to also be resized accordingly, so that the refractions are calculated correctly.
     * Otherwise, artifact stripes begin to appear near the edges of the window
     * @param size window size
     */
    void onResize(const glm::ivec2 &size) {
        createFrameBuffer(sceneFBOCrystal,
                          sceneTexColorCrystal,
                          sceneTexDepthCrystal,
                          sceneDepthBufferCrystal);
        createFrameBuffer(sceneFBORefraction,
                          sceneTexColorRefraction,
                          sceneTexDepthRefraction,
                          sceneDepthBufferRefraction);
        createFrameBuffer(sceneFBOReflection,
                          sceneTexColorReflection,
                          sceneTexDepthReflection,
                          sceneDepthBufferReflection);
        bloom.initBloom(size.x, size.y, 0);
        gems.updateBloom(bloom);
    }

private:
    Window m_window;

    CameraController cameraController{m_window};

    LightManager lightManager;
    Flower flowerManager;

    Bloom bloom;

    Shader crystalGuyShader;
    Shader skyboxShader;
    Shader groundShader;
    Shader waterShader;
    Shader simpleTextureShader;
    Shader gemShader;
    Shader flowerShader;
    Shader particleShader;

    GPUMesh idlePlayer;
    AnimatedMesh walkingPlayer;
    std::vector<GPUMesh> props;
    GemManager gems;
    GPUMesh skybox;

    Texture texCrystal;
    Texture texWell;
    std::vector<Texture> texFlower;
    MaterialManager materialManager;
    SkyboxManager skyboxManager;

    InfiniteGround infiniteGround;
    InfiniteGround infiniteSeabed;

    // To render the scene behind the crystal guy, so he knows what to refract
    GLuint sceneFBOCrystal;
    GLuint sceneFBORefraction;
    GLuint sceneFBOReflection;
    GLuint sceneDepthBufferCrystal;
    GLuint sceneDepthBufferRefraction;
    GLuint sceneDepthBufferReflection;
    Texture sceneTexColorCrystal;
    Texture sceneTexDepthCrystal;
    Texture sceneTexColorRefraction;
    Texture sceneTexDepthRefraction;
    Texture sceneTexColorReflection;
    Texture sceneTexDepthReflection;

    bool m_useMaterial{true};
};

int main() {
    Application app;
    app.update();

    return 0;
}
