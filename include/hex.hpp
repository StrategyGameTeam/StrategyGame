#pragma once
#include <array>
#include <vector>
#include <concepts>
#include <utility>
#include <optional>
#include <raymath.h>

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
    static std::vector<HexCoords> make_line(const HexCoords from, const HexCoords to);
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

    std::vector<HexCoords> all_within_unscaled_quad 
    (Vector2 top_left, Vector2 top_right, Vector2 bottom_left, Vector2 bottom_right) 
    {
        // this bellow is some code that works, but is not used at the moment.
        // it is not because it is not needed, but because the actual code that does things bellow
        // does them wrong, and this is what is needed to fix it
        // but it does them wrong in such a way, that it works, as long as the camera cannot change angles or rotate
        // there are issues with floating point precision, so the solution in that lambda is problematic at least
        // still, proper that will sadly be needed
        [&]{
            const auto axial_delta = Vector2Subtract(bottom_right, top_left);
            const auto to_vec =  [](std::pair<float, float> p) {
                return Vector2{p.first, p.second};
            };

            // raylib gives us Mat4 maths, so let's just use it
            
            // the matrix that makes, so that
            // top left has to become (0, 0)
            // bottom right has to become (1, 1)
            Matrix imtx = MatrixMultiply(MatrixScale(axial_delta.x, axial_delta.y, 1), MatrixTranslate(top_left.x, top_left.y, 0));
            Matrix mtx = MatrixInvert(imtx);

            // for doing transformation on directions rather than points
            const auto vector2_transform_direction = [](Vector2 v, Matrix mat) -> Vector2 {
                return Vector2{
                    mat.m0*v.x + mat.m4*v.y,
                    mat.m1*v.x + mat.m5*v.y
                }; 
            };

            auto tr_l = Vector2Transform(top_right, mtx);
            auto tl_l = Vector2Transform(top_left, mtx);
            Matrix final_scaling = MatrixScale(1.0f/(tr_l.x - tl_l.x), 1.0f, 1.0f);
            mtx = MatrixMultiply(mtx, final_scaling);
            imtx = MatrixInvert(mtx);

            tr_l = Vector2Transform(top_right, mtx);
            tl_l = Vector2Transform(top_left, mtx);
            const auto bl_l = Vector2Transform(bottom_left, mtx);
            const auto br_l = Vector2Transform(bottom_right, mtx);

            const auto hex_size = vector2_transform_direction({1.0f, 1.0f}, mtx);

            const auto from_x = -hex_size.x;
            const auto to_x = 1+hex_size.x;
            const auto from_y = -hex_size.y;
            const auto to_y = 1+hex_size.y;

            const auto hex_base_r = Vector2{1.0f, 0.0f};
            const auto hex_base_rd = Vector2{0.5f, 0.75f};

            // these are directions, and should not be offset
            const auto hex_local_base_r = vector2_transform_direction(hex_base_r, mtx);
            const auto hex_local_base_rd = vector2_transform_direction(hex_base_rd, mtx);

            const auto within_square = [=](Vector2 p){
                return p.x > from_x && p.x < to_x && p.y > from_y && p.y < to_y;
            };

            constexpr int hard_limit = 1000; // how many interations to do each trace step before giving up
        };

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


template <typename T>
concept HexPathfindingGrid = requires(T grid, HexCoords hex, EdgeCoords edge) {
    { grid.canWalkOver(hex) } -> std::convertible_to<bool>;
    { grid.canSwimOver(hex) } -> std::convertible_to<bool>;
    { grid.canFlyOver(hex) } -> std::convertible_to<bool>;
    { grid.walkCost(edge) } -> std::integral;
    { grid.swimCost(edge) } -> std::integral;
    { grid.flyCost(edge) } -> std::integral;
};