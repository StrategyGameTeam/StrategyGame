#pragma once
#include <array>
#include <vector>
#include <concepts>
#include <utility>

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
// struct CoordsAxial;

// Operators to get offsets from a cell. They are Left and Right combined with Up, Down, or just
HexCoords operator "" _LU (size_t x) { int v = x; return HexCoords{0, -v, +v}; };
HexCoords operator "" _RU (size_t x) { int v = x; return HexCoords{+v, -v, 0}; };
HexCoords operator "" _R  (size_t x) { int v = x; return HexCoords{+v, 0, -v}; };
HexCoords operator "" _RD (size_t x) { int v = x; return HexCoords{0, +v, -v}; };
HexCoords operator "" _LD (size_t x) { int v = x; return HexCoords{-v, +v, 0}; };
HexCoords operator "" _L  (size_t x) { int v = x; return HexCoords{-v, 0, +v}; };

// Edges of a hex
enum class Edge {
    RightUp = 0, RU = 0,
    Right = 1, R = 1,
    RightDown = 2, RD = 2,
    LeftDown = 3, LD = 3,
    Left = 4, L = 4,
    LeftUp = 5, LU = 5
};

struct HexCoords {
    int q;
    int r;
    int s;

    HexCoords operator+ (const HexCoords& other) const {
        return HexCoords{q+other.q, r+other.r, s+other.s};
    }
    
    HexCoords operator- (const HexCoords& other) const {
        return HexCoords{q-other.q, r-other.r, s-other.s};
    }

    EdgeCoords operator+ (const Edge& edge) const {
        return EdgeCoords {
            .hex = *this,
            .edge = edge
        };
    }

    std::array<HexCoords, 6> neighbours() const {
        return {
            *this + 1_RU,
            *this + 1_R,
            *this + 1_RD,
            *this + 1_LD,
            *this + 1_L,
            *this + 1_LU
        };
    }

    int distance (const HexCoords& to) const {
        const auto diff = *this - to;
        return std::max(std::abs(diff.q), std::max(std::abs(diff.r), std::abs(diff.s)));
    }
};

struct EdgeCoords {
    HexCoords hex;
    Edge edge;
};

