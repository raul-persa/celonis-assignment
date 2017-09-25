#include <iostream>

#include "ProxyServer.hpp"

int main ()
{
    ProxyServer server(4, 10);
    server.start();

    getchar();
    server.stop();
    return 0;
}
