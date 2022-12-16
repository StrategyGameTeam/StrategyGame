#include "uvw.hpp"
#include <memory>
#include <random>
#include "connection.hpp"
#include "packets.hpp"
#include "utils.hpp"

constexpr char *addr = "127.0.0.1";
constexpr int port = 4242;

void acceptClient(uvw::TCPHandle &srv);

template<Packet T>
void broadcast(uvw::Loop &loop, std::string &game_id, const T &packet);

void handleLogin(uvw::TCPHandle &handle, PacketReader &reader);

std::string genGameId() {
    int len = 4;
    const static auto &chrs = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    static std::mt19937 rg{std::random_device{}()};
    static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

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

int main() {
    auto loop = uvw::Loop::getDefault();
    std::shared_ptr<uvw::TCPHandle> tcp = loop->resource<uvw::TCPHandle>();

    tcp->on<uvw::ListenEvent>([](const uvw::ListenEvent &, uvw::TCPHandle &srv) {
        acceptClient(srv);
    });

    tcp->bind(addr, port);
    tcp->listen();
    logger::info("Server running at: ", addr, ":", port);
    loop->run();
}
std::vector<char> recv_buffer{};

void acceptClient(uvw::TCPHandle &srv) {
    //Allocate new TCPHandle for a client
    auto client = srv.loop().resource<uvw::TCPHandle>();

    //Listeners
    client->on<uvw::CloseEvent>([](const uvw::CloseEvent &, uvw::TCPHandle &client) {
        logger::info("[", client.peer().ip, "] disconnected ");
        client.close();
    });
    client->on<uvw::DataEvent>([&](const uvw::DataEvent &evt, uvw::TCPHandle &client) {
        logger::debug("[", client.peer().ip, "] Received bytes: ", evt.length);

        std::copy_n(evt.data.get(), evt.length, std::back_inserter(recv_buffer));
        logger::debug("buffer size: ", recv_buffer.size());
        if(recv_buffer.size() < 4){
            return;
        }

        auto reader = PacketReader(recv_buffer);
        auto size = reader.readUInt();
        if(recv_buffer.size() < size){
            logger::debug("missing data size: ", size - recv_buffer.size());
            return;
        }
        auto packetId = reader.readString();
        auto destination = reader.readString();

        auto client_data = client.data<HandleData>();

        if (packetId == LoginPacket::packetId) {
            handleLogin(client, reader);
        } else if (packetId == ChatPacket::packetId) {
            auto packet = ChatPacket::deserialize(reader);
            auto msg = "[" + client_data->nickname + "] " + packet.msg;
            broadcast(client.loop(), client_data->game_id, ChatPacket{msg});
        } else {

            logger::info("Forwarding packet for game: ", client_data->game_id, " and player: ", destination);
            client.loop().walk(uvw::Overloaded{
                    [&](uvw::TCPHandle &h) {
                        auto player_data = h.data<HandleData>();
                        if(player_data == nullptr){
                            return;
                        }
                        if (client_data->game_id == player_data->game_id &&
                            ((destination.empty() && player_data->is_host) || player_data->nickname == destination)) {
                            writeLargeData(h.shared_from_this(), recv_buffer.data(), recv_buffer.size());
                        }
                    },
                    [](auto &&) {}
            });

        }
        recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + size);
        logger::debug("cleaned buffer size: ", recv_buffer.size());
    });

    //Accept client
    srv.accept(*client);
    logger::info("Client connected: ", client->peer().ip);
    client->read();
}

void handleLogin(uvw::TCPHandle &handle, PacketReader &reader) {
    const auto packet = LoginPacket::deserialize(reader);
    logger::info("Logged in: ", packet.nickname);
    auto game_id = packet.game_id;
    bool is_host = false;
    std::shared_ptr<uvw::TCPHandle> host_handle;
    handle.loop().walk(uvw::Overloaded{
            [&](uvw::TCPHandle &h) {
                auto data = h.data<HandleData>();
                if(data == nullptr){
                    return;
                }
                if (data->game_id == game_id && data->is_host) {
                    host_handle = h.shared_from_this();
                }
            },
            [](auto &&) {}
    });
    if (!host_handle) {
        logger::info("Game not found, creating new!");
        game_id = genGameId();
        is_host = true;
        logger::info("Created game with ID: ", game_id);
    }
    logger::info("Client for game: ", game_id);
    handle.data(std::make_shared<HandleData>(HandleData{packet.nickname, game_id, is_host}));

    std::vector<std::shared_ptr<uvw::TCPHandle>> players;
    std::vector<std::string> playerNames;
    logger::info("Clients in game: ");
    handle.loop().walk(uvw::Overloaded{
            [&](uvw::TCPHandle &h) {
                auto data = h.data<HandleData>();
                if(data == nullptr){
                    return;
                }
                if (data->game_id == game_id) {
                    players.emplace_back(h.shared_from_this());
                    playerNames.push_back(data->nickname);
                    logger::info(" - ", data->nickname);
                }
            },
            [](auto &&) {}
    });

    for (const auto &item: players) {
        auto data = item->data<HandleData>();
        writePacket(item, ProxyDataPacket{data->is_host, playerNames, game_id});
        writePacket(item, ChatPacket{"Player " + packet.nickname + " joined!"});
        if (data->is_host && data->nickname != packet.nickname) {
            logger::info("Initializing player: ", packet.nickname, " host: ", data->nickname);
            //send update to host
            writePacket(item, InitializePlayerRequestPacket{packet.nickname});
        }
    }

}

template<Packet T>
void broadcast(uvw::Loop &loop, std::string &game_id, const T &packet) {
    loop.walk(uvw::Overloaded{
            [&](uvw::TCPHandle &h) {
                auto data = h.data<HandleData>();
                if (h.peer().port > 0 && data != nullptr && data->game_id == game_id) {
                    writePacket(h.shared_from_this(), packet);
                }
            },
            [](auto &&) {}
    });
}