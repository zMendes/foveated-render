#include <string>
#include <vector>
#include <utility>
#include <glm/glm.hpp>

struct Gaze
{
    double timestamp;
    float x;
    float y;
};

bool loadGazeSequence(const std::string &path, std::vector<Gaze> &gazeSeq);

std::pair<float, float> pixelsToDegreesFromNormalized(float norm_x, float norm_y);