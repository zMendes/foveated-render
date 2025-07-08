#include "pupil.h"
#include "utils.h"
#include <msgpack.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <chrono>

Pupil::Pupil(std::deque<std::array<float, 2>>& shared_history, std::mutex& shared_mutex, const std::string& host, int request_port) :
    ctx_(1),
    sub_port_(""),
    gaze_mutex_(shared_mutex),
    request_address_("tcp://" + host + ":" + std::to_string(request_port)),
    subscriber_address_(""),
    should_run_(false),
    gaze_history_(shared_history)
{}


bool Pupil::connect() {
    std::cout << "Connecting pupil..." << std::endl;
    try {
        zmq::socket_t req_socket(ctx_, zmq::socket_type::req);
        req_socket.set(zmq::sockopt::rcvtimeo, 5000);
        req_socket.connect(request_address_);
        std::cout << "Connected..." << std::endl;

        zmq::message_t request("SUB_PORT", 8);
        req_socket.send(request, zmq::send_flags::none);
        std::cout << "Sending subscription port request..." << std::endl;

        zmq::message_t reply;
        req_socket.recv(reply, zmq::recv_flags::none);
        std::cout << "Reply received..." << std::endl;

        if (reply.size() == 0){
            return false;
        }


        sub_port_ = std::string(static_cast<char*>(reply.data()), reply.size());
        subscriber_address_ = "tcp://localhost:" + sub_port_;

        std::cout << "Connected to Pupil. SUB_PORT: " << sub_port_ << std::endl;
        return true;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to connect to Pupil: " << e.what() << std::endl;
        return false;
    }
}

void Pupil::start() {
    should_run_ = true;
    listen_thread_ = std::thread(&Pupil::listen, this);
}

void Pupil::stop() {
    should_run_ = false;
    if (listen_thread_.joinable()) {
        listen_thread_.join();
    }
}

void Pupil::listen() {
    try {
        zmq::socket_t sub_socket(ctx_, zmq::socket_type::sub);
        sub_socket.connect(subscriber_address_);
        sub_socket.set(zmq::sockopt::subscribe, "gaze.");

        while (should_run_) {
            zmq::message_t topic_msg;
            zmq::message_t payload_msg;

            sub_socket.recv(topic_msg, zmq::recv_flags::none);
            sub_socket.recv(payload_msg, zmq::recv_flags::none);

            msgpack::object_handle oh = msgpack::unpack(static_cast<const char*>(payload_msg.data()), payload_msg.size());
            msgpack::object obj = oh.get();

            std::map<std::string, msgpack::object> msg;
            obj.convert(msg);

            if (msg.find("norm_pos") != msg.end()) {
                std::vector<double> norm_pos;
                msg["norm_pos"].convert(norm_pos);
                if (norm_pos.size() == 2) {
                    std::lock_guard<std::mutex> lock(gaze_mutex_);
                std::pair<float, float> deg = pixelsToDegreesFromNormalized(static_cast<float>(norm_pos[0]), static_cast<float>(norm_pos[1]));
                //gaze_history_.push_back({ deg.first, deg.second });
                gaze_history_.push_back({static_cast<float>(norm_pos[0]),static_cast<float>(norm_pos[1])});
                    if (gaze_history_.size() > 10)
                        gaze_history_.pop_front();
                }
            }
        }
    } catch (const zmq::error_t& e) {
        std::cerr << "Error in listen thread: " << e.what() << std::endl;
    }
}
