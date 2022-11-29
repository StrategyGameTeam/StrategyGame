#pragma once
#include <iostream>
#include "connection.hpp"
#include "game.hpp"


struct ChatMessage {
    std::string msg;
    std::chrono::system_clock::time_point tp;
};

struct ChatPacket {
    static const std::string packetId;
    std::string msg;

    void serialize(PacketWriter &wr) const {
        wr.writeString(msg);
        wr.writeInt(2050);
        wr.writeInt(-4567);
    }

    static ChatPacket deserialize(PacketReader &reader){
        auto data = reader.readString();
        auto i1 = reader.readInt();
        auto i2 = reader.readInt();
        std::cout << "Int 1: " << i1 << std::endl;
        std::cout << "Int 2: " << i2 << std::endl;
        return ChatPacket{data};
    }
};
const std::string ChatPacket::packetId = "chat";

struct Chat {
    std::vector<ChatMessage> chat_messages;

    explicit Chat(GameState &gs){
        gs.connection.registerPacketHandler(
                ChatPacket::packetId,
                [this](PacketReader &r) { handlePacket(r); }
        );
    }

    void render(){
        int posY = 420;
        for (int i = chat_messages.size() - 1; i >= 0; i--) {
            auto now = std::chrono::system_clock::now();
            auto dur = now - chat_messages[i].tp;
            if (std::chrono::duration_cast<std::chrono::seconds>(dur).count() > 5) {
                break;
            }
            DrawText(chat_messages[i].msg.data(), 5, posY, 18, {0, 0, 0, 255});
            posY -= 20;
        }
    }

    void handlePacket(PacketReader &reader){
        auto p = ChatPacket::deserialize(reader);
        chat_messages.push_back({p.msg, std::chrono::system_clock::now()});
    }
};