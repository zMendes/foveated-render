
#pragma once
#include <glm/glm.hpp>
#include <utility>
#include <string>
#include <vector>

struct Gaze
{
    double timestamp;
    float x;
    float y;
};

float deg2rad(float deg);
glm::vec2 gazeAngleToNorm(float degX, float degY);
std::pair<float, float> pixelsToDegreesFromNormalized(float normX, float normY);
float angleToNormRadius(float deg, float diagInInches, float distMM, int scrW, int scrH);
float pixelsToDegreesFromNormalizedRadius(float norm_radius);
float computeCircleCoverage(glm::vec2 center1, float r1, glm::vec2 center2, float r2);
bool loadGazeSequence(const std::string& path, std::vector<Gaze>& gazeSeq);
