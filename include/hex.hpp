#pragma once
#include <array>
#include <vector>
#include <concepts>
#include <utility>
#include <optional>

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

// Operators to get offsets from a cell. They are Left and Right combined with Up, Down, or just
HexCoords operator "" _LU (size_t x);
HexCoords operator "" _RU (size_t x);
HexCoords operator "" _R  (size_t x);
HexCoords operator "" _RD (size_t x);
HexCoords operator "" _LD (size_t x);
HexCoords operator "" _L  (size_t x);

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
};

struct EdgeCoords {
    HexCoords hex;
    Edge edge;
};

template <typename HexT>
struct CylinderHexWorld {
    size_t width;
    size_t height;
    bool wrap_vertical;
    bool wrap_horizontal;
    std::vector<HexT> data;

    CylinderHexWorld (size_t width, size_t height, bool wrap_vertical, bool wrap_horizontal) 
    requires std::default_initializable<HexT> 
        : CylinderHexWorld(width, height, wrap_vertical, wrap_horizontal, HexT()) {};

    CylinderHexWorld (size_t width, size_t height, bool wrap_vertical, bool wrap_horizontal, HexT default_hex)
        : width(width), height(height), wrap_vertical(wrap_vertical), wrap_horizontal(wrap_horizontal)
    {
        data.resize(width*height, default_hex);
    }

    std::optional<std::reference_wrapper<HexT>> at(HexCoords hc) {
        int direct_row = hc.r;
        int convertion_offset = (height-direct_row) / 2;
        int direct_column = hc.q - convertion_offset;
        if (!wrap_vertical && (direct_row < 0 || direct_row >= height)) {
            return {};
        }
        if (!wrap_horizontal && (direct_column < 0 || direct_column >= width)) {
            return {};
        }
        direct_column = direct_column % width;
        direct_row = direct_row % height;

        return data.at(direct_row*width + direct_column);
    }
};


template <typename T>
concept HexPathfindingGrid = requires(T grid, HexCoords hex, EdgeCoords edge) {
    { grid.canWalkOver(hex) } -> std::convertible_to<bool>;
    { grid.canSwimOver(hex) } -> std::convertible_to<bool>;
    { grid.canFlyOver(hex) } -> std::convertible_to<bool>;
    { grid.walkCost(edge) } -> std::integral;
    { grid.swimCost(edge) } -> std::integral;
    { grid.flyCost(edge) } -> std::integral;
};