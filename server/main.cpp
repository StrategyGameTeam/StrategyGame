#include "uvw.hpp"
#include <memory>
#include <iostream>
#include <random>
#include "connection.hpp"
#include "packets.hpp"

constexpr const char *addr = "127.0.0.1";
constexpr const int port = 4242;

void acceptClient(uvw::TCPHandle &srv);

template<Packet T>
void broadcast(uvw::Loop &loop, const T &packet);

void handleLogin(uvw::TCPHandle &handle, PacketReader &reader);

std::string genGameId() {
    int len = 4;
    static auto &chrs = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string s;

    s.reserve(len);

    while (len--)
        s += chrs[pick(rg)];

    return s;
}

struct HandleData {
    std::string nickname;
    std::string game_id;
    bool is_host;
};

std::vector<std::string> games;

int main() {
    auto loop = uvw::Loop::getDefault();
    std::shared_ptr<uvw::TCPHandle> tcp = loop->resource<uvw::TCPHandle>();

    tcp->on<uvw::ListenEvent>([](const uvw::ListenEvent &, uvw::TCPHandle &srv) {
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
    client->on<uvw::CloseEvent>([](const uvw::CloseEvent &, uvw::TCPHandle &client) {
        std::cout << "[" << client.peer().ip << "]" << " disconnected " << std::endl;
        client.close();
    });
    client->on<uvw::DataEvent>([](const uvw::DataEvent &evt, uvw::TCPHandle &client) {
        std::cout << "[" << client.peer().ip << "]" << " Data reveived: ";
        std::cout.write(evt.data.get(), evt.length);
        std::cout << std::endl;

        std::vector<char> data;
        std::copy_n(evt.data.get(), evt.length, std::back_inserter(data));

        auto reader = PacketReader(data);
        auto packetId = reader.readString();
        auto destination = reader.readString();

        auto client_data = client.data<HandleData>();

        if (packetId == LoginPacket::packetId) {
            handleLogin(client, reader);
        } else if (packetId == ChatPacket::packetId) {
            auto packet = ChatPacket::deserialize(reader);
            auto msg = "[" + client_data->nickname + "] " + packet.msg;
            broadcast(client.loop(), ChatPacket{msg});
        } else {

            std::cout << "Forwarding packet for game: " << client_data->game_id << " and player: " << destination << std::endl;
            client.loop().walk(uvw::Overloaded{
                    [&](uvw::TCPHandle &h) {
                        auto player_data = h.data<HandleData>();
                        if(player_data == nullptr){
                            return;
                        }
                        if (client_data->game_id == player_data->game_id &&
                            ((destination.empty() && player_data->is_host) || player_data->nickname == destination)) {
                            h.write(evt.data.get(), evt.length);
                        }
                    },
                    [](auto &&) {}
            });

        }
    });

    //Accept client
    srv.accept(*client);
    std::cout << "Client conntected: " << client->peer().ip << std::endl;
    client->read();
}

void handleLogin(uvw::TCPHandle &handle, PacketReader &reader) {
    const auto packet = LoginPacket::deserialize(reader);
    std::cout << "Logged in: " << packet.nickname << std::endl;
    auto game_id = packet.game_id;
    bool is_host = false;
    if (std::find(games.begin(), games.end(), game_id) == games.end()) {
        std::cout << "Game not found, creating new!" << std::endl;
        game_id = genGameId();
        games.push_back(game_id);
        is_host = true;
        std::cout << "Created game with ID: " << game_id << std::endl;
    }
    std::cout << "Client for game: " << game_id << std::endl;
    handle.data(std::make_shared<HandleData>(HandleData{packet.nickname, game_id, is_host}));

    std::vector<std::shared_ptr<uvw::TCPHandle>> players;
    std::vector<std::string> playerNames;
    std::cout << "Clients in game: " << std::endl;
    handle.loop().walk(uvw::Overloaded{
            [&](uvw::TCPHandle &h) {
                auto data = h.data<HandleData>();
                if(data.get() == nullptr){
                    return;
                }
                if (data->game_id == game_id) {
                    players.emplace_back(h.shared_from_this());
                    playerNames.push_back(data->nickname);
                    std::cout << " - " << data->nickname << std::endl;
                }
            },
            [](auto &&) {}
    });

    for (const auto &item: players) {
        auto data = item->data<HandleData>();
        writePacket(item, ProxyDataPacket{data->is_host, playerNames, game_id});
        if (data->is_host && data->nickname != packet.nickname) {
            std::cout << "Initializing player: " << packet.nickname << " host: " << data->nickname << std::endl;
            //send update to host
            writePacket(item, InitializePlayerRequestPacket{packet.nickname});
        }
    }

}

template<Packet T>
void broadcast(uvw::Loop &loop, const T &packet) {
    loop.walk(uvw::Overloaded{
            [&](uvw::TCPHandle &h) {
                if (h.peer().port > 0) writePacket(h.shared_from_this(), packet);
            },
            [](auto &&) {}
    });
}