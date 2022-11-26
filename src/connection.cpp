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
    this->m_tcp->on<uvw::DataEvent>([this](const uvw::DataEvent &evt, auto &) {
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
    std::string data = std::string(evt.data.get());
    auto dividerLoc = data.find('\0');
    auto packetId = data.substr(0, dividerLoc);

    m_handlers[packetId](data.substr(dividerLoc));
}

void Connection::onConnected(const uvw::ConnectEvent &evt) {
    std::cout << "Connected to: " << this->m_addr << ":" << this->m_port << std::endl;
}

template<Packet T>
void Connection::write(const T &packet) {
    std::string packetId = packet.name;
    PacketRegistration<T> packetDecl = m_packets[packetId];
    std::string packetData = packetDecl.serializer(packet);

    std::string data = packetId + '\0' + packetData;
    this->m_tcp->write(data.data(), data.length());
}

template<Packet T>
void Connection::registerPacket(const std::string &name, Serializer<T> serializer,
                                Deserializer<T> deserializer, Handler<T> handler) {
    auto pr = PacketRegistration<T>{serializer, deserializer, handler};
    m_packets.insert(name, pr);
    m_handlers.insert(name, pr.handler);
}
