#include "uvw.hpp"
#include <memory>
#include <iostream>

constexpr const char* addr = "127.0.0.1";
constexpr const int port = 4242;

void acceptClient(uvw::TCPHandle &srv);

int main() {
    auto loop = uvw::Loop::getDefault();
    std::shared_ptr<uvw::TCPHandle> tcp = loop->resource<uvw::TCPHandle>();

    tcp->once<uvw::ListenEvent>([](const uvw::ListenEvent &, uvw::TCPHandle &srv) {
        acceptClient(srv);
    });

    tcp->bind(addr, port);
    tcp->listen();
    std::cout << "Server running at: " << addr << ":" << port << std::endl;
    loop->run();
}

void acceptClient(uvw::TCPHandle &srv) {
    //Allocate new TCPHandle for a client
    auto client = srv.loop().resource<uvw::TCPHandle>();

    //Listeners
    client->on<uvw::EndEvent>([](const uvw::EndEvent &, uvw::TCPHandle &client) {
        client.close();
    });
    client->on<uvw::DataEvent>([](const uvw::DataEvent &evt, uvw::TCPHandle &client) {
        std::cout << "[" << client.peer().ip << "]" << "Data reveived: " << evt.data << std::endl;
    });

    //Accept client
    srv.accept(*client);
    std::cout << "Client conntected: " << client->peer().ip << std::endl;
    client->read();
}