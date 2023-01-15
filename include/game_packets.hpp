#pragma once

#include <iostream>
#include "connection.hpp"
#include "hex.hpp"
#include "units.hpp"

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

struct CreateMilitaryUnitPacket {
    static const std::string packetId;
    HexCoords coords;
    MilitaryUnit unit;

    void serialize(PacketWriter &wr) const {
        wr.writeInt(coords.q);
        wr.writeInt(coords.r);
        wr.writeInt(coords.s);

        wr.writeInt(unit.id);
        wr.writeInt(unit.fraction);
        wr.writeInt(unit.health);
        wr.writeInt(unit.vission_range);
        wr.writeInt(unit.stamina);
        wr.writeString(unit.owner);
    }

    static CreateMilitaryUnitPacket deserialize(PacketReader &reader){
        int q = reader.readInt();
        int r = reader.readInt();
        int s = reader.readInt();

        HexCoords coords{q,r,s};

        int id = reader.readInt();
        int fraction = reader.readInt();
        int health = reader.readInt();
        int vission_range = reader.readInt();
        int stamina = reader.readInt();
        std::string owner = reader.readString();

        MilitaryUnit unit;
        unit.id = id;
        unit.fraction = fraction;
        unit.health = health;
        unit.vission_range = vission_range;
        unit.stamina = stamina;
        unit.owner = owner;
        return CreateMilitaryUnitPacket{.coords = coords, .unit = unit};
    }

};

struct MoveUnitPacket {
    static const std::string packetId;
    HexCoords from;
    HexCoords to;

    void serialize(PacketWriter &wr) const {
        wr.writeInt(from.q);
        wr.writeInt(from.r);
        wr.writeInt(from.s);
        wr.writeInt(to.q);
        wr.writeInt(to.r);
        wr.writeInt(to.s);
    }

    static MoveUnitPacket deserialize(PacketReader &reader){
        int q = reader.readInt();
        int r = reader.readInt();
        int s = reader.readInt();
        HexCoords from{q,r,s};
        q = reader.readInt();
        r = reader.readInt();
        s = reader.readInt();
        HexCoords to{q,r,s};

        return MoveUnitPacket{.from = from, .to = to};
    }

};