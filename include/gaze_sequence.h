#pragma once
#include <vector>
#include <string>
#include "utils.h"



class GazeSequence
{
public:
    GazeSequence(const std::string &path, double og_sampling_rate, double target_sampling_rate);

    Gaze getLatestGaze(int ts);
    double getLastTimestamp();
    void resetCursor();

private:
    std::vector<Gaze> gazeSequence;
    size_t cursor = 0;
    void readSequence(const std::string &path);
    void undersample(double og_sampling_rate, double target_sampling_rate);
};
