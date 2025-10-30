#include "utils.h"
#include "constants.h"
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>
#include <utility>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

float angleToNormRadius(float deg, float diagInInches, float distMM, int scrWidth, int scrHeight)
{
    float diagPx = std::sqrt(scrWidth * scrWidth + scrHeight * scrHeight);
    float diagMM = diagInInches * 25.4f;
    float pixelSizeMM = diagMM / diagPx;

    float rad = deg2rad(deg);
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

void saveGazeRecords(const std::vector<Gaze> &gaze_record, const std::string &path)
{

    std::ofstream ofs(path);
    if (!ofs.is_open())
    {
        std::cerr << "[saveGazeRecords] error: cannot open file " << path << '\n';
        return;
    }

    ofs << "x,y,xT,yT\n";
    for (size_t i = 0; i < gaze_record.size(); i++)
    {
        ofs << gaze_record[i].x << ", " << gaze_record[i].y << ", " << gaze_record[i].xT << " ," << gaze_record[i].yT << " \n";
    }
    ofs.close();
}

void savePerformanceRecord(const std::string &path, const std::string &name, std::string gazeInputFileName, double alpha, double beta, double perf)
{
    bool file_exists = std::filesystem::exists(path);

    std::ofstream ofs(path, std::ios::app);
    if (!ofs.is_open())
    {
        std::cerr << "[savePerformanceRecord] error: cannot open file " << path << '\n';
        return;
    }

    if (!file_exists)
        ofs << "name,gaze_sequence, alpha,beta,perf,quality\n";

    ofs << name << "," << gazeInputFileName << "," << alpha << "," << beta << "," << perf << ",\n";

    ofs.close();
}