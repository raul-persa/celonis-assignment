#include "ProxyServer.hpp"

ProxyServer::ProxyServer(uint32_t numIoThreads, uint32_t numWorkerThreads)
    : context( numIoThreads)
    , client(context, ZMQ_ROUTER)
    , worker(context, ZMQ_DEALER)
    , numWorkerThreads(numWorkerThreads)
{

}

void ProxyServer::start()
{
    client.bind ("tcp://*:5555");
    worker.bind ("inproc://workers");

    launchWorkers();

    std::thread proxyThread(zmq::proxy, (void * ) client, (void * ) worker, nullptr);
    proxyThread.detach();
}

void ProxyServer::stop() {
    for (Worker &w : workers) {
        w.stop();
    }
}

void ProxyServer::launchWorkers() {
    //  Launch pool of worker threads
    for (uint32_t tId = 0; tId < numWorkerThreads; tId++) {
        Worker w(context);
        w.start();
        workers.emplace_back(std::move(w));
    }
}
