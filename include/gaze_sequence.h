#pragma once
#include <vector>
#include <string>

typedef struct
{
    int ts;
    int event;
    double x;
    double y;
    double xT;
    double yT;

} Gazes;

class GazeSequence
{
public:
    GazeSequence(const std::string &path, double og_sampling_rate, double target_sampling_rate);

    Gazes getLatestGaze(int ts);
    double getLastTimestamp();
    void resetCursor();

private:
    std::vector<Gazes> gazeSequence;
    size_t cursor = 0;
    void readSequence(const std::string &path);
    void undersample(double og_sampling_rate, double target_sampling_rate);
};
