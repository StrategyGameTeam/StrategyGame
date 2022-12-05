#pragma once

#include <iostream>
#include "connection.hpp"
#include "hex.hpp"

//Host to client
struct WorldUpdatePacket {
    static const std::string packetId;
    CylinderHexWorld<HexData> world;

    void serialize(PacketWriter &wr) const {
        wr.writeInt(world.width);
        wr.writeInt(world.height);
        world.empty_hex.serialize(wr);
        for(int i = 0; i<world.width * world.height; i++){
            world.data[i].serialize(wr);
        }
    }

    static WorldUpdatePacket deserialize(PacketReader &reader){
        auto width = reader.readInt();
        auto height = reader.readInt();
        auto empty_hex = HexData::deserialize(reader);
        CylinderHexWorld<HexData> world(width, height, empty_hex, empty_hex);
        for(int i = 0; i<width*height; i++){
            world.data[i] = HexData::deserialize(reader);
        }
        return WorldUpdatePacket{world};
    }
};
