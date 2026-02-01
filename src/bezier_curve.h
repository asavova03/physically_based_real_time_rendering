#pragma once
#include "light_manager.h"
#include "mesh.h"
#include "texture.h"
#include "framework/trackball.h"

GPUMesh createCurve(const std::vector<glm::vec3>& curve0, const std::vector<glm::vec3>& curve1, const std::vector<glm::vec3>& curve2);
glm::vec3 getPointOnCurveInCurve(float t, std::vector<glm::vec3> curve0, std::vector<glm::vec3> curve1, std::vector<glm::vec3> curve2);
GPUMesh createCurveFromPoints(std::vector<glm::vec3> curve);
glm::vec3 getPointOnOneCurve(float t, std::vector<glm::vec3> curve);
glm::vec3 getDirectionInCurve(float t, std::vector<glm::vec3> directions, const Trackball& trackball);
