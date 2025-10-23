
#pragma once
#include <glm/glm.hpp>
#include <utility>
#include <string>
#include <vector>

typedef struct
{
    int ts;
    int event;
    double x;
    double y;
    double xT;
    double yT;

} Gaze;

float deg2rad(float deg);
glm::vec2 gazeAngleToNorm(float degX, float degY);
std::pair<float, float> pixelsToDegreesFromNormalized(float normX, float normY);
float angleToNormRadius(float deg, float diagInInches, float distMM, int scrW, int scrH);
float pixelsToDegreesFromNormalizedRadius(float norm_radius);
void saveGazeRecords(const std::vector<Gaze> &gaze_record, const std::string &path);
void savePerformanceRecord(const std::string &path, const std::string &name, double alpha, double beta, double perf);