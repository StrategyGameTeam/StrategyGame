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
};

struct EdgeCoords {
    HexCoords hex;
    Edge edge;
};

template <typename HexT>
struct CylinderHexWorld {
    int width;
    int height;
    std::vector<HexT> data;

    CylinderHexWorld (int width, int height) 
    requires std::default_initializable<HexT> 
        : CylinderHexWorld(width, height, HexT()) {};

    CylinderHexWorld (int width, int height, HexT default_hex)
        : width(width), height(height)
    {
        data.resize((width+1)*(height+1), default_hex);
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
        int convertion_offset = direct_row / 2;
        int direct_column = hc.q - convertion_offset;
        return direct_row*width + direct_column;
    }

    std::optional<std::reference_wrapper<HexT>> at_abnormal(const HexCoords hc) {
        if (hc.r < 0 || hc.r >= height) {
            return {};
        }
        auto wrapped = hc;
        wrapped.q = positive_modulo(wrapped.q, width);
        return data.at(compute_index(wrapped));
    }

    HexT& at_normalized(const HexCoords hc) {
        return data.at(compute_index(normalized_coords(hc)));
    }

    // This ignores perspective, and as such is kinda broken. Have to use it carefully
    std::vector<HexCoords> all_within_unscaled_quad (float x, float y, float width, float height) 
    {
        auto top_left_hx = HexCoords::from_world_unscaled(x-2, y-2);
        auto top_right_hx = HexCoords::from_world_unscaled(x+height+2, y-2);
        auto bottom_left_hx = HexCoords::from_world_unscaled(x-2, y+width+2);
        auto bottom_right_hx = HexCoords::from_world_unscaled(x+height+2, y+width+2);

        // limit the lines only to hexes that exist vertically
        normalize_height(top_left_hx);
        normalize_height(bottom_left_hx);
        normalize_height(top_right_hx);
        normalize_height(bottom_right_hx);
        
        std::vector<HexCoords> scan_line;
        // make a line from top_left_hx to bottom_left_hx
        const auto samples = 1 + top_left_hx.distance(bottom_left_hx);
        scan_line.reserve(samples);
        const auto inv = 1.0f / samples;
        const auto [tlx, tly] = top_left_hx.to_world_unscaled();
        const auto [blx, bly] = bottom_left_hx.to_world_unscaled();
        const auto dx = blx - tlx;
        const auto dy = bly - tly;
        for(int i = 0; i < samples; i++) {
            const auto x = tlx + dx*inv*i;
            const auto y = tly + dy*inv*i;
            scan_line.push_back(HexCoords::from_world_unscaled(x, y));
        }

        // sweep that line from left to right
        std::vector<HexCoords> result;
        const auto top_distance = top_left_hx.distance(top_right_hx);
        const auto bottom_distance = bottom_left_hx.distance(bottom_right_hx);
        const auto max_distance = std::max(top_distance, bottom_distance);
        result.reserve(scan_line.size() * max_distance);
        for(int i = 0; i < max_distance; i++) {
            for(const auto hex : scan_line) {
                result.push_back(hex + hexR(i));
            }
        }

        // if we have very fucky angles, there can be overlaps. As some say - too bad
        return result;
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