#pragma once
#include <array>
#include <vector>
#include <concepts>
#include <utility>
#include <optional>
#include <raymath.h>
#include <random>
#include "connection.hpp"

/*
Some notes about what the code means.
Most taken from https://www.redblobgames.com/grids/hexagons/#coordinates
Please, read that, especially the section on Cube and Axial coordinates
This header defines some user defined literals operators
They are: _LU (Left Up), _L (Left), _LD (Left Down), _RU (Right Up), _R (Right), _RD (Right Down)
To get a cell, you can take an existing one, and add numbers suffixed by those UDLs
So, for example:
    HexCoords two_to_the_right = current + 2_R;
*/

struct HexCoords;
struct EdgeCoords;
struct HexData;

// Operators to get offsets from a cell. They are Left and Right combined with Up, Down, or just
HexCoords operator "" _LU (unsigned long long x);
HexCoords operator "" _RU (unsigned long long x);
HexCoords operator "" _R  (unsigned long long x);
HexCoords operator "" _RD (unsigned long long x);
HexCoords operator "" _LD (unsigned long long x);
HexCoords operator "" _L  (unsigned long long x);

// Function to get the same offsets as in the operators
HexCoords hexLU (int x);
HexCoords hexRU (int x);
HexCoords hexR  (int x);
HexCoords hexRD (int x);
HexCoords hexLD (int x);
HexCoords hexL  (int x);


// Edges of a hex 
enum class Edge {
    RightUp = 0, RU = 0,
    Right = 1, R = 1,
    RightDown = 2, RD = 2,
    LeftDown = 3, LD = 3,
    Left = 4, L = 4,
    LeftUp = 5, LU = 5
};

constexpr static float sqrt3 = 1.73205080757; // comes up in hex maths
// because fucking modulo operator
inline int positive_modulo(int i, int n) {
    return (i % n + n) % n;
}

struct HexCoords {
    int q;
    int r;
    int s;

    bool operator== (const HexCoords& other) const;
    static HexCoords from_axial(int q, int r);
    HexCoords operator+ (const HexCoords& other) const;
    HexCoords operator- (const HexCoords& other) const;
    EdgeCoords operator+ (const Edge& edge) const;
    std::array<HexCoords, 6> neighbours() const;
    int distance (const HexCoords& to) const;
    std::pair<float, float> to_world_unscaled () const;
    static HexCoords rounded_to_hex(float q, float r, float s);
    static HexCoords from_world_unscaled(float x, float y);
    static std::vector<HexCoords> make_line(const HexCoords from, const HexCoords to);
    static HexCoords from_offset(int col, int row);
};

struct HexData {
    int tileid = -1;
    uint_least32_t visibility_flags = 0; // this implies max factions to be 16, as this is the max number of fractions this flag can fit
    int owner_faction = -1;
    int structure_atop = -1;
    std::array<int, 6> structure_edges = {-1};
    int upgrade_atop = -1;
    std::array<int, 6> upgrade_edges = {-1};

    enum class Visibility {
        NONE = 0,
        FOG = 1,
        NORMAL = 2,
        SUPERIOR = 3
    };

    Visibility getFractionVisibility (int fraction) {
        return (Visibility)((visibility_flags & (0b11 << (fraction * 2))) >> (fraction * 2));
    }

    void setFractionVisibility (int fraction, Visibility vis) {
        visibility_flags |= (int)vis << (fraction * 2);
    }

    void overrideVisibility (decltype(visibility_flags) val) {
        visibility_flags = val;
    }

    void resetVisibility () {
        visibility_flags = 0;
    }

    void serialize(PacketWriter &wr) const {
        wr.writeInt(tileid);
        wr.writeUInt(visibility_flags);
        wr.writeInt(owner_faction);
        wr.writeInt(structure_atop);
        for (const auto &item: structure_edges)
            wr.writeInt(item);
        wr.writeInt(upgrade_atop);
        for (const auto &item: upgrade_edges)
            wr.writeInt(item);

    }

