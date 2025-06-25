// gaze_predictor.h
#pragma once

#include <vector>
#include <string>
#include <utility>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <deque>

class GazePredictor
{
public:
    GazePredictor(const std::string& model_path);
    std::pair<float, float> predict(const std::deque<std::array<float, 2>> &history);


private:
    Ort::Env env;
    Ort::SessionOptions session_options;
    Ort::Session session;
    Ort::MemoryInfo memory_info;

    std::array<int64_t, 3> input_shape;
    const char* input_names[1];
    const char* output_names[1];
};
