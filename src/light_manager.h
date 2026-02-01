#pragma once
#include <vector>
#include <glm/vec3.hpp>

#include "skybox_manager.h"

struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float intensity = 1.0f;
};

class LightManager {
public:
    LightManager() {
        // Default colored lights near origin
        m_lights = {
            {glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), 0.5f},
            {glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.5f},
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), 0.5f},
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 1.0f), 0.5f}
        };
        m_selectedIndex = 0;
    }

    /**
     * Reset returns back to default lights.
     */
    void reset() {
        m_lights.clear();
        m_lights = {
            // {glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), 0.5f},
            // {glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.5f},
            // {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), 0.5f},
            // {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 1.0f), 0.5f}
        };
        m_selectedIndex = 0;
    }

    /**
     * Select the next light (moves the selected index)
     */
    void selectNext() {
        m_selectedIndex = (m_selectedIndex + 1) % (m_lights.size() + 1);
    }

    /**
     * Select the previous light source.
     */
    void selectPrevious() {
        if (m_selectedIndex == 0)
            m_selectedIndex = m_lights.size() - 1;
        else
            --m_selectedIndex;
    }

    const size_t getSelectedIndex() const { return m_selectedIndex; }

    const Light getSelectedLight() const {
        if (m_selectedIndex == 0) {
            return sunlight;
        }
        return m_lights[m_selectedIndex - 1];
    }

    const std::vector<Light> &getAllLights() const { return m_lights; }

    /**
     * Adds a new light at the end of the list of lights
     * @param light a new light
     */
    void addLight(const Light &light) { m_lights.push_back(light); }

    /**
     * Selects the lamp at the given index
     * @param index some number
     */
    void setSelectedIndex(size_t index) {
        if (index <= m_lights.size()) {
            m_selectedIndex = index;
        }
    }

    /**
     * Changes the selected light to some new light
     * @param light some new light
     */
    void setLight(const Light &light) {
        if (m_selectedIndex == 0) {
            sunlight = light;
        }
        m_lights[m_selectedIndex - 1] = light;
    }

    /**
     * Moves the selected light to some chosen position
     * @param position some position in 3D space
     */
    void setLightPosition(const glm::vec3 &position) {
        if (m_selectedIndex == 0) {
            sunlight.position = position;
        }
        m_lights[m_selectedIndex - 1].position = position;
    }

    /**
     * Sets the color of the selected light to some given color
     * @param color some color
     */
    void setLightColor(const glm::vec3 &color) {
        if (m_selectedIndex == 0) {
            sunlight.color = color;
        }
        m_lights[m_selectedIndex - 1].color = color;
    }

    Light &sun(const SkyboxType &sky, glm::vec3 playerPos) {
        skyboxType = sky;
        if (skyboxType == SkyboxType::DAY) {
            sunlight.position = playerPos + glm::vec3(70.0f, 15.0f, -70.0f);
        } else if (skyboxType == SkyboxType::SUNSET) {
            sunlight.position = playerPos + glm::vec3(-100.0f, 10.0f, 20.0f);
        } else {
            sunlight.position = playerPos + glm::vec3(10.0f, 30.0f, -100.0f);
        }
        return sunlight;
    }

    void resetSun(const SkyboxType &sky, glm::vec3 playerPos) {
        skyboxType = sky;
        if (skyboxType == SkyboxType::DAY) {
            sunlight = {playerPos + glm::vec3(70.0f, 15.0f, -70.0f), glm::vec3(1.0f, 1.0f, 0.97f), 0.7f};
        } else if (skyboxType == SkyboxType::SUNSET) {
            sunlight = {playerPos + glm::vec3(-100.0f, 10.0f, 20.0f), glm::vec3(1.0f, 0.5f, 0.3f), 0.9f};
        } else {
            sunlight = {playerPos + glm::vec3(10.0f, 30.0f, -100.0f), glm::vec3(0.7f, 0.5f, 1.1f), 0.4f};
        }
    }

    void renderUI() {
        ImGui::Separator();
        ImGui::Text("Light Controls");

        std::vector<std::string> itemStrings;
        itemStrings.reserve(m_lights.size() + 1);
        itemStrings.push_back("Sunlight");
        for (size_t i = 0; i < m_lights.size(); ++i)
            itemStrings.push_back("Light " + std::to_string(i + 1));

        std::vector<const char *> itemCStrings;
        itemCStrings.reserve(itemStrings.size());
        for (const auto &s: itemStrings)
            itemCStrings.push_back(s.c_str());

        int tempSelectedItem = static_cast<int>(m_selectedIndex);
        if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(),
                           static_cast<int>(itemCStrings.size()), 4)) {
            setSelectedIndex(static_cast<size_t>(tempSelectedItem));
        }

        ImGui::Separator();
        if (ImGui::Button("Add Light")) {
            glm::vec3 defaultPos = glm::vec3(2.0f, 2.0f, 2.0f);
            addLight({defaultPos, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f});
            setSelectedIndex(m_lights.size());
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove Light")) {
            if (m_selectedIndex > 0 && m_selectedIndex - 1 < m_lights.size()) {
                m_lights.erase(m_lights.begin() + (m_selectedIndex - 1));
                if (m_selectedIndex > m_lights.size())
                    m_selectedIndex = m_lights.size();
            }
        }

        ImGui::Separator();

        Light *light = nullptr;
        if (m_selectedIndex == 0) {
            light = &sunlight;
        } else if (m_selectedIndex - 1 < m_lights.size()) {
            light = &m_lights[m_selectedIndex - 1];
        }

        if (light) {
            if (m_selectedIndex == 0) {
                ImGui::BeginDisabled();
            }
            ImGui::DragFloat3("Position", &light->position.x, 0.1f);
            if (m_selectedIndex == 0) {
                ImGui::EndDisabled();
            }
            ImGui::ColorEdit3("Color", &light->color.x);
            ImGui::SliderFloat("Intensity", &light->intensity, 0.0f, 1.0f);
        }

        ImGui::Separator();
        if (ImGui::Button("Reset Lights")) {
            reset();
            resetSun(skyboxType, glm::vec3(0, 0, 0));
        }
    }

private:
    /**
     * all lights
     */
    std::vector<Light> m_lights;
    /**
     * the index of the selected light
     */
    size_t m_selectedIndex{0};
    Light sunlight = {glm::vec3(-100.0f, 10.0f, 20.0f), glm::vec3(1.0f, 0.5f, 0.3f), 0.9f};
    SkyboxType skyboxType = SkyboxType::SUNSET;
};
