#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui/imgui.h>
#include <cmath>

#include "bezier_curve.h"
#include "framework/trackball.h"
#include "framework/window.h"
#include "camera_manager.h"

/**
 * Struct holding a camera state
 */
struct CameraSettings {
    glm::vec3 look_at{0.0f, 0.0f, -1.0f};
    glm::vec3 rotations{0.0f, 5.0f, 0.0f};
    float dist = 2.4f;
    float fovy = 50.0f;
};

/**
 * Adding more camera modes happens here.
 */
enum class CameraMode {
    Free,
    CharacterFollow,
    Curve
};

/**
 * Camera controller manages all the different cameras and their logic.
 */
class CameraController
{
public:
    CameraSettings camera;
    CameraMode mode = CameraMode::Free;
    Trackball trackball;

    // --- Shared state ---
    glm::vec3 smoothedTarget{0.0f};
    float smoothedYaw = 0.0f;
    float cameraBaseYaw = 0.0f;
    float camYawOffset = 0.0f;
    float lastTime = 0.0f;

    // --- Character Follow Parameters ---
    float distanceToPlayer = 3.0f;
    float smoothSpeed = 5.0f;
    float followLerp = 0.15f;
    float orbitSensitivity = 0.005f;

    // --- Free Camera Parameters ---
    float freecamSpeed = 3.0f;
    glm::vec3 freecamPosition{0.0f, 2.0f, 5.0f};

    // -- Bezier Curve Camera Paramters ---
    float maxT = 70.0f;
    float minT = 0.0f;
    bool displayCurve = false;
    bool enableSlerp = false;
    bool enableConstantSpeed = false;

    // ideally these parameters would be stored in a struct, however we decalre them explicitly below because of reference issues
    glm::vec3 p0 = glm::vec3(-2.0, 1.0f, -3.3);
    glm::vec3 p1 = glm::vec3(-5.0f, 1.8f, -1.5f);
    glm::vec3 p2 = glm::vec3(-4.7f, 0.1f, -2.8f);
    glm::vec3 p3 = glm::vec3(-3.0f, 0.1f, 2.5f); // first intersection point
    glm::vec3 p4 = p3 - p2 + p3; // this is to make the translation smooth, not the rotation right now. So I want the tangent P3-P2 to overlap with tangent P4 - P3
    glm::vec3 p5 = glm::vec3(1.0f, 0.1f, 1.0f);
    glm::vec3 p6 = glm::vec3(2.26f, 1.8f, 2.0f); // second intersection point
    glm::vec3 p7 = p6 + (p6 - p5);
    glm::vec3 p8 = glm::vec3(2.5f, 0.1f, 0.8f);
    glm::vec3 p9 = glm::vec3(2.8f, 0.1f, -1.0f);

    std::vector<glm::vec3> curve0 = {p0, p1, p2, p3}; // i write them separately because of initialization issues
    std::vector<glm::vec3> curve1 = {p3, p4, p5, p6}; // a curve is described by a list of its control points
    std::vector<glm::vec3> curve2 = {p6, p7, p8, p9};


    std::vector<glm::vec3> directions = {
        glm::normalize(glm::vec3(0,0,0) - p0), // direction at the start of curve 0
        glm::normalize(glm::vec3(0,0,0) - p3), // end of curve 0, start curve1
        glm::normalize(glm::vec3(0, 0, 0) - p6), // end of curve1 start of curve2
        glm::normalize(glm::vec3(0, 0, 0) - p9) // end of all
    };

    // -- constant speed on bezier curves parameters
    std::vector<glm::vec3> spacedPoints = {};
    float spacing = 0.5f;
    float speed = 20.0f; // how many units i want to move every second


    CameraController(Window& window) : trackball({&window, glm::radians(camera.fovy)}) {}

    /**
     * Updates the camera position depending on own logic
     * @param deltaTime how much time has passed since last update
     * @param player holds information about the player rotations and position
     */
    void update(float deltaTime, const AnimatedMesh& player) {
        switch (mode) {
            case CameraMode::CharacterFollow:
                updateCharacterFollow(deltaTime, player);
                break;
            case CameraMode::Free:
                updateFreeCamera(deltaTime);
                break;
            case CameraMode::Curve:
                updateCurveCamera(deltaTime);
                break;
        }
    }