    static HexData deserialize(PacketReader &reader){
        int tileid = reader.readInt();
        uint_least32_t visibility_flags = reader.readUInt();
        int owner_faction = reader.readInt();
        int structure_atop = reader.readInt();
        std::array<int, 6> structure_edges = {-1};
        for(int & structure_edge : structure_edges){
            structure_edge = reader.readInt();
        }
        int upgrade_atop = reader.readInt();
         std::array<int, 6> upgrade_edges = {-1};
        for(int & upgrade_edge : upgrade_edges){
            upgrade_edge = reader.readInt();
        }
        return HexData{tileid, visibility_flags, owner_faction, structure_atop, structure_edges, upgrade_atop, upgrade_edges};
    }
};

struct EdgeCoords {
    HexCoords hex;
    Edge edge;
};

template <typename HexT>
struct CylinderHexWorld {
    int width;
    int height;
    HexT empty_hex;
    std::vector<HexT> data;

    CylinderHexWorld (int width, int height, HexT default_hex, HexT empty_hex)
        : width(width), height(height), empty_hex(empty_hex)
    {
        data.resize((width)*(height), default_hex);
    }

    HexCoords normalized_coords (const HexCoords abnormal) const {
        const auto q = positive_modulo(abnormal.q, width);
        const auto r = std::max(std::min<int>(abnormal.r, height), 0);
        const auto s = -q-r;
        return HexCoords{q, r, s};
    }

    void normalize_height(HexCoords& hc) const {
        hc.r = std::max(std::min<int>(hc.r, height), 0);
    }

    int compute_index (const HexCoords hc) const {
        int direct_row = hc.r;
        int direct_column = hc.q;
        return direct_row*width + direct_column;
    }

    int compute_normalized_index (const HexCoords ahc) const {
        const auto hc = normalized_coords(ahc);
        return compute_index(hc);
    }

    HexT at(const HexCoords hc) {
        return at_ref_abnormal(hc).value_or(empty_hex);
    }

    std::optional<std::reference_wrapper<HexT>> at_ref_abnormal(const HexCoords hc) {
        if (hc.r < 0 || hc.r >= height) {
            return {};
        }
        auto wrapped = hc;
        wrapped.q = positive_modulo(wrapped.q, width);
        return data.at(compute_index(wrapped));
    }

    HexT& at_ref_normalized(const HexCoords hc) {
        return data.at(compute_index(normalized_coords(hc)));
    }

    std::vector<HexCoords> all_within_unscaled_quad (
        Vector2 top_left, Vector2 top_right, Vector2 bottom_left, Vector2 bottom_right
    ) {
        std::vector<HexCoords> line = ([&]{
            std::vector<HexCoords> result;
            // start from the top right
            const auto bl = HexCoords::from_world_unscaled(bottom_left.x, bottom_left.y) + 1_LD; 
            const auto br = HexCoords::from_world_unscaled(bottom_right.x, bottom_right.y) + 1_RD;
            const auto steps = bl.distance(br);
            result.reserve(steps+1);
            result.push_back(bl);
            for(int i = 0; i < steps; i++) {
                result.push_back(result.back() + 1_R);
            }
            return result;
        })();
        
        std::vector<HexCoords> result;
        result.insert(result.end(), line.begin(), line.end());

        const auto limit_r = HexCoords::from_world_unscaled(top_left.x, top_left.y).r - 1;

        while(line.front().r >= limit_r) {
            std::vector<HexCoords> local_line;
            local_line.reserve(line.size() + 2);
            for(size_t i = 0; i < line.size(); i += 2) {
                local_line.push_back(line.at(i) + 1_LU);
                local_line.push_back(line.at(i) + 1_RU);
            }
            if (line.size() % 2 == 0) {
                local_line.push_back(local_line.back() + 1_R);
            }
            result.insert(result.end(), local_line.begin(), local_line.end());
            line = local_line;
        }

        return result;
    }
};
