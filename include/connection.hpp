#pragma once
#include <uvw.hpp>
#include <iostream>
#include <thread>

struct PacketReader {
    std::unique_ptr<char*> buf;
    unsigned long idx = 0;
    unsigned long size;

    PacketReader(char* buf, std::size_t size): buf(std::make_unique<char*>(buf)), size(size){
    }

    std::string readString(){
        auto str = std::string(*buf + idx);
        idx += str.length() + 1;
        return str;
    }
    char readChar(){
        return (*buf)[idx++];
    }
    int readInt(){
        auto c1 = readChar();
        auto c2 = readChar();
        auto c3 = readChar();
        auto c4 = readChar();
        return (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
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
};

template <typename T>
concept Packet = requires(T ext, PacketWriter &wr) {
    { ext.packetId, ext.serialize(wr) };
};

struct Connection{
    std::string m_addr;
    unsigned int m_port;
    std::shared_ptr<uvw::Loop> m_loop;
    std::shared_ptr<uvw::TCPHandle> m_tcp;
    std::thread m_read_thread;

    std::unordered_multimap<std::string, std::function<void(PacketReader &)>> m_handlers;

    Connection(const std::string &, unsigned int);
    ~Connection();

    void onError(const uvw::ErrorEvent &);

    void onClose(const uvw::CloseEvent &);
    void onConnected(const uvw::ConnectEvent &evt);

    template<Packet T>
    void write(const T &packet) {
        PacketWriter wr;
        wr.writeString(packet.packetId);
        packet.serialize(wr);
        this->m_tcp->write(*wr.buf, wr.len);
    }

    void onData(const uvw::DataEvent &);
    void registerPacketHandler(const std::string &name, const std::function<void(PacketReader &)>& handler);

};