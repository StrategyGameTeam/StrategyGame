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
};

struct EdgeCoords {
    HexCoords hex;
    Edge edge;
};

template <typename HexT>
struct CylinderHexWorld {
    int width;
    int height;
    HexT default_hex;
    std::vector<HexT> data;
    std::array<Vector2, 4> looked_at;


    CylinderHexWorld (int width, int height) 
    requires std::default_initializable<HexT> 
        : CylinderHexWorld(width, height, HexT()) {};

    CylinderHexWorld (int width, int height, HexT default_hex)
        : width(width), height(height), default_hex(default_hex)
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

    HexT at(const HexCoords hc) {
        return at_ref_abnormal(hc).value_or(default_hex);
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
        // some notes on what is accomplished here
        // because we need to run this every frame, this has to be fast
        // since the shape of the visibility quad can be complex (but should not be concave)
        // we transform the space it's laying on, so that is becomes almost a square
        // then, we use that same transformation to morph a grid of triangles,
        // which contains all the points of a hexagon (6 outer and 1 center)
        // then we do a simple scan through that grid, that is just limited by a square,
        // which is simple to test if we're within

        const auto axial_delta = Vector2Subtract(bottom_right, top_left);
        const auto to_vec =  [](std::pair<float, float> p) {
            return Vector2{p.first, p.second};
        };

        // raylib gives us Mat4 maths, so let's just use it
        // note - these matricies are ordered column major, so it's written in code as the transposition
        // of what we are used from maths
        
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

        const auto tl_l = Vector2Transform(top_left, mtx);
        const auto tr_l = Vector2Transform(top_right, mtx);
        const auto bl_l = Vector2Transform(bottom_left, mtx);
        const auto br_l = Vector2Transform(bottom_right, mtx);
        // todo: scale this shit so it fit's into the 1x1@0,0 box
        const auto hex_size = vector2_transform_direction({1.0f, 1.0f}, mtx);

        const auto from_x = -hex_size.x;
        const auto to_x = 1+hex_size.x;
        const auto from_y = -hex_size.y;
        const auto to_y = 1+hex_size.y;

        const auto starting_point = ([=]{
            const auto middleish = Vector2{0.5f, 0.5f};
            const auto world_mid = Vector2Transform(middleish, imtx);
            const auto snapped = HexCoords::from_world_unscaled(world_mid.x, world_mid.y);
            const auto snapped_world = to_vec(snapped.to_world_unscaled());
            return Vector2Transform(snapped_world, mtx);
        })();

        const auto hex_base_r = Vector2{1.0f, 0.0f};
        const auto hex_base_rd = Vector2{0.5f, 0.75f};

        // these are directions, and should not be offset
        const auto hex_local_base_r = vector2_transform_direction(hex_base_r, mtx);
        const auto hex_local_base_rd = vector2_transform_direction(hex_base_rd, mtx);

        printf("R %f %f \t\tRD %f %f\n", hex_local_base_r.x, hex_local_base_r.y, hex_local_base_rd.x, hex_local_base_rd.y);

        const auto within_square = [=](Vector2 p){
            return p.x > from_x && p.x < to_x && p.y > from_y && p.y < to_y;
        };

        constexpr int hard_limit = 1000; // how many interations to do each trace step before giving up
        
        std::vector<Vector2> axis_line;
        axis_line.push_back(starting_point);

        // line towards right
        Vector2 march = starting_point;
        for(int i = 0; i < hard_limit; i++) {
            march = Vector2Add(march, hex_local_base_r);
            if (within_square(march)) {
                axis_line.push_back(march);
            } else {
                break;
            }
        }

        // line towards left
        march = starting_point;
        for(int i = 0; i < hard_limit; i++) {
            march = Vector2Subtract(march, hex_local_base_r);
            if (within_square(march)) {
                axis_line.push_back(march);
            } else {
                break;
            }
        }

        std::vector<HexCoords> results;
        for(const auto column : axis_line) {
            // line to down-right
            Vector2 march = column;
            for(int i = 0; i < hard_limit; i++) {
                march = Vector2Add(march, hex_local_base_rd);
                if (within_square(march)) {
                    Vector2 orig_space = Vector2Transform(march, imtx);
                    results.push_back(HexCoords::from_world_unscaled(orig_space.x, orig_space.y));
                } else {
                    break;
                }
            } 
            // line to top-left
            march = column;
            for(int i = 0; i < hard_limit; i++) {
                march = Vector2Subtract(march, hex_local_base_rd);
                if (within_square(march)) {
                    Vector2 orig_space = Vector2Transform(march, imtx);
                    results.push_back(HexCoords::from_world_unscaled(orig_space.x, orig_space.y));
                } else {
                    break;
                }
            }
        }

        // for debugging, but might be usefull somewhere
        looked_at[0] = tl_l;
        looked_at[1] = tr_l;
        looked_at[2] = br_l;
        looked_at[3] = bl_l;

        // printf("TL %f %f\t\tTR %f %f\t\tBL %f %f\t\tBR %f %f\n", tl_l.x, tl_l.y, tr_l.x, tr_l.y, bl_l.x, bl_l.y, br_l.x, br_l.y);
        return results;
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