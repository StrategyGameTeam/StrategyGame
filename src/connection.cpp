#include "connection.hpp"
#include <iostream>
#include <thread>

Connection::Connection(const std::string &addr, unsigned int port) {
    this->m_addr = addr;
    this->m_port = port;
    this->m_loop = uvw::Loop::getDefault();
    this->m_tcp = this->m_loop->resource<uvw::TCPHandle>();

    this->m_tcp->on<uvw::ErrorEvent>([this](const auto &evt, auto &) {
        this->onError(evt);
    });
    this->m_tcp->on<uvw::DataEvent>([this](const auto &evt, auto &) {
        this->onData(evt);
    });
    this->m_tcp->on<uvw::CloseEvent>([this](const auto &evt, auto &) {
        this->onClose(evt);
    });
    this->m_tcp->on<uvw::ConnectEvent>([this](const auto &evt, auto &) {
        this->onConnected(evt);
    });


    this->m_tcp->connect(this->m_addr, this->m_port);
    this->m_read_thread = std::thread([&]() {
        this->m_tcp->read();
        this->m_loop->run();
    });
}

Connection::~Connection() {
    this->m_loop->stop();
    this->m_tcp->close();
    this->m_loop->close();
}

void Connection::onError(const uvw::ErrorEvent &evt) {
    std::cerr << evt.what() << std::endl;
}

void Connection::onClose(const uvw::CloseEvent &evt) {
    std::cout << "Disconnected" << std::endl;
}

void Connection::onData(const uvw::DataEvent &evt) {
    std::cout << "Received data: " << evt.data << std::endl;
}

void Connection::onConnected(const uvw::ConnectEvent &evt) {
    std::cout << "Connected to: " << this->m_addr << ":" << this->m_port << std::endl;
}

void Connection::write(char *data, unsigned int len) {
    this->m_tcp->write(data, len);
}
