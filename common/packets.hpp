#pragma once

#include <iostream>
#include "connection.hpp"

//Client to proxy
struct LoginPacket {
    static const std::string packetId;
    std::string game_id;
    std::string nickname;

    void serialize(PacketWriter &wr) const {
        wr.writeString(game_id);
        wr.writeString(nickname);
    }

    static LoginPacket deserialize(PacketReader &reader){
        auto game_id = reader.readString();
        auto nickname = reader.readString();
        return LoginPacket{game_id, nickname};
    }
};

//Proxy to client
struct ProxyDataPacket {
    static const std::string packetId;
    bool is_host;
    std::vector<std::string> players;
    std::string game_id;


    void serialize(PacketWriter &wr) const {
        wr.writeBool(is_host);
        wr.writeInt(players.size());
        for (const auto &item: players){
            wr.writeString(item);
        }
        wr.writeString(game_id);
    }

    static ProxyDataPacket deserialize(PacketReader &reader){
        bool is_host = reader.readBool();
        int player_count = reader.readInt();
        std::vector<std::string> players;
        for(int i = 0; i<player_count; i++){
            players.push_back(reader.readString());
        }
        std::string game_id = reader.readString();
        return ProxyDataPacket{is_host, players, game_id};
    }
};

//Proxy to host client
struct InitializePlayerRequestPacket {
    static const std::string packetId;
    std::string player;


    void serialize(PacketWriter &wr) const {
        wr.writeString(player);
    }

    static InitializePlayerRequestPacket deserialize(PacketReader &reader){
        auto player = reader.readString();
        return InitializePlayerRequestPacket{player};
    }
};


struct ChatPacket {
    static const std::string packetId;
    std::string msg;

    void serialize(PacketWriter &wr) const {
        wr.writeString(msg);
    }

    static ChatPacket deserialize(PacketReader &reader){
        auto data = reader.readString();
        return ChatPacket{data};
    }
};