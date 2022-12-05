#pragma once

#include <iostream>
#include "connection.hpp"
#include "hex.hpp"

//Host to client
struct WorldUpdatePacket {
    static const std::string packetId;

    void serialize(PacketWriter &wr) const {
    }

    static WorldUpdatePacket deserialize(PacketReader &reader){
        return WorldUpdatePacket{};
    }
};