    /**
     * Updates the points on curve calculations since the points for the Bezier curve can be changed from the ui
     */
    void updateCurveParameters()
    {
        p4 = p3 - p2 + p3;
        p7 = p6 + (p6 - p5);
        curve0 = {p0, p1, p2, p3}; // i write them separately because of initialization issues
        curve1 = {p3, p4, p5, p6};
        curve2 = {p6, p7, p8, p9};
        directions = {
            glm::normalize(glm::vec3(0,0,0) - p0), // direction at the start of curve 0
            glm::normalize(glm::vec3(0,0,0) - p3), // end of curve 0, start curve1
            glm::normalize(glm::vec3(0, 0, 0) - p6), // end of curve1 start of curve2
            glm::normalize(glm::vec3(0, 0, 0) - p9) // end of all
        };
    }


    /**
     * Control camera parameters dynamically through the UI.
     */
    void renderUI() {
        const char* modes[] = {"Free", "Character-Follow", "Bezier Curve"};
        int currentMode = static_cast<int>(mode);
        ImGui::Text("Select Camera Preset:");

        if (ImGui::ListBox("##CameraList", &currentMode, modes, IM_ARRAYSIZE(modes), 3)) {
            mode = static_cast<CameraMode>(currentMode);
        }

        ImGui::Separator();
        ImGui::Text("Editing %s", modes[currentMode]);

        // --- Camera controls ---
        // ImGui::DragFloat3("Look At", glm::value_ptr(camera.look_at), 0.01f, -100.0f, 100.0f, "%.2f");
        // glm::vec3 rotationsDeg = glm::degrees(camera.rotations);
        // ImGui::DragFloat3("Rotations (Pitch/Yaw/Roll)", glm::value_ptr(rotationsDeg), 0.1f, -180.0f, 180.0f, "%.1f");
        // camera.rotations = glm::radians(rotationsDeg);
        //
        // ImGui::SliderFloat("FOV (degrees)", &camera.fovy, 10.0f, 120.0f, "%.1f");
        // if (mode == CameraMode::CharacterFollow) {
        //     ImGui::SliderFloat("Distance", &distanceToPlayer,1.0f, 30.0f);
        // } else {
        //     ImGui::SliderFloat("Distance", &camera.dist,1.0f, 30.0f);
        //     distanceToPlayer = camera.dist;
        // }
        // if (mode == CameraMode::Free) {
        //     ImGui::SliderFloat("Freecam Speed", &freecamSpeed, 0.5f, 10.0f);
        //     freecamPosition = trackball.position();
        //     ImGui::Text("Position: (%.2f, %.2f, %.2f)", freecamPosition.x, freecamPosition.y, freecamPosition.z);
        // }
        //
        // trackball.setCamera(camera.look_at, camera.rotations, camera.dist);
        // trackball.setFovy(glm::radians(camera.fovy));
        bool cameraChanged = false;

        cameraChanged |= ImGui::DragFloat3("Look At", glm::value_ptr(camera.look_at), 0.01f, -100.0f, 100.0f, "%.2f");
        glm::vec3 rotationsDeg = glm::degrees(camera.rotations);
        if (ImGui::DragFloat3("Rotations (Pitch/Yaw/Roll)", glm::value_ptr(rotationsDeg), 0.1f, -180.0f, 180.0f, "%.1f")) {
            camera.rotations = glm::radians(rotationsDeg);
            cameraChanged = true;
        }

        cameraChanged |= ImGui::SliderFloat("FOV (degrees)", &camera.fovy, 10.0f, 120.0f, "%.1f");
        if (mode == CameraMode::CharacterFollow) {
            cameraChanged |= ImGui::SliderFloat("Distance", &distanceToPlayer, 1.0f, 30.0f);
        } else {
            if (ImGui::SliderFloat("Distance", &camera.dist, 1.0f, 30.0f)) {
                distanceToPlayer = camera.dist;
                cameraChanged = true;
            }
        }
        if (mode == CameraMode::Free) {
            cameraChanged |= ImGui::SliderFloat("Freecam Speed", &freecamSpeed, 0.5f, 10.0f);
            freecamPosition = trackball.position();
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", freecamPosition.x, freecamPosition.y, freecamPosition.z);
        }

        // Only update trackball if something changed
        if (cameraChanged) {
            trackball.setCamera(camera.look_at, camera.rotations, camera.dist);
            trackball.setFovy(glm::radians(camera.fovy));
        }

        ImGui::Checkbox("Enable Slerp" , &enableSlerp);
        ImGui::Checkbox("Display Bezier Curve Path" , &displayCurve);
        ImGui::SliderFloat("Curve Range (Speed)", &maxT, 2.0f, 100.0f, "%.2f");
        ImGui::SliderFloat3("Point 0", glm::value_ptr(p0), -10.0f, 10);
        ImGui::SliderFloat3("Point 1", glm::value_ptr(p1), -10.0f, 10);
        ImGui::SliderFloat3("Point 2", glm::value_ptr(p2), -10.0f, 10);
        ImGui::SliderFloat3("Point 3", glm::value_ptr(p3), -10.0f, 10);
        ImGui::SliderFloat3("Point 5", glm::value_ptr(p5), -10.0f, 10);
        ImGui::SliderFloat3("Point 6", glm::value_ptr(p6), -10.0f, 10);
        ImGui::SliderFloat3("Point 8", glm::value_ptr(p8), -10.0f, 10);
        ImGui::SliderFloat3("Point 9", glm::value_ptr(p9), -10.0f, 10);
        ImGui::Text("Constant Speed Parameters");
        ImGui::Checkbox("Enable Constant Speed" , &enableConstantSpeed);
        ImGui::SliderFloat("Spacing of Reference Points", &spacing, 0.1f, 1.0f, "%.2f");
        ImGui::SliderFloat("Speed of Constant Curve", &speed, 1.0f, 30.0f, "%.2f");
    }

