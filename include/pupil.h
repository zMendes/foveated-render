#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <array>
#include <string>
#include <zmq.hpp>

class Pupil {
public:
    Pupil(std::deque<std::array<float, 2>>& gaze_history, std::mutex& mutex,
          const std::string& host = "localhost", int request_port = 50020);

    bool connect();
    void start();
    void stop();

private:
    void listen();

    zmq::context_t ctx_;
    std::string sub_port_;
    std::mutex& gaze_mutex_;
    std::string request_address_;
    std::string subscriber_address_;
    std::atomic<bool> should_run_;
    std::thread listen_thread_;
    std::deque<std::array<float, 2>>& gaze_history_;
};
