#include "gaze_predictor.h"
#include <stdexcept>
#include <cmath>

GazePredictor::GazePredictor(const std::string& model_path)
    : env(ORT_LOGGING_LEVEL_WARNING, "saccade_predictor"),
      session_options(),
      session(nullptr),
      memory_info(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      input_shape{1, 10, 2}
{
    session_options.SetIntraOpNumThreads(1);
    session = Ort::Session(env, model_path.c_str(), session_options);

    input_names[0] = "input";
    output_names[0] = "output";
}

std::pair<float, float> GazePredictor::predict(const std::deque<std::array<float, 2>> &history)
{
    if (history.size() != 10)
        throw std::runtime_error("History must contain exactly 10 gaze points");

    std::vector<float> input_tensor_values;
    for (const auto &pt : history)
    {
        input_tensor_values.push_back(pt[0]);
        input_tensor_values.push_back(pt[1]);
    }

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        input_tensor_values.data(),
        input_tensor_values.size(),
        input_shape.data(),
        input_shape.size()
    );

    auto output_tensors = session.Run(
        Ort::RunOptions{nullptr},
        input_names,
        &input_tensor,
        1,
        output_names,
        1
    );

    float* output = output_tensors.front().GetTensorMutableData<float>();
    return {output[0], output[1]};
}
