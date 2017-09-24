#pragma once

#include <cstdint>
#include <thread>
#include <zmq.hpp>
#include "Worker.hpp"

class ProxyServer {
public:
    ProxyServer(uint32_t numIoThreads, uint32_t numWorkerThreads);

    void start();
    void stop();

private:

    void launchWorkers();

    zmq::context_t context;
    zmq::socket_t client;
    zmq::socket_t worker;
    const uint32_t numWorkerThreads;
    std::vector<Worker> workers;

};
