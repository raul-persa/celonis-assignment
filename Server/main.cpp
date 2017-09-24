#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

#include <zmq.hpp>
#include "../DataTypes/RequestTypeEnum.hpp"
#include "ProxyServer.hpp"


void work_funct(zmq::context_t *context) {
    using namespace std::literals::chrono_literals;

    zmq::socket_t socket (*context, ZMQ_REP);
    socket.connect ("inproc://workers");
    while (true) {
        //  Wait for next request from client
        zmq::message_t request;
        socket.recv (&request);
        char * data = (char *) request.data();
        char * initData = data;

        size_t remainingMessageSize = request.size();

        if (remainingMessageSize < sizeof(RequestTypeEnum) + sizeof(char) ) {
            //drop message
            zmq::message_t reply (7);
            memcpy ((void *) reply.data (), "INVALID", 7);
            socket.send (reply);
            continue;
        }

        RequestTypeEnum reqType = *((RequestTypeEnum *) data);
        data += sizeof(RequestTypeEnum);
        remainingMessageSize -= sizeof(RequestTypeEnum);

        std::string key;
        std::string value;
        std::string reqStr;

        switch(reqType) {
            case PUT:
            {
                reqStr = "PUT";
                unsigned long long valueLen = *((unsigned long long *) data);
                data += sizeof(unsigned long long);
                remainingMessageSize -= sizeof(unsigned long long);
                if(remainingMessageSize < valueLen) {
                    //drop message
                    std::cout << "valueLen: " << valueLen << " remaining size " << remainingMessageSize << std::endl;
                    //todo: reply with invalid
                    break;
                }
                key = std::string((char*) data, valueLen);
                data += valueLen;
                remainingMessageSize -= valueLen;
                value = std::string((char *) data, remainingMessageSize);
                break;
            }
            case GET:
            {
                reqStr = "GET";
                key = std::string((char*) data, remainingMessageSize);
                break;
            }
            case DEL:
            {
                reqStr = "DEL";
                key = std::string((char*) data, remainingMessageSize);
                break;
            }
            default :
            {
                //todo: reply with invalid
                reqStr = "UNKNOWN: ";
                reqStr += reqType;
            }
        }

        std::cout << "Received request [" << reqStr << "]: (" << key << "," << value << ")" << " of size " << request.size() << std::endl;
//        std::this_thread::sleep_for(1s);

        //  Send reply back to client
        zmq::message_t reply (6);
        memcpy ((void *) reply.data (), "World", 6);
        socket.send (reply);
    }
}

void server(uint32_t numThreads) {

    //  Prepare our context and sockets
    zmq::context_t context (numThreads);
    zmq::socket_t clients (context, ZMQ_ROUTER);
    clients.bind ("tcp://*:5555");
    zmq::socket_t workers (context, ZMQ_DEALER);
    workers.bind ("inproc://workers");

    std::vector<std::thread> threads;

    //  Launch pool of worker threads
    for (uint32_t tId = 0; tId < numThreads; tId++) {
        threads.emplace_back(std::thread(work_funct, &context));
    }

    for (std::thread& t : threads) {
        t.detach();
    }


    //  Connect work threads to client threads via a queue
    zmq::proxy ((void * ) clients,(void * ) workers, NULL);

}


int main ()
{
    ProxyServer server(4, 10);
    server.start();

    getchar();
    return 0;
}
