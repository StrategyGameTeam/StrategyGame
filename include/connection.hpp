#pragma once
#include <uvw.hpp>
#include <thread>

struct Connection{
    std::string m_addr;
    unsigned int m_port;
    std::shared_ptr<uvw::Loop> m_loop;
    std::shared_ptr<uvw::TCPHandle> m_tcp;
    std::thread m_read_thread;
    std::function<void(char*, unsigned int)> packet_handler;

    Connection(const std::string &, unsigned int);
    ~Connection();

    void onError(const uvw::ErrorEvent &);
    void onData(const uvw::DataEvent &);
    void onClose(const uvw::CloseEvent &);
    void onConnected(const uvw::ConnectEvent &evt);
    void write(char*, unsigned int);

};