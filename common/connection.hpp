#pragma once
#include "uvw.hpp"
#include <iostream>
#include <thread>
#include <queue>

const std::string HOST = "";

struct PacketReader {
    std::vector<char> buf;
    unsigned long idx = 0;

    PacketReader(std::vector<char> &buf): buf(buf){
    }

    std::string readString(){
        auto str = std::string(buf.begin() + idx, std::find(buf.begin()+idx, buf.end(),'\0'));
        idx += str.length() + 1;
        return str;
    }
    char readChar(){
        return buf[idx++];
    }
    int readInt(){
        auto c1 = readChar();
        auto c2 = readChar();
        auto c3 = readChar();
        auto c4 = readChar();
        return (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
    }

    bool readBool(){
        return readChar()>0;
    }
};

struct PacketWriter {
    std::unique_ptr<char*> buf = std::make_unique<char*>(new char[64*1024]);
    unsigned long len = 0;

    void writeString(const std::string &str){
        for (const auto c: str){
            writeChar(c);
        }
        writeChar(0);
    }
    void writeChar(char c){
        (*buf)[len++] = c;
    }
    void writeInt(int n){
        writeChar((n >> 24) & 0xff);
        writeChar((n >> 16) & 0xff);
        writeChar((n >> 8) & 0xff);
        writeChar(n & 0xff);
    }

    void writeBool(bool val) {
        writeChar(val ? 1 : 0);
    }
};

template <typename T>
concept Packet = requires(T ext, PacketWriter &wr) {
    { ext.packetId, ext.serialize(wr) };
};


template<Packet T>
void writePacket(const std::shared_ptr<uvw::TCPHandle> &handle, const std::string &destination, const T &packet) {
    PacketWriter wr;
    wr.writeString(packet.packetId);
    wr.writeString(destination);
    packet.serialize(wr);
    handle->write(*wr.buf, wr.len);
}
template<Packet T>
void writePacket(const std::shared_ptr<uvw::TCPHandle> &handle, const T &packet) {
    writePacket(handle, HOST, packet);
}

struct Connection{
    std::string m_addr;
    unsigned int m_port;
    std::shared_ptr<uvw::Loop> m_loop;
    std::shared_ptr<uvw::TCPHandle> m_tcp;
    std::thread m_read_thread;
    std::queue<PacketReader> tasks;

    std::unordered_map<std::string, std::function<void(PacketReader &)>> m_handlers;

    Connection(const std::string &, unsigned int);
    ~Connection();

    void onError(const uvw::ErrorEvent &);

    void onClose(const uvw::CloseEvent &);
    void onConnected(const uvw::ConnectEvent &evt);

    template<Packet T>
    void write(const T &packet) {
        writePacket(this->m_tcp, packet);
    }
    template<Packet T>
    void write(std::string player, const T &packet) {
        writePacket(this->m_tcp, player, packet);
    }

    void onData(const uvw::DataEvent &);
    void registerPacketHandler(const std::string &name, const std::function<void(PacketReader &)>& handler);

    void clearHandlers(){
        m_handlers.clear();
    }

    void handleTasks(){
        while(tasks.size() > 0){
            auto reader = tasks.front();
            tasks.pop();
            while(reader.idx < reader.buf.size()) {
                auto packetId = reader.readString();
                auto destination = reader.readString();
                std::cout << "Received packet with id: " << packetId << " and destination: " << destination
                          << std::endl;
                if (!m_handlers.contains(packetId)) {
                    std::cout << "Handler not found for packet: " << packetId << std::endl;
                    continue;
                }
                auto fhandler = m_handlers.find(packetId);
                if (fhandler != m_handlers.end()) {
                    fhandler->second(reader);
                }
            }
        }
    }

};