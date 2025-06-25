#include "gaze_data.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;
extern const float DIST_MM;
extern const float SCR_WIDTH_MM;
extern const float SCR_HEIGHT_MM;

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