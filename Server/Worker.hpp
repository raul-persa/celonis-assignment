#pragma once

#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <functional>

class Worker {
public:
    Worker(zmq::context_t &context);
    Worker(Worker& other) = delete;
    Worker(Worker&& other);

    Worker& operator=(Worker& other) = delete;
    Worker& operator=(Worker&& other);

    ~Worker() = default;

    void start();
    void stop();
private:
    void doWork();
    std::thread workerThread;
    std::reference_wrapper<zmq::context_t> context;
    std::atomic_bool stopRequest;
};
