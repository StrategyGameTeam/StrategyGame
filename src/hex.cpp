#include "hex.hpp"

HexCoords operator "" _LU (size_t x) { int v = x; return HexCoords{0, -v, +v}; };
HexCoords operator "" _RU (size_t x) { int v = x; return HexCoords{+v, -v, 0}; };
HexCoords operator "" _R  (size_t x) { int v = x; return HexCoords{+v, 0, -v}; };
HexCoords operator "" _RD (size_t x) { int v = x; return HexCoords{0, +v, -v}; };
HexCoords operator "" _LD (size_t x) { int v = x; return HexCoords{-v, +v, 0}; };
HexCoords operator "" _L  (size_t x) { int v = x; return HexCoords{-v, 0, +v}; };

HexCoords HexCoords::from_axial(int q, int r) {
    return HexCoords{q, r, -q-r};
}

bool HexCoords::operator== (const HexCoords& other) const {
    return q == other.q && r == other.r && s == other.s;
};

HexCoords HexCoords::operator+ (const HexCoords& other) const {
    return HexCoords{q+other.q, r+other.r, s+other.s};
}

HexCoords HexCoords::operator- (const HexCoords& other) const {
    return HexCoords{q-other.q, r-other.r, s-other.s};
}

EdgeCoords HexCoords::operator+ (const Edge& edge) const {
    return EdgeCoords {
        .hex = *this,
        .edge = edge
    };
}

std::array<HexCoords, 6> HexCoords::neighbours() const {
    return {
        *this + 1_RU,
        *this + 1_R,
        *this + 1_RD,
        *this + 1_LD,
        *this + 1_L,
        *this + 1_LU
    };
}

int HexCoords::distance (const HexCoords& to) const {
    const auto diff = *this - to;
    return std::max(std::abs(diff.q), std::max(std::abs(diff.r), std::abs(diff.s)));
}

std::pair<float, float> HexCoords::to_world_unscaled () const {
    return {
        sqrt3*q + sqrt3/2.0f*r,
        1.5*r
    };
}

HexCoords HexCoords::rounded_to_hex(float q, float r, float s) {
    auto rq = round(q);
    auto rr = round(r);
    auto rs = round(s);
    const auto dq = abs(q - rq);
    const auto dr = abs(r - rr);
    const auto ds = abs(s - rs);
    if (dq > dr && dq > ds) {
        rq = -rr-rs;
    } else if (dr > rs) {
        rr = -rq-rs;
    } else {
        rs = -rq-rr;
    }
    return HexCoords{static_cast<int>(rq), static_cast<int>(rr), static_cast<int>(rs)};
}

HexCoords HexCoords::from_world_unscaled(float x, float y) {
    const auto comp_q = sqrt3/3.0f*x - y/3.0f;
    const auto comp_r = 2.0f/3.0f*y;
    const auto comp_s = -comp_q-comp_r;  
    return rounded_to_hex(comp_q, comp_r, comp_s);
}