//
// Created by raul on 24.09.17.
//

#include <iostream>
#include "Worker.hpp"
#include "../DataTypes/RequestTypeEnum.hpp"

Worker::Worker(zmq::context_t &context) : context(context), stopRequest(false) {

}

void Worker::start() {
    workerThread = std::thread(& Worker::doWork, this);
}

void Worker::stop() {
    stopRequest = true;
    workerThread.join();
}

void Worker::doWork() {
    using namespace std::literals::chrono_literals;

    zmq::socket_t socket(context, ZMQ_REP);
    socket.connect("inproc://workers");

    while (!stopRequest) {
        //  Wait for next request from client
        zmq::message_t request;
        socket.recv(&request);
        char *data = (char *) request.data();
        char *initData = data;

        size_t remainingMessageSize = request.size();

        if (remainingMessageSize < sizeof(RequestTypeEnum) + sizeof(char)) {
            //drop message
            zmq::message_t reply(7);
            memcpy((void *) reply.data(), "INVALID", 7);
            socket.send(reply);
            continue;
        }

        RequestTypeEnum reqType = *((RequestTypeEnum *) data);
        data += sizeof(RequestTypeEnum);
        remainingMessageSize -= sizeof(RequestTypeEnum);

        std::string key;
        std::string value;
        std::string reqStr;

        switch (reqType) {
            case PUT: {
                reqStr = "PUT";
                unsigned long long valueLen = *((unsigned long long *) data);
                data += sizeof(unsigned long long);
                remainingMessageSize -= sizeof(unsigned long long);
                if (remainingMessageSize < valueLen) {
                    //drop message
                    std::cout << "valueLen: " << valueLen << " remaining size " << remainingMessageSize << std::endl;
                    //todo: reply with invalid
                    break;
                }
                key = std::string((char *) data, valueLen);
                data += valueLen;
                remainingMessageSize -= valueLen;
                value = std::string((char *) data, remainingMessageSize);
                break;
            }
            case GET: {
                reqStr = "GET";
                key = std::string((char *) data, remainingMessageSize);
                break;
            }
            case DEL: {
                reqStr = "DEL";
                key = std::string((char *) data, remainingMessageSize);
                break;
            }
            default : {
                //todo: reply with invalid
                reqStr = "UNKNOWN: ";
                reqStr += reqType;
            }
        }

        std::cout << "Received request [" << reqStr << "]: (" << key << "," << value << ")" << " of size "
                  << request.size() << std::endl;
//        std::this_thread::sleep_for(1s);

        //  Send reply back to client
        zmq::message_t reply(6);
        memcpy((void *) reply.data(), "World", 6);
        socket.send(reply);
    }
    socket.close();
}

Worker::Worker(Worker &&other)
        : workerThread(std::move(other.workerThread))
        , context(other.context)
        , stopRequest(other.stopRequest.load())
{

}


Worker& Worker::operator=(Worker&& other)
{
    workerThread = std::move(other.workerThread);
    context = other.context;
    stopRequest = other.stopRequest.load();
}
