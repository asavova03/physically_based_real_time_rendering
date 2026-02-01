//
// Created by ada on 10/24/2025.
//

#include "bezier_curve.h"

#include <iostream>

#include "mesh.h"
#include "texture.h"
#include "framework/trackball.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"

GPUMesh createCurve(const std::vector<glm::vec3>& curve0, const std::vector<glm::vec3>& curve1, const std::vector<glm::vec3>& curve2)
{
    Mesh curve;
    std::vector<Vertex> vertices = {};

    // Generate dummy triangle indices
    for (uint32_t i = 0; i < 60; ++i) {
        curve.triangles.push_back({ 3 * i, 3 * i + 1, 3 * i + 2 });
    }

    for (float t = 0; t < 540; t++)
    {
        float factor = t/180.0f; // map it to a range 0 - 3 because we hae 3 curves
        glm::vec3 pos = getPointOnCurveInCurve(factor, curve0, curve1, curve2);
        vertices.push_back({pos });
    }
    curve.vertices = std::move(vertices);
    curve.material = Material();

    glBufferData(GL_ARRAY_BUFFER, sizeof(curve.vertices), &curve.vertices, GL_STATIC_DRAW);


    curve.material = Material{};


    return GPUMesh(curve);
}

GPUMesh createCurveFromPoints(std::vector<glm::vec3> points)
{
    Mesh curve;
    std::vector<Vertex> vertices = {};
    int numofPoints = points.size();

    // Generate dummy triangle indices
    for (uint32_t i = 0; i < (numofPoints/3); ++i) {
        curve.triangles.push_back({ 3 * i, 3 * i + 1, 3 * i + 2 });
    }

    for (glm::vec3 pos : points)
    {
        vertices.push_back({pos });
    }
    curve.vertices = std::move(vertices);
    curve.material = Material();

    glBufferData(GL_ARRAY_BUFFER, sizeof(curve.vertices), &curve.vertices, GL_STATIC_DRAW);

    // Skybox has no material
    curve.material = Material{};
    std::cout << "Curve size: " << curve.vertices.size() << std::endl;

    return GPUMesh(curve);
}


glm::vec3 getPointOnOneCurve(float t, std::vector<glm::vec3> curve)
{
    const glm::vec3 p0 = curve[0];
    glm::vec3 p1 = curve[1];
    glm::vec3 p2 = curve[2];
    glm::vec3 p3 = curve[3];

    return std::pow((1.0f - t), 3.0f) * p0 + 3.0f * std::pow(1.0f-t, 2.0f) * t * p1 + 3 * (1.0f - t) * std::pow(t,2.0f)*p2+ std::pow(t, 3.0f)*p3;
}

glm::vec3 getPointOnCurveInCurve(float t, std::vector<glm::vec3> curve0, std::vector<glm::vec3> curve1, std::vector<glm::vec3> curve2)
{
    // t determines which curve i am in
    // 0-1 is curve 0
    // 1-2 curve 1
    // 2-3 curve 2
    std::vector<glm::vec3> &curve = curve0;
    if (1 < t && t <= 2)
    {
        curve = curve1;
        t = t - 1.0f; // we have to map t back o to 0-1
    }
    else if (2 < t && t <= 3)
    {
        curve = curve2;
        t = t - 2.0f; // map back to 0-1 range
    }

    return getPointOnOneCurve(t, curve);
}

glm::vec3 getDirectionInCurve(float t, std::vector<glm::vec3> directions, const Trackball& trackball)
{
    // t determines which curve i am in
    // 0-1 is curve 0
    // 1-2 curve 1
    // 2-3 curve 2
    if (directions.size() < 3)
    {
        std::cout<< "direction is too small for bezier calculation" << std::endl;
        return {0, 0, 0};
    }
    // if in first curve
    glm::vec3 &startDir = directions[0];
    glm::vec3 &endDir = directions[1];
    if (1 < t && t <= 2) // in second curve
    {
        startDir = directions[1];
        endDir = directions[2];
        t = t - 1.0f; // we have to map t back o to 0-1
    }
    else if (2 < t && t <= 3)
    {
        startDir = directions[2];
        endDir = directions[3];
        t = t - 2.0f; // map back to 0-1 range
    }

    glm::quat start = glm::rotation(glm::vec3(0,0,1), startDir);
    glm::quat end = glm::rotation(glm::vec3(0,0,1), endDir);


    return glm::eulerAngles(glm::slerp(start, end, t));
}