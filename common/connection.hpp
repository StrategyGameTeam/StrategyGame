#pragma once
// disable bunch of windows stuff that colides with raylib
#define NODRAWTEXT
#define NOGDI
#define NOCOLOR
#define NOUSER
#define NOMINMAX
#include "uvw.hpp"
#undef near
#undef far
#include <thread>
#include <queue>
#include "utils.hpp"

const std::string HOST;

struct PacketReader {
    std::vector<char> buf;
    unsigned long idx = 0;

    PacketReader(std::vector<char> &buf): buf(buf){
    }

    std::string readString(){
        auto str = std::string(buf.begin() + idx, std::find(buf.begin()+idx, buf.end(), 0));
        idx += str.length() + 1;
        return str;
    }
    char readChar(){
        return buf[idx++];
    }
    int readInt(){
        uint8_t c1 = readChar();
        uint8_t c2 = readChar();
        uint8_t c3 = readChar();
        uint8_t c4 = readChar();
        return (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
    }
    unsigned int readUInt(){
        uint8_t c1 = readChar();
        uint8_t c2 = readChar();
        uint8_t c3 = readChar();
        uint8_t c4 = readChar();
        return (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
    }

    bool readBool(){
        return readChar()>0;
    }
};

static constexpr int BUFFER_SIZE = 2*1024*1024;
struct PacketWriter {
    std::unique_ptr<std::array<char, BUFFER_SIZE>> buf = std::make_unique<std::array<char, BUFFER_SIZE>>();
    unsigned long len = 4;

    void writeString(const std::string &str){
        for (const auto c: str){
            writeChar(c);
        }
        writeChar(0);
    }
    void writeChar(char c){
        (*buf)[len++] = c;
        if(len > BUFFER_SIZE){
            throw std::overflow_error("packet writer buffer overflow");
        }
    }
    void writeInt(int n){
        writeChar((n >> 24) & 0xff);
        writeChar((n >> 16) & 0xff);
        writeChar((n >> 8) & 0xff);
        writeChar(n & 0xff);
    }
    void writeUInt(unsigned int n){
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
    { ext.packetId }; 
    { ext.serialize(wr) };
};

void writeLargeData(const std::shared_ptr<uvw::TCPHandle> handle, char* buf, unsigned long len);

template<Packet T>
void writePacket(const std::shared_ptr<uvw::TCPHandle> &handle, const std::string &destination, const T &packet) {
    PacketWriter wr;
    wr.writeString(packet.packetId);
    wr.writeString(destination);
    packet.serialize(wr);
    (*wr.buf)[0] = (char)((wr.len >> 24) & 0xff);
    (*wr.buf)[1] = (char)((wr.len >> 16) & 0xff);
    (*wr.buf)[2] = (char)((wr.len >> 8) & 0xff);
    (*wr.buf)[3] = (char)(wr.len & 0xff);
    writeLargeData(handle, wr.buf->data(), wr.len);

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
    std::vector<char> data;

    std::unordered_map<std::string, std::function<void(PacketReader &)>> m_handlers;

    Connection(const std::string &, unsigned int);
    ~Connection();

    void onError(const uvw::ErrorEvent &);

    void onClose(const uvw::CloseEvent &);
    void onConnected(const uvw::ConnectEvent &evt);

    template<Packet T>
    void writeToHost(const T &packet) {
        writePacket(this->m_tcp, packet);
    }
    template<Packet T>
    void writeToPlayer(std::string player, const T &packet) {
        writePacket(this->m_tcp, player, packet);
    }

    void onData(const uvw::DataEvent &);
    void registerPacketHandler(const std::string &name, const std::function<void(PacketReader &)>& handler);

    void clearHandlers(){
        m_handlers.clear();
    }

    void handleTasks(){
        if(data.size() < 4){
            return;
        }

        auto reader = PacketReader(data);
        auto size = reader.readUInt();
        if(size < 4){
            throw std::underflow_error("Illegal state, data size: " + std::to_string(data.size()));
        }
        if(data.size() < size){
            logging::debug("missing data size: ", size - data.size());
            return;
        }

        auto packetId = reader.readString();
        auto destination = reader.readString();
        logging::debug("Received packet with id: ", packetId, " and destination: ", destination);
        if (!m_handlers.contains(packetId)) {
            logging::error("Handler not found for packet: ", packetId);
            data.erase(data.begin(), data.begin() + size);
            return;
        }
        auto fhandler = m_handlers.find(packetId);
        if (fhandler != m_handlers.end()) {
            fhandler->second(reader);
        }
        data.erase(data.begin(), data.begin() + size);
    }

};