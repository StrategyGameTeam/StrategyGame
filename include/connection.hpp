#pragma once
#include <uvw.hpp>
#include <thread>

template<typename T>
using Serializer = std::function<std::string(T &)>;

template<typename T>
using Deserializer = std::function<T(std::string &)>;

template<typename T>
using Handler = std::function<void(T &)>;

struct PacketRegistrationBase{};

template<typename T>
struct PacketRegistration : PacketRegistrationBase {
    Serializer<T> serializer;
    Deserializer<T> deserializer;
    Handler<T> handler;

    void handle(const std::string &data){
        handler(deserializer(data));
    }
};

template <typename T>
concept Packet = requires(T ext) {
{ ext.packetId };
};

struct ChatPacket {
    static const std::string packetId;
    std::string msg;
};
const std::string ChatPacket::packetId = "chat";


struct Connection{
    std::string m_addr;
    unsigned int m_port;
    std::shared_ptr<uvw::Loop> m_loop;
    std::shared_ptr<uvw::TCPHandle> m_tcp;
    std::thread m_read_thread;

    std::unordered_map<std::string, PacketRegistrationBase> m_packets;
    std::unordered_map<std::string, std::function<void(std::string)>> m_handlers;

    Connection(const std::string &, unsigned int);
    ~Connection();

    void onError(const uvw::ErrorEvent &);

    void onClose(const uvw::CloseEvent &);
    void onConnected(const uvw::ConnectEvent &evt);

    template<Packet T>
    void write(const T &packet);

    void onData(const uvw::DataEvent &);

    template<Packet T>
    void registerPacket(const std::string &name, Serializer<T> serializer,
                        Deserializer<T> deserializer, Handler<T> handler);

};