    /**
     * Creates the lookup table of positions evenly-spaced along the bezier curve to move with constant speed
     */
    void buildConstantSpeedLookupPoints()
    {
        spacedPoints = {};
        float resolution = 1.0f; // how fine we want it, affects how many points we sample
        // calculate evenly spaced points along the curve
        spacedPoints.push_back(curve0[0]); // start from the start
        glm::vec3 previousPoint = p0;
        float distanceSinceLastPoint = 0.0f;
        for (int curveId = 0; curveId < 3; curveId++)
        { // for each curve make a lookup table of evenly spaced points
            std::vector<glm::vec3> curve = curve0; // sadly, we have to manually map the id to curve because we cant have a nested list because reference issues
            switch (curveId)
            {
                case 1:
                    curve = curve1;
                    break;
                case 2:
                    curve = curve2;
                    break;
                default:
                    curve = curve0;
            }
            // i calculate the upper bound on the real curve length by the length of control
            float netLengthOfControl = glm::distance(curve[0], curve[1]) + glm::distance(curve[1], curve[2]) + glm::distance(curve[2], curve[3]);
            float estimatedCurveLength = glm::distance(curve[0], curve[3]) + netLengthOfControl / 2.0f; // take average (kind of) of lower and upper bound
            int numOfDivisions = std::ceil(estimatedCurveLength * resolution * 10); // delta t is 1/divisions
            float t = 0.0f;
            while (t <= 1)
            {
                t += 1.0f / numOfDivisions;
                glm::vec3 pointOnCurve = getPointOnOneCurve(t, curve);
                distanceSinceLastPoint += glm::distance(previousPoint, pointOnCurve);

                while (distanceSinceLastPoint >= spacing)
                {
                    float overShootDistance = distanceSinceLastPoint - spacing;
                    glm::vec3 newPoint = pointOnCurve + glm::normalize(previousPoint - pointOnCurve) * overShootDistance;
                    spacedPoints.push_back(newPoint);
                    distanceSinceLastPoint = overShootDistance;
                    previousPoint = newPoint;

                }
                previousPoint = pointOnCurve;
            }
        }
    }

private:
    // --- Character Follow Camera ---
    void updateCharacterFollow(float deltaTime, const AnimatedMesh& walkingPlayer) {
        // Rotate the camera when dragging with right click
        handleOrbitInput();

        float deltaYawBase = glm::mod(walkingPlayer.rotationY - cameraBaseYaw + glm::pi<float>(), glm::two_pi<float>())
                             - glm::pi<float>();
        cameraBaseYaw += deltaYawBase * (1.0f - expf(-smoothSpeed * deltaTime));

        // Add player look offset only for visual camera rotation
        float targetYaw = cameraBaseYaw + camYawOffset;

        float deltaYaw = glm::mod(targetYaw - smoothedYaw + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
        smoothedYaw += deltaYaw * (1.0f - expf(-smoothSpeed * deltaTime));

        glm::vec3 target = walkingPlayer.newPos + glm::vec3(0.0f, 1.0f, 0.0f);
        smoothedTarget = glm::mix(smoothedTarget, target, followLerp);

        glm::vec3 offset = glm::vec3(
            sin(smoothedYaw),
            distanceToPlayer,
            cos(smoothedYaw)
        );

        camera.look_at = smoothedTarget;
        camera.dist = glm::length(offset);
        camera.rotations = glm::vec3(0.25f, smoothedYaw, 0.0f);
        trackball.setCamera(camera.look_at, camera.rotations, camera.dist);
    }


    /**
     * Update free camera position.
     * @param deltaTime time that has passed since last update, relevant for the WASD movement of the camera
     */
    void updateFreeCamera(float deltaTime) {
        const ImGuiIO& io = ImGui::GetIO();

        glm::vec3 position = trackball.position();
        glm::vec3 look  = trackball.lookAt();

        // Direction vectors based on current trackball orientation
        glm::vec3 forward = glm::normalize(look - position);
        glm::vec3 right   = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up      = glm::cross(right, forward);

        // Move camera position
        glm::vec3 move(0.0f);
        if (io.KeysDown['W']) move += forward;
        if (io.KeysDown['S']) move -= forward;
        if (io.KeysDown['A']) move -= right;
        if (io.KeysDown['D']) move += right;
        if (io.KeysDown['E']) move += up;
        if (io.KeysDown['Q']) move -= up;

        if (glm::length(move) > 0.0f) {
            move = glm::normalize(move) * freecamSpeed * deltaTime;
        }

        // Translate trackball look
        if (glm::length(move) > 0.0f) {
            trackball.translate(move);
        }

        // Update camera struct
        camera.look_at = trackball.lookAt();
        camera.dist    = trackball.distanceFromLookAt();
        camera.rotations = trackball.rotationEulerAngles();
    }

    /**
     * Samples from the lookup table of evenly-spaced positions on the curve. Converts a factor in range (0,3) to a
     * position by calculating which two points in the lookup point it falls in and then interpolating them.
     * @param factor
     * @return
     */
    glm::vec3 getConstantCurvePosition(float factor) // factor is in range 0 - 3
    {
        // I need to transfer time to distance
        float distanceTraveled = speed * factor / 3.0f;
        int index = distanceTraveled / spacing; // get the index of the point we are at at the lookup table
        // now we need to interpolate
        glm::vec3 firstPoint = spacedPoints[index];
        if (spacedPoints.size() < index) // we are at the end of the lookup table
        {
            return firstPoint;
        }
        glm::vec3 secondPoint = spacedPoints[index + 1];

        float factorBetweenPoints = (fmod(distanceTraveled, spacing)) / spacing;

        return firstPoint * (1.0f - factorBetweenPoints) + secondPoint * (factorBetweenPoints);

    }

    /**
     * Maps the time factor into a sampled position from three cubic bezier curves
     * @param deltaTime
     */
    void updateCurveCamera(float deltaTime)
    {
        float timeNow = static_cast<float>(glfwGetTime());
        float factor =  (fmod(timeNow, maxT) / ((maxT - minT))) * 3.0f; // we need a t value between 0 - 3 for interpolation, this is the t

        glm::vec3 pos;

        if (enableConstantSpeed)
        {
            pos = getConstantCurvePosition(factor);
        } else
        {
            pos = getPointOnCurveInCurve(factor, curve0, curve1, curve2);
        }

        glm::vec3 move = pos - trackball.position();

        if (glm::length(move) > 0.0f) // todo check if this is needed
        {
            trackball.translate(move);
        }
        glm::vec3 center = glm::vec3(0,0,0);
        float distToLookAt = glm::distance(center, pos);
        trackball.setDist(distToLookAt);

        glm::vec3 direction = glm::normalize(center - pos); // the direction where I am looking at, funnily different than my lookAt

        float pitch = asin(-direction.y); // first i get the rotation around x, y is the up
        float yaw  = atan2(direction.x, direction.z); // then rotation around y
        float roll = 0.0f; // rotation around z, i put 0 because we are upright and I don't really want to tilt the camera
        glm::vec3 overallRotations = glm::vec3(pitch, yaw, roll);

        if (enableSlerp) // calculate rotations with slerp
        {
            overallRotations = getDirectionInCurve(factor, directions, trackball);
        }
        trackball.setCamera(center, overallRotations,distToLookAt);

        // Update camera struct
        camera.look_at = trackball.lookAt();
        camera.dist    = trackball.distanceFromLookAt();
        camera.rotations = trackball.rotationEulerAngles();
    }

    /**
     * Rotate where the camera is looking through right click and dragging around the screen.
     */
    void handleOrbitInput() {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            ImVec2 drag = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
            camYawOffset -= drag.x * orbitSensitivity;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
        }
    }
};
