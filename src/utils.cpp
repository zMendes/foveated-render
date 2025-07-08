#include "utils.h"
#include "constants.h"
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>
#include <utility>
#include <fstream>
#include <sstream>
#include <iostream>



float angleToNormRadius(float deg, float diagInInches, float distMM, int scrWidth, int scrHeight)
{
    float diagPx = std::sqrt(scrWidth * scrWidth + scrHeight * scrHeight);
    float diagMM = diagInInches * 25.4f;
    float pixelSizeMM = diagMM / diagPx;

    float rad = glm::radians(deg);
    float sizeMM = 2.0f * distMM * std::tan(rad / 2.0f);
    float sizePx = sizeMM / pixelSizeMM;

    float screenSizePx = std::min(scrWidth, scrHeight);
    return (sizePx / screenSizePx) / 2.0f; // radius
}

float deg2rad(float deg)
{
    return deg * 3.1415 / 180.0f;
}

glm::vec2 gazeAngleToNorm(float predicted_x_deg, float predicted_y_deg)
{

    float x_rad = deg2rad(predicted_x_deg);
    float y_rad = deg2rad(predicted_y_deg);

    float x_mm = std::tan(x_rad) * DIST_MM;
    float y_mm = std::tan(y_rad) * DIST_MM;

    float px_x = (x_mm / SCR_WIDTH_MM) * SCR_WIDTH + SCR_WIDTH / 2.0f;
    float px_y = (y_mm / SCR_HEIGHT_MM) * SCR_HEIGHT + SCR_HEIGHT / 2.0f;

    float norm_x = px_x / SCR_WIDTH;
    float norm_y = px_y / SCR_HEIGHT;

    return glm::vec2(norm_x, norm_y);
}
std::pair<float, float> pixelsToDegreesFromNormalized(float norm_x, float norm_y)
{
    float px_x = norm_x * SCR_WIDTH / 2.0;
    float px_y = norm_y * SCR_HEIGHT / 2.0;

    float px_per_mm_x = SCR_WIDTH / SCR_WIDTH_MM;
    float px_per_mm_y = SCR_HEIGHT / SCR_HEIGHT_MM;

    float dx_mm = (px_x) / px_per_mm_x;
    float dy_mm = (px_y) / px_per_mm_y;

    float deg_x = std::atan2(dx_mm, DIST_MM) * (180.0f / static_cast<float>(3.1415));
    float deg_y = std::atan2(dy_mm, DIST_MM) * (180.0f / static_cast<float>(3.1415));

    return {deg_x, deg_y};
}

float computeCircleCoverage(glm::vec2 center1, float r1, glm::vec2 center2, float r2)
{
    float d = glm::distance(center1, center2);

    // No overlap
    if (d >= r1 + r2)
        return 0.0f;

    // Circle 1 fully inside Circle 2
    if (d <= std::abs(r2 - r1) && r2 > r1)
        return 1.0f;

    // Circle 2 fully inside Circle 1
    if (d <= std::abs(r2 - r1) && r1 > r2)
        return (M_PI * r2 * r2) / (M_PI * r1 * r1);

    float r1_sq = r1 * r1;
    float r2_sq = r2 * r2;

    float alpha = std::acos((r1_sq + d * d - r2_sq) / (2.0f * r1 * d));
    float beta = std::acos((r2_sq + d * d - r1_sq) / (2.0f * r2 * d));

    float intersection_area =
        r1_sq * alpha + r2_sq * beta -
        0.5f * std::sqrt(
                   (-d + r1 + r2) *
                   (d + r1 - r2) *
                   (d - r1 + r2) *
                   (d + r1 + r2));

    float circle1_area = M_PI * r1_sq;

    return intersection_area / circle1_area;
}

bool loadGazeSequence(const std::string& path, std::vector<Gaze>& gazeSeq)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open gaze sequence file: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        double timestamp;
        float x, y;
        char comma1, comma2;
        ss >> timestamp >> comma1 >> x >> comma2 >> y;
        if (ss.fail())
        {
            std::cerr << "Malformed line in gaze sequence file: " << line << std::endl;
            continue;
        }
        gazeSeq.push_back({timestamp, x, y});
    }
    return true;
}