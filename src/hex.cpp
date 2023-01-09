#include "hex.hpp"
#include <algorithm>

HexCoords operator "" _LU (unsigned long long x) { int v = x; return HexCoords{0, -v, +v}; };
HexCoords operator "" _RU (unsigned long long x) { int v = x; return HexCoords{+v, -v, 0}; };
HexCoords operator "" _R  (unsigned long long x) { int v = x; return HexCoords{+v, 0, -v}; };
HexCoords operator "" _RD (unsigned long long x) { int v = x; return HexCoords{0, +v, -v}; };
HexCoords operator "" _LD (unsigned long long x) { int v = x; return HexCoords{-v, +v, 0}; };
HexCoords operator "" _L  (unsigned long long x) { int v = x; return HexCoords{-v, 0, +v}; };

HexCoords hexLU (int v) { return HexCoords{0, -v, +v}; };
HexCoords hexRU (int v) { return HexCoords{+v, -v, 0}; };
HexCoords hexR  (int v) { return HexCoords{+v, 0, -v}; };
HexCoords hexRD (int v) { return HexCoords{0, +v, -v}; };
HexCoords hexLD (int v) { return HexCoords{-v, +v, 0}; };
HexCoords hexL  (int v) { return HexCoords{-v, 0, +v}; };

HexCoords HexCoords::from_axial(int q, int r) {
    return HexCoords{q, r, -q-r};
}

HexCoords HexCoords::from_offset(int col, int row) {
    const auto q = col - (row - (row&1)) / 2;
    const auto r = row;
    return from_axial(q, r);
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

std::vector<HexCoords> HexCoords::ring_around(int range) const {
    if (range == 0) return {};    
    std::vector<HexCoords> result;
    result.reserve(range * 6);
    for(int i = 0; i < range; i++) {
        result.push_back(*this + hexLD(range) + hexR(i));
        result.push_back(*this + hexRD(range) + hexRU(i));
        result.push_back(*this + hexR(range) + hexLU(i));
        result.push_back(*this + hexRU(range) + hexL(i));
        result.push_back(*this + hexLU(range) + hexLD(i));
        result.push_back(*this + hexL(range) + hexRD(i));
    }
    return result;
}

std::vector<HexCoords> HexCoords::spiral_around(int range) const {
    if (range == 0) return {};    
    std::vector<HexCoords> result;
    result.reserve(1 + 3 * range * (range-1));
    result.push_back(*this);
    for(int r = 1; r <= range; r++) {
        for(int i = 0; i < range; i++) {
            result.push_back(*this + hexLD(r) + hexR(i));
            result.push_back(*this + hexRD(r) + hexRU(i));
            result.push_back(*this + hexR(r) + hexLU(i));
            result.push_back(*this + hexRU(r) + hexL(i));
            result.push_back(*this + hexLU(r) + hexLD(i));
            result.push_back(*this + hexL(r) + hexRD(i));
        }
    } 
    return result;
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

//to be removed in future
struct PFHexCoords: HexCoords{
    int travel_cost;
    int estimated_cost;
};

int getPathCost(const HexCoords from, const HexCoords to){
    return std::max(abs(from.r - to.r), std::max(abs(from.q - to.q), abs(from.s - to.s)));
}

std::pair<unsigned int, std::vector<HexCoords>>
HexCoords::make_line(const HexCoords from, const HexCoords to, unsigned int max_cost) {
    std::vector<PFHexCoords> open;
    std::vector<PFHexCoords> closed;
    auto current = PFHexCoords{from, 0, getPathCost(from, to)};
    open.push_back(current);

    std::optional<PFHexCoords> end = {};

    while (!open.empty())
    {
        std::sort(open.begin(), open.end(), [](PFHexCoords &a, PFHexCoords &b){
            if(a.estimated_cost + a.travel_cost == b.estimated_cost + b.travel_cost){
                return a.travel_cost > b.travel_cost;
            }
            return a.estimated_cost + a.travel_cost < b.estimated_cost + b.travel_cost;
        });
        current = open.at(0);
        open.erase(open.begin());

        if(current.r == to.r && current.q == to.q && current.s == to.s){
            end = current;
            break;
        }

        closed.push_back(current);

        int travel_cost = current.travel_cost + 1;

        for (const auto &item: current.neighbours()){
            auto neighbour = PFHexCoords{item, 0, 0};

            if (std::find_if(closed.begin(), closed.end(),[&neighbour](PFHexCoords &item){
               return item.r == neighbour.r && item.q == neighbour.q && item.s == neighbour.s;
            }) != closed.end())
            {
                continue;
            }

            if (std::find_if(open.begin(), open.end(),[&neighbour](PFHexCoords &item){
                return item.r == neighbour.r && item.q == neighbour.q && item.s == neighbour.s;
            }) == open.end())
            {
                neighbour.travel_cost = travel_cost;
                neighbour.estimated_cost = getPathCost(neighbour, to);
                if(neighbour.travel_cost <= max_cost) {
                    open.push_back(neighbour);
                }
            }
            else if (neighbour.travel_cost + neighbour.estimated_cost > travel_cost + neighbour.estimated_cost)
            {
                neighbour.travel_cost = travel_cost;
            }
        }
    }

    std::vector<HexCoords> path;
    if (end.has_value())
    {
        current = end.value();
        path.push_back(current);

        for (int i = end->travel_cost - 1; i >= 0; i--)
        {
            current = *std::find_if(closed.begin(), closed.end(),[&](PFHexCoords &item){
                return item.travel_cost == i && std::find_if(current.neighbours().begin(), current.neighbours().end(),[&item](HexCoords &neighbour){
                    return item.r == neighbour.r && item.q == neighbour.q && item.s == neighbour.s;
                }) != current.neighbours().end();
            });
            path.push_back(current);
        }

        std::reverse(path.begin(), path.end());
    }

    return std::make_pair(0, path);

}